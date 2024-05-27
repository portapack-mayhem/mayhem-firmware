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

#ifndef __BLE_RX_APP_H__
#define __BLE_RX_APP_H__

#include "ble_tx_app.hpp"

#include "ui.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "ui_freq_field.hpp"
#include "ui_record_view.hpp"
#include "app_settings.hpp"
#include "radio_state.hpp"
#include "log_file.hpp"
#include "utility.hpp"
#include "usb_serial_thread.hpp"
#include "file_path.hpp"

#include "recent_entries.hpp"

class BLELogger {
   public:
    Optional<File::Error> append(const std::filesystem::path& filename) {
        return log_file.append(filename);
    }

    void log_raw_data(const std::string& data);

   private:
    LogFile log_file{};
};

namespace ui {
typedef enum {
    ADV_IND = 0,
    ADV_DIRECT_IND = 1,
    ADV_NONCONN_IND = 2,
    SCAN_REQ = 3,
    SCAN_RSP = 4,
    CONNECT_REQ = 5,
    ADV_SCAN_IND = 6,
    RESERVED0 = 7,
    RESERVED1 = 8,
    RESERVED2 = 9,
    RESERVED3 = 10,
    RESERVED4 = 11,
    RESERVED5 = 12,
    RESERVED6 = 13,
    RESERVED7 = 14,
    RESERVED8 = 15
} ADV_PDU_TYPE;

struct BleRecentEntry {
    using Key = uint64_t;

    static constexpr Key invalid_key = 0xffffffff;

    uint64_t macAddress;
    int dbValue;
    BlePacketData packetData;
    std::string timestamp;
    std::string dataString;
    std::string nameString;
    bool include_name;
    uint16_t numHits;
    ADV_PDU_TYPE pduType;
    uint8_t channelNumber;
    bool entryFound;

    BleRecentEntry()
        : BleRecentEntry{0} {
    }

    BleRecentEntry(
        const uint64_t macAddress)
        : macAddress{macAddress},
          dbValue{},
          packetData{},
          timestamp{},
          dataString{},
          nameString{},
          include_name{},
          numHits{},
          pduType{},
          channelNumber{},
          entryFound{} {
    }

    Key key() const {
        return macAddress;
    }
};

using BleRecentEntries = RecentEntries<BleRecentEntry>;
using BleRecentEntriesView = RecentEntriesView<BleRecentEntries>;

class BleRecentEntryDetailView : public View {
   public:
    BleRecentEntryDetailView(NavigationView& nav, const BleRecentEntry& entry);

    void set_entry(const BleRecentEntry& new_entry);
    const BleRecentEntry& entry() const { return entry_; };

    void update_data();
    void focus() override;
    void paint(Painter&) override;

   private:
    NavigationView& nav_;
    BleRecentEntry entry_{};
    BLETxPacket build_packet();
    void on_save_file(const std::string value, BLETxPacket packetToSave);
    bool saveFile(const std::filesystem::path& path, BLETxPacket packetToSave);
    std::string packetFileBuffer{};
    std::filesystem::path packet_save_path{blerx_dir / u"Lists/????.csv"};

    static constexpr uint8_t total_data_lines{5};

    Labels label_mac_address{
        {{0 * 8, 0 * 16}, "Mac Address:", Theme::getInstance()->fg_light->foreground}};

    Text text_mac_address{
        {12 * 8, 0 * 16, 17 * 8, 16},
        "-"};

    Labels label_pdu_type{
        {{0 * 8, 1 * 16}, "PDU Type:", Theme::getInstance()->fg_light->foreground}};

    Text text_pdu_type{
        {9 * 8, 1 * 16, 17 * 8, 16},
        "-"};

    Labels labels{
        {{0 * 8, 3 * 16}, "Len", Theme::getInstance()->fg_light->foreground},
        {{5 * 8, 3 * 16}, "Type", Theme::getInstance()->fg_light->foreground},
        {{10 * 8, 3 * 16}, "Value", Theme::getInstance()->fg_light->foreground},
    };

    Button button_send{
        {19, 224, 96, 24},
        "Send"};

    Button button_done{
        {125, 224, 96, 24},
        "Done"};

    Button button_save{
        {72, 264, 96, 24},
        "Save"};

    bool send_updates{false};

    Rect draw_field(
        Painter& painter,
        const Rect& draw_rect,
        const Style& style,
        const std::string& label,
        const std::string& value);
};

class BLERxView : public View {
   public:
    BLERxView(NavigationView& nav);
    ~BLERxView();

    void set_parent_rect(const Rect new_parent_rect) override;
    void paint(Painter&) override{};

    void focus() override;

    std::string title() const override { return "BLE RX"; };

