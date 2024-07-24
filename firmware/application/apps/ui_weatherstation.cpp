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

#include "ui_weatherstation.hpp"
#include "modems.hpp"
#include "audio.hpp"
#include "baseband_api.hpp"
#include "string_format.hpp"
#include "portapack_persistent_memory.hpp"

using namespace portapack;
using namespace ui;

namespace pmem = portapack::persistent_memory;

namespace ui {

void WeatherRecentEntryDetailView::update_data() {
    // set text elements
    text_type.set(WeatherView::getWeatherSensorTypeName((FPROTO_WEATHER_SENSOR)entry_.sensorType));
    if (entry_.id != WS_NO_ID)
        text_id.set("0x" + to_string_hex(entry_.id));
    else
        text_id.set("-");
    if (entry_.temp != WS_NO_TEMPERATURE)
        text_temp.set(weather_units_fahr ? to_string_decimal((entry_.temp * 9 / 5) + 32, 1) + STR_DEGREES_F : to_string_decimal(entry_.temp, 2) + STR_DEGREES_C);
    else
        text_temp.set("-");
    if (entry_.humidity != WS_NO_HUMIDITY)
        text_hum.set(to_string_dec_uint(entry_.humidity) + "%");
    else
        text_hum.set("-");
    if (entry_.channel != WS_NO_CHANNEL)
        text_ch.set(to_string_dec_uint(entry_.channel));
    else
        text_ch.set("-");
    if (entry_.battery_low != WS_NO_BATT)
        text_batt.set(to_string_dec_uint(entry_.battery_low) + " " + ((entry_.battery_low == 0) ? "OK" : "LOW"));
    else
        text_batt.set("-");
    text_age.set(to_string_dec_uint(entry_.age) + " sec");
}

WeatherRecentEntryDetailView::WeatherRecentEntryDetailView(NavigationView& nav, const WeatherRecentEntry& entry)
    : nav_{nav},
      entry_{entry} {
    add_children({&button_done,
                  &text_type,
                  &text_id,
                  &text_temp,
                  &text_hum,
                  &text_ch,
                  &text_batt,
                  &text_age,
                  &labels});

    button_done.on_select = [&nav](const ui::Button&) {
        nav.pop();
    };
    update_data();
}

void WeatherRecentEntryDetailView::focus() {
    button_done.focus();
}

void WeatherView::focus() {
    field_frequency.focus();
}

WeatherView::WeatherView(NavigationView& nav)
    : nav_{nav} {
    add_children({&rssi,
                  &field_rf_amp,
                  &field_lna,
                  &field_vga,
                  &field_volume,
                  &field_frequency,
                  &options_temperature,
                  &button_clear_list,
                  &recent_entries_view});

    baseband::run_image(portapack::spi_flash::image_tag_weather);

    button_clear_list.on_select = [this](Button&) {
        recent.clear();
        recent_entries_view.set_dirty();
    };
    field_frequency.set_step(10000);

    options_temperature.on_change = [this](size_t, int32_t i) {
        weather_units_fahr = (bool)i;
        recent_entries_view.set_dirty();
    };
    options_temperature.set_selected_index(weather_units_fahr, false);

    const Rect content_rect{0, header_height, screen_width, screen_height - header_height};
    recent_entries_view.set_parent_rect(content_rect);
    recent_entries_view.on_select = [this](const WeatherRecentEntry& entry) {
        nav_.push<WeatherRecentEntryDetailView>(entry);
    };
    baseband::set_subghzd_config(0, receiver_model.sampling_rate());  // 0=am
    receiver_model.enable();
    signal_token_tick_second = rtc_time::signal_tick_second += [this]() {
        on_tick_second();
    };

    if (pmem::beep_on_packets()) {
        audio::set_rate(audio::Rate::Hz_24000);
        audio::output::start();
    }
}

void WeatherView::on_tick_second() {
    for (auto& entry : recent) {
        entry.inc_age(1);
    }
    recent_entries_view.set_dirty();
}

void WeatherView::on_data(const WeatherDataMessage* data) {
    WeatherRecentEntry key{data->sensorType, data->id, data->temp, data->humidity, data->channel, data->battery_low};
    auto matching_recent = find(recent, key.key());
    if (matching_recent != std::end(recent)) {
        // Found within. Move to front of list, increment counter.
        (*matching_recent).reset_age();
        recent.push_front(*matching_recent);
        recent.erase(matching_recent);
    } else {
        recent.emplace_front(key);
        truncate_entries(recent, 64);
    }
    recent_entries_view.set_dirty();

    if (pmem::beep_on_packets()) {
        baseband::request_audio_beep(1000, 24000, 60);
    }
}

WeatherView::~WeatherView() {
    rtc_time::signal_tick_second -= signal_token_tick_second;
    audio::output::stop();
    receiver_model.disable();
    baseband::shutdown();
}

const char* WeatherView::getWeatherSensorTypeName(FPROTO_WEATHER_SENSOR type) {
    switch (type) {
        case FPW_NexusTH:
            return "NexusTH";
        case FPW_Acurite592TXR:
            return "Acurite592TXR";
        case FPW_Acurite606TX:
            return "Acurite606TX";
        case FPW_Acurite609TX:
            return "Acurite609TX";
        case FPW_Ambient:
            return "Ambient";
        case FPW_AuriolAhfl:
            return "AuriolAhfl";
        case FPW_AuriolTH:
            return "AuriolTH";
        case FPW_GTWT02:
            return "GT-WT02";
        case FPW_GTWT03:
            return "GT-WT03";
        case FPW_INFACTORY:
            return "InFactory";
        case FPW_LACROSSETX:
            return "LaCrosse TX";
        case FPW_LACROSSETX141thbv2:
            return "LaCrosse TX141THBv2";
        case FPW_OREGON2:
            return "Oregon2";
        case FPW_OREGON3:
            return "Oregon3";
        case FPW_OREGONv1:
            return "OregonV1";
        case FPW_THERMOPROTX4:
            return "ThermoPro TX4";
        case FPW_TX_8300:
            return "TX 8300";
        case FPW_WENDOX_W6726:
            return "Wendox W6726";
        case FPW_Acurite986:
            return "Acurite986";
        case FPW_KEDSUM:
            return "Kedsum";
        case FPW_Acurite5in1:
            return "Acurite5in1";
        case FPW_EmosE601x:
            return "EmosE601x";
        case FPW_Invalid:
        default:
            return "Unknown";
    }
}

std::string WeatherView::pad_string_with_spaces(int snakes) {
    std::string paddedStr(snakes, ' ');
    return paddedStr;
}

template <>
void RecentEntriesTable<ui::WeatherRecentEntries>::draw(
    const Entry& entry,
    const Rect& target_rect,
    Painter& painter,
    const Style& style) {
    std::string line{};
    line.reserve(30);

    line = WeatherView::getWeatherSensorTypeName((FPROTO_WEATHER_SENSOR)entry.sensorType);
    if (line.length() < 10) {
        line += WeatherView::pad_string_with_spaces(10 - line.length());
    } else {
        line = truncate(line, 10);
    }

    std::string temp = (weather_units_fahr ? to_string_decimal((entry.temp * 9 / 5) + 32, 1) : to_string_decimal(entry.temp, 1));
    std::string humStr = (entry.humidity != WS_NO_HUMIDITY) ? to_string_dec_uint(entry.humidity) + "%" : "-";
    std::string chStr = (entry.channel != WS_NO_CHANNEL) ? to_string_dec_uint(entry.channel) : "-";
    std::string ageStr = to_string_dec_uint(entry.age);

    line += WeatherView::pad_string_with_spaces(6 - temp.length()) + temp;
    line += WeatherView::pad_string_with_spaces(5 - humStr.length()) + humStr;
    line += WeatherView::pad_string_with_spaces(4 - chStr.length()) + chStr;
    line += WeatherView::pad_string_with_spaces(5 - ageStr.length()) + ageStr;

    line.resize(target_rect.width() / 8, ' ');
    painter.draw_string(target_rect.location(), style, line);
}

}  // namespace ui
