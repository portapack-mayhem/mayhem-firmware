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

#ifndef __UI_DFU_MENU_H__
#define __UI_DFU_MENU_H__

#include <cstdint>

#include "ui_widget.hpp"
#include "event_m0.hpp"

namespace ui {
class NavigationView;

class DfuMenu : public View {
public:
	DfuMenu(NavigationView& nav);
	~DfuMenu() = default;

	void focus() override;
	void paint(Painter& painter) override;

	std::string title() const override { return "DFU Menu"; };	

private:
	NavigationView& nav_;
	
	Text text_info {
		{ 10 * 8, 16 * 8, 10 * 8, 16 },
		"Working..."
	};
	
	ProgressBar progress {
		{ 2 * 8, 19 * 8, 26 * 8, 24 }
	};
	
	Button dummy {
		{ 240, 0, 0, 0 },
		""
	};
};

} /* namespace ui */

#endif/*__UI_DFU_MENU_H__*/
