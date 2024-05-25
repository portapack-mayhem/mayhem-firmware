/*
 * Copyright (C) 2023 Kyle Reed
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

#include "ui_remote.hpp"

#include "binder.hpp"
#include "convert.hpp"
#include "file_reader.hpp"
#include "io_convert.hpp"
#include "irq_controls.hpp"
#include "oversample.hpp"
#include "string_format.hpp"
#include "ui_fileman.hpp"
#include "ui_receiver.hpp"
#include "ui_textentry.hpp"
#include "utility.hpp"
#include "file_path.hpp"

using namespace portapack;
namespace fs = std::filesystem;

namespace ui {

static constexpr uint8_t text_edit_max = 30;

/* RemoteEntryModel **************************************/

std::string RemoteEntryModel::to_string() const {
    return join(',', {path.string(),
                      name,
                      to_string_dec_uint(icon),
                      to_string_dec_uint(bg_color),
                      to_string_dec_uint(fg_color),
                      to_string_dec_uint(metadata.center_frequency),
                      to_string_dec_uint(metadata.sample_rate)});
}

Optional<RemoteEntryModel> RemoteEntryModel::parse(std::string_view line) {
    auto cols = split_string(line, ',');

    if (cols.size() < 7)
        return {};

    RemoteEntryModel entry{};

    entry.path = cols[0];
    entry.name = std::string{cols[1]};
    parse_int(cols[2], entry.icon);
    parse_int(cols[3], entry.bg_color);
    parse_int(cols[4], entry.fg_color);
    parse_int(cols[5], entry.metadata.center_frequency);
    parse_int(cols[6], entry.metadata.sample_rate);

    return entry;
}

/* RemoteModel *******************************************/

bool RemoteModel::delete_entry(const RemoteEntryModel* entry) {
    // NB: expecting 'entry' to be a pointer to an entry in vector.
    auto it = std::find_if(
        entries.begin(), entries.end(),
        [entry](auto& item) { return entry == &item; });
    if (it == entries.end())
        return false;

    entries.erase(it);
    return true;
}

bool RemoteModel::load(const std::filesystem::path& path) {
    File f;
    auto error = f.open(path);
    if (error)
        return false;

    entries.clear();

    bool first = true;
    auto reader = FileLineReader(f);
    for (const auto& line : reader) {
        if (line.length() == 0 || line[0] == '#')
            continue;  // Empty or comment line.

        // First line is the "name" field.
        if (first) {
            name = trim(line);
            first = false;
            continue;
        }

        // All the other lines are button entries.
        auto entry = RemoteEntryModel::parse(line);
        if (entry)
            entries.push_back(*std::move(entry));
    }

    return true;
}

bool RemoteModel::save(const std::filesystem::path& path) {
    File f;
    auto error = f.create(path);
    if (error)
        return false;

    f.write_line(name);
    for (auto& entry : entries)
        f.write_line(entry.to_string());

    return true;
}

/* RemoteButton ******************************************/

RemoteButton::RemoteButton(Rect parent_rect, RemoteEntryModel* entry)
    : NewButton{parent_rect, {}, nullptr},
      entry_{nullptr} {
    set_entry(entry);
    // Forward to on_select2 -- this isn't ideal, but works for now.
    on_select = [this]() {
        if (on_select2)
            on_select2(*this);
    };
}

void RemoteButton::on_focus() {
    // Enable long press on "Select".
    SwitchesState config;
    config[toUType(Switch::Sel)] = true;
    set_switches_long_press_config(config);
}

void RemoteButton::on_blur() {
    // Reset long press.
    SwitchesState config{};
    set_switches_long_press_config(config);
}

bool RemoteButton::on_key(KeyEvent key) {
    if (key == KeyEvent::Select) {
        if (key_is_long_pressed(key) && on_long_select) {
            on_long_select(*this);
            return true;
        }

        if (on_select2) {
            on_select2(*this);
            return true;
        }
    }

    return false;
}

