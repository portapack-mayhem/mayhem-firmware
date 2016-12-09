/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __UI_NAVIGATION_H__
#define __UI_NAVIGATION_H__

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_focus.hpp"
#include "ui_menu.hpp"

#include "ui_rssi.hpp"
#include "ui_channel.hpp"
#include "ui_audio.hpp"
#include "ui_sd_card_status_view.hpp"

#include "bitmap.hpp"

#include <vector>
#include <utility>

namespace ui {

enum modal_t {
	INFO = 0,
	YESNO,
	ABORT
};

class SystemStatusView : public View {
public:
	std::function<void(void)> on_back;

	SystemStatusView();

	void set_back_enabled(bool new_value);
	void set_title(const std::string new_value);

private:
	static constexpr auto default_title = "PortaPack|Havoc";
	//static constexpr auto back_text_enabled = " < ";
	//static constexpr auto back_text_disabled = " * ";

	Button button_back {
		{ 0 * 8, 0 * 16, 20, 16 },
		"",		//back_text_disabled,
	};

	Text title {
		{ 3 * 8, 0, 16 * 8, 1 * 16 },
		default_title,
	};
	
	ImageButton button_textentry {
		{ 164, 0, 2 * 8, 1 * 16 },
		&bitmap_unistroke,
		Color::white(),
		Color::black()
	};

	ImageButton button_camera {
		{ 184, 0, 2 * 8, 1 * 16 },
		&bitmap_camera,
		Color::white(),
		Color::black()
	};

	ImageButton button_sleep {
		{ 204, 0, 2 * 8, 1 * 16 },
		&bitmap_sleep,
		Color::white(),
		Color::black()
	};

	SDCardStatusView sd_card_status_view {
		{ 28 * 8, 0 * 16,  2 * 8, 1 * 16 }
	};

	void on_camera();
	void on_textentry();
};

class NavigationView : public View {
public:
	std::function<void(const View&)> on_view_changed;

	NavigationView() { }

	NavigationView(const NavigationView&) = delete;
	NavigationView(NavigationView&&) = delete;

	bool is_top() const;

	template<class T, class... Args>
	T* push(Args&&... args) {
		return reinterpret_cast<T*>(push_view(std::unique_ptr<View>(new T(*this, std::forward<Args>(args)...))));
	}
	
	void push(View* v);

	void pop();
	void pop_modal();

	void display_modal(const std::string& title, const std::string& message);
	void display_modal(const std::string& title, const std::string& message, const modal_t type, const std::function<void(bool)> on_choice);

	void focus() override;

private:
	std::vector<std::unique_ptr<View>> view_stack;
	Widget* modal_view { nullptr };

	Widget* view() const;

	void free_view();
	void update_view();
	View* push_view(std::unique_ptr<View> new_view);
};

class TranspondersMenuView : public MenuView {
public:
	TranspondersMenuView(NavigationView& nav);
};

class BMPView : public View {
public:
	BMPView(NavigationView& nav);
	void paint(Painter& painter) override;
	void focus() override;

private:
	Text text_info {
		{ 5 * 8, 284, 20 * 8, 16 },
		"shrbrnd-sig-ftk 2016"
	};
	
	Button button_done {
		{ 240, 0, 1, 1 },
		""
	};
};

class PlayDeadView : public View {
public:
	PlayDeadView(NavigationView& nav, bool booting);
	void focus() override;

private:
	bool _booting;
	uint32_t sequence = 0;
	Text text_playdead1 {
		{ 6 * 8, 7 * 16, 14 * 8, 16 },
		"Firmware error"
	};
	Text text_playdead2 {
		{ 6 * 8, 9 * 16, 16 * 8, 16 },
		"0x1400_0000 : 2C"
	};
	
	Button button_done {
		{ 240, 0, 1, 1 },
		""
	};
};

class ReceiverMenuView : public MenuView {
public:
	ReceiverMenuView(NavigationView& nav);
	std::string title() const override { return "Receivers"; };
};

class TransmitterCodedMenuView : public MenuView {
public:
	TransmitterCodedMenuView(NavigationView& nav);
	std::string title() const override { return "Coded TX"; };
};

class TransmitterAudioMenuView : public MenuView {
public:
	TransmitterAudioMenuView(NavigationView& nav);
	std::string title() const override { return "Audio TX"; };
};

class UtilitiesView : public MenuView {
public:
	UtilitiesView(NavigationView& nav);
	std::string title() const override { return "Utilities"; };	
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

class HackRFFirmwareView : public View {
public:
	HackRFFirmwareView(NavigationView& nav);

	void focus() override;

private:
	Text text_title {
		{ 76, 4 * 16, 19 * 8, 16 },
		"HackRF Mode"
	};

	Text text_description_1 {
		{  4, 7 * 16, 19 * 8, 16 },
		"Run stock HackRF firmware and"
	};

	Text text_description_2 {
		{ 12, 8 * 16, 19 * 8, 16 },
		"disable PortaPack until the"
	};

	Text text_description_3 {
		{  4, 9 * 16, 19 * 8, 16 },
		"unit is reset or disconnected"
	};

	Text text_description_4 {
		{ 76, 10 * 16, 19 * 8, 16 },
		"from power?"
	};

	Button button_yes {
		{ 4 * 8, 13 * 16, 8 * 8, 24 },
		"Yes",
	};

	Button button_no {
		{ 18 * 8, 13 * 16, 8 * 8, 24 },
		"No",
	};
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

class ModalMessageView : public View {
public:
	ModalMessageView(
		NavigationView& nav,
		const std::string& title,
		const std::string& message,
		const modal_t type,
		const std::function<void(bool)> on_choice
	);
	void paint(Painter& painter) override;

	void focus() override;

	// std::string title() const override { return title_; };

private:
	const std::string title_;
	const modal_t type_;
	const std::function<void(bool)> on_choice_;
	
	Text text_message { };

	Button button_ok {
		{ 10 * 8, 13 * 16, 10 * 8, 24 },
		"OK",
	};
	
	Button button_yes {
		{ 40, 13 * 16, 64, 24 },
		"YES",
	};
	Button button_no {
		{ 152, 13 * 16, 64, 24 },
		"NO",
	};
};

} /* namespace ui */

#endif/*__UI_NAVIGATION_H__*/
