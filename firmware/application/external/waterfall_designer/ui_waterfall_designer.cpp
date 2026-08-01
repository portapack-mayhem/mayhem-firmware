/*
 * Copyleft Mr. Robot
 * Copyright HTotoo
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
#include "ui_textentry.hpp"
#include "convert.hpp"

#include <algorithm>

using namespace portapack;

namespace ui::external_app::waterfall_designer {

namespace {

/* Parses an "index,R,G,B" profile line.
 * NB: std::stoi() must not be used here - the firmware is built with
 * -fno-exceptions, so any malformed field in a hand-edited profile makes it
 * call std::terminate() instead of throwing. parse_int() is what gradient.cpp
 * uses to read the very same file format. */
bool parse_level(std::string_view line, uint8_t& index, uint8_t& r, uint8_t& g, uint8_t& b) {
    // Comments are skipped by Gradient::load_file() too; parse_int() ignores
    // trailing junk, so "#0,0,0,0" would otherwise read back as a real level.
    if (line.empty() || line.front() == '#') return false;

    auto cols = split_string(line, ',');
    if (cols.size() != 4) return false;

    uint8_t* const out[4] = {&index, &r, &g, &b};

    for (size_t i = 0; i < 4; i++) {
        int32_t value;
        if (!parse_int(cols[i], value) || value < 0 || value > 255) return false;
        *out[i] = static_cast<uint8_t>(value);
    }

    return true;
}

bool is_color_level(const std::string& line) {
    uint8_t index, r, g, b;
    return parse_level(line, index, r, g, b);
}

}  // namespace

WaterfallDesignerView::WaterfallDesignerView(NavigationView& nav)
    : nav_{nav} {
    baseband::run_prepared_image(portapack::memory::map::m4_code.base());

    add_children({&labels,
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
                  &waterfall});
    ensure_directory(waterfalls_dir);
    ui::Rect waterfall_rect{0, header_height, screen_rect().width(), screen_rect().height() - header_height};
    waterfall.set_parent_rect(waterfall_rect);

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

        waterfall.stop();

        // record_view determines the correct oversampling to apply and returns the actual sample rate.
        // NB: record_view is what actually updates proc_capture baseband settings.
        auto actual_sample_rate = record_view.set_sampling_rate(new_capture_rate);

        // Update the radio model with the actual sampling rate.
        receiver_model.set_sampling_rate(actual_sample_rate);

        // Get suitable anti-aliasing BPF bandwidth for MAX2837 given the actual sample rate.
        auto anti_alias_filter_bandwidth = filter_bandwidth_for_sampling_rate(actual_sample_rate);
        receiver_model.set_baseband_bandwidth(anti_alias_filter_bandwidth);

        capture_rate = new_capture_rate;

        waterfall.start();
    };

    button_new.on_select = [this]() {
        on_create_new_profile();
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
        // nav_.pop();
    };

    menu_view.on_highlight = [this]() {
        highlighted_index_ = menu_view.highlighted_index();
    };

    receiver_model.enable();
    option_bandwidth.set_by_value(capture_rate);

    record_view.on_error = [&nav](std::string message) {
        nav.display_modal("Error", message);
    };

    button_save.hidden(true);
    button_add_level.hidden(true);
    button_remove_level.hidden(true);
    button_edit_color.hidden(true);
    button_apply_setting.hidden(true);

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
    if (!if_apply_setting)
        restore_current_profile();
    else if (profile_backed_up_)
        delete_file(waterfalls_dir / u"wtf_des_bk.bk");  // kept the edit, drop the backup

    receiver_model.disable();
    baseband::shutdown();
}

void WaterfallDesignerView::set_parent_rect(const Rect new_parent_rect) {
    View::set_parent_rect(new_parent_rect);

    ui::Rect waterfall_rect{0, header_height, new_parent_rect.width(), new_parent_rect.height() - header_height};
    waterfall.set_parent_rect(waterfall_rect);
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
        // NB: FileLoadView calls this and then pops, same as text_prompt does.
        // Defer for the same two reasons - see on_create_new_profile().
        pending_profile_path = std::move(new_file_path);
        nav_.set_on_pop([this]() {
            on_profile_changed(pending_profile_path);
        });
    };
}

bool WaterfallDesignerView::read_profile_file(const std::filesystem::path& path) {
    File profile_file;

    if (profile_file.open(path)) return false;

    profile_levels.clear();
    auto reader = FileLineReader(profile_file);

    for (const auto& line : reader) {
        auto entry = line;

        // Strip the whole line terminator; the old code only dropped one char,
        // so CRLF files kept a stray '\r' in every entry.
        while (!entry.empty() && (entry.back() == '\n' || entry.back() == '\r'))
            entry.pop_back();

        if (entry.empty()) continue;
        profile_levels.push_back(std::move(entry));
    }

    return true;
}

