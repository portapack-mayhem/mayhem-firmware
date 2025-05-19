/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2017 Furrtek
 * Copyright (C) 2023 Kyle Reed
 * Copyright (C) 2024 Mark Thompson
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

#include <algorithm>

#include "ui_adsb_rx.hpp"
#include "ui_alphanum.hpp"

#include "baseband_api.hpp"
#include "portapack_persistent_memory.hpp"
#include "rtc_time.hpp"
#include "string_format.hpp"
#include "file_path.hpp"
#include "audio.hpp"

using namespace portapack;

namespace pmem = portapack::persistent_memory;

namespace ui {

static std::string get_map_tag(const AircraftRecentEntry& entry) {
    return trimr(entry.callsign.empty() ? entry.icao_str : entry.callsign);
}

template <>
void RecentEntriesTable<AircraftRecentEntries>::draw(
    const Entry& entry,
    const Rect& target_rect,
    Painter& painter,
    const Style& style) {
    Color target_color;
    std::string entry_string;

    switch (entry.state) {
        case ADSBAgeState::Invalid:
        case ADSBAgeState::Current:
            entry_string = "";
            target_color = Theme::getInstance()->fg_green->foreground;
            break;
        case ADSBAgeState::Recent:
            entry_string = STR_COLOR_LIGHT_GREY;
            target_color = Theme::getInstance()->fg_light->foreground;
            break;
        default:
            entry_string = STR_COLOR_DARK_GREY;
            target_color = Theme::getInstance()->fg_medium->foreground;
    };

    entry_string +=
        (entry.callsign.empty() ? entry.icao_str + "   " : entry.callsign + " ") +
        to_string_dec_uint((unsigned int)(entry.pos.altitude / 100), 4) +
        to_string_dec_uint((unsigned int)entry.velo.speed, 4) +
        to_string_dec_uint((unsigned int)(entry.amp >> 9), 4) + " " +
        (entry.hits <= 999 ? to_string_dec_uint(entry.hits, 3) + " " : "1k+ ") +
        to_string_dec_uint(entry.age, 4);

    painter.draw_string(
        target_rect.location(),
        style,
        entry_string);

    if (entry.pos.valid)
        painter.draw_bitmap(target_rect.location() + Point(8 * 8, 0),
                            bitmap_target, target_color, style.background);
}

/* ADSBLogger ********************************************/

void ADSBLogger::log(const ADSBLogEntry& log_entry) {
    std::string log_line;
    log_line.reserve(100);

    log_line = log_entry.raw_data;
    log_line += "ICAO:" + log_entry.icao;

    if (!log_entry.callsign.empty())
        log_line += " " + log_entry.callsign;

    if (log_entry.pos.valid)
        log_line += " Alt:" + to_string_dec_int(log_entry.pos.altitude) +
                    " Lat:" + to_string_decimal(log_entry.pos.latitude, 7) +
                    " Lon:" + to_string_decimal(log_entry.pos.longitude, 7);

    if (log_entry.vel.valid)
        log_line += " Type:" + to_string_dec_uint(log_entry.vel_type) +
                    " Hdg:" + to_string_dec_uint(log_entry.vel.heading) +
                    " Spd: " + to_string_dec_int(log_entry.vel.speed);
    if (log_entry.sil != 0)
        log_line += " Sil:" + to_string_dec_uint(log_entry.sil);
    log_file.write_entry(log_line);
}

/* ADSBRxAircraftDetailsView *****************************/

ADSBRxAircraftDetailsView::ADSBRxAircraftDetailsView(
    NavigationView& nav,
    const AircraftRecentEntry& entry) {
    add_children(
        {&labels,
         &text_icao_address,
         &text_registration,
         &text_manufacturer,
         &text_model,
         &text_type,
         &text_number_of_engines,
         &text_engine_type,
         &text_owner,
         &text_operator,
         &button_close});

    text_icao_address.set(entry.icao_str);

    // Try getting the aircraft information from icao24.db
    database db{};
    database::AircraftDBRecord aircraft_record;
    auto return_code = db.retrieve_aircraft_record(&aircraft_record, entry.icao_str);
    switch (return_code) {
        case DATABASE_RECORD_FOUND:
            text_registration.set(aircraft_record.aircraft_registration);
            text_manufacturer.set(aircraft_record.aircraft_manufacturer);
            text_model.set(aircraft_record.aircraft_model);
            text_owner.set(aircraft_record.aircraft_owner);
            text_operator.set(aircraft_record.aircraft_operator);

            // Check for ICAO type, e.g. L2J
            if (strlen(aircraft_record.icao_type) == 3) {
                switch (aircraft_record.icao_type[0]) {
                    case 'L':
                        text_type.set("Landplane");
                        break;
                    case 'S':
                        text_type.set("Seaplane");
                        break;
                    case 'A':
                        text_type.set("Amphibian");
                        break;
                    case 'H':
                        text_type.set("Helicopter");
                        break;
                    case 'G':
                        text_type.set("Gyrocopter");
                        break;
                    case 'T':
                        text_type.set("Tilt-wing aircraft");
                        break;
                }

                text_number_of_engines.set(std::string{1, aircraft_record.icao_type[1]});
                switch (aircraft_record.icao_type[2]) {
                    case 'P':
                        text_engine_type.set("Piston engine");
                        break;
                    case 'T':
                        text_engine_type.set("Turboprop/Turboshaft engine");
                        break;
                    case 'J':
                        text_engine_type.set("Jet engine");
                        break;
                    case 'E':
                        text_engine_type.set("Electric engine");
                        break;
                }
            }

            // Check for ICAO type designator
            else if (strlen(aircraft_record.icao_type) == 4) {
                if (strcmp(aircraft_record.icao_type, "SHIP") == 0)
                    text_type.set("Airship");
                else if (strcmp(aircraft_record.icao_type, "BALL") == 0)
                    text_type.set("Balloon");
                else if (strcmp(aircraft_record.icao_type, "GLID") == 0)
                    text_type.set("Glider / sailplane");
                else if (strcmp(aircraft_record.icao_type, "ULAC") == 0)
                    text_type.set("Micro/ultralight aircraft");
                else if (strcmp(aircraft_record.icao_type, "GYRO") == 0)
                    text_type.set("Micro/ultralight autogyro");
                else if (strcmp(aircraft_record.icao_type, "UHEL") == 0)
                    text_type.set("Micro/ultralight helicopter");
                else if (strcmp(aircraft_record.icao_type, "SHIP") == 0)
                    text_type.set("Airship");
                else if (strcmp(aircraft_record.icao_type, "PARA") == 0)
                    text_type.set("Powered parachute/paraplane");
            }
            break;

        case DATABASE_RECORD_NOT_FOUND:
            // Defaults should be filled by the constructor
            break;

        case DATABASE_NOT_FOUND:
            text_manufacturer.set("No icao24.db file");
            break;
    }

    button_close.on_select = [&nav](Button&) {
        nav.pop();
    };
}

void ADSBRxAircraftDetailsView::focus() {
    button_close.focus();
}

/* ADSBRxDetailsView *************************************/

ADSBRxDetailsView::ADSBRxDetailsView(
    NavigationView& nav,
    const AircraftRecentEntry& entry)
    : entry_(entry) {
    add_children(
        {&labels,
         &text_icao_address,
         &text_callsign,
         &text_last_seen,
         &text_airline,
         &text_country,
         &text_infos,
         &text_info2,
         &text_frame_pos_even,
         &text_frame_pos_odd,
         &button_aircraft_details,
         &button_see_map});

    text_icao_address.set(entry_.icao_str);

    button_aircraft_details.on_select = [this, &nav](Button&) {
        aircraft_details_view_ = nav.push<ADSBRxAircraftDetailsView>(entry_);
        nav.set_on_pop([this]() {
            aircraft_details_view_ = nullptr;
            refresh_ui();
        });
    };

    button_see_map.on_select = [this, &nav](Button&) {
        geomap_view_ = nav.push<GeoMapView>(
            get_map_tag(entry_),
            entry_.pos.altitude,
            GeoPos::alt_unit::FEET,
            GeoPos::spd_unit::MPH,
            entry_.pos.latitude,
            entry_.pos.longitude,
            entry_.velo.heading);
        nav.set_on_pop([this]() {
            geomap_view_ = nullptr;
            refresh_ui();
        });
    };

    refresh_ui();
};

void ADSBRxDetailsView::focus() {
    button_see_map.focus();
}

void ADSBRxDetailsView::update(const AircraftRecentEntry& entry) {
    entry_ = entry;

    if (aircraft_details_view_) {
        // AC Details view is showing, nothing to update.
    } else if (geomap_view_) {
        // Map is showing, update the current item.
        geomap_view_->update_tag(get_map_tag(entry_));
        geomap_view_->update_position(entry.pos.latitude, entry.pos.longitude, entry.velo.heading, entry.pos.altitude, entry.velo.speed);
    } else {
        // Details is showing, update details.
        refresh_ui();
    }
}

void ADSBRxDetailsView::clear_map_markers() {
    if (geomap_view_)
        geomap_view_->clear_markers();
}

bool ADSBRxDetailsView::add_map_marker(const AircraftRecentEntry& entry) {
    // Map not shown, can't add markers.
    if (!geomap_view_)
        return false;

    GeoMarker marker{};
    marker.lon = entry.pos.longitude;
    marker.lat = entry.pos.latitude;
    marker.angle = entry.velo.heading;
    marker.tag = get_map_tag(entry);

    auto markerStored = geomap_view_->store_marker(marker);
    return markerStored == MARKER_STORED;
}

void ADSBRxDetailsView::on_gps(const GPSPosDataMessage* msg) {
    if (!geomap_view_)
        return;
    geomap_view_->update_my_position(msg->lat, msg->lon, msg->altitude);
}
void ADSBRxDetailsView::on_orientation(const OrientationDataMessage* msg) {
    if (!geomap_view_)
        return;
    geomap_view_->update_my_orientation(msg->angle);
}

void ADSBRxDetailsView::refresh_ui() {
    // The following won't change for a given airborne aircraft.
    // Try getting the airline's name from airlines.db.
    if (!airline_checked && !entry_.callsign.empty()) {
        airline_checked = true;

        database db;
        database::AirlinesDBRecord airline_record;
        std::string airline_code = entry_.callsign.substr(0, 3);
        auto return_code = db.retrieve_airline_record(&airline_record, airline_code);

        switch (return_code) {
            case DATABASE_RECORD_FOUND:
                text_airline.set(airline_record.airline);
                text_country.set(airline_record.country);
                break;
            case DATABASE_RECORD_NOT_FOUND:
                // text_airline.set("-"); // It's what it is constructed with
                // text_country.set("-"); // It's what it is constructed with
                break;
            case DATABASE_NOT_FOUND:
                text_airline.set("No airlines.db file");
                break;
        }
    }

    auto age = entry_.age;
    if (age < 60)
        text_last_seen.set(to_string_dec_uint(age) + " seconds ago");
    else
        text_last_seen.set(to_string_dec_uint(age / 60) + " minutes ago");

    text_callsign.set(entry_.callsign);
    text_infos.set(entry_.info_string);
    std::string str_sil = (entry_.sil > 0) ? " Sil:" + to_string_dec_uint(entry_.sil) : "";
    if (entry_.velo.heading < 360 && entry_.velo.speed >= 0)
        text_info2.set("Hdg:" + to_string_dec_uint(entry_.velo.heading) +
                       " Spd:" + to_string_dec_int(entry_.velo.speed) + str_sil);
    else
        text_info2.set(str_sil);

    text_frame_pos_even.set(to_string_hex_array(entry_.frame_pos_even.get_raw_data(), 14));
    text_frame_pos_odd.set(to_string_hex_array(entry_.frame_pos_odd.get_raw_data(), 14));
}

/* ADSBRxView ********************************************/

ADSBRxView::ADSBRxView(NavigationView& nav) {
    baseband::run_image(portapack::spi_flash::image_tag_adsb_rx);
    add_children(
        {&labels,
         &field_lna,
         &field_vga,
         &field_rf_amp,
         &rssi,
         &recent_entries_view,
         &status_frame,
         &status_good_frame,
         &field_volume});

    recent_entries_view.set_parent_rect({0, 16, 240, 272});
    recent_entries_view.on_select = [this, &nav](const AircraftRecentEntry& entry) {
        detail_key = entry.key();
        details_view = nav.push<ADSBRxDetailsView>(entry);

        nav.set_on_pop([this]() {
            detail_key = AircraftRecentEntry::invalid_key;
            details_view = nullptr;
        });
    };

    signal_token_tick_second = rtc_time::signal_tick_second += [this]() {
        on_tick_second();
    };

    logger = std::make_unique<ADSBLogger>();
    logger->append(logs_dir / u"ADSB.TXT");

    receiver_model.enable();
    baseband::set_adsb();

    if (pmem::beep_on_packets()) {
        audio::set_rate(audio::Rate::Hz_24000);
        audio::output::start();
    }
}

ADSBRxView::~ADSBRxView() {
    rtc_time::signal_tick_second -= signal_token_tick_second;
    audio::output::stop();
    receiver_model.disable();
    baseband::shutdown();
}

void ADSBRxView::focus() {
    field_vga.focus();
}

void ADSBRxView::on_frame(const ADSBFrameMessage* message) {
    auto frame = message->frame;
    uint32_t ICAO_address = frame.get_ICAO_address();
    status_frame.toggle();

    // Bad frame, skip it.
    if (!frame.check_CRC() || ICAO_address == 0)
        return;

    ADSBLogEntry log_entry;
    status_good_frame.toggle();

    rtc::RTC datetime;
    rtcGetTime(&RTCD1, &datetime);  // Reading RTC directly to avoid DST transitions when calculating delta
    frame.set_rx_timestamp(datetime.minute() * 60 + datetime.second());

    // NB: Reference to update entry in-place.
    auto& entry = find_or_create_entry(ICAO_address);
    entry.inc_hit();
    entry.reset_age();

    // Store smoothed amplitude on updates.
    entry.amp = entry.hits == 0
                    ? message->amp
                    : ((entry.amp * 15) + message->amp) >> 4;

    log_entry.raw_data = to_string_hex_array(frame.get_raw_data(), 14);
    log_entry.icao = entry.icao_str;

    if (frame.get_DF() == DF_ADSB) {
        uint8_t msg_type = frame.get_msg_type();
        uint8_t msg_sub = frame.get_msg_sub();
        uint8_t* raw_data = frame.get_raw_data();

        // 4: // surveillance, altitude reply
        if ((msg_type >= AIRCRAFT_ID_L) && (msg_type <= AIRCRAFT_ID_H)) {
            entry.set_callsign(decode_frame_id(frame));
            log_entry.callsign = entry.callsign;
        }

        // 9:
        // 18: // Extended squitter/non-transponder
        // 21: // Comm-B, identity reply
        // 20: // Comm-B, altitude reply
        else if (((msg_type >= AIRBORNE_POS_BARO_L) && (msg_type <= AIRBORNE_POS_BARO_H)) ||
                 ((msg_type >= AIRBORNE_POS_GPS_L) && (msg_type <= AIRBORNE_POS_GPS_H))) {
            entry.set_frame_pos(frame, raw_data[6] & 4);
            log_entry.pos = entry.pos;

            if (entry.pos.valid) {
                std::string str_info =
                    "Alt:" + to_string_dec_int(entry.pos.altitude) +
                    " Lat:" + to_string_decimal(entry.pos.latitude, 2) +
                    " Lon:" + to_string_decimal(entry.pos.longitude, 2);
                entry.set_info_string(std::move(str_info));
            }

        } else if (msg_type == AIRBORNE_VEL && msg_sub >= VEL_GND_SUBSONIC && msg_sub <= VEL_AIR_SUPERSONIC) {
            entry.set_frame_velo(frame);
            log_entry.vel = entry.velo;
            log_entry.vel_type = msg_sub;
        } else if (msg_type == AIRBORNE_OP_STATUS) {  // for ver 1
            entry.sil = frame.get_sil_value();
        }
    }

    logger->log(log_entry);

    if (pmem::beep_on_packets()) {
        baseband::request_audio_beep(1000, 24000, 60);
    }
}

void ADSBRxView::on_tick_second() {
    status_frame.reset();
    status_good_frame.reset();

    ++tick_count;
    ++ticks_since_marker_refresh;

    // Small list, update all at once.
    if (recent.size() <= max_update_entries) {
        update_recent_entries(/*age_delta*/ 1);
        refresh_ui();
        return;
    }

    // Too many items, split update work into two phases:
    // Entry maintenance and UI update.
    if ((tick_count & 1) == 0)
        update_recent_entries(/*age_delta*/ 2);
    else
        refresh_ui();
}

void ADSBRxView::refresh_ui() {
    // There's only one ticks handler, but 3 UIs that need to be updated.
    // This code will dispatch updates to the currently active view.

    if (details_view) {
        // The details view is showing, forward updates to that UI.
        bool current_updated = false;
        bool map_needs_update = false;

        if (details_view->map_active()) {
            // Is it time to clear and refresh the map's markers?
            if (ticks_since_marker_refresh >= MARKER_UPDATE_SECONDS) {
                map_needs_update = true;
                ticks_since_marker_refresh = 0;
                details_view->clear_map_markers();
            }
        } else {
            // Refresh map immediately once active.
            ticks_since_marker_refresh = MARKER_UPDATE_SECONDS;
        }

        // Process the entries list.
        for (const auto& entry : recent) {
            // Found the entry being shown in details view. Update it.
            if (entry.key() == detail_key) {
                details_view->update(entry);
                current_updated = true;
            }

            // NB: current entry also gets a marker so it shows up if map is panned.
            if (map_needs_update && entry.pos.valid && entry.state <= ADSBAgeState::Recent) {
                map_needs_update = details_view->add_map_marker(entry);
            }

            // Any work left to do?
            if (current_updated && !map_needs_update)
                break;
        }
    } else {
        // Main page is the top view. Redraw the entries view.
        recent_entries_view.set_dirty();
    }
}

void ADSBRxView::update_recent_entries(int age_delta) {
    for (auto& entry : recent)
        entry.inc_age(age_delta);

    // Sort and truncate the entries, grouped by state, newest first.
    sort_entries_by_state();
    truncate_entries(recent);
    remove_expired_entries();
}

AircraftRecentEntry& ADSBRxView::find_or_create_entry(uint32_t ICAO_address) {
    // Find ...
    auto it = find(recent, ICAO_address);
    if (it != recent.end())
        return *it;

    // ... or Create.
    return recent.emplace_front(ICAO_address);
}

void ADSBRxView::sort_entries_by_state() {
    recent.sort([](const auto& left, const auto& right) {
        return (left.state < right.state);
    });
}

void ADSBRxView::remove_expired_entries() {
    // NB: Assumes entried are sorted with oldest last.
    auto it = recent.rbegin();
    auto end = recent.rend();

    // Find the first !expired entry from the back.
    while (it != end) {
        if (it->state != ADSBAgeState::Expired)
            break;

        std::advance(it, 1);
    }

    // Remove the range of expired items.
    recent.erase(it.base(), recent.end());
}

} /* namespace ui */
