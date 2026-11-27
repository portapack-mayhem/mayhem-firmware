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
#include "rtc_time.hpp"

#include <cstring>

#if WSPR_SD_LOGGING
#include "file_path.hpp"
#include "string_format.hpp"
#endif

using namespace portapack;

namespace ui::external_app::wspr_rx {

/* The status channel is considered dead after this long without a status
 * message; the 1 Hz tick then replaces the line with an explicit NO STATUS +
 * age. Two full WSPR windows plus margin. Both builds use it. */
static constexpr uint32_t STATUS_STALE_TIMEOUT_S = 300;

#if WSPR_SD_LOGGING
#if WSPR_RX_DIAG
/* The SD win line is built into THIS file-scope buffer, not a local: 768
 * bytes on the M0 UI stack is a real overflow risk (the UI thread stack is
 * small and the log write already runs inside message dispatch). File scope is
 * safe here because the app has exactly one WSPRRxView and every access is on
 * the UI thread: on_telemetry is a message handler and the log write is
 * synchronous, so there is no concurrent reader or writer. The formatter is
 * bounded, so the buffer size is a capacity contract, not a correctness
 * dependency (T01 worst case is well under it; see format_win_line). */
static char win_line_buf[768];

static_assert(diag::WINLINE_MAX_CANDIDATES == WSPRTelemetryMessage::max_candidates,
              "wspr_diag.hpp mirrors WSPRTelemetryMessage::max_candidates");

/* Write the bounded diagnostic win line from the single telemetry snapshot.
 * Called only for a NEW analysis (analysis_index changed) that passed the
 * debug_line_due rate gate. No status/debug message pairing is involved. */
static void write_win_line(const diag::WinLineFields& fields,
                           LogFile& file) {
    bool truncated = false;
    const size_t line_len =
        diag::format_win_line(win_line_buf, sizeof(win_line_buf), fields, &truncated);
    /* With cap 768 the documented worst case cannot truncate (T01 proof). If
     * the capacity contract is ever lowered, the marker makes the loss
     * explicit in the log rather than presenting a cut-off line as complete;
     * the copy is bounds-checked, never an overflow. */
    if (truncated) {
        static const char trunc_marker[] = " TRUNC";
        if (line_len + sizeof(trunc_marker) <= sizeof(win_line_buf))
            std::memcpy(win_line_buf + line_len, trunc_marker, sizeof(trunc_marker));
    }
    rtc::RTC datetime;
    rtcGetTime(&RTCD1, &datetime);
    file.write_entry(datetime, std::string{win_line_buf});
}
#endif
#endif  // WSPR_SD_LOGGING

void WSPRRxView::focus() {
    field_frequency.focus();
}

WSPRRxView::~WSPRRxView() {
    rtc_time::signal_tick_second -= tick_token_;
    receiver_model.disable();
    audio::output::stop();
    baseband::shutdown();
}

WSPRRxView::WSPRRxView(NavigationView& nav)
    : nav_{nav}
#if WSPR_RX_DIAG && WSPR_SD_LOGGING
      , delta_sel_{}
      , delta_stage_{}
      , delta_rep_{}
      , delta_fec_{}
#endif
{
    // The 1 Hz UI tick runs in BOTH configurations: the
    // last-packet line and the status staleness override all need it.
    tick_token_ = rtc_time::signal_tick_second += [this]() {
        this->on_tick_second();
    };
    status_last_tick_ = chTimeNow();

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
                  &text_status,
                  &header_,
                  &station_list,
                  &text_empty,
                  &text_last_packet});

    /* Standard recent-entries header: the CALL / N / GRID labels are painted
     * by the framework at the same x positions the row renderer uses
     * (0 / 80 / 112 px). Fixed row above the list. */
    header_.set_parent_rect(
        {0, wspr_layout::header_y, wspr_layout::app_width,
         wspr_layout::header_height});

    channel.set_overload_threshold(-3);

    // RX RESTART: start a new capture session on the baseband. The producer
    // resets at its next buffer boundary and the decode thread adopts the new
    // generation; the table and N counts are preserved. Guarded while in
    // flight: the button reads WAIT and drops extra presses until the matching
    // restart_ack status arrives. Only this request id completes this press.
    button_restart.on_select = [this](Button&) {
        if (session_gate_.pending()) return;
        request_session_restart();
    };

    button_clear_list.on_select = [this](Button&) {
        // CLEAR LIST: the station table, its counters and the UI
        // confirmation/session display state go to zero; the SD logs keep
        // every line already written, and capture/diagnostics are untouched
        // (no fake WAIT M4). The next REPORT appears as a fresh record.
        station_table_.reset();
        /* Clear the last-packet line's snapshot too (CLEAR LIST owns it). */
        last_report_valid_ = false;
        last_report_call_[0] = '\0';
        last_report_grid_[0] = '\0';
        /* The model is empty: drop the standard table's selected key so no
         * stale callsign survives (on_focus() runs the table's advance(0),
         * which clears the key for an empty list without stealing focus). */
        station_list.on_focus();
        station_list.set_dirty();
        update_empty_state();
        update_last_packet();
    };

    // Focus hand-off from the list: Left returns to the upper controls. The
    // list itself would otherwise trap Left (it spans the full width).
    station_list.on_left = [this]() {
        button_clear_list.focus();
    };

    /* Standard table over the one station model. No visibility threshold:
     * the model shows every accepted REPORT; a stale saved "min_agr"
     * setting is deliberately not bound anywhere. */
    update_empty_state();
    update_last_packet();

    field_frequency.set_step(100);

    /* Going through the frequency field rather than the model keeps the two in step:
     * the field retunes the receiver itself and redraws with the new dial. */
    options_band.on_change = [this](size_t, OptionsField::value_t v) {
        if (v != 0)
            field_frequency.set_value(v);
    };
    field_frequency.updated = [this](rf::Frequency f) {
        options_band.set_by_value(f);
        /* The dial is already applied to the receiver model (BoundFrequencyField
         * calls set_target_frequency before this callback). Any retune - inside
         * one band or across bands - closes the old capture session: the new
         * session must not inherit the old matrix, accumulated weights or FIR
         * history. */
        request_session_restart();
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
     * keeps the per-analysis writes from hitting a never-opened file. */
    debug_log_ok_ = !debug_log_file_.append(logs_dir / u"WSPR_DEBUG.TXT");
#endif
}

