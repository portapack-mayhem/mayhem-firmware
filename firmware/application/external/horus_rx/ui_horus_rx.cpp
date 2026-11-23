/*
 * Copyright (C) 2026 OE1LBS
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

#include "ui_horus_rx.hpp"

#include "audio.hpp"
#include "baseband_api.hpp"
#include "string_format.hpp"

using namespace portapack;

namespace ui::external_app::horus_rx {

namespace {

// Fixed-point formatting (v / div with the given decimals, truncated), so the M0 needs
// no float for the common fields.
std::string fixed(int32_t v, uint32_t div, int decimals) {
    std::string s = v < 0 ? "-" : "";
    const uint32_t a = (uint32_t)(v < 0 ? -v : v);
    s += to_string_dec_uint(a / div);
    if (decimals > 0) {
        uint32_t p = 1;
        for (int i = 0; i < decimals; i++) p *= 10;
        s += "." + to_string_dec_uint((uint64_t)(a % div) * p / div, decimals, '0');
    }
    return s;
}

std::string hms(int32_t t) {
    if (t < 0) return "--:--:--";
    return to_string_dec_uint(t / 3600, 2, '0') + ":" + to_string_dec_uint((t / 60) % 60, 2, '0') + ":" +
           to_string_dec_uint(t % 60, 2, '0') + "Z";
}

std::string extra_text(const ExtraSensor& e) {
    std::string s = e.name[0] ? std::string{e.name} + ":" : "?:";
    switch (e.kind) {
        case ExtraKind::Str:
            s += e.str;
            break;
        case ExtraKind::Int:
            for (int i = 0; i < e.count; i++) s += " " + to_string_dec_int(e.v[i].mant);
            break;
        case ExtraKind::Real:
            for (int i = 0; i < e.count; i++) {
                float f = (float)e.v[i].mant;
                int x = e.v[i].exp;
                for (; x > 0; x--) f *= 2.0f;
                for (; x < 0; x++) f *= 0.5f;
                s += " " + to_string_decimal(f, 2);
            }
            break;
        case ExtraKind::Bool:
            // b0..b7 as 0/1 (written out here: to_string_bin is not linked into stock builds)
            s += ' ';
            for (int b = 7; b >= 0; b--) s += ((e.v[0].mant >> b) & 1) ? '1' : '0';
            break;
        default:
            break;
    }
    return s;
}

}  // namespace

void HorusRxView::focus() {
    field_frequency.focus();
}

HorusRxView::~HorusRxView() {
    receiver_model.disable();
    audio::output::stop();
    baseband::shutdown();
}

HorusRxView::HorusRxView(NavigationView& nav)
    : nav_{nav} {
    add_children({&field_frequency,
                  &field_rf_amp,
                  &field_lna,
                  &field_vga,
                  &rssi,
                  &channel,
                  &field_volume,
                  &labels,
                  &text_status,
                  &text_count,
                  &button_map,
                  &text_type,
                  &text_id,
                  &text_time,
                  &text_voltage,
                  &text_frame,
                  &text_temp,
                  &text_humid,
                  &text_press,
                  &text_vspeed,
                  &text_link,
                  &geopos,
                  &text_extra,
                  &console});

    field_frequency.set_step(1000);
    text_status.set_style(Theme::getInstance()->fg_red);
    geopos.set_read_only(true);
    // small font as in POCSAG / FLEX: five history entries instead of two
    console.set_style(Theme::getInstance()->bg_darkest_small);

    button_map.hidden(true);
    button_map.on_select = [this](Button&) {
        open_map();
    };

    baseband::run_image(portapack::spi_flash::image_tag_horus_rx);

    /* set_modulation() is not called: it would replace the Horus baseband image. */
    receiver_model.set_sampling_rate(sampling_rate);
    receiver_model.set_baseband_bandwidth(baseband_bandwidth);
    receiver_model.enable();

    audio::output::start();
}

void HorusRxView::on_status(const HorusRxStatusMessage* m) {
    if (status_since_packet != 0xFFFFFFFF) status_since_packet++;

    const bool signal = m->signal_x10 >= 60;           // tones at least 6 dB over the spectrum floor
    const bool recent = status_since_packet < 2 * 20;  // a packet within the last 20 s

    if (!m->tones_valid) {
        text_status.set_style(Theme::getInstance()->fg_red);
        text_status.set("Searching");
    } else if (recent) {
        text_status.set_style(Theme::getInstance()->fg_green);
        text_status.set(signal ? "Receiving" : "Locked, idle");
    } else if (signal) {
        text_status.set_style(Theme::getInstance()->fg_yellow);
        text_status.set("Heard, no decode");
    } else {
        text_status.set_style(Theme::getInstance()->fg_light);
        text_status.set("Waiting");
    }

    if (m->tones_valid) {
        const int centre = (m->tone_hz[0] + m->tone_hz[1] + m->tone_hz[2] + m->tone_hz[3]) / 4;
        const int spacing = (m->tone_hz[3] - m->tone_hz[0]) / 3;
        // tone offset, tone spacing, tone power over the noise floor
        text_link.set(std::string(centre >= 0 ? "+" : "") + to_string_dec_int(centre) + "Hz " +
                      to_string_dec_int(spacing) + "Hz " + fixed(m->signal_x10, 10, 0) + "dB");
    }
}

