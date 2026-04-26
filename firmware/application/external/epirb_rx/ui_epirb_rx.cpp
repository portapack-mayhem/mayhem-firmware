/*
 * Copyright (C) 2024 EPIRB Decoder Implementation
 * Copyright (C) 2026 Frederic BORRY - ADRASEC 31
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

#include "baseband_api.hpp"
#include "portapack_persistent_memory.hpp"
#include "file_path.hpp"

#include "ui_epirb_rx.hpp"

using namespace portapack;

#include "rtc_time.hpp"
#include "string_format.hpp"
#include "ui.hpp"

#include "message.hpp"
#include "resources.hpp"

// #include "usb_serial_asyncmsg.hpp"

namespace ui::external_app::epirb_rx {

// URL templates
#define MAPS_URL_TEMPLATE "https://www.google.com/maps/search/?api=1&query=%s%%2C%s"
#define BEACON_URL_TEMPALTE "https://decoder2.herokuapp.com/decoded/"

#ifndef DISABLE_COUNTRY_CACHE
int CountryManager::cache_count = 0;
Country CountryManager::cache[16];
#endif

TextArea::TextArea(
    Rect parent_rect)
    : Widget{parent_rect} {
}

void TextArea::paint(Painter& painter) {
    const auto rect = screen_rect();
    const Style& s = style();
    painter.fill_rectangle(rect, s.background);
    // We use \t as line separator since \n is used in STR_COLOR_GREEN
    auto rows = split_string(content, '\t');

    const int line_height = s.font.line_height();
    size_t line_idx = 0;
    for (auto row : rows) {
        painter.draw_string(rect.location() + Point(0, line_idx * line_height), s, row);
        line_idx++;
    }
}

void TextArea::set_content(std::string_view value) {
    content = std::string{value};
    set_dirty();
}

/*
std::string EPIRBAppView::beacon_to_hex_string(const baseband::Packet& packet) {
    const char hex[] = "0123456789ABCDEF";

    std::string out;
    out.resize(36);
    size_t frame_size = std::min(packet.size(), (size_t)144);

    for (size_t i = 0; i < frame_size / 8; i++) {
        uint8_t byte_val = 0;
        for (size_t bit = 0; bit < 8 && (i * 8 + bit) < frame_size; bit++) {
            if (packet[i * 8 + bit]) {
                byte_val |= (1 << (7 - bit));
            }
        }

        out[i * 2] = hex[byte_val >> 4];
        out[i * 2 + 1] = hex[byte_val & 0x0F];
    }

    return out;
}*/

void EPIRBAppView::decode_packet(const baseband::Packet& packet, Beacon& beacon) {
    // Convert packet bits to byte array for easier processing
    uint8_t data[18];
    for (size_t i = 0; i < std::min(packet.size() / 8, (size_t)BEACON_DATA_SIZE); i++) {
        uint8_t byte_val = 0;
        for (int bit = 0; bit < 8 && (i * 8 + bit) < packet.size(); bit++) {
            if (packet[i * 8 + bit]) {
                byte_val |= (1 << (7 - bit));
            }
        }
        data[i] = byte_val;
    }
    beacon.setFrame(data);
    // Set timestamp
    rtc::RTC datetime;
    rtcGetTime(&RTCD1, &datetime);
    beacon.date = datetime;
}
/*
void EPIRBLogger::on_packet(Beacon& beacon) {
    std::string entry = std::string(beacon.getType()) + "," +
                        beacon.hexId + "," +
                        beacon.getProtocolName();  // + ",";
                                                   // to_string_dec_uint(static_cast<uint8_t>(beacon.emergency_type)) + ",";
    UsbSerialAsyncmsg::asyncmsg("Log location");
    if (!beacon.location.isUnknown()) {
        entry += beacon.location.toString(Location::LocationFormat::DECIMAL);
    } else {
        entry += ",";
    }
    UsbSerialAsyncmsg::asyncmsg("Log country");
    entry += "," + beacon.country.toString() + "," +
             beacon.getSatus() + "\n";
    log_file.write_entry(beacon.date, entry);
}
*/

