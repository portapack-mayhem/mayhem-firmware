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

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_painter.hpp"
#include "ui_menu.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "ui_textentry.hpp"
#include "freqman.hpp"
#include "rtc_time.hpp"

namespace ui {

class FrequencySaveView : public View {
public:
	FrequencySaveView(NavigationView& nav, const rf::Frequency value);
	~FrequencySaveView();
	
	void focus() override;

	std::string title() const override { return "Save frequency"; };

private:
	NavigationView& nav_;
	freqman_error error { NO_ERROR };
	char desc_buffer[32] = { 0 };
	rtc::RTC datetime { };
	rf::Frequency value_ { };
	std::string str_timestamp { };
	
	void on_save_name(NavigationView& nav);
	void on_save_timestamp(NavigationView& nav);
	void on_tick_second();
	
	std::vector<freqman_entry> frequencies { };
	
	SignalToken signal_token_tick_second { };
	
	BigFrequency big_display {
		{ 4, 2 * 16, 28 * 8, 32 },
		0
	};
	
	Text text_save {
		{ 88, 120, 8 * 8, 16 },
		"Save as:",
	};
	Button button_save_name {
		{ 72, 144, 96, 32 },
		"Name (set)"
	};
	Button button_save_timestamp {
		{ 72, 184, 96, 32 },
		"Timestamp:"
	};
	Text text_timestamp {
		{ 76, 220, 11 * 8, 16 },
		"MM/DD HH:MM",
	};

	Button button_cancel {
		{ 72, 264, 96, 32 },
		"Cancel"
	};
};

class FrequencyLoadView : public View {
public:
	std::function<void(rf::Frequency)> on_changed { };
	
	FrequencyLoadView(NavigationView& nav);
	
	void focus() override;

	std::string title() const override { return "Load frequency"; };

private:
	NavigationView& nav_;
	freqman_error error { NO_ERROR };
	
	void on_frequency_select();
	void setup_list();
	
	std::vector<freqman_entry> frequencies { };
	
	MenuView menu_view { };
	
	Button button_cancel {
		{ 72, 264, 96, 32 },
		"Cancel"
	};	
};

class FreqManView : public View {
public:
	FreqManView(NavigationView& nav);
	~FreqManView();
	
	void focus() override;

	std::string title() const override { return "Freq. manager"; };

private:
	NavigationView& nav_;
	
	freqman_error error { NO_ERROR };
	
	void on_frequency_select();
	void on_edit_freq(rf::Frequency f);
	void on_edit_desc(NavigationView& nav);
	void on_delete();
	void setup_list();
	
	std::vector<freqman_entry> frequencies { };

	MenuView menu_view { true };

	Text text_edit {
		{ 16, 194, 5 * 8, 16 },
		"Edit:"
	};
	Button button_edit_freq {
		{ 16, 194 + 16, 104, 32 },
		"Frequency"
	};
	Button button_edit_desc {
		{ 16, 194 + 16 + 34, 104, 32 },
		"Description"
	};
	Button button_del {
		{ 160, 192, 72, 64 },
		"Delete"
	};
	
	Button button_exit {
		{ 160, 264, 72, 32 },
		"Exit"
	};
};

} /* namespace ui */
