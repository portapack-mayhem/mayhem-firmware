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
#include "file_path.hpp"
#include "portapack_persistent_memory.hpp"
#include "../baseband/fprotos/fprotogeneral.hpp"

using namespace portapack;
using namespace ui;

namespace pmem = portapack::persistent_memory;

namespace ui {

std::string WeatherRecentEntry::to_csv() {
    std::string csv = ";";
    csv += WeatherView::getWeatherSensorTypeName((FPROTO_WEATHER_SENSOR)sensorType);
    csv += ";" + to_string_dec_uint(id) + ";";
    csv += to_string_decimal(temp, 2) + ";";
    csv += to_string_dec_uint(humidity) + ";";
    csv += to_string_dec_uint(channel) + ";";
    csv += to_string_dec_uint(battery_low);
    return csv;
}

void WeatherLogger::log_data(WeatherRecentEntry& data) {
    log_file.write_entry(data.to_csv());
}

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
                  &channel,
                  &field_rf_amp,
                  &field_lna,
                  &field_vga,
                  &field_volume,
                  &field_frequency,
                  &options_temperature,
                  &button_clear_list,
                  &check_log,
                  &recent_entries_view});

    logger = std::make_unique<WeatherLogger>();

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

    check_log.on_select = [this](Checkbox&, bool v) {
        logging = v;
        if (logger && logging) {
            logger->append(logs_dir.string() + "/WEATHERLOG_" + to_string_timestamp(rtc_time::now()) + ".CSV");
            logger->write_header();
        }
    };
    check_log.set_value(logging);

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
    WeatherRecentEntry key = process_data(data);
    if (logger && logging) {
        logger->log_data(key);
    }
    // WeatherRecentEntry key{data->sensorType, data->id, data->temp, data->humidity, data->channel, data->battery_low};
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

uint16_t WeatherView::bcd_decode_short(uint32_t data) {
    return (data & 0xF) * 10 + ((data >> 4) & 0xF);
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
        case FPW_OREGON2B:
            return "Oregon2B";
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
        case FPW_SolightTE44:
            return "SolightTE44";
        case FPW_Bresser3CH:
        case FPW_Bresser3CH_V1:
            return "Bresser3CH";
        case FPW_Vauno_EN8822:
            return "Vauno EN8822";
        case FPW_Invalid:
        default:
            return "Unknown";
    }
}

std::string WeatherView::pad_string_with_spaces(int snakes) {
    std::string paddedStr(snakes, ' ');
    return paddedStr;
}

void WeatherView::on_freqchg(int64_t freq) {
    field_frequency.set_value(freq);
}

