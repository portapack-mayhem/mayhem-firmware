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

#include "ui_navigation.hpp"

#include "receiver_model.hpp"

#include "ui_setup.hpp"
#include "ui_debug.hpp"
#include "ui_receiver.hpp"

extern ReceiverModel receiver_model;

namespace ui {

/* SystemStatusView ******************************************************/

SystemStatusView::SystemStatusView() {
	add_children({ {
		&portapack,
		//&text_app_fifo_n,
		//&text_baseband_fifo_n,
		&rssi,
		&channel,
		&audio,
	} });
}

/* Navigation ************************************************************/

NavigationView::NavigationView()
{
}

void NavigationView::push(View* new_view) {
	// TODO: Trap nullptr?
	// TODO: Trap push of object already on stack?
	view_stack.push_back(new_view);
	set_view(new_view);
}

void NavigationView::pop() {
	// Can't pop last item from stack.
	if( view_stack.size() > 1 ) {
		const auto old_view = view_stack.back();
		view_stack.pop_back();
		const auto new_view = view_stack.back();
		set_view(new_view);
		delete old_view;
	}
}

Widget* NavigationView::view() const {
	return children_.empty() ? nullptr : children_[0];
}

void NavigationView::set_view(Widget* const new_view) {
	const auto old_view = view();
	if( old_view ) {
		remove_child(old_view);
	}

	// TODO: Allow new_view == nullptr?!
	if( new_view ) {
		add_child(new_view);
		new_view->set_parent_rect({ {0, 0}, size() });
		focus();
	}

	set_dirty();
}

void NavigationView::focus() {
	if( view() ) {
		view()->focus();
	}
}

/* SystemMenuView ********************************************************/

SystemMenuView::SystemMenuView(NavigationView& nav) {
	add_items<6>({ {
		{ "Receiver", [&nav](){ nav.push(new ReceiverView       { nav, receiver_model }); } },
		{ "Capture",  [&nav](){ nav.push(new NotImplementedView { nav }); } },
		{ "Analyze",  [&nav](){ nav.push(new NotImplementedView { nav }); } },
		{ "Setup",    [&nav](){ nav.push(new SetupMenuView      { nav }); } },
		{ "About",    [&nav](){ nav.push(new AboutView          { nav }); } },
		{ "Debug",    [&nav](){ nav.push(new DebugMenuView      { nav }); } },
	} });
}

/* SystemView ************************************************************/

static constexpr ui::Style style_default {
	.font = ui::font::fixed_8x16,
	.background = ui::Color::black(),
	.foreground = ui::Color::white(),
};

SystemView::SystemView(
	Context& context,
	const Rect parent_rect
) : View { parent_rect },
	context_ { context }
{
	style_ = &style_default;

	constexpr ui::Dim status_view_height = 16;

	add_child(&status_view);
	status_view.set_parent_rect({
		{ 0, 0 },
		{ parent_rect.width(), status_view_height }
	});

	add_child(&navigation_view);
	navigation_view.set_parent_rect({
		{ 0, status_view_height },
		{ parent_rect.width(), static_cast<ui::Dim>(parent_rect.height() - status_view_height) }
	});

	// Initial view.
	// TODO: Restore from non-volatile memory?
	navigation_view.push(new SystemMenuView { navigation_view });
}

Context& SystemView::context() const {
	return context_;
}

/* NotImplementedView ****************************************************/

NotImplementedView::NotImplementedView(NavigationView& nav) {
	button_done.on_select = [&nav](Button&){
		nav.pop();
	};

	add_children({ {
		&text_title,
		&button_done,
	} });
}

void NotImplementedView::focus() {
	button_done.focus();
}

} /* namespace ui */
