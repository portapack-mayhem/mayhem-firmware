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

//#include "usb_serial_asyncmsg.hpp"

namespace ui::external_app::epirb_rx {
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
    Rect parent_rect)
    : View(parent_rect) {

    //add_children({&labels});
    add_children({&text_beacon});
}

void EPIRBDetailView::paint(Painter& /*painter*/) {
    // Prevent scrolling
    text_beacon.clear(false);
}

void EPIRBDetailView::set_beacon(Beacon& beacon) {
    std::string beacon_text = STR_COLOR_CYAN + std::string("Beacon: ") + STR_COLOR_WHITE + beacon.getType() + "(";
    if(beacon.frameMode == Beacon::FrameMode::NORMAL) {
        beacon_text += STR_COLOR_YELLOW + std::string("Real");
    } else {
        beacon_text += STR_COLOR_GREEN + std::string("Test");
    }
    beacon_text += STR_COLOR_WHITE +  std::string(") - ") + beacon.formatTime() + "\n";
    beacon_text += STR_COLOR_CYAN + std::string("Protocol: ") + STR_COLOR_WHITE + beacon.getProtocolName() + "\n";
    //beacon_text += beacon.getProtocolDesciption() + ;
    if(beacon.hasAdditionalData) {
        beacon_text += beacon.additionalData + "\n";
    }
    beacon_text += STR_COLOR_CYAN + std::string("Location: ") + STR_COLOR_WHITE + beacon.location.toString(Location::LocationFormat::MAIDENHEAD_LOCATOR) + "\n";
    //beacon_text += beacon.country.toString() + "\n";
    if(!beacon.location.isUnknown()) {
        beacon_text += beacon.location.toString(Location::LocationFormat::SEXAGESIMAL) + "\n";
        beacon_text += beacon.location.toString(Location::LocationFormat::DECIMAL) + "\n";
    }
    beacon_text += STR_COLOR_CYAN + std::string("Control: ");
    beacon_text += beacon.isBch1Valid() ? STR_COLOR_GREEN + std::string("BCH1-OK") : STR_COLOR_RED + std::string("BCH1-KO");
    if(beacon.hasBch2) {
        beacon_text += beacon.isBch2Valid() ? STR_COLOR_GREEN + std::string(" BCH2-OK") : STR_COLOR_RED + std::string(" BCH2-KO");
    }
    beacon_text += "\n";
    beacon_text += STR_COLOR_CYAN + std::string("Hex ID: ") + STR_COLOR_WHITE + beacon.hexId + "\n";
    if(beacon.hasSerialNumber) {
        beacon_text += STR_COLOR_CYAN + std::string("S/N: ") + STR_COLOR_WHITE + beacon.serialNumber + "\n";
    }
    text_beacon.clear(true);
    text_beacon.write(beacon_text);
}

EPIRBMapView::EPIRBMapView(
    Rect parent_rect)
    : View(parent_rect) {

    add_children({&geomap});
    geomap.set_mode(DISPLAY);
    geomap.set_manual_panning(false);
    geomap.set_tag(NO_BEACON);
    // geomap.set_hide_center_marker(true); //todo test if needed
    geomap.init();
    geomap.set_focusable(true);
    geomap.clear_markers();
    geomap.move(lon_, lat_);
}

void EPIRBMapView::set_main_marker(const std::string& label, float lat, float lon){
    geomap.set_tag(label);
    lat_ = lat;
    lon_ = lon;
    geomap.move(lon_, lat_);
}

void EPIRBMapView::clear_main_marker() {
    set_main_marker(NO_BEACON,EPIRB_RX_DEFAULT_LATITUDE,EPIRB_RX_DEFAULT_LONGITUDE);
}

void EPIRBMapView::clear_markers() {
    geomap.clear_markers();
}

void EPIRBMapView::add_marker(GeoMarker& marker) {
    geomap.store_marker(marker);
}

void EPIRBMapView::paint(Painter& /*painter*/) {
    // Prevent view from clearing background
}

void EPIRBMapView::on_show() {
    // Force redrawing map
    repaint();
}

void EPIRBMapView::repaint() {
    // Fake orientation change to force map redraw
    geomap.update_my_orientation(180,false);
    geomap.update_my_orientation(0,true);
}

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

EPIRBAppView::EPIRBAppView(ui::NavigationView& nav)
    : nav_(nav) {
    baseband::run_prepared_image(portapack::memory::map::m4_code.base());

    add_children({
                  &label_frequency,
                  &options_frequency,
                  &field_rf_amp,
                  &field_lna,
                  &field_vga,
                  &rssi,
                  &field_volume,
                  &channel,
                  //&label_status,
                  //&label_beacons_count,
                  //&label_latest,
                  //&text_latest_info,
                  //&label_packet_stats,
                  &tab_view,
                  &view_list,
                  &view_detail,
                  &view_map,
                  &view_rx});

    options_frequency.on_change = [this](size_t, ui::OptionsField::value_t v) {
        receiver_model.set_target_frequency(v);
    };
    options_frequency.set_by_value(receiver_model.target_frequency());

    /*signal_token_tick_second = rtc_time::signal_tick_second += [this]() {
        this->on_tick_second();
    };*/
    tab_view.set_parent_rect(Rect(0,UI_POS_HEIGHT(4),screen_width,3*8));
    view_list.hidden(false); 
    view_detail.hidden(true);
    view_map.hidden(true);
    view_rx.hidden(true);

    view_list.set_db(beacon_db);

    // Configure receiver for default EPIRB frequency (406.028 MHz)
    // TODO : Load from conf
    receiver_model.set_target_frequency(406025000);
    receiver_model.set_rf_amp(true);
    receiver_model.set_lna(32);
    receiver_model.set_vga(32);
    receiver_model.set_sampling_rate(3072000);

    audio::set_rate(audio::Rate::Hz_24000);
    audio::output::start();

    receiver_model.enable();

    /*logger = std::make_unique<EPIRBLogger>();
    if (logger) {
        logger->append(logs_dir / "epirb_rx.txt");
    }*/
}