void WSPRRxView::request_session_restart() {
    /* Monotonic id per request. A retune that happens while a request is in
     * flight simply supersedes the expected id: the M4 may coalesce resets, but
     * the ACK always carries the newest id it actually serviced, and only an
     * exact match re-opens packet acceptance. */
    const uint16_t id = ++restart_request_seq_;
    session_gate_.request(id);
    button_restart.set_text("WAIT");
#if WSPR_RX_DIAG
    diag_reports_ = 0;
    analysis_seen_ = false;
    last_analysis_index_ = 0;
#if WSPR_SD_LOGGING
    delta_sel_.reset();
    delta_stage_.reset();
    delta_rep_.reset();
    delta_fec_.reset();
#endif
#endif
    baseband::request_wspr_restart(id);
}

void WSPRRxView::on_packet(const WSPRPacketMessage* message) {
    if (message->callsign[0] == '\0')
        return;

    /* Session gate (T05): while a restart is in flight, reports already queued
     * from the old session are discarded. After the ACK, a report from any
     * other generation is also discarded. This is independent of message
     * adjacency or queue order. */
    if (!session_gate_.packet_allowed(message->session_id))
        return;

    // One decode may be a candidate, an ambiguous re-decode of one packet, a
    // re-delivery, or an independent station report. StationTable is the ONLY
    // authority for that decision: it is fed monotonic wall-clock seconds
    // (candidate jobs and status heartbeats have no influence) plus the
    // packet's physical TX identity (session generation + absolute start
    // block-row + tone-0 column) and the dial it was received on, and returns
    // the explicit outcome consumed below.
    const uint32_t now_s = chTimeNow() / CH_FREQUENCY;
    const stations::ReportOutcome outcome = station_table_.report(
        message->callsign, message->grid, message->dbm, message->freq_hz,
        message->snr_db, message->agreement_pct, now_s, message->start_br,
        message->col, message->session_id,
        (uint32_t)receiver_model.target_frequency(), message->early_report);

    // Duplicate/Ambiguous change no record: no repaint and no freshness
    // refresh. Only an independent TX (Unique*) updates the standard table
    // and the last-packet line.
    if (outcome.result == stations::ReportResult::UniqueConfirmed ||
        outcome.result == stations::ReportResult::UniqueUnconfirmed) {
        station_list.set_dirty();
        update_empty_state();
        /* Snapshot the accepted payload for the last-packet line (duplicates
         * and ambiguous re-decodes never reach this branch). */
        std::strncpy(last_report_call_, message->callsign,
                     sizeof(last_report_call_) - 1);
        last_report_call_[sizeof(last_report_call_) - 1] = '\0';
        std::strncpy(last_report_grid_, message->grid,
                     sizeof(last_report_grid_) - 1);
        last_report_grid_[sizeof(last_report_grid_) - 1] = '\0';
        last_report_dbm_ = message->dbm;
        last_report_valid_ = true;
        update_last_packet();
    }

#if WSPR_RX_DIAG
    /* Win-line report counter: every WSPRPacket message, independent of the
     * model's classification (not the station N). Diagnostics only. */
    diag_reports_++;
#endif

#if WSPR_SD_LOGGING
    if (outcome.result == stations::ReportResult::UniqueConfirmed) {
        /* One line per confirmed report: callsign, grid, the station's own
         * dBm, the decoder's rough SNR and the audio frequency, prefixed by
         * the logger with the RTC timestamp - the fields wsprnet.org reports
         * are built from, minus the upload. Diagnostic builds only; the
         * ordinary build performs no SD writes at all. */
        rtc::RTC datetime;
        rtcGetTime(&RTCD1, &datetime);
        const std::string entry = std::string{message->callsign} + " " + message->grid + " " +
                                  to_string_dec_int(message->dbm) + " " +
                                  to_string_dec_int(message->snr_db) + " " +
                                  to_string_dec_uint(message->freq_hz);
        log_file_.write_entry(datetime, entry);
    } else if (outcome.result == stations::ReportResult::UniqueUnconfirmed &&
               debug_log_ok_) {
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
    /* Duplicate/Ambiguous: no report line (nothing was accepted, so there is
     * nothing new to log). */
#endif
}

void WSPRRxView::on_status(const WSPRRxStatusMessage* message) {
    /* The status is a single self-contained snapshot: state word + ring row.
     * Nothing here needs a diagnostic message (there is no debug message any
     * more) and no pairing by queue adjacency is performed. */
    const char* state_name = "FILLING";
    switch (message->state) {
        case WSPRRxStatusMessage::SyncState::Filling:
            state_name = "FILLING";
            break;
        case WSPRRxStatusMessage::SyncState::Decoding:
            state_name = "DECODING";
            break;
        case WSPRRxStatusMessage::SyncState::Receiving:
            state_name = "RECEIVING";
            break;
    }

#if WSPR_RX_DIAG
    /* Cache the compact word for the diagnostic win line. It is a label only:
     * the win line is emitted from telemetry, never by pairing this message
     * with anything. */
    std::strncpy(win_state_word_, state_name, sizeof(win_state_word_) - 1);
    win_state_word_[sizeof(win_state_word_) - 1] = '\0';
#endif

    /* Compact 30-char STATUS line at y56: "RECEIVING r123/351". */
    std::snprintf(status_buf_, sizeof(status_buf_), "%s r%u/351", state_name,
                  (unsigned)message->ring_rows);
    if (std::strcmp(status_buf_, status_prev_) != 0) {
        std::strcpy(status_prev_, status_buf_);
        text_status.set(status_buf_);
    }
    text_status.set_style(Theme::getInstance()->fg_light);

    /* A status arrived: clear the staleness override. */
    status_last_tick_ = chTimeNow();
    status_stale_shown_ = false;

    /* Session gate: ONLY the status that acknowledges our exact request id
     * proves the restart completed. A status arriving for any other reason
     * (including one from the previous session still in the queue) does not. */
    if (session_gate_.on_status(message->session_id, message->restart_ack)) {
        button_restart.set_text("RX RESTART");
    }
}

#if WSPR_RX_DIAG
/* The single diagnostic snapshot is the ONLY win-line source: no status or
 * debug message is assumed to be adjacent in the queue. When the analysis
 * index changes (the producer increments it once per analysis) and the rate
 * gate allows it, the bounded formatter writes the line. */
void WSPRRxView::on_telemetry(const WSPRTelemetryMessage* message) {
    /* A restart is in flight: the telemetry queued from the previous session
     * has no session id of its own and must not produce a win line for the new
     * session. The ACK status is queued before any new-session telemetry, so
     * skipping while pending loses nothing after the boundary. */
    if (session_gate_.pending()) return;

    diag::WinLineFields& f = win_fields_;

    /* Ring-scan health and cumulative counters travel on every snapshot. */
    f.scan_expired = message->scan_expired;
    f.snap_drops = message->scan_snap_drops;
    f.scan_max_lag = message->scan_max_lag;
    if (message->valid_flags & WSPRTelemetryMessage::VALID_DIAG) {
        f.snap_weak = message->d_snap_weak;
        f.snap_bad_args = message->d_snap_bad_args;
        f.cand_selected = message->d_cand_selected;
        f.stage_entries = message->d_stage_entries;
        f.fec_calls = message->d_fec_calls;
        f.fec_no_path = message->d_fec_no_path;
        f.agg_await = message->d_agg_await;
        f.agg_dedup = message->d_agg_dedup;
        f.agg_open_floor = message->d_agg_open_floor;
        f.agg_reject = message->d_agg_reject;
        f.agg_fail = message->d_agg_fail;
        f.agg_other = message->d_agg_other;
        f.agg_reports = message->d_agg_report;
        f.push_fail_report = message->d_push_fail_report;
        f.push_fail_aux = message->d_push_fail_aux;
    }

    /* Per-analysis observation (VALID_ANALYSIS). A periodic 1 Hz snapshot has
     * no analysis payload; it only refreshes the counters above. */
    if (message->valid_flags & WSPRTelemetryMessage::VALID_ANALYSIS) {
        f.decode_count = message->decode_count;
        f.sync_score = message->sync_score;
        f.num_candidates = message->num_candidates;
        f.best_metric = message->best_metric;
        /* last_skip was removed from the message in T04; the win-line field
         * stays in place for log compatibility and reads 0. */
        f.last_skip = 0;
        f.probe_valid = (message->valid_flags & WSPRTelemetryMessage::VALID_PROBE) &&
                        message->probe_row >= 0;
        f.probe_row = message->probe_row;
        f.probe_score = message->probe_score;
        for (size_t i = 0; i < diag::WINLINE_MAX_CANDIDATES; i++) {
            f.candidates[i].sync_excess = message->candidates[i].sync_excess;
            f.candidates[i].row_offset = message->candidates[i].row_offset;
            f.candidates[i].freq_hz = message->candidates[i].freq_hz;
        }

        const bool new_analysis =
            !analysis_seen_ || message->analysis_index != last_analysis_index_;
        analysis_seen_ = true;
        last_analysis_index_ = message->analysis_index;

#if WSPR_SD_LOGGING
        if (new_analysis && debug_log_ok_) {
            const systime_t now = chTimeNow();
            if (diag::debug_line_due(false, message->decode_count > 0,
                                     (uint32_t)now,
                                     (uint32_t)debug_last_write_tick_,
                                     DEBUG_MIN_INTERVAL_S * CH_FREQUENCY)) {
                debug_last_write_tick_ = now;  // only advances on an actual write

                /* Stage accounting for THIS interval (deltas since the
                 * previous win line, computed per actual write so they always
                 * correspond to the logging interval; IntervalDelta
                 * re-baselines when a counter was reset, so RX RESTART never
                 * produces a bogus wrap delta):
                 *   sel/dSel  candidates released by the scan
                 *   stg/dStg  candidates that ENTERED the demod/aggregate stage
                 *   fec/dFec  real stack_decode() calls (NOT stage entries)
                 *   rep/dRep  accepted REPORTs (M0 packet count) */
                f.d_cand_selected = delta_sel_.update(f.cand_selected);
                f.d_stage_entries = delta_stage_.update(f.stage_entries);
                f.d_reports = delta_rep_.update(diag_reports_);
                f.d_fec_calls = delta_fec_.update(f.fec_calls);
                f.reports = diag_reports_;

                /* Receiver settings at write time. */
                f.dial_hz = (uint32_t)receiver_model.target_frequency();
                f.lna = (uint32_t)receiver_model.lna();
                f.vga = (uint32_t)receiver_model.vga();
                f.amp = (uint32_t)receiver_model.rf_amp();
                f.state_name = win_state_word_;

                write_win_line(f, debug_log_file_);
            }
        }
#else
        (void)new_analysis;
#endif
    }
}
#endif  // WSPR_RX_DIAG

void WSPRRxView::on_tick_second() {
    /* Details line carries the selected record's age, so it is refreshed once
     * a second (change-gated Text::set). The standard table's own ages are
     * rendered by the table. */

    /* STATUS-line staleness: a quiet band produces no status message, so an
     * old word must not stay on screen forever. After
     * STATUS_STALE_TIMEOUT_S without a status the line degrades to an explicit
     * NO STATUS + age; a fresh status clears the override in on_status. */
    if (!status_stale_shown_) {
        const systime_t now = chTimeNow();
        const uint32_t status_age_s = (now - status_last_tick_) / CH_FREQUENCY;
        if (status_age_s > STATUS_STALE_TIMEOUT_S) {
            status_stale_shown_ = true;
            std::snprintf(status_buf_, sizeof(status_buf_), "NO STATUS %us",
                          (unsigned)status_age_s);
            if (std::strcmp(status_buf_, status_prev_) != 0) {
                std::strcpy(status_prev_, status_buf_);
                text_status.set(status_buf_);
            }
            text_status.set_style(Theme::getInstance()->fg_red);
        }
    }
}

/* Last accepted REPORT line: only UniqueUnconfirmed/UniqueConfirmed update
 * the snapshot, so a duplicate/ambiguous decode never refreshes this line.
 * Fixed buffer, change-gated, cleared by CLEAR LIST. */
void WSPRRxView::update_last_packet() {
    if (last_report_valid_) {
        std::snprintf(last_packet_buf_, sizeof(last_packet_buf_),
                      "LAST %s %s %ddBm", last_report_call_, last_report_grid_,
                      (int)last_report_dbm_);
    } else {
        last_packet_buf_[0] = '\0';
    }
    if (std::strcmp(last_packet_buf_, last_packet_prev_) != 0) {
        std::strcpy(last_packet_prev_, last_packet_buf_);
        text_last_packet.set(last_packet_buf_);
    }
}

/* The empty-state Text is hidden as soon as the one model is non-empty; the
 * standard table paints the rows. No per-paint allocation: hidden() only
 * dirties when the flag changes. */
void WSPRRxView::update_empty_state() {
    text_empty.hidden(station_table_.count() != 0);
}

}  // namespace ui::external_app::wspr_rx
