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

/* TODO:
 * - Paging menu items
 */

#include <algorithm>
#include "ui_fileman.hpp"
#include "ui_playlist.hpp"
#include "ui_ss_viewer.hpp"
#include "ui_text_editor.hpp"
#include "string_format.hpp"
#include "portapack.hpp"
#include "event_m0.hpp"

using namespace portapack;
namespace fs = std::filesystem;

namespace ui {
static const fs::path txt_ext{u".TXT"};
static const fs::path ppl_ext{u".PPL"};
static const fs::path c8_ext{u".C8"};
static const fs::path c16_ext{u".C16"};
static const fs::path cxx_ext{u".C*"};
static const fs::path png_ext{u".PNG"};
static const fs::path bmp_ext{u".BMP"};
}  // namespace ui

namespace {
using namespace ui;

bool is_hidden_file(const fs::path& path) {
    return !path.empty() && path.native()[0] == u'.';
}

// Gets a truncated name from a path for display.
std::string truncate(const fs::path& path, size_t max_length) {
    return ::truncate(path.string(), max_length);
}

// Inserts the entry into the entry list sorted directories first then by file name.
void insert_sorted(std::vector<fileman_entry>& entries, fileman_entry&& entry) {
    auto it = std::lower_bound(
        std::begin(entries), std::end(entries), entry,
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
fs::path get_partner_file(fs::path path) {
    if (fs::is_directory(path))
        return {};
    auto ext = path.extension();

    if (is_cxx_capture_file(path))
        path.replace_extension(txt_ext);
    else if (path_iequal(ext, txt_ext)) {
        path.replace_extension(c8_ext);
        if (!fs::file_exists(path))
            path.replace_extension(c16_ext);
    } else
        return {};

    return fs::file_exists(path) && !fs::is_directory(path) ? path : fs::path{};
}

// Modal prompt to update the partner file if it exists.
// Runs continuation on_partner_action to update the partner file.
// Returns true is a partner is found, otherwise false.
// Path must be the full path to the file.
bool partner_file_prompt(
    NavigationView& nav,
    const fs::path& path,
    std::string action_name,
    std::function<void(const fs::path&, bool)> on_partner_action) {
    auto partner = get_partner_file(path);

    if (partner.empty())
        return false;

    nav.push_under_current<ModalMessageView>(
        "Partner File",
        partner.filename().string() + "\n" + action_name + " this file too?",
        YESNO,
        [&nav, partner, on_partner_action](bool choice) {
            if (on_partner_action)
                on_partner_action(partner, choice);
        });

    return true;
}

fs::path get_unique_filename(const fs::path& path, const fs::path& file) {
    auto stem = file.stem();
    auto ext = file.extension();
    auto serial = 1;
    fs::path new_path = file;

    while (fs::file_exists(path / new_path)) {
        new_path = stem;
        new_path += fs::path{u"_"};
        new_path += to_string_dec_int(serial++);
        new_path += ext;
    }

    return new_path;
}

}  // namespace

namespace ui {

/* FileManBaseView ***********************************************************/

void FileManBaseView::load_directory_contents(const fs::path& dir_path) {
    current_path = dir_path;
    entry_list.clear();
    auto filtering = !extension_filter.empty();
    bool cxx_file = path_iequal(cxx_ext, extension_filter);

    text_current.set(dir_path.empty() ? "(sd root)" : truncate(dir_path, 24));

    for (const auto& entry : fs::directory_iterator(dir_path, u"*")) {
        // Hide files starting with '.' (hidden / tmp).
        if (is_hidden_file(entry.path()))
            continue;

        if (fs::is_regular_file(entry.status())) {
            if (!filtering || path_iequal(entry.path().extension(), extension_filter) || (cxx_file && is_cxx_capture_file(entry.path())))
                insert_sorted(entry_list, {entry.path(), (uint32_t)entry.size(), false});
        } else if (fs::is_directory(entry.status())) {
            insert_sorted(entry_list, {entry.path(), 0, true});
        }
    }

    // Add "parent" directory if not at the root.
    if (!dir_path.empty())
        entry_list.insert(entry_list.begin(), {parent_dir_path, 0, true});
}

fs::path FileManBaseView::get_selected_full_path() const {
    if (get_selected_entry().path == parent_dir_path)
        return current_path.parent_path();

    return current_path / get_selected_entry().path;
}

const fileman_entry& FileManBaseView::get_selected_entry() const {
    // TODO: return reference to an "empty" entry on OOB?
    return entry_list[menu_view.highlighted_index()];
}

FileManBaseView::FileManBaseView(
    NavigationView& nav,
    std::string filter)
    : nav_{nav},
      extension_filter{filter} {
    add_children({&labels,
                  &text_current,
                  &button_exit});

    button_exit.on_select = [this, &nav](Button&) {
        nav.pop();
    };

    if (!sdcIsCardInserted(&SDCD1)) {
        empty_ = EmptyReason::NoSDC;
        text_current.set("NO SD CARD!");
        return;
    }

    load_directory_contents(current_path);

    if (!entry_list.size()) {
        empty_ = EmptyReason::NoFiles;
        text_current.set("EMPTY SD CARD!");
    } else {
        menu_view.on_left = [this]() {
            pop_dir();
        };
    }
}

void FileManBaseView::focus() {
    if (empty_ != EmptyReason::NotEmpty) {
        button_exit.focus();
    } else {
        menu_view.focus();
    }
}

void FileManBaseView::push_dir(const fs::path& path) {
    if (path == parent_dir_path) {
        pop_dir();
    } else {
        current_path /= path;
        saved_index_stack.push_back(menu_view.highlighted_index());
        menu_view.set_highlighted(0);
        reload_current();
    }
}

void FileManBaseView::pop_dir() {
    if (saved_index_stack.empty())
        return;

    current_path = current_path.parent_path();
    reload_current();
    menu_view.set_highlighted(saved_index_stack.back());
    saved_index_stack.pop_back();
}

void FileManBaseView::refresh_list() {
    if (on_refresh_widgets)
        on_refresh_widgets(false);

    auto prev_highlight = menu_view.highlighted_index();
    menu_view.clear();

    for (const auto& entry : entry_list) {
        auto entry_name = truncate(entry.path, 20);

        if (entry.is_directory) {
            auto size_str =
                (entry.path == parent_dir_path)
                    ? ""
                    : to_string_dec_uint(file_count(current_path / entry.path));

            menu_view.add_item(
                {entry_name + std::string(21 - entry_name.length(), ' ') + size_str,
                 ui::Color::yellow(),
                 &bitmap_icon_dir,
                 [this](KeyEvent key) {
                     if (on_select_entry)
                         on_select_entry(key);
                 }});

        } else {
            const auto& assoc = get_assoc(entry.path.extension());
            auto size_str = to_string_file_size(entry.size);

            menu_view.add_item(
                {entry_name + std::string(21 - entry_name.length(), ' ') + size_str,
                 assoc.color,
                 assoc.icon,
                 [this](KeyEvent key) {
                     if (on_select_entry)
                         on_select_entry(key);
                 }});
        }

        // HACK: Should page menu items instead of limiting the number.
        if (menu_view.item_count() >= max_items_shown)
            break;
    }

    menu_view.set_highlighted(prev_highlight);
}

void FileManBaseView::reload_current() {
    load_directory_contents(current_path);
    refresh_list();
}

const FileManBaseView::file_assoc_t& FileManBaseView::get_assoc(
    const fs::path& ext) const {
    size_t index = 0;

    for (; index < file_types.size() - 1; ++index)
        if (path_iequal(ext, file_types[index].extension))
            return file_types[index];

    // Default to last entry in the list.
    return file_types[index];
}

/* FileLoadView **************************************************************/

FileLoadView::FileLoadView(
    NavigationView& nav,
    std::string filter)
    : FileManBaseView(nav, filter) {
    on_refresh_widgets = [this](bool v) {
        refresh_widgets(v);
    };

    add_children({&menu_view});

    // Resize menu view to fill screen
    menu_view.set_parent_rect({0, 3 * 8, 240, 29 * 8});

    refresh_list();

    on_select_entry = [this](KeyEvent) {
        if (get_selected_entry().is_directory) {
            push_dir(get_selected_entry().path);
        } else {
            nav_.pop();
            if (on_changed)
                on_changed(get_selected_full_path());
        }
    };
}

void FileLoadView::refresh_widgets(const bool) {
    set_dirty();
}

/* FileSaveView **************************************************************/
/*
FileSaveView::FileSaveView(
        NavigationView& nav,
        const fs::path& path,
        const fs::path& file
) : nav_{ nav },
        path_{ path },
        file_{ file }
{
        add_children({
                &text_path,
                &button_edit_path,
                &text_name,
                &button_edit_name,
                &button_save,
                &button_cancel,
        });

        button_edit_path.on_select = [this](Button&) {
                buffer_ = path_.string();
                text_prompt(nav_, buffer_, max_filename_length,
                        [this](std::string&) {
                                path_ = buffer_;
                                refresh_widgets();
                        });
        };

        button_edit_name.on_select = [this](Button&) {
                buffer_ = file_.string();
                text_prompt(nav_, buffer_, max_filename_length,
                        [this](std::string&) {
                                file_ = buffer_;
                                refresh_widgets();
                        });
        };

        button_save.on_select = [this](Button&) {
                if (on_save)
                        on_save(path_ / file_);
                else
                        nav_.pop();
        };

        button_cancel.on_select = [this](Button&) {
                nav_.pop();
        };

        refresh_widgets();
}

void FileSaveView::refresh_widgets() {
        text_path.set(truncate(path_, 30));
        text_name.set(truncate(file_, 30));
        set_dirty();
}
*/
/* FileManagerView ***********************************************************/

void FileManagerView::refresh_widgets(const bool v) {
    button_rename.hidden(v);
    button_delete.hidden(v);
    button_cut.hidden(v);
    button_copy.hidden(v);
    button_paste.hidden(v);
    button_new_dir.hidden(v);
    button_new_file.hidden(v);

    set_dirty();
}

void FileManagerView::on_rename(std::string_view hint) {
    auto& entry = get_selected_entry();

    // Append the hint to the filename stem as a rename suggestion.
    name_buffer = entry.path.stem().string();
    if (!hint.empty())
        name_buffer += "_" + std::string{hint};
    name_buffer += entry.path.extension().string();

    // Set the rename cursor to before the extension to make renaming simpler.
    uint32_t cursor_pos = (uint32_t)name_buffer.length();
    if (auto pos = name_buffer.find_last_of(".");
        pos != name_buffer.npos && !entry.is_directory)
        cursor_pos = pos;

    text_prompt(
        nav_, name_buffer, cursor_pos, max_filename_length,
        [this](std::string& renamed) {
            auto renamed_path = fs::path{renamed};
            rename_file(get_selected_full_path(), current_path / renamed_path);

            auto has_partner = partner_file_prompt(
                nav_, get_selected_full_path(), "Rename",
                [this, renamed_path](const fs::path& partner, bool should_rename) mutable {
                    if (should_rename) {
                        auto new_name = renamed_path.replace_extension(partner.extension());
                        rename_file(partner, current_path / new_name);
                    }
                    reload_current();
                });

            if (!has_partner)
                reload_current();
        });
}

void FileManagerView::on_delete() {
    if (is_directory(get_selected_full_path()) && !is_empty_directory(get_selected_full_path())) {
        nav_.display_modal("Delete", "Directory not empty!");
        return;
    }

    auto name = get_selected_entry().path.filename().string();
    nav_.push<ModalMessageView>(
        "Delete", "Delete " + name + "\nAre you sure?", YESNO,
        [this](bool choice) {
            if (choice) {
                delete_file(get_selected_full_path());

                auto has_partner = partner_file_prompt(
                    nav_, get_selected_full_path(), "Delete",
                    [this](const fs::path& partner, bool should_delete) {
                        if (should_delete)
                            delete_file(partner);
                        reload_current();
                    });

                if (!has_partner)
                    reload_current();
            }
        });
}

void FileManagerView::on_new_dir() {
    name_buffer = "";
    text_prompt(nav_, name_buffer, max_filename_length, [this](std::string& dir_name) {
        make_new_directory(current_path / dir_name);
        reload_current();
    });
}

void FileManagerView::on_paste() {
    // TODO: handle partner file. Need to fix nav stack first.
    auto new_name = get_unique_filename(current_path, clipboard_path.filename());
    fs::filesystem_error result;

    if (clipboard_mode == ClipboardMode::Cut)
        result = rename_file(clipboard_path, current_path / new_name);

    else if (clipboard_mode == ClipboardMode::Copy)
        result = copy_file(clipboard_path, current_path / new_name);

    if (result.code() != FR_OK)
        nav_.display_modal("Paste Failed", result.what());

    clipboard_path = fs::path{};
    clipboard_mode = ClipboardMode::None;
    menu_view.focus();
    reload_current();
}

void FileManagerView::on_new_file() {
    name_buffer = "";
    text_prompt(nav_, name_buffer, max_filename_length, [this](std::string& file_name) {
        make_new_file(current_path / file_name);
        reload_current();
    });
}

bool FileManagerView::handle_file_open() {
    if (!selected_is_valid())
        return false;

    auto path = get_selected_full_path();
    auto ext = path.extension();

    if (path_iequal(txt_ext, ext)) {
        nav_.push<TextEditorView>(path);
        return true;
    } else if (is_cxx_capture_file(path) || path_iequal(ppl_ext, ext)) {
        // TODO: Enough memory to push?
        nav_.push<PlaylistView>(path);
        return true;
    } else if (path_iequal(png_ext, ext)) {
        nav_.push<ScreenshotViewer>(path);
        return true;
    } else if (path_iequal(bmp_ext, ext)) {
        nav_.push<SplashViewer>(path);
        reload_current();
        return true;
    }

    return false;
}

bool FileManagerView::selected_is_valid() const {
    return !entry_list.empty() &&
           get_selected_entry().path != parent_dir_path;
}

FileManagerView::FileManagerView(
    NavigationView& nav)
    : FileManBaseView(nav, "") {
    // Don't bother with the UI in the case of no SDC.
    if (empty_ == EmptyReason::NoSDC)
        return;

    on_refresh_widgets = [this](bool v) {
        refresh_widgets(v);
    };

    add_children({
        &menu_view,
        &text_date,
        &button_rename,
        &button_delete,
        &button_cut,
        &button_copy,
        &button_paste,
        &button_new_dir,
        &button_new_file,
        &button_open_notepad,
        &button_rename_timestamp,
    });

    menu_view.on_highlight = [this]() {
        if (menu_view.highlighted_index() >= max_items_shown - 1) {
            text_date.set_style(&Styles::red);
            text_date.set("Too many files!");
        } else {
            text_date.set_style(&Styles::grey);
            if (selected_is_valid())
                text_date.set((is_directory(get_selected_full_path()) ? "Created " : "Modified ") + to_string_FAT_timestamp(file_created_date(get_selected_full_path())));
            else
                text_date.set("");
        }
    };

    refresh_list();

    on_select_entry = [this](KeyEvent key) {
        if (key == KeyEvent::Select && get_selected_entry().is_directory) {
            push_dir(get_selected_entry().path);
        } else if (key == KeyEvent::Select && handle_file_open()) {
            return;
        } else {
            button_rename.focus();
        }
    };

    button_rename.on_select = [this]() {
        if (selected_is_valid())
            on_rename("");
    };

    button_delete.on_select = [this]() {
        if (selected_is_valid())
            on_delete();
    };

    button_cut.on_select = [this]() {
        if (selected_is_valid() && !get_selected_entry().is_directory) {
            clipboard_path = get_selected_full_path();
            clipboard_mode = ClipboardMode::Cut;
        } else
            nav_.display_modal("Cut", "Can't cut that.");
    };

    button_copy.on_select = [this]() {
        if (selected_is_valid() && !get_selected_entry().is_directory) {
            clipboard_path = get_selected_full_path();
            clipboard_mode = ClipboardMode::Copy;
        } else
            nav_.display_modal("Copy", "Can't copy that.");
    };

    button_paste.on_select = [this]() {
        if (clipboard_mode != ClipboardMode::None)
            on_paste();
        else
            nav_.display_modal("Paste", "Cut or copy a file first.");
    };

    button_new_dir.on_select = [this]() {
        on_new_dir();
    };

    button_new_file.on_select = [this]() {
        on_new_file();
    };

    button_open_notepad.on_select = [this]() {
        if (selected_is_valid() && !get_selected_entry().is_directory) {
            auto path = get_selected_full_path();
            nav_.replace<TextEditorView>(path);
        } else
            nav_.display_modal("Open in Notepad", "Can't open that in Notepad.");
    };

    button_rename_timestamp.on_select = [this]() {
        if (selected_is_valid() && !get_selected_entry().is_directory) {
            on_rename(::truncate(to_string_timestamp(rtc_time::now()), 8));
        } else
            nav_.display_modal("Timestamp Rename", "Can't rename that.");
    };
}

}  // namespace ui
