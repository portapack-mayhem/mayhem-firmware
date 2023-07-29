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
                  &checkbox_clear_output});

    checkbox_autosave_freqs.set_value(persistent_memory::recon_autosave_freqs());
    checkbox_autostart_recon.set_value(persistent_memory::recon_autostart_recon());
    checkbox_clear_output.set_value(persistent_memory::recon_clear_output());

    text_input_file.set(_input_file);
    button_output_file.set_text(_output_file);

    button_load_freqs.on_select = [this, &nav](Button&) {
        auto open_view = nav.push<FileLoadView>(".TXT");
        open_view->push_dir(freqman_dir);
        open_view->on_changed = [this, &nav](std::filesystem::path new_file_path) {
            if (new_file_path.native().find(freqman_dir.native()) == 0) {
                _input_file = new_file_path.stem().string();
                text_input_file.set(_input_file);
            } else {
                nav.display_modal("LOAD ERROR", "A valid file from\nFREQMAN directory is\nrequired.");
            }
        };
    };

    button_save_freqs.on_select = [this, &nav](Button&) {
        auto open_view = nav.push<FileLoadView>(".TXT");
        open_view->push_dir(freqman_dir);
        open_view->on_changed = [this, &nav](std::filesystem::path new_file_path) {
            if (new_file_path.native().find(freqman_dir.native()) == 0) {
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
                        _output_file = std::move(buffer);
                        button_output_file.set_text(_output_file);
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
    persistent_memory::set_recon_update_ranges_when_recon(checkbox_update_ranges_when_recon.value());
    persistent_memory::set_recon_auto_record_locked(checkbox_auto_record_locked.value());
};

void ReconSetupViewMain::focus() {
    button_load_freqs.focus();
}

ReconSetupViewMore::ReconSetupViewMore(NavigationView& nav, Rect parent_rect)
    : View(parent_rect) {
    (void)nav;
    hidden(true);

    add_children({&checkbox_load_freqs,
                  &checkbox_load_ranges,
                  &checkbox_load_hamradios,
                  &checkbox_update_ranges_when_recon,
                  &checkbox_auto_record_locked});

    checkbox_load_freqs.set_value(persistent_memory::recon_load_freqs());
    checkbox_load_ranges.set_value(persistent_memory::recon_load_ranges());
    checkbox_load_hamradios.set_value(persistent_memory::recon_load_hamradios());
    checkbox_update_ranges_when_recon.set_value(persistent_memory::recon_update_ranges_when_recon());
    checkbox_auto_record_locked.set_value(persistent_memory::recon_auto_record_locked());
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
