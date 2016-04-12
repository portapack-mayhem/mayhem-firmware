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
#include "ui_receiver.hpp"

#include "audio_thread.hpp"

#include <string>
#include <memory>

namespace ui {

static constexpr uint8_t bitmap_record_data[] = {
	0x00, 0x00,
	0x00, 0x00,
	0xc0, 0x03,
	0xf0, 0x0f,
	0xf8, 0x1f,
	0xf8, 0x1f,
	0xfc, 0x3f,
	0xfc, 0x3f,
	0xfc, 0x3f,
	0xfc, 0x3f,
	0xf8, 0x1f,
	0xf8, 0x1f,
	0xf0, 0x0f,
	0xc0, 0x03,
	0x00, 0x00,
	0x00, 0x00,
};

static constexpr Bitmap bitmap_record {
	{ 16, 16 }, bitmap_record_data
};

static constexpr uint8_t bitmap_stop_data[] = {
	0x00, 0x00,
	0x00, 0x00,
	0xfc, 0x3f,
	0xfc, 0x3f,
	0xfc, 0x3f,
	0xfc, 0x3f,
	0xfc, 0x3f,
	0xfc, 0x3f,
	0xfc, 0x3f,
	0xfc, 0x3f,
	0xfc, 0x3f,
	0xfc, 0x3f,
	0xfc, 0x3f,
	0xfc, 0x3f,
	0x00, 0x00,
	0x00, 0x00,
};

static constexpr Bitmap bitmap_stop {
	{ 16, 16 }, bitmap_stop_data
};

class CaptureAppView : public View {
public:
	CaptureAppView(NavigationView& nav);
	~CaptureAppView();

	void focus() override;

	std::string title() const override { return "Capture"; };

private:
	static constexpr uint32_t sampling_rate = 2457600;
	static constexpr uint32_t baseband_bandwidth = 1750000;

	std::unique_ptr<AudioThread> capture_thread;

	void on_start_stop();

	void on_tuning_frequency_changed(rf::Frequency f);
	void on_lna_changed(int32_t v_db);
	void on_vga_changed(int32_t v_db);

	ImageButton button_start_stop {
		{ 0 * 8, 0, 2 * 8, 1 * 16 },
		&bitmap_record,
		Color::red(),
		Color::black()
	};

	RSSI rssi {
		{ 21 * 8, 0, 6 * 8, 4 },
	};

	Channel channel {
		{ 21 * 8, 5, 6 * 8, 4 },
	};

	FrequencyField field_frequency {
		{ 5 * 8, 0 * 16 },
	};

	LNAGainField field_lna {
		{ 15 * 8, 0 * 16 }
	};

	VGAGainField field_vga {
		{ 18 * 8, 0 * 16 }
	};
};

} /* namespace ui */

#endif/*__CAPTURE_APP_HPP__*/
