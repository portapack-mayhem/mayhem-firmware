/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
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

/*#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_painter.hpp"
#include "ui_menu.hpp"
#include "ui_navigation.hpp"
#include "clock_manager.hpp"
#include "message.hpp"
#include "rf_path.hpp"
#include "volume.hpp"
#include "receiver_model.hpp"*/

#include "receiver_model.hpp"

#include "spectrum_color_lut.hpp"

#include "ui_receiver.hpp"
#include "ui_spectrum.hpp"
#include "ui_record_view.hpp"

#include "ui_font_fixed_8x16.hpp"

namespace ui {

#define CC_SLICE_WIDTH 10000000

class CloseCallView : public View {
public:
	CloseCallView(NavigationView& nav);
	~CloseCallView();
	
	void on_show() override;
	void on_hide() override;
	void focus() override;
	std::string title() const override { return "Close call"; };

private:
	Coord last_pos = 0;
	ChannelSpectrumFIFO* fifo { nullptr };
	uint8_t detect_counter = 0, release_counter = 0;
	uint8_t wait = 2;
	uint32_t mean = 0;
	rf::Frequency slice_start;
	rf::Frequency slice_frequency;
	uint8_t slices_max;
	uint8_t slices_counter;
	uint16_t last_channel;
	rf::Frequency scan_span;
	uint8_t slicemax_db[32];	// Todo: Cap max slices !
	uint8_t slicemax_idx[32];
	bool ignore = true;
	bool slicing;
	bool locked = false;
	void on_channel_spectrum(const ChannelSpectrum& spectrum);
	void on_range_changed();
	void do_detection();
	
	FrequencyField field_frequency_min {
		{ 1 * 8, 0 * 16 },
	};
	FrequencyField field_frequency_max {
		{ 12 * 8, 0 * 16 },
	};
	
	Text text_slice {
		{ 23 * 8, 0 * 16, 2 * 8, 16 },
		"--"
	};
	
	Text text_infos {
		{ 1 * 8, 1 * 16, 18 * 8, 16 },
		"..."
	};
	
	Button button_exit {
		{ 92, 264, 56, 32 },
		"Exit"
	};
};

} /* namespace ui */
