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

#include "tpms_app.hpp"

#include "baseband_api.hpp"
#include "audio.hpp"
#include "portapack.hpp"
using namespace portapack;

#include "string_format.hpp"

#include "utility.hpp"
#include "file_path.hpp"

namespace pmem = portapack::persistent_memory;

namespace ui::external_app::tpmsrx {

namespace format {

std::string type(tpms::Reading::Type type) {
    return to_string_dec_uint(toUType(type), 2);
}

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
        default:
            return "Unknown";
    }
}

std::string id(tpms::TransponderID id) {
    return to_string_hex(id.value(), 8);
}

std::string pressure(Pressure pressure) {
    return to_string_dec_int(pressure_unit == PRESSURE_UNIT_PSI ? pressure.psi() : pressure_unit == PRESSURE_UNIT_BAR ? pressure.bar()
                                                                                                                      : pressure.kilopascal(),
                             3);
}

std::string temperature(Temperature temperature) {
    return to_string_dec_int(temp_unit == TEMP_UNIT_CELSIUS ? temperature.celsius() : temperature.fahrenheit(), 3);
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

} /* namespace format */

void TPMSLogger::on_packet(const tpms::Packet& packet, const uint32_t target_frequency) {
    const auto hex_formatted = packet.symbols_formatted();

    // TODO: function doesn't take uint64_t, so when >= 1<<32, weirdness will ensue!
    const auto target_frequency_str = to_string_dec_uint(target_frequency, 10);

    std::string entry = target_frequency_str + " " + ui::external_app::tpmsrx::format::signal_type(packet.signal_type()) + " " + hex_formatted.data + "/" + hex_formatted.errors;
    log_file.write_entry(packet.received_at(), entry);
}

const TPMSRecentEntry::Key TPMSRecentEntry::invalid_key = {tpms::Reading::Type::None, 0};

