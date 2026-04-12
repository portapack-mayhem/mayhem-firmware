/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2023 Mark Thompson
 * Copyright (C) 2025 Speedster04 (TPMS EU — 433.92 MHz EU protocol support)
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

#include "tpms_eu_app.hpp"

#include "baseband_api.hpp"
#include "audio.hpp"
#include "portapack.hpp"
using namespace portapack;

#include "string_format.hpp"
#include "utility.hpp"
#include "file_path.hpp"

namespace pmem = portapack::persistent_memory;

namespace ui::external_app::tpmseu {

namespace format {

std::string type(tpms::Reading::Type type) {
    return to_string_dec_uint(toUType(type), 2);
}

// ---------------------------------------------------------------------------
// type_name() — returns human-readable sensor protocol name.
//
// EU protocols (types 6-8) are named after the sensor manufacturer/protocol,
// NOT the vehicle brand (a single sensor type is used across many brands).
// Vehicle brand mappings:
//   Ford     = VDO/Continental S180084730Z (Ford Fiesta/Focus/Kuga/Escape/Transit)
//   Cit/PSA  = PSA Group sensors (Citroën, Peugeot, Fiat, Mitsubishi)
//   Renault  = Renault Clio/Captur/Zoe, Dacia Sandero
// ---------------------------------------------------------------------------
std::string type_name(tpms::Reading::Type type) {
    switch (type) {
        case tpms::Reading::Type::None:
            return "None";
        case tpms::Reading::Type::FLM_64:
            return "FLM_64";
        case tpms::Reading::Type::FLM_72:
            return "FLM_72";
        case tpms::Reading::Type::FLM_80:
            return "FLM_80";
        case tpms::Reading::Type::Schrader:
            return "Schrader";
        case tpms::Reading::Type::GMC_96:
            return "GMC_96";
        // EU extensions:
        case tpms::Reading::Type::Ford:
            return "Ford/VDO";
        case tpms::Reading::Type::Citroen_PSA:
            return "Cit/PSA";
        case tpms::Reading::Type::Renault:
            return "Renault";
        // Phase 2 EU (proc_tpms_eu required):
        case tpms::Reading::Type::BMW_G45:
            return "BMW G4/5";   // Also: Audi, HUF/Beru, Continental, Schrader/Sensata
        case tpms::Reading::Type::BMW_G23:
            return "BMW G2/3";
        case tpms::Reading::Type::Porsche:
            return "Porsche987";
        default:
            return "Unknown";
    }
}

std::string id(tpms::TransponderID id) {
    return to_string_hex(id.value(), 8);
}

std::string pressure(Pressure pressure) {
    return to_string_dec_int(
        pressure_unit == PRESSURE_UNIT_PSI   ? pressure.psi()
        : pressure_unit == PRESSURE_UNIT_BAR ? pressure.bar()
                                             : pressure.kilopascal(),
        3);
}

std::string temperature(Temperature temperature) {
    return to_string_dec_int(
        temp_unit == TEMP_UNIT_CELSIUS ? temperature.celsius() : temperature.fahrenheit(), 3);
}

std::string flags(tpms::Flags flags) {
    return to_string_hex(flags, 2);
}

static std::string signal_type(tpms::SignalType signal_type) {
    switch (signal_type) {
        case tpms::SignalType::FSK_19k2_Schrader:
            return "FSK 38400 19200 Schrader";
        case tpms::SignalType::OOK_8k192_Schrader:
            return "OOK - 8192 Schrader";
        case tpms::SignalType::OOK_8k4_Schrader:
            return "OOK - 8400 Schrader";
        default:
            return "- - - -";
    }
}

}  // namespace format

// ---------------------------------------------------------------------------
// Logger
// ---------------------------------------------------------------------------

void TPMSEULogger::on_packet(const tpms::Packet& packet, const uint32_t target_frequency) {
    const auto hex_formatted = packet.symbols_formatted();
    const auto target_frequency_str = to_string_dec_uint(target_frequency, 10);

    std::string entry = target_frequency_str + " " +
                        ui::external_app::tpmseu::format::signal_type(packet.signal_type()) +
                        " " + hex_formatted.data + "/" + hex_formatted.errors;
    log_file.write_entry(packet.received_at(), entry);
}

// ---------------------------------------------------------------------------
// TPMSEURecentEntry
// ---------------------------------------------------------------------------

const TPMSEURecentEntry::Key TPMSEURecentEntry::invalid_key = {tpms::Reading::Type::None, 0};

void TPMSEURecentEntry::update(const tpms::Reading& reading) {
    received_count++;

    if (reading.pressure().is_valid()) {
        last_pressure = reading.pressure();
    }
    if (reading.temperature().is_valid()) {
        last_temperature = reading.temperature();
    }
    if (reading.flags().is_valid()) {
        last_flags = reading.flags();
    }
}

// ---------------------------------------------------------------------------
// TPMSEUAppView — main view
// ---------------------------------------------------------------------------

TPMSEUAppView::TPMSEUAppView(NavigationView& nav)
    : nav_{nav} {
    baseband::run_prepared_image(portapack::memory::map::m4_code.base());

    add_children({&rssi,
                  &field_volume,
                  &channel,
                  &options_band,
                  &options_pressure,
                  &options_temperature,
                  &field_rf_amp,
                  &field_lna,
                  &field_vga,
                  &recent_entries_view});

    receiver_model.enable();

    options_band.on_change = [this](size_t, OptionsField::value_t v) {
        receiver_model.set_target_frequency(v);
    };
    options_band.set_by_value(receiver_model.target_frequency());

    options_pressure.on_change = [this](size_t, int32_t i) {
        format::pressure_unit = (uint8_t)i;
        update_view();
    };
    options_pressure.set_by_value(format::pressure_unit);

    options_temperature.on_change = [this](size_t, int32_t i) {
        format::temp_unit = (uint8_t)i;
        update_view();
    };
    options_temperature.set_by_value(format::temp_unit);

    recent_entries_view.on_select = [this](const TPMSEURecentEntry& entry) {
        on_show_detail(entry);
    };

    logger = std::make_unique<TPMSEULogger>();
    if (logger) {
        logger->append(logs_dir / u"TPMS_EU.TXT");
    }

    if (pmem::beep_on_packets()) {
        audio::set_rate(audio::Rate::Hz_24000);
        audio::output::start();
    }
}

TPMSEUAppView::~TPMSEUAppView() {
    audio::output::stop();
    receiver_model.disable();
    baseband::shutdown();
}

void TPMSEUAppView::focus() {
    options_band.focus();
}

void TPMSEUAppView::update_view() {
    recent_entries_view.set_parent_rect(view_normal_rect);
}

void TPMSEUAppView::set_parent_rect(const Rect new_parent_rect) {
    View::set_parent_rect(new_parent_rect);

    view_normal_rect = {0, header_height, new_parent_rect.width(),
                        new_parent_rect.height() - header_height};
    update_view();
}

void TPMSEUAppView::on_packet(const tpms::Packet& packet) {
    if (logger) {
        logger->on_packet(packet, receiver_model.target_frequency());
    }

    const auto reading_opt = packet.reading();
    if (reading_opt.is_valid()) {
        const auto reading = reading_opt.value();
        auto& entry = ::on_packet(recent, TPMSEURecentEntry::Key{reading.type(), reading.id()});
        entry.update(reading);
        entry.signal_type = packet.signal_type();
        recent_entries_view.set_dirty();
    }

    if (pmem::beep_on_packets()) {
        baseband::request_audio_beep(1000, 24000, 60);
    }
}

void TPMSEUAppView::on_show_list() {
    recent_entries_view.hidden(false);
    recent_entries_view.focus();
}

void TPMSEUAppView::on_show_detail(const TPMSEURecentEntry& entry) {
    nav_.push<TPMSEURecentEntryDetailView>(entry);
}

// ---------------------------------------------------------------------------
// TPMSEURecentEntryDetailView
// ---------------------------------------------------------------------------

TPMSEURecentEntryDetailView::TPMSEURecentEntryDetailView(NavigationView& nav,
                                                         const TPMSEURecentEntry& entry)
    : nav_{nav},
      entry_{entry} {
    add_children({&labels,
                  &text_type,
                  &text_id,
                  &text_pressure,
                  &text_temperature,
                  &text_flags,
                  &text_count,
                  &button_done,
                  &button_save});

    text_type.set(format::type_name(entry.type));
    text_id.set(to_string_hex(entry.id.value(), 8));

    if (entry.last_pressure.is_valid()) {
        std::string pressure_str = format::pressure(entry.last_pressure.value());
        std::string unit_str = format::pressure_unit == PRESSURE_UNIT_PSI   ? " PSI"
                               : format::pressure_unit == PRESSURE_UNIT_BAR ? " BAR"
                                                                             : " kPa";
        text_pressure.set(pressure_str + unit_str);
    } else {
        text_pressure.set("---");
    }

    if (entry.last_temperature.is_valid()) {
        text_temperature.set(
            to_string_dec_int(entry.last_temperature.value().celsius(), 3) + " C");
    } else {
        text_temperature.set("---");
    }

    if (entry.last_flags.is_valid()) {
        // Show flags as hex; for Ford: bit6=moving, bit3=learn, bit2=at-rest
        text_flags.set(to_string_hex(entry.last_flags.value(), 2));
    } else {
        text_flags.set("--");
    }

    text_count.set(to_string_dec_uint(entry.received_count, 4));

    button_done.on_select = [&nav](Button&) {
        nav.pop();
    };

    button_save.on_select = [this](Button&) {
        on_save();
    };
}

void TPMSEURecentEntryDetailView::focus() {
    button_done.focus();
}

void TPMSEURecentEntryDetailView::set_entry(const TPMSEURecentEntry& entry) {
    entry_ = entry;
}

void TPMSEURecentEntryDetailView::on_save() {
    auto timestamp = to_string_timestamp(rtc_time::now());
    std::string file_name = "TPMS_" + timestamp + ".TXT";
    ensure_directory(tpms_dir);
    auto file_path = tpms_dir / file_name;

    if (save_file(file_path)) {
        nav_.display_modal("Saved", "Packet saved to:\n" + file_name);
    } else {
        nav_.display_modal("Error", "Failed to save\npacket");
    }
}

bool TPMSEURecentEntryDetailView::save_file(const std::filesystem::path& path) {
    File f;
    auto error = f.create(path);
    if (error.is_valid())
        return false;

    // Compatible with TPMS TX save format
    std::string content = "Type=" + to_string_dec_uint(toUType(entry_.type), 1) + "\n";
    content += "ID=" + to_string_hex(entry_.id.value(), 8) + "\n";

    if (entry_.last_pressure.is_valid()) {
        content += "Pressure=" +
                   to_string_dec_int(entry_.last_pressure.value().kilopascal(), 1) + "\n";
    } else {
        content += "Pressure=240\n";
    }

    if (entry_.last_temperature.is_valid()) {
        content += "Temperature=" +
                   to_string_dec_int(entry_.last_temperature.value().celsius(), 1) + "\n";
    } else {
        content += "Temperature=25\n";
    }

    if (entry_.last_flags.is_valid()) {
        content += "Flags=" + to_string_hex(entry_.last_flags.value(), 2) + "\n";
    } else {
        content += "Flags=00\n";
    }

    content += "SignalType=" + to_string_dec_uint(toUType(entry_.signal_type), 1) + "\n";

    f.write(content.c_str(), content.length());
    return true;
}

// ---------------------------------------------------------------------------
// RecentEntriesTable specialisation — list row renderer
// ---------------------------------------------------------------------------

}  // namespace ui::external_app::tpmseu

