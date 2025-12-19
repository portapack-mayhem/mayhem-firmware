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

#include "ui_subcar.hpp"
#include "audio.hpp"
#include "baseband_api.hpp"
#include "string_format.hpp"
#include "file_path.hpp"
#include "portapack_persistent_memory.hpp"

using namespace portapack;
using namespace ui;

namespace ui {

std::string SubCarRecentEntry::to_csv() {
    std::string csv = ";";
    csv += SubCarView::getSensorTypeName((FPROTO_SUBCAR_SENSOR)sensorType);
    csv += ";" + to_string_dec_uint(bits) + ";";
    csv += to_string_hex(data, 64 / 4) + ";" + to_string_hex(data2, 64 / 4);
    return csv;
}

void SubCarLogger::log_data(SubCarRecentEntry& data) {
    log_file.write_entry(data.to_csv());
}

void SubCarRecentEntryDetailView::update_data() {
    // process protocol data
    parseProtocol();
    // set text elements
    text_type.set(SubCarView::getSensorTypeName((FPROTO_SUBCAR_SENSOR)entry_.sensorType));

    text_id.set("0x" + to_string_hex(serial));
    if (entry_.bits > 0) console.writeln("Bits: " + to_string_dec_uint(entry_.bits));
    if (!btn.empty()) console.writeln("Btn: " + btn);
    if (cnt != SD_NO_CNT) console.writeln("Cnt: " + to_string_dec_uint(cnt));
    if (entry_.data != 0) console.writeln("Data : " + to_string_hex(entry_.data));
    if (entry_.data2 != 0) console.writeln("Data2: " + to_string_hex(entry_.data2));
}

SubCarRecentEntryDetailView::SubCarRecentEntryDetailView(NavigationView& nav, const SubCarRecentEntry& entry)
    : nav_{nav},
      entry_{entry} {
    add_children({&button_done,
                  &text_type,
                  &text_id,
                  &console,
                  &labels});

    button_done.on_select = [&nav](const ui::Button&) {
        nav.pop();
    };
    update_data();
}

void SubCarRecentEntryDetailView::focus() {
    button_done.focus();
}

void SubCarView::focus() {
    field_frequency.focus();
}

SubCarView::SubCarView(NavigationView& nav)
    : nav_{nav} {
    add_children({&rssi,
                  &channel,
                  &field_rf_amp,
                  &field_lna,
                  &field_vga,
                  &field_frequency,
                  &button_clear_list,
                  &check_log,
                  &recent_entries_view});

    baseband::run_image(portapack::spi_flash::image_tag_subcar);
    logger = std::make_unique<SubCarLogger>();

    button_clear_list.on_select = [this](Button&) {
        recent.clear();
        recent_entries_view.set_dirty();
    };
    field_frequency.set_step(10000);
    check_log.on_select = [this](Checkbox&, bool v) {
        logging = v;
        if (logger && logging) {
            logger->append(logs_dir.string() + "/SubCarLOG_" + to_string_timestamp(rtc_time::now()) + ".CSV");
            logger->write_header();
        }
    };
    check_log.set_value(logging);
    const Rect content_rect{0, header_height, screen_width, screen_height - header_height};
    recent_entries_view.set_parent_rect(content_rect);
    recent_entries_view.on_select = [this](const SubCarRecentEntry& entry) {
        nav_.push<SubCarRecentEntryDetailView>(entry);
    };
    baseband::set_subghzd_config(0, receiver_model.sampling_rate());  // 0=am
    receiver_model.enable();
    signal_token_tick_second = rtc_time::signal_tick_second += [this]() {
        on_tick_second();
    };
}

void SubCarView::on_tick_second() {
    for (auto& entry : recent) {
        entry.inc_age(1);
    }
    recent_entries_view.set_dirty();
}

void SubCarView::on_data(const SubCarDataMessage* data) {
    SubCarRecentEntry key{data->sensorType, data->data, data->data2, data->bits};
    if (logger && logging) {
        logger->log_data(key);
    }
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

SubCarView::~SubCarView() {
    rtc_time::signal_tick_second -= signal_token_tick_second;
    receiver_model.disable();
    baseband::shutdown();
}

const char* SubCarView::getSensorTypeName(FPROTO_SUBCAR_SENSOR type) {
    switch (type) {
        case FPC_SUZUKI:
            return "Suzuki";
        case FPC_VW:
            return "VW";
        case FPC_SUBARU:
            return "Subaru";

        case FPC_Invalid:
        default:
            return "Unknown";
    }
}

std::string SubCarView::pad_string_with_spaces(int snakes) {
    std::string paddedStr(snakes, ' ');
    return paddedStr;
}

void SubCarView::on_freqchg(int64_t freq) {
    field_frequency.set_value(freq);
}

template <>
void RecentEntriesTable<ui::SubCarRecentEntries>::draw(
    const Entry& entry,
    const Rect& target_rect,
    Painter& painter,
    const Style& style,
    RecentEntriesColumns& columns) {
    std::string line{};
    line.reserve(30);

    line = SubCarView::getSensorTypeName((FPROTO_SUBCAR_SENSOR)entry.sensorType);
    line = line + " " + to_string_hex(entry.data << 32);
    line.resize(columns.at(0).second, ' ');
    std::string ageStr = to_string_dec_uint(entry.age);
    std::string bitsStr = to_string_dec_uint(entry.bits);
    line += SubCarView::pad_string_with_spaces(5 - bitsStr.length()) + bitsStr;
    line += SubCarView::pad_string_with_spaces(4 - ageStr.length()) + ageStr;

    line.resize(target_rect.width() / 8, ' ');
    painter.draw_string(target_rect.location(), style, line);
}

void subaru_decode_count(const uint8_t* KB, uint16_t* count) {
    uint8_t lo = 0;
    if ((KB[4] & 0x40) == 0)
        lo |= 0x01;
    if ((KB[4] & 0x80) == 0)
        lo |= 0x02;
    if ((KB[5] & 0x01) == 0)
        lo |= 0x04;
    if ((KB[5] & 0x02) == 0)
        lo |= 0x08;
    if ((KB[6] & 0x01) == 0)
        lo |= 0x10;
    if ((KB[6] & 0x02) == 0)
        lo |= 0x20;
    if ((KB[5] & 0x40) == 0)
        lo |= 0x40;
    if ((KB[5] & 0x80) == 0)
        lo |= 0x80;

    uint8_t REG_SH1 = (KB[7] << 4) & 0xF0;
    if (KB[5] & 0x04)
        REG_SH1 |= 0x04;
    if (KB[5] & 0x08)
        REG_SH1 |= 0x08;
    if (KB[6] & 0x80)
        REG_SH1 |= 0x02;
    if (KB[6] & 0x40)
        REG_SH1 |= 0x01;

    uint8_t REG_SH2 = ((KB[6] << 2) & 0xF0) | ((KB[7] >> 4) & 0x0F);

    uint8_t SER0 = KB[3];
    uint8_t SER1 = KB[1];
    uint8_t SER2 = KB[2];

    uint8_t total_rot = 4 + lo;
    for (uint8_t i = 0; i < total_rot; ++i) {
        uint8_t t_bit = (SER0 >> 7) & 1;
        SER0 = ((SER0 << 1) & 0xFE) | ((SER1 >> 7) & 1);
        SER1 = ((SER1 << 1) & 0xFE) | ((SER2 >> 7) & 1);
        SER2 = ((SER2 << 1) & 0xFE) | t_bit;
    }

    uint8_t T1 = SER1 ^ REG_SH1;
    uint8_t T2 = SER2 ^ REG_SH2;

    uint8_t hi = 0;
    if ((T1 & 0x10) == 0)
        hi |= 0x04;
    if ((T1 & 0x20) == 0)
        hi |= 0x08;
    if ((T2 & 0x80) == 0)
        hi |= 0x02;
    if ((T2 & 0x40) == 0)
        hi |= 0x01;
    if ((T1 & 0x01) == 0)
        hi |= 0x40;
    if ((T1 & 0x02) == 0)
        hi |= 0x80;
    if ((T2 & 0x08) == 0)
        hi |= 0x20;
    if ((T2 & 0x04) == 0)
        hi |= 0x10;
    *count = ((hi << 8) | lo) & 0xFFFF;
}

void SubCarRecentEntryDetailView::parseProtocol() {
    btn = "";
    cnt = SD_NO_CNT;
    serial = 0;

    if (entry_.sensorType == FPC_Invalid) return;

    if (entry_.sensorType == FPC_SUZUKI) {
        uint32_t serial_button = (((entry_.data >> 32) & 0xFFF) << 20) | (entry_.data >> 12);
        serial = serial_button >> 4;
        uint8_t buttonid = serial_button & 0xF;
        cnt = (entry_.data >> 44) & 0xFFFF;
        btn = to_string_dec_uint(buttonid);
        return;
    }
    if (entry_.sensorType == FPC_VW) {
        // uint32_t key_high = (entry_.data >> 32) & 0xFFFFFFFF;
        uint32_t key_low = entry_.data & 0xFFFFFFFF;
        serial = key_low;  // trimmed to 32 bits for VW
        uint8_t check = entry_.data2 & 0xFF;
        uint8_t btnid = (check >> 4) & 0xF;
        switch (btnid) {
            case 0x1:
                btn = "UNLOCK";
                break;
            case 0x2:
                btn = "LOCK";
                break;
            case 0x3:
                btn = "Un+Lk";
                break;
            case 0x4:
                btn = "TRUNK";
                break;
            case 0x5:
                btn = "Un+Tr";
                break;
            case 0x6:
                btn = "Lk+Tr";
                break;
            case 0x7:
                btn = "Un+Lk+Tr";
                break;
            case 0x8:
                btn = "PANIC";
                break;
            default:
                btn = "Unknown";
                break;
        }
    }
    if (entry_.sensorType == FPC_SUBARU) {
        uint8_t* data_bytes = (uint8_t*)entry_.data;
        serial = ((uint32_t)data_bytes[1] << 16) | ((uint32_t)data_bytes[2] << 8) | data_bytes[3];
        uint8_t button = data_bytes[0] & 0x0F;
        btn = to_string_dec_uint(button);
        uint16_t cnttmp = 0;
        subaru_decode_count(data_bytes, &cnttmp);
        cnt = cnttmp;
    }
    return;
}

}  // namespace ui