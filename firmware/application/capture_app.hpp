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

#ifndef __CAPTURE_APP_HPP__
#define __CAPTURE_APP_HPP__

#include "ui_widget.hpp"
#include "ui_navigation.hpp"

#include "audio_thread.hpp"

#include <string>
#include <memory>

namespace ui {

class CaptureAppView : public View {
public:
	CaptureAppView(NavigationView& nav);
	~CaptureAppView();

	void focus() override;

	std::string title() const override { return "Capture"; };

private:
	static constexpr uint32_t initial_target_frequency = 91500000;
	static constexpr uint32_t sampling_rate = 2457600;
	static constexpr uint32_t baseband_bandwidth = 1750000;

	std::unique_ptr<AudioThread> capture_thread;

	uint32_t target_frequency() const;
	uint32_t tuning_frequency() const;

	void on_start();
	void on_stop();

	Button button_start {
		{ 16, 17 * 16, 96, 24 },
		"Start"
	};

	Button button_stop {
		{ 240 - 96 - 16, 17 * 16, 96, 24 },
		"Stop"
	};
};

} /* namespace ui */

#endif/*__CAPTURE_APP_HPP__*/
