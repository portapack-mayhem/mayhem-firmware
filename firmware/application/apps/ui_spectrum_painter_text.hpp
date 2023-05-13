/*
 * Copyright (C) 2023 Bernd Herzog
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

#pragma once

#include "ui.hpp"
#include "ui_widget.hpp"

#include "ui_navigation.hpp"
#include "ui_tabview.hpp"
#include "ui_transmitter.hpp"
#include "baseband_api.hpp"

#include "portapack.hpp"
#include "message.hpp"

namespace ui {

class SpectrumInputTextView : public View {
public:
	SpectrumInputTextView(NavigationView& nav);
	~SpectrumInputTextView();

	void focus() override;
	void paint(Painter&) override;
	
private:
	// Button button_start {
	// 	{ 0 * 8, 11 * 8, 11 * 8, 28 },
	// 	"s2"
	// };
	Labels labels {
		{ { 10 * 8, 80 + 1 * 16 }, "Text input is not", Color::light_grey() },
		{ { 1 * 8, 80 + 2 * 16 }, "yet implemented", Color::light_grey() },
	};

};

}
