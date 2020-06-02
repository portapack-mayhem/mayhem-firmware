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

namespace ui {

struct fileman_entry {
	std::filesystem::path entry_path { };
	uint32_t size { };
	bool is_directory { };
};

class FileManBaseView : public View {
public:
	FileManBaseView(
		NavigationView& nav,
		std::string filter
	);

	void focus() override;
	
	void load_directory_contents(const std::filesystem::path& dir_path);
	std::filesystem::path get_selected_path();
	
	std::string title() const override { return "File manager"; };
	
protected:
	NavigationView& nav_;
	
	static constexpr size_t max_filename_length = 30 - 2;
	
	const std::string suffix[5] = { "B", "kB", "MB", "GB", "??" };
	
	struct file_assoc_t {
		std::string extension;
		const Bitmap* icon;
		ui::Color color;
	};
	
	const std::vector<file_assoc_t> file_types = {
		{ ".TXT", &bitmap_icon_file_text, ui::Color::white() },
		{ ".PNG", &bitmap_icon_file_image, ui::Color::green() },
		{ ".BMP", &bitmap_icon_file_image, ui::Color::green() },
		{ ".C8",  &bitmap_icon_file_iq, ui::Color::blue() },
		{ ".C16", &bitmap_icon_file_iq, ui::Color::blue() },
		{ ".WAV", &bitmap_icon_file_wav, ui::Color::dark_magenta() },
		{ "", &bitmap_icon_file, ui::Color::light_grey() }
	};
	
	bool empty_root { false };
	std::function<void(void)> on_select_entry { nullptr };
	std::function<void(bool)> on_refresh_widgets { nullptr };
	std::vector<fileman_entry> entry_list { };
	std::filesystem::path current_path { u"" };
	std::string extension_filter { "" };
	
	void change_category(int32_t category_id);
	std::filesystem::path get_parent_dir();
	void refresh_list();
	
	Labels labels {
		{ { 0, 0 }, "Path:", Color::light_grey() }
	};
	Text text_current {
		{ 6 * 8, 0 * 8, 24 * 8, 16 },
		"",
	};
	
	MenuView menu_view {
		{ 0, 2 * 8, 240, 26 * 8 },
		true
	};
	
	Button button_exit {
		{ 16 * 8, 34 * 8, 14 * 8, 32 },
		"Exit"
	};
};

/*class FileSaveView : public FileManBaseView {
public:
	FileSaveView(NavigationView& nav);
	~FileSaveView();

private:
	std::string name_buffer { };
	
	void on_save_name();
	
	Text text_save {
		{ 4 * 8, 15 * 8, 8 * 8, 16 },
		"Save as:",
	};
	Button button_save_name {
		{ 4 * 8, 18 * 8, 12 * 8, 32 },
		"Name (set)"
	};
	LiveDateTime live_timestamp {
		{ 17 * 8, 24 * 8, 11 * 8, 16 }
	};
};*/

class FileLoadView : public FileManBaseView {
public:
	std::function<void(std::filesystem::path)> on_changed { };
	
	FileLoadView(NavigationView& nav, std::string filter);

private:
	void refresh_widgets(const bool v);
};

class FileManagerView : public FileManBaseView {
public:
	FileManagerView(NavigationView& nav);
	~FileManagerView();

private:
	std::string name_buffer { };
	
	void refresh_widgets(const bool v);
	void on_rename(NavigationView& nav);
	void on_delete();
	
	Labels labels {
		{ { 0, 26 * 8 }, "Created ", Color::light_grey() }
	};
	
	Text text_date {
		{ 8 * 8, 26 * 8 , 19 * 8, 16 },
		""
	};

	Button button_rename {
		{ 0 * 8, 29 * 8, 14 * 8, 32 },
		"Rename"
	};
	Button button_delete {
		{ 16 * 8, 29 * 8, 14 * 8, 32 },
		"Delete"
	};
	
	Button button_new_dir {
		{ 0 * 8, 34 * 8, 14 * 8, 32 },
		"New dir"
	};
	
};

} /* namespace ui */
