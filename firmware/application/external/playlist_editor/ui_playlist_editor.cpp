/*
 * copyleft Elliot Alderson from F society
 * copyleft Darlene Alderson from F society
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

#include "ui_playlist_editor.hpp"
#include "ui_navigation.hpp"
#include "ui_external_items_menu_loader.hpp"

#include "file.hpp"
#include "ui_fileman.hpp"
#include "file_path.hpp"
#include "string_format.hpp"

namespace fs = std::filesystem;

#include "string_format.hpp"

#include "file_reader.hpp"

using namespace portapack;

namespace ui::external_app::playlist_editor {

/*********menu**********/
PlaylistEditorView::PlaylistEditorView(NavigationView& nav)
    : nav_{nav} {
    portapack::async_tx_enabled = true;
    add_children({&labels,
                  &button_new,
                  &text_current_ppl_file,
                  &menu_view,
                  &text_hint,
                  &button_open_playlist,
                  &button_edit,
                  &button_insert,
                  &button_save_playlist});

    menu_view.set_parent_rect({0, 2 * 8, screen_width, 24 * 8});

    menu_view.on_highlight = [this]() {
        text_hint.set("Edit:" +
                      playlist[menu_view.highlighted_index()].substr(playlist[menu_view.highlighted_index()].find_last_of('/') + 1,
                                                                     playlist[menu_view.highlighted_index()].find(',') -
                                                                         playlist[menu_view.highlighted_index()].find_last_of('/') - 1));
    };

    button_new.on_select = [this](Button&) {
        if (on_create_ppl()) {
            swap_opened_file_or_new_button(DisplayFilenameOrNewButton::DISPLAY_FILENAME);
            refresh_interface();
        }
    };

    button_open_playlist.on_select = [this](Button&) {
        open_file();
    };

    button_edit.on_select = [this](Button&) {
        on_edit_item();
    };

    button_insert.on_select = [this](Button&) {
        on_insert_item();
    };

    button_save_playlist.on_select = [this](Button&) {
        save_ppl();
    };

    swap_opened_file_or_new_button(DisplayFilenameOrNewButton::DISPLAY_NEW_BUTTON);
}

void PlaylistEditorView::focus() {
    menu_view.focus();
}

void PlaylistEditorView::open_file() {
    auto open_view = nav_.push<FileLoadView>(".PPL");
    open_view->push_dir(playlist_dir);
    open_view->on_changed = [this](fs::path new_file_path) {
        current_ppl_path = new_file_path;
        on_file_changed(new_file_path);
    };
}

void PlaylistEditorView::swap_opened_file_or_new_button(DisplayFilenameOrNewButton d) {
    if (d == DisplayFilenameOrNewButton::DISPLAY_NEW_BUTTON) {
        button_new.hidden(false);
        text_current_ppl_file.hidden(true);
    } else {
        button_new.hidden(true);
        text_current_ppl_file.hidden(false);
        text_current_ppl_file.set(current_ppl_name_buffer);
    }
    refresh_interface();
}

/*
NB: same name would became as "open file"
*/
bool PlaylistEditorView::on_create_ppl() {
    bool success = false;
    text_prompt(
        nav_,
        current_ppl_name_buffer,
        100,
        [&](std::string& s) {
            current_ppl_name_buffer = s;

            success = true;
            current_ppl_name_buffer += ".PPL";
            current_ppl_path = playlist_dir / std::filesystem::path(current_ppl_name_buffer);

            File f;
            f.open(current_ppl_path, true, true);  // prob safer here as standalone obj as read only and then open again in process func
            f.close();
            on_file_changed(current_ppl_path);
        });

    return success;
}

void PlaylistEditorView::on_file_changed(const fs::path& new_file_path) {
    File playlist_file;
    auto error = playlist_file.open(new_file_path.string());

    if (error) return;

    menu_view.clear();
    auto reader = FileLineReader(playlist_file);

    for (const auto& line : reader) {
        playlist.push_back(line);
    }

    for (auto& line : playlist) {
        // remove empty lines
        if (line == "\n" || line == "\r\n" || line == "\r") {
            playlist.erase(std::remove(playlist.begin(), playlist.end(), line), playlist.end());
        }

        // remove line end \n etc
        if (line.length() > 0 && (line[line.length() - 1] == '\n' || line[line.length() - 1] == '\r')) {
            line = line.substr(0, line.length() - 1);
        }
    }
    text_hint.set("Highlight an entry");

    text_current_ppl_file.set(new_file_path.string());

    ever_opened = true;

    swap_opened_file_or_new_button(DisplayFilenameOrNewButton::DISPLAY_FILENAME);

    refresh_menu_view();
}

