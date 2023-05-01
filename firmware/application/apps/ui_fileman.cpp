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

#include <algorithm>
#include "ui_fileman.hpp"
#include "string_format.hpp"
#include "portapack.hpp"
#include "event_m0.hpp"

using namespace portapack;

namespace {
using namespace ui;

bool is_hidden_file(const std::filesystem::path& path) {
	return !path.empty() && path.native()[0] == u'.';
}

// Gets a truncated name from a path for display.
std::string truncate(const std::filesystem::path& path, size_t max_length = 25) {
	auto name = path.string();
	return name.length() <= max_length ? name : name.substr(0, max_length);
}

// Gets a human readable size string.
std::string get_pretty_size(uint32_t file_size) {
	static const std::string suffix[5] = { "B", "kB", "MB", "GB", "??" };
	size_t suffix_index = 0;
	
	while (file_size >= 1024) {
		file_size /= 1024;
		suffix_index++;
	}

	if (suffix_index > 4)
		suffix_index = 4;
	
	return to_string_dec_uint(file_size) + suffix[suffix_index];
}

// Case insensitive path equality on underlying "native" string.
bool iequal(
	const std::filesystem::path& lhs,
	const std::filesystem::path& rhs
) {
	const auto& lhs_str = lhs.native();
	const auto& rhs_str = rhs.native();

	// NB: Not correct for Unicode/locales.
	if (lhs_str.length() == rhs_str.length()) {
		for (size_t i = 0; i < lhs_str.length(); ++i)
			if (towupper(lhs_str[i]) != towupper(rhs_str[i]))
				return false;

		return true;
	}

	return false;
}

// Inserts the entry into the entry list sorted directories first then by file name.
void insert_sorted(std::vector<fileman_entry>& entries, fileman_entry&& entry) {
	auto it = std::lower_bound(std::begin(entries), std::end(entries), entry,
		[](const fileman_entry& lhs, const fileman_entry& rhs) {
			if (lhs.is_directory && !rhs.is_directory)
				return true;
			else if (!lhs.is_directory && rhs.is_directory)
				return false;
			else
				return lhs.path < rhs.path;
		});

	entries.insert(it, std::move(entry));
}

// Returns the partner file path or an empty path if no partner is found.
std::filesystem::path get_partner_file(std::filesystem::path path) {
	const auto txt_path = std::filesystem::path{ u".TXT" };
	const auto c16_path = std::filesystem::path{ u".C16" };
	auto ext = path.extension();

	if (iequal(ext, txt_path))
		ext = c16_path;
	else if (iequal(ext, c16_path))
		ext = txt_path;
	else
		return { };

	path.replace_extension(ext);
	return file_exists(path) ? path : std::filesystem::path{ };
}

// Modal prompt to update the partner file.
// Returns true if user wants to update the partner file too.
bool partner_file_prompt(NavigationView& nav, const std::filesystem::path& partner) {
	bool result = false;

	if (partner.empty())
		return result;

	nav.push<ModalMessageView>(
		"Partner File Exists",
		partner.filename().string() + "\nUpdate this file too?",
		YESNO,
		[&result](bool choice) { result = choice; }
	);

	return result;
}

}

