/*
 * Copyright (C) 2016 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2018 Furrtek
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

#include "ui_waterfall_designer.hpp"
#include "baseband_api.hpp"
#include "portapack.hpp"
#include "ui_freqman.hpp"
#include "file_path.hpp"
#include "ui_fileman.hpp"
#include "file_reader.hpp"

using namespace portapack;

namespace ui::external_app::waterfall_designer {

WaterfallDesignerView::WaterfallDesignerView(NavigationView& nav)
    : nav_{nav} {
    baseband::run_image(portapack::spi_flash::image_tag_capture);

    add_children({
        &labels,
        &field_frequency,
        &field_frequency_step,
        &field_rf_amp,
        &field_lna,
        &field_vga,
        &option_bandwidth,
        &record_view,
        &menu_view,
        &button_new,
        &button_open,
        &button_save,
        &button_add_level,
        &button_remove_level,
        &button_edit_color,
        &button_apply_setting,
    });

    waterfall = std::make_unique<spectrum::WaterfallView>();
    add_child(waterfall.get());

    menu_view.set_parent_rect({0, 1 * 16, screen_width, 7 * 16});

    field_frequency_step.set_by_value(receiver_model.frequency_step());
    field_frequency_step.on_change = [this](size_t, OptionsField::value_t v) {
        receiver_model.set_frequency_step(v);
        this->field_frequency.set_step(v);
    };

    freqman_set_bandwidth_option(SPEC_MODULATION, option_bandwidth);
    option_bandwidth.on_change = [this](size_t, uint32_t new_capture_rate) {
        /* Nyquist would imply a sample rate of 2x bandwidth, but because the ADC
         * provides 2 values (I,Q), the sample_rate is equal to bandwidth here. */

        /* capture_rate (bandwidth) is used for FFT calculation and display LCD, and also in recording writing SD Card rate. */
        /* ex. sampling_rate values, 4Mhz, when recording 500 kHz (BW) and fs 8 Mhz, when selected 1 Mhz BW ... */
        /* ex. recording 500kHz BW to .C16 file, base_rate clock 500kHz x2(I,Q) x 2 bytes (int signed) =2MB/sec rate SD Card. */

        waterfall->stop();

        // record_view determines the correct oversampling to apply and returns the actual sample rate.
        // NB: record_view is what actually updates proc_capture baseband settings.
        auto actual_sample_rate = record_view.set_sampling_rate(new_capture_rate);

        // Update the radio model with the actual sampling rate.
        receiver_model.set_sampling_rate(actual_sample_rate);

        // Get suitable anti-aliasing BPF bandwidth for MAX2837 given the actual sample rate.
        auto anti_alias_filter_bandwidth = filter_bandwidth_for_sampling_rate(actual_sample_rate);
        receiver_model.set_baseband_bandwidth(anti_alias_filter_bandwidth);

        capture_rate = new_capture_rate;

        waterfall->start();
    };

    button_new.on_select = [this]() {
        profile_levels.clear();
        current_profile_path = "";
        refresh_menu_view();
    };

    button_open.on_select = [this]() {
        on_open_profile();
    };

    button_save.on_select = [this]() {
        on_save_profile();
    };

    button_add_level.on_select = [this]() {
        on_add_level();
    };

    button_remove_level.on_select = [this]() {
        on_remove_level();
    };

    button_edit_color.on_select = [this]() {
        on_edit_color();
    };

    button_apply_setting.on_select = [this]() {
        if_apply_setting = true;
        on_apply_current_to_wtf();
        nav_.pop();
    };

    receiver_model.enable();
    option_bandwidth.set_by_value(capture_rate);

    record_view.on_error = [&nav](std::string message) {
        nav.display_modal("Error", message);
    };

    refresh_menu_view();
}

WaterfallDesignerView::WaterfallDesignerView(
    NavigationView& nav,
    ReceiverModel::settings_t override)
    : WaterfallDesignerView(nav) {
    // Settings to override when launched from another app (versus from AppSettings .ini file)
    field_frequency.set_value(override.frequency_app_override);
}

WaterfallDesignerView::~WaterfallDesignerView() {
    if(!if_apply_setting) restore_current_profile();
    receiver_model.disable();
    baseband::shutdown();
}