EPIRBDetailView::EPIRBDetailView(
    Rect parent_rect,
    ResourceManager& rm)
    : View(parent_rect), resource_manager(rm) {
    add_children({&text_beacon});
}

void EPIRBDetailView::set_beacon(Beacon& beacon) {
    char buffer[400];
    char* buffer_pointer = buffer;
    bool isReal = (beacon.frameMode == Beacon::FrameMode::NORMAL);
    buffer_pointer += sprintf(buffer_pointer, "%sBeacon:%s %s(%s%s%s) - ", STR_COLOR_CYAN, STR_COLOR_WHITE, beacon.getType(), isReal ? STR_COLOR_YELLOW : STR_COLOR_GREEN, isReal ? "Real" : "Test", STR_COLOR_WHITE);
    buffer_pointer += beacon.formatTime(buffer_pointer);
    buffer_pointer += sprintf(buffer_pointer, "\t%sProtocol:%s %s\t", STR_COLOR_CYAN, STR_COLOR_WHITE, beacon.getProtocolName());
    buffer_pointer += sprintf(buffer_pointer, "%s\t", resource_manager.get_protocol_description((uint8_t)beacon.protocol));
    if (beacon.hasAdditionalData) {
        buffer_pointer += sprintf(buffer_pointer, "%s\t", beacon.additionalData.c_str());
    }
    buffer_pointer += sprintf(buffer_pointer, "%sCountry:%s %s(%d) - %s\t%sLocation:%s ", STR_COLOR_CYAN, STR_COLOR_WHITE, beacon.country.alphaCode, beacon.country.code, beacon.country.shortName, STR_COLOR_CYAN, STR_COLOR_WHITE);
    buffer_pointer += beacon.location.toString(buffer_pointer, Location::LocationFormat::MAIDENHEAD_LOCATOR, 8);
    (*(buffer_pointer++)) = '\t';
    if (!beacon.location.isUnknown()) {
        buffer_pointer += beacon.location.toString(buffer_pointer, Location::LocationFormat::SEXAGESIMAL);
        (*(buffer_pointer++)) = '\t';
        buffer_pointer += beacon.location.toString(buffer_pointer, Location::LocationFormat::DECIMAL);
        (*(buffer_pointer++)) = '\t';
    }
    buffer_pointer += sprintf(buffer_pointer, "%sControl: %s%s", STR_COLOR_CYAN, beacon.isBch1Valid() ? STR_COLOR_GREEN : STR_COLOR_RED, beacon.isBch1Valid() ? "BCH1-OK" : "BCH1-KO");
    if (beacon.hasBch2) {
        buffer_pointer += sprintf(buffer_pointer, " %s%s", beacon.isBch2Valid() ? STR_COLOR_GREEN : STR_COLOR_RED, beacon.isBch2Valid() ? "BCH2-OK" : "BCH2-KO");
    }
    buffer_pointer += sprintf(buffer_pointer, "\t%sHex ID:%s %s\t", STR_COLOR_CYAN, STR_COLOR_WHITE, beacon.hexId);
    if (beacon.hasSerialNumber) {
        buffer_pointer += sprintf(buffer_pointer, "%sS/N:%s %s\t", STR_COLOR_CYAN, STR_COLOR_WHITE, beacon.serialNumber);
    }
    if (beacon.hasMainLocatingDevice()) {
        buffer_pointer += sprintf(buffer_pointer, "%sMain loc. dev.:%s %s\t", STR_COLOR_CYAN, STR_COLOR_WHITE, beacon.getMainLocatingDeviceName());
        if (beacon.hasAuxLocatingDevice()) {
            sprintf(buffer_pointer, "%sAux loc. dev.:%s %s\t", STR_COLOR_CYAN, STR_COLOR_WHITE, beacon.getAuxLocatingDeviceName());
        }
    }
    text_beacon.set_content(buffer);
}

EPIRBMapView::EPIRBMapView(
    Rect parent_rect)
    : View(parent_rect) {
    add_children({&geomap});
    geomap.set_mode(DISPLAY);
    geomap.set_manual_panning(false);
    geomap.init();
    geomap.set_focusable(true);
    geomap.clear_markers();
    geomap.move(lon_, lat_);
    // Hide for now
    geomap.hidden(true);
}