void TPMSRecentEntry::update(const tpms::Reading& reading) {
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

TPMSAppView::TPMSAppView(NavigationView& nav)
    : nav_{nav} {
    // baseband::run_image(portapack::spi_flash::image_tag_tpms);
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

    // Add on_select handler for entries
    recent_entries_view.on_select = [this](const TPMSRecentEntry& entry) {
        on_show_detail(entry);
    };

    logger = std::make_unique<TPMSLogger>();
    if (logger) {
        logger->append(logs_dir / u"TPMS.TXT");
    }

    if (pmem::beep_on_packets()) {
        audio::set_rate(audio::Rate::Hz_24000);
        audio::output::start();
    }
}

TPMSAppView::~TPMSAppView() {
    audio::output::stop();
    receiver_model.disable();
    baseband::shutdown();
}

void TPMSAppView::focus() {
    options_band.focus();
}

void TPMSAppView::update_view() {
    recent_entries_view.set_parent_rect(view_normal_rect);
}

void TPMSAppView::set_parent_rect(const Rect new_parent_rect) {
    View::set_parent_rect(new_parent_rect);

    view_normal_rect = {0, header_height, new_parent_rect.width(), new_parent_rect.height() - header_height};

    update_view();
}

void TPMSAppView::on_packet(const tpms::Packet& packet) {
    if (logger) {
        logger->on_packet(packet, receiver_model.target_frequency());
    }

    const auto reading_opt = packet.reading();
    if (reading_opt.is_valid()) {
        const auto reading = reading_opt.value();
        auto& entry = ::on_packet(recent, TPMSRecentEntry::Key{reading.type(), reading.id()});
        entry.update(reading);
        entry.signal_type = packet.signal_type();
        recent_entries_view.set_dirty();
    }

    if (pmem::beep_on_packets()) {
        baseband::request_audio_beep(1000, 24000, 60);
    }
}

void TPMSAppView::on_show_list() {
    recent_entries_view.hidden(false);
    recent_entries_view.focus();
}

void TPMSAppView::on_show_detail(const TPMSRecentEntry& entry) {
    nav_.push<TPMSRecentEntryDetailView>(entry);
}

// Detail view implementation
TPMSRecentEntryDetailView::TPMSRecentEntryDetailView(NavigationView& nav, const TPMSRecentEntry& entry)
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

    // Display entry data
    text_type.set(format::type_name(entry.type));
    text_id.set(to_string_hex(entry.id.value(), 8));

    if (entry.last_pressure.is_valid()) {
        std::string pressure_str = format::pressure(entry.last_pressure.value());
        std::string unit_str = format::pressure_unit == PRESSURE_UNIT_PSI ? " PSI" : format::pressure_unit == PRESSURE_UNIT_BAR ? " BAR"
                                                                                                                                : " kPa";
        text_pressure.set(pressure_str + unit_str);
    } else {
        text_pressure.set("---");
    }

    if (entry.last_temperature.is_valid()) {
        text_temperature.set(to_string_dec_int(entry.last_temperature.value().celsius(), 3) + " C");
    } else {
        text_temperature.set("---");
    }

    if (entry.last_flags.is_valid()) {
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

void TPMSRecentEntryDetailView::focus() {
    button_done.focus();
}

void TPMSRecentEntryDetailView::set_entry(const TPMSRecentEntry& entry) {
    entry_ = entry;
}

void TPMSRecentEntryDetailView::on_save() {
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

bool TPMSRecentEntryDetailView::save_file(const std::filesystem::path& path) {
    File f;
    auto error = f.create(path);
    if (error.is_valid())
        return false;

    // Save in format compatible with TX app
    std::string content = "Type=" + to_string_dec_uint(toUType(entry_.type), 1) + "\n";
    content += "ID=" + to_string_hex(entry_.id.value(), 8) + "\n";

    if (entry_.last_pressure.is_valid()) {
        content += "Pressure=" + to_string_dec_int(entry_.last_pressure.value().kilopascal(), 1) + "\n";
    } else {
        content += "Pressure=240\n";  // Default value
    }

    if (entry_.last_temperature.is_valid()) {
        content += "Temperature=" + to_string_dec_int(entry_.last_temperature.value().celsius(), 1) + "\n";
    } else {
        content += "Temperature=25\n";  // Default value
    }

    if (entry_.last_flags.is_valid()) {
        content += "Flags=" + to_string_hex(entry_.last_flags.value(), 2) + "\n";
    } else {
        content += "Flags=00\n";
    }

    // Save signal type for proper retransmission
    content += "SignalType=" + to_string_dec_uint(toUType(entry_.signal_type), 1) + "\n";

    f.write(content.c_str(), content.length());
    return true;
}

}  // namespace ui::external_app::tpmsrx

namespace ui {

template <>
void RecentEntriesTable<ui::external_app::tpmsrx::TPMSRecentEntries>::draw(
    const Entry& entry,
    const Rect& target_rect,
    Painter& painter,
    const Style& style,
    RecentEntriesColumns& columns) {
    std::string line = ui::external_app::tpmsrx::format::type(entry.type) + " ";
    std::string lid = ui::external_app::tpmsrx::format::id(entry.id);
    lid.resize(columns.at(1).second, ' ');
    line += lid;

    if (entry.last_pressure.is_valid()) {
        line += "  " + ui::external_app::tpmsrx::format::pressure(entry.last_pressure.value());
    } else {
        line +=
            "  "
            "   ";
    }

    if (entry.last_temperature.is_valid()) {
        line += "  " + ui::external_app::tpmsrx::format::temperature(entry.last_temperature.value());
    } else {
        line +=
            "  "
            "   ";
    }

    if (entry.received_count > 999) {
        line += " +++";
    } else {
        line += " " + to_string_dec_uint(entry.received_count, 3);
    }

    if (entry.last_flags.is_valid()) {
        line += " " + ui::external_app::tpmsrx::format::flags(entry.last_flags.value());
    } else {
        line +=
            " "
            "  ";
    }

    line.resize(target_rect.width() / 8, ' ');
    painter.draw_string(target_rect.location(), style, line);
}

}  // namespace ui
