/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2017 Furrtek
 * Copyright (C) 2023 Kyle Reed
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

using namespace portapack;

namespace ui {

bool ac_details_view_active{false};  // WTF?

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
            target_color = Color::green();
            break;
        case ADSBAgeState::Recent:
            entry_string = STR_COLOR_LIGHT_GREY;
            target_color = Color::light_grey();
            break;
        default:
            entry_string = STR_COLOR_DARK_GREY;
            target_color = Color::grey();
    };

    entry_string +=
        (entry.callsign[0] != ' ' ? entry.callsign + " " : entry.icao_str + "   ") +
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
        painter.draw_bitmap(target_rect.location() + Point(8 * 8, 0), bitmap_target, target_color, style.background);
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

    log_file.write_entry(log_line);
}

/* ADSBRxAircraftDetailsView *****************************/

ADSBRxAircraftDetailsView::ADSBRxAircraftDetailsView(
    NavigationView& nav,
    const AircraftRecentEntry& entry,
    const std::function<void(void)> on_close)
    : entry_copy(entry),
      on_close_(on_close) {
    add_children({&labels,
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

    std::unique_ptr<ADSBLogger> logger{};

    icao_code = to_string_hex(entry_copy.ICAO_address, 6);
    text_icao_address.set(to_string_hex(entry_copy.ICAO_address, 6));

    // Try getting the aircraft information from icao24.db
    return_code = db.retrieve_aircraft_record(&aircraft_record, icao_code);
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
                text_number_of_engines.set(std::string(1, aircraft_record.icao_type[1]));
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
        case DATABASE_NOT_FOUND:
            text_manufacturer.set("No icao24.db file");
            break;
    }
    button_close.on_select = [&nav](Button&) {
        ac_details_view_active = false;
        nav.pop();
    };
}

ADSBRxAircraftDetailsView::~ADSBRxAircraftDetailsView() {
    on_close_();
}

void ADSBRxAircraftDetailsView::focus() {
    button_close.focus();
}

void ADSBRxDetailsView::update(const AircraftRecentEntry& entry) {
    entry_copy = entry;
    uint32_t age = entry_copy.age;

    if (age < 60)
        text_last_seen.set(to_string_dec_uint(age) + " seconds ago");
    else
        text_last_seen.set(to_string_dec_uint(age / 60) + " minutes ago");

    text_infos.set(entry_copy.info_string);
    if (entry_copy.velo.heading < 360 && entry_copy.velo.speed >= 0) {  // I don't like this but...
        text_info2.set("Hdg:" + to_string_dec_uint(entry_copy.velo.heading) + " Spd:" + to_string_dec_int(entry_copy.velo.speed));
    } else {
        text_info2.set("");
    }
    text_frame_pos_even.set(to_string_hex_array(entry_copy.frame_pos_even.get_raw_data(), 14));
    text_frame_pos_odd.set(to_string_hex_array(entry_copy.frame_pos_odd.get_raw_data(), 14));

    if (send_updates) {
        geomap_view->update_tag(trimr(entry.callsign[0] != ' ' ? entry.callsign : to_string_hex(entry.ICAO_address, 6)));
        geomap_view->update_position(entry_copy.pos.latitude, entry_copy.pos.longitude, entry_copy.velo.heading, entry_copy.pos.altitude);
    }
}

/* ADSBRxDetailsView *************************************/

ADSBRxDetailsView::ADSBRxDetailsView(
    NavigationView& nav,
    const AircraftRecentEntry& entry,
    const std::function<void(void)> on_close)
    : entry_copy(entry),
      on_close_(on_close) {
    add_children({&labels,
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

    std::unique_ptr<ADSBLogger> logger{};
    update(entry_copy);

    // The following won't (shouldn't !) change for a given airborne aircraft
    // Try getting the airline's name from airlines.db
    airline_code = entry_copy.callsign.substr(0, 3);
    return_code = db.retrieve_airline_record(&airline_record, airline_code);
    switch (return_code) {
        case DATABASE_RECORD_FOUND:
            text_airline.set(airline_record.airline);
            text_country.set(airline_record.country);
            break;
        case DATABASE_NOT_FOUND:
            text_airline.set("No airlines.db file");
            break;
    }

    text_callsign.set(entry_copy.callsign);
    text_icao_address.set(to_string_hex(entry_copy.ICAO_address, 6));

    button_aircraft_details.on_select = [this, &nav](Button&) {
        ac_details_view_active = true;
        aircraft_details_view = nav.push<ADSBRxAircraftDetailsView>(entry_copy, [this]() { send_updates = false; });
        send_updates = false;
    };

    button_see_map.on_select = [this, &nav](Button&) {
        if (!send_updates) {  // Prevent recursively launching the map
            geomap_view = nav.push<GeoMapView>(
                trimr(entry_copy.callsign[0] != ' ' ? entry_copy.callsign : entry_copy.icao_str),
                entry_copy.pos.altitude,
                GeoPos::alt_unit::FEET,
                entry_copy.pos.latitude,
                entry_copy.pos.longitude,
                entry_copy.velo.heading,
                [this]() {
                    send_updates = false;
                });
            send_updates = true;
        }
    };
};

ADSBRxDetailsView::~ADSBRxDetailsView() {
    ac_details_view_active = false;
    on_close_();
}

void ADSBRxDetailsView::focus() {
    button_see_map.focus();
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
         &status_good_frame});

    recent_entries_view.set_parent_rect({0, 16, 240, 272});
    recent_entries_view.on_select = [this, &nav](const AircraftRecentEntry& entry) {
        detailed_entry_key = entry.key();
        details_view = nav.push<ADSBRxDetailsView>(
            entry,
            [this]() {
                send_updates = false;
            });
        send_updates = true;
    };

    signal_token_tick_second = rtc_time::signal_tick_second += [this]() {
        on_tick_second();
    };

    logger = std::make_unique<ADSBLogger>();
    logger->append(LOG_ROOT_DIR "/ADSB.TXT");

    receiver_model.enable();
    baseband::set_adsb();
}

ADSBRxView::~ADSBRxView() {
    rtc_time::signal_tick_second -= signal_token_tick_second;
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
    rtcGetTime(&RTCD1, &datetime);
    frame.set_rx_timestamp(datetime.minute() * 60 + datetime.second());

    // NB: Ref to update entry in-place.
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
        }
    }

    logger->log(log_entry);
}

