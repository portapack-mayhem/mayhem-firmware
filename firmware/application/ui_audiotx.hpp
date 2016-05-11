/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_painter.hpp"
#include "ui_menu.hpp"
#include "ui_navigation.hpp"
#include "ui_font_fixed_8x16.hpp"
#include "clock_manager.hpp"
#include "message.hpp"
#include "rf_path.hpp"
#include "max2837.hpp"
#include "volume.hpp"
#include "ui_receiver.hpp"
#include "transmitter_model.hpp"

namespace ui {

class AudioTXView : public View {
public:
	AudioTXView(NavigationView& nav);
	~AudioTXView();

	void focus() override;

private:
	void on_tuning_frequency_changed(rf::Frequency f);
	
	FrequencyField field_frequency {
		{ 5 * 8, 3 * 16 },
	};
	
	Text text_title {
		{ 76, 64, 88, 16 },
		"Audio TX"
	};
	
	Button button_transmit {
		{ 72, 130, 96, 32 },
		"Transmit"
	};
	
	Button button_exit {
		{ 72, 270, 96, 32 },
		"Exit"
	};
};

} /* namespace ui */
