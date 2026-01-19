/*
 * Copyright (C) 2026 HTotoo
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
/*
   This and The other files related to this is based on a lot of great people's work. https://github.com/RocketGod-git/ProtoPirate Check the repo, and the credits inside.
*/

#ifndef __UI_SubCar_H__
#define __UI_SubCar_H__

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

#include "../../baseband/fprotos/subcartypes.hpp"

using namespace ui;

namespace ui::external_app::subcarrx {

struct SubCarRecentEntry {
    using Key = uint64_t;
    static constexpr Key invalid_key = 0x0fffffff;
    uint8_t sensorType = FPC_Invalid;
    uint16_t bits = 0;
    uint16_t age = 0;  // updated on each seconds, show how long the signal was last seen
    uint64_t data = 0;
    uint64_t data2 = 0;
    SubCarRecentEntry() {}
    SubCarRecentEntry(
        uint8_t sensorType,
        uint64_t data = 0,
        uint64_t data2 = 0,
        uint16_t bits = 0)
        : sensorType{sensorType},
          bits{bits},
          data{data},
          data2{data2} {
    }
    Key key() const {
        return (data ^ ((static_cast<uint64_t>(sensorType) & 0xFF) << 0));  // should be optimized...
    }
    void inc_age(int delta) {
        if (UINT16_MAX - delta > age) age += delta;
    }
    void reset_age() {
        age = 0;
    }

    std::string to_csv();
};

class SubCarLogger {
   public:
    Optional<File::Error> append(const std::filesystem::path& filename) {
        return log_file.append(filename);
    }

    void log_data(SubCarRecentEntry& data);
    void write_header() {
        log_file.write_entry(";Type; Bits; Data;");
    }

   private:
    LogFile log_file{};
};
using SubCarRecentEntries = RecentEntries<SubCarRecentEntry>;
using SubCarRecentEntriesView = RecentEntriesView<SubCarRecentEntries>;

class SubCarView : public View {
   public:
    SubCarView(NavigationView& nav);
    ~SubCarView();

    void focus() override;

    std::string title() const override { return "SubCar"; };
    static const char* getSensorTypeName(FPROTO_SUBCAR_SENSOR type);
    static std::string pad_string_with_spaces(int snakes);

   private:
    void on_tick_second();
    void on_data(const SubCarDataMessage* data);

    NavigationView& nav_;
    RxRadioState radio_state_{
        433'920'000 /* frequency */,
        1'750'000 /* bandwidth */,
        4'000'000 /* sampling rate */,
        ReceiverModel::Mode::AMAudio};
    bool logging = false;
    app_settings::SettingsManager settings_{
        "rx_subcar",
        app_settings::Mode::RX,
        {
            {"log"sv, &logging},
        }};

    SubCarRecentEntries recent{};

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

    Labels labels{
        {{UI_POS_X_RIGHT(14), UI_POS_Y(1)}, "no fm yet :(", Theme::getInstance()->fg_light->foreground},
    };

    static constexpr auto header_height = 3 * 16;

    std::unique_ptr<SubCarLogger> logger{};

    ui::RecentEntriesColumns columns{{
        {"Type", 0},
        {"Bits", 4},
        {"Age", 3},
    }};
    SubCarRecentEntriesView recent_entries_view{columns, recent};

    void on_freqchg(int64_t freq);
    MessageHandlerRegistration message_handler_freqchg{
        Message::ID::FreqChangeCommand,
        [this](Message* const p) {
            const auto message = static_cast<const FreqChangeCommandMessage*>(p);
            this->on_freqchg(message->freq);
        }};

    MessageHandlerRegistration message_handler_packet{
        Message::ID::SubCarData,
        [this](Message* const p) {
            const auto message = static_cast<const SubCarDataMessage*>(p);
            this->on_data(message);
        }};
};

class SubCarRecentEntryDetailView : public View {
   public:
    SubCarRecentEntryDetailView(NavigationView& nav, const SubCarRecentEntry& entry);

    void update_data();
    void focus() override;

   private:
    NavigationView& nav_;
    SubCarRecentEntry entry_{};

    uint32_t serial = 0;
    std::string btn = "";
    uint32_t cnt = SD_NO_CNT;

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

}  // namespace ui::external_app::subcarrx

#endif /*__UI_SubCar_H__*/
