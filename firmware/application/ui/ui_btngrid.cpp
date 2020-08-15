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

#include "ui_btngrid.hpp"
#include "rtc_time.hpp"

namespace ui {

/* BtnGridView **************************************************************/

BtnGridView::BtnGridView(
	Rect new_parent_rect,
	bool keep_highlight
) : keep_highlight { keep_highlight }
{
	set_parent_rect(new_parent_rect);

	set_focusable(true);

	signal_token_tick_second = rtc_time::signal_tick_second += [this]() {
		this->on_tick_second();
	};

	add_child(&arrow_more);
	arrow_more.set_focusable(false);
	arrow_more.set_foreground(Color::black());
}

BtnGridView::~BtnGridView() {
	rtc_time::signal_tick_second -= signal_token_tick_second;

	for (auto item : menu_item_views) {
		delete item;
	}
}

void BtnGridView::set_max_rows(int rows) {
	rows_ = rows;
}

int BtnGridView::rows() {
	return rows_;
}

void BtnGridView::set_parent_rect(const Rect new_parent_rect) {
	View::set_parent_rect(new_parent_rect);

	displayed_max = (parent_rect().size().height() / button_h);
	arrow_more.set_parent_rect( { 228, (Coord)(displayed_max * button_h), 8, 8 } );
	displayed_max *= rows_;

	// TODO: Clean this up :(
	if (menu_item_views.size()) {

		for (auto item : menu_item_views) {
			remove_child(item);
			delete item;
		}
		menu_item_views.clear();
	}

	button_w = 240 / rows_;
	for (size_t c = 0; c < displayed_max; c++) {
		auto item = new NewButton {  };
		menu_item_views.push_back(item);
		add_child(item);

		item->set_parent_rect({
			(int)(c % rows_) * button_w,
			(int)(c / rows_) * button_h,
			button_w, button_h
		});
	}

	update_items();
}

void BtnGridView::set_arrow_enabled(bool new_value) {
	if(new_value){
		add_child(&arrow_more);
	}
	else{
		remove_child(&arrow_more);
	}
};

void BtnGridView::on_tick_second() {
	if (more && blink)
		arrow_more.set_foreground(Color::white());
	else
		arrow_more.set_foreground(Color::black());

	blink = !blink;

	arrow_more.set_dirty();
}

void BtnGridView::clear() {
	menu_items.clear();
}

void BtnGridView::add_items(std::initializer_list<GridItem> new_items) {
	for (auto item : new_items) {
		menu_items.push_back(item);
	}
	update_items();
}

void BtnGridView::update_items() {
	size_t i = 0;

	if ((menu_items.size()) > (displayed_max + offset)) {
		more = true;
		blink = true;
	} else
		more = false;

	for (NewButton* item : menu_item_views) {
		if (i >= menu_items.size()) break;

		// Assign item data to NewButtons according to offset
		item->set_text(menu_items[i + offset].text);
		item->set_bitmap(menu_items[i + offset].bitmap);
		item->set_color(menu_items[i + offset].color);
		item->on_select = menu_items[i + offset].on_select;
		item->set_dirty();

		i++;
	}
}

NewButton* BtnGridView::item_view(size_t index) const {
	return menu_item_views[index];
}

bool BtnGridView::set_highlighted(int32_t new_value) {
	int32_t item_count = (int32_t)menu_items.size();

	if (new_value < 0)
		return false;

	if (new_value >= item_count)
		new_value = item_count - 1;

	if (((uint32_t)new_value > offset) && ((new_value - offset) >= displayed_max)) {
		// Shift BtnGridView up
		highlighted_item = new_value;
		offset = new_value - displayed_max + rows_;
		update_items();
	} else if ((uint32_t)new_value < offset) {
		// Shift BtnGridView down
		highlighted_item = new_value;
		offset = (new_value / rows_) * rows_;
		update_items();
	} else {
		// Just update highlight
		highlighted_item = new_value;
		if (visible())
		  item_view(highlighted_item - offset)->focus();
	}

	return true;
}

uint32_t BtnGridView::highlighted_index() {
	return highlighted_item;
}

void BtnGridView::on_focus() {
	item_view(highlighted_item - offset)->focus();
}

void BtnGridView::on_blur() {
#if 0
	if (!keep_highlight)
		item_view(highlighted_item - offset)->unhighlight();
#endif
}

bool BtnGridView::on_key(const KeyEvent key) {
	switch(key) {
	case KeyEvent::Up:
		return set_highlighted(highlighted_item - rows_);

	case KeyEvent::Down:
		return set_highlighted(highlighted_item + rows_);

	case KeyEvent::Right:
		return set_highlighted(highlighted_item + 1);

	case KeyEvent::Left:
		return set_highlighted(highlighted_item - 1);

	case KeyEvent::Select:
		if( menu_items[highlighted_item].on_select ) {
			menu_items[highlighted_item].on_select();
		}
		return true;

	default:
		return false;
	}
}

bool BtnGridView::on_encoder(const EncoderEvent event) {
	return set_highlighted(highlighted_item + event);
}

} /* namespace ui */
