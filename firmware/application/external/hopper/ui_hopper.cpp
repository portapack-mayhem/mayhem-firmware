/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
 * Copyright (C) 2020 euquiq
 * copyleft mr.r0b0t from the F society
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

#include "ui_hopper.hpp"
#include "ui_receiver.hpp"
#include "ui_freqman.hpp"
#include "ui_fileman.hpp"
#include "file_path.hpp"
#include "file_reader.hpp"
#include "convert.hpp"

#include "baseband_api.hpp"
#include "string_format.hpp"

using namespace portapack;

namespace ui::external_app::hopper {

void HopperView::focus() {
    button_load_list.focus();
}

HopperView::~HopperView() {
    transmitter_model.disable();
    baseband::shutdown();
}

void HopperView::update_freq_list_menu_view() {
    menu_freq_list.clear();
    uint8_t list_count = freq_list.size();

    if (list_count == 0) {
        menu_freq_list.add_item({"Add freq or load list",
                                 Theme::getInstance()->bg_darkest->foreground,
                                 nullptr,
                                 nullptr});
    }

    for (uint8_t i = 0; i < list_count; i++) {
        menu_freq_list.add_item({to_string_rounded_freq(freq_list[i], INT8_MAX),
                                 Theme::getInstance()->bg_darkest->foreground,
                                 nullptr,
                                 nullptr});
    }

    set_dirty();
}

void HopperView::on_retune(const rf::Frequency freq, const uint32_t range) {
    if (freq) {
        transmitter_model.set_target_frequency(freq);
        text_range_number.set(to_string_dec_uint(range, 2));
    }
}

void HopperView::set_hopper_channel(uint32_t i, uint32_t width, uint64_t center, uint32_t duration) {
    hopper_channels[i].enabled = true;
    hopper_channels[i].width = (width * 0xFFFFFFULL) / 1536000;
    hopper_channels[i].center = center;
    hopper_channels[i].duration = 30720 * duration;
}

void HopperView::start_tx() {
    uint32_t c, i = 0;
    bool out_of_ranges = false;

    size_t hop_value = options_hop.selected_index_value();

    // Disable all channels by default
    for (c = 0; c < JAMMER_MAX_CH; c++)
        hopper_channels[c].enabled = false;

    uint8_t channel_count = freq_list.size();

    if (channel_count <= JAMMER_MAX_CH) {
        for (c = 0; c < channel_count; c++) {
            if (i >= JAMMER_MAX_CH) {
                out_of_ranges = true;
                break;
            }
            set_hopper_channel(i, JAMMER_CH_WIDTH, freq_list[c], hop_value);
            i++;
        }
    } else {
        out_of_ranges = true;
    }

    if (!out_of_ranges && i) {
        text_range_total.set("/" + to_string_dec_uint(i, 2));

        jamming = true;
        button_transmit.set_style(&style_cancel);
        button_transmit.set_text("STOP");

        transmitter_model.set_rf_amp(field_amp.value());
        transmitter_model.set_tx_gain(field_gain.value());
        transmitter_model.set_baseband_bandwidth(28'000'000);  // Although tx is narrowband , let's use Max TX LPF .
        transmitter_model.enable();

        baseband::set_jammer(true, (JammerType)options_type.selected_index(), options_speed.selected_index_value());
        mscounter = 0;  // euquiq: Reset internal ms counter for do_timer()
    } else {
        if (out_of_ranges)
            nav_.display_modal("Error", "Jam freq too much.");
    }
}

void HopperView::stop_tx() {
    button_transmit.set_style(&style_val);
    button_transmit.set_text("START");
    transmitter_model.disable();
    baseband::set_jammer(false, JammerType::TYPE_FSK, 0);
    jamming = false;
    cooling = false;
}

// called each 1/60th of second
void HopperView::on_timer() {
    if (++mscounter == 60) {
        mscounter = 0;
        if (jamming) {
            if (cooling) {
                if (++seconds >= field_timepause.value()) {                // Re-start TX
                    transmitter_model.set_baseband_bandwidth(28'000'000);  // Although tx is narrowband , let's use Max TX LPF .
                    transmitter_model.enable();
                    button_transmit.set_text("STOP");
                    baseband::set_jammer(true, (JammerType)options_type.selected_index(), options_speed.selected_index_value());

                    int32_t jitter_amount = field_jitter.value();
                    if (jitter_amount) {
                        lfsr_v = lfsr_iterate(lfsr_v);
                        jitter_amount = (jitter_amount / 2) - (lfsr_v & jitter_amount);
                        mscounter += jitter_amount;
                    }

                    cooling = false;
                    seconds = 0;
                }
            } else {
                if (++seconds >= field_timetx.value())  // Start cooling period:
                {
                    transmitter_model.disable();
                    button_transmit.set_text("PAUSED");
                    baseband::set_jammer(false, JammerType::TYPE_FSK, 0);

                    int32_t jitter_amount = field_jitter.value();
                    if (jitter_amount) {
                        lfsr_v = lfsr_iterate(lfsr_v);
                        jitter_amount = (jitter_amount / 2) - (lfsr_v & jitter_amount);
                        mscounter += jitter_amount;
                    }

                    cooling = true;
                    seconds = 0;
                }
            }
        }
    }
}

void HopperView::load_list() {
    freq_list.clear();

    auto open_view = nav_.push<FileLoadView>(".PHOP");
    open_view->push_dir(hopper_dir);
    open_view->on_changed = [this](std::filesystem::path path) {
        File f;
        auto error = f.open(path);
        if (error) {
            nav_.display_modal("Err", "Can't open.");
            update_freq_list_menu_view();
            return;
        }

        freq_list.clear();

        auto reader = FileLineReader(f);
        for (const auto& line : reader) {
            if (line.length() == 0 || line[0] == '#')
                continue;

            rf::Frequency freq;
            if (parse_int(line, freq)) {
                if (freq_list.size() >= JAMMER_MAX_CH) {
                    break;
                }
                freq_list.push_back(freq);
            }
        }
        update_freq_list_menu_view();
    };

    update_freq_list_menu_view();
}

void HopperView::save_list() {
    if (freq_list.empty()) {
        nav_.display_modal("Err", "Nothing to save");
        return;
    }

    ensure_directory(hopper_dir);

    filename_buffer = "";
    text_prompt(
        nav_,
        filename_buffer,
        64,
        [this](std::string& value) {
            auto path = hopper_dir / (value + ".PHOP");

            File f;
            auto error = f.create(path);
            if (error) {
                nav_.display_modal("Err", "Create fail.");
                return;
            }

            for (const auto& freq : freq_list) {
                auto freq_str = to_string_dec_uint(freq) + "\n";
                f.write(freq_str.c_str(), freq_str.length());
            }

            text_range_number.set("Saved: " + path.filename().string());
        });
}

HopperView::HopperView(
    NavigationView& nav)
    : nav_{nav} {
    // baseband::run_image(portapack::spi_flash::image_tag_jammer);
    baseband::run_prepared_image(portapack::memory::map::m4_code.base());

    add_children({&menu_freq_list,
                  &button_load_list,
                  &button_add_freq,
                  &button_delete_freq,
                  &button_save_list,
                  &button_clear,
                  &labels,
                  &options_type,
                  &text_range_number,
                  &text_range_total,
                  &options_speed,
                  &options_hop,
                  &field_timetx,
                  &field_timepause,
                  &field_jitter,
                  &field_gain,
                  &field_amp,
                  &button_transmit});

    options_type.set_selected_index(3);   // Rand CW
    options_speed.set_selected_index(3);  // 10kHz
    options_hop.set_selected_index(1);    // 50ms
    button_transmit.set_style(&style_val);

    field_timetx.set_value(30);
    field_timepause.set_value(1);
    field_gain.set_value(transmitter_model.tx_gain());
    field_amp.set_value(transmitter_model.rf_amp());

    button_load_list.on_select = [this]() {
        load_list();
        update_freq_list_menu_view();
    };

    button_add_freq.on_select = [this]() {
        if (freq_list.size() < JAMMER_MAX_CH) {
            auto kb_view = nav_.push<FrequencyKeypadView>(0);
            kb_view->on_changed = [this](rf::Frequency freq) {
                freq_list.push_back(freq);
                update_freq_list_menu_view();
            };

        } else {
            nav_.display_modal("Err", "No more.");
        }
        update_freq_list_menu_view();
    };

    button_delete_freq.on_select = [this]() {
        auto i = menu_freq_list.highlighted_index();
        if (i < freq_list.size()) {
            freq_list.erase(freq_list.begin() + i);
        }
        update_freq_list_menu_view();
    };

    button_save_list.on_select = [this]() {
        save_list();
        update_freq_list_menu_view();
    };

    button_clear.on_select = [this]() {
        // clang-format off
        nav_.display_modal("Del:", "Clean all?\n", YESNO, [this](bool choice) {
                if (choice){
                    freq_list.clear();
                    update_freq_list_menu_view(); 
        } }, TRUE);
        // clang-format on

        update_freq_list_menu_view();
    };

    button_transmit.on_select = [this](Button&) {
        if (jamming || cooling)
            stop_tx();
        else
            start_tx();
    };

    menu_freq_list.on_left = [this]() {
        button_load_list.focus();
    };

    menu_freq_list.on_right = [this]() {
        button_load_list.focus();
    };

    update_freq_list_menu_view();
}

}  // namespace ui::external_app::hopper
