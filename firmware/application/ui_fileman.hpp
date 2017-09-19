/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2017 Furrtek
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
#include "file.hpp"
#include "ui_navigation.hpp"
#include "ui_textentry.hpp"
#include "rtc_time.hpp"

namespace ui {

struct fileman_entry {
	std::filesystem::path entry_path { };
	uint32_t size { };
	bool is_directory { };
};

class FileManBaseView : public View {
public:
	FileManBaseView(
		NavigationView& nav
	);

	void focus() override;
	
	void load_directory_contents(const std::filesystem::path& dir_path);
	std::filesystem::path get_absolute_path();
	
	std::string title() const override { return "File manager"; };
	
protected:
	NavigationView& nav_;
	
	const std::string suffix[5] = { "B", "kB", "MB", "GB", "??" };
	
	bool error_ { false };
	std::function<void(void)> on_select_entry { nullptr };
	std::function<void(bool)> on_refresh_widgets { nullptr };
	std::vector<fileman_entry> entry_list { };
	std::filesystem::path current_path { u"" };
	
	void change_category(int32_t category_id);
	void refresh_list();
	
	Labels labels {
		{ { 0, 0 }, "Current:", Color::light_grey() }
	};
	Text text_current {
		{ 8 * 8, 0 * 8, 22 * 8, 16 },
		"",
	};
	
	MenuView menu_view {
		{ 0, 2 * 8, 240, 26 * 8 },
		true
	};
	Text text_empty {
		{ 7 * 8, 12 * 8, 16 * 8, 16 },
		"Empty directory !",
	};
	
	Button button_exit {
		{ 20 * 8, 34 * 8, 10 * 8, 4 * 8 },
		"Exit"
	};
};

class FileSaveView : public FileManBaseView {
public:
	FileSaveView(NavigationView& nav);
	~FileSaveView();

private:
	std::string filename_buffer { };
	rtc::RTC datetime { };
	std::string str_timestamp { };
	
	void on_save_name();
	void on_tick_second();
	
	SignalToken signal_token_tick_second { };
	
	Text text_save {
		{ 4 * 8, 15 * 8, 8 * 8, 16 },
		"Save as:",
	};
	Button button_save_name {
		{ 4 * 8, 18 * 8, 12 * 8, 32 },
		"Name (set)"
	};
	Text text_timestamp {
		{ 17 * 8, 24 * 8, 11 * 8, 16 },
		"MM/DD HH:MM",
	};
};

class FileLoadView : public FileManBaseView {
public:
	std::function<void(std::filesystem::path)> on_changed { };
	
	FileLoadView(NavigationView& nav);

private:
	void refresh_widgets(const bool v);
};

class FileManagerView : public FileManBaseView {
public:
	FileManagerView(NavigationView& nav);
	~FileManagerView();

private:
	std::string filename_buffer { };
	
	void refresh_widgets(const bool v);
	void on_rename(NavigationView& nav);
	void on_delete();

	/*Labels labels {
		{ { 4 * 8 + 4, 26 * 8 }, "Edit:", Color::light_grey() }
	};*/

	Button button_rename {
		{ 0 * 8, 28 * 8, 14 * 8, 32 },
		"Rename"
	};
	
	Button button_new_dir {
		{ 0 * 8, 33 * 8, 14 * 8, 32 },
		"New dir"
	};
	
	Button button_delete {
		{ 18 * 8, 28 * 8, 12 * 8, 32 },
		"Delete"
	};
};

} /* namespace ui */
