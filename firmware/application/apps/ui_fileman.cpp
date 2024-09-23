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
#include "ui_remote.hpp"
#include "ui_ss_viewer.hpp"
#include "ui_bmp_file_viewer.hpp"
#include "ui_text_editor.hpp"
#include "ui_iq_trim.hpp"
#include "string_format.hpp"
#include "portapack.hpp"
#include "event_m0.hpp"
#include "file_path.hpp"

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
static const fs::path rem_ext{u".REM"};
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
void insert_sorted(std::list<fileman_entry>& entries, fileman_entry&& entry) {
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

    // Display the continuation UI once the current has been popped.
    nav.set_on_pop([&nav, partner, action_name, on_partner_action] {
        nav.display_modal(
            "Partner File",
            partner.filename().string() + "\n" + action_name + " this file too?",
            YESNO,
            [&nav, partner, on_partner_action](bool choice) {
                if (on_partner_action)
                    on_partner_action(partner, choice);
            });
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

void FileManBaseView::load_directory_contents_unordered(const fs::path& dir_path, size_t file_cnt) {
    current_path = dir_path;
    entry_list.clear();
    menu_view.clear();
    auto filtering = !extension_filter.empty();
    bool cxx_file = path_iequal(cxx_ext, extension_filter);

    text_current.set(dir_path.empty() ? "(sd root)" : truncate(dir_path, 24));

    nb_pages = 1 + (file_cnt / items_per_page);
    size_t start = pagination * items_per_page;
    size_t stop = start + items_per_page;
    if (file_cnt < stop) stop = file_cnt;
    if (start > file_cnt) start = 0;  // shouldn't hapen but check against it won't hurt

    size_t curr = 0;

    for (const auto& entry : fs::directory_iterator(dir_path, u"*")) {
        if (entry_list.size() >= items_per_page) {
            break;
        }
        // Hide files starting with '.' (hidden / tmp).
        if (!show_hidden_files && is_hidden_file(entry.path()))
            continue;

        if (fs::is_regular_file(entry.status())) {
            if (!filtering || path_iequal(entry.path().extension(), extension_filter) || (cxx_file && is_cxx_capture_file(entry.path()))) {
                curr++;
                if (curr >= start) insert_sorted(entry_list, {entry.path().string(), (uint32_t)entry.size(), false});
            }

        } else if (fs::is_directory(entry.status())) {
            curr++;
            if (curr >= start) insert_sorted(entry_list, {entry.path().string(), 0, true});
        }
    }

    // Add "parent" directory if not at the root.
    if (!dir_path.empty() && pagination == 0)
        entry_list.insert(entry_list.begin(), {parent_dir_path.string(), 0, true});

    // add next page
    if (file_cnt > start + items_per_page) {
        entry_list.push_back({str_next, (uint32_t)pagination + 1, true});
    }

    // add prev page
    if (pagination > 0) {
        entry_list.insert(entry_list.begin(), {str_back, (uint32_t)pagination - 1, true});
    }
}

int FileManBaseView::file_count_filtered(const fs::path& directory) {
    int count{0};
    auto filtering = !extension_filter.empty();
    bool cxx_file = path_iequal(cxx_ext, extension_filter);

    for (auto& entry : std::filesystem::directory_iterator(directory, (const TCHAR*)u"*")) {
        if (fs::is_regular_file(entry.status())) {
            if (!filtering || path_iequal(entry.path().extension(), extension_filter) || (cxx_file && is_cxx_capture_file(entry.path())))
                ++count;
        } else
            ++count;
    }
    return count;
}

void FileManBaseView::load_directory_contents(const fs::path& dir_path) {
    size_t file_cnt = file_count_filtered(dir_path);
    if (file_cnt >= max_items_loaded) {
        load_directory_contents_unordered(dir_path, file_cnt);
        return;
    }
    current_path = dir_path;
    entry_list.clear();
    menu_view.clear();
    auto filtering = !extension_filter.empty();
    bool cxx_file = path_iequal(cxx_ext, extension_filter);

    text_current.set(dir_path.empty() ? "(sd root)" : truncate(dir_path, 24));

    for (const auto& entry : fs::directory_iterator(dir_path, u"*")) {
        // Hide files starting with '.' (hidden / tmp).
        if (!show_hidden_files && is_hidden_file(entry.path()))
            continue;

        if (fs::is_regular_file(entry.status())) {
            if (!filtering || path_iequal(entry.path().extension(), extension_filter) || (cxx_file && is_cxx_capture_file(entry.path())))
                insert_sorted(entry_list, {entry.path().string(), (uint32_t)entry.size(), false});
        } else if (fs::is_directory(entry.status())) {
            insert_sorted(entry_list, {entry.path().string(), 0, true});
        }
    }

    // paginating
    auto list_size = entry_list.size();
    nb_pages = 1 + (list_size / items_per_page);
    size_t start = pagination * items_per_page;
    size_t stop = start + items_per_page;
    if (list_size > start) {
        if (list_size < stop)
            stop = list_size;
        entry_list.erase(std::next(entry_list.begin(), stop), entry_list.end());
        entry_list.erase(entry_list.begin(), std::next(entry_list.begin(), start));
    }

    // Add "parent" directory if not at the root.
    if (!dir_path.empty() && pagination == 0)
        entry_list.insert(entry_list.begin(), {parent_dir_path.string(), 0, true});

    // add next page
    if (list_size > start + items_per_page) {
        entry_list.push_back({str_next, (uint32_t)pagination + 1, true});
    }

    // add prev page
    if (pagination > 0) {
        entry_list.insert(entry_list.begin(), {str_back, (uint32_t)pagination - 1, true});
    }
}

fs::path FileManBaseView::get_selected_full_path() const {
    if (get_selected_entry().path == parent_dir_path.string())
        return current_path.parent_path();

    return current_path / get_selected_entry().path;
}

const fileman_entry& FileManBaseView::get_selected_entry() const {
    // TODO: return reference to an "empty" entry on OOB?
    auto it = entry_list.begin();
    if (menu_view.highlighted_index() >= 1) std::advance(it, menu_view.highlighted_index());
    return *it;
    // return entry_list[menu_view.highlighted_index()];
}

FileManBaseView::FileManBaseView(
    NavigationView& nav,
    std::string filter)
    : nav_{nav},
      extension_filter{filter} {
    add_children({&labels,
                  &text_current,
                  &button_exit});

    button_exit.on_select = [this](Button&) {
        nav_.pop();
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
        reload_current(true);
    }
}

void FileManBaseView::pop_dir() {
    if (saved_index_stack.empty())
        return;

    current_path = current_path.parent_path();
    reload_current(true);
    menu_view.set_highlighted(saved_index_stack.back());
    saved_index_stack.pop_back();
}

std::string get_extension(std::string t) {
    const auto index = t.find_last_of(u'.');
    if (index == t.npos) {
        return {};
    } else {
        return t.substr(index);
    }
}

std::string get_stem(std::string t) {
    const auto index = t.find_last_of(u'.');
    if (index == t.npos) {
        return t;
    } else {
        return t.substr(0, index);
    }
}
std::string get_filename(std::string _s) {
    const auto index = _s.find_last_of("/");
    if (index == _s.npos) {
        return _s;
    } else {
        return _s.substr(index + 1);
    }
}
void FileManBaseView::refresh_list() {
    if (on_refresh_widgets)
        on_refresh_widgets(false);

    prev_highlight = menu_view.highlighted_index();
    menu_view.clear();

    for (const auto& entry : entry_list) {
        auto entry_name = std::string{entry.path.length() <= 20 ? entry.path : entry.path.substr(0, 20)};

        if (entry.is_directory) {
            std::string size_str{};
            if (entry.path == str_next || entry.path == str_back) {
                size_str = to_string_dec_uint(1 + entry.size) + "/" + to_string_dec_uint(nb_pages);  // show computed number of pages
            } else {
                size_str = (entry.path == parent_dir_path.string()) ? "" : to_string_dec_uint(file_count(current_path / entry.path));
            }

            menu_view.add_item(
                {entry_name.substr(0, max_filename_length) + std::string(21 - entry_name.length(), ' ') + size_str,
                 Theme::getInstance()->fg_yellow->foreground,
                 &bitmap_icon_dir,
                 [this](KeyEvent key) {
                     if (on_select_entry)
                         on_select_entry(key);
                 }});

        } else {
            const auto& assoc = get_assoc(get_extension(entry.path));
            auto size_str = to_string_file_size(entry.size);

            menu_view.add_item(
                {entry_name.substr(0, max_filename_length) + std::string(21 - entry_name.length(), ' ') + size_str,
                 assoc.color,
                 assoc.icon,
                 [this](KeyEvent key) {
                     if (on_select_entry)
                         on_select_entry(key);
                 }});
        }
    }

    menu_view.set_highlighted(prev_highlight);
}

void FileManBaseView::reload_current(bool reset_pagination) {
    if (reset_pagination) pagination = 0;
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
            if (get_selected_entry().path == str_full) {
                return;
            }
            if (get_selected_entry().path == str_back) {
                pagination--;
                menu_view.set_highlighted(0);
                reload_current(false);
                return;
            }
            if (get_selected_entry().path == str_next) {
                pagination++;
                menu_view.set_highlighted(0);
                reload_current(false);
                return;
            }
            push_dir(get_selected_entry().path);
        } else {
            if (on_changed)
                on_changed(get_selected_full_path());
            nav_.pop();
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
    button_clean.hidden(v);
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
    name_buffer = get_stem(entry.path);
    if (!hint.empty())
        name_buffer += "_" + std::string{hint};
    name_buffer += get_extension(entry.path);

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
                    reload_current(false);
                });

            if (!has_partner)
                reload_current(false);
        });
}

