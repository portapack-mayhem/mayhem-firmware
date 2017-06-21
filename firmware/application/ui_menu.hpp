/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __UI_MENU_H__
#define __UI_MENU_H__

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_painter.hpp"
#include "bitmap.hpp"
#include "signal.hpp"

#include <cstddef>
#include <string>
#include <functional>

namespace ui {

struct MenuItem {
	std::string text;
	ui::Color color;
	const Bitmap* bitmap;
	std::function<void(void)> on_select;

	// TODO: Prevent default-constructed MenuItems.
	// I managed to construct a menu with three extra, unspecified menu items
	// in the array that were default constructed...
};

class MenuItemView : public Widget {
public:
	MenuItemView(
		MenuItem item,
		bool keep_highlight
	) : item { item },
		keep_highlight_ { keep_highlight }
	{
	}

	void paint(Painter& painter) override;

	void select();
	void highlight();
	void unhighlight();

private:
	const MenuItem item;
	bool keep_highlight_ = false;
};

class MenuView : public View {
public:
	std::function<void(void)> on_left { };

	MenuView(Rect new_parent_rect = { 0, 0, 240, 304 }, bool keep_highlight = false);
	
	~MenuView();

	void add_item(MenuItem item);
	void add_items(std::initializer_list<MenuItem> items);
	void clear();
	
	MenuItemView* item_view(size_t index) const;

	size_t highlighted() const;
	bool set_highlighted(int32_t new_value);

	void on_focus() override;
	void on_blur() override;
	bool on_key(const KeyEvent event) override;
	bool on_encoder(const EncoderEvent event) override;

private:
	void update_items();
	void on_tick_second();
	
	bool keep_highlight_ { false };
	
	SignalToken signal_token_tick_second { };
	
	Image arrow_more {
		{ 228, 320 - 8, 8, 8 },
		&bitmap_more,
		Color::white(),
		Color::black()
	};

	const size_t item_height = 24;
	bool blink_ = false;
	bool more_ = false;
	size_t displayed_max_ { 0 };
	size_t highlighted_ { 0 };
	size_t offset_ { 0 };
};

} /* namespace ui */

#endif/*__UI_MENU_H__*/
