/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
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

#include "ais_app.hpp"

#include "string_format.hpp"
#include "database.hpp"

#include "baseband_api.hpp"

#include "portapack.hpp"
using namespace portapack;

#include <algorithm>

namespace ais {
namespace format {

static std::string latlon_abs_normalized(const int32_t normalized, const char suffixes[2]) {
    const auto suffix = suffixes[(normalized < 0) ? 0 : 1];
    const uint32_t normalized_abs = std::abs(normalized);
    const uint32_t t = (normalized_abs * 5) / 3;
    const uint32_t degrees = t / (100 * 10000);
    const uint32_t fraction = t % (100 * 10000);
    return to_string_dec_uint(degrees) + "." + to_string_dec_uint(fraction, 6, '0') + suffix;
}

static std::string latlon(const Latitude latitude, const Longitude longitude) {
    if (latitude.is_valid() && longitude.is_valid()) {
        return latlon_abs_normalized(latitude.normalized(), "SN") + " " + latlon_abs_normalized(longitude.normalized(), "WE");
    } else if (latitude.is_not_available() && longitude.is_not_available()) {
        return "not available";
    } else {
        return "invalid";
    }
}

static float latlon_float(const int32_t normalized) {
    return ((((float)normalized) * 5) / 3) / (100 * 10000);
}

static std::string mmsi(
    const ais::MMSI& mmsi) {
    return to_string_dec_uint(mmsi, 9, '0');  // MMSI is always is always 9 characters pre-padded with zeros
}

static std::string mid(
    const ais::MMSI& mmsi) {
    database db;
    std::string mid_code = "";
    database::MidDBRecord mid_record = {};
    int return_code = 0;

    // Try getting the country name from mids.db using MID code for given MMSI
    mid_code = to_string_dec_uint(mmsi, 9, ' ').substr(0, 3);
    return_code = db.retrieve_mid_record(&mid_record, mid_code);
    switch (return_code) {
        case DATABASE_RECORD_FOUND:
            return mid_record.country;
        case DATABASE_NOT_FOUND:
            return "No mids.db file";
        default:
            return "";
    }
}

static std::string text(const std::string& text) {
    size_t end = text.find_last_not_of("@");
    return (end == std::string::npos) ? "" : text.substr(0, end + 1);
}

static std::string navigational_status(const unsigned int value) {
    switch (value) {
        case 0:
            return "under way w/engine";
        case 1:
            return "at anchor";
        case 2:
            return "not under command";
        case 3:
            return "restricted maneuv";
        case 4:
            return "constrained draught";
        case 5:
            return "moored";
        case 6:
            return "aground";
        case 7:
            return "fishing";
        case 8:
            return "sailing";
        case 9:
        case 10:
        case 13:
            return "reserved";
        case 11:
            return "towing astern";
        case 12:
            return "towing ahead/along";
        case 14:
            return "SART/MOB/EPIRB";
        case 15:
            return "undefined";
        default:
            return "unknown";
    }
}

static std::string rate_of_turn(const RateOfTurn value) {
    switch (value) {
        case -128:
            return "not available";
        case -127:
            return "left >5 deg/30sec";
        case 0:
            return "0 deg/min";
        case 127:
            return "right >5 deg/30sec";
        default: {
            std::string result = (value < 0) ? "left " : "right ";
            const float value_deg_sqrt = value / 4.733f;
            const int32_t value_deg = value_deg_sqrt * value_deg_sqrt;
            result += to_string_dec_uint(value_deg);
            result += " deg/min";
            return result;
        }
    }
}

static std::string speed_over_ground(const SpeedOverGround value) {
    if (value == 1023) {
        return "not available";
    } else if (value == 1022) {
        return ">= 102.2 knots";
    } else {
        return to_string_dec_uint(value / 10) + "." + to_string_dec_uint(value % 10, 1) + " knots";
    }
}

static std::string course_over_ground(const CourseOverGround value) {
    if (value > 3600) {
        return "invalid";
    } else if (value == 3600) {
        return "not available";
    } else {
        return to_string_dec_uint(value / 10) + "." + to_string_dec_uint(value % 10, 1) + " deg";
    }
}

static std::string true_heading(const TrueHeading value) {
    if (value == 511) {
        return "not available";
    } else if (value > 359) {
        return "invalid";
    } else {
        return to_string_dec_uint(value) + " deg";
    }
}

} /* namespace format */
} /* namespace ais */

