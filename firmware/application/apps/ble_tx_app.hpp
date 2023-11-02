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

#ifndef __BLE_TX_APP_H__
#define __BLE_TX_APP_H__

#include "ui.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "ui_transmitter.hpp"
#include "ui_freq_field.hpp"
#include "ui_record_view.hpp"
#include "app_settings.hpp"
#include "radio_state.hpp"
#include "replay_thread.hpp"
#include "log_file.hpp"
#include "utility.hpp"

#include "recent_entries.hpp"

#include <string>
#include <memory>

class BLELoggerTx {
   public:
    Optional<File::Error> append(const std::string& filename) {
        return log_file.append(filename);
    }

    void log_raw_data(const std::string& data);

   private:
    LogFile log_file{};
};

namespace ui {

class BLETxView : public View {
   public:
    BLETxView(NavigationView& nav);
    ~BLETxView();

    void set_parent_rect(const Rect new_parent_rect) override;
    void paint(Painter&) override{};

    void focus() override;

    bool is_active() const;
    void toggle();
    void start();
    void stop();
    void handle_replay_thread_done(const uint32_t return_code);
    void file_error();

    std::string title() const override { return "BLE TX"; };

    struct BLETxPacket {
        char macAddress[13];
        char advertisementData[63];
        char packetCount[11];
        uint32_t packet_count;
    };

   private:
    void on_data(uint32_t value, bool is_data);
    void on_tx_progress(const bool done);
    void on_file_changed(const std::filesystem::path& new_file_path);
    void update_packet_display(BLETxPacket packet);

    NavigationView& nav_;
    TxRadioState radio_state_{
        2'402'000'000 /* frequency */,
        4'000'000 /* bandwidth */,
        4'000'000 /* sampling rate */
    };
    app_settings::SettingsManager settings_{
        "ble_tx_app", app_settings::Mode::TX};

    uint8_t console_color{0};
    uint32_t prev_value{0};

    std::filesystem::path file_path{};
    uint8_t channel_number = 37;

    // char macAddress[13] = "010203040506";
    // char advertisementData[63] = "00112233445566778899AABBCCDDEEFF";
    // char packetCount[11] = "0";

    bool is_running = false;
    uint64_t timer_count{0};
    uint64_t timer_period{256};
    bool repeatLoop = false;
    uint32_t packet_counter{0};
    uint32_t num_packets{0};
    uint32_t current_packet{0};

    enum PKT_TYPE {
        INVALID_TYPE,
        RAW,
        DISCOVERY,
        IBEACON,
        ADV_IND,
        ADV_DIRECT_IND,
        ADV_NONCONN_IND,
        ADV_SCAN_IND,
        SCAN_REQ,
        SCAN_RSP,
        CONNECT_REQ,
        LL_DATA,
        LL_CONNECTION_UPDATE_REQ,
        LL_CHANNEL_MAP_REQ,
        LL_TERMINATE_IND,
        LL_ENC_REQ,
        LL_ENC_RSP,
        LL_START_ENC_REQ,
        LL_START_ENC_RSP,
        LL_UNKNOWN_RSP,
        LL_FEATURE_REQ,
        LL_FEATURE_RSP,
        LL_PAUSE_ENC_REQ,
        LL_PAUSE_ENC_RSP,
        LL_VERSION_IND,
        LL_REJECT_IND,
        NUM_PKT_TYPE
    };

    static constexpr uint8_t mac_address_size_str{12};
    static constexpr uint8_t max_packet_size_str{62};
    static constexpr uint8_t max_packet_repeat_str{10};
    static constexpr uint32_t max_packet_repeat_count{UINT32_MAX};
    static constexpr uint32_t max_num_packets{256};

    BLETxPacket packets[max_num_packets];

    PKT_TYPE pduType = {RAW};

    static constexpr auto header_height = 9 * 16;

    Button button_open{
        {0 * 8, 0 * 16, 10 * 8, 2 * 16},
        "Open file"};

    Text text_filename{
        {11 * 8, 0 * 16, 12 * 8, 16},
        "-"};

    ProgressBar progressbar{
        {11 * 8, 1 * 16, 12 * 8, 16}};

    TxFrequencyField field_frequency{
        {0 * 8, 2 * 16},
        nav_};

    TransmitterView2 tx_view{
        {11 * 8, 2 * 16},
        /*short_ui*/ true};

    Checkbox check_loop{
        {21 * 8, 2 * 16},
        4,
        "Loop",
        true};

    ImageButton button_play{
        {28 * 8, 2 * 16, 2 * 8, 1 * 16},
        &bitmap_play,
        Color::green(),
        Color::black()};

    Labels label_speed{
        {{0 * 8, 6 * 8}, "Speed:", Color::light_grey()}};

    OptionsField options_speed{
        {7 * 8, 6 * 8},
        3,
        {{"1 ", 256},
         {"2 ", 128},
         {"3 ", 64},
         {"4 ", 32},
         {"5 ", 16}}};

    OptionsField options_channel{
        {11 * 8, 6 * 8},
        5,
        {{"Ch.37 ", 37},
         {"Ch.38", 38},
         {"Ch.39", 39}}};

    OptionsField options_adv_type{
        {17 * 8, 6 * 8},
        14,
        {{"DISCOVERY ", DISCOVERY},
         {"ADV_IND", ADV_IND},
         {"ADV_DIRECT", ADV_DIRECT_IND},
         {"ADV_NONCONN", ADV_NONCONN_IND},
         {"SCAN_REQ", SCAN_REQ},
         {"SCAN_RSP", SCAN_RSP},
         {"CONNECT_REQ", CONNECT_REQ}}};

    Labels label_packet_index{
        {{0 * 8, 10 * 8}, "Packet Index:", Color::light_grey()}};

    Text text_packet_index{
        {13 * 8, 5 * 16, 12 * 8, 16},
        "-"};

    Labels label_packets_sent{
        {{0 * 8, 12 * 8}, "Packets Left:", Color::light_grey()}};

    Text text_packets_sent{
        {13 * 8, 6 * 16, 12 * 8, 16},
        "-"};

    Labels label_mac_address{
        {{0 * 8, 14 * 8}, "Mac Address:", Color::light_grey()}};

    Text text_mac_address{
        {12 * 8, 7 * 16, 20 * 8, 16},
        "-"};

    Labels label_data_packet{
        {{0 * 8, 16 * 8}, "Packet Data:", Color::light_grey()}};

    Console console{
        {0, 8 * 16, 240, 240}};

    std::string str_log{""};
    bool logging{true};
    bool logging_done{false};

    std::unique_ptr<BLELoggerTx> logger{};

    MessageHandlerRegistration message_handler_packet{
        Message::ID::AFSKData,
        [this](Message* const p) {
            const auto message = static_cast<const AFSKDataMessage*>(p);
            this->on_data(message->value, message->is_data);
        }};

    MessageHandlerRegistration message_handler_tx_progress{
        Message::ID::TXProgress,
        [this](const Message* const p) {
            const auto message = *reinterpret_cast<const TXProgressMessage*>(p);
            this->on_tx_progress(message.done);
        }};
};

} /* namespace ui */

#endif /*__UI_AFSK_RX_H__*/
