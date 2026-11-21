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

#ifndef __UI_WSPR_RX_H__
#define __UI_WSPR_RX_H__

#include "app_settings.hpp"
#include "message.hpp"
#include "radio_state.hpp"
#include "receiver_model.hpp"
#include "ui.hpp"
#include "ui_channel.hpp"
#include "ui_freq_field.hpp"
#include "ui_receiver.hpp"
#include "ui_rssi.hpp"
#include "ui_widget.hpp"
#include "log_file.hpp"
#include "ui_station_table.hpp"
#include "ui_station_list.hpp"

// WSPR_RX_DIAG: temporary on-air diagnostics build (live M4 telemetry and the
// three diagnostic screen lines). To remove the feature later, delete the
// WSPR_RX_DIAG blocks in this file, ui_wspr_rx.cpp and proc_wspr_rx.{hpp,cpp},
// plus wspr_diag.hpp, the diag host test and the WSPRTelemetry message.
#ifndef WSPR_RX_DIAG
#define WSPR_RX_DIAG 1
#endif

/* WSPR_CANDIDATE_TRACE: candidate-level diagnostic logging. Additive, removable
 * feature: the M4 stamps each candidate's FINAL disposition (start_br, freq_hz,
 * snap_outcome, agg_code) onto the EXISTING per-candidate WSPRDebugMessage
 * (cand_snap != 0xFF marks a real candidate, the heartbeat carries 0xFF); the M0
 * on_debug handler appends one rate-limited "cand " line per candidate to the
 * WSPR_DEBUG.TXT stream. No separate message type or handler. 0 default: no
 * hot-path code. Enabled on BOTH sides; mixed flag configs unsupported. */
#ifndef WSPR_CANDIDATE_TRACE
#define WSPR_CANDIDATE_TRACE 0
#endif

/* WSPR_SD_LOGGING: master compile-time gate for EVERYTHING this app writes
 * to the SD card - LOGS/WSPR.TXT (normal reports), LOGS/WSPR_DEBUG.TXT (win
 * lines, "unconf" lines, candidate traces). 0 (the ordinary build) removes
 * the file objects, the open/append calls, the formatting and every write/
 * sync at compile time: the app CANNOT create or append any log, and
 * existing SD logs are left untouched. 1 (explicit diagnostic build only)
 * restores the full documented logging behaviour. UI telemetry, the station
 * table, confirmation and the candidate panel are unaffected - they never
 * depended on the logs. No CMake option exists; the #ifndef default is the
 * single source of truth (diagnostic builds define it on the compiler
 * command line on BOTH sides that link this header). */
#ifndef WSPR_SD_LOGGING
#define WSPR_SD_LOGGING 0
#endif

#include "wspr_diag.hpp"

namespace ui::external_app::wspr_rx {

/* UI liveness breadcrumbs, file-scope statics in ui_wspr_rx.cpp (12 bytes of
 * .bss, debugger-visible only; no thread, no buffer). They separate the
 * always-running second tick heartbeat from actual message handling:
 *  - dispatch_seq increments on every WSPR message handler ENTRY;
 *  - last_handler is the Message::ID of the handler most recently entered;
 *  - tick_count increments once per second regardless of traffic, so a
 *    waiting (empty) event loop still shows a moving tick: a frozen
 *    tick_count means a dead UI thread, a moving tick with a frozen
 *    dispatch_seq means normal waiting, not a hang. */
extern volatile uint32_t wspr_ui_dispatch_seq;
extern volatile uint32_t wspr_ui_last_handler;
extern volatile uint32_t wspr_ui_tick_count;

class WSPRRxView : public View {
   public:
    WSPRRxView(NavigationView& nav);
    ~WSPRRxView();

    WSPRRxView(const WSPRRxView&) = delete;
    WSPRRxView& operator=(const WSPRRxView&) = delete;

    void focus() override;
    std::string title() const override { return "WSPR RX"; }

    /* NavigationView assigns the real application viewport through this
     * virtual (240x304 at physical y16). Re-layout the children from the
     * actual parent size instead of assuming the raw 320-px screen. */
    void set_parent_rect(const Rect new_parent_rect) override {
        View::set_parent_rect(new_parent_rect);
        apply_layout();
    }

   private:
    /* The decoder needs the whole 1400-1600 Hz WSPR sub-band, which is why the receiver
     * runs at a wide sampling rate and the baseband narrows it rather than the radio. */
    static constexpr uint32_t sampling_rate = 3072000;
    static constexpr uint32_t baseband_bandwidth = 1750000;

