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

#include "ui_wspr_rx.hpp"

#include "audio.hpp"
#include "baseband_api.hpp"
#include "file_path.hpp"
#include "rtc_time.hpp"
#include "string_format.hpp"

#include <cstring>

using namespace portapack;

namespace ui::external_app::wspr_rx {

/* UI liveness breadcrumbs (see ui_wspr_rx.hpp). Plain volatile 32-bit words,
 * single-writer (UI thread), read by a debugger or a future diagnostic dump.
 * They cost 12 bytes of .bss and a few stores per message - no thread, no
 * buffer, no lock. */
volatile uint32_t wspr_ui_dispatch_seq = 0;
volatile uint32_t wspr_ui_last_handler = 0;
volatile uint32_t wspr_ui_tick_count = 0;

/* Candidate panel text style: dark yellow for the transitional "..."
 * candidate (the ONLY state the panel shows from the panel). Static const -
 * Text::set_style takes a pointer, applied once in the constructor. */
static const Style wspr_cand_style_candidate{
    font::fixed_8x16, Color::black(), Color::dark_yellow()};

/* Breadcrumb at handler entry: seq advances before any work so a crash or a
 * wedged handler inside on_X still leaves the handler id behind. */
#define WSPR_UI_BREADCRUMB(id)       \
    do {                             \
        wspr_ui_last_handler = (id); \
        wspr_ui_dispatch_seq++;      \
    } while (0)

void WSPRRxView::focus() {
    field_frequency.focus();
}

WSPRRxView::~WSPRRxView() {
#if WSPR_RX_DIAG
    rtc_time::signal_tick_second -= diag_tick_token_;
#endif
    receiver_model.disable();
    audio::output::stop();
    baseband::shutdown();
}

WSPRRxView::WSPRRxView(NavigationView& nav)
    : nav_{nav} {
#if WSPR_RX_DIAG
    // AGE and the M4 STALE detection refresh once a second on the UI timer.
    diag_tick_token_ = rtc_time::signal_tick_second += [this]() {
        this->on_tick_second();
    };
#endif
    add_children({&field_rf_amp,
                  &field_lna,
                  &field_vga,
                  &rssi,
                  &channel,
                  &field_volume,
                  &field_frequency,
                  &options_band,
                  &button_restart,
                  &button_clear_list,
#if WSPR_RX_DIAG
                  &text_diag_op,
                  &text_candidate,
                  &text_diag_last,
#endif
                  &text_header,
                  &station_list});

    // The header row is generated from the list's own column constants
    // plus the visible-range summary (v1.4 form), so the CALL / N / GRID
    // headings always line up with the rendered data.
    // Candidate panel: dark yellow (transitional "..." candidate), applied
    // once - the panel has no other state any more .
    text_candidate.set_style(&wspr_cand_style_candidate);

    channel.set_overload_threshold(-3);

    // RX RESTART: re-start the capture session on the baseband (producer
    // resets its state between buffers; the decode thread resets its scan
    // session + accumulator). Station table and N counts are preserved -
    // only the capture/timer restarts. Guarded while in flight: the button
    // reads WAIT and drops extra presses until the baseband answers with
    // fresh telemetry/status.
    button_restart.on_select = [this](Button&) {
        if (restart_pending_) return;
        restart_pending_ = true;
        button_restart.set_text("WAIT");
        // Candidate panel: drop the pending observation. The candidate
        // counter belongs to the same session the M4 resets, so zero it too,
        // and re-baseline the win-line interval deltas (the M4 counters will
        // restart from zero; IntervalDelta would also catch the backwards
        // step, this makes the intent explicit).
        cand_state_.clear();
        cand_attempts_ = 0;
#if WSPR_SD_LOGGING
        delta_sel_.reset();
        delta_stage_.reset();
        delta_rep_.reset();
#endif
        baseband::request_wspr_restart();
    };

    button_clear_list.on_select = [this](Button&) {
        // CLEAR LIST: the station table, its counters and the UI
        // confirmation/session display state go to zero; the SD logs keep
        // every line already written, and capture/diagnostics are untouched
        // (no fake WAIT M4). The next REPORT appears as a fresh record.
        station_table_.reset();
        confirm_table_.reset();
        last_decode_s_ = 0;
        ever_decoded_ = false;
        cand_state_.clear();
        station_list.notify_data_changed();
        update_summary_line();
    };

    // Focus hand-off from the list: Left returns to the upper controls. The
    // list itself would otherwise trap Left (it spans the full width).
    station_list.on_left = [this]() {
        button_clear_list.focus();
    };

    // Wire the list to the table BEFORE any REPORT can arrive: without the
    // model the widget paints an empty list no matter what the decoder finds.
    station_list.set_model(&station_table_);
    // No visibility threshold: the model shows every accepted REPORT. A
    // stale saved "min_agr" setting is deliberately not bound anywhere.
    station_list.notify_data_changed();
    update_summary_line();

    field_frequency.set_step(100);

    /* Going through the frequency field rather than the model keeps the two in step:
     * the field retunes the receiver itself and redraws with the new dial. */
    options_band.on_change = [this](size_t, OptionsField::value_t v) {
        if (v != 0)
            field_frequency.set_value(v);
    };
    field_frequency.updated = [this](rf::Frequency f) {
        options_band.set_by_value(f);
    };
    options_band.set_by_value(receiver_model.target_frequency());

    // A previous spectrum view must not leave the LO on the dial: the
    // WSPR decimator always removes the fs/4 offset. Capture mode selects
    // that RF policy without sending AM/FM configuration to this processor.
    receiver_model.set_modulation(ReceiverModel::Mode::Capture);
    receiver_model.set_hidden_offset(0);
    baseband::run_image(portapack::spi_flash::image_tag_wspr_rx);

    // Capture the full WSPR sub-band; the M4 channel filter narrows it.
    receiver_model.set_sampling_rate(sampling_rate);
    receiver_model.set_baseband_bandwidth(baseband_bandwidth);
    receiver_model.enable();

    audio::output::start();

#if WSPR_SD_LOGGING
    /* Diagnostic builds only: the ordinary build defines no file objects and
     * performs no SD access whatsoever - existing logs are left untouched. */
    log_file_.append(logs_dir / u"WSPR.TXT");

    /* Debug log opened once like WSPR.TXT; if it fails (no SD card), the flag
     * keeps the per-window writes from hitting a never-opened file. */
    debug_log_ok_ = !debug_log_file_.append(logs_dir / u"WSPR_DEBUG.TXT");
#endif
}

void WSPRRxView::on_packet(const WSPRPacketMessage* message) {
    WSPR_UI_BREADCRUMB((uint32_t)Message::ID::WSPRPacket);
    if (message->callsign[0] == '\0')
        return;

    // A single decode is a candidate, including a high-agreement completion.
    // Only the same message repeated in a later window becomes a station
    // report. The confirmation table runs on monotonic wall-clock seconds:
    // candidate jobs and status heartbeats have no influence on it.
    const uint32_t now_s = chTimeNow() / CH_FREQUENCY;
    last_decode_s_ = now_s;
    ever_decoded_ = true;
    // The packet's physical TX identity (absolute start block-row + tone-0
    // column) goes with the report: the table stores it per slot, so a
    // re-delivery of the SAME transmission cannot confirm by itself, and
    // neither can a same-onset column-separated re-decode of one packet -
    // confirmation still requires a physically independent transmission
    // (a new onset), matching the station table's policy.
    const bool confirmed = confirm_table_.observe(message->callsign, message->grid,
                                                  message->dbm, message->freq_hz,
                                                  now_s, message->start_br, message->col);
#if WSPR_RX_DIAG
    // Feeds the LAST line of the window this decode belongs to; the flag is
    // consumed by the next window analysis, so a same-packet repeat of an
    // already-confirmed decode cannot simulate a new confirmation.
    if (confirmed) diag_state_.on_confirmed_decode();
#endif
#if WSPR_SD_LOGGING
    if (!confirmed && debug_log_ok_) {
        rtc::RTC datetime;
        rtcGetTime(&RTCD1, &datetime);
        const std::string entry = std::string{"unconf "} + message->callsign + " " +
                                  message->grid + " " +
                                  to_string_dec_int(message->dbm) + " " +
                                  to_string_dec_int(message->snr_db) + " " +
                                  to_string_dec_uint(message->freq_hz) +
                                  " a=" + to_string_dec_int(message->agreement_pct);
        debug_log_file_.write_entry(datetime, entry);
    }
#endif

    // The station table counts UNIQUE REPORTs (its own TX-identity dedup,
    // same tolerances and the same payload-only confirmation identity as
    // the confirm table above - one policy for the log and the screen) and
    // updates the latest fields in place. The selected callsign is passed
    // so eviction can never remove the row the user is looking at. Every
    // accepted report is visible, and the session confirmation flag is
    // persistent: silence never greys a confirmed payload, only a changed
    // payload re-bases it.
    station_table_.report(message->callsign, message->grid,
                          message->dbm, message->freq_hz,
                          message->snr_db, message->agreement_pct,
                          now_s, message->start_br, message->col,
                          station_list.selected_callsign());
    station_list.notify_data_changed();
    update_summary_line();

    // An accepted REPORT answers any live candidate .
    cand_state_.clear();
    diag_reports_++;

#if WSPR_SD_LOGGING
    if (!confirmed) return;

    /* One line per report: callsign, grid, the station's own dBm, the decoder's rough
     * SNR and the audio frequency, prefixed by the logger with the RTC timestamp — the
     * fields wsprnet.org reports are built from, minus the upload. Diagnostic
     * builds only; the ordinary build performs no SD writes at all. */
    rtc::RTC datetime;
    rtcGetTime(&RTCD1, &datetime);
    const std::string entry = std::string{message->callsign} + " " + message->grid + " " +
                              to_string_dec_int(message->dbm) + " " +
                              to_string_dec_int(message->snr_db) + " " +
                              to_string_dec_uint(message->freq_hz);
    log_file_.write_entry(datetime, entry);
#endif
}

void WSPRRxView::on_status(const WSPRRxStatusMessage* message) {
    WSPR_UI_BREADCRUMB((uint32_t)Message::ID::WSPRRxStatus);

    /* The confirmation table does NOT age here at all: its slot timers run
     * on monotonic seconds inside confirm_table_.observe() (on_packet), so
     * the heartbeat cadence and candidate bursts cannot retire slots. */
    const systime_t now = chTimeNow();
    const bool window_event = debug_fresh_;
    /* Probe anchor of THIS window, valid only when its debug message
     * arrived; used by both the LAST line (below, diag build) and the SD
     * win log line. Heartbeats leave both at -1. */
    int8_t probe_row = -1;
    int8_t probe_score = 0;
    if (window_event) {
        probe_row = last_debug_.probe_row;
        probe_score = last_debug_.probe_score;
    }

#if WSPR_RX_DIAG
    /* The window analysis just completed: the status carries the accepted
     * decode count and the debug message (stashed by on_debug one message
     * earlier) carries this window's sync-probe anchor. A status without a
     * fresh debug message (heartbeat) does NOT count as an analysis: probe
     * -1 is forwarded so a stale score from an older window is never shown,
     * and the LAST verdict stays until a real window analysis replaces it. */
    if (window_event) {
        diag_state_.on_window_analysis(message->decode_count, probe_row, probe_score);
        debug_fresh_ = false;
    }
#endif

    /* The sync score is the only live signal-quality readout: noise windows
     * score a couple of units, a real transmission several times that. It is
     * logged per window; the UI v1 layout has no per-heartbeat status word -
     * the honest staleness verdict lives in the diagnostics row. */
    const std::string score = " s=" + to_string_dec_int(message->sync_score);
    (void)score;

    /* The state name is reused verbatim for the debug log line, so the file
     * reads the same words the screen shows. The same word feeds the STATUS
     * line (what the receiver is doing right now). */
    const char* state_name = "Searching";

    switch (message->state) {
        case WSPRRxStatusMessage::SyncState::Searching:
            state_name = "SCANNING";
            break;
        case WSPRRxStatusMessage::SyncState::Heard:
            state_name = "HEARD";
            break;
        case WSPRRxStatusMessage::SyncState::Syncing:
            state_name = "SYNCING";
            break;
        case WSPRRxStatusMessage::SyncState::Locked:
            /* The baseband keeps state Locked long after the last real
             * decode: green-recent means the last window had decodes, a
             * stale carry degrades to LISTENING (idle after a session) or
             * TRACKING (no decode this session yet). */
            if (ever_decoded_) {
                const uint32_t now_s = now / CH_FREQUENCY;
                state_name = (now_s - last_decode_s_ <= LOCKED_VALID_S)
                                 ? "LOCKED"
                                 : "LISTENING";
            } else {
                state_name = "TRACKING";
            }
            break;
    }
    std::strncpy(status_word_, state_name, sizeof(status_word_) - 1);
    status_word_[sizeof(status_word_) - 1] = '\0';

    /* The diagnostics staleness override stands down: a status arrived. */
    status_last_tick_ = now;
    status_seen_ = true;
    status_stale_shown_ = false;

#if WSPR_SD_LOGGING
    /* Everything below is diagnostic-build-only SD logging (the win line,
     * its rate gate and the candidate trace). The ordinary build performs
     * no SD access here - the telemetry that drives the UI was already
     * processed above. */

    /* One line per status into WSPR_DEBUG.TXT. Each write costs an SD sync
     * (LogFile::write_entry syncs), so when the baseband bursts statuses per
     * candidate the write is rate limited: at most one per
     * DEBUG_MIN_INTERVAL_S unless the state changed or a decode landed -
     * the interesting transitions always get through immediately. The gate
     * is diag::debug_line_due (host-tested). */
    const bool state_changed = std::strncmp(state_name, last_status_name_,
                                            sizeof(last_status_name_)) != 0;
    bool write_line = false;
    if (debug_log_ok_) {
        if (diag::debug_line_due(state_changed, message->decode_count > 0,
                                 (uint32_t)now, (uint32_t)debug_last_write_tick_,
                                 DEBUG_MIN_INTERVAL_S * CH_FREQUENCY)) {
            write_line = true;
            debug_last_write_tick_ = now;  // only advances on an actual write
        }
        std::strncpy(last_status_name_, state_name, sizeof(last_status_name_) - 1);
        last_status_name_[sizeof(last_status_name_) - 1] = '\0';
    }
    if (debug_log_ok_ && write_line) {
        rtc::RTC datetime;
        rtcGetTime(&RTCD1, &datetime);
        /* Same field set and order as before, built in ONE bounded buffer:
         * the previous chained std::string concatenations cost far more M0
         * code (and heap churn) than the snprintf calls they become. */
        /* Worst case (4 candidate slots, full 32-bit counters, 8-digit dial)
         * is ~330 chars; 384 leaves room so the tail fields never truncate. */
        char line[384];
        int n = std::snprintf(
            line, sizeof(line),
            "win %s decodes=%u score=%d cand=%u metric=%d skip=%d", state_name,
            (unsigned)message->decode_count, (int)message->sync_score,
            (unsigned)last_debug_.num_candidates, (int)last_debug_.best_metric,
            (int)last_debug_.last_skip);
        size_t pos = (n > 0 && (size_t)n < sizeof(line)) ? (size_t)n : 0;
        /* The sync-only acquisition probe of an empty window: the anchor row
         * and its sync excess, only when a debug message fresh for this
         * window arrived (probe_row < 0 otherwise). */
        if (probe_row >= 0 && pos < sizeof(line))
            pos += (size_t)std::snprintf(line + pos, sizeof(line) - pos,
                                         " probe=%d/%d", (int)probe_row,
                                         (int)probe_score);
        /* Only slots backed by a result are printed: an unused slot is
         * zero-filled, and a real decode always sits in the 1400-1600 Hz
         * passband, never at frequency zero. Kept on one line so a session
         * reads as one row per window. */
        for (size_t i = 0;
             i < WSPRDebugMessage::max_candidates && pos < sizeof(line); i++) {
            const auto& candidate = last_debug_.candidates[i];
            if (candidate.freq_hz == 0)
                continue;
            pos += (size_t)std::snprintf(
                line + pos, sizeof(line) - pos, " c%u=%d/%d/%d", (unsigned)i,
                (int)candidate.sync_excess, (int)candidate.row_offset,
                (int)candidate.freq_hz);
        }
        if (pos >= sizeof(line) - 1)
            pos = sizeof(line) - 1;
        /* Ring-scan health (VALID_LOSS), settings, and the bounded aggregate
         * diagnostics: sel = candidates released (counted from Decode-phase
         * telemetry), stg = demod/aggregate STAGE ENTRIES (M4; NOT FEC calls
         * and not failures), rep = accepted REPORTs (local). exp=/snap= are
         * the pre-existing scan health counters. Diagnostics only; no
         * decision path reads any of them. */
        /* Stage accounting for THIS interval (deltas since the previous win
         * line, computed per actual write so they always correspond to the
         * logging interval; IntervalDelta re-baselines when a counter was
         * reset, so RX RESTART never produces a bogus wrap delta):
         *   sel/dSel    candidates released by the scan
         *   stg/dStg    candidates that ENTERED the demod/aggregate stage
         *   rep/dRep    accepted REPORTs (M0 packet count)
         *   snapE/snapO snapshot rejected: expired / overwritten
         *   rej         association, dedup, open-floor or report-gate reject
         *   fail        completed path failed metric/agreement/unpack
         *   oth         unclassified (demod failure, codes 0/3/5)
         * A window with sel>0, stg==0 and snapE/snapO>0 lost its candidates
         * to the ring; stg>0 with rej>0 lost them in the decoder. No failure
         * figure is derived by subtracting reports from stage entries.
         * Diagnostics only; no decision path reads any of them. */
        const uint32_t dsel = delta_sel_.update(cand_attempts_);
        const uint32_t dstg = delta_stage_.update(diag_stage_entries_);
        const uint32_t drep = delta_rep_.update(diag_reports_);
        std::snprintf(line + pos, sizeof(line) - pos,
                      " exp=%u snap=%u lag=%u dial=%u lna=%u vga=%u amp=%u",
                      (unsigned)diag_state_.telemetry().scan_expired,
                      (unsigned)diag_state_.telemetry().scan_snap_drops,
                      (unsigned)diag_state_.telemetry().scan_max_lag,
                      (unsigned)receiver_model.target_frequency(),
                      (unsigned)receiver_model.lna(),
                      (unsigned)receiver_model.vga(),
                      (unsigned)receiver_model.rf_amp());
        pos = diag::append_diag_counters(
            line, sizeof(line), pos,
            cand_attempts_, dsel,
            diag_stage_entries_, dstg,
            diag_reports_, drep,
            diag_snap_expired_,
            diag_snap_overwritten_,
            diag_agg_reject_,
            diag_agg_fail_,
            diag_agg_other_);
        const std::string entry{line};
        debug_log_file_.write_entry(datetime, entry);

#if WSPR_CANDIDATE_TRACE
        /* Candidate-level trace: if a FRESH debug message (window_event) this
         * window carried a real candidate outcome (cand_freq_hz != 0), append
         * one "cand " line. Two SD writes happen here (win line + cand line) —
         * that is up to two SD writes per minute (two syncs within the one
         * rate-gated diagnostic event; the debug_line_due gate above fires at
         * most once per DEBUG_MIN_INTERVAL_S in the quiet steady state). For a
         * temporary diagnostic this is acceptable. The sampling picture is
         * visible host-side: sel (in the win line) counts every selected
         * candidate, so the rate-limited "cand " trace is never a complete
         * population. window_event (not cand_freq_hz alone) gates this so a
         * heartbeat status that carries no debug message cannot re-write a
         * stale candidate from a previous window. */
        if (window_event && last_debug_.cand_freq_hz != 0) {
            char cline[32];
            std::snprintf(cline, sizeof(cline), "cand %d,%u",
                          (int)last_debug_.cand_freq_hz,
                          (unsigned)last_debug_.cand_agg);
            debug_log_file_.write_entry(datetime, std::string{cline});
        }
#endif
    }
#endif  // WSPR_SD_LOGGING
}

void WSPRRxView::on_debug(const WSPRDebugMessage* message) {
    WSPR_UI_BREADCRUMB((uint32_t)Message::ID::WSPRDebug);
    /* The rich telemetry rides one message ahead of the status message that writes
     * it; stashing here keeps on_status the single place the debug line is built.
     * debug_fresh_ marks the stash as belonging to THIS window, so a status that
     * arrives without its debug message cannot reuse the previous probe. */
    debug_fresh_ = true;
    last_debug_.num_candidates = message->num_candidates;
    last_debug_.best_metric = message->best_metric;
    last_debug_.last_skip = message->last_skip;
    last_debug_.probe_row = message->probe_row;
    last_debug_.probe_score = message->probe_score;
    for (size_t i = 0; i < WSPRDebugMessage::max_candidates; i++)
        last_debug_.candidates[i] = message->candidates[i];
#if WSPR_CANDIDATE_TRACE
    /* Candidate-level trace: stash the per-candidate FINAL disposition so
     * on_status writes one "cand " line using the same rate-gated write path
     * as the win line (no second write plumbing). cand_freq_hz != 0 = a real
     * admitted candidate; the heartbeat carries 0. */
    last_debug_.cand_freq_hz = message->cand_freq_hz;
    last_debug_.cand_agg = message->cand_agg;
#endif
}

#if WSPR_RX_DIAG
/* Live M4 telemetry: copy the numeric fields into the diagnostic state and
 * refresh the lines. Formatting uses fixed buffers; the widgets are touched
 * only when a rendered value actually changed. */
void WSPRRxView::on_telemetry(const WSPRTelemetryMessage* message) {
    WSPR_UI_BREADCRUMB((uint32_t)Message::ID::WSPRTelemetry);
    diag::TelemetrySnapshot snapshot;
    snapshot.phase = (uint8_t)message->phase;
    snapshot.valid_flags = message->valid_flags;
    snapshot.rows = message->rows;
    snapshot.captured_samples = message->captured_samples;
    snapshot.skip_samples_remaining = message->skip_samples_remaining;
    snapshot.last_decode_ms = message->last_decode_ms;
    snapshot.window_count = message->window_count;
    snapshot.telem_dropped = message->telem_dropped;
    snapshot.scan_scanned = message->scan_scanned;
    snapshot.scan_expired = message->scan_expired;
    snapshot.scan_max_lag = message->scan_max_lag;
    snapshot.scan_snap_drops = message->scan_snap_drops;
    if (message->valid_flags & WSPRTelemetryMessage::VALID_STACK)
        snapshot.decode_stack_free = message->decode_stack_free;
    if (message->valid_flags & WSPRTelemetryMessage::VALID_PROBE) {
        snapshot.probe_row = message->probe_row;
        snapshot.probe_score = message->probe_score;
    }
    diag_state_.on_telemetry(snapshot);
    telem_last_tick_ = chTimeNow();

    // Candidate panel (night spec): the coalesced latest candidate of the
    // 1 Hz telemetry. Arrival time is the monotonic second of THIS message.
    if (message->valid_flags & WSPRTelemetryMessage::VALID_CAND) {
        cand_state_.observe(message->cand_freq_hz,
                            telem_last_tick_ / CH_FREQUENCY);
    }
    // Honest candidate count and the demod/aggregate stage-entry counter
    // come from the M4's single release funnel (message counts cannot be
    // used: the periodic heartbeat also publishes in the Decode phase).
    // VALID_DIAG is bit 8 of the 16-bit flags word; the M4 publishes it from
    // a uint16_t (a truncated word would leave these counters at zero
    // forever - fixed 2026-11-19, guarded by static_asserts on both sides).
    if (message->valid_flags & WSPRTelemetryMessage::VALID_DIAG) {
        cand_attempts_ = message->d_cand_selected;
        diag_stage_entries_ = message->d_stage_entries;
        diag_snap_expired_ = message->d_snap_expired;
        diag_snap_overwritten_ = message->d_snap_overwritten;
        diag_agg_reject_ = message->d_agg_reject;
        diag_agg_fail_ = message->d_agg_fail;
        diag_agg_other_ = message->d_agg_other;
    }

    if (restart_pending_) {
        // Fresh telemetry from the restarted baseband: the request completed.
        restart_pending_ = false;
        button_restart.set_text("RX RESTART");
    }

    /* Decode-phase wall clock for the DECODE elapsed readout. */
    if (snapshot.phase == diag::TelemetrySnapshot::PHASE_DECODE) {
        if (!in_decode_phase_) {
            in_decode_phase_ = true;
            decode_phase_start_ = telem_last_tick_;
        }
    } else {
        in_decode_phase_ = false;
    }

    diag_drops_total_ = diag_state_.telem_dropped_total();
    update_diag_lines();
}

void WSPRRxView::on_tick_second() {
    /* This is the heartbeat, not UI progress: it runs once a second whether
     * or not any message arrived, so a moving tick with a quiet dispatch
     * counter means the event loop is alive and simply waiting. */
    wspr_ui_tick_count++;

    // Row clock: the list renders last-report ages from this monotonic time.
    station_list.set_now(chTimeNow() / CH_FREQUENCY);

    // Candidate panel clock: the UI owns the 150 s TTL expiry (the M4
    // publishes raw observations without a freshness window).
    cand_state_.tick(chTimeNow() / CH_FREQUENCY);

    update_diag_lines();
    update_summary_line();

    /* STATUS-line staleness: the baseband only sends a status alongside a
     * scanned candidate, so a quiet band can leave a stale state word
     * forever. After diag::STATUS_STALE_TIMEOUT_S without a status the word
     * degrades to an explicit NO STATUS + age; a fresh status clears the
     * override in on_status. */
    if (status_seen_ && !status_stale_shown_) {
        const systime_t now = chTimeNow();
        const uint32_t status_age_s = (now - status_last_tick_) / CH_FREQUENCY;
        if (diag::status_is_stale(status_age_s)) {
            status_stale_shown_ = true;
            std::snprintf(status_word_, sizeof(status_word_), "NO STATUS %us",
                          (unsigned)status_age_s);
            update_diag_lines();
        }
    }
}

void WSPRRxView::update_diag_lines() {
    const systime_t now = chTimeNow();
    const uint32_t age_s = (now - telem_last_tick_) / CH_FREQUENCY;

    /* STATUS line: what the receiver is doing (signal word from the last
     * status message) + ring capture progress + the FEC verdict - the honest
     * activity readout the user asked back. Debug counters (AGE/EXP/SNAP/Q/
     * decode ms/stack) are deliberately NOT shown on screen (user decision);
     * the transport-stale colour stays: red M4 STALE, yellow WAIT/REPHASE. */
    diag::format_operation(diag_state_, age_s, status_word_, diag_op_buf_);
    diag::format_last(diag_state_, diag_last_buf_);

    /* Colour differentiation on the STATUS word (user request): green =
     * locked/heard (evidence), yellow = transitional (syncing, transport
     * wait, rephase), red = problems (no status, M4 stale), light = steady
     * scanning/listening/tracking. The word itself always carries the
     * meaning; colour only reinforces it. */
    const bool transport_stale =
        diag_state_.operation(age_s) == diag::Operation::M4Stale;
    if (transport_stale || std::strstr(status_word_, "NO STATUS") != nullptr) {
        text_diag_op.set_style(Theme::getInstance()->fg_red);
    } else if (diag_state_.operation(age_s) == diag::Operation::WaitM4 ||
               diag_state_.operation(age_s) == diag::Operation::Rephase) {
        text_diag_op.set_style(Theme::getInstance()->fg_yellow);
    } else if (std::strstr(status_word_, "LOCKED") == status_word_ ||
               std::strstr(status_word_, "HEARD") == status_word_) {
        text_diag_op.set_style(Theme::getInstance()->fg_green);
    } else if (std::strstr(status_word_, "SYNCING") == status_word_) {
        text_diag_op.set_style(Theme::getInstance()->fg_yellow);
    } else {
        text_diag_op.set_style(Theme::getInstance()->fg_light);
    }

    if (std::strcmp(diag_op_buf_, diag_op_prev_) != 0) {
        std::strcpy(diag_op_prev_, diag_op_buf_);
        text_diag_op.set(diag_op_buf_);
    }
    if (std::strcmp(diag_last_buf_, diag_last_prev_) != 0) {
        std::strcpy(diag_last_prev_, diag_last_buf_);
        text_diag_last.set(diag_last_buf_);
    }

    /* Candidate panel line : the dark_yellow style is applied
     * ONCE in the constructor; an empty NONE line clears the widget via the
     * change-gated Text::set below. */
    diag::format_candidate_panel(cand_state_, chTimeNow() / CH_FREQUENCY,
                                 cand_buf_);
    /* Change-gated like the other lines: no repaint while the panel is empty,
     * one per age second while a candidate is live. */
    if (std::strcmp(cand_buf_, cand_prev_) != 0) {
        std::strcpy(cand_prev_, cand_buf_);
        text_candidate.set(cand_buf_);
    }
}

/* v1.4 reviewed form: the constant CALL/N/GRID header literal and the
 * visible-range summary are two widgets on the same row, so the per-tick
 * path only formats the (short) summary string. */
void WSPRRxView::update_summary_line() {
    char buf[16];
    buf[0] = '\0';
    station_list.format_summary(buf);
    text_summary.set(buf);
}
#endif

}  // namespace ui::external_app::wspr_rx
