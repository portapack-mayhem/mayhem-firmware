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

#include "ui_recon_settings.hpp"
#include "ui_navigation.hpp"
#include "ui_fileman.hpp"
#include "ui_textentry.hpp"

#include "file.hpp"
#include "portapack.hpp"
#include "portapack_persistent_memory.hpp"

using namespace std;
using namespace portapack;

namespace ui {

ReconSetupViewMain::ReconSetupViewMain(NavigationView& nav, Rect parent_rect, std::string input_file, std::string output_file)
    : View(parent_rect), _input_file{input_file}, _output_file{output_file} {
    hidden(true);
    add_children({&button_load_freqs,
                  &text_input_file,
                  &button_save_freqs,
                  &button_output_file,
                  &checkbox_autosave_freqs,
                  &checkbox_autostart_recon,
                  &checkbox_continuous,
                  &checkbox_clear_output});

    checkbox_autosave_freqs.set_value(persistent_memory::recon_autosave_freqs());
    checkbox_autostart_recon.set_value(persistent_memory::recon_autostart_recon());
    checkbox_continuous.set_value(persistent_memory::recon_continuous());
    checkbox_clear_output.set_value(persistent_memory::recon_clear_output());

    text_input_file.set(_input_file);
    button_output_file.set_text(_output_file);

    button_load_freqs.on_select = [this, &nav](Button&) {
        auto open_view = nav.push<FileLoadView>(".TXT");
        open_view->on_changed = [this, &nav](std::filesystem::path new_file_path) {
            std::string dir_filter = "FREQMAN/";
            std::string str_file_path = new_file_path.string();
            if (str_file_path.find(dir_filter) != string::npos) {  // assert file from the FREQMAN folder
                                                                   // get the filename without txt extension so we can use load_freqman_file fcn
                _input_file = new_file_path.stem().string();
                text_input_file.set(_input_file);
            } else {
                nav.display_modal("LOAD ERROR", "A valid file from\nFREQMAN directory is\nrequired.");
            }
        };
    };

    button_save_freqs.on_select = [this, &nav](Button&) {
        auto open_view = nav.push<FileLoadView>(".TXT");
        open_view->on_changed = [this, &nav](std::filesystem::path new_file_path) {
            std::string dir_filter = "FREQMAN/";
            std::string str_file_path = new_file_path.string();
            if (str_file_path.find(dir_filter) != string::npos) {  // assert file from the FREQMAN folder
                _output_file = new_file_path.stem().string();
                button_output_file.set_text(_output_file);
            } else {
                nav.display_modal("LOAD ERROR", "A valid file from\nFREQMAN directory is\nrequired.");
            }
        };
    };

    button_output_file.on_select = [this, &nav](Button&) {
        text_prompt(nav, _output_file, 28,
                    [this](std::string& buffer) {
                        _output_file = buffer;
                        button_output_file.set_text(_output_file);
                    });
    };
};

void ReconSetupViewMain::save(std::string& input_file, std::string& output_file) {
    persistent_memory::set_recon_autosave_freqs(checkbox_autosave_freqs.value());
    persistent_memory::set_recon_autostart_recon(checkbox_autostart_recon.value());
    persistent_memory::set_recon_continuous(checkbox_continuous.value());
    persistent_memory::set_recon_clear_output(checkbox_clear_output.value());
    input_file = _input_file;
    output_file = _output_file;
};
void ReconSetupViewMore::save(uint32_t& recon_lock_duration, uint32_t& recon_lock_nb_match, uint32_t& recon_match_mode) {
    persistent_memory::set_recon_load_freqs(checkbox_load_freqs.value());
    persistent_memory::set_recon_load_ranges(checkbox_load_ranges.value());
    persistent_memory::set_recon_load_hamradios(checkbox_load_hamradios.value());
    persistent_memory::set_recon_update_ranges_when_recon(checkbox_update_ranges_when_recon.value());
    recon_lock_duration = field_recon_lock_duration.value();
    recon_lock_nb_match = field_recon_lock_nb_match.value();
    recon_match_mode = field_recon_match_mode.selected_index_value();
};

void ReconSetupViewMain::focus() {
    button_load_freqs.focus();
}

ReconSetupViewMore::ReconSetupViewMore(NavigationView& nav, Rect parent_rect, uint32_t recon_lock_duration, uint32_t recon_lock_nb_match, uint32_t recon_match_mode)
    : View(parent_rect), _recon_lock_duration{recon_lock_duration}, _recon_lock_nb_match{recon_lock_nb_match}, _recon_match_mode{recon_match_mode} {
    (void)nav;
    hidden(true);

    add_children({
        &checkbox_load_freqs,
        &checkbox_load_ranges,
        &checkbox_load_hamradios,
        &checkbox_update_ranges_when_recon,
        &text_recon_lock_duration,
        &field_recon_lock_duration,
        &text_recon_lock_nb,
        &field_recon_lock_nb_match,
        &field_recon_match_mode,
    });

    checkbox_load_freqs.set_value(persistent_memory::recon_load_freqs());
    checkbox_load_ranges.set_value(persistent_memory::recon_load_ranges());
    checkbox_load_hamradios.set_value(persistent_memory::recon_load_hamradios());
    checkbox_update_ranges_when_recon.set_value(persistent_memory::recon_update_ranges_when_recon());
    field_recon_lock_duration.set_value(_recon_lock_duration);
    field_recon_lock_nb_match.set_value(_recon_lock_nb_match);
    field_recon_match_mode.set_by_value(_recon_match_mode);
};

void ReconSetupViewMore::focus() {
    checkbox_load_freqs.focus();
}

void ReconSetupView::focus() {
    viewMain.focus();
}

ReconSetupView::ReconSetupView(
    NavigationView& nav,
    std::string _input_file,
    std::string _output_file,
    uint32_t _recon_lock_duration,
    uint32_t _recon_lock_nb_match,
    uint32_t _recon_match_mode)
    : nav_{nav}, input_file{_input_file}, output_file{_output_file}, recon_lock_duration{_recon_lock_duration}, recon_lock_nb_match{_recon_lock_nb_match}, recon_match_mode{_recon_match_mode} {
    add_children({&tab_view,
                  &viewMain,
                  &viewMore,
                  &button_save});

    button_save.on_select = [this, &nav](Button&) {
        viewMain.save(input_file, output_file);
        viewMore.save(recon_lock_duration, recon_lock_nb_match, recon_match_mode);
        std::vector<std::string> messages;
        messages.push_back(input_file);
        messages.push_back(output_file);
        messages.push_back(to_string_dec_uint(recon_lock_duration));
        messages.push_back(to_string_dec_uint(recon_lock_nb_match));
        messages.push_back(to_string_dec_uint(recon_match_mode));
        on_changed(messages);
        nav.pop();
    };
}
} /* namespace ui */
