/*
 * Copyright (C) 2016 Jared Boone, ShareBrained Technology, Inc.
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

#include "ui_touch_calibration.hpp"

#include "irq_controls.hpp"

#include "portapack_persistent_memory.hpp"
using namespace portapack;

namespace ui {

TouchCalibrationView::TouchCalibrationView(
	NavigationView& nav
) : nav { nav },
	calibration { touch::Calibration() }
{
	add_children({
		&image_calibrate_0,
		&image_calibrate_1,
		&image_calibrate_2,
		&image_verify_0,
		&image_verify_1,
		&image_verify_2,
		&label_calibrate,
		&label_verify,
		&label_success,
		&label_failure,
		&button_cancel,
		&button_ok,
	});

	button_cancel.on_select = [this](Button&){ this->on_cancel(); };
	button_ok.on_select = [this](Button&){ this->on_ok(); };

	set_phase(Phase::Calibrate0);
}

void TouchCalibrationView::focus() {
	button_cancel.focus();
}

void TouchCalibrationView::update_target() {
	const auto phase_calibrate = (phase == Phase::Calibrate0) || (phase == Phase::Calibrate1) || (phase == Phase::Calibrate2);
	const auto phase_verify =    (phase == Phase::Verify0)    || (phase == Phase::Verify1)    || (phase == Phase::Verify2);

	image_calibrate_0.hidden(phase != Phase::Calibrate0);
	image_calibrate_1.hidden(phase != Phase::Calibrate1);
	image_calibrate_2.hidden(phase != Phase::Calibrate2);

	image_verify_0.hidden(phase != Phase::Verify0);
	image_verify_1.hidden(phase != Phase::Verify1);
	image_verify_2.hidden(phase != Phase::Verify2);

	label_calibrate.hidden(!phase_calibrate);
	label_verify.hidden(!phase_verify);
	label_success.hidden(phase != Phase::Success);
	label_failure.hidden(phase != Phase::Failure);

	button_ok.hidden((phase != Phase::Success) && (phase != Phase::Failure));

	/* TODO: Such a hack to get around a poor repaint implementation! This "technique"
	 * occurs in other places...
	 */
	set_dirty();
}

void TouchCalibrationView::set_phase(const Phase value) {
	if( value != phase ) {
		phase = value;
		update_target();
	}
}

uint32_t TouchCalibrationView::distance_squared(const Point& touch_point, const Image& target) {
	const auto target_point = target.screen_rect().center();
	const int32_t dx = target_point.x() - touch_point.x();
	const int32_t dy = target_point.y() - touch_point.y();
	const uint32_t dx2 = dx * dx;
	const uint32_t dy2 = dy * dy;
	return dx2 + dy2;
}

void TouchCalibrationView::touch_complete() {
	auto next_phase = static_cast<Phase>(toUType(phase) + 1);

	switch(phase) {
	case Phase::Calibrate0:
	case Phase::Verify0:
		digitizer_points[0] = average;
		break;

	case Phase::Calibrate1:
	case Phase::Verify1:
		digitizer_points[1] = average;
		break;

	case Phase::Calibrate2:
	case Phase::Verify2:
		digitizer_points[2] = average;
		break;

	default:
		break;
	}

	if( phase == Phase::Calibrate2 ) {
		const std::array<Point, 3> display_points { {
			image_calibrate_0.screen_rect().center(),
			image_calibrate_1.screen_rect().center(),
			image_calibrate_2.screen_rect().center(),
		} };

		calibration = { digitizer_points, display_points };
	}

	if( phase == Phase::Verify2 ) {
		const auto calibrated_0 = calibration.translate(digitizer_points[0]);
		const auto d_sq_0 = distance_squared(calibrated_0, image_verify_0);

		const auto calibrated_1 = calibration.translate(digitizer_points[1]);
		const auto d_sq_1 = distance_squared(calibrated_1, image_verify_1);

		const auto calibrated_2 = calibration.translate(digitizer_points[2]);
		const auto d_sq_2 = distance_squared(calibrated_2, image_verify_2);

		if( (d_sq_0 < verify_d_sq_max) && (d_sq_1 < verify_d_sq_max) && (d_sq_2 < verify_d_sq_max) ) {
			next_phase = Phase::Success;
		} else {
			next_phase = Phase::Failure;
		}
	}

	set_phase(next_phase);
}

void TouchCalibrationView::on_ok() {
	if( phase == Phase::Success ) {
		persistent_memory::set_touch_calibration(calibration);
		nav.pop();
	}
	if( phase == Phase::Failure ) {
		set_phase(Phase::Calibrate0);
	}
}

void TouchCalibrationView::on_cancel() {
	nav.pop();
}

void TouchCalibrationView::on_frame_sync() {
	switch(phase) {
	case Phase::Calibrate0:
	case Phase::Calibrate1:
	case Phase::Calibrate2:
	case Phase::Verify0:
	case Phase::Verify1:
	case Phase::Verify2:
		break;

	default:
		return;
	}

	const auto frame = get_touch_frame();
	const auto metrics = touch::calculate_metrics(frame);
	const auto x = metrics.x * 1024;
	const auto y = metrics.y * 1024;

	if( metrics.r < 640.0f ) {
		if( samples_count > 0 ) {
			average.x = ((average.x * 7) + x) / 8;
			average.y = ((average.y * 7) + y) / 8;
		} else {
			average.x = x;
			average.y = y;
		}

		samples_count += 1;
	} else {
		if( samples_count >= samples_limit ) {
			touch_complete();
		}
		samples_count = 0;
	}
}

} /* namespace ui */
