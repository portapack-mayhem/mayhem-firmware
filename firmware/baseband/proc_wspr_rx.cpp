/*
 * Copyright (C) 2026 Ivan
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include "proc_wspr_rx.hpp"
#include "portapack_shared_memory.hpp"
#include "audio_dma.hpp"
#include "event_m4.hpp"

#include <cstring>

// The ring scan/snapshot geometry (symbol n at start + 2n, one start per block-row,
// ring depth 352) exists only for the OSR2 producer; an OSR1 build must not compile
// the decode loop against it (the wspr_lib ring functions also reject such a ring at
// runtime - see WSPR_RING_MIN_BLOCK_ROWS).
static_assert(WSPR_RX_TIME_OSR == 2, "ring capture is OSR2-only");

// Dedicated decoder stack, matching the 4 KiB budget used by FT8 RX.
// Keep decoding separate from the small main/event-loop stack; DMA and RSSI
// processing also run on their own thread stacks.
WORKING_AREA(wspr_decode_thread_wa, 4096);

Thread* WSPRDecodeThread::thread_ = nullptr;

// Live producer cursor for the ring reader protocol (wspr_ring_cursor_fn): every
// scan job and every snapshot reads the counter FRESH through this accessor instead
// of trusting a value latched before a long FEC decode. The volatile load plus the
// compiler barrier inside wspr_portapack_total_br is the strongest primitive this
// codebase uses - a single-core M4 ordering argument (context switches preserve
// program order; the compiler cannot reorder bytes across the barrier), NOT a C11
// data-race-freedom claim. See the protocol note in wspr_decode.h.
static uint32_t wspr_ring_live_cursor(void* ctx) {
    return wspr_portapack_total_br(reinterpret_cast<wspr_portapack_state_t*>(ctx));
}

WSPRDecodeThread::WSPRDecodeThread(wspr_portapack_state_t* state, WSPRRxProcessor* processor)
    : state_{state}, processor_{processor} {
    wspr_ring_scan_init(&scan_);
    chBSemInit(&sem_, TRUE);  // start taken
    // Started explicitly by WSPRRxProcessor's constructor AFTER the producer
    // state and the slot accumulator are initialised; a member-order start
    // would let this thread run against an uninitialised ring.
}

WSPRDecodeThread::~WSPRDecodeThread() {
    stop();
}

void WSPRDecodeThread::stop() {
    if (thread_) {
        terminate_.store(true, std::memory_order_release);
        chBSemSignal(&sem_);
        chThdWait(thread_);
        thread_ = nullptr;
    }
}

void WSPRDecodeThread::start() {
    if (!thread_) {
        thread_ = chThdCreateStatic(
            wspr_decode_thread_wa, sizeof(wspr_decode_thread_wa),
            NORMALPRIO - 10,  // Lower than baseband (NORMALPRIO) so DMA processing wins.
            ThreadBase::fn, this);
    }
}

void WSPRDecodeThread::signal_row() {
    // The signal is a WAKEUP, not a per-row token: the loop drains everything the ring
    // scan owes (wspr_ring_scan_poll catches up to total_br) on every wake. Coalescing
    // keeps the semaphore count at 0/1, so a long decode cannot bank hundreds of stale
    // signals.
    bool expected = false;
    if (wake_pending_.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
        chBSemSignal(&sem_);
    }
}

void WSPRDecodeThread::run() {
    // The last known sync score rides on every status message: a quiet band keeps
    // the previous value instead of flickering to zero.
    int16_t last_sync_score = 0;

    // Periodic status heartbeat (no candidate needed). Ticks are milliseconds on
    // this baseband build; MS2ST keeps it correct if that ever changes.
    constexpr systime_t HEARTBEAT_INTERVAL = MS2ST(10000);  // one every 10 s
    systime_t last_heartbeat = chTimeNow();

    // Session this thread's scan/results currently belong to. The producer
    // increments the generation only AFTER it has finished its in-place reset,
    // so adopting a new value here is the decode-thread half of the boundary.
    uint32_t session = processor_->session_generation();
    uint16_t session_ack = 0xFFFF;
    bool session_known = false;
    bool skip_wait = false;

    while (true) {
        if (!skip_wait) chBSemWait(&sem_);
        skip_wait = false;
        if (terminate_.load(std::memory_order_acquire)) break;

        // Boundary: the producer may have completed one or more resets while
        // this thread was busy. Reset everything THIS thread owns (ring scan
        // session and slot accumulator) and publish the ACK
        // status for the new generation. Any candidate computed across the
        // boundary is dropped before publication by the generation checks
        // below - memory safety never depends on that (fixed arrays).
        // session_view() is a CONSISTENT pair, so an old generation can never
        // be published together with a newer ack id.
        const auto view = processor_->session_view();
        if (!session_known || view.generation != session) {
            session = view.generation;
            session_ack = view.ack;
            session_known = true;
            wspr_ring_scan_init(&scan_);
            wspr_decode_reset_aggregation();
            last_sync_score = 0;
            // ACK: this status carries the generation the reset produced and
            // the exact request id the producer serviced. The UI accepts
            // packets only after this message. A failed push is not retried
            // here; the ordinary statuses below repeat the same ack id, so the
            // next candidate or the 10 s heartbeat resolves it.
            (void)shared_memory.application_queue.push(WSPRRxStatusMessage{
                processor_->sync_state_code(), (uint8_t)0, (int16_t)0,
                processor_->ring_rows(), session, session_ack});
        }

        busy_.store(true, std::memory_order_release);

        // Drain until the scan is caught up with the published ring. The cursor is
        // RE-READ before every candidate: a FEC decode can run for seconds, and a
        // stale total_br would let the scan's deadline check pass starts the producer
        // has already rewritten (and hand the snapshot those rows). Reading the
        // counter fresh per iteration keeps both gates honest; rows completed while
        // we worked are picked up either by this drain or by the next signal.
        bool restart = false;
        uint32_t total_br = wspr_portapack_total_br(state_);
        do {
            uint32_t start_br = 0;
            int16_t col = 0;
            int16_t excess = 0;
            int8_t drift = 0;
            while (true) {
                // Session/termination boundary: never start another scan job or
                // keep decoding for a session the producer has already closed.
                if (terminate_.load(std::memory_order_acquire) ||
                    processor_->session_generation() != session) {
                    restart = true;
                    break;
                }
                // FRESH cursor per scan job: a stale total_br could pass starts the
                // producer has already rewritten.
                total_br = wspr_portapack_total_br(state_);
                if (!wspr_ring_scan_poll(wspr_portapack_matrix(),
                                         wspr_portapack_bins(),
                                         WSPR_RX_BLOCK_ROWS,
                                         total_br, &scan_,
                                         WSPR_RING_SCAN_MAX_STARTS_PER_POLL,
                                         wspr_ring_live_cursor, state_,
                                         &start_br, &col, &excess, &drift))
                    break;
                // Freeze the candidate out of the published ring (wrap- and seam-safe),
                // then run the stage-2 snapshot decode on the frozen copy. The ring
                // keeps capturing meanwhile; the snapshot re-checks EVERY row against
                // the LIVE cursor (guarded pre/post-copy) and drops - with a counted
                // reason - any candidate that expired past the overwrite deadline or
                // raced the writer, so no rewritten byte can ever reach the decode.
                wspr_snapshot_t snap;
                if (!wspr_ring_snapshot_candidate(wspr_portapack_matrix(),
                                                  wspr_portapack_bins(),
                                                  WSPR_RX_BLOCK_ROWS,
                                                  total_br, start_br, col,
                                                  wspr_portapack_bin0_hz(), drift,
                                                  excess, &snap,
                                                  wspr_ring_live_cursor, state_,
                                                  nullptr)) {
                    scan_.snap_drops++;
                    continue;
                }

                // The snapshot was taken for `session`. If the producer closed
                // the session while this job was being prepared, the frozen
                // bytes belong to a mixture of the old and new ring and must
                // not be demodulated or published as the new session.
                if (terminate_.load(std::memory_order_acquire) ||
                    processor_->session_generation() != session) {
                    restart = true;
                    break;
                }

                // Fold the snapshot's soft weights into the current track.
                // A tentative single-slot report keeps the track available
                // for the ordinary multislot decode.
                int8_t sym_weight[WSPR_CODED_BITS];
                wspr_decode_result_t decoded;
                int contribution = WSPR_CONTRIBUTION_NONE;
                if (wspr_snapshot_demod_weights(&snap, sym_weight)) {
                    contribution = wspr_agg_track_contribute(&snap, sym_weight, start_br,
                                                              &decoded);
                }

                // Final stale-session check: the (possibly seconds-long) FEC
                // pass above may have straddled the producer's reset. Its
                // result, the dedup anchor and the aggregation contribution all
                // belong to the old session; drop them instead of publishing a
                // mixed-session report. Adoption at the loop top resets the
                // scan session + accumulator.
                if (terminate_.load(std::memory_order_acquire) ||
                    processor_->session_generation() != session) {
                    restart = true;
                    break;
                }

                // One dedup anchor per REPORTED decode: a failed attempt leaves its
                // neighbourhood scannable, so the aligned bank still gets its chance
                // after the smeared twin failed. A reported decode owns the ghost
                // window around its alignment and frequency.
                if (contribution == WSPR_CONTRIBUTION_REPORT ||
                    contribution == WSPR_CONTRIBUTION_ALREADY_REPORTED) {
                    wspr_ring_scan_mark_decoded(&scan_, start_br, col);
                    // An accepted REPORT answers the latest selected candidate.
                    // (reports are counted by the M0 from WSPRPacketMessages.)
                }

                const bool early_report = contribution == WSPR_CONTRIBUTION_EARLY_REPORT;
                const bool has_report = early_report || contribution == WSPR_CONTRIBUTION_REPORT;
                if (has_report) {
                    // Packet identity rides along: the absolute start block-row and the
                    // tone-0 column identify the PHYSICAL transmission, so the UI can
                    // tell a repeat of the same TX (same start_br) from a new one.
                    // Best-effort push: never block the decoder on the UI queue.
                    const bool pushed = shared_memory.application_queue.push(WSPRPacketMessage{
                        decoded.callsign,
                        decoded.grid,
                        decoded.dbm,
                        decoded.snr_db,
                        decoded.freq_hz,
                        decoded.drift_bins,
                        decoded.agreement_pct,
                        start_br,
                        col,
                        session,
                        early_report});
#if defined(WSPR_RX_EARLY_REPORTS)
                    if (pushed && early_report)
                        wspr_agg_ack_early_report();
#endif
                    (void)pushed;
                    last_sync_score = decoded.sync_score;
                }

                const bool status_pushed = shared_memory.application_queue.push(WSPRRxStatusMessage{
                    processor_->sync_state_code(),
                    has_report,
                    last_sync_score,
                    processor_->ring_rows(),
                    session,
                    session_ack});
                (void)status_pushed;
            }
            // Re-read the counter: rows completed while we scanned/decoded.
            total_br = wspr_portapack_total_br(state_);
        } while (!restart &&
                 (wake_pending_.exchange(false, std::memory_order_acq_rel) ||
                  (int32_t)(total_br - wspr_portapack_total_br(state_)) != 0));

        busy_.store(false, std::memory_order_release);

        // A closed session must be adopted before anything else is published.
        // Do not wait for a semaphore here: the producer's signal may already
        // have been consumed while the thread was inside the drain loop.
        if (terminate_.load(std::memory_order_acquire)) break;
        if (restart) {
            skip_wait = true;
            continue;
        }

        // Periodic heartbeat, rate-limited well below the per-row signal rate: a
        // quiet band produces NO candidate jobs, and without this the UI would sit
        // on whatever the last candidate-era status said with no way to tell a
        // silent band from a wedged worker. decode_count is 0 here; the sync state
        // and ring position are current. Never more often than one snapshot per
        // interval, and the pushes never block (a full queue drops, as elsewhere).
        const systime_t now = chTimeNow();
        if ((systime_t)(now - last_heartbeat) >= HEARTBEAT_INTERVAL) {
            last_heartbeat = now;
            // Slot-accumulator aging against the live stream position: the
            // accumulator self-ages on candidate jobs, but a quiet band produces
            // none - the heartbeat keeps idle tracks aging in real SLOT units even
            // when nothing transmits (idempotent: wspr_agg_age_to recomputes ages
            // absolutely from each track's newest contribution).
            wspr_agg_age_to(wspr_portapack_total_br(state_));
            // Heartbeat status snapshot: same fields as the per-attempt status,
            // decode_count 0. Best-effort delivery.
            const bool hb_pushed = shared_memory.application_queue.push(WSPRRxStatusMessage{
                processor_->sync_state_code(),
                (uint8_t)0,
                last_sync_score,
                processor_->ring_rows(),
                session,
                session_ack});
            (void)hb_pushed;
        }
    }
}

WSPRRxProcessor::WSPRRxProcessor() {
    frontend_.configure();

    audio_output.configure(false);

    wspr_portapack_init(&decoder_state);
    // Fresh slot-accumulator tracks per app launch: the multi-slot weight
    // integration must not inherit weights from a previous session's
    // band/frequency.
    wspr_decode_reset_aggregation();

    configured = true;

    decode_thread.start();
    baseband_thread.emplace(baseband_fs, this, baseband::Direction::Receive);
}

WSPRRxProcessor::~WSPRRxProcessor() {
    // Stop the producer first, then the worker, and only then release the
    // producer state. The previous order cleared decoder_state while both
    // threads could still touch it. Reset joins the existing producer thread
    // through the BasebandThread destructor.
    baseband_thread.reset();
    decode_thread.stop();
    wspr_portapack_free(&decoder_state);
}

void WSPRRxProcessor::reset_capture_chain() {
    // Capture thread only, at a buffer boundary. Clears the ring, both
    // interleaved Goertzel banks, the coefficient table and every frontend
    // delay line (the T07 USB stage included), so no pre-retune magnitude,
    // weight or FIR history can be presented as part of the new session.
    // The residual FIR startup is a few dozen samples (<1 ms), far below one
    // WSPR symbol. This reset prevents old samples from being MIXED into the
    // new session; it does not by itself prove that the transition cannot
    // influence metrics or false decodes. The transition duration alone is not
    // that proof (T07 wording closure 2; OPEN_ITEMS O11).
    wspr_portapack_init(&decoder_state);
    frontend_.reset();
    decimate_phase_ = false;
}

void WSPRRxProcessor::request_session_restart(uint16_t request_id) {
    // Event thread: record only. execute() performs the reset.
    session_.request(request_id);
    decode_thread.signal_row();
}

void WSPRRxProcessor::execute(const buffer_c8_t& buffer) {
    if (!configured) return;

    // Producer-side session boundary. The event thread only records a request;
    // this thread (the sole writer of decoder_state/matrix and the sole caller
    // of feed_sample) performs the reset before any sample of the new buffer is
    // processed. commit_reset() publishes the new generation only after the
    // reset is complete, so a worker that observes it can safely discard the
    // old session.
    uint16_t request_id = 0;
    if (session_.take_reset(request_id)) {
        reset_capture_chain();
        session_.commit_reset(request_id);
        decode_thread.signal_row();
    }

    const auto frontend_out = frontend_.execute(buffer, dst_buffer, audio_buffer);
    feed_channel_stats(frontend_out.channel);
    auto audio = frontend_out.audio;

    // Decode ahead of the headphone gain stage, so clipping cannot reach the decoder.
    process_wspr_audio(audio);

    for (size_t i = 0; i < audio.count; i++) {
        float sample = audio.p[i] * 2.0f;
        if (sample > 1.0f) sample = 1.0f;
        if (sample < -1.0f) sample = -1.0f;
        audio.p[i] = sample;
    }
    audio_output.write(audio);
}

void WSPRRxProcessor::process_wspr_audio(const buffer_f32_t& audio) {
    // 24 kHz to 12 kHz. Aliasing into the 1400-1600 Hz WSPR band needs content at
    // 10.4-10.6 kHz or 13.4-13.6 kHz at the 24 kHz stage (12 kHz -/+ f). The
    // 11 kHz channel filter is applied at 48 kHz, before its 2:1 decimation, and
    // measures -75.5 dB at 10.6 kHz and -61.5 dB at 13.5 kHz (DFT of the exact
    // taps, see .zcode/wspr_refactor/t07/T07_FRONTEND_MAP.txt); the plain 2:1
    // sample selection therefore aliases nothing significant into the WSPR band.
    // The negative-frequency image is a separate mechanism and is rejected
    // before Re{} by the complex USB stage in wspr_frontend.hpp.
    for (size_t i = 0; i < audio.count; i++) {
        float sample = audio.p[i];

        decimate_phase_ = !decimate_phase_;
        if (!decimate_phase_) continue;

        // Inf passes the NaN check, since Inf == Inf holds.
        if (sample != sample || sample > 1e15f || sample < -1e15f) sample = 0.0f;

        // Continuous capture: every sample that reaches this function feeds
        // the ring; the feed path itself drops none. A completed block-row
        // wakes the scan/decode thread; capture continues regardless of what
        // that thread does with the published history.
        //
        // O4 CORRECTION: "the feed skips no samples" is NOT evidence that the
        // hardware DMA path lost nothing. Input loss upstream of execute()
        // is not observed or counted anywhere in this app; no DMA-miss
        // counter exists here and none was invented. Any claim about lost
        // captures needs device-side T06/T10 instrumentation, not this
        // code-path argument.
        if (wspr_portapack_feed_sample(&decoder_state, sample)) {
            decode_thread.signal_row();
        }

    }
}

WSPRRxStatusMessage::SyncState WSPRRxProcessor::sync_state_code() const {
    // Filling while the ring does not yet hold a full 352 rows; otherwise
    // Decoding while a scan/decode pass is running (NOT FEC success), else
    // Receiving. No Searching/Heard/Syncing/Locked wording: those mixed
    // capture state with a decoder verdict.
    if (wspr_portapack_valid_rows(&decoder_state) < (uint32_t)WSPR_RX_BLOCK_ROWS)
        return WSPRRxStatusMessage::SyncState::Filling;
    return decode_thread.is_busy() ? WSPRRxStatusMessage::SyncState::Decoding
                                   : WSPRRxStatusMessage::SyncState::Receiving;
}

uint16_t WSPRRxProcessor::ring_rows() const {
    // Absolute stream position % 352: the block-row position inside the ring.
    return (uint16_t)(wspr_portapack_total_br(&decoder_state) %
                      (uint32_t)WSPR_RX_BLOCK_ROWS);
}

void WSPRRxProcessor::on_message(const Message* const message) {
    switch (message->id) {
        case Message::ID::CaptureConfig:
            capture_config(*reinterpret_cast<const CaptureConfigMessage*>(message));
            break;

        case Message::ID::WSPRRestart:
            /* RX RESTART / RETUNE, event thread: RECORD the request only. This
             * thread is NOT the capture thread, so it must not touch the
             * matrix, the Goertzel banks or total_br. The capture thread's
             * execute() consumes the request at the next buffer boundary, runs
             * wspr_portapack_init() + decimator reset there, and commits the
             * new generation; the decode thread adopts that generation and
             * publishes WSPRRxStatusMessage::restart_ack. request_id is echoed
             * verbatim, so the UI can prove THIS request completed. */
            request_session_restart(
                reinterpret_cast<const WSPRRestartMessage*>(message)->request_id);
            break;

        default:
            break;
    }
}

void WSPRRxProcessor::capture_config(const CaptureConfigMessage& message) {
    if (message.config) {
        audio_output.set_stream(std::make_unique<StreamInput>(message.config));
    } else {
        audio_output.set_stream(nullptr);
    }
}

int main() {
    audio::dma::init_audio_out();

    EventDispatcher event_dispatcher{std::make_unique<WSPRRxProcessor>()};
    event_dispatcher.run();
    return 0;
}