void EPIRBMapView::set_main_marker(const std::string& label, float lat, float lon) {
    geomap.set_tag(label);
    lat_ = lat;
    lon_ = lon;
    geomap.move(lon_, lat_);
}

void EPIRBMapView::clear_markers() {
    geomap.clear_markers();
}

void EPIRBMapView::add_marker(GeoMarker& marker) {
    geomap.store_marker(marker);
}

void EPIRBMapView::paint(Painter& painter) {
    // Prevent view from clearing background if map is not hidden
    if (map_hidden) {
        View::paint(painter);
        painter.draw_string({UI_POS_X_CENTER(7), UI_POS_MAXHEIGHT / 2 - (UI_POS_HEIGHT(1) / 2)}, *Theme::getInstance()->fg_light, "No data");
    }
}

void EPIRBMapView::on_show() {
    // Force redrawing map
    repaint();
}

void EPIRBMapView::hide_map(bool hide) {
    map_hidden = hide;
    geomap.hidden(hide);
}

void EPIRBMapView::repaint() {
    // Fake orientation change to force map redraw
    geomap.update_my_orientation(180, false);
    geomap.update_my_orientation(0, true);
    set_dirty();
}

EPRIBQRView::EPRIBQRView(Rect parent_rect) : View(parent_rect) {
    add_children({&text_data, &options_qr, &qr_code});
    // Hide for now
    qr_code.hidden(true);
    options_qr.on_change = [this](size_t, ui::OptionsField::value_t v) {
        show_map = (v == 0);
        update_qr();
    };
    update_display();
}

void EPRIBQRView::set_beacon(Beacon* beacon) {
    current_beacon = beacon;
    update_qr();
    update_display();
}

void EPRIBQRView::update_display() {
    // Update data
    char buffer[128];
    char* buffer_pointer = buffer;
    buffer_pointer += sprintf(buffer_pointer, "%sQR:%s\t\t\t\t\t\t\t\t", STR_COLOR_CYAN, STR_COLOR_WHITE);
    if (current_beacon) {
        buffer_pointer += sprintf(buffer_pointer, "%sData:%s\t", STR_COLOR_CYAN, STR_COLOR_WHITE);
        // HEX ID 30 Hexa or HEX ID 22 Hexa bit 26 to 112
        buffer_pointer += current_beacon->toHexString(buffer_pointer, current_beacon->frame, true, 3, 11);
        (*(buffer_pointer++)) = '\t';
        if (current_beacon->longFrame) {
            current_beacon->toHexString(buffer_pointer, current_beacon->frame, true, 11, 18);
        } else {
            current_beacon->toHexString(buffer_pointer, current_beacon->frame, true, 11, 14);
        }
    }
    text_data.set_content(buffer);
}

void EPRIBQRView::update_qr() {
    bool show_qr = false;
    if (current_beacon) {
        if (show_map) {
            if (!current_beacon->location.isUnknown()) {
                current_beacon->location.formatFloatLocation(qr_url, MAPS_URL_TEMPLATE);
                show_qr = true;
            }
        } else {
            char* buffer_pointer = qr_url;
            buffer_pointer += sprintf(qr_url, BEACON_URL_TEMPALTE);
            current_beacon->hexString(buffer_pointer, false);
            show_qr = true;
        }
    }
    if (show_qr) qr_code.set_text(qr_url);
    qr_code.hidden(!show_qr);
    set_dirty();
}

#ifdef SPECAN
EPIRBRxView::EPIRBRxView(
    EPIRBAppView& parent,
    Rect parent_rect)
    : spectrum::WaterfallView(), app_view(parent) {
    ui::Rect waterfall_rect{0, EPIRB_TAB_POS_Y, parent_rect.width(), parent_rect.height()};
    spectrum::WaterfallView::set_parent_rect(waterfall_rect);
}

void EPIRBRxView::on_show() {
    // Turn on spectrum
    app_view.epirb_rx_config_message.scpectrum_on = true;
    app_view.send_config();
    start();
}

