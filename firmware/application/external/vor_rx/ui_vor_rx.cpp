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

#include <algorithm>
#include <cstdint>

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

int32_t VorCdiIndicator::normalize_signed_degrees(int32_t degrees) {
    while (degrees <= -180) {
        degrees += 360;
    }
    while (degrees > 180) {
        degrees -= 360;
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
    const int32_t deviation = normalize_signed_degrees(static_cast<int32_t>(radial_deg_) - static_cast<int32_t>(course_deg_));
    const int32_t scaled = std::max<int32_t>(-10, std::min<int32_t>(10, deviation));
    // +/-10 deg full-scale maps to +/-2 ticks (2 * tick_spacing); the division
    // is exact for the clamped range so no rounding is needed.
    const int16_t needle_offset = static_cast<int16_t>((scaled * (2 * tick_spacing)) / 10);
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
                  &rssi,
                  &channel,
                  &audio,
                  &labels,
                  &text_status,
                  &text_band,
                  &text_next,
                  &field_course,
                  &text_course_unit,
                  &field_calibration,
                  &text_calib_unit,
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

    field_calibration.set_value(0);
    field_calibration.on_change = [this](int32_t) {
        refresh_radial();
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

    radial_filter_valid_ = false;
    flag_state_ = 0;

    receiver_model.set_hidden_offset(0);
    receiver_model.set_modulation(ReceiverModel::Mode::AMAudio);
    receiver_model.set_frequency_step(8333);
    receiver_model.set_sampling_rate(3072000);
    receiver_model.set_baseband_bandwidth(1750000);
    receiver_model.enable();

    // enable() applies the AM configuration, which forces the audio codec to
    // 12 kHz. The VOR baseband keeps its channel at 48 kHz (so the 9960 Hz
    // subcarrier stays representable), so re-assert 48 kHz audio afterwards.
    audio::set_rate(audio::Rate::Hz_48000);
    audio::output::start();

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
    text_status.set(running_ ? "Running" : "Idle");
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

    last_radial_deg_ = message.radial_deg;
    last_valid_ = message.valid;
    have_status_ = true;

    if (message.valid) {
        last_radial_deg_ = smooth_radial(message.radial_deg);
    } else {
        // Drop the filter history so a fresh lock doesn't drag from stale data.
        radial_filter_valid_ = false;
    }

    text_next.set(message.valid ? "Locked" : "Searching");
    refresh_radial();
    if (logger && logging_) {
        VorRxStatusDataMessage calibrated = message;
        calibrated.radial_deg = calibrated_radial(last_radial_deg_);
        calibrated.to_from = (flag_state_ == 2);
        logger->log_status(calibrated, field_course.value());
    }
}

uint16_t VorRxView::smooth_radial(uint16_t radial_deg) {
    // Wrapped exponential moving average. At ~10 updates/s blending 20% of each
    // new reading (alpha = 0.8) gives a ~0.5 s time constant while cutting the
    // jitter by ~3x. Stepping along the shortest arc in 1/64 deg fixed point
    // handles the 0/360 deg wrap without any trig (which would otherwise pull
    // newlib's float sin/cos/atan2 argument-reduction code into flash ROM).
    constexpr int32_t scale = 64;
    constexpr int32_t full_circle = 360 * scale;
    const int32_t target = static_cast<int32_t>(radial_deg) * scale;

    if (!radial_filter_valid_) {
        radial_smoothed_fp_ = target;
        radial_filter_valid_ = true;
    } else {
        int32_t diff = (target - radial_smoothed_fp_) % full_circle;
        if (diff < -full_circle / 2) {
            diff += full_circle;
        } else if (diff > full_circle / 2) {
            diff -= full_circle;
        }
        // 20% step toward the new reading (1 - alpha).
        radial_smoothed_fp_ += diff / 5;
        radial_smoothed_fp_ %= full_circle;
        if (radial_smoothed_fp_ < 0) {
            radial_smoothed_fp_ += full_circle;
        }
    }

    return static_cast<uint16_t>((radial_smoothed_fp_ + scale / 2) / scale) % 360;
}

uint16_t VorRxView::calibrated_radial(uint16_t radial_deg) const {
    int32_t value = (static_cast<int32_t>(radial_deg) + field_calibration.value()) % 360;
    if (value < 0) {
        value += 360;
    }
    return static_cast<uint16_t>(value);
}

void VorRxView::refresh_radial() {
    if (!have_status_) {
        return;
    }

    const uint16_t radial = calibrated_radial(last_radial_deg_);
    text_radial.set(to_string_dec_uint(radial, 3) + " deg");
    text_flag.set(to_from_label(radial));
    cdi_indicator.set_radial(radial);
    cdi_indicator.set_valid(last_valid_);
}

const char* VorRxView::to_from_label(uint16_t radial_deg) {
    if (!last_valid_) {
        flag_state_ = 0;
        return "--";
    }

    // TO/FROM is set by the selected OBS course relative to the received radial,
    // not by the radial alone. Take the difference wrapped to [-180, 180]: the
    // switch is at the 90 deg abeam point. |diff| < 90 means the selected course
    // leads away from the station (FROM); |diff| > 90 means toward it (TO).
    int32_t diff = (static_cast<int32_t>(radial_deg) - field_course.value() + 540) % 360 - 180;
    const int32_t adiff = (diff < 0) ? -diff : diff;

    // Hysteresis: hold the current flag within a +/-5 deg dead zone around the
    // 90 deg boundary so noise near abeam doesn't rapidly toggle TO/FROM.
    if (adiff < 85) {
        flag_state_ = 1;  // FROM
    } else if (adiff > 95) {
        flag_state_ = 2;  // TO
    }

    if (flag_state_ == 1) return "FROM";
    if (flag_state_ == 2) return "TO";
    return "--";  // abeam / ambiguous
}

void VorRxView::update_cdi() {
    cdi_indicator.set_course(field_course.value());
}

}  // namespace ui::external_app::vor_rx