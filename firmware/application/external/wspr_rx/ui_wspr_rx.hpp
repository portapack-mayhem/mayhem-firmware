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
#include "ui_station_table.hpp"
#include "ui_station_list.hpp"
#include "wspr_session.hpp"

namespace ui::external_app::wspr_rx {

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
    /* Requests a new capture session (RX RESTART button or a dial change).
     * Records a fresh request id, stops accepting reports until the matching
     * ACK status arrives, and leaves the actual producer reset to the M4. */
    void request_session_restart();
    /* The 1 Hz tick is registered in BOTH configurations: the status line
     * age, the last-packet display and the status staleness override all need
     * it. It carries no diagnostic state. */
    void on_tick_second();

    /* Recomputes list and last-report rects from the actual parent viewport
     * (see wspr_layout). Safe before attachment: a zero size falls back to
     * the verified 304-px application viewport. The standard header is at a
     * fixed y; everything below the list is bottom-anchored so a shorter
     * viewport can never paint past its parent. */
    void apply_layout() {
        Dim viewport_h = (Dim)size().height();
        if (viewport_h <= 0 || viewport_h > wspr_layout::app_viewport_height)
            viewport_h = wspr_layout::app_viewport_height;

        Dim list_h = wspr_layout::list_height_at(viewport_h);
        if (list_h < 0)
            list_h = 0;
        station_list.set_parent_rect(
            {0, wspr_layout::list_y, wspr_layout::app_width, list_h});
        /* Empty-state message lives in the first list row. */
        text_empty.set_parent_rect(
            {0, wspr_layout::list_y, wspr_layout::app_width,
             wspr_layout::row_height});

        text_last_packet.set_parent_rect(
            {0, wspr_layout::last_y_at(viewport_h), wspr_layout::app_width,
             wspr_layout::last_height});
    }

    // ---- UI layout (approved UI v1 spec, T03 standard-list revision) ----
    // The application is NOT the raw 240x320 screen: SystemView reserves the
    // top 16 px for its status strip and gives the pushed view a 240x304
    // parent rect (physical y16..320). All child geometry below therefore
    // lives in app-local coordinates 0..304 (see wspr_layout in
    // ui_station_list.hpp, shared with the host preview and tests).
    // At the real 304-px viewport:
    //   local y  0 radio (+RSSI/channel bars), 16 settings, 32 buttons,
    //           56 STATUS line (self-contained receiver state + ring row),
    //           72 standard recent-entries header (CALL / N / GRID),
    //           88..288 station list (up to 12 rows, standard table),
    //           288..304 last accepted REPORT line.
    // apply_layout() derives list and last-report from the actual parent size;
    // the static initializers below are the same layout for the pre-attach
    // state.
    //
    // STATUS line (both builds): a compact, self-contained 30-char snapshot
    // of what the receiver is doing right now, rendered straight from the
    // WSPRRxStatusMessage - no debug message needs to exist or arrive next to
    // it. The word comes from the message state (FILLING / DECODING /
    // RECEIVING) and the number is the ring row counter:
    //   "RECEIVING r123/351", "DECODING r200/351", "FILLING r45/351".
    // After STATUS_STALE_TIMEOUT_S without a status it degrades to an
    // explicit "NO STATUS <age>s" so a quiet band cannot leave a stale word
    // on screen forever.
    static constexpr size_t status_line_chars = 30;
    Text text_status{
        {0, wspr_layout::status_y, wspr_layout::app_width,
         wspr_layout::status_height},
        ""};




    /* A first decode remains a grey candidate. An identical callsign/grid/power
     * at a later physical onset becomes a green station report. StationTable is
     * the ONLY authority for that decision: on_packet consumes its explicit
     * station table and never keeps a second copy of its policy. Its timers run on
     * MONOTONIC SECONDS, never on message counts - those arrive per candidate
     * job and per heartbeat, so a message-counted age would retire records
     * after a handful of unrelated candidates and block the confirmation of a
     * real repeat. */

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