template <>
void RecentEntriesTable<ui::WeatherRecentEntries>::draw(
    const Entry& entry,
    const Rect& target_rect,
    Painter& painter,
    const Style& style,
    RecentEntriesColumns& columns) {
    std::string line{};
    line.reserve(30);

    line = WeatherView::getWeatherSensorTypeName((FPROTO_WEATHER_SENSOR)entry.sensorType);
    line.resize(columns.at(0).second, ' ');

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

#define LACROSSE_TX_MSG_TYPE_TEMP 0x00
#define LACROSSE_TX_MSG_TYPE_HUM 0x0E

WeatherRecentEntry WeatherView::process_data(const WeatherDataMessage* data) {
    WeatherRecentEntry ret = {};
    ret.sensorType = data->sensorType;
    int16_t i16 = 0;
    int32_t i32 = 0;
    uint16_t u16 = 0;

    uint64_t u64 = 0;
    uint8_t u8 = 0;
    uint8_t channel_3021[] = {3, 0, 2, 1};
    float flo = 0.0;
    switch (data->sensorType) {
        case FPW_NexusTH:
            ret.id = (data->decode_data >> 28) & 0xFF;
            ret.battery_low = !((data->decode_data >> 27) & 1);
            ret.channel = ((data->decode_data >> 24) & 0x03) + 1;
            if (!((data->decode_data >> 23) & 1)) {
                ret.temp = (float)((data->decode_data >> 12) & 0x07FF) / 10.0f;
            } else {
                ret.temp = (float)((~(data->decode_data >> 12) & 0x07FF) + 1) / -10.0f;
            }
            ret.humidity = data->decode_data & 0xFF;
            if (ret.humidity > 95)
                ret.humidity = 95;
            else if (ret.humidity < 20)
                ret.humidity = 20;
            break;
        case FPW_Acurite592TXR:

            u8 = ((data->decode_data >> 54) & 0x03);
            ret.channel = channel_3021[u8];
            ret.id = (data->decode_data >> 40) & 0x3FFF;
            ret.battery_low = !((data->decode_data >> 38) & 1);
            ret.humidity = (data->decode_data >> 24) & 0x7F;
            u16 = ((data->decode_data >> 9) & 0xF80) | ((data->decode_data >> 8) & 0x7F);
            ret.temp = ((float)(u16)-1000) / 10.0f;
            break;
        case FPW_Acurite606TX:
            ret.id = (data->decode_data >> 24) & 0xFF;
            ret.battery_low = (data->decode_data >> 23) & 1;
            if (!((data->decode_data >> 19) & 1)) {
                ret.temp = (float)((data->decode_data >> 8) & 0x07FF) / 10.0f;
            } else {
                ret.temp = (float)((~(data->decode_data >> 8) & 0x07FF) + 1) / -10.0f;
            }
            break;
        case FPW_Acurite609TX:
            ret.id = (data->decode_data >> 32) & 0xFF;
            ret.battery_low = (data->decode_data >> 31) & 1;
            ret.channel = WS_NO_CHANNEL;
            // Temperature in Celsius is encoded as a 12 bit integer value
            // multiplied by 10 using the 4th - 6th nybbles (bytes 1 & 2)
            // negative values are recovered by sign extend from int16_t.
            i16 = (int16_t)(((data->decode_data >> 12) & 0xf000) | ((data->decode_data >> 16) << 4));
            ret.temp = (i16 >> 4) * 0.1f;
            ret.humidity = (data->decode_data >> 8) & 0xff;
            break;
        case FPW_Ambient:
            id = (data->decode_data >> 32) & 0xFF;
            ret.battery_low = (data->decode_data >> 31) & 1;
            ret.channel = ((data->decode_data >> 28) & 0x07) + 1;
            ret.temp = FProtoGeneral::locale_fahrenheit_to_celsius(((float)((data->decode_data >> 16) & 0x0FFF) - 400.0f) / 10.0f);
            ret.humidity = (data->decode_data >> 8) & 0xFF;
            break;
        case FPW_AuriolAhfl:
            ret.id = data->decode_data >> 34;
            ret.battery_low = (data->decode_data >> 33) & 1;
            // btn = (data >> 32) & 1;
            ret.channel = ((data->decode_data >> 30) & 0x3) + 1;
            if (!((data->decode_data >> 29) & 1)) {
                ret.temp = (float)((data->decode_data >> 18) & 0x07FF) / 10.0f;
            } else {
                ret.temp = (float)((~(data->decode_data >> 18) & 0x07FF) + 1) / -10.0f;
            }
            ret.humidity = (data->decode_data >> 11) & 0x7F;
            break;
        case FPW_AuriolTH:
            ret.id = (data->decode_data >> 31) & 0xFF;
            ret.battery_low = ((data->decode_data >> 30) & 1);
            ret.channel = ((data->decode_data >> 25) & 0x03) + 1;
            if (!((data->decode_data >> 23) & 1)) {
                ret.temp = (float)((data->decode_data >> 13) & 0x07FF) / 10.0f;
            } else {
                ret.temp = (float)((~(data->decode_data >> 13) & 0x07FF) + 1) / -10.0f;
            }
            ret.humidity = (data->decode_data >> 1) & 0x7F;
            break;
        case FPW_GTWT02:
            ret.id = (data->decode_data >> 29) & 0xFF;
            ret.battery_low = (data->decode_data >> 28) & 1;
            // btn = (data->decode_data >> 27) & 1;
            ret.channel = ((data->decode_data >> 25) & 0x3) + 1;

            if (!((data->decode_data >> 24) & 1)) {
                ret.temp = (float)((data->decode_data >> 13) & 0x07FF) / 10.0f;
            } else {
                ret.temp = (float)((~(data->decode_data >> 13) & 0x07FF) + 1) / -10.0f;
            }
            ret.humidity = (data->decode_data >> 6) & 0x7F;
            if (ret.humidity <= 10)  // actually the sensors sends 10 below working range of 20%
                ret.humidity = 0;
            else if (ret.humidity > 90)  // actually the sensors sends 110 above working range of 90%
                ret.humidity = 100;
            break;
        case FPW_GTWT03:
            ret.id = data->decode_data >> 33;
            ret.humidity = (data->decode_data >> 25) & 0xFF;
            if (ret.humidity <= 10) {  // actually the sensors sends 10 below working range of 20%
                ret.humidity = 0;
            } else if (ret.humidity > 95) {  // actually the sensors sends 110 above working range of 90%
                ret.humidity = 100;
            }
            ret.battery_low = (data->decode_data >> 24) & 1;
            // (data->decode_data >> 23) & 1;
            ret.channel = ((data->decode_data >> 21) & 0x03) + 1;
            if (!((data->decode_data >> 20) & 1)) {
                ret.temp = (float)((data->decode_data >> 9) & 0x07FF) / 10.0f;
            } else {
                ret.temp = (float)((~(data->decode_data >> 9) & 0x07FF) + 1) / -10.0f;
            }
            break;
        case FPW_INFACTORY:
            ret.id = data->decode_data >> 32;
            ret.battery_low = (data->decode_data >> 26) & 1;
            ret.temp = FProtoGeneral::locale_fahrenheit_to_celsius(((float)((data->decode_data >> 12) & 0x0FFF) - 900.0f) / 10.0f);
            ret.humidity = (((data->decode_data >> 8) & 0x0F) * 10) + ((data->decode_data >> 4) & 0x0F);  // BCD, 'A0'=100%rH
            ret.channel = data->decode_data & 0x03;
            break;
        case FPW_LACROSSETX:
            u8 = (data->decode_data >> 32) & 0x0F;
            ret.id = (((data->decode_data >> 28) & 0x0F) << 3) | (((data->decode_data >> 24) & 0x0F) >> 1);
            flo = (float)((data->decode_data >> 20) & 0x0F) * 10.0f + (float)((data->decode_data >> 16) & 0x0F) + (float)((data->decode_data >> 12) & 0x0F) * 0.1f;
            if (u8 == LACROSSE_TX_MSG_TYPE_TEMP) {  //-V1051
                ret.temp = flo - 50.0f;
                ret.humidity = WS_NO_HUMIDITY;
            } else if (u8 == LACROSSE_TX_MSG_TYPE_HUM) {
                // ToDo for verification, records are needed with sensors maintaining temperature and temperature for this standard
                ret.humidity = (uint8_t)flo;
            }
            break;
        case FPW_LACROSSETX141thbv2:
            ret.id = data->decode_data >> 32;
            ret.battery_low = (data->decode_data >> 31) & 1;
            // btn = (data->decode_data >> 30) & 1;
            ret.channel = ((data->decode_data >> 28) & 0x03) + 1;
            ret.temp = ((float)((data->decode_data >> 16) & 0x0FFF) - 500.0f) / 10.0f;
            ret.humidity = (data->decode_data >> 8) & 0xFF;
            break;
        case FPW_OREGON2:
            i32 = bcd_decode_short(data->decode_data >> 4);
            i32 *= 10;
            i32 += (data->decode_data >> 12) & 0xf;
            if (data->decode_data & 0xF) i32 = -i32;
            ret.temp = (float)i32 / 10.0;
            ret.battery_low = ((data->decode_data >> 32) & 0x4) ? 1 : 0;
            u8 = (data->decode_data >> 44) & 0xF;
            ret.channel = 1;
            while (u8 > 1) {
                ret.channel++;
                u8 >>= 1;
            }
            break;
        case FPW_OREGON2B:
            ret.humidity = bcd_decode_short(data->decode_data);
            u64 = data->decode_data >> 8;
            i32 = bcd_decode_short(u64 >> 4);
            i32 *= 10;
            i32 += (u64 >> 12) & 0xf;
            if (u64 & 0xF) i32 = -i32;
            ret.temp = (float)i32 / 10.0;
            ret.battery_low = ((data->decode_data >> 32) & 0x4) ? 1 : 0;
            u8 = (data->decode_data >> 44) & 0xF;
            ret.channel = 1;
            while (u8 > 1) {
                ret.channel++;
                u8 >>= 1;
            }
            break;
        case FPW_OREGON3:
            // todo check
            ret.humidity = ((data->decode_data >> 4) & 0xF) * 10 + (((data->decode_data >> 4) >> 4) & 0xF);
            i16 = (((data->decode_data >> 12) >> 4) & 0xF) * 10 + ((((data->decode_data >> 12) >> 4) >> 4) & 0xF);  // ws_oregon3_bcd_decode_short((data->decode_data >> 12) >> 4);
            i16 *= 10;
            i16 += ((data->decode_data >> 12) >> 12) & 0xF;
            if ((data->decode_data >> 12) & 0xF) i16 = -i16;
            ret.temp = (float)i16 / 10.0;
            break;
        case FPW_OREGONv1:
            u64 = FProtoGeneral::subghz_protocol_blocks_reverse_key(data->decode_data, 32);
            id = u64 & 0xFF;
            ret.channel = ((u64 >> 6) & 0x03) + 1;
            ret.temp = ((u64 >> 8) & 0x0F) * 0.1f + ((u64 >> 12) & 0x0F) + ((u64 >> 16) & 0x0F) * 10.0f;
            if (!((u64 >> 21) & 1)) {
                ret.temp = ret.temp;
            } else {
                ret.temp = -ret.temp;
            }
            ret.battery_low = !((u64 >> 23) & 1ULL);
            break;
        case FPW_THERMOPROTX4:
            ret.id = (data->decode_data >> 25) & 0xFF;
            ret.battery_low = (data->decode_data >> 24) & 1;
            // btn = (data->decode_data >> 23) & 1;
            ret.channel = ((data->decode_data >> 21) & 0x03) + 1;
            if (!((data->decode_data >> 20) & 1)) {
                ret.temp = (float)((data->decode_data >> 9) & 0x07FF) / 10.0f;
            } else {
                ret.temp = (float)((~(data->decode_data >> 9) & 0x07FF) + 1) / -10.0f;
            }
            ret.humidity = (data->decode_data >> 1) & 0xFF;
            break;
        case FPW_TX_8300:
            ret.humidity = (((data->decode_data >> 28) & 0x0F) * 10) + ((data->decode_data >> 24) & 0x0F);
            if (!((data->decode_data >> 22) & 0x03))
                ret.battery_low = 0;
            else
                ret.battery_low = 1;
            ret.channel = (data->decode_data >> 20) & 0x03;
            ret.id = (data->decode_data >> 12) & 0x7F;
            ret.temp = ((data->decode_data >> 8) & 0x0F) * 10.0f + ((data->decode_data >> 4) & 0x0F) +
                       (data->decode_data & 0x0F) * 0.1f;
            if (!((data->decode_data >> 19) & 1)) {
                ret.temp = ret.temp;
            } else {
                ret.temp = -ret.temp;
            }
            break;
        case FPW_WENDOX_W6726:
            ret.id = (data->decode_data >> 24) & 0xFF;
            ret.battery_low = (data->decode_data >> 6) & 1;
            if (((data->decode_data >> 23) & 1)) {
                ret.temp = (float)(((data->decode_data >> 14) & 0x1FF) + 12) / 10.0f;
            } else {
                ret.temp = (float)((~(data->decode_data >> 14) & 0x1FF) + 1 - 12) / -10.0f;
            }
            if (ret.temp < -50.0f) {
                ret.temp = -50.0f;
            } else if (ret.temp > 70.0f) {
                ret.temp = 70.0f;
            }
            break;
        case FPW_Acurite986:
            ret.id = FProtoGeneral::subghz_protocol_blocks_reverse_key(data->decode_data >> 24, 8);
            ret.id = (id << 8) | FProtoGeneral::subghz_protocol_blocks_reverse_key(data->decode_data >> 16, 8);
            ret.battery_low = (data->decode_data >> 14) & 1;
            ret.channel = ((data->decode_data >> 15) & 1) + 1;
            i16 = FProtoGeneral::subghz_protocol_blocks_reverse_key(data->decode_data >> 32, 8);
            if (i16 & 0x80) {
                i16 = -(i16 & 0x7F);
            }
            ret.temp = FProtoGeneral::locale_fahrenheit_to_celsius((float)i16);
            break;
        case FPW_KEDSUM:
            ret.id = data->decode_data >> 32;
            if ((data->decode_data >> 30) & 0x3) {
                ret.battery_low = 0;
            } else {
                ret.battery_low = 1;
            }
            ret.channel = ((data->decode_data >> 28) & 0x3) + 1;
            u16 = ((data->decode_data >> 16) & 0x0f) << 8 | ((data->decode_data >> 20) & 0x0f) << 4 | ((data->decode_data >> 24) & 0x0f);
            ret.temp = FProtoGeneral::locale_fahrenheit_to_celsius(((float)u16 - 900.0f) / 10.0f);
            ret.humidity = ((data->decode_data >> 8) & 0x0f) << 4 | ((data->decode_data >> 12) & 0x0f);
            break;
        case FPW_Acurite5in1:
            u8 = ((data->decode_data >> 62) & 0x03);
            ret.channel = channel_3021[u8];
            ret.id = (data->decode_data >> 48) & 0x3FFF;
            ret.battery_low = !((data->decode_data >> 46) & 1);
            ret.humidity = (data->decode_data >> 8) & 0x7F;
            u16 = ((data->decode_data >> (24 - 7)) & 0x780) | ((data->decode_data >> 16) & 0x7F);
            ret.temp = FProtoGeneral::locale_fahrenheit_to_celsius(((float)(u16)-400) / 10.0f);
            break;
        case FPW_EmosE601x:
            ret.id = 0xffff;  //(upper_decode_data >> 24) & 0xff; //todo maybe with oregon, and +64bit usage/message
            ret.battery_low = (data->decode_data >> 10) & 1;
            i16 = (data->decode_data >> 40) & 0xfff;
            /* Handle signed data */
            if (i16 & 0x800) {
                i16 |= 0xf000;
            }
            ret.temp = (float)i16 / 10.0;
            ret.humidity = (data->decode_data >> 32) & 0xff;
            ret.channel = (data->decode_data >> 52) & 0x03;
            break;
        case FPW_SolightTE44:
            ret.id = (data->decode_data >> 28) & 0xff;
            ret.battery_low = !(data->decode_data >> 27) & 0x01;
            ret.channel = ((data->decode_data >> 24) & 0x03) + 1;
            i16 = (data->decode_data >> 12) & 0x0fff;
            /* Handle signed data */
            if (i16 & 0x0800) {
                i16 |= 0xf000;
            }
            ret.temp = (float)i16 / 10.0;
            break;
        case FPW_Bresser3CH:
            ret.id = (data->decode_data >> 28) & 0xff;
            ret.channel = ((data->decode_data >> 27) & 0x01) | (((data->decode_data >> 26) & 0x01) << 1);
            // ret.btn = ((data->decode_data >> 25) & 0x1);
            ret.battery_low = ((data->decode_data >> 24) & 0x1);
            i16 = (data->decode_data >> 12) & 0x0fff;
            /* Handle signed data */
            if (i16 & 0x0800) {
                i16 |= 0xf000;
            }
            ret.temp = (float)i16 / 10.0;
            ret.humidity = data->decode_data & 0xff;
            break;
        case FPW_Bresser3CH_V1:
            ret.id = (data->decode_data >> 32) & 0xff;
            ret.battery_low = ((data->decode_data >> 31) & 0x1);
            // ret.btn = (data->decode_data >> 30) & 0x1;
            ret.channel = (data->decode_data >> 28) & 0x3;
            ret.temp = (data->decode_data >> 16) & 0xfff;
            ret.temp = FProtoGeneral::locale_fahrenheit_to_celsius((float)(ret.temp - 900) / 10.0);
            ret.humidity = (data->decode_data >> 8) & 0xff;
            break;

        case FPW_Vauno_EN8822:
            ret.id = (data->decode_data >> 34) & 0xff;
            ret.battery_low = (data->decode_data >> 33) & 0x01;
            ret.channel = ((data->decode_data >> 30) & 0x03);
            i16 = (data->decode_data >> 18) & 0x0fff;
            /* Handle signed data */
            if (i16 & 0x0800) {
                i16 |= 0xf000;
            }
            ret.temp = (float)i16 / 10.0;
            ret.humidity = (data->decode_data >> 11) & 0x7f;

            break;
        case FPW_Invalid:
        default:
            break;
    }

    return ret;
}

}  // namespace ui