namespace ui {

template <>
void RecentEntriesTable<ui::external_app::tpmseu::TPMSEURecentEntries>::draw(
    const Entry& entry,
    const Rect& target_rect,
    Painter& painter,
    const Style& style,
    RecentEntriesColumns& columns) {
    std::string line = ui::external_app::tpmseu::format::type(entry.type) + " ";
    std::string lid = ui::external_app::tpmseu::format::id(entry.id);
    lid.resize(columns.at(1).second, ' ');
    line += lid;

    if (entry.last_pressure.is_valid()) {
        line += "  " + ui::external_app::tpmseu::format::pressure(entry.last_pressure.value());
    } else {
        line += "     ";
    }

    if (entry.last_temperature.is_valid()) {
        line += "  " + ui::external_app::tpmseu::format::temperature(entry.last_temperature.value());
    } else {
        line += "     ";
    }

    if (entry.received_count > 999) {
        line += " +++";
    } else {
        line += " " + to_string_dec_uint(entry.received_count, 3);
    }

    if (entry.last_flags.is_valid()) {
        line += " " + ui::external_app::tpmseu::format::flags(entry.last_flags.value());
    } else {
        line += "   ";
    }

    line.resize(target_rect.width() / 8, ' ');
    painter.draw_string(target_rect.location(), style, line);
}

}  // namespace ui
