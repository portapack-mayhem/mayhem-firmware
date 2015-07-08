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

#ifndef __UI_NAVIGATION_H__
#define __UI_NAVIGATION_H__

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_focus.hpp"
#include "ui_menu.hpp"

#include "ui_rssi.hpp"
#include "ui_channel.hpp"
#include "ui_audio.hpp"

#include <vector>

namespace ui {

class SystemStatusView : public View {
public:
	SystemStatusView();

	void on_show() override;
	void on_hide() override;

private:
	Text portapack {
		{ 0, 0, 9 * 8, 1 * 16 },
		"PortaPack",
	};
	/*
	Text text_app_fifo_n {
		{ 12 * 8, 0, 3 * 8, 1 * 16 },
		"---",
	};

	Text text_baseband_fifo_n {
		{ 16 * 8, 0, 3 * 8, 1 * 16 },
		"---",
	};
	*/
	Text text_ticks {
		{ 9 * 8, 0, 7 * 8, 1 * 16 },
		"",
	};

	RSSI rssi {
		{ 19 * 8, 0, 11 * 8, 4 },
	};

	Channel channel {
		{ 19 * 8, 5, 11 * 8, 4 },
	};

	Audio audio {
		{ 19 * 8, 10, 11 * 8, 4 },
	};

	void on_statistics_update(const BasebandStatistics& statistics);
};

class NavigationView : public View {
public:
	NavigationView();

	NavigationView(const NavigationView&) = delete;
	NavigationView(NavigationView&&) = delete;

	void push(View* new_view);
	void pop();

	void focus() override;

private:
	std::vector<View*> view_stack;

	Widget* view() const;
	void set_view(Widget* const new_view);
};

class SystemMenuView : public MenuView {
public:
	SystemMenuView(NavigationView& nav);
};

class SystemView : public View {
public:
	SystemView(
		Context& context,
		const Rect parent_rect
	);

	Context& context() const override;

private:
	SystemStatusView status_view;
	NavigationView navigation_view;
	Context& context_;
};

class NotImplementedView : public View {
public:
	NotImplementedView(NavigationView& nav);

	void focus() override;

private:
	Text text_title {
		{ 5 * 8, 7 * 16, 19 * 8, 16 },
		"Not Yet Implemented"
	};

	Button button_done {
		{ 10 * 8, 13 * 16, 10 * 8, 24 },
		"Bummer",
	};
};

} /* namespace ui */

#endif/*__UI_NAVIGATION_H__*/