EPIRBAppView::~EPIRBAppView() {
    //rtc_time::signal_tick_second -= signal_token_tick_second;
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
    set_dirty();
}

void EPIRBAppView::focus() {
    options_frequency.focus();
}

void EPIRBAppView::on_packet(const baseband::Packet& packet) {
    //std::string beacon_size = "Data size :" + to_string_dec_int(packet.size()) + "\n";
    //UsbSerialAsyncmsg::asyncmsg(beacon_size);
    //std::string beacon_string = "Data:" + beacon_to_hex_string(packet) + "\n";
    //UsbSerialAsyncmsg::asyncmsg(beacon_string);

    // Decode the EPIRB packet
    if (packet.size() > 64) {
        // Actual beacon
        Beacon& beacon = beacon_db.add_beacon();
        decode_packet(packet, beacon);
        beacons_received++;
        view_detail.set_beacon(beacon);

        // Track packet statistics
        if (beacon.isFrameValid())
            packets_valid++;
        else
            packets_error++;

        if(!beacon.location.isUnknown()) {
            update_map();
        }

        // Update display
        update_display();

        // Log the beacon
        /*if (logger) {
            logger->on_packet(beacon);
        }*/

        // Display in console with full details and colored status
        /*if (beacon.emergency_type != EmergencyType::Other) {
            beacon_info += " [" + format_emergency_type(beacon.emergency_type) + "]";
        }*/

        // Add colored status indicator
        /*if (beacon.error_count > 0 && beacon.packet_status == PacketStatus::Corrected) {
            beacon_info += " (" + to_string_dec_uint(beacon.error_count) + "e)";
        }*/

        // console.write(beacon_info + "\n");
        //  TODO update beacon list
        //view_list.refresh();
        view_list.set_dirty();
    }
}

void EPIRBAppView::update_map() {
    size_t size = beacon_db.size();
    if (size > 0) {
        // Find latest beacon with valid location
        for (size_t i = 0; i < size; i++) {
            Beacon& beacon = beacon_db.get_beacon(i);
            if (!beacon.location.isUnknown()) {
                // Clear previously saved markers
                view_map.clear_markers();
                // Set new position
                view_map.set_main_marker(std::string(beacon.getType()) + "-" + beacon.shortId(), beacon.location.latitude.getFloatValue(),beacon.location.longitude.getFloatValue());
                // Add all beacons with valid locations as markers
                for (size_t j = 0; j < size; j++) {
                    if (i != j) {
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
                return;
            }
        }
    }
}

void EPIRBAppView::on_clear_beacons() {
    beacon_db.clear();
    beacons_received = 0;
    packets_valid = 0;
    packets_corrected = 0;
    packets_error = 0;
    view_map.clear_markers();
    view_map.clear_main_marker();
    // console.clear(true);
    //  TODO update beacon list
    update_display();
}

void EPIRBAppView::on_toggle_log() {
    // Toggle logging functionality
    /*if (logger) {
        logger.reset();
        button_log.set_text("Log");
    } else {
        logger = std::make_unique<EPIRBLogger>();
        logger->append("epirb_rx.txt");
        button_log.set_text("Stop");
    }*/
}

void EPIRBAppView::on_tick_second() {
    // Update status display every second
    rtc::RTC datetime;
    rtcGetTime(&RTCD1, &datetime);

    //label_status.set("Listening... " + to_string_datetime(datetime, HM));
}

void EPIRBAppView::update_display() {
    //label_beacons_count.set("Beacons: " + to_string_dec_uint(beacons_received));

    // Update packet statistics display
    /*std::string stats = std::string("Stats: ") +
                        STR_COLOR_GREEN + to_string_dec_uint(packets_valid) + "OK " +
                        STR_COLOR_YELLOW + to_string_dec_uint(packets_corrected) + "CORR " +
                        STR_COLOR_RED + to_string_dec_uint(packets_error) + "ERR";
    //label_packet_stats.set(stats);

    if (beacon_db.size() > 0) {
        auto& latest = beacon_db.get_latest_beacon();
        text_latest_info.set(latest.formatSummary(false));
    }*/
}

void EPIRBAppView::send_config() {
    // Send config to baseband
    baseband::set_epirb_rx_config(epirb_rx_config_message);    
}

}  // namespace ui::external_app::epirb_rx