namespace ui {

void FileManBaseView::load_directory_contents(const std::filesystem::path& dir_path) {
	current_path = dir_path;
	entry_list.clear();
	auto filtering = !extension_filter.empty();

	text_current.set(dir_path.empty() ? "(sd root)" : truncate(dir_path));
	
	for (const auto& entry : std::filesystem::directory_iterator(dir_path, u"*")) {
		// Hide files starting with '.' (hidden / tmp).
		if (is_hidden_file(entry.path()))
			continue;

		if (std::filesystem::is_regular_file(entry.status())) {
			if (!filtering || iequal(entry.path().extension(), extension_filter))
				insert_sorted(entry_list, { entry.path(), (uint32_t)entry.size(), false });
		} else if (std::filesystem::is_directory(entry.status())) {
			insert_sorted(entry_list, { entry.path(), 0, true });
		}
	}

	// Add "parent" directory if not at the root.
	if (!dir_path.empty())
		entry_list.insert(entry_list.begin(), { parent_dir_path, 0, true });
}

std::filesystem::path FileManBaseView::get_selected_full_path() const {
	if (get_selected_entry().path == parent_dir_path)
		return current_path.parent_path();

	return current_path / get_selected_entry().path;
}

const fileman_entry& FileManBaseView::get_selected_entry() const {
	return entry_list[menu_view.highlighted_index()];
}

FileManBaseView::FileManBaseView(
	NavigationView& nav,
	std::string filter
) : nav_ (nav),
	extension_filter { filter }
{
	add_children({
		&labels,
		&text_current,
		&button_exit
	});

	button_exit.on_select = [this, &nav](Button&) {
		nav.pop();
	};

	if (!sdcIsCardInserted(&SDCD1)) {
		empty_root = true;
		text_current.set("NO SD CARD!");
		return;
	}
	
	load_directory_contents(current_path);

	if (!entry_list.size()) {
		empty_root = true;
		text_current.set("EMPTY SD CARD!");
	} else {
		menu_view.on_left = [&nav, this]() {
			load_directory_contents(current_path.parent_path());
			refresh_list();
		};
	}
}

void FileManBaseView::focus() {
	if (empty_root) {
		button_exit.focus();
	} else {
		menu_view.focus();
	}
}

void FileManBaseView::refresh_list() {
	if (on_refresh_widgets)
		on_refresh_widgets(false);

	menu_view.clear();
	
	for (const auto& entry : entry_list) {
		auto entry_name = truncate(entry.path, 20);
	
		if (entry.is_directory) {
			menu_view.add_item({
				entry_name,
				ui::Color::yellow(),
				&bitmap_icon_dir,
				[this]() {
					if (on_select_entry)
						on_select_entry();
				}
			});
	
		} else {
			const auto& assoc = get_assoc(entry.path.extension());
			auto size_str = get_pretty_size(entry.size);
			
			menu_view.add_item({
				entry_name + std::string(21 - entry_name.length(), ' ') + size_str,
				assoc.color,
				assoc.icon,
				[this]() {
					if (on_select_entry)
						on_select_entry();
				}
			});
		}
	}
	
	menu_view.set_highlighted(0); // Refresh
}

const FileManBaseView::file_assoc_t& FileManBaseView::get_assoc(
	const std::filesystem::path& ext) const
{
	size_t index = 0;

	for (; index < file_types.size() - 1; ++index)
		if (iequal(ext, file_types[index].extension))
			return file_types[index];

	// Default to last entry in the list.
	return file_types[index];
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

void FileLoadView::refresh_widgets(const bool) {
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
	
	on_select_entry = [this]() {
		if (get_selected_entry().is_directory) {
			load_directory_contents(get_selected_full_path());
			refresh_list();
		} else {
			nav_.pop();
			if (on_changed)
				on_changed(get_selected_full_path());
		}
	};
}

void FileManagerView::on_rename() {
	// Don't allow rename of ".."
	if (get_selected_entry().path == parent_dir_path)
		return;

	auto& entry = get_selected_entry();
	auto name = entry.path.filename().string();
	auto orig_ext = entry.path.extension();

	uint32_t cursor_pos = (uint32_t)name.length();
	if (auto pos = name.find_last_of("."); pos != name.npos)
		cursor_pos = pos;

	text_prompt(nav_, name, cursor_pos, max_filename_length,
		[this, &entry, &orig_ext](std::string& renamed) {
			auto renamed_path = std::filesystem::path{ renamed };
			rename_file(get_selected_full_path(), current_path / renamed_path);

			if (iequal(renamed_path.extension(), orig_ext)) {
				auto partner = get_partner_file(current_path / entry.path);
				if (partner_file_prompt(nav_, partner)) {
					auto new_name = renamed_path.replace_extension(partner.extension());
					rename_file(partner, current_path / new_name);
				}
			}

			load_directory_contents(current_path);
			refresh_list();
		});
}

void FileManagerView::on_delete() {
	// Don't allow delete of ".."
	if (get_selected_entry().path == parent_dir_path)
		return;

	auto& entry = get_selected_entry();
	auto name = entry.path.filename().string();
	nav_.push<ModalMessageView>("Delete", "Delete " + name + "\nAre you sure?", YESNO,
		[this, &entry](bool choice) {
			if (choice) {
				delete_file(get_selected_full_path());

				auto partner = get_partner_file(current_path / entry.path);
				if (partner_file_prompt(nav_, partner))
					delete_file(partner);

				load_directory_contents(current_path);
				refresh_list();
			}
		}
	);
}

void FileManagerView::on_new_dir() {
	std::string name;
	text_prompt(nav_, name, max_filename_length, [this](std::string& dir_name) {
		make_new_directory(current_path / dir_name);
		load_directory_contents(current_path);
		refresh_list();
	});
}

void FileManagerView::refresh_widgets(const bool v) {
	button_rename.hidden(v);
	button_delete.hidden(v);
	button_new_dir.hidden(v);
	set_dirty();
}

FileManagerView::~FileManagerView() {
	// Flush ?
}

FileManagerView::FileManagerView(
	NavigationView& nav
) : FileManBaseView(nav, "")
{
	if (!empty_root) {
		on_refresh_widgets = [this](bool v) {
			refresh_widgets(v);
		};
		
		add_children({
			&menu_view,
			&labels,
			&text_date,
			&button_rename,
			//&button_copy,
			//&button_move,
			&button_delete,
			//&button_new_file,
			&button_new_dir,
		});
		
		menu_view.on_highlight = [this]() {
			text_date.set(to_string_FAT_timestamp(file_created_date(get_selected_full_path())));
		};
		
		refresh_list();
		
		on_select_entry = [this]() {
			if (get_selected_entry().is_directory) {
				load_directory_contents(get_selected_full_path());
				refresh_list();
			} else
				button_rename.focus();
		};
		
		button_rename.on_select = [this](Button&) {
			on_rename();
		};

		button_delete.on_select = [this](Button&) {
			on_delete();
		};

		button_new_dir.on_select = [this](Button&) {
			on_new_dir();
		};
	} 
}

}