    NavigationView& nav_;

    RxRadioState radio_state_{
        7038600 /* 40 m WSPR dial frequency */,
        baseband_bandwidth,
        sampling_rate};

    /* The MIN A% threshold was removed (user feedback): it was a UI filter,
     * not a DSP gate. The settings manager stays so the app keeps its own
     * settings file; the old "min_agr" key is simply no longer bound, so a
     * stale saved value can never filter anything. */
    app_settings::SettingsManager settings_{
        "rx_wspr",
        app_settings::Mode::RX};

    void on_packet(const WSPRPacketMessage* message);
    void on_status(const WSPRRxStatusMessage* message);
    void on_debug(const WSPRDebugMessage* message);

    /* Recomputes header/list/summary/LAST rects from the actual parent
     * viewport (see wspr_layout). Safe before attachment: a zero size falls
     * back to the verified 304-px application viewport. */
    void apply_layout() {
        Dim viewport_h = (Dim)size().height();
        if (viewport_h <= 0 || viewport_h > wspr_layout::app_viewport_height)
            viewport_h = wspr_layout::app_viewport_height;
        /* Only the viewport-dependent bands are re-set; the fixed-position
         * widgets (header, summary, STATUS) keep their static rects. */
        text_candidate.set_parent_rect(
            {0, wspr_layout::cand_y_at(viewport_h), wspr_layout::app_width,
             wspr_layout::cand_height});
        station_list.set_parent_rect(
            {0, wspr_layout::list_y, wspr_layout::app_width,
             wspr_layout::list_height(viewport_h)});
        text_diag_last.set_parent_rect(
            {0, wspr_layout::last_y(viewport_h), wspr_layout::app_width,
             wspr_layout::last_height});
    }

#if WSPR_RX_DIAG
    // ---- UI layout (approved UI v1 spec, diagnostics phase revision) ----
    // The application is NOT the raw 240x320 screen: SystemView reserves the
    // top 16 px for its status strip and gives the pushed view a 240x304
    // parent rect (physical y16..320). All child geometry below therefore
    // lives in app-local coordinates 0..304 (see wspr_layout in
    // ui_station_list.hpp, shared with the host preview and geometry tests).
    // At the real 304-px viewport:
    //   local y  0 radio (+RSSI/channel bars), 16 settings, 32 buttons,
    //           56 STATUS line (signal word + capture progress + FEC),
    //           72 table header (constant CALL/N/GRID literal) with the
    //              visible-range summary on its right end (v1.4 form),
    //           88..248 station list (5 x 32; row count follows the viewport),
    //           256..272 candidate panel line ("... <freq>Hz <age>s"),
    //           272..288 LAST line (kept above the app-local 288 info-band).
    // apply_layout() derives this from the actual parent size; the static
    // initializers below are the same layout for the pre-attach state.
    // STATUS line: the honest activity readout the user asked back - what the
    // receiver is doing right now (SCANNING/SYNCING/LOCKED/LISTENING), the
    // ring capture progress and the FEC busy/idle verdict. Per user feedback
    // the debug counters (AGE/EXP/SNAP/Q/decode ms) are NOT shown on screen.
    // Buttons occupy local y32..56; the STATUS line sits right below them.
    Text text_diag_op{
        {0, 56, screen_width, 16},
        "WAIT M4"};

    // Candidate panel (candidate states only): one 16 px
    // line in the band directly above LAST (the old summary slot; the
    // visible-range summary rides the header row again). Empty when no
    // candidate is live, "... <freq>Hz <age>s" in dark yellow while one is.
    Text text_candidate{
        {0, wspr_layout::cand_y, wspr_layout::app_width,
         wspr_layout::cand_height},
        ""};

    // Table header row above the list: a CONSTANT literal aligned with the
    // rendered columns (callsign @1, N right edge @16, GRID @18). Constant
    // text costs no per-tick formatting code (v1.4 reviewed form); the
    // visible-range summary is a separate small widget on the same row.
    Text text_header{
        {0, wspr_layout::header_y, wspr_layout::header_width,
         wspr_layout::header_height},
        StationListView::HEADER_TEXT};

    // Visible-range summary ("1-5/12"), right end of the header row.
    Text text_summary{
        {wspr_layout::summary_x, wspr_layout::header_y,
         wspr_layout::summary_width, wspr_layout::header_height},
        ""};