bool WaterfallDesignerView::write_profile_file(const std::filesystem::path& path) {
    File profile_file;

    if (profile_file.open(path, false, true)) return false;

    profile_file.seek(0);
    profile_file.truncate();

    for (const auto& entry : profile_levels)
        profile_file.write_line(entry);

    return true;
}

void WaterfallDesignerView::on_profile_changed(const std::filesystem::path& new_profile_path) {
    // read_profile_file() only touches profile_levels once the open succeeded,
    // so a failure here leaves the previously loaded profile intact.
    if (!read_profile_file(new_profile_path)) {
        nav_.display_modal("Err", "open err");
        return;
    }

    current_profile_path = new_profile_path;
    highlighted_index_ = 0;

    button_save.hidden(false);
    button_add_level.hidden(false);
    button_remove_level.hidden(false);
    button_edit_color.hidden(false);
    button_apply_setting.hidden(false);

    refresh_menu_view();
    on_apply_current_to_wtf();
}

void WaterfallDesignerView::refresh_menu_view() {
    menu_view.clear();

    for (const auto& line : profile_levels) {
        uint8_t index, r, g, b;

        // index,R,G,B - anything else (comment, header, garbage) is shown greyed.
        if (parse_level(line, index, r, g, b)) {
            menu_view.add_item({line,
                                ui::Color(r, g, b),
                                &bitmap_icon_cwgen,
                                [this](KeyEvent) {
                                    button_remove_level.focus();
                                }});
        } else {
            menu_view.add_item({line,
                                ui::Color::grey(),
                                &bitmap_icon_notepad,
                                [this](KeyEvent) {
                                    button_add_level.focus();
                                }});
        }
    }

    // menu_view.clear() resets its own highlight to 0 but does not fire
    // on_highlight, so highlighted_index_ has to be re-synced by hand or the
    // edit/remove buttons act on a different row than the one the user sees.
    if (highlighted_index_ >= profile_levels.size())
        highlighted_index_ = profile_levels.empty() ? 0 : profile_levels.size() - 1;

    if (!profile_levels.empty())
        menu_view.set_highlighted(highlighted_index_);

    set_dirty();
}

void WaterfallDesignerView::on_apply_current_to_wtf() {
    /* Take the backup lazily, right before the first overwrite of the live
     * gradient. Doing it in the constructor meant copy_file() (~1.8kB of stack)
     * ran underneath run_external_app(), which holds a 776 byte frame with a
     * File of its own - about 3.6kB of the 4kB M0 process stack. It also did SD
     * I/O even when the user just opened the app and backed straight out. */
    backup_current_profile();

    /* NB: this used to copy_file(current_profile_path, "waterfall.txt"), which
     * costs ~1.8kB of stack (two FILs + a 512 byte block buffer) out of the 4kB
     * M0 process stack, on top of whatever UI callback we are nested under.
     * profile_levels is the authoritative copy of the profile anyway, so write
     * it straight out through a single File instead. */
    if (!write_profile_file(u"waterfall.txt")) return;

    waterfall.load_gradient();

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

    if (!write_profile_file(current_profile_path)) {
        nav_.display_modal("Err", "open err");
        return;
    }

    nav_.display_modal("Save", "Saved profile\n" + current_profile_path.string());
}

void WaterfallDesignerView::on_add_level() {
    if (current_profile_path.empty()) return;

    /* A freshly created profile has no rows at all, and the old guard
     * (highlighted_index_ >= size()) then refused with no feedback - so the
     * button looked dead on exactly the file you just made. Seed instead. */
    size_t insert_pos = std::min<size_t>(highlighted_index_, profile_levels.size());

    // Don't push a level above a comment/header row; drop it after instead.
    if (insert_pos < profile_levels.size() && !is_color_level(profile_levels[insert_pos]))
        insert_pos++;

    profile_levels.insert(profile_levels.begin() + insert_pos, "0,128,128,128");
    highlighted_index_ = insert_pos;

    refresh_menu_view();
    on_edit_color();
}

void WaterfallDesignerView::on_remove_level() {
    if (highlighted_index_ >= profile_levels.size()) return;
    if (!is_color_level(profile_levels[highlighted_index_])) return;

    profile_levels.erase(profile_levels.begin() + highlighted_index_);
    refresh_menu_view();
}

void WaterfallDesignerView::on_edit_color() {
    if (highlighted_index_ >= profile_levels.size()) return;
    if (!is_color_level(profile_levels[highlighted_index_])) return;

    // Capture the row by value: highlighted_index_ can move (or the list can
    // shrink) while the picker is up, and the old code wrote to whatever
    // highlighted_index_ happened to be at save time with no bounds check.
    const size_t index = highlighted_index_;

    auto color_picker_view = nav_.push<WaterfallDesignerColorPickerView>(profile_levels[index]);
    color_picker_view->on_save = [this, index](std::string new_color) {
        if (index >= profile_levels.size()) return;

        profile_levels[index] = std::move(new_color);
        refresh_menu_view();
        on_apply_current_to_wtf();
    };
}

