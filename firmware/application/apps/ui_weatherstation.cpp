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

namespace ui {

void WeatherRecentEntryDetailView::update_data() {
    // set text elements
    text_type.set(WeatherView::getWeatherSensorTypeName((FPROTO_WEATHER_SENSOR)entry_.sensorType));
    text_id.set("0x" + to_string_hex(entry_.id));
    text_temp.set(weather_units_fahr ? to_string_decimal((entry_.temp * 9 / 5) + 32, 1) : to_string_decimal(entry_.temp, 2));
    text_hum.set(to_string_dec_uint(entry_.humidity) + "%");
    text_ch.set(to_string_dec_uint(entry_.channel));
    text_batt.set(to_string_dec_uint(entry_.battery_low) + " " + ((entry_.battery_low == 0) ? "OK" : "LOW"));
}

void WeatherRecentEntryDetailView::set_entry(const WeatherRecentEntry& entry) {
    entry_ = entry;
    update_data();
    set_dirty();
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
                  &field_frequency,
                  &options_temperature,
                  &button_clear_list,
                  &recent_entries_view});

    baseband::run_image(portapack::spi_flash::image_tag_weather);

    button_clear_list.on_select = [this](Button&) {
        recent.clear();
        recent_entries_view.set_dirty();
    };
    field_frequency.set_step(100000);

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
    baseband::set_weather();
    receiver_model.enable();
}

void WeatherView::on_data(const WeatherDataMessage* data) {
    WeatherRecentEntry key{data->sensorType, data->id, data->temp, data->humidity, data->channel, data->battery_low};
    auto matching_recent = find(recent, key.key());
    if (matching_recent != std::end(recent)) {
        // Found within. Move to front of list, increment counter.
        recent.push_front(*matching_recent);
        recent.erase(matching_recent);
    } else {
        recent.emplace_front(key);
        truncate_entries(recent, 64);
    }
    recent_entries_view.set_dirty();
}

WeatherView::~WeatherView() {
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
    if (line.length() < 13) {
        line += WeatherView::pad_string_with_spaces(13 - line.length());
    } else {
        line = truncate(line, 13);
    }

    std::string temp = to_string_decimal(entry.temp, 2);
    std::string humStr = to_string_dec_uint(entry.humidity) + "%";
    std::string chStr = to_string_dec_uint(entry.channel);

    line += WeatherView::pad_string_with_spaces(7 - temp.length()) + temp;
    line += WeatherView::pad_string_with_spaces(5 - humStr.length()) + humStr;
    line += WeatherView::pad_string_with_spaces(4 - chStr.length()) + chStr;

    line.resize(target_rect.width() / 8, ' ');
    painter.draw_string(target_rect.location(), style, line);
}

}  // namespace ui