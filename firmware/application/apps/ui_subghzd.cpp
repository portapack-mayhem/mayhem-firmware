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

#include "ui_subghzd.hpp"
#include "audio.hpp"
#include "baseband_api.hpp"
#include "string_format.hpp"
#include "portapack_persistent_memory.hpp"

using namespace portapack;
using namespace ui;

namespace ui {

void SubGhzDRecentEntryDetailView::update_data() {
    // set text elements
    text_type.set(SubGhzDView::getSensorTypeName((FPROTO_SUBGHZD_SENSOR)entry_.sensorType));
    text_id.set("0x" + to_string_hex(entry_.id));
    text_temp.set(to_string_decimal(entry_.temp, 2));
    text_hum.set(to_string_dec_uint(entry_.humidity) + "%");
    text_ch.set(to_string_dec_uint(entry_.channel));
    text_batt.set(to_string_dec_uint(entry_.battery_low) + " " + ((entry_.battery_low == 0) ? "OK" : "LOW"));
    text_age.set(to_string_dec_uint(entry_.age) + " sec");
}

SubGhzDRecentEntryDetailView::SubGhzDRecentEntryDetailView(NavigationView& nav, const SubGhzDRecentEntry& entry)
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

void SubGhzDRecentEntryDetailView::focus() {
    button_done.focus();
}

void SubGhzDView::focus() {
    field_frequency.focus();
}

SubGhzDView::SubGhzDView(NavigationView& nav)
    : nav_{nav} {
    add_children({&rssi,
                  &field_rf_amp,
                  &field_lna,
                  &field_vga,
                  &field_frequency,
                  &button_clear_list,
                  &recent_entries_view});

    baseband::run_image(portapack::spi_flash::image_tag_weather);

    button_clear_list.on_select = [this](Button&) {
        recent.clear();
        recent_entries_view.set_dirty();
    };
    field_frequency.set_step(100000);

    const Rect content_rect{0, header_height, screen_width, screen_height - header_height};
    recent_entries_view.set_parent_rect(content_rect);
    recent_entries_view.on_select = [this](const SubGhzDRecentEntry& entry) {
        nav_.push<SubGhzDRecentEntryDetailView>(entry);
    };
    baseband::set_weather();
    receiver_model.enable();
    signal_token_tick_second = rtc_time::signal_tick_second += [this]() {
        on_tick_second();
    };
}

void SubGhzDView::on_tick_second() {
    for (auto& entry : recent) {
        entry.inc_age(1);
    }
    recent_entries_view.set_dirty();
}

void SubGhzDView::on_data(const WeatherDataMessage* data) {
    SubGhzDRecentEntry key{data->sensorType, data->id, data->temp, data->humidity, data->channel, data->battery_low};
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
}

SubGhzDView::~SubGhzDView() {
    rtc_time::signal_tick_second -= signal_token_tick_second;
    receiver_model.disable();
    baseband::shutdown();
}

const char* SubGhzDView::getSensorTypeName(FPROTO_SUBGHZD_SENSOR type) {
    switch (type) {
        case FPS_Invalid:
        default:
            return "Unknown";
    }
}

std::string SubGhzDView::pad_string_with_spaces(int snakes) {
    std::string paddedStr(snakes, ' ');
    return paddedStr;
}

template <>
void RecentEntriesTable<ui::SubGhzDRecentEntries>::draw(
    const Entry& entry,
    const Rect& target_rect,
    Painter& painter,
    const Style& style) {
    std::string line{};
    line.reserve(30);

    line = SubGhzDView::getSensorTypeName((FPROTO_SUBGHZD_SENSOR)entry.sensorType);
    if (line.length() < 10) {
        line += SubGhzDView::pad_string_with_spaces(10 - line.length());
    } else {
        line = truncate(line, 10);
    }

    std::string temp = to_string_decimal(entry.temp, 1);
    std::string humStr = to_string_dec_uint(entry.humidity) + "%";
    std::string chStr = to_string_dec_uint(entry.channel);
    std::string ageStr = to_string_dec_uint(entry.age);

    line += SubGhzDView::pad_string_with_spaces(6 - temp.length()) + temp;
    line += SubGhzDView::pad_string_with_spaces(5 - humStr.length()) + humStr;
    line += SubGhzDView::pad_string_with_spaces(4 - chStr.length()) + chStr;
    line += SubGhzDView::pad_string_with_spaces(4 - ageStr.length()) + ageStr;

    line.resize(target_rect.width() / 8, ' ');
    painter.draw_string(target_rect.location(), style, line);
}

}  // namespace ui