void AISLogger::on_packet(const ais::Packet& packet) {
    // TODO: Unstuff here, not in baseband!
    std::string entry;
    entry.reserve((packet.length() + 3) / 4);

    for (size_t i = 0; i < packet.length(); i += 4) {
        const auto nibble = packet.read(i, 4);
        entry += (nibble >= 10) ? ('W' + nibble) : ('0' + nibble);
    }

    log_file.write_entry(packet.received_at(), entry);
}

void AISRecentEntry::update(const ais::Packet& packet) {
    received_count++;

    switch (packet.message_id()) {
        case 1:
        case 2:
        case 3:
            navigational_status = packet.read(38, 4);
            last_position.rate_of_turn = packet.read(42, 8);
            last_position.speed_over_ground = packet.read(50, 10);
            last_position.timestamp = packet.received_at();
            last_position.latitude = packet.latitude(89);
            last_position.longitude = packet.longitude(61);
            last_position.course_over_ground = packet.read(116, 12);
            last_position.true_heading = packet.read(128, 9);
            break;

        case 4:
            last_position.timestamp = packet.received_at();
            last_position.latitude = packet.latitude(107);
            last_position.longitude = packet.longitude(79);
            break;

        case 5:
            call_sign = packet.text(70, 7);
            name = packet.text(112, 20);
            destination = packet.text(302, 20);
            break;

        case 18:
            last_position.timestamp = packet.received_at();
            last_position.speed_over_ground = packet.read(46, 10);
            last_position.latitude = packet.latitude(85);
            last_position.longitude = packet.longitude(57);
            last_position.course_over_ground = packet.read(112, 12);
            last_position.true_heading = packet.read(124, 9);
            break;

        case 21:
            name = packet.text(43, 20);
            last_position.timestamp = packet.received_at();
            last_position.latitude = packet.latitude(192);
            last_position.longitude = packet.longitude(164);
            break;

        case 24:
            switch (packet.read(38, 2)) {
                case 0:
                    name = packet.text(40, 20);
                    break;

                case 1:
                    call_sign = packet.text(90, 7);
                    break;

                default:
                    break;
            }

            break;

        default:
            break;
    }
}

