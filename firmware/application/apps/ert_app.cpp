/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2023 Mark Thompson
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

#include "ert_app.hpp"

#include "baseband_api.hpp"

#include "portapack.hpp"
using namespace portapack;

#include "manchester.hpp"

#include "crc.hpp"
#include "string_format.hpp"

namespace ert {

namespace format {

std::string type(Packet::Type value) {
    switch (value) {
        default:
        case Packet::Type::Unknown:
            return "???";
        case Packet::Type::IDM:
            return "IDM";
        case Packet::Type::SCM:
            return "SCM";
        case Packet::Type::SCMPLUS:
            return "SCM+";
    }
}

std::string id(ID value) {
    return to_string_dec_uint(value, 10);
}

std::string consumption(Consumption value) {
    return to_string_dec_uint(value, 8);
}

std::string commodity_type(CommodityType value) {
    return to_string_dec_uint(value, 2);
}

std::string tamper_flags(TamperFlags value) {
    return to_string_hex(value & 0xFFFF, 4);  // Note: ignoring bits 32-47 of tamper flags in IDM type due to screen width
}

std::string tamper_flags_scm(TamperFlags value) {
    return " " + to_string_hex(value & 0x0F, 1) + "/" + to_string_hex(value >> 4, 1);  // Physical/Encoder flags
}

} /* namespace format */

} /* namespace ert */

void ERTLogger::on_packet(const ert::Packet& packet, const uint32_t target_frequency) {
    const auto formatted = packet.symbols_formatted();
    const auto target_frequency_str = to_string_dec_uint(target_frequency, 10);

    std::string entry = target_frequency_str + " " + ert::format::type(packet.type()) + " " + formatted.data + "/" + formatted.errors + " ID:" + to_string_dec_uint(packet.id(), 1);

    log_file.write_entry(packet.received_at(), entry);
}

const ERTRecentEntry::Key ERTRecentEntry::invalid_key{};

void ERTRecentEntry::update(const ert::Packet& packet) {
    received_count++;

    last_consumption = packet.consumption();
    last_tamper_flags = packet.tamper_flags();
    packet_type = packet.type();
}

namespace ui {

template <>
void RecentEntriesTable<ERTRecentEntries>::draw(
    const Entry& entry,
    const Rect& target_rect,
    Painter& painter,
    const Style& style) {
    std::string line = ert::format::id(entry.id) + " " + ert::format::commodity_type(entry.commodity_type) + " " + ert::format::consumption(entry.last_consumption) + " ";

    line += (entry.packet_type == ert::Packet::Type::SCM) ? ert::format::tamper_flags_scm(entry.last_tamper_flags) : ert::format::tamper_flags(entry.last_tamper_flags);
    line += (entry.received_count > 99) ? " ++" : to_string_dec_uint(entry.received_count, 3);

    line.resize(target_rect.width() / 8, ' ');
    painter.draw_string(target_rect.location(), style, line);
}

ERTAppView::ERTAppView(NavigationView& nav)
    : nav_{nav} {
    baseband::run_image(portapack::spi_flash::image_tag_ert);

    add_children({
        &field_frequency,
        &field_rf_amp,
        &field_lna,
        &field_vga,
        &rssi,
        &recent_entries_view,
    });

    field_frequency.set_step(1000000);

    receiver_model.enable();

    logger = std::make_unique<ERTLogger>();
    if (logger) {
        logger->append(LOG_ROOT_DIR "/ERT.TXT");
    }
}

ERTAppView::~ERTAppView() {
    receiver_model.disable();
    baseband::shutdown();
}

void ERTAppView::focus() {
    field_vga.focus();
}

void ERTAppView::set_parent_rect(const Rect new_parent_rect) {
    View::set_parent_rect(new_parent_rect);
    recent_entries_view.set_parent_rect({0, header_height, new_parent_rect.width(), new_parent_rect.height() - header_height});
}

void ERTAppView::on_packet(const ert::Packet& packet) {
    if (logger) {
        logger->on_packet(packet, receiver_model.target_frequency());
    }

    if (packet.crc_ok()) {
        auto& entry = ::on_packet(recent, ERTRecentEntry::Key{packet.id(), packet.commodity_type()});
        entry.update(packet);
        recent_entries_view.set_dirty();
    }
}

void ERTAppView::on_show_list() {
    recent_entries_view.hidden(false);
    recent_entries_view.focus();
}

} /* namespace ui */
