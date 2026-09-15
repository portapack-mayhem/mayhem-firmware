/*
 * Copyright (C) 2026 Dmytro Onyshko
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

#ifndef __UI_FT8_RX_H__
#define __UI_FT8_RX_H__

#include "app_settings.hpp"
#include "message.hpp"
#include "radio_state.hpp"
#include "receiver_model.hpp"
#include "ui.hpp"
#include "ui_freq_field.hpp"
#include "ui_receiver.hpp"
#include "ui_rssi.hpp"
#include "ui_widget.hpp"

namespace ui::external_app::ft8_rx {

class FT8RxView : public View {
   public:
    FT8RxView(NavigationView& nav);
    ~FT8RxView();

    void focus() override;
    std::string title() const override { return "FT8 RX"; }

   private:
    /* The decoder needs the whole 2.5 kHz FT8 sub-band, which is why the receiver runs at
     * a wide sampling rate and the baseband narrows it rather than the radio. */
    static constexpr uint32_t sampling_rate = 3072000;
    static constexpr uint32_t baseband_bandwidth = 1750000;
    /* Lowest Costas score a candidate needs before its decode is reported. */
    static constexpr uint8_t initial_threshold = 10;
    /* Payloads remembered within a slot, so one transmission decoded from several
     * candidates is printed once. */
    static constexpr int recent_max = 20;

    NavigationView& nav_;

    RxRadioState radio_state_{
        7074000 /* 40 m FT8 calling frequency */,
        baseband_bandwidth,
        sampling_rate};

    app_settings::SettingsManager settings_{
        "rx_ft8",
        app_settings::Mode::RX};

    void on_packet(const FT8PacketMessage* message);
    void on_status(const FT8RxStatusMessage* message);

    RFAmpField field_rf_amp{
        {11 * 8, 0}};

    LNAGainField field_lna{
        {13 * 8, 0}};

    VGAGainField field_vga{
        {16 * 8, 0}};

    RSSI rssi{
        {19 * 8 - 4, 3, 52, 8}};

    NumberField field_threshold{
        {UI_POS_X_RIGHT(5), 0},
        2,
        {10, 99},
        1,
        ' '};

    AudioVolumeField field_volume{
        {UI_POS_X_RIGHT(2), 0}};

    RxFrequencyField field_frequency{
        {0, 0},
        nav_};

    Text text_status{
        {0, 1 * 16, 240, 16},
        "Searching for slot"};

    Console console{
        {0, 2 * 16, 240, screen_height - 2 * 16}};

    uint8_t recent_payloads[recent_max][FT8PacketMessage::payload_length]{};
    int recent_count{0};
    int recent_index{0};
    uint32_t decodes_total{0};

    MessageHandlerRegistration message_handler_packet{
        Message::ID::FT8Packet,
        [this](const Message* const p) {
            this->on_packet(static_cast<const FT8PacketMessage*>(p));
        }};

    MessageHandlerRegistration message_handler_status{
        Message::ID::FT8RxStatus,
        [this](const Message* const p) {
            this->on_status(static_cast<const FT8RxStatusMessage*>(p));
        }};
};

}  // namespace ui::external_app::ft8_rx

#endif  // __UI_FT8_RX_H__