void HorusRxView::on_packet(const HorusPacketMessage* m) {
    Telemetry t;
    if (!decode_payload(m->data, m->size, m->version, t)) {
        console.writeln(STR_COLOR_RED "v" + to_string_dec_uint(m->version) + " " + to_string_dec_uint(m->size) +
                        "B: CRC ok, content not decodable");
        return;
    }
    packets_total++;
    status_since_packet = 0;
    text_count.set(to_string_dec_uint(packets_total) + " RX");
    show_telemetry(t, *m);
}

void HorusRxView::show_telemetry(const Telemetry& t, const HorusPacketMessage& m) {
    const std::string call = t.version == 3 ? std::string{t.callsign} : "ID " + to_string_dec_uint(t.payload_id);
    const std::string none = "---";

    text_type.set("Horus v" + to_string_dec_uint(t.version) + " " + to_string_dec_uint(m.size) + "B");
    text_id.set_style(Theme::getInstance()->fg_green);
    text_id.set(call);

    std::string time = hms(t.time_of_day);
    if (t.has_sats) time += "  " + to_string_dec_uint(t.sats) + " sats";
    text_time.set(time);

    text_voltage.set(t.has_mv[0] ? fixed(t.mv[0], 1000, 2) + "V" : none);
    text_frame.set("#" + to_string_dec_uint(t.sequence) + "  SNR " + fixed(m.snr_x10, 10, 1) + "dB");

    // external temperature, then internal (Ti) and the first custom one (T1) if sent
    std::string temp = t.has_temp[1] ? fixed(t.temp_x10[1], 10, 1) + "C" : none;
    if (t.has_temp[0]) temp += "  Ti " + fixed(t.temp_x10[0], 10, 0);
    if (t.has_temp[2]) temp += " T1 " + fixed(t.temp_x10[2], 10, 0);
    text_temp.set(temp);

    text_humid.set(t.has_humidity ? to_string_dec_uint(t.humidity) + "%" : none);
    // 0 hPa is what payloads without a pressure sensor (e.g. RS41-SG) send
    text_press.set(t.has_pressure && t.pressure_x10 > 0 ? fixed(t.pressure_x10, 10, 1) + "hPa" : none);
    text_vspeed.set(t.has_ascent ? fixed(t.ascent_cms, 100, 1) + "m/s" : none);

    std::string extra;
    if (t.extra_count > 0) extra = "   Extra: " + extra_text(t.extra[0]);
    if (t.extra_count > 1) extra += "  " + extra_text(t.extra[1]);
    text_extra.set(extra);

    // history: time, callsign, sequence, altitude
    std::string line = hms(t.time_of_day).substr(0, 8) + " " + call + " #" + to_string_dec_uint(t.sequence) + " ";
    line += t.alt_m == -1000 ? std::string{"---"} : to_string_dec_int(t.alt_m) + "m";
    if (m.bits_corrected) line += STR_COLOR_YELLOW " fec" + to_string_dec_uint(m.bits_corrected);
    console.writeln(line);

    // Map: payloads without a fix send 0/0, which is not worth showing.
    if (t.lat_e5 != 0 || t.lon_e5 != 0) {
        map_lat = (float)t.lat_e5 / 100000.0f;
        map_lon = (float)t.lon_e5 / 100000.0f;
        map_alt = t.alt_m == -1000 ? 0 : t.alt_m;
        map_speed = t.has_speed ? t.speed_kmh : 0;
        geopos.set_lat(map_lat);
        geopos.set_lon(map_lon);
        geopos.set_altitude(map_alt);
        geopos.set_speed(map_speed);
        if (geomap_view && map_tag != call) geomap_view->update_tag(call);
        map_tag = call;
        button_map.hidden(false);
        if (geomap_view) geomap_view->update_position(map_lat, map_lon, 0, map_alt, map_speed);
    }
}

void HorusRxView::open_map() {
    /* The close callback is a lambda of this app on purpose: every template the push
     * instantiates then carries this namespace and links into the app image, so the app
     * adds nothing to the main firmware and stays loadable on a stock build. */
    geomap_view = nav_.push<GeoMapView>(
        map_tag,
        map_alt,
        GeoPos::alt_unit::METERS,
        GeoPos::spd_unit::KMPH,
        map_lat,
        map_lon,
        0,
        [this]() {
            geomap_view = nullptr;
        });
}

}  // namespace ui::external_app::horus_rx