void PlaylistEditorView::refresh_menu_view() {
    menu_view.clear();

    for (const auto& line : playlist) {
        if (line.length() == 0 || line[0] == '#') {
            menu_view.add_item({line,
                                ui::Color::grey(),
                                &bitmap_icon_notepad,
                                [this](KeyEvent) {
                                    button_insert.focus();
                                }});
        } else {
            const auto filename = line.substr(line.find_last_of('/') + 1, line.find(',') - line.find_last_of('/') - 1);
            menu_view.add_item({filename,
                                ui::Color::white(),
                                &bitmap_icon_cwgen,
                                [this](KeyEvent) {
                                    button_edit.focus();
                                }});
        }
    }
}

void PlaylistEditorView::on_edit_item() {
    if (!ever_opened || playlist.empty()) {
        nav_.display_modal("Err", "No entry");
        return;
    }
    auto edit_view = nav_.push<PlaylistItemEditView>(
        playlist[menu_view.highlighted_index()]);

    edit_view->set_on_delete([this]() {
        playlist.erase(playlist.begin() + menu_view.highlighted_index());
        refresh_interface();
    });

    edit_view->on_save = [this](std::string new_item) {
        playlist[menu_view.highlighted_index()] = new_item;
        refresh_interface();
    };
}

void PlaylistEditorView::on_insert_item() {
    // if (current_ppl_path.empty() || current_ppl_path.string().find_first_not_of(" \t\n\r") == std::string::npos) {
    if (!ever_opened) {  // TODO: this is a workaround because the above line is not working and I took one hour and didn't find the issue
        nav_.display_modal("Err", "No playlist file loaded");
        return;
    }

    auto edit_view = nav_.push<PlaylistItemEditView>(
        "");

    edit_view->on_save = [&](std::string new_item) {
        if (playlist.empty()) {
            playlist.push_back(new_item);
        } else {
            playlist.insert(playlist.begin() + menu_view.highlighted_index() + 1, new_item);
        }
        refresh_interface();
    };
}

void PlaylistEditorView::refresh_interface() {
    const auto previous_index = menu_view.highlighted_index();
    refresh_menu_view();
    set_dirty();
    menu_view.set_highlighted(previous_index);
}

void PlaylistEditorView::save_ppl() {
    if (current_ppl_path.empty()) {
        nav_.display_modal("Err", "No playlist file loaded");
        return;
    } else if (playlist.empty()) {
        nav_.display_modal("Err", "List is empty");
        return;
    }

    File playlist_file;
    auto error = playlist_file.open(current_ppl_path.string(), false, false);

    if (error) {
        nav_.display_modal("Err", "open err");
        return;
    }

    // clear file
    playlist_file.seek(0);
    playlist_file.truncate();

    // write new data
    for (const auto& entry : playlist) {
        playlist_file.write_line(entry);
    }

    nav_.display_modal("Save", "Saved playlist\n" + current_ppl_path.string());
}

/*********edit**********/

PlaylistItemEditView::PlaylistItemEditView(
    NavigationView& nav,
    std::string item)
    : nav_{nav},
      original_item_{item} {
    add_children({&labels,
                  &field_path,
                  &field_delay,
                  &button_browse,
                  &button_input_delay,
                  &button_delete,
                  &button_save});

    button_browse.on_select = [this, &nav](Button&) {
        auto open_view = nav.push<FileLoadView>(".C16");
        open_view->push_dir(captures_dir);
        open_view->on_changed = [this](fs::path path) {
            field_path.set_text(path.string());
            path_ = path.string();
        };
        field_delay.on_change = [&](auto) {
            delay_ = field_delay.value();
        };
    };

    button_input_delay.on_select = [this](Button&) {
        delay_str = to_string_dec_uint(delay_);
        if (delay_str == "0") {
            delay_str = "";
        }
        text_prompt(
            nav_,
            delay_str,
            100,
            [&](std::string& s) {
                delay_ = atoi(s.c_str());
                field_delay.set_value(delay_);
                refresh_ui();
            });
    };

    button_delete.on_select = [this](Button&) {
        if (on_delete) on_delete();
        nav_.pop();
    };

    button_save.on_select = [&](Button&) {
        if (path_.empty()) {
            nav_.display_modal("Err", "Select a file\n or press back to cancel");
            return;
        }
        if (on_save) on_save(build_item());
        nav_.pop();
    };

    if (!on_delete) {
        button_delete.hidden(true);
    }

    parse_item(item);
    refresh_ui();
}

void PlaylistItemEditView::focus() {
    button_save.focus();
}

void PlaylistItemEditView::refresh_ui() {
    field_path.set_text(path_);
    field_delay.set_value(delay_);
}

void PlaylistItemEditView::parse_item(std::string item) {
    // Parse format: path,delay
    if (item.empty()) {
        return;
    }
    auto parts = split_string(item, ',');
    if (parts.size() >= 1) {
        path_ = std::string{parts[0]};
    }
    if (parts.size() >= 2) {
        delay_ = atoi(std::string{parts[1]}.c_str());
    }
}

std::string PlaylistItemEditView::build_item() const {
    const auto v = path_ + "," + to_string_dec_uint(field_delay.value());
    return path_ + "," + to_string_dec_uint(field_delay.value());
}

}  // namespace ui::external_app::playlist_editor