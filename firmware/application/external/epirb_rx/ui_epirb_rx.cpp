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

namespace ui::external_app::epirb_rx {

// URL templates
#define MAPS_URL_TEMPLATE "https://www.google.com/maps/search/?api=1&query=%s%%2C%s"
#define BEACON_URL_TEMPLATE "https://decoder2.herokuapp.com/decoded/"

#ifndef DISABLE_COUNTRY_CACHE
int CountryManager::cache_count = 0;
Country CountryManager::cache[16];
#endif

// ResourceManager class
ResourceManager* ResourceManager::current = nullptr;

void ResourceManager::destroy() {
    if (current != nullptr)
        delete current;
    current = nullptr;
}

ResourceManager* ResourceManager::getInstance() {
    if (current == nullptr) current = new ResourceManager();
    return ResourceManager::current;
}

// TextArea class
TextArea::TextArea(
    Rect parent_rect)
    : Widget{parent_rect} {
}

void TextArea::paint(Painter& painter) {
    const auto rect = screen_rect();
    const Style& s = has_focus() ? style().invert() : style();

    painter.fill_rectangle(rect, s.background);

    const int line_height = s.font.line_height();
    int line_idx = 0;

    // Efficiently draw lines separated by \t without heap allocations
    std::string_view sv{content};
    size_t start = 0;
    size_t end;

    while ((end = sv.find('\t', start)) != std::string_view::npos) {
        painter.draw_string(rect.location() + Point(0, line_idx * line_height), s, sv.substr(start, end - start));
        start = end + 1;
        line_idx++;
    }

    if (start < sv.length()) {
        painter.draw_string(rect.location() + Point(0, line_idx * line_height), s, sv.substr(start));
    }
}

void TextArea::set_content(std::string_view value) {
    content = std::string{value};
    set_dirty();
}

#ifdef RESET_TIMER
bool TextArea::on_key(const KeyEvent key) {
    if (key == KeyEvent::Select && on_select) {
        on_select(*this);
        set_dirty();
        return true;
    }
    return false;
}
#endif

// EPIRBAppView class
void EPIRBAppView::decode_packet(const baseband::Packet& packet, Beacon& beacon) {
    // Convert packet bits to byte array for easier processing
    uint8_t data[BEACON_DATA_SIZE]{};
    size_t max_bytes = std::min(packet.size() / 8, (size_t)BEACON_DATA_SIZE);
    size_t packet_bit_idx = 0;
    for (size_t i = 0; i < max_bytes; i++) {
        uint8_t byte_val = 0;
        for (int bit = 0; bit < 8; bit++, packet_bit_idx++) {
            if (packet[packet_bit_idx]) {
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

#ifdef LOGGER
void EPIRBLogger::on_packet(Beacon& beacon) {
    std::string entry;
    // Pre-allocate enough memory to avoid heap fragmentation and resizing
    // 128 bytes should be plenty for this specific log line
    entry.reserve(128);
    entry += beacon.getType();
    entry += ",";
    entry += beacon.hexId;
    entry += ",";
    entry += beacon.getProtocolName();

    /* If you ever uncomment the rest of the logging,
     * keep using the same pattern! Like this:
     *
     * if (!beacon.location.isUnknown()) {
     * entry += beacon.location.toString(Location::LocationFormat::DECIMAL);
     * } else {
     * entry += ",";
     * }
     * entry += ",";
     * entry += beacon.country.toString();
     * entry += ",";
     * entry += beacon.getSatus();
     * entry += "\n";
     */

    log_file.write_entry(beacon.date, entry);
}
#endif

// EPIRBDetailView class
EPIRBDetailView::EPIRBDetailView(
    Rect parent_rect,
    EPIRBAppView& parent)
    : View(parent_rect), parent_app(parent) {
    add_children({&text_beacon});
    set_focusable(true);
}

#ifdef DETAIL_TAB_BEACON_SEL
bool EPIRBDetailView::on_encoder(EncoderEvent delta) {
    // Change position in the list according to encoder
    int32_t size = (int32_t)parent_app.beacon_db.size();
    if (size == 0) return true;
    // Get current  index
    int32_t current_index = (int32_t)parent_app.beacon_db.get_current_beacon_index();
    // Compute new index with wrap around
    int32_t new_index = (current_index + delta + size) % size;
    // Set new index
    parent_app.beacon_db.set_current_beacon(new_index % size);
    parent_app.on_beacon_change();
    return true;
}
#endif

// Append "CYAN<label>:WHITE <value>\t" — the most repeated pattern in set_beacon.
// Shared format string deduplicates ~5 unique formats; %s for label lets the
// per-call-site cost drop to a single function call.
static char* append_field(char* p, const char* label, const char* value) {
    return p + sprintf(p, STR_COLOR_CYAN "%s:" STR_COLOR_WHITE " %s\t", label, value);
}

void EPIRBDetailView::set_beacon(Beacon& beacon) {
    // We use a single TextArea widget to display beacon information for code size optimization
    char buffer[400];
    char* p = buffer;
    bool isReal = (beacon.frameMode == Beacon::FrameMode::NORMAL);
    p += sprintf(p, STR_COLOR_CYAN "Beacon:" STR_COLOR_WHITE " %s(%s%s" STR_COLOR_WHITE ") - ",
                 beacon.getType(),
                 isReal ? STR_COLOR_YELLOW : STR_COLOR_GREEN,
                 isReal ? "Real" : "Test");
    p += beacon.formatTime(p);
    *p++ = '\t';
    p = append_field(p, "Protocol", beacon.getProtocolName());
    p += sprintf(p, "%s\t", ResourceManager::get_beacon_resource((uint8_t)beacon.protocol));
    if (beacon.hasAdditionalData) {
        p += sprintf(p, "%s\t", beacon.additionalData);
    }
    if (beacon.hasEmergency) {
        p += sprintf(p, STR_COLOR_CYAN "Emergency:" STR_COLOR_RED " %s\t", beacon.emergencyType);
    }
    p += sprintf(p, STR_COLOR_CYAN "Country:" STR_COLOR_WHITE " %s(%d) - %s\t" STR_COLOR_CYAN "Location:" STR_COLOR_WHITE " ",
                 beacon.country.alphaCode, beacon.country.code, beacon.country.shortName);
    p += beacon.location.toString(p, Location::LocationFormat::MAIDENHEAD_LOCATOR, 8);
    *p++ = '\t';
    if (!beacon.location.isUnknown()) {
        p += beacon.location.toString(p, Location::LocationFormat::SEXAGESIMAL);
        *p++ = '\t';
        p += beacon.location.toString(p, Location::LocationFormat::DECIMAL);
        *p++ = '\t';
    }
    // BCH status: color depends on validity/correction, so the color stays a %s substitution.
    p += sprintf(p, STR_COLOR_CYAN "Control: %s%s",
                 beacon.isBch1Valid() ? (beacon.bch1Corrected ? STR_COLOR_YELLOW : STR_COLOR_GREEN) : STR_COLOR_RED,
                 beacon.isBch1Valid() ? "BCH1-OK" : "BCH1-KO");
    if (beacon.hasBch2) {
        p += sprintf(p, " %s%s",
                     beacon.isBch2Valid() ? (beacon.bch2Corrected ? STR_COLOR_YELLOW : STR_COLOR_GREEN) : STR_COLOR_RED,
                     beacon.isBch2Valid() ? "BCH2-OK" : "BCH2-KO");
    }
    *p++ = '\t';
    p = append_field(p, "Hex ID", beacon.hexId);
    if (beacon.hasSerialNumber) {
        p = append_field(p, "S/N", beacon.serialNumber);
    }
    if (beacon.hasMainLocatingDevice()) {
        p = append_field(p, "Main loc. dev.", beacon.getMainLocatingDeviceName());
        if (beacon.hasAuxLocatingDevice()) {
            append_field(p, "Aux loc. dev.", beacon.getAuxLocatingDeviceName());
        }
    }
    text_beacon.set_content(buffer);
}

// EPIRBMapView class
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

// EPRIBQRView class
EPRIBQRView::EPRIBQRView(Rect parent_rect)
    : View(parent_rect) {
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
    // Update data => we use a single TextArea component for code size optimization
    char buffer[128];
    char* buffer_pointer = buffer;
    buffer_pointer += sprintf(buffer_pointer, STR_COLOR_CYAN "QR:" STR_COLOR_WHITE "\t\t\t\t\t\t\t\t");
    if (current_beacon) {
        buffer_pointer += sprintf(buffer_pointer, STR_COLOR_CYAN "Data:" STR_COLOR_WHITE "\t");
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
    // Update QR code
    bool show_qr = false;
    if (current_beacon) {
        // We have a beacon
        if (show_map) {
            // Map is selected
            if (!current_beacon->location.isUnknown()) {
                // Location is known => actually draw QR
                current_beacon->location.formatFloatLocation(qr_url, MAPS_URL_TEMPLATE);
                show_qr = true;
            }
        } else {
            // Detail is selected
            char* buffer_pointer = qr_url;
            // Send to heroku decoder
            buffer_pointer += sprintf(qr_url, BEACON_URL_TEMPLATE);
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
    app_view.epirb_rx_config_message.spectrum_on = true;
    app_view.send_config();
    start();
}

void EPIRBRxView::on_hide() {
    // Turn off spectrum
    stop();
    app_view.epirb_rx_config_message.spectrum_on = false;
    app_view.send_config();
    app_view.refresh();
}
#endif

// App View
EPIRBAppView::EPIRBAppView(ui::NavigationView& nav)
    : nav_(nav) {
    baseband::run_prepared_image(portapack::memory::map::m4_code.base());

    add_children({&options_frequency,
                  &field_rf_amp,
                  &field_lna,
                  &field_vga,
                  &rssi,
    //&audio,
#ifdef SQUELCH
                  &field_squelch,
#endif
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

    ui::OptionsField::options_t frequ_options;
    // Set frequency combo content according to FREQ.TXT file content
    for (const auto& freq : ResourceManager::get_frequencies()) {
        int32_t freq_value = atol(freq.c_str());
        frequ_options.emplace_back(to_string_rounded_freq(freq_value, 3), freq_value);
    }
    options_frequency.set_options(frequ_options);

    options_frequency.on_change = [this](size_t, ui::OptionsField::value_t v) {
        receiver_model.set_target_frequency(v);
    };
    // Restore frequency from preferences
    options_frequency.set_by_value(receiver_model.target_frequency());

    // Tick second timer
    signal_token_tick_second = rtc_time::signal_tick_second += [this]() {
        this->on_tick_second();
    };

#ifdef RESET_TIMER
    text_timeout.set_focusable(true);
    text_timeout.on_select = [this](TextArea&) {
        timeout = ((countdown + 1) * -1);
        on_tick_second();
    };
#endif

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

#ifdef SQUELCH
    // Restore squelch value
    field_squelch.set_value(squelch);
#endif
    epirb_rx_config_message.squelch = squelch;
    send_config();
#ifdef SQUELCH
    field_squelch.on_change = [this](int32_t v) {
        squelch = v;
        epirb_rx_config_message.squelch = squelch;
        send_config();
    };
#endif

    // Force sample rate to match baseband processor
    receiver_model.set_sampling_rate(3072000);

    receiver_model.enable();

    audio::set_rate(audio::Rate::Hz_24000);
    audio::output::start();

    // Tint the channel-power bar red when the front-end overloads (ADC near
    // full scale). Clipping distorts the constant-envelope carrier and the
    // +/-1.1 rad biphase jumps the decoder relies on, so this cues the user to
    // reduce RF-amp/LNA/VGA gain. -3 dBFS is a heuristic on the post-decimation
    // level (not a literal ADC clip count); tune if it trips too early/late.
    channel.set_overload_threshold(-3);

    update_display();

#ifdef LOGGER
    logger = std::make_unique<EPIRBLogger>();
    if (logger) {
        logger->append(logs_dir / "epirb_rx.txt");
    }
#endif
}

EPIRBAppView::~EPIRBAppView() {
    // Remove tick second timer
    rtc_time::signal_tick_second -= signal_token_tick_second;
    // Stop audio processor
    audio::output::stop();
    // Stop receiver
    receiver_model.disable();
    baseband::shutdown();
    // Cleanup ResourceManager
    ResourceManager::destroy();
}

void EPIRBAppView::refresh() {
    // Force map repaint
    view_map.repaint();
}

void EPIRBAppView::focus() {
    options_frequency.focus();
}

void EPIRBAppView::on_packet(Message* const p) {
    const auto message = static_cast<const EPIRBPacketMessage*>(p);
    const baseband::Packet& packet = message->packet;

    // Decode the EPIRB packet
    if (packet.size() > 64) {
        // Actual beacon
        Beacon& beacon = beacon_db.add_beacon();
        // Get a new beacon from database and load it
        decode_packet(packet, beacon);
        beacons_received++;

        // Track packet statistics
        if (beacon.isFrameValid()) {
            if (beacon.bch1Corrected || beacon.bch2Corrected) {
                packets_corrected++;
            } else {
                packets_valid++;
            }
        } else {
            packets_error++;
        }

        // Update timeout
        timeout = (countdown * -1);
        // Reset selection
        beacon_db.set_current_beacon(0);
        // Update display
        on_beacon_change();

#ifdef LOGGER
        // Log the beacon
        if (logger) {
            logger->on_packet(beacon);
        }
#endif

        view_list.set_dirty();
    }
}

void EPIRBAppView::on_beacon_change() {
    // Beacon selection has changed => get new selection
    Beacon& cur_beacon = beacon_db.get_current_beacon();
    // Update detail, qr and map tab
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
            std::string tag = beacon.getType();
            tag += "-";
            tag += beacon.shortId();
            view_map.set_main_marker(tag, beacon.location.latitude.getFloatValue(), beacon.location.longitude.getFloatValue());
            // Add all beacons with valid locations as markers
            for (size_t j = 0; j < size; j++) {
                if (cur_index != j) {
                    Beacon& other_beacon = beacon_db.get_beacon(j);
                    if (!other_beacon.location.isUnknown()) {
                        ui::GeoMarker marker;
                        marker.lat = other_beacon.location.latitude.getFloatValue();
                        marker.lon = other_beacon.location.longitude.getFloatValue();
                        tag = other_beacon.getType();
                        tag += "-";
                        tag += other_beacon.shortId();
                        marker.tag = tag;
                        view_map.add_marker(marker);
                    }
                }
            }
        }
    }
    view_map.hide_map(hide_map);
    view_map.repaint();
}

void EPIRBAppView::on_tick_second() {
    timeout++;
    // Limit to 3 digits
    if (timeout > 999) timeout = 0;
    text_timeout.set_content(to_string_dec_uint(abs(timeout)));
}

void EPIRBAppView::update_display() {
    char buffer[128];
    int len = sprintf(buffer,
                      STR_COLOR_CYAN "Listening...    Beacon:" STR_COLOR_WHITE "%2d/%d\t" STR_COLOR_CYAN "Stats: " STR_COLOR_GREEN "%03dOK " STR_COLOR_YELLOW "%03dCOR " STR_COLOR_RED "%03dERR\t" STR_COLOR_CYAN "Current:" STR_COLOR_WHITE " ",
                      (beacon_db.get_current_beacon_index() + 1), beacons_received,
                      packets_valid, packets_corrected, packets_error);

    if (!beacon_db.empty()) {
        beacon_db.get_current_beacon().formatSummary(buffer + len, false);
    }

    text_status.set_content(buffer);
}

void EPIRBAppView::send_config() {
    // Send config to baseband
    baseband::set_epirb_rx_config(epirb_rx_config_message);
}

}  // namespace ui::external_app::epirb_rx
