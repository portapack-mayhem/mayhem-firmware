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
	
	text_current.set(dir_path.string().length()? dir_path.string().substr(0, 30 - 6):"(sd root)");

	entry_list.clear();
	
	auto filtering = (bool)extension_filter.size();
	
	// List directories and files, put directories up top
	if (dir_path.string().length())
		entry_list.push_back({ u"..", 0, true });
	
	for (const auto& entry : std::filesystem::directory_iterator(dir_path, u"*")) {
		if (std::filesystem::is_regular_file(entry.status())) {
			if (entry.path().string().length()) {
				bool matched = true;
				if (filtering) {
					auto entry_extension = entry.path().extension().string();
				
					for (auto &c: entry_extension)
						c = toupper(c);
					
					if (entry_extension != extension_filter)
						matched = false;
				}
				
				if (matched)
					entry_list.push_back({ entry.path(), (uint32_t)entry.size(), false });
			}
		} else if (std::filesystem::is_directory(entry.status())) {
			entry_list.insert(entry_list.begin(), { entry.path(), 0, true });
		}
	}
}

std::filesystem::path FileManBaseView::get_selected_path() {
	auto selected_path_str = current_path.string();
	auto entry_path = entry_list[menu_view.highlighted_index()].entry_path.string();
	
	if (entry_path == "..") {
		selected_path_str = get_parent_dir().string();
	} else {
		if (selected_path_str.back() != '/')
			selected_path_str += '/';
		
		selected_path_str += entry_path;
	}
	
	return selected_path_str;
}

std::filesystem::path FileManBaseView::get_parent_dir() {
	auto current_path_str = current_path.string();
	return current_path.string().substr(0, current_path_str.find_last_of('/'));
}

FileManBaseView::FileManBaseView(
	NavigationView& nav,
	std::string filter
) : nav_ (nav),
	extension_filter { filter }
{
	load_directory_contents(current_path);
	
	if (!entry_list.size())
		empty_root = true;
	
	add_children({
		&labels,
		&text_current,
		&button_exit
	});

	menu_view.on_left = [&nav, this]() {
		load_directory_contents(get_parent_dir());
		refresh_list();
	};
	
	button_exit.on_select = [this, &nav](Button&) {
		nav.pop();
	};
};

void FileManBaseView::focus() {
	if (empty_root) {
		button_exit.focus();
		nav_.display_modal("Error", "No files in root.", ABORT, nullptr);
	} else {
		menu_view.focus();
	}
}

void FileManBaseView::refresh_list() {
	if (on_refresh_widgets)
		on_refresh_widgets(false);

	menu_view.clear();
	
	for (size_t n = 0; n < entry_list.size(); n++) {
		auto entry = &entry_list[n];
		auto entry_name = entry->entry_path.filename().string().substr(0, 20);
		
		if (entry->is_directory) {
			
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
			
			auto file_size = entry->size;
			size_t suffix_index = 0;
			
			while (file_size >= 1024) {
				file_size /= 1024;
				suffix_index++;
			}
			if (suffix_index > 4)
				suffix_index = 4;
			
			std::string size_str = to_string_dec_uint(file_size) + suffix[suffix_index];
			
			auto entry_extension = entry->entry_path.extension().string();
			for (auto &c: entry_extension)
				c = toupper(c);
			
			// Associate extension to icon and color
			size_t c;
			for (c = 0; c < file_types.size() - 1; c++) {
				if (entry_extension == file_types[c].extension)
					break;
			}
			
			menu_view.add_item({
				entry_name + std::string(21 - entry_name.length(), ' ') + size_str,
				file_types[c].color,
				file_types[c].icon,
				[this](){
					if (on_select_entry)
						on_select_entry();
				}
			});
			
		}
	}
	
	menu_view.set_highlighted(0);	// Refresh
}

/*void FileSaveView::on_save_name() {
	text_prompt(nav_, &filename_buffer, 8, [this](std::string * buffer) {
		nav_.pop();
	});
}

FileSaveView::FileSaveView(
	NavigationView& nav
) : FileManBaseView(nav)
{
	name_buffer.clear();
	
	add_children({
		&text_save,
		&button_save_name,
		&live_timestamp
	});
	
	button_save_name.on_select = [this, &nav](Button&) {
		on_save_name();
	};
}*/

void FileLoadView::refresh_widgets(const bool v) {
	set_dirty();
}

FileLoadView::FileLoadView(
	NavigationView& nav,
	std::string filter
) : FileManBaseView(nav, filter)
{
	on_refresh_widgets = [this](bool v) {
		refresh_widgets(v);
	};
	
	add_children({
		&menu_view
	});
	
	// Resize menu view to fill screen
	menu_view.set_parent_rect({ 0, 3 * 8, 240, 29 * 8 });
	
	refresh_list();
	
	on_select_entry = [&nav, this]() {
		if (entry_list[menu_view.highlighted_index()].is_directory) {
			load_directory_contents(get_selected_path());
			refresh_list();
		} else {
			nav_.pop();
			if (on_changed)
				on_changed(current_path.string() + '/' + entry_list[menu_view.highlighted_index()].entry_path.string());
		}
	};
}

void FileManagerView::on_rename(NavigationView& nav) {
	text_prompt(nav, name_buffer, max_filename_length, [this](std::string& buffer) {
		rename_file(get_selected_path(), buffer);
		load_directory_contents(current_path);
		refresh_list();
	});
}

void FileManagerView::on_delete() {
	delete_file(get_selected_path());
	load_directory_contents(current_path);
	refresh_list();
}

void FileManagerView::refresh_widgets(const bool v) {
	button_rename.hidden(v);
	button_new_dir.hidden(v);
	button_delete.hidden(v);
	set_dirty();
}

FileManagerView::~FileManagerView() {
	// Flush ?
}

FileManagerView::FileManagerView(
	NavigationView& nav
) : FileManBaseView(nav, "")
{
	on_refresh_widgets = [this](bool v) {
		refresh_widgets(v);
	};
	
	add_children({
		&menu_view,
		&labels,
		&text_date,
		&button_rename,
		&button_new_dir,
		&button_delete
	});
	
	menu_view.on_highlight = [this]() {
		text_date.set(to_string_FAT_timestamp(file_created_date(get_selected_path())));
	};
	
	refresh_list();
	
	on_select_entry = [this]() {
		if (entry_list[menu_view.highlighted_index()].is_directory) {
			load_directory_contents(get_selected_path());
			refresh_list();
		} else
			button_rename.focus();
	};
	
	button_new_dir.on_select = [this, &nav](Button&) {
		name_buffer.clear();
		
		text_prompt(nav, name_buffer, max_filename_length, [this](std::string& buffer) {
			make_new_directory(current_path.string() + '/' + buffer);
			load_directory_contents(current_path);
			refresh_list();
		});
	};
	
	button_rename.on_select = [this, &nav](Button&) {
		name_buffer = entry_list[menu_view.highlighted_index()].entry_path.filename().string().substr(0, max_filename_length);
		on_rename(nav);
	};
	
	button_delete.on_select = [this, &nav](Button&) {
		// Use display_modal ?
		nav.push<ModalMessageView>("Delete", "Delete " + entry_list[menu_view.highlighted_index()].entry_path.filename().string() + "\nAre you sure?", YESNO,
			[this](bool choice) {
				if (choice)
					on_delete();
			}
		);
	};
}

}