void FileManagerView::on_delete() {
    if (is_directory(get_selected_full_path()) && !is_empty_directory(get_selected_full_path())) {
        nav_.display_modal("Delete", " Folder is not empty;\n Use \"Clean\" button to\n empty it first.");
        return;
    }

    auto name = get_filename(get_selected_entry().path);
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
                        reload_current(true);
                    });

                if (!has_partner)
                    reload_current(true);
            }
        });
}

void FileManagerView::on_clean() {
    if (is_empty_directory(get_selected_full_path())) {
        nav_.display_modal("Clean", "Folder is Empty;\nUse \"Delete\" button instead\nof \"Clean\" button to delete\nit.");
        return;
    }

    auto path_name = is_directory(get_selected_full_path()) ? get_selected_full_path() : get_selected_full_path().parent_path();

    // selected either a single file (delete files in this directory) or a directory that is not empty (del sub files)
    nav_.push<ModalMessageView>(
        "Clean", " ALL FILES in this folder\n (excluding sub-folders)\n will be DELETED!\n\n Delete all files?", YESNO,
        [this, path_name](bool choice) {
            if (choice) {
                std::vector<std::filesystem::path> file_list;
                file_list = scan_root_files(path_name, u"*");

                for (const auto& file_name : file_list) {
                    std::filesystem::path current_full_path = path_name / file_name;
                    delete_file(current_full_path);
                }
                reload_current(true);
            }
        });
}

