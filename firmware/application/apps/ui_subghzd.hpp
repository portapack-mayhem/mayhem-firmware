/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2017 Furrtek
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

#ifndef __UI_SUBGHZD_H__
#define __UI_SUBGHZD_H__

#define SD_NO_SERIAL 0xFFFFFFFF
#define SD_NO_BTN 0xFF
#define SD_NO_CNT 0xFF

#include "ui.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "ui_freq_field.hpp"
#include "app_settings.hpp"
#include "radio_state.hpp"
#include "utility.hpp"
#include "log_file.hpp"
#include "recent_entries.hpp"

#include "../baseband/fprotos/subghztypes.hpp"
#include "../baseband/fprotos/fprotogeneral.hpp"

using namespace ui;

namespace ui {

struct SubGhzDRecentEntry {
    using Key = uint64_t;
    static constexpr Key invalid_key = 0x0fffffff;
    uint8_t sensorType = FPS_Invalid;
    uint16_t bits = 0;
    uint16_t age = 0;  // updated on each seconds, show how long the signal was last seen
    uint64_t data = 0;
    SubGhzDRecentEntry() {}
    SubGhzDRecentEntry(
        uint8_t sensorType,
        uint64_t data = 0,
        uint16_t bits = 0)
        : sensorType{sensorType},
          bits{bits},
          data{data} {
    }
    Key key() const {
        return (data ^ ((static_cast<uint64_t>(sensorType) & 0xFF) << 0));
    }
    void inc_age(int delta) {
        if (UINT16_MAX - delta > age) age += delta;
    }
    void reset_age() {
        age = 0;
    }

    std::string to_csv();
};

class SubGhzDLogger {
   public:
    Optional<File::Error> append(const std::filesystem::path& filename) {
        return log_file.append(filename);
    }

    void log_data(SubGhzDRecentEntry& data);
    void write_header() {
        log_file.write_entry(";Type; Bits; Data;");
    }

   private:
    LogFile log_file{};
};
using SubGhzDRecentEntries = RecentEntries<SubGhzDRecentEntry>;
using SubGhzDRecentEntriesView = RecentEntriesView<SubGhzDRecentEntries>;

class SubGhzDView : public View {
   public:
    SubGhzDView(NavigationView& nav);
    ~SubGhzDView();

    void focus() override;

    std::string title() const override { return "SubGhzD"; };
    static const char* getSensorTypeName(FPROTO_SUBGHZD_SENSOR type);
    static std::string pad_string_with_spaces(int snakes);

   private:
    void on_tick_second();
    void on_data(const SubGhzDDataMessage* data);

    NavigationView& nav_;
    RxRadioState radio_state_{
        433'920'000 /* frequency */,
        1'750'000 /* bandwidth */,
        4'000'000 /* sampling rate */,
        ReceiverModel::Mode::AMAudio};
    bool logging = false;
    app_settings::SettingsManager settings_{
        "rx_subghzd",
        app_settings::Mode::RX,
        {
            {"log"sv, &logging},
        }};

    SubGhzDRecentEntries recent{};

    RFAmpField field_rf_amp{
        {13 * 8, UI_POS_Y(0)}};
    LNAGainField field_lna{
        {15 * 8, UI_POS_Y(0)}};
    VGAGainField field_vga{
        {18 * 8, UI_POS_Y(0)}};
    RSSI rssi{
        {21 * 8, 0, UI_POS_WIDTH_REMAINING(24), 4}};
    Channel channel{
        {21 * 8, 5, UI_POS_WIDTH_REMAINING(24), 4},
    };
    RxFrequencyField field_frequency{
        {UI_POS_X(0), UI_POS_Y(0)},
        nav_};

    SignalToken signal_token_tick_second{};

    Button button_clear_list{
        {0, 16, 7 * 8, 32},
        "Clear"};

    Checkbox check_log{
        {10 * 8, 18},
        3,
        "Log",
        true};

    static constexpr auto header_height = 3 * 16;

    std::unique_ptr<SubGhzDLogger> logger{};

    RecentEntriesColumns columns{{
        {"Type", 0},
        {"Bits", 4},
        {"Age", 3},
    }};
    SubGhzDRecentEntriesView recent_entries_view{columns, recent};

    void on_freqchg(int64_t freq);
    MessageHandlerRegistration message_handler_freqchg{
        Message::ID::FreqChangeCommand,
        [this](Message* const p) {
            const auto message = static_cast<const FreqChangeCommandMessage*>(p);
            this->on_freqchg(message->freq);
        }};

    MessageHandlerRegistration message_handler_packet{
        Message::ID::SubGhzDData,
        [this](Message* const p) {
            const auto message = static_cast<const SubGhzDDataMessage*>(p);
            this->on_data(message);
        }};
};

class SubGhzDRecentEntryDetailView : public View {
   public:
    SubGhzDRecentEntryDetailView(NavigationView& nav, const SubGhzDRecentEntry& entry);

    void update_data();
    void focus() override;

   private:
    NavigationView& nav_;
    SubGhzDRecentEntry entry_{};

    uint32_t serial = 0;
    uint8_t btn = SD_NO_BTN;
    uint32_t cnt = SD_NO_CNT;
    uint32_t seed = 0;

    Text text_type{{UI_POS_X(0), 1 * 16, 15 * 8, 16}, "?"};
    Text text_id{{6 * 8, 2 * 16, 10 * 8, 16}, "?"};

    Console console{
        {0, 4 * 16, screen_width, screen_height - (4 * 16) - 36}};

    Labels labels{
        {{UI_POS_X(0), UI_POS_Y(0)}, "Type:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), 2 * 16}, "Serial: ", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), 3 * 16}, "Data:", Theme::getInstance()->fg_light->foreground},
    };

    Button button_done{
        {screen_width - 96 - 4, screen_height - 32 - 12, 96, 32},
        "Done"};

    void parseProtocol();
};

}  // namespace ui

#endif /*__UI_SUBGHZD_H__*/
