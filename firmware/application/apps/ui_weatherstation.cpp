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

// the values only some sensors have. Same order in the log and in the detail view
struct WeatherExtraValue {
    const char* label;
    const char* unit;
    uint32_t value;
    uint32_t none;
    bool tenths;
};

// Arrays with a flat solar panel that has to face the equator. In the southern hemisphere they are installed
// turned around, so S.Hem rotates their wind direction. The WS80 / WS90 have a ring shaped panel, they are not turned.
static bool weather_panel_faces_equator(uint8_t type) {
    return type == FPW_FO_WH24 || type == FPW_FO_WH65 || type == FPW_FO_WS69 || type == FPW_FO_WS68;
}

static uint8_t weather_extra_values(const WeatherReadings& r, WeatherExtraValue* v) {
    v[0] = {"Wind:     ", " m/s", r.wind_avg, WS_NO_U16, true};
    uint16_t wind_dir = r.wind_dir;
    if (weather_wind_south && wind_dir != WS_NO_U16 && weather_panel_faces_equator(r.sensorType)) wind_dir = (wind_dir + 180) % 360;
    v[1] = {"Wind dir: ", " deg", wind_dir, WS_NO_U16, false};
    v[2] = {"Gust:     ", " m/s", r.wind_gust, WS_NO_U16, true};
    v[3] = {"Rain:     ", " mm", r.rain, WS_NO_U32, true};
    v[4] = {"Pressure: ", " hPa", r.pressure, WS_NO_U16, true};
    v[5] = {"UV index: ", "", r.uv, WS_NO_U8, true};
    v[6] = {"Light:    ", " lux", r.light, WS_NO_U32, false};
    v[7] = {"Moisture: ", "%", r.moisture, WS_NO_U8, false};
    v[8] = {"PM2.5:    ", " ug/m3", r.pm25, WS_NO_U16, true};
    v[9] = {"Storm:    ", " km", r.storm_dist, WS_NO_U8, false};
    v[10] = {"Strikes:  ", "", r.strikes, WS_NO_U16, false};
    v[11] = {"Leak:     ", "", r.leak, WS_NO_U8, false};
    return 12;
}

static std::string weather_extra_to_string(const WeatherExtraValue& v) {
    if (v.value == v.none) return "";
    return v.tenths ? to_string_decimal(v.value / 10.0f, 1) : to_string_dec_uint(v.value);
}

std::string WeatherRecentEntry::to_csv() {
    std::string csv = ";";
    csv += WeatherView::getWeatherSensorTypeName((FPROTO_WEATHER_SENSOR)sensorType);
    csv += ";" + to_string_dec_uint(id) + ";";
    csv += to_string_decimal(temp, 2) + ";";
    csv += to_string_dec_uint(humidity) + ";";
    csv += to_string_dec_uint(channel) + ";";
    csv += to_string_dec_uint(battery_low);
    // extended values, empty if the sensor doesn't have them
    WeatherExtraValue v[12];
    uint8_t count = weather_extra_values(*this, v);
    for (uint8_t i = 0; i < count; i++) csv += ";" + weather_extra_to_string(v[i]);
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

    // only the values the sensor has
    WeatherExtraValue v[12];
    uint8_t count = weather_extra_values(entry_, v);
    uint8_t line = 0;
    for (uint8_t i = 0; i < count && line < extra_lines; i++) {
        if (v[i].value != v[i].none) text_extra[line++].set(v[i].label + weather_extra_to_string(v[i]) + v[i].unit);
    }
    while (line < extra_lines) text_extra[line++].set("");
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
    for (uint8_t i = 0; i < extra_lines; i++) {
        text_extra[i].set_parent_rect({UI_POS_X(0), (8 + i) * 16, screen_width, 16});
        add_child(&text_extra[i]);
    }

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
                  &check_wind_south,
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

    check_wind_south.on_select = [](Checkbox&, bool v) {
        weather_wind_south = v;
    };
    check_wind_south.set_value(weather_wind_south);

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
        *matching_recent = key;  // the key doesn't cover all the values (wind, rain...), take the new ones
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
        case FPW_FO_WH2:
            return "FO-WH2";
        case FPW_FO_WH5:
            return "FO-WH5";
        case FPW_FO_WH0530:
            return "FO-WH0530";
        case FPW_FO_WH1080:
            return "FO-WH1080";
        case FPW_FO_WH3080UV:
            return "FO-WH3080UV";
        case FPW_FO_WH53:
            return "Ecowitt WH53";
        case FPW_FO_WH24:
            return "FO-WH24";
        case FPW_FO_WH65:
            return "FO-WH65";
        case FPW_FO_WS69:
            return "FO-WS69";
        case FPW_FO_WH25:
            return "FO-WH25";
        case FPW_FO_WH32:
            return "FO-WH32";
        case FPW_FO_WH0290:
            return "FO-WH0290";
        case FPW_FO_WH31E:
            return "FO-WH31";
        case FPW_FO_WH40:
            return "FO-WH40";
        case FPW_FO_WN20:
            return "FO-WN20";
        case FPW_FO_WS68:
            return "FO-WS68";
        case FPW_FO_WH51:
            return "FO-WH51";
        case FPW_FO_WS80:
            return "FO-WS80";
        case FPW_FO_WS85:
            return "FO-WS85";
        case FPW_FO_WS90:
            return "FO-WS90";
        case FPW_FO_WN34:
            return "FO-WN34";
        case FPW_FO_WH57:
            return "FO-WH57";
        case FPW_FO_WH55:
            return "FO-WH55";
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

    std::string temp = (entry.temp == WS_NO_TEMPERATURE) ? "-" : (weather_units_fahr ? to_string_decimal((entry.temp * 9 / 5) + 32, 1) : to_string_decimal(entry.temp, 1));
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

WeatherRecentEntry WeatherView::process_data(const WeatherDataMessage* data) {
    WeatherRecentEntry ret = {};
    weather_decode_all(data->sensorType, data->decode_data, data->payload, data->payload_len, ret);
    return ret;
}

}  // namespace ui