void RemoteButton::paint(Painter& painter) {
    NewButton::paint(painter);

    // Add a border on the highlighted button.
    if (has_focus() || highlighted()) {
        auto r = screen_rect();
        painter.draw_rectangle(r, Theme::getInstance()->bg_darkest->foreground);

        auto p = r.location() + Point{1, 1};
        auto s = Size{r.size().width() - 2, r.size().height() - 2};
        painter.draw_rectangle({p, s}, Theme::getInstance()->fg_light->foreground);
    }
};

RemoteEntryModel* RemoteButton::entry() {
    return entry_;
}

void RemoteButton::set_entry(RemoteEntryModel* entry) {
    entry_ = entry;
    set_focusable(entry_ != nullptr);
    hidden(entry_ == nullptr);

    if (entry_) {
        set_text(entry_->name);
        set_bitmap(RemoteIcons::get(entry_->icon));
    }

    set_dirty();
}

Style RemoteButton::paint_style() {
    if (!entry_)
        return style();

    MutableStyle s{style()};
    s.foreground = RemoteColors::get(entry_->fg_color);
    s.background = RemoteColors::get(entry_->bg_color);

    if (has_focus() || highlighted())
        s.invert();

    // It's kind of a hack to set 'color_' here, but the base
    // class' paint logic is kind of convoluted but isn't worth
    // the extra bytes to copy and paste a paint override.
    color_ = s.foreground;

    return s;
}

/* RemoteEntryEditView ***********************************/

RemoteEntryEditView::RemoteEntryEditView(
    NavigationView& nav,
    RemoteEntryModel& entry)
    : entry_{entry} {
    add_children({
        &labels,
        &field_name,
        &field_path,
        &field_freq,
        &text_rate,
        &field_icon_index,
        &field_fg_color_index,
        &field_bg_color_index,
        &button_preview,
        &button_delete,
        &button_done,
    });

    bind(field_name, entry_.name, nav, [this](auto& v) {
        button_preview.set_text(v);
    });

    field_path.on_select = [this, &nav](TextField&) {
        auto open_view = nav.push<FileLoadView>(".C*");
        open_view->push_dir(captures_dir);
        open_view->on_changed = [this](fs::path path) {
            load_path(std::move(path));
            refresh_ui();
        };
    };

    bind(field_freq, entry_.metadata.center_frequency, nav);
    bind(field_icon_index, entry_.icon, [this](auto v) {
        button_preview.set_bitmap(RemoteIcons::get(v));
    });
    bind(field_fg_color_index, entry_.fg_color, [this](auto v) {
        button_preview.set_color(RemoteColors::get(v));
    });
    bind(field_bg_color_index, entry_.bg_color, [this](auto) {
        button_preview.set_dirty();
    });

    button_delete.on_select = [this, &nav]() {
        nav.display_modal(
            "Delete?", "    Delete this button?", YESNO,
            [this, &nav](bool choice) {
                if (choice) {
                    if (on_delete)
                        on_delete(entry_);

                    // Exit the edit view upon delete.
                    nav.set_on_pop([&nav]() { nav.pop(); });
                }
            });
    };

    button_done.on_select = [&nav](Button&) {
        nav.pop();
    };

    refresh_ui();
}

void RemoteEntryEditView::focus() {
    button_done.focus();
}

void RemoteEntryEditView::refresh_ui() {
    field_path.set_text(entry_.path.filename().string());
    field_freq.set_value(entry_.metadata.center_frequency);
    text_rate.set(unit_auto_scale(entry_.metadata.sample_rate, 3, 0) + "Hz");
}

