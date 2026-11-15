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
#include "dsp_fir_taps.hpp"
#include "dsp_iir_config.hpp"
#include "event_m4.hpp"

#include <cstring>

// 4 KB, the same budget the FT8 decoder thread gets: the WSPR search works on the
// byte-sized magnitude matrix in place and keeps its scratch in statics, so only small
// locals (the result array among them) land on this stack.
WORKING_AREA(wspr_decode_thread_wa, 4096);

Thread* WSPRDecodeThread::thread_ = nullptr;

// Saturate an int16 into the byte-sized debug message slots: real row offsets stay
// 0..14 and sync scores reach only a few dozen, so the clamp only catches values that
// are pathological anyway.
static inline int8_t saturate_i8(const int16_t value) {
    if (value > 127) return 127;
    if (value < -128) return -128;
    return (int8_t)value;
}

WSPRDecodeThread::WSPRDecodeThread(wspr_portapack_state_t* state, WSPRRxProcessor* processor)
    : state_{state}, processor_{processor} {
    chBSemInit(&sem_, TRUE);  // start taken
    start();
}

WSPRDecodeThread::~WSPRDecodeThread() {
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

bool WSPRDecodeThread::signal_decode() {
    // Don't double-trigger if we're still working on the previous window.
    bool expected = false;
    if (!busy_.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
        return false;
    }
    chBSemSignal(&sem_);
    return true;
}

void WSPRDecodeThread::run() {
    // The last known sync score and path metric ride on every status/debug message; a
    // quiet band keeps the previous values instead of flickering to zero.
    int16_t last_sync_score = 0;
    int16_t last_best_metric = 0;
    uint8_t window_index = 0;

    while (true) {
        chBSemWait(&sem_);
        if (terminate_.load(std::memory_order_acquire)) break;

        // Zeroed, so a decoder that fills a result partway cannot leak stack bytes into
        // the message.
        wspr_decode_result_t results[WSPR_MAX_RESULTS]{};
        const int num_results = wspr_decode_window(
            wspr_portapack_matrix(),
            wspr_portapack_rows(),
            wspr_portapack_bins(),
            wspr_portapack_bin0_hz(),
            results,
            WSPR_MAX_RESULTS,
            wspr_portapack_time_osr());

        // Close the tracking loop first: the row offset is only valid for the window
        // that was just decoded, and the correction it computes is consumed when the
        // NEXT window fills (one window of lag, as in FT8). The correction comes back
        // saturated for the debug telemetry below.
        const int16_t last_skip = processor_->on_window_decoded(
            (uint8_t)(num_results > 255 ? 255 : num_results), results);

        for (int i = 0; i < num_results; i++) {
            shared_memory.application_queue.push(WSPRPacketMessage{
                results[i].callsign,
                results[i].grid,
                results[i].dbm,
                results[i].snr_db,
                results[i].freq_hz,
                results[i].drift_bins});
        }

        // One update per window, so the UI can show that the two-minute capture is alive.
        // The sync score rides along only on decoding windows; a quiet band keeps the
        // last known value so the status line does not flicker to zero.
        if (num_results > 0) {
            last_sync_score = results[0].sync_score;
            last_best_metric = results[0].metric;
        }

        // Rich per-window telemetry, pushed just before the status message so the UI
        // writes one debug line carrying both. The decoder offers no candidate count
        // (wspr_decode_window returns only the accepted results), so num_candidates
        // conservatively mirrors num_results until the decoder grows a real output for
        // it. The slots are the byte-sized view of each result; on a quiet window the
        // score and metric hold their last known values.
        WSPRDebugMessage::Candidate slots[WSPRDebugMessage::max_candidates]{};
        for (int i = 0; i < num_results && i < (int)WSPRDebugMessage::max_candidates; i++) {
            slots[i].sync_excess = saturate_i8(results[i].sync_score);
            slots[i].row_offset = saturate_i8(results[i].row_offset);
            slots[i].freq_hz = results[i].freq_hz;
        }
        const WSPRDebugMessage debug{
            window_index,
            (uint8_t)num_results,
            (uint8_t)num_results,
            processor_->sync_state_code(),
            last_sync_score,
            last_best_metric,
            last_skip,
            processor_->tracker_best_score(),
            slots,
            (size_t)(num_results > (int)WSPRDebugMessage::max_candidates
                         ? WSPRDebugMessage::max_candidates
                         : num_results)};
        shared_memory.application_queue.push(debug);

        WSPRRxStatusMessage status{
            (WSPRRxStatusMessage::SyncState)processor_->sync_state_code(),
            (uint8_t)num_results,
            last_sync_score};
        shared_memory.application_queue.push(status);

        // Release the window so the baseband thread can start the next capture.
        wspr_portapack_reset_window(state_);

        busy_.store(false, std::memory_order_release);
        window_index++;
    }
}

WSPRRxProcessor::WSPRRxProcessor() {
    decim_0.configure(taps_11k0_decim_0.taps);
    decim_1.configure(taps_11k0_decim_1.taps);
    channel_filter.configure(taps_11k0_channel.taps, 2);

    audio_output.configure(false);

    wspr_portapack_init(&decoder_state);

    configured = true;
}

WSPRRxProcessor::~WSPRRxProcessor() {
    wspr_portapack_free(&decoder_state);
}

void WSPRRxProcessor::execute(const buffer_c8_t& buffer) {
    if (!configured) return;

    const auto decim_0_out = decim_0.execute(buffer, dst_buffer);
    const auto decim_1_out = decim_1.execute(decim_0_out, dst_buffer);
    const auto channel_out = channel_filter.execute(decim_1_out, dst_buffer);
    feed_channel_stats(channel_out);
    auto audio = demod_ssb.execute(channel_out, audio_buffer);

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
    // 24 kHz to 12 kHz. The 11 kHz channel filter already bandlimits the input, so the
    // decimation aliases nothing back into the 1400-1600 Hz WSPR band.
    static bool decimate_phase = false;

    for (size_t i = 0; i < audio.count; i++) {
        float sample = audio.p[i];

        decimate_phase = !decimate_phase;
        if (!decimate_phase) continue;

        // Tail of the window: the decoder runs here, and any phase correction the
        // tracker asked for is already folded into this count.
        if (skip_samples > 0) {
            skip_samples--;
            continue;
        }

        // Inf passes the NaN check, since Inf == Inf holds.
        if (sample != sample || sample > 1e15f || sample < -1e15f) sample = 0.0f;

        if (wspr_portapack_feed_sample(&decoder_state, sample)) {
            if (decode_thread.signal_decode()) {
                // The window just filled. The correction the PREVIOUS window's decode
                // computed applies from here: normally zero (back-to-back capture), a
                // ~111 s jump when the transmission reached the bottom of the slack, or
                // a dither step after failed windows. The decode runs in the dropped
                // samples that follow.
                skip_samples = (uint32_t)pending_skip.exchange(0, std::memory_order_acq_rel);
            } else {
                // Still decoding the previous window: drop a row at a time until the
                // reset, then the skip above takes over.
                skip_samples = WSPR_SAMPLES_PER_SYMBOL;
            }
        }
    }
}

int16_t WSPRRxProcessor::on_window_decoded(uint8_t num_results, const wspr_decode_result_t* results) {
    int32_t skip = 0;

    if (num_results > 0) {
        locked = true;
        quiet_windows = 0;
        const int16_t row = results[0].row_offset;
        // Sliding out of the bottom of the slack: jump the window phase forward so the
        // transmission lands near TRACK_LAND_ROW. row' = row + (SLOT - OVERHANG - skip)
        // / 8192, so the skip below yields row' = TRACK_LAND_ROW; the dead air is one
        // slot minus the transmission, about 111 s.
        if (row <= 1) {
            skip = SLOT_SAMPLES - WINDOW_OVERHANG -
                   (int32_t)(TRACK_LAND_ROW - row) * WSPR_SAMPLES_PER_SYMBOL;
        }

        // A decode proves the alignment only coarsely: WSPR has no CRC and a strong
        // enough transmission unpacks even with its symbols smeared across two rows,
        // so impostor decodes keep quiet_windows at 0 and the empty-window dither
        // below never runs - the sub-row phase then stays wherever it walked to. Score
        // the phase instead. The best score seen is the reference for where the row
        // grid last sat on the symbol boundaries; a window scoring well below it means
        // the phase has drifted off that spot, so one dither step walks the capture
        // back toward it. A score back within a couple of units of the reference
        // re-arms the reference instead, which stops the walk as soon as the good
        // phase is recovered and lets it follow a slowly changing signal level, so a
        // genuine decline cannot keep the loop stepping forever. The one-value dead
        // band between the two thresholds keeps bin-edge jitter from toggling the
        // branch on consecutive windows.
        // Sub-row chase on decode quality, not on the sync score: the sync excess
        // barely moves with the sub-row phase (band noise dominates it - on air it
        // sat at 6-7 for every window, aligned or not), but the re-encode agreement
        // tracks alignment directly: an aligned window decodes with high agreement,
        // a misaligned one completes into impostors at 50-70%. Walk the phase on
        // weak agreement, hold on strong; the natural overhang drift moves the phase
        // anyway, so the hold is a jitter around the good spot. (The acquisition jump
        // re-lands mid-slack with an arbitrary phase, and this walk has to find the
        // good one from there too.)
        int best_agr = 0;
        for (int i = 0; i < num_results; i++) {
            if (results[i].agreement_pct > best_agr) best_agr = results[i].agreement_pct;
        }
        if (best_agr < WSPR_AGREEMENT_HOLD) {
            skip += DITHER_STEP;
        }
    } else if (locked) {
        if (++quiet_windows < QUIET_LIMIT) {
            // Locked but nothing decoded: the row grid has likely walked off the symbol
            // boundaries, so step the phase a sixth of a row and retry.
            skip = DITHER_STEP;
        } else {
            // Four empty windows in a row: the band went quiet or the alignment is
            // gone either way. Start over; the tracker jumps back to the top of the
            // slack until a decode re-locks it.
            locked = false;
            quiet_windows = 0;
        }
    } else {
        // Unlocked, nothing decoded. The window is longer than the slot, but the sync
        // scan only finds transmissions that start at or after row 0, and the row
        // offset drifts down by the overhang plus the decode overrun every window -
        // once a transmission starts before the window, back-to-back capture never
        // recovers on its own. FT8 sweeps its shorter window through the slot; the
        // WSPR window already spans every position, so the sweep is a periodic jump
        // back to the top of the slack: about 110 s of dead air every third window,
        // after which the transmission re-appears near row TRACK_LAND_ROW.
        if (++acquire_quiet >= ACQUIRE_JUMP_QUIET) {
            acquire_quiet = 0;
            skip = SLOT_SAMPLES - WINDOW_OVERHANG -
                   (int32_t)TRACK_LAND_ROW * WSPR_SAMPLES_PER_SYMBOL;
        }
    }

    pending_skip.store(skip, std::memory_order_release);

    // The debug message wants the correction as an int16; the slot jump is about
    // 1.33M samples, so saturate rather than wrap.
    if (skip > 32767) skip = 32767;
    if (skip < -32768) skip = -32768;
    return (int16_t)skip;
}

uint8_t WSPRRxProcessor::sync_state_code() const {
    if (!locked)
        return (uint8_t)WSPRRxStatusMessage::SyncState::Searching;
    return (uint8_t)(quiet_windows > 0 ? WSPRRxStatusMessage::SyncState::Syncing
                                       : WSPRRxStatusMessage::SyncState::Locked);
}

void WSPRRxProcessor::on_message(const Message* const message) {
    switch (message->id) {
        case Message::ID::CaptureConfig:
            capture_config(*reinterpret_cast<const CaptureConfigMessage*>(message));
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
