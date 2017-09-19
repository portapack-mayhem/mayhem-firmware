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

#include "ui_fileman.hpp"
#include "string_format.hpp"
#include "portapack.hpp"
#include "event_m0.hpp"

using namespace portapack;

namespace ui {

void FileManBaseView::load_directory_contents(const std::filesystem::path& dir_path) {
	current_path = dir_path;
	
	text_current.set(dir_path.string());
	
	entry_list.clear();
	
	for (const auto& entry : std::filesystem::directory_iterator(dir_path, u"*")) {
		if (std::filesystem::is_regular_file(entry.status())) {
			entry_list.push_back({ entry.path(), (uint32_t)entry.size(), false });
		} else if (std::filesystem::is_directory(entry.status())) {
			entry_list.insert(entry_list.begin(), { entry.path(), 0, true });
		}
	}
}

std::filesystem::path FileManBaseView::get_absolute_path() {
	std::string current_path_str = current_path.string();
	
	if (current_path_str.back() != '/')
		current_path_str += '/';
	current_path_str += (entry_list[menu_view.highlighted()].entry_path.string());
	
	return current_path_str;
}

FileManBaseView::FileManBaseView(
	NavigationView& nav
) : nav_ (nav)
{
	load_directory_contents(current_path);
	
	if (!entry_list.size())
		error_ = true;
	
	add_children({
		&labels,
		&text_current,
		&button_exit
	});
	
	button_exit.on_select = [this, &nav](Button&) {
		nav.pop();
	};
};

void FileManBaseView::focus() {
	
	if (error_) {
		button_exit.focus();
		nav_.display_modal("Error", "No files in root.", ABORT, nullptr);
	} else {
		menu_view.focus();
	}
}

void FileManBaseView::refresh_list() {
	std::string size_str { };
	uint32_t suffix_index;
	size_t file_size;
	
	if (!entry_list.size()) {
		if (on_refresh_widgets)
			on_refresh_widgets(true);
	} else {
		if (on_refresh_widgets)
			on_refresh_widgets(false);
	
		menu_view.clear();
		
		for (size_t n = 0; n < entry_list.size(); n++) {
			auto entry_name = entry_list[n].entry_path.filename().string();
			
			if (entry_list[n].is_directory) {
				
				menu_view.add_item({
					entry_name,
					ui::Color::yellow(),
					&bitmap_icon_dir,
					[this](){
						if (on_select_entry)
							on_select_entry();
					}
				});
				
			} else {
				
				file_size = entry_list[n].size;
				suffix_index = 0;
				
				while (file_size >= 1024) {
					file_size /= 1024;
					suffix_index++;
				}
				if (suffix_index > 4)
					suffix_index = 4;
				
				size_str = to_string_dec_uint(file_size) + suffix[suffix_index];
				
				menu_view.add_item({
					entry_name + std::string(21 - entry_name.length(), ' ') + size_str,
					ui::Color::white(),
					&bitmap_icon_file,
					[this](){
						if (on_select_entry)
							on_select_entry();
					}
				});
				
			}
		}
		
		menu_view.set_highlighted(0);	// Refresh
	}
}

void FileSaveView::on_save_name() {
	/*text_prompt(nav_, &filename_buffer, 8, [this](std::string * buffer) {
		//database.entries.push_back({ value_, "", *buffer });
		//save_freqman_file(entry_list[current_category_id], database);
		nav_.pop();
	});*/
}

void FileSaveView::on_tick_second() {
	rtcGetTime(&RTCD1, &datetime);
	str_timestamp = to_string_dec_uint(datetime.month(), 2, '0') + "/" + to_string_dec_uint(datetime.day(), 2, '0') + " " +
						to_string_dec_uint(datetime.hour(), 2, '0') + ":" + to_string_dec_uint(datetime.minute(), 2, '0');
	text_timestamp.set(str_timestamp);
}

FileSaveView::~FileSaveView() {
	rtc_time::signal_tick_second -= signal_token_tick_second;
}

FileSaveView::FileSaveView(
	NavigationView& nav
) : FileManBaseView(nav)
{
	filename_buffer.reserve(8);
	
	signal_token_tick_second = rtc_time::signal_tick_second += [this]() {
		this->on_tick_second();
	};
	
	add_children({
		&text_save,
		&button_save_name,
		&text_timestamp
	});
	
	on_tick_second();
	
	button_save_name.on_select = [this, &nav](Button&) {
		on_save_name();
	};
}

void FileLoadView::refresh_widgets(const bool v) {
	menu_view.hidden(v);
	text_empty.hidden(!v);
	set_dirty();
}

FileLoadView::FileLoadView(
	NavigationView& nav
) : FileManBaseView(nav)
{
	on_refresh_widgets = [this](bool v) {
		refresh_widgets(v);
	};
	
	add_children({
		&menu_view,
		&text_empty
	});
	
	// Resize menu view to fill screen
	menu_view.set_parent_rect({ 0, 3 * 8, 240, 29 * 8 });
	
	// Just to allow exit on left
	menu_view.on_left = [&nav, this]() {
		nav.pop();
	};
	
	refresh_list();
	
	on_select_entry = [&nav, this]() {
		nav_.pop();
		if (on_changed)
			on_changed(entry_list[menu_view.highlighted()].entry_path);
	};
}

void FileManagerView::on_rename(NavigationView& nav) {
	text_prompt(nav, &filename_buffer, 12, [this](std::string * buffer) {
		rename_file(get_absolute_path(), *buffer);
		load_directory_contents(current_path);
		refresh_list();
	});
}

void FileManagerView::on_delete() {
	delete_file(get_absolute_path());
	load_directory_contents(current_path);
	refresh_list();
}

void FileManagerView::refresh_widgets(const bool v) {
	button_rename.hidden(v);
	button_new_dir.hidden(v);
	button_delete.hidden(v);
	menu_view.hidden(v);
	text_empty.hidden(!v);
	set_dirty();
}

FileManagerView::~FileManagerView() {
	// Flush ?
}

FileManagerView::FileManagerView(
	NavigationView& nav
) : FileManBaseView(nav)
{
	on_refresh_widgets = [this](bool v) {
		refresh_widgets(v);
	};
	
	add_children({
		//&labels,
		&menu_view,
		&text_empty,
		&button_rename,
		&button_new_dir,
		&button_delete
	});
	
	// Go back one level on left
	menu_view.on_left = [&nav, this]() {
		std::string current_path_str = current_path.string();
		
		current_path_str = current_path_str.substr(0, current_path_str.find_last_of('/'));
		load_directory_contents(current_path_str);
		refresh_list();
	};
	
	refresh_list();
	
	on_select_entry = [this]() {
		if (entry_list[menu_view.highlighted()].is_directory) {
			load_directory_contents(get_absolute_path());
			refresh_list();
		} else
			button_rename.focus();
	};
	
	button_new_dir.on_select = [this, &nav](Button&) {
		filename_buffer = "";
		
		text_prompt(nav, &filename_buffer, 12, [this](std::string * buffer) {
			std::string path_str = *buffer;
			
			make_new_directory(current_path.string() + '/' + path_str);
			load_directory_contents(current_path);
			refresh_list();
		});
	};
	
	button_rename.on_select = [this, &nav](Button&) {
		if (!entry_list[menu_view.highlighted()].is_directory) {
			filename_buffer = entry_list[menu_view.highlighted()].entry_path.filename().string();
			on_rename(nav);
		}
	};
	
	button_delete.on_select = [this, &nav](Button&) {
		nav.push<ModalMessageView>("Delete", "Delete " + entry_list[menu_view.highlighted()].entry_path.filename().string() + "\nAre you sure ?", YESNO,
			[this](bool choice) {
				if (choice)
					on_delete();
			}
		);
	};
}

}
