/*
 * Copyright (C) 2026 PortaPack Mayhem
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
 * along with this program; if not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include "ui_vor_rx.hpp"

#include "audio.hpp"
#include "string_format.hpp"

#include <cmath>

using namespace portapack;
using namespace ui;

namespace ui::external_app::vor_rx {

void VorLogger::write_header() {
    log_file.write_raw("Time;Course;Radial;Deviation;Quality;Flag;");
}

void VorLogger::log_status(const VorRxStatusDataMessage& message, uint16_t course_deg) {
    const int16_t deviation = static_cast<int16_t>((static_cast<int32_t>(message.radial_deg) - static_cast<int32_t>(course_deg) + 540) % 360 - 180);
    std::string row = ";";
    row += to_string_dec_uint(course_deg);
    row += ";" + to_string_dec_uint(message.radial_deg);
    row += ";" + std::to_string(deviation);
    row += ";" + to_string_dec_uint(message.quality);
    row += ";" + std::string(message.valid ? (message.to_from ? "TO" : "FROM") : "--");
    log_file.write_entry(rtc_time::now(), row);
}

VorCdiIndicator::VorCdiIndicator(Point position)
    : Widget{{position, {screen_width, 32}}} {
}

void VorCdiIndicator::set_course(uint16_t course_deg) {
    if (course_deg_ != course_deg) {
        course_deg_ = course_deg;
        set_dirty();
    }
}

void VorCdiIndicator::set_radial(uint16_t radial_deg) {
    if (radial_deg_ != radial_deg) {
        radial_deg_ = radial_deg;
        set_dirty();
    }
}

void VorCdiIndicator::set_valid(bool valid) {
    if (valid_ != valid) {
        valid_ = valid;
        set_dirty();
    }
}

float VorCdiIndicator::normalize_signed_degrees(float degrees) {
    while (degrees <= -180.0f) {
        degrees += 360.0f;
    }
    while (degrees > 180.0f) {
        degrees -= 360.0f;
    }
    return degrees;
}

void VorCdiIndicator::paint(Painter& painter) {
    const auto r = screen_rect();
    const auto theme = Theme::getInstance();
    const auto line_color = valid_ ? theme->fg_light->foreground : theme->bg_medium->foreground;
    const auto needle_color = valid_ ? theme->fg_red->foreground : theme->bg_medium->foreground;

    // Clear background so old needle positions don't linger.
    painter.fill_rectangle(r, theme->bg_darkest->background);

    const auto center_x = static_cast<Coord>(r.left() + r.width() / 2);
    const auto center_y = static_cast<Coord>(r.top() + r.height() / 2);

    // Horizontal scale line.
    painter.draw_hline({static_cast<Coord>(r.left() + 8), center_y}, r.width() - 16, line_color);

    // Evenly spaced tick marks, longer at center.
    constexpr int32_t tick_spacing = 40;
    for (int32_t tick = -2; tick <= 2; ++tick) {
        const auto x = static_cast<Coord>(center_x + tick * tick_spacing);
        const auto len = (tick == 0) ? 14 : 8;
        painter.draw_vline({x, static_cast<Coord>(center_y - len / 2)}, len, line_color);
    }

    // Deviation needle, clamped to +/-10 degrees full-scale.
    const float deviation = normalize_signed_degrees(static_cast<float>(radial_deg_) - static_cast<float>(course_deg_));
    const float scaled = std::max(-10.0f, std::min(10.0f, deviation));
    const int16_t needle_offset = static_cast<int16_t>(std::lround((scaled / 10.0f) * (2 * tick_spacing)));
    const auto needle_x = static_cast<Coord>(center_x + needle_offset);
    painter.draw_vline({needle_x, static_cast<Coord>(r.top() + 4)}, r.height() - 8, needle_color);
}

VorRxView::VorRxView(NavigationView& nav)
    : nav_{nav} {
    add_children({&field_rf_amp,
                  &field_lna,
                  &field_vga,
                  &field_volume,
                  &field_frequency,
                  &labels,
                  &text_status,
                  &text_band,
                  &text_next,
                  &field_course,
                  &text_course,
                  &text_radial,
                  &text_flag,
                  &text_cdi_title,
                  &cdi_indicator,
                  &check_log,
                  &button_start_stop});

    field_frequency.set_step(8333);
    if (field_frequency.value() == 0) {
        field_frequency.set_value(110'000'000);
    }

    field_course.set_value(0);
    field_course.on_change = [this](int32_t) {
        update_cdi();
    };

    logger = std::make_unique<VorLogger>();
    check_log.set_value(logging_);
    check_log.on_select = [this](Checkbox&, bool v) {
        logging_ = v;
        update_logging();
    };
    update_logging();

    button_start_stop.on_select = [this](Button&) {
        if (running_) {
            stop_receiver();
        } else {
            start_receiver();
        }
    };

    start_receiver();
}

VorRxView::~VorRxView() {
    stop_receiver();
}

void VorRxView::focus() {
    field_frequency.focus();
}

void VorRxView::start_receiver() {
    baseband::run_prepared_image(portapack::memory::map::m4_code.base());
    baseband::set_vor_config(true);

    receiver_model.set_hidden_offset(0);
    receiver_model.set_modulation(ReceiverModel::Mode::AMAudio);
    receiver_model.set_frequency_step(8333);
    receiver_model.set_sampling_rate(3072000);
    receiver_model.set_baseband_bandwidth(1750000);
    audio::set_rate(audio::Rate::Hz_48000);
    audio::output::start();
    receiver_model.enable();

    running_ = true;
    update_status();
}

void VorRxView::stop_receiver() {
    if (!running_) {
        return;
    }

    running_ = false;
    baseband::set_vor_config(false);
    receiver_model.disable();
    baseband::shutdown();
    audio::output::stop();
    update_status();
}

void VorRxView::update_status() {
    text_status.set(running_ ? "Audio on" : "Off");
    button_start_stop.set_text(running_ ? "Stop" : "Start");
}

void VorRxView::update_logging() {
    if (logger && logging_) {
        logger->append(logs_dir / (std::string("VOR_") + to_string_timestamp(rtc_time::now()) + ".CSV"));
        logger->write_header();
    }
}

void VorRxView::on_vor_status(const VorRxStatusDataMessage& message) {
    if (!running_) {
        return;
    }

    text_radial.set("Radial: " + to_string_dec_uint(message.radial_deg) + " deg");
    text_next.set(message.valid ? "Decoder locked" : "Decoder pending");
    text_flag.set(message.valid ? (message.to_from ? "TO" : "FROM") : "--");
    cdi_indicator.set_radial(message.radial_deg);
    cdi_indicator.set_valid(message.valid);
    if (logger && logging_) {
        logger->log_status(message, field_course.value());
    }
}

void VorRxView::update_cdi() {
    text_course.set(to_string_dec_uint(field_course.value(), 3) + " deg");
    cdi_indicator.set_course(field_course.value());
}

}  // namespace ui::external_app::vor_rx