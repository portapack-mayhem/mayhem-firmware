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

#include "capture_app.hpp"

#include "baseband_api.hpp"

#include "utility.hpp"

namespace ui {

CaptureAppView::CaptureAppView(NavigationView&) {
	add_children({ {
		&button_start,
		&button_stop,
	} });

	button_start.on_select = [this](Button&){ this->on_start(); };
	button_stop.on_select = [this](Button&){ this->on_stop(); };

	radio::enable({
		tuning_frequency(),
		sampling_rate,
		baseband_bandwidth,
		rf::Direction::Receive,
		false, 16, 16,
		1,
	});

	baseband::start({
		.mode = 7,
		.sampling_rate = sampling_rate,
		.decimation_factor = 1,
	});
}

CaptureAppView::~CaptureAppView() {
	baseband::stop();
	radio::disable();
}

void CaptureAppView::focus() {
	button_start.focus();
}

void CaptureAppView::on_start() {
	if( !capture_thread ) {
		capture_thread = std::make_unique<AudioThread>("baseband.c16");
	}
}

void CaptureAppView::on_stop() {
	capture_thread.reset();
}

uint32_t CaptureAppView::target_frequency() const {
	return initial_target_frequency;
}

uint32_t CaptureAppView::tuning_frequency() const {
	return target_frequency() - (sampling_rate / 4);
}

} /* namespace ui */