    /* UI: one station model (StationTable, T02) displayed by Mayhem's
     * STANDARD recent-entries components (T03). The standard Header + Table
     * draw StationTable::entries() directly; WsprStationTable only adds the
     * Left hand-off and the selected-callsign accessor. The table derives its
     * visible row count from its rect (9 rows of 16 px at the real 304-px
     * application viewport). Every accepted REPORT is shown; there is no MIN
     * A% control any more (user feedback). */
    stations::StationTable station_table_{};

    ui::RecentEntriesColumns columns_ = station_columns();
    ui::RecentEntriesHeader header_{columns_};
    WsprStationTable station_list{station_table_.entries(), columns_};

    /* Empty state: a single line inside the list area, hidden as soon as the
     * model is non-empty. Added/removed by change-gated updates, never by a
     * per-paint allocation. */
    Text text_empty{
        {0, wspr_layout::list_y, wspr_layout::app_width,
         wspr_layout::row_height},
        "No reports yet"};

    /* Last accepted REPORT (UniqueUnconfirmed/UniqueConfirmed only). Separate
     * from capture status. Cleared by CLEAR LIST. */
    Text text_last_packet{
        {0, wspr_layout::last_y, wspr_layout::app_width,
         wspr_layout::last_height},
        ""};

    /* Change gate for the last-report widget. 40 chars is
     * comfortably above the 30-char line width; every formatter is bounded. */
    char last_packet_buf_[40]{};
    char last_packet_prev_[40]{};

    /* Snapshot of the last accepted REPORT for the last-packet line: only an
     * independent TX (UniqueUnconfirmed/UniqueConfirmed) updates it; a
     * duplicate/ambiguous decode never does. Cleared by CLEAR LIST. */
    char last_report_call_[stations::WSPR_CALL_MAX + 1]{};
    char last_report_grid_[5]{};
    int8_t last_report_dbm_{0};
    bool last_report_valid_{false};

    /* Formats the last accepted REPORT line. */
    void update_last_packet();
    /* Shows "No reports yet" only while the model is empty. */
    void update_empty_state();

    /* RX RESTART / retune (v1.2, T05): every request gets a monotonic id and
     * is acknowledged by WSPRRxStatusMessage::restart_ack only after the M4
     * producer has actually reset at a buffer boundary. Until then the UI
     * accepts no WSPRPacket messages (reports already queued from the previous
     * session are discarded deterministically) and the button reads WAIT.
     * The station table and N counts are preserved; session/dial provenance is
     * recorded per station. */
    Button button_restart{
        {4, 2 * 16, 14 * 8, 24},
        "RX RESTART"};
    wspr::session::UiGate session_gate_{};
    uint16_t restart_request_seq_{0};

    /* The one deliberate button action: CLEAR LIST. Clears the station
     * table, its counters and the UI confirmation/session display state -
     * never the capture, the M4 accumulator or the SD log files. The next
     * REPORT reappears in the table. */
    Button button_clear_list{
        {15 * 8 + 8, 2 * 16, 14 * 8, 24},
        "CLEAR LIST"};

    /* Status channel freshness, shared by both builds. The baseband pushes a
     * status after every admitted candidate attempt plus a periodic heartbeat,
     * but a quiet band must not leave an old word on screen forever: after
     * STATUS_STALE_TIMEOUT_S the tick rewrites the line to "NO STATUS <age>s".
     * status_last_tick_ is initialised in the ctor so the override also covers
     * a UI that never received a status at all. */
    systime_t status_last_tick_{0};
    bool status_stale_shown_{false};

    /* Change gate for the STATUS line: a fixed 30-char buffer, bounded
     * formatting, Text::set only when the rendered string changed. */
    char status_buf_[status_line_chars + 1]{};
    char status_prev_[status_line_chars + 1]{};

    /* 1 Hz UI tick token (always registered; see on_tick_second). */
    SignalToken tick_token_{};

    MessageHandlerRegistration message_handler_packet{
        Message::ID::WSPRPacket,
        [this](const Message* const p) {
            this->on_packet(static_cast<const WSPRPacketMessage*>(p));
        }};

    MessageHandlerRegistration message_handler_status{
        Message::ID::WSPRRxStatus,
        [this](const Message* const p) {
            this->on_status(static_cast<const WSPRRxStatusMessage*>(p));
        }};
};

}  // namespace ui::external_app::wspr_rx

#endif  // __UI_WSPR_RX_H__