void EPIRBRxView::on_hide() {
    // Turn off spectrum
    stop();
    app_view.epirb_rx_config_message.scpectrum_on = false;
    app_view.send_config();
    app_view.refresh();
}
#endif

EPIRBAppView::EPIRBAppView(ui::NavigationView& nav)
    : nav_(nav) {
    baseband::run_prepared_image(portapack::memory::map::m4_code.base());

    add_children({&options_frequency,
                  &field_rf_amp,
                  &field_lna,
                  &field_vga,
                  &rssi,
                  //&audio,
                  //&field_squelch,
                  &field_volume,
                  &channel,
                  &text_status,
                  &text_timeout,
                  &tab_view,
                  &view_list,
                  &view_detail,
                  &view_map,
#ifdef SPECAN
                  &view_rx,
#endif
                  &view_qr});
    using option_t = std::pair<std::string, int32_t>;
    using options_t = std::vector<option_t>;
    options_t frequ_options;
    for (auto freq : resource_manager.get_frequencies()) {
        int32_t freq_value = atol(freq.c_str());
        frequ_options.emplace_back(to_string_rounded_freq(freq_value, 3), freq_value);
    }
    options_frequency.set_options(frequ_options);

    options_frequency.on_change = [this](size_t, ui::OptionsField::value_t v) {
        receiver_model.set_target_frequency(v);
    };
    options_frequency.set_by_value(receiver_model.target_frequency());

    signal_token_tick_second = rtc_time::signal_tick_second += [this]() {
        this->on_tick_second();
    };

    tab_view.set_parent_rect(Rect(0, UI_POS_HEIGHT(4), screen_width, 3 * 8));
    view_list.hidden(false);
    view_detail.hidden(true);
    view_map.hidden(true);
#ifdef SPECAN
    view_rx.hidden(true);
#endif
    view_qr.hidden(true);

    view_list.set_db(beacon_db);

    view_list.on_select = [this](size_t selected) {
        beacon_db.set_current_beacon(selected);
        on_beacon_change();
    };

    // Restore squelch value
    // field_squelch.set_value(squelch);
    epirb_rx_config_message.squelch = squelch;
    send_config();
    // field_squelch.on_change = [this](int32_t v) {
    //     squelch = v;
    //     epirb_rx_config_message.squelch = squelch;
    //     send_config();
    // };

    // Configure receiver for default EPIRB frequency (406.028 MHz)
    // Receiver parameters are loaded from settings
    // receiver_model.set_target_frequency(406025000);
    // receiver_model.set_rf_amp(true);
    // receiver_model.set_lna(32);
    // receiver_model.set_vga(32);

    // Force sample rate to patch baseband processor
    receiver_model.set_sampling_rate(3072000);

    receiver_model.enable();

    audio::set_rate(audio::Rate::Hz_24000);
    audio::output::start();

    update_display();

    /*logger = std::make_unique<EPIRBLogger>();
    if (logger) {
        logger->append(logs_dir / "epirb_rx.txt");
    }*/
}

EPIRBAppView::~EPIRBAppView() {
    rtc_time::signal_tick_second -= signal_token_tick_second;
    audio::output::stop();
    receiver_model.disable();
    baseband::shutdown();
}

void EPIRBAppView::set_parent_rect(const ui::Rect new_parent_rect) {
    View::set_parent_rect(new_parent_rect);
}

void EPIRBAppView::refresh() {
    // Force map repaint
    view_map.repaint();
}

/*void EPIRBAppView::focus() {
    options_frequency.focus();
}*/

