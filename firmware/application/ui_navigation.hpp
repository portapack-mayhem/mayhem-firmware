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
#include "ff.h"
#include "diskio.h"
#include "lfsr_random.hpp"
#include "sd_card.hpp"

#include <vector>
#include <utility>

using namespace sd_card;

namespace ui {

enum modal_t {
	INFO = 0,
	YESNO,
	YESCANCEL,
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
		{ 0 * 8, 0 * 16, 16, 16 },
		"",		//back_text_disabled,
	};

	Text title {
		{ 20, 0, 16 * 8, 1 * 16 },
		default_title,
	};
	
	ImageButton button_stealth {
		{ 152, 0, 2 * 8, 1 * 16 },
		&bitmap_stealth,
		Color::light_grey(),
		Color::black()
	};
	
	ImageButton button_textentry {
		{ 170, 0, 2 * 8, 1 * 16 },
		&bitmap_unistroke,
		Color::white(),
		Color::black()
	};

	ImageButton button_camera {
		{ 188, 0, 2 * 8, 1 * 16 },
		&bitmap_camera,
		Color::white(),
		Color::black()
	};

	ImageButton button_sleep {
		{ 206, 0, 2 * 8, 1 * 16 },
		&bitmap_sleep,
		Color::white(),
		Color::black()
	};

	SDCardStatusView sd_card_status_view {
		{ 28 * 8, 0 * 16,  2 * 8, 1 * 16 }
	};

	void on_stealth();
	void on_textentry();
	void on_camera();
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
	void paint(Painter&) override;
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

class WipeSDView : public View {
public:
	WipeSDView(NavigationView& nav);
	~WipeSDView();
	void focus() override;
	
	std::string title() const override { return "SD card wipe"; };	

private:
	NavigationView& nav_;
	
	bool confirmed = false;
	uint32_t blocks;
	static Thread* thread;
	
	static msg_t static_fn(void* arg) {
		auto obj = static_cast<WipeSDView*>(arg);
		obj->run();
		return 0;
	}
	
	void run() {
		uint32_t n, b;
		lfsr_word_t v = 1;
		const auto buffer = std::make_unique<std::array<uint8_t, 16384>>();

		for (b = 0; b < blocks; b++) {
			progress.set_value(b);
			
			lfsr_fill(v,
				reinterpret_cast<lfsr_word_t*>(buffer->data()),
				sizeof(*buffer.get()) / sizeof(lfsr_word_t));
			
			// 1MB
			for (n = 0; n < 64; n++) {
				if (disk_write(sd_card::fs.drv, buffer->data(), n + (b * 64), 16384 / 512) != RES_OK) nav_.pop();
			}
		}
		nav_.pop();
	}
	
	Text text_info {
		{ 10 * 8, 16 * 8, 10 * 8, 16 },
		"Working..."
	};
	
	ProgressBar progress {
		{ 2 * 8, 19 * 8, 26 * 8, 24 }
	};
	
	Button dummy {
		{ 240, 0, 0, 0 },
		""
	};
};

class PlayDeadView : public View {
public:
	PlayDeadView(NavigationView& nav);
	
	void focus() override;
	void paint(Painter& painter) override;

private:
	uint32_t sequence = 0;
	
	Text text_playdead1 {
		{ 6 * 8, 7 * 16, 14 * 8, 16 },
		"\x46irmwa" "re " "er\x72o\x72"
	};
	Text text_playdead2 {
		{ 6 * 8, 9 * 16, 16 * 8, 16 },
		""
	};
	Text text_playdead3 {
		{ 6 * 8, 12 * 16, 16 * 8, 16 },
		"Please reset"
	};
	
	Button button_seq_entry {
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
private:
	void hackrf_mode(NavigationView& nav);
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

	std::string title() const override { return title_; };

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
		{ 5 * 8, 13 * 16, 8 * 8, 48 },
		"YES",
	};

	Button button_no {
		{ 17 * 8, 13 * 16, 8 * 8, 48 },
		"NO",
	};
};

} /* namespace ui */

#endif/*__UI_NAVIGATION_H__*/