void WaterfallDesignerView::set_parent_rect(const Rect new_parent_rect) {
    View::set_parent_rect(new_parent_rect);

    ui::Rect waterfall_rect{0, header_height, new_parent_rect.width(), new_parent_rect.height() - header_height};
    waterfall->set_parent_rect(waterfall_rect);
}

void WaterfallDesignerView::focus() {
    button_open.focus();
}

void WaterfallDesignerView::on_freqchg(int64_t freq) {
    field_frequency.set_value(freq);
}

void WaterfallDesignerView::on_open_profile() {
    auto open_view = nav_.push<FileLoadView>(".txt");
    open_view->push_dir(waterfalls_dir);
    open_view->on_changed = [this](std::filesystem::path new_file_path) {
        on_profile_changed(new_file_path);
    };
}

void WaterfallDesignerView::on_profile_changed(std::filesystem::path new_profile_path) {
    current_profile_path = new_profile_path;
    profile_levels.clear();

    File playlist_file;
    auto error = playlist_file.open(new_profile_path.string());

    if (error) return;

    menu_view.clear();
    auto reader = FileLineReader(playlist_file);

    for (const auto& line : reader) {
        profile_levels.push_back(line);
    }

    for (auto& line : profile_levels) {
        // remove empty lines
        if (line == "\n" || line == "\r\n" || line == "\r") {
            profile_levels.erase(std::remove(profile_levels.begin(), profile_levels.end(), line), profile_levels.end());
        }

        // remove line end \n etc
        if (line.length() > 0 && (line[line.length() - 1] == '\n' || line[line.length() - 1] == '\r')) {
            line = line.substr(0, line.length() - 1);
        }
    }

    refresh_menu_view();
}

void WaterfallDesignerView::refresh_menu_view() {
    menu_view.clear();

    for (const auto& line : profile_levels) {
        if (line.length() == 0 || line[0] == '#') {
            menu_view.add_item({line,
                                ui::Color::grey(),
                                &bitmap_icon_notepad,
                                [this](KeyEvent) {
                                    button_add_level.focus();
                                }});
        } else {
            // index,R,G,B
            size_t pos = 0;
            size_t next_pos = 0;

            // pass index
            next_pos = line.find(',', pos);
            if (next_pos == std::string::npos) continue;
            pos = next_pos + 1;

            // r
            next_pos = line.find(',', pos);
            if (next_pos == std::string::npos) continue;
            uint8_t r = static_cast<uint8_t>(std::stoi(line.substr(pos, next_pos - pos)));
            pos = next_pos + 1;

            // g
            next_pos = line.find(',', pos);
            if (next_pos == std::string::npos) continue;
            uint8_t g = static_cast<uint8_t>(std::stoi(line.substr(pos, next_pos - pos)));
            pos = next_pos + 1;

            // b
            uint8_t b = static_cast<uint8_t>(std::stoi(line.substr(pos)));

            ui::Color color = ui::Color(r, g, b);
            menu_view.add_item({line,
                                color,
                                &bitmap_icon_cwgen,
                                [this](KeyEvent) {
                                    button_remove_level.focus();
                                }});
        }
    }
    set_dirty();
}

void WaterfallDesignerView::on_apply_current_to_wtf() {
    std::filesystem::path current_path = "waterfall.txt";
    copy_file(current_profile_path, current_path);

    remove_child(waterfall.get());
    waterfall.reset();
    waterfall = std::make_unique<spectrum::WaterfallView>();
    add_child(waterfall.get());
    
    ui::Rect waterfall_rect{0, header_height, screen_rect().width(), screen_rect().height() - header_height};
    waterfall->set_parent_rect(waterfall_rect);
    
    set_dirty();
}

void WaterfallDesignerView::on_save_profile() {
    if (current_profile_path.empty()) {
        nav_.display_modal("Err", "No profile file loaded");
        return;
    } else if (profile_levels.empty()) {
        nav_.display_modal("Err", "List is empty");
        return;
    }

    File profile_file;
    auto error = profile_file.open(current_profile_path.string(), false, false);

    if (error) {
        nav_.display_modal("Err", "open err");
        return;
    }

    // clear file
    profile_file.seek(0);
    profile_file.truncate();

    // write new data
    for (const auto& entry : profile_levels) {
        profile_file.write_line(entry);
    }

    nav_.display_modal("Save", "Saved profile\n" + current_profile_path.string());
}

