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

#include "portapack.hpp"
#include "receiver_model.hpp"

#include "ui_setup.hpp"
#include "ui_debug.hpp"
#include "ui_receiver.hpp"

#include "m4_startup.hpp"

namespace ui {

/* SystemStatusView ******************************************************/

SystemStatusView::SystemStatusView() {
	add_children({ {
		&portapack,
		&sd_card_status_view,
	} });
	sd_card_status_view.set_parent_rect({ 28 * 8, 0 * 16,  2 * 8, 1 * 16 });
}

/* Navigation ************************************************************/

NavigationView::NavigationView()
{
}

View* NavigationView::push_view(std::unique_ptr<View> new_view) {
	free_view();

	const auto p = new_view.get();
	view_stack.emplace_back(std::move(new_view));

	update_view();

	return p;
}

void NavigationView::pop() {
	// Can't pop last item from stack.
	if( view_stack.size() > 1 ) {
		free_view();

		view_stack.pop_back();

		update_view();
	}
}

void NavigationView::free_view() {
	remove_child(view());
}

void NavigationView::update_view() {
	const auto new_view = view_stack.back().get();
	add_child(new_view);
	new_view->set_parent_rect({ {0, 0}, size() });
	focus();
	set_dirty();
}

Widget* NavigationView::view() const {
	return children_.empty() ? nullptr : children_[0];
}

void NavigationView::focus() {
	if( view() ) {
		view()->focus();
	}
}

/* SystemMenuView ********************************************************/

SystemMenuView::SystemMenuView(NavigationView& nav) {
	add_items<7>({ {
		{ "Receiver", [&nav](){ nav.push<ReceiverView>(nav, portapack::receiver_model); } },
		{ "Capture",  [&nav](){ nav.push<NotImplementedView>(nav); } },
		{ "Analyze",  [&nav](){ nav.push<NotImplementedView>(nav); } },
		{ "Setup",    [&nav](){ nav.push<SetupMenuView>(nav); } },
		{ "About",    [&nav](){ nav.push<AboutView>(nav); } },
		{ "Debug",    [&nav](){ nav.push<DebugMenuView>(nav); } },
		{ "HackRF",   [&nav](){ nav.push<HackRFFirmwareView>(nav); } },
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
	context_(context)
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
	navigation_view.push<SystemMenuView>(navigation_view);
}

Context& SystemView::context() const {
	return context_;
}

/* HackRFFirmwareView ****************************************************/

HackRFFirmwareView::HackRFFirmwareView(NavigationView& nav) {
	button_yes.on_select = [&nav](Button&){
		m4_request_shutdown();
	};

	button_no.on_select = [&nav](Button&){
		nav.pop();
	};

	add_children({ {
		&text_title,
		&text_description_1,
		&text_description_2,
		&text_description_3,
		&text_description_4,
		&button_yes,
		&button_no,
	} });
}

void HackRFFirmwareView::focus() {
	button_no.focus();
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