   private:
    std::string build_line_str(BleRecentEntry entry);
    void on_save_file(const std::string value);
    bool saveFile(const std::filesystem::path& path);
    std::unique_ptr<UsbSerialThread> usb_serial_thread{};
    void on_data(BlePacketData* packetData);
    void on_filter_change(std::string value);
    void on_file_changed(const std::filesystem::path& new_file_path);
    void file_error();
    void on_timer();
    void handle_entries_sort(uint8_t index);
    void updateEntry(const BlePacketData* packet, BleRecentEntry& entry, ADV_PDU_TYPE pdu_type);

    NavigationView& nav_;
    RxRadioState radio_state_{
        2402000000 /* frequency */,
        4000000 /* bandwidth */,
        4000000 /* sampling rate */,
        ReceiverModel::Mode::WidebandFMAudio};

    uint8_t channel_index{0};
    uint8_t sort_index{0};
    std::string filter{};
    bool logging{false};
    bool serial_logging{false};

    bool name_enable{true};
    app_settings::SettingsManager settings_{
        "rx_ble",
        app_settings::Mode::RX,
        {
            {"channel_index"sv, &channel_index},
            {"sort_index"sv, &sort_index},
            {"filter"sv, &filter},
            {"log"sv, &logging},
            // disabled to always start without USB serial activated until we can make it non blocking if not connected
            // {"serial_log"sv, &serial_logging},
            {"name"sv, &name_enable},
        }};

    std::string str_console = "";
    uint8_t console_color{0};
    uint32_t prev_value{0};
    uint8_t channel_number = 37;
    bool auto_channel = false;

    int16_t timer_count{0};
    int16_t timer_period{6};  // 100ms

    std::string filterBuffer{};
    std::string listFileBuffer{};
    std::string headerStr = "Timestamp, MAC Address, Name, Packet Type, Data, Hits, dB, Channel";
    uint16_t maxLineLength = 140;

    std::filesystem::path file_path{};
    uint64_t found_count = 0;
    uint64_t total_count = 0;
    std::vector<std::string> searchList{};

    std::filesystem::path find_packet_path{blerx_dir / u"Find/????.TXT"};
    std::filesystem::path log_packets_path{blerx_dir / u"Logs/????.TXT"};
    std::filesystem::path packet_save_path{blerx_dir / u"Lists/????.csv"};

    static constexpr auto header_height = 4 * 16;
    static constexpr auto switch_button_height = 3 * 16;

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

    Labels label_sort{
        {{0 * 8, 3 * 8}, "Sort:", Theme::getInstance()->fg_light->foreground}};

    OptionsField options_sort{
        {5 * 8, 3 * 8},
        4,
        {{"MAC", 0},
         {"Hits", 1},
         {"dB", 2},
         {"Time", 3},
         {"Name", 4}}};

    Button button_filter{
        {11 * 8, 3 * 8, 4 * 8, 16},
        "Filter"};

    Checkbox check_log{
        {17 * 8, 3 * 8},
        3,
        "Log",
        true};

    Checkbox check_name{
        {23 * 8, 3 * 8},
        3,
        "Name",
        true};

    Button button_find{
        {0 * 8, 6 * 8, 4 * 8, 16},
        "Find"};

    Labels label_found{
        {{5 * 8, 6 * 8}, "Found:", Theme::getInstance()->fg_light->foreground}};

    Text text_found_count{
        {11 * 8, 3 * 16, 20 * 8, 16},
        "0/0"};

    Checkbox check_serial_log{
        {17 * 8, 3 * 16 - 2},
        7,
        "USB Log",
        true};

    Console console{
        {0, 4 * 16, 240, 240}};

    Button button_clear_list{
        {2 * 8, 320 - (16 + 32), 7 * 8, 32},
        "Clear"};

    Button button_save_list{
        {11 * 8, 320 - (16 + 32), 11 * 8, 32},
        "Export CSV"};

    Button button_switch{
        {240 - 6 * 8, 320 - (16 + 32), 4 * 8, 32},
        "Tx"};

    std::string str_log{""};
    std::unique_ptr<BLELogger> logger{};

    BleRecentEntries recent{};
    BleRecentEntries tempList{};

    const RecentEntriesColumns columns{{
        {"Mac Address", 17},
        {"Hits", 7},
        {"dB", 4},
    }};

    BleRecentEntriesView recent_entries_view{columns, recent};

    MessageHandlerRegistration message_handler_packet{
        Message::ID::BlePacket,
        [this](Message* const p) {
            const auto message = static_cast<const BLEPacketMessage*>(p);
            this->on_data(message->packet);
        }};

    MessageHandlerRegistration message_handler_frame_sync{
        Message::ID::DisplayFrameSync,
        [this](const Message* const) {
            this->on_timer();
        }};
};

} /* namespace ui */

#endif /*__UI_AFSK_RX_H__*/
