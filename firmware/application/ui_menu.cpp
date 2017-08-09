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

#include "ui_menu.hpp"

namespace ui {

/* MenuItemView **********************************************************/

void MenuItemView::select() {
	if( item.on_select ) {
		item.on_select();
	}
}

void MenuItemView::highlight() {
	set_highlighted(true);
	set_dirty();
}

void MenuItemView::unhighlight() {
	set_highlighted(false);
	set_dirty();
}

void MenuItemView::paint(Painter& painter) {
	const auto r = screen_rect();

	const auto paint_style = (highlighted() && parent()->has_focus()) ? style().invert() : style();

	const auto font_height = paint_style.font.line_height();

	painter.fill_rectangle(
		r,
		paint_style.background
	);

	painter.draw_string(
		{ r.left() + 8, r.top() + (r.height() - font_height) / 2 },
		paint_style,
		item.text
	);
}

/* MenuView **************************************************************/

MenuView::~MenuView() {
	/* TODO: Double-check this */
	for(auto child : children_) {
		delete child;
	}
}

void MenuView::add_item(MenuItem item) {
	add_child(new MenuItemView { item });
}

void MenuView::add_items(std::initializer_list<MenuItem> items) {
	for(auto item : items) {
		add_item(item);
	}
}

void MenuView::set_parent_rect(const Rect new_parent_rect) {
	View::set_parent_rect(new_parent_rect);

	constexpr int item_height = 24;
	int i = 0;
	for(auto child : children_) {
		child->set_parent_rect({ 0, i * item_height, size().width(), item_height });
		i++;
	}
}

MenuItemView* MenuView::item_view(size_t index) const {
	/* TODO: Terrible cast! Take it as a sign I must be doing something
	 * shamefully wrong here, right?
	 */
	return static_cast<MenuItemView*>(children_[index]);
}

size_t MenuView::highlighted() const {
	return highlighted_;
}

bool MenuView::set_highlighted(const size_t new_value) {
	if( new_value >= children_.size() ) {
		return false;
	}

	item_view(highlighted())->unhighlight();
	highlighted_ = new_value;
	item_view(highlighted())->highlight();

	return true;
}

void MenuView::on_focus() {
	item_view(highlighted())->highlight();
}

void MenuView::on_blur() {
	item_view(highlighted())->unhighlight();
}

bool MenuView::on_key(const KeyEvent key) {
	switch(key) {
	case KeyEvent::Up:
		return set_highlighted(highlighted() - 1);

	case KeyEvent::Down:
		return set_highlighted(highlighted() + 1);

	case KeyEvent::Select:
	case KeyEvent::Right:
		item_view(highlighted())->select();
		return true;

	case KeyEvent::Left:
		if( on_left ) {
			on_left();
		}
		return true;

	default:
		return false;
	}
}

bool MenuView::on_encoder(const EncoderEvent event) {
	set_highlighted(highlighted() + event);
	return true;
}

/* TODO: This could be handled by default behavior, if the UI stack were to
 * transmit consumable events from the top of the hit-stack down, and each
 * MenuItem could respond to a touch and update its parent MenuView.
 */
/*
bool MenuView::on_touch(const TouchEvent event) {
	size_t i = 0;
	for(const auto child : children_) {
		if( child->screen_rect().contains(event.point) ) {
			return set_highlighted(i);
		}
		i++;
	}

	return false;
}
*/
} /* namespace ui */