void ADSBRxView::on_tick_second() {
    status_frame.reset();
    status_good_frame.reset();

    // Update in single pass.
    if (recent.size() <= max_update_entries) {
        update_details_and_map(/*age_delta*/ 1);
        update_recent_entries();
        return;
    }

    // Too many, split update into two passes.
    ++tick_count;
    if ((tick_count & 1) == 1)
        update_details_and_map(/*age_delta*/ 2);
    else
        update_recent_entries();
}

void ADSBRxView::update_details_and_map(int ageStep) {
    ui::GeoMarker marker;
    bool storeNewMarkers = false;

    // NB: Temporarily pausing updates in rtc_timer_tick context when viewing AC Details screen (kludge for some Guru faults)
    // TODO: More targeted blocking of updates in rtc_timer_tick when ADSB processes are running
    if (ac_details_view_active)
        return;

    // Sort and truncate the entries, grouped by state, newest first.
    sort_entries_by_state();
    truncate_entries(recent);
    remove_expired_entries();

    // Calculate if it is time to update markers
    if (send_updates && details_view && details_view->geomap_view) {
        ticksSinceMarkerRefresh += ageStep;
        if (ticksSinceMarkerRefresh >= MARKER_UPDATE_SECONDS) {  // Update other aircraft every few seconds
            storeNewMarkers = true;
            ticksSinceMarkerRefresh = 0;
        }
    } else {
        ticksSinceMarkerRefresh = MARKER_UPDATE_SECONDS;  // Send the markers as soon as the geoview exists
    }

    // Increment age, and pass updates to the details and map
    const bool otherMarkersCanBeSent = send_updates && storeNewMarkers && details_view && details_view->geomap_view;  // Save retesting all of this
    MapMarkerStored markerStored = MARKER_NOT_STORED;
    if (otherMarkersCanBeSent) {
        details_view->geomap_view->clear_markers();
    }
    // Loop through all entries
    for (auto& entry : recent) {
        entry.inc_age(ageStep);

        // Only if there is a details view
        if (send_updates && details_view) {
            if (entry.key() == detailed_entry_key)  // Check if the ICAO address match
            {
                details_view->update(entry);
            }
            // Store if the view is present and the list isn't full
            // Note -- Storing the selected entry too, in case map panning occurs
            if (otherMarkersCanBeSent && (markerStored != MARKER_LIST_FULL) && entry.pos.valid && (toUType(entry.state) <= 2)) {
                marker.lon = entry.pos.longitude;
                marker.lat = entry.pos.latitude;
                marker.angle = entry.velo.heading;
                marker.tag = trimr(entry.callsign[0] != ' ' ? entry.callsign : entry.icao_str);
                markerStored = details_view->geomap_view->store_marker(marker);
            }
        }
    }  // Loop through all entries, if only to update the age
}

void ADSBRxView::update_recent_entries() {
    // Redraw the list of aircraft
    if (!send_updates) {
        recent_entries_view.set_dirty();
    }
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
