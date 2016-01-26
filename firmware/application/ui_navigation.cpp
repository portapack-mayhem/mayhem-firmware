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

#include "ais_app.hpp"
#include "ert_app.hpp"
#include "tpms_app.hpp"

#include "m4_startup.hpp"

namespace ui {

/* SystemStatusView ******************************************************/

SystemStatusView::SystemStatusView() {
	add_children({ {
		&button_back,
		&title,
		&sd_card_status_view,
	} });
	sd_card_status_view.set_parent_rect({ 28 * 8, 0 * 16,  2 * 8, 1 * 16 });

	button_back.on_select = [this](Button&){
		if( this->on_back ) {
			this->on_back();
		}
	};
}

void SystemStatusView::set_back_visible(bool new_value) {
	button_back.hidden(!new_value);
}

void SystemStatusView::set_title(const std::string new_value) {
	if( new_value.empty() ) {
		title.set(default_title);
	} else {
		title.set(new_value);
	}
}

/* Navigation ************************************************************/

bool NavigationView::is_top() const {
	return view_stack.size() == 1;
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

	if( on_view_changed ) {
		on_view_changed(*new_view);
	}
}

Widget* NavigationView::view() const {
	return children_.empty() ? nullptr : children_[0];
}

void NavigationView::focus() {
	if( view() ) {
		view()->focus();
	}
}

/* TransceiversMenuView **************************************************/

TranspondersMenuView::TranspondersMenuView(NavigationView& nav) {
	add_items<3>({ {
		{ "AIS:  Boats",          [&nav](){ nav.push<AISAppView>(); } },
		{ "ERT:  Utility Meters", [&nav](){ nav.push<ERTAppView>(); } },
		{ "TPMS: Cars",           [&nav](){ nav.push<TPMSAppView>(); } },
	} });
}

/* ReceiverMenuView ******************************************************/

ReceiverMenuView::ReceiverMenuView(NavigationView& nav) {
	add_items<2>({ {
		{ "Audio",        [&nav](){ nav.push<ReceiverView>(); } },
		{ "Transponders", [&nav](){ nav.push<TranspondersMenuView>(); } },
	} });
}

/* SystemMenuView ********************************************************/

SystemMenuView::SystemMenuView(NavigationView& nav) {
	add_items<7>({ {
		{ "Receiver", [&nav](){ nav.push<ReceiverMenuView>(); } },
		{ "Capture",  [&nav](){ nav.push<NotImplementedView>(); } },
		{ "Analyze",  [&nav](){ nav.push<NotImplementedView>(); } },
		{ "Setup",    [&nav](){ nav.push<SetupMenuView>(); } },
		{ "About",    [&nav](){ nav.push<AboutView>(); } },
		{ "Debug",    [&nav](){ nav.push<DebugMenuView>(); } },
		{ "HackRF",   [&nav](){ nav.push<HackRFFirmwareView>(); } },
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
	status_view.on_back = [this]() {
		this->navigation_view.pop();
	};

	add_child(&navigation_view);
	navigation_view.set_parent_rect({
		{ 0, status_view_height },
		{ parent_rect.width(), static_cast<ui::Dim>(parent_rect.height() - status_view_height) }
	});
	navigation_view.on_view_changed = [this](const View& new_view) {
		this->status_view.set_back_visible(!this->navigation_view.is_top());
		this->status_view.set_title(new_view.title());
	};

	// Initial view.
	// TODO: Restore from non-volatile memory?
	navigation_view.push<SystemMenuView>();
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
