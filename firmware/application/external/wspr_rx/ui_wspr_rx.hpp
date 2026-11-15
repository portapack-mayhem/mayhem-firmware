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

namespace ui::external_app::wspr_rx {

class WSPRRxView : public View {
   public:
    WSPRRxView(NavigationView& nav);
    ~WSPRRxView();

    WSPRRxView(const WSPRRxView&) = delete;
    WSPRRxView& operator=(const WSPRRxView&) = delete;

    void focus() override;
    std::string title() const override { return "WSPR RX"; }

   private:
    /* The decoder needs the whole 1400-1600 Hz WSPR sub-band, which is why the receiver
     * runs at a wide sampling rate and the baseband narrows it rather than the radio. */
    static constexpr uint32_t sampling_rate = 3072000;
    static constexpr uint32_t baseband_bandwidth = 1750000;

    NavigationView& nav_;

    RxRadioState radio_state_{
        7078600 /* 40 m WSPR dial frequency */,
        baseband_bandwidth,
        sampling_rate};

    app_settings::SettingsManager settings_{
        "rx_wspr",
        app_settings::Mode::RX};

    void on_packet(const WSPRPacketMessage* message);
    void on_status(const WSPRRxStatusMessage* message);
    void on_debug(const WSPRDebugMessage* message);

    /* WSPR has no CRC: roughly one in seven random payloads unpacks into a
     * structurally valid message, so the decoder hands over a steady drip of
     * impostors, and a decode alone proves little. Real stations transmit the same
     * message every slot for hours, impostors are essentially random, so the
     * two-slot correlation in this table is the acceptance gate: a decode is only
     * believed when the same callsign, grid, dBm and a near-same frequency already
     * came out of an earlier window. A first-time decode is stored and logged raw to
     * the debug file, but stays off the console and WSPR.TXT until a later window
     * repeats it. */
    struct ConfirmSlot {
        char callsign[13];
        char grid[5];
        int8_t dbm;
        int16_t freq_hz;
        uint8_t age;  // windows since this slot was written, 0 = fresh
    };

    static constexpr size_t CONFIRM_SLOTS = 12;
    static constexpr uint8_t CONFIRM_MAX_AGE = 6;  // 6 windows = 12 minutes
    static constexpr int16_t CONFIRM_FREQ_TOLERANCE = 3;

    ConfirmSlot confirm_slots_[CONFIRM_SLOTS]{};  // zeroed = free

    bool confirm_match(const ConfirmSlot& slot, const WSPRPacketMessage* message) const;
    bool confirm_decode(const WSPRPacketMessage* message);
    ConfirmSlot* confirm_stale_slot(const WSPRPacketMessage* message);
    void confirm_store(const WSPRPacketMessage* message);
    void confirm_age_all();

    RFAmpField field_rf_amp{
        {11 * 8, 0}};

    LNAGainField field_lna{
        {13 * 8, 0}};

    VGAGainField field_vga{
        {16 * 8, 0}};

    RSSI rssi{
        {19 * 8 - 4, 0, 74, 4}};

    /* Channel power, in the pair the other receivers draw. There is no AGC in front of
     * the decoder, and too much gain pins waterfall bins to the top of the byte scale,
     * where a decoder that weighs bytes against their neighbours collapses. The overload
     * threshold turns the bar red before that happens. */
    Channel channel{
        {19 * 8 - 4, 5, 74, 4}};

    AudioVolumeField field_volume{
        {UI_POS_X_RIGHT(2), 0}};

    RxFrequencyField field_frequency{
        {0, 0},
        nav_};

    /* The standard WSPR dial frequencies. Picking one retunes the receiver; the frequency
     * field above stays usable for anything else, and simply shows no band once the dial
     * no longer matches an entry. */
    OptionsField options_band{
        {0, 1 * 16},
        3,
        /* set_by_value falls back to the first entry when the dial matches nothing, so
         * that entry has to mean "no band" rather than 160 m, or tuning by hand would
         * snap the receiver onto the first preset. */
        {{"---", 0},
         {"160", 1836600},
         {"80m", 3592600},
         {"60m", 5287200},
         {"40m", 7078600},
         {"30m", 10138700},
         {"20m", 14095600},
         {"17m", 18104600},
         {"15m", 21094600},
         {"12m", 24924600},
         {"10m", 28124600},
         {"6m ", 50293000}}};

    /* State on the left, count on the right with a gap between them: the count sits
     * directly above the decoded-message column, and run together they read as one line. */
    Text text_status{
        {4 * 8, 1 * 16, 16 * 8, 16},
        "Searching"};

    Text text_decodes{
        {20 * 8 + 4, 1 * 16, 7 * 8, 16},
        ""};

    /* NavigationView sits below the 16 px system status bar, so a view's own y = 0 is
     * screen y = 16 and the last usable row is screen_height - 16. Console hands its
     * screen rect straight to the display's hardware scroll region, so a rect that runs
     * past that row sets the region's bottom fixed area negative and the panel then shows
     * frame rows nothing has written. */
    Console console{
        {0, 2 * 16, screen_width, screen_height - 3 * 16}};

    uint32_t decodes_total{0};

    /* Decodes land in LOGS/WSPR.TXT, one line per report, the same logger the
     * AIS and VDL2 receivers use. Appending at construction opens the file once;
     * a missing SD card surfaces as an error return the app ignores. */
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

    /* The rich telemetry message arrives just before the status message of the same
     * window, so stashing it here lets on_status fold its fields into the debug line
     * it already writes. A plain struct, not the message itself: messages carry a
     * const id and cannot be assigned. */
    struct DebugSnapshot {
        uint8_t num_candidates;
        int16_t best_metric;
        int16_t last_skip;
        WSPRDebugMessage::Candidate candidates[WSPRDebugMessage::max_candidates];
    };
    DebugSnapshot last_debug_{};

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
};

}  // namespace ui::external_app::wspr_rx

#endif  // __UI_WSPR_RX_H__