namespace ui {

template <>
void RecentEntriesTable<AISRecentEntries>::draw(
    const Entry& entry,
    const Rect& target_rect,
    Painter& painter,
    const Style& style) {
    std::string line = ais::format::mmsi(entry.mmsi) + " ";
    if (!entry.name.empty()) {
        line += ais::format::text(entry.name);
    } else {
        line += ais::format::text(entry.call_sign);
    }

    line.resize(target_rect.width() / 8, ' ');
    painter.draw_string(target_rect.location(), style, line);
}

AISRecentEntryDetailView::AISRecentEntryDetailView(NavigationView& nav) {
    add_children({
        &button_done,
        &button_see_map,
    });

    button_done.on_select = [this](const ui::Button&) {
        if (on_close) {
            on_close();
        }
    };

    button_see_map.on_select = [this, &nav](Button&) {
        geomap_view = nav.push<GeoMapView>(
            ais::format::text(entry_.name),
            0,
            GeoPos::alt_unit::METERS,
            GeoPos::spd_unit::NONE,
            ais::format::latlon_float(entry_.last_position.latitude.normalized()),
            ais::format::latlon_float(entry_.last_position.longitude.normalized()),
            entry_.last_position.true_heading,
            [this]() {
                send_updates = false;
            });
        send_updates = true;
    };
}

AISRecentEntryDetailView::AISRecentEntryDetailView(const AISRecentEntryDetailView& Entry)
    : View() {
    (void)Entry;
}

AISRecentEntryDetailView& AISRecentEntryDetailView::operator=(const AISRecentEntryDetailView& Entry) {
    (void)Entry;
    return *this;
}

void AISRecentEntryDetailView::update_position() {
    if (send_updates)
        geomap_view->update_position(ais::format::latlon_float(entry_.last_position.latitude.normalized()), ais::format::latlon_float(entry_.last_position.longitude.normalized()), (float)entry_.last_position.true_heading, 0, entry_.last_position.speed_over_ground);
}

void AISRecentEntryDetailView::focus() {
    button_done.focus();
}

Rect AISRecentEntryDetailView::draw_field(
    Painter& painter,
    const Rect& draw_rect,
    const Style& style,
    const std::string& label,
    const std::string& value) {
    const int label_length_max = 4;

    painter.draw_string(Point{draw_rect.left(), draw_rect.top()}, style, label);
    painter.draw_string(Point{draw_rect.left() + (label_length_max + 1) * 8, draw_rect.top()}, style, value);

    return {draw_rect.left(), draw_rect.top() + draw_rect.height(), draw_rect.width(), draw_rect.height()};
}

void AISRecentEntryDetailView::paint(Painter& painter) {
    View::paint(painter);

    const auto s = style();
    const auto rect = screen_rect();

    auto field_rect = Rect{rect.left(), rect.top() + 16, rect.width(), 16};

    field_rect = draw_field(painter, field_rect, s, "MMSI", ais::format::mmsi(entry_.mmsi));
    field_rect = draw_field(painter, field_rect, s, "Ctry", ais::format::mid(entry_.mmsi));
    field_rect = draw_field(painter, field_rect, s, "Name", ais::format::text(entry_.name));
    field_rect = draw_field(painter, field_rect, s, "Call", ais::format::text(entry_.call_sign));
    field_rect = draw_field(painter, field_rect, s, "Dest", ais::format::text(entry_.destination));
    field_rect = draw_field(painter, field_rect, s, "Last", to_string_datetime(entry_.last_position.timestamp));
    field_rect = draw_field(painter, field_rect, s, "Pos ", ais::format::latlon(entry_.last_position.latitude, entry_.last_position.longitude));
    field_rect = draw_field(painter, field_rect, s, "Stat", ais::format::navigational_status(entry_.navigational_status));
    field_rect = draw_field(painter, field_rect, s, "RoT ", ais::format::rate_of_turn(entry_.last_position.rate_of_turn));
    field_rect = draw_field(painter, field_rect, s, "SoG ", ais::format::speed_over_ground(entry_.last_position.speed_over_ground));
    field_rect = draw_field(painter, field_rect, s, "CoG ", ais::format::course_over_ground(entry_.last_position.course_over_ground));
    field_rect = draw_field(painter, field_rect, s, "Head", ais::format::true_heading(entry_.last_position.true_heading));
    field_rect = draw_field(painter, field_rect, s, "Rx #", to_string_dec_uint(entry_.received_count));
}

void AISRecentEntryDetailView::set_entry(const AISRecentEntry& entry) {
    entry_ = entry;
    set_dirty();
}

AISAppView::AISAppView(NavigationView& nav)
    : nav_{nav} {
    baseband::run_image(portapack::spi_flash::image_tag_ais);

    add_children({
        &label_channel,
        &options_channel,
        &field_rf_amp,
        &field_lna,
        &field_vga,
        &rssi,
        &channel,
        &recent_entries_view,
        &recent_entry_detail_view,
    });

    recent_entry_detail_view.hidden(true);

    receiver_model.enable();

    options_channel.on_change = [this](size_t, OptionsField::value_t v) {
        receiver_model.set_target_frequency(v);
    };
    options_channel.set_by_value(receiver_model.target_frequency());

    recent_entries_view.on_select = [this](const AISRecentEntry& entry) {
        on_show_detail(entry);
    };
    recent_entry_detail_view.on_close = [this]() {
        on_show_list();
    };

    logger = std::make_unique<AISLogger>();
    if (logger) {
        logger->append(LOG_ROOT_DIR "/AIS.TXT");
    }
}

AISAppView::~AISAppView() {
    receiver_model.disable();
    baseband::shutdown();
}

void AISAppView::focus() {
    options_channel.focus();
}

void AISAppView::set_parent_rect(const Rect new_parent_rect) {
    View::set_parent_rect(new_parent_rect);
    const Rect content_rect{0, header_height, new_parent_rect.width(), new_parent_rect.height() - header_height};
    recent_entries_view.set_parent_rect(content_rect);
    recent_entry_detail_view.set_parent_rect(content_rect);
}

void AISAppView::on_packet(const ais::Packet& packet) {
    if (logger) {
        logger->on_packet(packet);
    }

    auto& entry = ::on_packet(recent, packet.source_id());
    entry.update(packet);
    recent_entries_view.set_dirty();

    // TODO: Crude hack, should be a more formal listener arrangement...
    if (entry.key() == recent_entry_detail_view.entry().key()) {
        recent_entry_detail_view.set_entry(entry);
        recent_entry_detail_view.update_position();
    }
}

void AISAppView::on_show_list() {
    recent_entries_view.hidden(false);
    recent_entry_detail_view.hidden(true);
    recent_entries_view.focus();
}

void AISAppView::on_show_detail(const AISRecentEntry& entry) {
    recent_entries_view.hidden(true);
    recent_entry_detail_view.hidden(false);
    recent_entry_detail_view.set_entry(entry);
    recent_entry_detail_view.focus();
}

} /* namespace ui */
