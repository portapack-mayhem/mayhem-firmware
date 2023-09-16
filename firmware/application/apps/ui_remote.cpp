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

#include "irq_controls.hpp"
#include "string_format.hpp"
#include "ui_textentry.hpp"

using namespace portapack;

namespace ui {

/* RemoteButton *******************************************/

RemoteButton::RemoteButton(Rect parent_rect, RemoteEntryModel& entry)
    : NewButton{
          parent_rect,
          entry.name,
          RemoteIcons::get(entry.icon),
          RemoteColors::get(entry.fg_color)},
      entry_{entry} {
}

void RemoteButton::on_focus() {
    // Enable long press on "Select".
    SwitchesState config;
    config[toUType(Switch::Sel)] = true;
    set_switches_long_press_config(config);
}

void RemoteButton::on_blur() {
    // Reset long press
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

RemoteEntryModel& RemoteButton::entry() {
    return entry_;
}

Style RemoteButton::paint_style() {
    MutableStyle s{style()};
    s.foreground = RemoteColors::get(entry_.fg_color);
    s.background = RemoteColors::get(entry_.bg_color);

    if (has_focus() || highlighted())
        s.invert();

    // It's kind of a hack to set 'color_' here, but the base
    // class' paint logic is kind of convoluted but isn't worth
    // the extra bytes to copy and paste a paint override.
    color_ = s.foreground;

    return s;
}

/* RemoteEntryEditView ************************************/

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

    // TODO: It's time to make field bindings and clean this mess up.
    field_name.on_change = [this](TextField& tf) {
        entry_.name = tf.get_text();
        button_preview.set_text(entry_.name);
    };
    field_name.on_select = [this, &nav](TextField& tf) {
        temp_buffer_ = tf.get_text();
        text_prompt(nav, temp_buffer_, text_edit_max,
                    [this, &tf](std::string& str) {
                        tf.set_text(str);
                    });
    };
    field_name.set_text(entry_.name);

    field_path.set_text(entry_.path.string());
    field_freq.set_value(entry_.metadata.center_frequency);
    text_rate.set(unit_auto_scale(entry_.metadata.sample_rate, 3, 0) + "Hz");

    field_icon_index.on_change = [this](int32_t v) {
        entry_.icon = v;
        button_preview.set_bitmap(RemoteIcons::get(v));
    };
    field_icon_index.set_value(entry.icon);

    field_fg_color_index.on_change = [this](int32_t v) {
        entry_.fg_color = v;
        button_preview.set_color(RemoteColors::get(v));
    };
    field_fg_color_index.set_value(entry_.fg_color);

    field_bg_color_index.on_change = [this](int32_t v) {
        entry_.bg_color = v;
        button_preview.set_dirty();
    };
    field_bg_color_index.set_value(entry_.bg_color);

    button_delete.on_select = [this, &nav](Button&) {
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
}

void RemoteEntryEditView::focus() {
    button_done.focus();
}

/* RemoteView *********************************************/

RemoteView::RemoteView(
    NavigationView& nav)
    : nav_{nav} {
    add_children({
        &button_edit,
        &button_add,
        &button_delete,
        &button_open,
        &button_save,
    });

    load_test();
    refresh_ui();
}

RemoteView::~RemoteView() {
}

void RemoteView::focus() {
    if (buttons_.empty())
        button_add.focus();
    else
        buttons_[0]->focus();
}

void RemoteView::refresh_ui() {
    // Delete exising buttons.
    for (auto& btn : buttons_)
        remove_child(btn.get());

    buttons_.clear();

    auto handle_send = [this](RemoteButton&) {
        nav_.display_modal("Send", "Sending");
    };

    auto handle_edit = [this](RemoteButton& btn) {
        auto edit_view = nav_.push<RemoteEntryEditView>(btn.entry());
        nav_.set_on_pop([this]() { refresh_ui(); });

        edit_view->on_delete = [this](RemoteEntryModel& to_delete) {
            model_.delete_entry(&to_delete);
        };
    };

    // Add new buttons from model.
    for (auto& entry : model_.entries) {
        Coord x = buttons_.size() % button_cols;
        Coord y = buttons_.size() / button_cols;

        auto btn = std::make_unique<RemoteButton>(
            Rect{x * button_width, y * button_height, button_width, button_height},
            entry);
        btn->on_select2 = handle_send;
        btn->on_long_select = handle_edit;

        add_child(btn.get());
        buttons_.push_back(std::move(btn));
    }
}

void RemoteView::load_test() {
    model_.name = "Cool Remote!";
    model_.entries = {
        RemoteEntryModel{{}, "Lamp On", 3, 1, 2},
        RemoteEntryModel{{}, "Lamp Off", 8, 9, 1},
        RemoteEntryModel{{}, "Fan Hi", 10, 2, 12},
    };
}

} /* namespace ui */