    // Bottom line: verdict of the last completed window analysis plus the
    // decode-thread stack highwater in bytes. Bottom-anchored INSIDE the
    // 304-px application viewport, above the app-local 288 info-bar line.
    Text text_diag_last{
        {0, wspr_layout::last_y(wspr_layout::app_viewport_height),
         wspr_layout::app_width, wspr_layout::last_height},
        "LAST: --"};
    diag::DiagState diag_state_{};

    // Candidate panel state (diet round 2: candidate states ONLY - the
    // station list already renders '?' / green for accepted reports). POD
    // struct from wspr_diag.hpp; the UI owns the 150 s TTL expiry from its
    // tick clock and clears it on CLEAR LIST / RX RESTART. Never mutates
    // the station table.
    diag::CandidateState cand_state_{};
    // Render buffers for the candidate line: the widget is only re-set when
    // the formatted text actually changed, so an empty panel (no candidate)
    // costs no repaint at all and a live one repaints once per age second.
    char cand_buf_[diag::LINE_CHARS + 1]{};
    char cand_prev_[diag::LINE_CHARS + 1]{};

    // SD diagnostic counters (contract v3): the win line
    // carries the cumulative counters - sel/stg copied from VALID_DIAG
    // telemetry (stg = demod/aggregate STAGE ENTRIES, not FEC calls and not
    // failures), rep counted LOCALLY in on_packet (no message field). All
    // diagnostics-only, never read by any decision path.
    uint32_t cand_attempts_{0};  // candidates released by the scan
    uint32_t diag_stage_entries_{0};
    uint32_t diag_reports_{0};
    /* Stage counters + the per-interval deltas at the previous win-line
     * write (IntervalDelta: reset-safe re-baselining, see wspr_diag.hpp).
     * The deltas exist only for the diagnostic win line, so diagnostic
     * builds only (WSPR_SD_LOGGING 1); the raw counters stay - on_telemetry
     * updates them unconditionally. */
    uint32_t diag_snap_expired_{0};
    uint32_t diag_snap_overwritten_{0};
    uint32_t diag_agg_reject_{0};
    uint32_t diag_agg_fail_{0};
    uint32_t diag_agg_other_{0};
#if WSPR_SD_LOGGING
    diag::IntervalDelta delta_sel_;
    diag::IntervalDelta delta_stage_;
    diag::IntervalDelta delta_rep_;
#endif

    // Tick of the last ACCEPTED telemetry message, on the M0 monotonic UI
    // timer. AGE is computed against it at render time; if the UI thread
    // itself hangs, AGE freezes with it - a known limitation.
    systime_t telem_last_tick_{0};

    // Decode-phase wall clock: when the telemetry stream first shows the
    // Decode phase, and whether it still does.
    systime_t decode_phase_start_{0};
    bool in_decode_phase_{false};

    // Freshness of the MAIN status line (Searching/Heard/Syncing/Tracking):
    // tick of the last WSPRRxStatusMessage, and the tick of the last status
    // text refresh. The baseband only sends a status with a scanned
    // candidate, so after diag::STATUS_STALE_TIMEOUT_S without one the tick
    // handler replaces the text with an explicit NO STATUS + age readout
    // instead of leaving an old state word on screen forever.
    systime_t status_last_tick_{0};
    bool status_seen_{false};
    bool status_stale_shown_{false};

    // Previous rendered strings, so Text::set (which allocates) only runs on
    // actual changes.
    char diag_op_prev_[diag::LINE_CHARS + 1]{};
    char diag_last_prev_[diag::LINE_CHARS + 1]{};
    char diag_op_buf_[diag::LINE_CHARS + 1]{};
    char diag_last_buf_[diag::LINE_CHARS + 1]{};
    // Signal word for the STATUS line, kept from the last status message
    // (SCANNING/SYNCING/LOCKED/...); rewritten to a NO STATUS verdict by the
    // staleness path when the status channel goes silent.
    char status_word_[24]{"WAIT M4"};

    uint32_t diag_drops_total_{0};
    SignalToken diag_tick_token_{};

    void on_telemetry(const WSPRTelemetryMessage* message);
    void on_tick_second();
    void update_diag_lines();
    /* Composes the header row text: format_header_line + " " + the
     * visible-range summary (;
     * there is no separate summary widget any more). */
    void update_summary_line();
#endif

