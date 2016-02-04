/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __UI_AFSKRX_H__
#define __UI_AFSKRX_H__

#include "ui.hpp"
#include "ui_font_fixed_8x16.hpp"
#include "ui_navigation.hpp"
#include "ui_painter.hpp"
#include "ui_widget.hpp"
#include "ui_console.hpp"

#include "utility.hpp"

#include "receiver_model.hpp"

#include <cstddef>
#include <cstdint>
#include <algorithm>
#include <functional>

namespace ui {

class AFSKRXView : public View {
public:
	AFSKRXView(NavigationView& nav);
	~AFSKRXView();

	void focus() override;

	void on_show() override;
	void on_hide() override;

private:
	std::unique_ptr<Widget> widget_content;

	Button button_done {
		{ 4 * 8, 0 * 16, 3 * 8, 16 },
		" < "
	};

	Text text_rx {
		{ 1 * 8, 2 * 16, 28 * 8, 16 },
		"Ready..."
	};

	void on_tuning_frequency_changed(rf::Frequency f);
	void on_edit_frequency();

	void on_data_afsk(const AFSKDataMessage& message);
};

} /* namespace ui */

#endif/*__UI_RECEIVER_H__*/