void EPIRBAppView::on_packet(Message* const p) {
    const auto message = static_cast<const EPIRBPacketMessage*>(p);
    const baseband::Packet& packet = message->packet;

    // std::string beacon_size = "Data size :" + to_string_dec_int(packet.size()) + "\n";
    // UsbSerialAsyncmsg::asyncmsg(beacon_size);
    // std::string beacon_string = "Data:" + beacon_to_hex_string(packet) + "\n";
    // UsbSerialAsyncmsg::asyncmsg(beacon_string);

    // Decode the EPIRB packet
    // if (packet.size() > 64) {
    // Actual beacon
    Beacon& beacon = beacon_db.add_beacon();
    decode_packet(packet, beacon);
    beacons_received++;

    // Track packet statistics
    if (beacon.isFrameValid())
        packets_valid++;
    else
        packets_error++;

    // Update timeout
    timeout = (timeout_delay * -1);
    // Update display
    on_beacon_change();

    // Log the beacon
    /*if (logger) {
        logger->on_packet(beacon);
    }*/

    view_list.set_dirty();
    //}
}

void EPIRBAppView::on_beacon_change() {
    Beacon& cur_beacon = beacon_db.get_current_beacon();
    view_detail.set_beacon(cur_beacon);
    view_qr.set_beacon(&cur_beacon);
    update_map();
    // Update display
    update_display();
}

void EPIRBAppView::update_map() {
    // Clear previously saved markers
    view_map.clear_markers();
    bool hide_map = true;
    size_t size = beacon_db.size();
    if (size > 0) {
        // Check if current beacon has a valid location
        Beacon& beacon = beacon_db.get_current_beacon();
        size_t cur_index = beacon_db.get_current_beacon_index();
        if (!beacon.location.isUnknown()) {
            hide_map = false;
            // Set new position
            view_map.set_main_marker(std::string(beacon.getType()) + "-" + beacon.shortId(), beacon.location.latitude.getFloatValue(), beacon.location.longitude.getFloatValue());
            // Add all beacons with valid locations as markers
            for (size_t j = 0; j < size; j++) {
                if (cur_index != j) {
                    Beacon& other_beacon = beacon_db.get_beacon(j);
                    if (!other_beacon.location.isUnknown()) {
                        ui::GeoMarker marker;
                        marker.lat = other_beacon.location.latitude.getFloatValue();
                        marker.lon = other_beacon.location.longitude.getFloatValue();
                        marker.angle = 0;
                        marker.tag = std::string(other_beacon.getType()) + "-" + other_beacon.shortId();
                        view_map.add_marker(marker);
                    }
                }
            }
        }
    }
    view_map.hide_map(hide_map);
    view_map.repaint();
}

/*
void EPIRBAppView::on_clear_beacons() {
    beacon_db.clear();
    beacons_received = 0;
    packets_valid = 0;
    packets_corrected = 0;
    packets_error = 0;
    update_map();
    view_qr.set_beacon(nullptr);
    update_display();
}*/

/*void EPIRBAppView::on_toggle_log() {
    // Toggle logging functionality
    if (logger) {
        logger.reset();
        button_log.set_text("Log");
    } else {
        logger = std::make_unique<EPIRBLogger>();
        logger->append("epirb_rx.txt");
        button_log.set_text("Stop");
    }
}*/

void EPIRBAppView::on_tick_second() {
    timeout++;
    text_timeout.set(to_string_dec_uint(abs(timeout)));
}

void EPIRBAppView::update_display() {
    char buffer[128];
    char* buffer_pointer = buffer;
    buffer_pointer += sprintf(buffer_pointer, "%sListening...     Beacons:%s%3d\t", STR_COLOR_CYAN, STR_COLOR_WHITE, beacons_received);
    buffer_pointer += sprintf(buffer_pointer, "%sStats: %s%03dOK %s%03dCOR %s%03dERR\t", STR_COLOR_CYAN, STR_COLOR_GREEN, packets_valid, STR_COLOR_YELLOW, packets_corrected, STR_COLOR_RED, packets_error);
    buffer_pointer += sprintf(buffer_pointer, "%sCurrent:%s ", STR_COLOR_CYAN, STR_COLOR_WHITE);
    if (!beacon_db.empty()) {
        beacon_db.get_current_beacon().formatSummary(buffer_pointer, false);
    }
    text_status.set_content(buffer);
}

void EPIRBAppView::send_config() {
    // Send config to baseband
    baseband::set_epirb_rx_config(epirb_rx_config_message);
}

}  // namespace ui::external_app::epirb_rx