    // A first decode remains a grey candidate. An identical callsign/grid/power
    // at a nearby frequency in a later window becomes a green station report.
    // Pure logic (diag::ConfirmTable, host-tested); its slot timers run on
    // MONOTONIC SECONDS, never on status/debug/candidate message counts -
    // those arrive per candidate job and per heartbeat, so a message-counted
    // age would retire slots after a handful of unrelated candidates and
    // block the confirmation of a real repeat.
    diag::ConfirmTable confirm_table_{};

    // Wall-clock seconds (chTimeNow/CH_FREQUENCY) of the last accepted
    // decode, for the Locked-idle semantics below.
    uint32_t last_decode_s_{0};
    bool ever_decoded_{false};
    // Green "Locked" is only honest while decodes are recent; after this
    // much idle time the line drops to a yellow "Session <idle>s" (the
    // session had a decode; the carried sync score of the heartbeats is a
    // stale carry, so it is dropped too).
    static constexpr uint32_t LOCKED_VALID_S = 30 * 60;

    RFAmpField field_rf_amp{
        {11 * 8, 0}};

    LNAGainField field_lna{
        {13 * 8, 0}};

    VGAGainField field_vga{
        {16 * 8, 0}};

    /* Live signal-level bars, back per user feedback (they were dropped in the
     * first v1 layout): RSSI above, channel power below, top right of the
     * radio row. No AGC in front of the decoder, and too much gain pins the
     * waterfall bins to the top of the byte scale where the decoder collapses;
     * the channel bar turns red before that happens. */
    RSSI rssi{
        {19 * 8 - 4, 0, 74, 4}};

    Channel channel{
        {19 * 8 - 4, 5, 74, 4}};

    AudioVolumeField field_volume{
        {UI_POS_X_RIGHT(2), 1 * 16}};

    RxFrequencyField field_frequency{
        {0, 0},
        nav_};

    /* The standard WSPR dial frequencies. Picking one retunes the receiver; the frequency
     * field above stays usable for anything else, and simply shows no band once the dial
     * no longer matches an entry. */
    OptionsField options_band{
        {0, 1 * 16},
        5,  // spec: band x0 width 40 px
        /* set_by_value falls back to the first entry when the dial matches nothing, so
         * that entry has to mean "no band" rather than 160 m, or tuning by hand would
         * snap the receiver onto the first preset. */
        {{"---", 0},
         {"160", 1836600},
         {"80m", 3568600},
         {"60m", 5287200},
         {"40m", 7038600},
         {"30m", 10138700},
         {"20m", 14095600},
         {"17m", 18104600},
         {"15m", 21094600},
         {"12m", 24924600},
         {"10m", 28124600},
         {"6m ", 50293000}}};

    /* UI: the text console is replaced by a fixed-capacity station table
     * (one row per callsign, updated in place) and a scrollable list view
     * over it. The list derives its row count from its rect (5 rows in the
     * real 304-px application viewport). Every accepted REPORT is shown;
     * there is no MIN A% control any more (user feedback). */
    stations::StationTable station_table_{};

    StationListView station_list{
        {0, wspr_layout::list_y, wspr_layout::app_width,
         wspr_layout::list_height(wspr_layout::app_viewport_height)}};

    /* RX RESTART (v1.1, functional): sends WSPRRestartMessage to the WSPR
     * baseband. Each side resets only what it owns (producer state on the
     * message thread; scan session + accumulator in the decode thread) - no
     * threads are stopped, the station table and N counts are preserved.
     * While the request is in flight the button reads WAIT and ignores
     * presses; the next telemetry/status re-enables it. */
    Button button_restart{
        {4, 2 * 16, 14 * 8, 24},
        "RX RESTART"};
    bool restart_pending_{false};

    /* The one deliberate button action: CLEAR LIST. Clears the station
     * table, its counters and the UI confirmation/session display state -
     * never the capture, the M4 accumulator or the SD log files. The
     * diagnostic heartbeat/telemetry continue (no fake WAIT M4 after a
     * clear); the next REPORT reappears in the table. */
    Button button_clear_list{
        {15 * 8 + 8, 2 * 16, 14 * 8, 24},
        "CLEAR LIST"};