void WaterfallDesignerView::backup_current_profile() {
    if (backup_attempted_) return;
    backup_attempted_ = true;

    std::filesystem::path current_wtf_path = u"waterfall.txt";
    std::filesystem::path backup_path = waterfalls_dir / u"wtf_des_bk.bk";

    if (!file_exists(current_wtf_path)) return;

    profile_backed_up_ = copy_file(current_wtf_path, backup_path).ok();
}

void WaterfallDesignerView::restore_current_profile() {
    // Only touch waterfall.txt if we actually took a backup on entry; otherwise
    // this would silently do nothing and leave the edited gradient installed.
    if (!profile_backed_up_) return;

    std::filesystem::path backup_path = waterfalls_dir / u"wtf_des_bk.bk";

    copy_file(backup_path, u"waterfall.txt");
    delete_file(backup_path);
}

void WaterfallDesignerView::on_apply_setting() {
}

void WaterfallDesignerView::on_create_new_profile() {
    text_prompt(
        nav_,
        file_name_buffer,
        32 - 4,  // fat32
        ENTER_KEYBOARD_MODE_ALPHA,
        [this](std::string& buffer) {
            if (buffer.empty()) return;

            if (buffer.length() < 4 || buffer.substr(buffer.length() - 4) != ".txt") {
                buffer += ".txt";
            }

            auto new_path = waterfalls_dir / buffer;
            bool created;

            {
                // Scoped so the FIL is closed (and its 512 byte sector cache is
                // off the stack) before anything re-opens the same file - _FS_LOCK
                // is 0, so FatFs will not stop us from double-opening it.
                File new_file;
                created = !new_file.create(new_path);
            }

            /* NB: everything below has to run *after* the keyboard view is gone.
             * text_prompt()'s caller invokes this handler and then pops, so:
             *  - pushing a modal from here would pop the modal, not the keyboard;
             *  - and running on_profile_changed() inline stacks its File plus the
             *    gradient write under the alphanum/button/dispatch frames, which
             *    is what blows the 4kB M0 process stack ("Stack Overflow" guru).
             * set_on_pop() defers it to just after the pop, at a shallow depth. */
            if (!created) {
                nav_.set_on_pop([this]() {
                    nav_.display_modal("Err", "create file err");
                });
                return;
            }

            pending_profile_path = new_path;
            nav_.set_on_pop([this]() {
                on_profile_changed(pending_profile_path);
            });
        });
}

WaterfallDesignerColorPickerView::WaterfallDesignerColorPickerView(NavigationView& nav, std::string color_str)
    : nav_{nav},
      color_str_{color_str} {
    add_children({&labels,
                  &field_red,
                  &field_green,
                  &field_blue,
                  &field_step,
                  &field_index,
                  &progressbar,
                  &button_save});

    progressbar.set_max(UINT8_MAX);

    parse_level(color_str_, index_, red_, green_, blue_);

    field_red.set_value(red_);
    field_green.set_value(green_);
    field_blue.set_value(blue_);
    field_index.set_value(index_);

    // cb
    field_red.on_change = [this](int32_t) {
        update_color_index();
    };

    field_green.on_change = [this](int32_t) {
        update_color_index();
    };

    field_blue.on_change = [this](int32_t) {
        update_color_index();
    };

    button_save.on_select = [this](Button&) {
        if (on_save) on_save(build_color_str());
        nav_.pop();
    };

    field_index.on_change = [this](int32_t) {
        update_color_index();
    };

    field_step.on_change = [this](int32_t) {
        field_red.set_step(field_step.value());
        field_green.set_step(field_step.value());
        field_blue.set_step(field_step.value());
        field_index.set_step(field_step.value());
    };

    update_color_index();
}

void WaterfallDesignerColorPickerView::focus() {
    button_save.focus();
}

void WaterfallDesignerColorPickerView::update_color_index() {
    index_ = static_cast<uint8_t>(field_index.value());
    red_ = static_cast<uint8_t>(field_red.value());
    green_ = static_cast<uint8_t>(field_green.value());
    blue_ = static_cast<uint8_t>(field_blue.value());

    // Repaint through the normal dirty pass instead of driving the display from
    // inside an on_change handler; paint() below already draws the swatch.
    set_dirty();
}

void WaterfallDesignerColorPickerView::paint(Painter& painter) {
    const Rect preview_rect{screen_width - 48, 1 * 16, 40, 40};

    painter.fill_rectangle(
        {preview_rect.left(), preview_rect.top(), preview_rect.width(), preview_rect.height()},
        ui::Color(red_, green_, blue_));
}

std::string WaterfallDesignerColorPickerView::build_color_str() {
    // Always emit index_ - the old version kept the index substring from the
    // input line, so edits made with field_index were silently discarded.
    return to_string_dec_uint(index_) + "," +
           to_string_dec_uint(red_) + "," +
           to_string_dec_uint(green_) + "," +
           to_string_dec_uint(blue_);
}

} /* namespace ui::external_app::waterfall_designer */