void RemoteEntryEditView::load_path(std::filesystem::path&& path) {
    // Read metafile if it exists.
    auto metadata_path = get_metadata_path(path);
    auto metadata = read_metadata_file(metadata_path);
    entry_.path = std::move(path);

    // Use metadata if found, otherwise fallback to the TX frequency.
    if (metadata)
        entry_.metadata = *metadata;
    else
        entry_.metadata = {transmitter_model.target_frequency(), 500'000};
}

/* RemoteView ********************************************/

RemoteView::RemoteView(
    NavigationView& nav)
    : nav_{nav} {
    baseband::run_image(portapack::spi_flash::image_tag_replay);

    add_children({
        &field_title,
        &tx_view,
        &check_loop,
        &field_filename,
        &button_add,
        &button_new,
        &button_open,
        &waterfall,
    });

    create_buttons();

    field_title.on_select = [this, &nav](TextField&) {
        temp_buffer_ = model_.name;
        text_prompt(nav_, temp_buffer_, text_edit_max, [this](std::string& new_name) {
            model_.name = new_name;
            refresh_ui();
            set_needs_save();
        });
    };

    field_filename.on_select = [this, &nav](TextField&) {
        temp_buffer_ = remote_path_.stem().string();
        text_prompt(nav_, temp_buffer_, text_edit_max, [this](std::string& new_name) {
            rename_remote(new_name);
            refresh_ui();
        });
    };

    button_add.on_select = [this]() { add_button(); };
    button_new.on_select = [this]() { new_remote(); };
    button_open.on_select = [this]() { open_remote(); };

    // Fill in the area between the remote buttons and bottom UI with waterfall.
    Dim waterfall_top = buttons_top_.y() + button_area_height;
    Dim waterfall_bottom = button_add.parent_rect().top();
    Dim waterfall_height = waterfall_bottom - waterfall_top;
    waterfall.set_parent_rect({0, waterfall_top, screen_width, waterfall_height});

    ensure_directory(remotes_dir);

    // Load the previously loaded remote if exists.
    if (!load_remote(settings_.remote_path))
        init_remote();

    refresh_ui();
}

RemoteView::RemoteView(NavigationView& nav, fs::path path)
    : RemoteView(nav) {
    load_remote(std::move(path));
    refresh_ui();
}

RemoteView::~RemoteView() {
    stop();
    baseband::shutdown();

    save_remote(/*show_error*/ false);
}

void RemoteView::focus() {
    if (model_.entries.empty())
        button_add.focus();
    else
        buttons_[0]->focus();
}

void RemoteView::create_buttons() {
    // Handler callbacks.
    auto handle_send = [this](RemoteButton& btn) {
        if (btn.entry()->path.empty())
            // No path set? Go to edit mode instead.
            edit_button(btn);
        else if (is_sending() && &btn == current_btn_)
            // Pressed the same button again? Stop.
            stop();
        else
            // Start sending.
            send_button(btn);
    };

    auto handle_edit = [this](RemoteButton& btn) {
        edit_button(btn);
    };

    // Create and add RemoteButtons for the whole grid.
    for (size_t i = 0; i < max_buttons; ++i) {
        Coord x = i % button_cols;
        Coord y = i / button_cols;
        Point pos = Point{x * button_width, y * button_height} + buttons_top_;

        auto btn = std::make_unique<RemoteButton>(
            Rect{pos, {button_width, button_height}},
            nullptr);
        btn->on_select2 = handle_send;
        btn->on_long_select = handle_edit;

        add_child(btn.get());
        buttons_.push_back(std::move(btn));
    }
}

void RemoteView::reset_buttons() {
    // Whever the model's entries instance is invalidated,
    // all the pointers in the buttons will end up dangling.
    // TODO: This is pretty lame. Could maybe static alloc?
    for (auto& btn : buttons_)
        btn->set_entry(nullptr);
}

void RemoteView::refresh_ui() {
    field_title.set_text(model_.name);
    field_filename.set_text(remote_path_.stem().string());

    // Update buttons from the model.
    for (size_t i = 0; i < buttons_.size(); ++i) {
        if (i < model_.entries.size())
            buttons_[i]->set_entry(&model_.entries[i]);
        else
            buttons_[i]->set_entry(nullptr);
    }
}

void RemoteView::add_button() {
    if (model_.entries.size() >= max_buttons)
        return;

    // Don't let replay thread read the model while editing.
    stop();

    model_.entries.push_back({{}, "<EMPTY>", 0, 3, 1});
    reset_buttons();
    refresh_ui();
    set_needs_save();
}

void RemoteView::edit_button(RemoteButton& btn) {
    // Don't let replay thread read the model while editing.
    stop();

    auto edit_view = nav_.push<RemoteEntryEditView>(*btn.entry());
    nav_.set_on_pop([this]() {
        refresh_ui();
        set_needs_save();
        focus();  // Need to refocus after refreshing the buttons.
    });

    edit_view->on_delete = [this](RemoteEntryModel& to_delete) {
        model_.delete_entry(&to_delete);
        reset_buttons();
    };
}

void RemoteView::send_button(RemoteButton& btn) {
    // TODO: If this is called while is_sending() == true,
    // it just stops and doesn't start the new button?

    // Reset everything to prepare to send a file.
    stop();
    current_btn_ = &btn;  // Stash for looping.

    // Open the sample file to send.
    auto reader = std::make_unique<FileConvertReader>();
    auto error = reader->open(btn.entry()->path);
    if (error) {
        show_error("Can't open file:\n" + btn.entry()->path.stem().string());
        return;
    }

    // Update the sample rate in proc_replay baseband.
    baseband::set_sample_rate(
        btn.entry()->metadata.sample_rate,
        get_oversample_rate(btn.entry()->metadata.sample_rate));

    // ReplayThread starts immediately on construction; must be set before creating.
    transmitter_model.set_target_frequency(btn.entry()->metadata.center_frequency);
    transmitter_model.set_sampling_rate(get_actual_sample_rate(btn.entry()->metadata.sample_rate));
    transmitter_model.set_baseband_bandwidth(baseband_bandwidth);
    transmitter_model.enable();

    // ReplayThread reads the file and sends to the baseband.
    replay_thread_ = std::make_unique<ReplayThread>(
        std::move(reader),
        /* read_size */ 0x4000,
        /* buffer_count */ 3,
        &ready_signal_,
        [](uint32_t return_code) {
            ReplayThreadDoneMessage message{return_code};
            EventDispatcher::send_message(message);
        });
}

void RemoteView::stop() {
    // This terminates the underlying chThread.
    replay_thread_.reset();
    transmitter_model.disable();
    ready_signal_ = false;
}

void RemoteView::new_remote() {
    save_remote();
    init_remote();
    refresh_ui();

    // View needs to redraw to hide old buttons.
    set_dirty();
}

void RemoteView::open_remote() {
    auto open_view = nav_.push<FileLoadView>(".REM");
    open_view->push_dir(remotes_dir);
    open_view->on_changed = [this](fs::path path) {
        save_remote();
        load_remote(std::move(path));
        refresh_ui();
    };
}

void RemoteView::init_remote() {
    model_ = {"<Unnamed Remote>", {}};
    reset_buttons();
    set_remote_path(next_filename_matching_pattern(remotes_dir / u"REMOTE_????.REM"));
    set_needs_save(false);

    if (remote_path_.empty())
        show_error("Couldn't make new remote file.");
}

bool RemoteView::load_remote(fs::path&& path) {
    set_remote_path(std::move(path));
    set_needs_save(false);
    reset_buttons();
    return model_.load(remote_path_);
}

void RemoteView::save_remote(bool show_errors) {
    if (!needs_save_)
        return;

    bool ok = model_.save(remote_path_);
    if (!ok && show_errors)
        show_error("Save failed for:\n" + remote_path_.stem().string());

    set_needs_save(false);
}

void RemoteView::rename_remote(const std::string& new_name) {
    auto folder = remote_path_.parent_path();
    auto ext = remote_path_.extension();
    auto new_path = folder / new_name + ext;

    if (file_exists(new_path)) {
        show_error("Remote " + new_name + " already exists");
        return;
    }

    // Rename file if there is one.
    if (fs::file_exists(remote_path_))
        rename_file(remote_path_, new_path);

    set_remote_path(std::move(new_path));
}

void RemoteView::handle_replay_thread_done(uint32_t return_code) {
    if (return_code == ReplayThread::END_OF_FILE) {
        if (check_loop.value() && current_btn_) {
            send_button(*current_btn_);
            return;
        }
    }

    /*
    // TODO: This can happen when stopping an in-progress Tx.
    if (return_code == ReplayThread::READ_ERROR)
        show_error("Bad capture file.");
    */

    stop();
}

void RemoteView::set_remote_path(fs::path&& path) {
    // Unfortunately, have to keep these two in sync because
    // settings doesn't know about fs::path.
    remote_path_ = std::move(path);
    settings_.remote_path = remote_path_.string();
}

void RemoteView::show_error(const std::string& msg) const {
    nav_.display_modal("Error", msg);
}

} /* namespace ui */