void WaterfallDesignerView::on_add_level() {
    // new view to let user add
}

void WaterfallDesignerView::on_remove_level() {
    // remove entrance from vec
    profile_levels.erase(profile_levels.begin() + menu_view.highlighted_index());
    refresh_menu_view();
}

void WaterfallDesignerView::on_edit_color() {
    if (menu_view.highlighted_index() >= profile_levels.size()) return;
    
    auto color_picker_view = nav_.push<WaterfallDesignerColorPickerView>(profile_levels[menu_view.highlighted_index()]);
    color_picker_view->on_save = [this](std::string new_color) {
        profile_levels[menu_view.highlighted_index()] = new_color;
        refresh_menu_view();
    };
}

void WaterfallDesignerView::backup_current_profile() {
    std::filesystem::path curren_wtf_path = "waterfall.txt";
    std::filesystem::path backup_path = waterfalls_dir / "wtf_des_bk.bk";
    copy_file(curren_wtf_path, backup_path);
}

void WaterfallDesignerView::restore_current_profile() {
    std::filesystem::path backup_path = waterfalls_dir / "wtf_des_bk.bk";
    std::filesystem::path put_back_path = "waterfall.txt";
    copy_file(backup_path, put_back_path);
    delete_file(backup_path);
}

void WaterfallDesignerView::on_apply_setting() {

}

WaterfallDesignerColorPickerView::WaterfallDesignerColorPickerView(NavigationView& nav, std::string color_str)
    : nav_{nav},
      color_str_{color_str} {
    add_children({&labels,
                  &field_red,
                  &field_green,
                  &field_blue,
                  &button_save});

    size_t pos = 0;
    size_t next_pos = 0;

    // index
    next_pos = color_str.find(',', pos);
    if (next_pos != std::string::npos) {
        pos = next_pos + 1;
    }

    // r
    next_pos = color_str.find(',', pos);
    if (next_pos != std::string::npos) {
        red_ = static_cast<uint8_t>(std::stoi(color_str.substr(pos, next_pos - pos)));
        pos = next_pos + 1;
    }

    // g
    next_pos = color_str.find(',', pos);
    if (next_pos != std::string::npos) {
        green_ = static_cast<uint8_t>(std::stoi(color_str.substr(pos, next_pos - pos)));
        pos = next_pos + 1;
    }

    // b
    blue_ = static_cast<uint8_t>(std::stoi(color_str.substr(pos)));

    field_red.set_value(red_);
    field_green.set_value(green_);
    field_blue.set_value(blue_);

    // cb
    field_red.on_change = [this](int32_t) {
        update_color();
    };

    field_green.on_change = [this](int32_t) {
        update_color();
    };

    field_blue.on_change = [this](int32_t) {
        update_color();
    };

    button_save.on_select = [this](Button&) {
        if (on_save) on_save(build_color_str());
        nav_.pop();
    };

    update_color();
}

void WaterfallDesignerColorPickerView::focus() {
    button_save.focus();
}

void WaterfallDesignerColorPickerView::update_color() {
    red_ = static_cast<uint8_t>(field_red.value());
    green_ = static_cast<uint8_t>(field_green.value());
    blue_ = static_cast<uint8_t>(field_blue.value());

    const Rect preview_rect{screen_width - 48, 1 * 16, 40, 40};
    

    painter.fill_rectangle(
        {preview_rect.left() , preview_rect.top() , preview_rect.width() , preview_rect.height() },
        ui::Color(red_, green_, blue_));
}

void WaterfallDesignerColorPickerView::paint(Painter& painter) {

}

std::string WaterfallDesignerColorPickerView::build_color_str() {
    size_t index_pos = color_str_.find(',');
    if (index_pos != std::string::npos) {
        return color_str_.substr(0, index_pos + 1) + 
               std::to_string(red_) + "," +
               std::to_string(green_) + "," +
               std::to_string(blue_);
    }
    return "0," + std::to_string(red_) + "," + std::to_string(green_) + "," + std::to_string(blue_);
}

} /* namespace ui::external_app::waterfall_designer */
