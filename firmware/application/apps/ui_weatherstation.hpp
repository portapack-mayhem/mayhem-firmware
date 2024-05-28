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

#ifndef __UI_WEATHER_H__
#define __UI_WEATHER_H__

#include "ui.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "ui_freq_field.hpp"
#include "app_settings.hpp"
#include "radio_state.hpp"
#include "utility.hpp"
#include "recent_entries.hpp"

#include "../baseband/fprotos/weathertypes.hpp"
using namespace ui;

namespace ui {

static bool weather_units_fahr{false};

struct WeatherRecentEntry {
    using Key = uint64_t;
    static constexpr Key invalid_key = 0x0fffffff;  // todo calc the invalid all
    uint8_t sensorType = FPW_Invalid;
    uint32_t id = WS_NO_ID;
    float temp = WS_NO_TEMPERATURE;
    uint8_t humidity = WS_NO_HUMIDITY;
    uint8_t battery_low = WS_NO_BATT;
    uint8_t channel = WS_NO_CHANNEL;
    uint16_t age = 0;  // updated on each seconds, show how long the signal was last seen

    WeatherRecentEntry() {}
    WeatherRecentEntry(
        uint8_t sensorType,
        uint32_t id,
        float temp,
        uint8_t humidity,
        uint8_t channel,
        uint8_t battery_low = WS_NO_BATT)
        : sensorType{sensorType},
          id{id},
          temp{temp},
          humidity{humidity},
          battery_low{battery_low},
          channel{channel} {
    }
    Key key() const {
        return (((static_cast<uint64_t>(temp * 10) & 0xFFFF) << 48) ^ static_cast<uint64_t>(id) << 24) |
               (static_cast<uint64_t>(sensorType) & 0xFF) << 16 |
               (static_cast<uint64_t>(humidity) & 0xFF) << 8 |
               (static_cast<uint64_t>(battery_low) & 0xF) << 4 |
               (static_cast<uint64_t>(channel) & 0xF);
    }
    void inc_age(int delta) {
        if (UINT16_MAX - delta > age) age += delta;
    }
    void reset_age() {
        age = 0;
    }
};
using WeatherRecentEntries = RecentEntries<WeatherRecentEntry>;
using WeatherRecentEntriesView = RecentEntriesView<WeatherRecentEntries>;

class WeatherView : public View {
   public:
    WeatherView(NavigationView& nav);
    ~WeatherView();

    void focus() override;

    std::string title() const override { return "Weather"; };
    static const char* getWeatherSensorTypeName(FPROTO_WEATHER_SENSOR type);
    static std::string pad_string_with_spaces(int snakes);

   private:
    void on_tick_second();
    void on_data(const WeatherDataMessage* data);

    NavigationView& nav_;
    RxRadioState radio_state_{
        433'920'000 /* frequency */,
        1'750'000 /* bandwidth */,
        2'000'000 /* sampling rate */,
        ReceiverModel::Mode::AMAudio};
    app_settings::SettingsManager settings_{
        "rx_weather",
        app_settings::Mode::RX,
        {
            {"units_fahr"sv, &weather_units_fahr},
        }};

    WeatherRecentEntries recent{};

    OptionsField options_temperature{
        {10 * 8, 0 * 16},
        2,
        {{STR_DEGREES_C, 0},
         {STR_DEGREES_F, 1}}};

    RFAmpField field_rf_amp{
        {13 * 8, 0 * 16}};
    LNAGainField field_lna{
        {15 * 8, 0 * 16}};
    VGAGainField field_vga{
        {18 * 8, 0 * 16}};
    RSSI rssi{
        {21 * 8, 0, 6 * 8, 4}};

    AudioVolumeField field_volume{
        {28 * 8, 0 * 16}};

    RxFrequencyField field_frequency{
        {0 * 8, 0 * 16},
        nav_};

    SignalToken signal_token_tick_second{};

    Button button_clear_list{
        {0, 16, 7 * 8, 32},
        "Clear"};

    static constexpr auto header_height = 3 * 16;

    const RecentEntriesColumns columns{{
        {"Type", 10},
        {"Temp", 5},
        {"Hum", 4},
        {"Ch", 3},
        {"Age", 4},
    }};
    WeatherRecentEntriesView recent_entries_view{columns, recent};

    MessageHandlerRegistration message_handler_packet{
        Message::ID::WeatherData,
        [this](Message* const p) {
            const auto message = static_cast<const WeatherDataMessage*>(p);
            this->on_data(message);
        }};
};

class WeatherRecentEntryDetailView : public View {
   public:
    WeatherRecentEntryDetailView(NavigationView& nav, const WeatherRecentEntry& entry);

    void update_data();
    void focus() override;

   private:
    NavigationView& nav_;
    WeatherRecentEntry entry_{};
    Text text_type{{10 * 8, 1 * 16, 15 * 8, 16}, "?"};
    Text text_id{{10 * 8, 2 * 16, 10 * 8, 16}, "?"};
    Text text_temp{{10 * 8, 3 * 16, 8 * 8, 16}, "?"};
    Text text_hum{{10 * 8, 4 * 16, 6 * 8, 16}, "?"};
    Text text_ch{{10 * 8, 5 * 16, 6 * 8, 16}, "?"};
    Text text_batt{{10 * 8, 6 * 16, 6 * 8, 16}, "?"};
    Text text_age{{10 * 8, 7 * 16, 10 * 8, 16}, "?"};

    Labels labels{
        {{0 * 8, 0 * 16}, "Weather Station", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 1 * 16}, "Type:", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 2 * 16}, "Id: ", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 3 * 16}, "Temp:", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 4 * 16}, "Humidity:", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 5 * 16}, "Channel:", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 6 * 16}, "Battery:", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 7 * 16}, "Age:", Theme::getInstance()->fg_light->foreground},
    };

    Button button_done{
        {screen_width - 96 - 4, screen_height - 32 - 12, 96, 32},
        "Done"};
};

}  // namespace ui

#endif /*__UI_WEATHER_H__*/