    uint32_t decodes_total{0};

#if WSPR_SD_LOGGING
    /* Decodes land in LOGS/WSPR.TXT, one line per report, the same logger the
     * AIS and VDL2 receivers use. Appending at construction opens the file once;
     * a missing SD card surfaces as an error return the app ignores.
     * Diagnostic builds only: the ordinary build (WSPR_SD_LOGGING 0) has no
     * file objects at all - nothing can be created, opened or written. */
    LogFile log_file_{};

    /* Temporary debug file in LOGS/WSPR_DEBUG.TXT: one line per 120 s window with the
     * sync state, the decode count, the sync score and the rich per-window telemetry
     * the baseband sends alongside the status (candidate count, best path metric, the
     * tracker's phase correction and the candidate slots), plus one raw line per
     * decode the confirmation gate has not accepted yet, so per-window decoder
     * behaviour and impostor patterns can be read back without a serial console.
     * debug_log_ok_ records whether the file opened; when it did not, every
     * write would just return a FatFs error, so the writes are skipped. */
    LogFile debug_log_file_{};
    bool debug_log_ok_{false};
#endif

#if WSPR_CANDIDATE_TRACE
    /* Candidate-level diagnostic trace, appended to the SAME LOGS/WSPR_DEBUG.TXT
     * handle and the SAME debug_last_write_tick_ rate budget as the win line
     * (no third file handle, no separate tick — a separate LogFile + its open
     * path and write plumbing were the dominant M0 flash cost and are
     * unnecessary for a trace). Each candidate is a single rate-gated "cand "
     * line emitted immediately; the win line and candidate line share the
     * documented <=1/min steady state. */
#endif

    /* The rich telemetry message arrives just before the status message of the same
     * window, so stashing it here lets on_status fold its fields into the debug line
     * it already writes. A plain struct, not the message itself: messages carry a
     * const id and cannot be assigned. */
    struct DebugSnapshot {
        uint8_t num_candidates;
        int16_t best_metric;
        int16_t last_skip;
        int8_t probe_row;
        int8_t probe_score;
        WSPRDebugMessage::Candidate candidates[WSPRDebugMessage::max_candidates];
#if WSPR_CANDIDATE_TRACE
        /* Candidate-level trace outcome of the window whose debug message just
         * arrived; cand_freq_hz != 0 marks a real (admitted) candidate. Stashed
         * here so on_status writes the "cand " line alongside the win line. */
        int16_t cand_freq_hz{0};
        uint8_t cand_agg{0xFF};
#endif
    };
    DebugSnapshot last_debug_{};
    /* Set by on_debug, consumed by the next on_status: the stashed debug
     * snapshot is only forwarded to the diagnostics while it belongs to the
     * window whose status just arrived. A status without a fresh debug
     * message never re-shows the previous window's probe score. */
    bool debug_fresh_{false};

    /* Rate limiting: the baseband now sends a status heartbeat every ~10 s
     * even without candidates, and can also burst one status per scanned
     * candidate, so the SD debug log may not key off the raw message rate.
     * The win log line is written at most every DEBUG_MIN_INTERVAL_S (one
     * per minute of quiet band; every state change and every decode still
     * logs immediately). The gate itself is diag::debug_line_due and is
     * host-tested. The confirmation table has no message-rate exposure
     * at all: its timers run on monotonic seconds inside observe(). */
    static constexpr uint32_t DEBUG_MIN_INTERVAL_S = 60;
#if WSPR_SD_LOGGING
    systime_t debug_last_write_tick_{0};
    char last_status_name_[10]{};  // last state word written to the debug log
#endif

    MessageHandlerRegistration message_handler_packet{
        Message::ID::WSPRPacket,
        [this](const Message* const p) {
            this->on_packet(static_cast<const WSPRPacketMessage*>(p));
        }};

    MessageHandlerRegistration message_handler_debug{
        Message::ID::WSPRDebug,
        [this](const Message* const p) {
            this->on_debug(static_cast<const WSPRDebugMessage*>(p));
        }};

    MessageHandlerRegistration message_handler_status{
        Message::ID::WSPRRxStatus,
        [this](const Message* const p) {
            this->on_status(static_cast<const WSPRRxStatusMessage*>(p));
        }};

#if WSPR_RX_DIAG
    MessageHandlerRegistration message_handler_telemetry{
        Message::ID::WSPRTelemetry,
        [this](const Message* const p) {
            this->on_telemetry(static_cast<const WSPRTelemetryMessage*>(p));
        }};
#endif
};

}  // namespace ui::external_app::wspr_rx

#endif  // __UI_WSPR_RX_H__
