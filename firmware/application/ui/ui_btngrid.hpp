/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
 * Copyright (C) 2019 Elia Yehuda (z4ziggy)
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

#ifndef __UI_BTNGRID_H__
#define __UI_BTNGRID_H__

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_painter.hpp"
#include "bitmap.hpp"
#include "signal.hpp"

#include <cstddef>
#include <string>
#include <functional>

namespace ui {

struct GridItem {
	std::string text;
	ui::Color color;
	const Bitmap* bitmap;
	std::function<void(void)> on_select;

	// TODO: Prevent default-constructed GridItems.
};

class BtnGridView : public View {
public:
	BtnGridView(Rect new_parent_rect = { 0, 0, 240, 304 }, bool keep_highlight = false);

	~BtnGridView();

	void add_items(std::initializer_list<GridItem> new_items);
	void set_max_rows(int rows);
	int rows();
	void clear();

	NewButton* item_view(size_t index) const;

	bool set_highlighted(int32_t new_value);
	uint32_t highlighted_index();

	void set_parent_rect(const Rect new_parent_rect) override;
	void on_focus() override;
	void on_blur() override;
	bool on_key(const KeyEvent event) override;
	bool on_encoder(const EncoderEvent event) override;

private:
	int rows_ { 3 };
	void update_items();
	void on_tick_second();

	bool keep_highlight { false };

	SignalToken signal_token_tick_second { };
	std::vector<GridItem> menu_items { };
	std::vector<NewButton*> menu_item_views { };

	Image arrow_more {
		{ 228, 320 - 8, 8, 8 },
		&bitmap_more,
		Color::white(),
		Color::black()
	};

	int button_w = 240 / rows_;
	static constexpr int button_h = 48;
	bool blink = false;
	bool more = false;
	size_t displayed_max { 0 };
	size_t highlighted_item { 0 };
	size_t offset { 0 };
};

} /* namespace ui */

#endif/*__UI_BTNGRID_H__*/
