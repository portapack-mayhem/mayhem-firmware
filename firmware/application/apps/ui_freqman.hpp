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

namespace ui {
	
class FreqManBaseView : public View {
public:
	FreqManBaseView(
		NavigationView& nav
	);

	void focus() override;
	
protected:
	using option_t = std::pair<std::string, int32_t>;
	using options_t = std::vector<option_t>;
	
	NavigationView& nav_;
	freqman_error error_ { NO_ERROR };
	options_t categories { };
	std::function<void(int32_t category_id)> on_change_category { nullptr };
	std::function<void(void)> on_select_frequency { nullptr };
	std::function<void(bool)> on_refresh_widgets { nullptr };
	std::vector<std::string> file_list { };
	int32_t current_category_id { 0 };
	
	void populate_categories();
	void change_category(int32_t category_id);
	void refresh_list();
	
	freqman_db database { };
	
	Labels label_category {
		{ { 0, 4 }, "Category:", Color::light_grey() }
	};
	
	OptionsField options_category {
		{ 9 * 8, 4 },
		12,
		{ }
	};
	
	MenuView menu_view {
		{ 0, 3 * 8, 240, 23 * 8 },
		true
	};
	Text text_empty {
		{ 7 * 8, 12 * 8, 16 * 8, 16 },
		"Empty category !",
	};
	
	Button button_exit {
		{ 20 * 8, 34 * 8, 10 * 8, 4 * 8 },
		"Exit"
	};
};

class FrequencySaveView : public FreqManBaseView {
public:
	FrequencySaveView(NavigationView& nav, const rf::Frequency value);

	std::string title() const override { return "Save frequency"; };
	
private:
	std::string desc_buffer { };
	rf::Frequency value_ { };
	
	void on_save_name();
	void on_save_timestamp();
	void save_current_file();
	
	BigFrequency big_display {
		{ 4, 2 * 16, 28 * 8, 32 },
		0
	};
	
	Labels labels {
		{ { 2 * 8, 14 * 8 }, "Save as:", Color::white() }
	};
	
	Button button_save_name {
		{ 2 * 8, 17 * 8, 14 * 8, 48 },
		"Name (set)"
	};
	Button button_save_timestamp {
		{ 2 * 8, 25 * 8, 14 * 8, 48 },
		"Timestamp:"
	};
	LiveDateTime live_timestamp {
		{ 17 * 8, 27 * 8, 11 * 8, 16 }
	};
};

class FrequencyLoadView : public FreqManBaseView {
public:
	std::function<void(rf::Frequency)> on_frequency_loaded { };
	std::function<void(rf::Frequency, rf::Frequency)> on_range_loaded { };
	
	FrequencyLoadView(NavigationView& nav);

	std::string title() const override { return "Load frequency"; };
	
private:
	void refresh_widgets(const bool v);
};

class FrequencyManagerView : public FreqManBaseView {
public:
	FrequencyManagerView(NavigationView& nav);
	~FrequencyManagerView();

	std::string title() const override { return "Freq. manager"; };
	
private:
	std::string desc_buffer { };
	
	void refresh_widgets(const bool v);
	void on_edit_freq(rf::Frequency f);
	void on_edit_desc(NavigationView& nav);
	void on_new_category(NavigationView& nav);
	void on_delete();

	Labels labels {
		{ { 4 * 8 + 4, 26 * 8 }, "Edit:", Color::light_grey() }
	};
	
	Button button_new_category {
		{ 23 * 8, 2, 7 * 8, 20 },
		"New"
	};

	Button button_edit_freq {
		{ 0 * 8, 29 * 8, 14 * 8, 32 },
		"Frequency"
	};
	Button button_edit_desc {
		{ 0 * 8, 34 * 8, 14 * 8, 32 },
		"Description"
	};
	
	Button button_delete {
		{ 18 * 8, 27 * 8, 12 * 8, 32 },
		"Delete"
	};
};

} /* namespace ui */
