/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
 * Copyright (C) 2023 gullradriel, Nilorea Studio Inc.
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
#include "ui_navigation.hpp"
#include "ui_recon_settings.hpp"
#include "ui_textentry.hpp"

#include "file.hpp"
#include "freqman_db.hpp"
#include "portapack.hpp"
#include "portapack_persistent_memory.hpp"
#include "file_path.hpp"

using namespace std;
using namespace portapack;

namespace ui {

ReconSetupViewMain::ReconSetupViewMain(NavigationView& nav, Rect parent_rect, std::string input_file, std::string output_file)
    : View(parent_rect), _input_file{input_file}, _output_file{output_file} {
    hidden(true);
    add_children({&button_input_file,
                  &text_input_file,
                  &button_choose_output_file,
                  &button_choose_output_name,
                  &checkbox_autosave_freqs,
                  &checkbox_autostart_recon,
                  &checkbox_clear_output});

    checkbox_autosave_freqs.set_value(persistent_memory::recon_autosave_freqs());
    checkbox_autostart_recon.set_value(persistent_memory::recon_autostart_recon());
    checkbox_clear_output.set_value(persistent_memory::recon_clear_output());

    text_input_file.set(_input_file);
    button_choose_output_name.set_text(_output_file);

    button_input_file.on_select = [this, &nav](Button&) {
        auto open_view = nav.push<FileLoadView>(".TXT");
        open_view->push_dir(freqman_dir);
        open_view->on_changed = [this, &nav](std::filesystem::path new_file_path) {
            if (new_file_path.native().find((u"/" / freqman_dir).native()) == 0) {
                _input_file = new_file_path.stem().string();
                text_input_file.set(_input_file);
            } else {
                nav.display_modal("LOAD ERROR", "A valid file from\nFREQMAN directory is\nrequired.");
            }
        };
    };

    button_choose_output_file.on_select = [this, &nav](Button&) {
        auto open_view = nav.push<FileLoadView>(".TXT");
        open_view->push_dir(freqman_dir);
        open_view->on_changed = [this, &nav](std::filesystem::path new_file_path) {
            if (new_file_path.native().find((u"/" / freqman_dir).native()) == 0) {
                _output_file = new_file_path.stem().string();
                button_choose_output_name.set_text(_output_file);
            } else {
                nav.display_modal("SAVE ERROR", "A valid file from\nFREQMAN directory is\nrequired.");
            }
        };
    };

    button_choose_output_name.on_select = [this, &nav](Button&) {
        text_prompt(nav, _output_file, 28, [this](std::string& buffer) {
            _output_file = buffer;
            button_choose_output_name.set_text(_output_file);
        });
    };
};

void ReconSetupViewMain::save(std::string& input_file, std::string& output_file) {
    persistent_memory::set_recon_autosave_freqs(checkbox_autosave_freqs.value());
    persistent_memory::set_recon_autostart_recon(checkbox_autostart_recon.value());
    persistent_memory::set_recon_clear_output(checkbox_clear_output.value());
    input_file = _input_file;
    output_file = _output_file;
};
void ReconSetupViewMore::save() {
    persistent_memory::set_recon_load_freqs(checkbox_load_freqs.value());
    persistent_memory::set_recon_load_ranges(checkbox_load_ranges.value());
    persistent_memory::set_recon_load_hamradios(checkbox_load_hamradios.value());
    persistent_memory::set_recon_load_repeaters(checkbox_load_repeaters.value());
    persistent_memory::set_recon_update_ranges_when_recon(checkbox_update_ranges_when_recon.value());
    persistent_memory::set_recon_auto_record_locked(checkbox_auto_record_locked.value());
    persistent_memory::set_recon_repeat_recorded(checkbox_repeat_recorded.value());
    persistent_memory::set_recon_repeat_recorded_file_mode(field_repeat_file_mode.selected_index_value());
    persistent_memory::set_recon_repeat_nb(field_repeat_nb.value());
    persistent_memory::set_recon_repeat_amp(checkbox_repeat_amp.value());
    persistent_memory::set_recon_repeat_gain(field_repeat_gain.value());
    persistent_memory::set_recon_repeat_delay(field_repeat_delay.value());
};

void ReconSetupViewMain::focus() {
    button_input_file.focus();
}

ReconSetupViewMore::ReconSetupViewMore(NavigationView& nav, Rect parent_rect)
    : View(parent_rect) {
    (void)nav;
    hidden(true);

    add_children({&checkbox_load_freqs,
                  &checkbox_load_repeaters,
                  &checkbox_load_ranges,
                  &checkbox_load_hamradios,
                  &checkbox_update_ranges_when_recon,
                  &checkbox_auto_record_locked,
                  &checkbox_repeat_recorded,
                  &field_repeat_file_mode,
                  &text_repeat_nb,
                  &field_repeat_nb,
                  &checkbox_repeat_amp,
                  &text_repeat_gain,
                  &field_repeat_gain,
                  &text_repeat_delay,
                  &field_repeat_delay});

    // tx options have to be in yellow to inform the users that activating them will make the device transmit
    checkbox_repeat_recorded.set_style(Theme::getInstance()->fg_yellow);
    field_repeat_file_mode.set_style(Theme::getInstance()->fg_yellow);
    text_repeat_nb.set_style(Theme::getInstance()->fg_yellow);
    field_repeat_nb.set_style(Theme::getInstance()->fg_yellow);
    checkbox_repeat_amp.set_style(Theme::getInstance()->fg_yellow);
    text_repeat_gain.set_style(Theme::getInstance()->fg_yellow);
    field_repeat_gain.set_style(Theme::getInstance()->fg_yellow);
    text_repeat_delay.set_style(Theme::getInstance()->fg_yellow);
    field_repeat_delay.set_style(Theme::getInstance()->fg_yellow);

    checkbox_load_freqs.set_value(persistent_memory::recon_load_freqs());
    checkbox_load_repeaters.set_value(persistent_memory::recon_load_repeaters());
    checkbox_load_ranges.set_value(persistent_memory::recon_load_ranges());
    checkbox_load_hamradios.set_value(persistent_memory::recon_load_hamradios());
    checkbox_update_ranges_when_recon.set_value(persistent_memory::recon_update_ranges_when_recon());
    checkbox_auto_record_locked.set_value(persistent_memory::recon_auto_record_locked());
    checkbox_repeat_recorded.set_value(persistent_memory::recon_repeat_recorded());
    field_repeat_file_mode.set_selected_index(persistent_memory::recon_repeat_recorded_file_mode());
    checkbox_repeat_amp.set_value(persistent_memory::recon_repeat_amp());
    field_repeat_nb.set_value(persistent_memory::recon_repeat_nb());
    field_repeat_gain.set_value(persistent_memory::recon_repeat_gain());
    field_repeat_delay.set_value(persistent_memory::recon_repeat_delay());

    // tx warning modal
    checkbox_repeat_recorded.on_select = [this, &nav](Checkbox&, bool v) {
        if (v) {
            nav.display_modal(
                "TX WARNING",
                "This activate TX ability\nin Recon !",
                YESNO,
                [this, &nav](bool choice) {
                    if (!choice)
                        checkbox_repeat_recorded.set_value(choice);
                });
        }
    };
};

void ReconSetupViewMore::focus() {
    checkbox_load_freqs.focus();
}

void ReconSetupView::focus() {
    viewMain.focus();
}

ReconSetupView::ReconSetupView(NavigationView& nav, std::string _input_file, std::string _output_file)
    : nav_{nav}, input_file{_input_file}, output_file{_output_file} {
    add_children({&tab_view,
                  &viewMain,
                  &viewMore,
                  &button_save});

    button_save.on_select = [this, &nav](Button&) {
        viewMain.save(input_file, output_file);
        viewMore.save();
        std::vector<std::string> messages;
        messages.push_back(input_file);
        messages.push_back(output_file);
        on_changed(messages);
        nav.pop();
    };
}
} /* namespace ui */
