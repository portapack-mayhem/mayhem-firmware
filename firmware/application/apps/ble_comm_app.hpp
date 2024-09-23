/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2017 Furrtek
 * Copyright (C) 2023 TJ Baginski
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

#ifndef __BLE_COMM_APP_H__
#define __BLE_COMM_APP_H__

#include "ble_rx_app.hpp"
#include "ble_tx_app.hpp"

#include "ui.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "ui_transmitter.hpp"
#include "ui_freq_field.hpp"
#include "ui_record_view.hpp"
#include "app_settings.hpp"
#include "radio_state.hpp"
#include "log_file.hpp"
#include "utility.hpp"

#include "recent_entries.hpp"

class BLECommLogger {
   public:
    Optional<File::Error> append(const std::filesystem::path& filename) {
        return log_file.append(filename);
    }

    void log_raw_data(const std::string& data);

   private:
    LogFile log_file{};
};

namespace ui {
class BLECommView : public View {
   public:
    BLECommView(NavigationView& nav);
    ~BLECommView();

    void set_parent_rect(const Rect new_parent_rect) override;
    void paint(Painter&) override{};

    void focus() override;

    std::string title() const override { return "BLE Comm"; };

   private:
    BLETxPacket build_adv_packet();
    void switch_rx_tx(bool inRxMode);
    void startTx(BLETxPacket packetToSend);
    void stopTx();
    void toggle();
    bool in_tx_mode() const;
    void on_timer();
    void on_data(BlePacketData* packetData);
    void on_tx_progress(const bool done);
    void parse_received_packet(const BlePacketData* packet, ADV_PDU_TYPE pdu_type);
    void sendAdvertisement(bool enable);

    NavigationView& nav_;

    RxRadioState radio_state_rx_{
        2'402'000'000 /* frequency */,
        4'000'000 /* bandwidth */,
        4'000'000 /* sampling rate */,
        ReceiverModel::Mode::WidebandFMAudio};

    TxRadioState radio_state_tx_{
        2'402'000'000 /* frequency */,
        4'000'000 /* bandwidth */,
        4'000'000 /* sampling rate */
    };

    app_settings::SettingsManager settings_{
        "BLE Comm Tx", app_settings::Mode::RX_TX};

    uint8_t console_color{0};
    uint32_t prev_value{0};

    uint8_t channel_number_tx = 37;
    uint8_t channel_number_rx = 37;
    bool auto_channel = false;

    char randomMac[13] = "010203040506";

    bool is_running_tx = false;
    bool is_sending = false;
    bool is_adv = false;

    int16_t timer_period{6};  // Delay each packet by 16ms.
    int16_t timer_counter = 0;
    int16_t timer_rx_counter = 0;
    int16_t timer_rx_period{12};  // Poll Rx for at least 200ms. (TBD)

    uint32_t packet_counter{0};
    BLETxPacket advertisePacket{};
    BLETxPacket currentPacket{};
    BleRecentEntry receivedPacket{};

    static constexpr auto header_height = 5 * 16;

    OptionsField options_channel{
        {0 * 8, 0 * 8},
        5,
        {{"Ch.37 ", 37},
         {"Ch.38", 38},
         {"Ch.39", 39},
         {"Auto", 40}}};

    RxFrequencyField field_frequency{
        {6 * 8, 0 * 16},
        nav_};

    RFAmpField field_rf_amp{
        {16 * 8, 0 * 16}};

    LNAGainField field_lna{
        {18 * 8, 0 * 16}};

    VGAGainField field_vga{
        {21 * 8, 0 * 16}};

    RSSI rssi{
        {24 * 8, 0, 6 * 8, 4}};

    Channel channel{
        {24 * 8, 5, 6 * 8, 4}};

    Labels label_send_adv{
        {{0 * 8, 2 * 8}, "Send Advertisement:", Theme::getInstance()->fg_light->foreground}};

    ImageButton button_send_adv{
        {21 * 8, 1 * 16, 10 * 8, 2 * 16},
        &bitmap_play,
        Theme::getInstance()->fg_green->foreground,
        Theme::getInstance()->fg_green->background};

    Checkbox check_log{
        {24 * 8, 2 * 8},
        3,
        "Log",
        true};

    Labels label_packets_sent{
        {{0 * 8, 4 * 8}, "Packets Left:", Theme::getInstance()->fg_light->foreground}};

    Text text_packets_sent{
        {13 * 8, 2 * 16, 12 * 8, 16},
        "-"};

    Console console{
        {0, 4 * 16, 240, 240}};

    std::string str_log{""};
    bool logging{false};

    std::unique_ptr<BLECommLogger> logger{};

    MessageHandlerRegistration message_handler_packet{
        Message::ID::BlePacket,
        [this](Message* const p) {
            const auto message = static_cast<const BLEPacketMessage*>(p);
            this->on_data(message->packet);
        }};

    MessageHandlerRegistration message_handler_tx_progress{
        Message::ID::TXProgress,
        [this](const Message* const p) {
            const auto message = *reinterpret_cast<const TXProgressMessage*>(p);
            this->on_tx_progress(message.done);
        }};

    MessageHandlerRegistration message_handler_frame_sync{
        Message::ID::DisplayFrameSync,
        [this](const Message* const) {
            this->on_timer();
        }};
};

} /* namespace ui */

#endif /*__UI_AFSK_RX_H__*/
