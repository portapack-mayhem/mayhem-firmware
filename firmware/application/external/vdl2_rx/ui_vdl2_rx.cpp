/*
 * Copyright (C) 2026 Xmoo26
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

#include "ui_vdl2_rx.hpp"

#include "baseband_api.hpp"
#include "portapack.hpp"
#include "rtc_time.hpp"
#include "string_format.hpp"
#include "file_path.hpp"
#include "database.hpp"

using namespace portapack;

namespace ui::external_app::vdl2_rx {

namespace {

std::string icao_hex(const uint32_t icao) {
    return to_string_hex(icao, 6);
}

// ACARS labels of the OOOI reports (with and without fuel, return to gate, ...):
// 0 out, 1 off, 2 on, 3 in, -1 none.
int oooi_event(const std::string& label) {
    if (label.size() != 2 || label[0] != 'Q')
        return -1;
    switch (label[1]) {
        case 'A':
        case 'E':
        case 'G':
        case 'H':
        case 'P':
        case 'T':
            return 0;
        case 'B':
        case 'F':
        case 'Q':
            return 1;
        case 'C':
        case 'K':
        case 'R':
            return 2;
        case 'D':
        case 'L':
        case 'S':
            return 3;
        default:
            return -1;
    }
}

std::string oooi_time(const VDL2Entry& e, const size_t i) {
    return e.has_oooi[i] ? to_string_datetime(e.oooi[i], HM) : "--:--";
}

}  // namespace

void VDL2Entry::update(const Frame& frame) {
    frames++;
    last_seen = rtc_time::now();
    if (frame.kind == Frame::Kind::Link) {
        link++;
        return;
    }
    if (frame.kind == Frame::Kind::Data) {
        data++;
        return;
    }
    acars++;
    if (!frame.reg.empty())
        reg = frame.reg;
    if (!frame.flight.empty())
        flight = frame.flight;
    label = frame.label;
    const int event = frame.from_aircraft() ? oooi_event(frame.label) : -1;
    if (event >= 0) {
        oooi[event] = last_seen;
        has_oooi[event] = true;
    }
    if (frame.text.empty())
        return;
    for (size_t i = TEXTS - 1; i > 0; i--)
        texts[i] = texts[i - 1];
    texts[0] = (frame.from_aircraft() ? "> " : "< ") + frame.label + " " + frame.text;
}

VDL2SettingsView::VDL2SettingsView(NavigationView& nav, VDL2Settings& settings)
    : settings_{settings} {
    add_children({&check_log, &check_small_font, &button_save});
    check_log.set_value(settings_.enable_log);
    check_small_font.set_value(settings_.small_font);
    button_save.on_select = [this, &nav](Button&) {
        settings_.enable_log = check_log.value();
        settings_.small_font = check_small_font.value();
        nav.pop();
    };
}

VDL2DetailView::VDL2DetailView() {
    add_children({&button_done});
    button_done.on_select = [this](Button&) {
        if (on_close)
            on_close();
    };
}

void VDL2DetailView::set_entry(const VDL2Entry& entry) {
    entry_ = entry;
    set_dirty();
}

void VDL2DetailView::focus() {
    button_done.focus();
}

void VDL2DetailView::paint(Painter& painter) {
    View::paint(painter);
    const auto s = style();
    const auto r = screen_rect();
    const size_t columns = r.width() / s.font.char_width();
    const Dim height = s.font.line_height();
    Coord y = r.top();
    auto line = [&](const std::string& text) {
        painter.draw_string({r.left(), y}, s, text.substr(0, columns));
        y += height;
    };

    line((entry_.ground() ? "Ground " : "ICAO   ") + icao_hex(entry_.address()));
    if (!entry_.ground()) {
        line("Reg    " + entry_.reg);
        line("Flight " + entry_.flight);
        line("Type   " + entry_.model);
        line("Oper   " + entry_.operator_);
    }
    if (!entry_.ground() && (entry_.has_oooi[0] || entry_.has_oooi[1] || entry_.has_oooi[2] || entry_.has_oooi[3])) {
        line("Out    " + oooi_time(entry_, 0) + "  Off " + oooi_time(entry_, 1));
        line("On     " + oooi_time(entry_, 2) + "  In  " + oooi_time(entry_, 3));
    }
    line("Frames " + to_string_dec_uint(entry_.frames) + ": link " + to_string_dec_uint(entry_.link) +
         " data " + to_string_dec_uint(entry_.data) + " ACARS " + to_string_dec_uint(entry_.acars));
    line("Last   " + to_string_datetime(entry_.last_seen, HMS) + (entry_.label.empty() ? "" : "  label " + entry_.label));

    // The last messages, wrapped, as far as there is room above the button.
    const Coord bottom = r.top() + button_done.parent_rect().top() - height;
    for (const auto& text : entry_.texts) {
        for (size_t pos = 0; pos < text.size() && y < bottom; pos += columns)
            line(text.substr(pos, columns));
        if (!text.empty())
            y += height / 2;
    }
}

VDL2RxView::VDL2RxView(NavigationView& nav)
    : nav_{nav} {
    baseband::run_image(portapack::spi_flash::image_tag_vdl2_rx);

    add_children({&options_channel, &label_mhz, &field_rf_amp, &field_lna, &field_vga, &rssi, &channel,
                  &text_frames, &button_config, &recent_view, &detail_view});
    detail_view.hidden(true);

    // The channel is the tuning: off a VDL2 channel there is nothing to decode.
    options_channel.on_change = [](size_t, OptionsField::value_t f) {
        receiver_model.set_target_frequency(f);
    };
    options_channel.set_by_value(receiver_model.target_frequency());

    button_config.on_select = [this](Button&) {
        nav_.push<VDL2SettingsView>(vdl2_settings_);
        nav_.set_on_pop([this]() { refresh_ui(); });
    };
    log_file_.append(logs_dir / u"VDL2.TXT");

    recent_view.on_select = [this](const VDL2Entry& entry) {
        show_detail(entry);
    };
    detail_view.on_close = [this]() {
        show_list();
    };

    refresh_ui();
    receiver_model.enable();
}

VDL2RxView::~VDL2RxView() {
    receiver_model.disable();
    baseband::shutdown();
}

void VDL2RxView::set_parent_rect(const Rect new_parent_rect) {
    View::set_parent_rect(new_parent_rect);
    const Rect content{0, header_height, new_parent_rect.width(), new_parent_rect.height() - header_height};
    recent_view.set_parent_rect(content);
    detail_view.set_parent_rect(content);
}

void VDL2RxView::focus() {
    options_channel.focus();
}

// Apply the Config screen's choices: the font of the list and detail view, the status row.
void VDL2RxView::refresh_ui() {
    const auto style = vdl2_settings_.small_font ? Theme::getInstance()->bg_darkest_small
                                                 : Theme::getInstance()->bg_darkest;
    recent_view.set_style(style);
    detail_view.set_style(style);
    text_frames.set("Frames " + to_string_dec_uint(frames_) + (vdl2_settings_.enable_log ? "  LOG" : ""));
    set_dirty();
}

void VDL2RxView::show_list() {
    detail_view.hidden(true);
    recent_view.hidden(false);
    recent_view.focus();
}

void VDL2RxView::show_detail(const VDL2Entry& entry) {
    recent_view.hidden(true);
    detail_view.hidden(false);
    detail_view.set_entry(entry);
    detail_view.focus();
}

void VDL2RxView::update_entry(const VDL2Entry::Key key, const Frame& frame) {
    auto& entry = ::on_packet(recent_, key);
    if (entry.frames == 0 && !entry.ground()) {
        // Most frames carry no registration (link management, ATN); the database knows it.
        database db{};
        database::AircraftDBRecord record;
        if (db.retrieve_aircraft_record(&record, icao_hex(entry.address())) == DATABASE_RECORD_FOUND) {
            entry.reg = record.aircraft_registration;
            entry.model = record.aircraft_model;
            entry.operator_ = record.aircraft_operator;
        }
    }
    entry.update(frame);
    recent_view.set_dirty();
    if (!detail_view.hidden() && detail_view.entry().key() == key)
        detail_view.set_entry(entry);
}

void VDL2RxView::on_frame(const VDL2FrameMessage& message) {
    const size_t length = (message.length < VDL2FrameMessage::MAX_LENGTH) ? message.length : VDL2FrameMessage::MAX_LENGTH;
    Frame frame{};
    if (!parse_frame(message.data, length, frame))
        return;

    frames_++;
    text_frames.set("Frames " + to_string_dec_uint(frames_) + (vdl2_settings_.enable_log ? "  LOG" : ""));

    if (frame.aircraft())
        update_entry(frame.aircraft(), frame);
    if (frame.ground_station())
        update_entry(frame.ground_station() | VDL2Entry::GROUND_FLAG, frame);

    if (vdl2_settings_.enable_log && frame.kind == Frame::Kind::ACARS) {
        log_file_.write_entry(
            icao_hex(frame.src) + ">" + icao_hex(frame.dst) + " " + frame.reg + " " + frame.flight +
            " " + frame.label + " " + frame.msg_num + " " + frame.text +
            " (" + to_string_dec_int(message.offset_hz) + " Hz)");
    }
}

}  // namespace ui::external_app::vdl2_rx

namespace ui {

template <>
void RecentEntriesTable<external_app::vdl2_rx::VDL2Entries>::draw(
    const Entry& entry,
    const Rect& target_rect,
    Painter& painter,
    const Style& style,
    RecentEntriesColumns& columns) {
    const std::string fields[] = {
        to_string_hex(entry.address(), 6),
        entry.ground() ? "GROUND" : entry.reg,
        entry.flight,
        to_string_dec_uint(entry.frames),
    };
    // Each field where the header put its column: the header steps (width + 1) * 8 pixels
    // whatever the font, so with the small font the fields must not just follow each other.
    painter.fill_rectangle(target_rect, style.background);
    Coord x = target_rect.left();
    for (size_t i = 0; i < columns.size() && i < 4; i++) {
        const size_t width = columns.at(i).second;
        const Coord end = width ? x + (Coord)(width * 8) : target_rect.right();
        const size_t chars = (end - x) / style.font.char_width();
        painter.draw_string({x, target_rect.top()}, style, fields[i].substr(0, chars));
        x += (Coord)(width * 8 + 8);
    }
}

}  // namespace ui