void FileManagerView::on_new_dir() {
    name_buffer = "";
    text_prompt(nav_, name_buffer, max_filename_length, [this](std::string& dir_name) {
        make_new_directory(current_path / dir_name);
        reload_current(true);
    });
}

void FileManagerView::on_paste() {
    // TODO: handle partner file. Need to fix nav stack first.
    auto new_name = get_unique_filename(current_path, clipboard_path.filename());
    fs::filesystem_error result;

    if (clipboard_mode == ClipboardMode::Cut)
        if ((current_path / clipboard_path.filename()) == clipboard_path)
            result = FR_OK;  // Skip paste to avoid renaming if path is unchanged
        else
            result = rename_file(clipboard_path, current_path / new_name);

    else if (clipboard_mode == ClipboardMode::Copy)
        result = copy_file(clipboard_path, current_path / new_name);

    if (result.code() != FR_OK)
        nav_.display_modal("Paste Failed", result.what());

    clipboard_path = fs::path{};
    clipboard_mode = ClipboardMode::None;
    menu_view.focus();
    reload_current(true);
}

void FileManagerView::on_new_file() {
    name_buffer = "";
    text_prompt(nav_, name_buffer, max_filename_length, [this](std::string& file_name) {
        make_new_file(current_path / file_name);
        reload_current(true);
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
        if (path_iequal(current_path, u"/" + splash_dir)) {
            nav_.push<SplashViewer>(path);  // splash, so load that viewer
        } else {
            nav_.push<BMPFileViewer>(path);  // any other bmp
        }

        reload_current(false);
        return true;
    } else if (path_iequal(rem_ext, ext)) {
        nav_.push<RemoteView>(path);
        return true;
    }

    return false;
}

bool FileManagerView::selected_is_valid() const {
    return !entry_list.empty() &&
           get_selected_entry().path != parent_dir_path.string();
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
        &button_clean,
        &button_cut,
        &button_copy,
        &button_paste,
        &button_new_dir,
        &button_new_file,
        &button_open_notepad,
        &button_rename_timestamp,
        &button_open_iq_trim,
        &button_show_hidden_files,
    });

    menu_view.on_highlight = [this]() {
        if (menu_view.highlighted_index() >= max_items_loaded - 1) {  // todo check this if correct
            text_date.set_style(Theme::getInstance()->fg_red);
            text_date.set("Too many files!");
        } else {
            text_date.set_style(Theme::getInstance()->fg_medium);
            if (selected_is_valid())
                text_date.set((is_directory(get_selected_full_path()) ? "Created " : "Modified ") + to_string_FAT_timestamp(file_created_date(get_selected_full_path())));
            else
                text_date.set("");
        }
    };

    refresh_list();

    on_select_entry = [this](KeyEvent key) {
        if (key == KeyEvent::Select) {
            if (get_selected_entry().is_directory) {
                if (get_selected_entry().path == str_full) {
                    return;
                }
                if (get_selected_entry().path == str_back) {
                    pagination--;
                    menu_view.set_highlighted(0);
                    reload_current(false);
                    return;
                }
                if (get_selected_entry().path == str_next) {
                    pagination++;
                    menu_view.set_highlighted(0);
                    reload_current(false);
                    return;
                }
                push_dir(get_selected_entry().path);
                return;
            } else if (handle_file_open()) {
                return;
            }
        }
        button_rename.focus();
    };

    button_rename.on_select = [this]() {
        if (selected_is_valid())
            on_rename("");
    };

    button_delete.on_select = [this]() {
        if (selected_is_valid())
            on_delete();
    };

    button_clean.on_select = [this]() {
        if (selected_is_valid())
            on_clean();
    };

    button_cut.on_select = [this]() {
        if (selected_is_valid()) {
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
            nav_.display_modal("Paste", "    Cut or copy a file,\n  or cut a folder, first.");
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
            nav_.push<TextEditorView>(path);
        } else
            nav_.display_modal("Open in Notepad", "Can't open that in Notepad.");
    };

    button_rename_timestamp.on_select = [this]() {
        if (selected_is_valid()) {
            on_rename(::truncate(to_string_timestamp(rtc_time::now()), 8));
        } else
            nav_.display_modal("Timestamp Rename", "Can't rename that.");
    };

    button_open_iq_trim.on_select = [this]() {
        auto path = get_selected_full_path();
        if (selected_is_valid() && !get_selected_entry().is_directory && is_cxx_capture_file(path)) {
            nav_.push<IQTrimView>(path);
        } else
            nav_.display_modal("IQ Trim", "Not a capture file.");
    };

    button_show_hidden_files.on_select = [this]() {
        show_hidden_files = !show_hidden_files;
        button_show_hidden_files.set_color(show_hidden_files ? *Theme::getInstance()->status_active : Theme::getInstance()->bg_dark->background);
        reload_current();
    };
}

}  // namespace ui
