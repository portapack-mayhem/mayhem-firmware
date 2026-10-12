/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2020 euquiq
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

#include "ui_looking_glass_app.hpp"
#include "convert.hpp"
#include "file_reader.hpp"
#include "file.hpp"
#include "string_format.hpp"
#include "audio.hpp"
#include "file_path.hpp"
#include "event_m0.hpp"
#include "radio.hpp"

#include <algorithm>

using namespace portapack;

namespace ui {

void GlassView::focus() {
    range_presets.focus();
}

GlassView::~GlassView() {
    sweep.stop();
    audio::output::stop();
    radio::set_fast_retune(false);
    receiver_model.set_sampling_rate(3072000);  // Just a hack to avoid hanging other apps
    receiver_model.disable();
    baseband::shutdown();
}

// Function to map the value from one range to another
int32_t GlassView::map(int32_t value, int32_t fromLow, int32_t fromHigh, int32_t toLow, int32_t toHigh) {
    return toLow + ((value - fromLow) * (toHigh - toLow) + (fromHigh - fromLow) / 2) / (fromHigh - fromLow);
}

void GlassView::update_display_beep() {
    if (beep_enabled) {
        button_beep_squelch.set_style(Theme::getInstance()->fg_green);
        // bip-XXdb
        button_beep_squelch.set_text("bip" + to_string_dec_int(beep_squelch, 3) + "db");
        receiver_model.set_headphone_volume(receiver_model.headphone_volume());  // WM8731 hack.
    } else {
        button_beep_squelch.set_style(Theme::getInstance()->bg_darkest);
        button_beep_squelch.set_text("bip OFF ");
    }
}

void GlassView::manage_beep_audio() {
    if (beep_enabled) {
        audio::set_rate(audio::Rate::Hz_24000);
        audio::output::start();
    } else {
        baseband::request_beep_stop();
        audio::output::stop();
    }
}

rf::Frequency GlassView::get_freq_from_bin_pos(uint16_t pos) {
    // The sweep may start a little above f_min near 0 Hz, ask it.
    return sweep.point_freq(pos);
}

void GlassView::on_marker_change(int8_t delta) {
    if ((marker_pixel_index + delta) < 0)
        marker_pixel_index = marker_pixel_index + delta + points;
    else if ((marker_pixel_index + delta) >= points)
        marker_pixel_index = marker_pixel_index + delta - points;
    else
        marker_pixel_index = marker_pixel_index + delta;

    marker = get_freq_from_bin_pos(marker_pixel_index);
    field_marker.set_text(to_string_short_freq(marker));
    plot_marker(marker_pixel_index);  // Refresh marker on screen
}

void GlassView::start_sweep() {
    if (!sweep_enabled)
        return;  // the constructor is still setting up the fields

    spec_an::Sweep::Params params{};
    params.start = f_min;
    params.span = looking_glass_range;
    params.points = points;
    params.rbw_idx = (mode == LOOKING_GLASS_SLOWSCAN) ? 4 : 0;  // 0: auto, 4: N = 2048
    params.avg = std::max<uint16_t>(trigger / 8, 1);            // FFTs power-averaged per tune

    std::fill(sweep_max.begin(), sweep_max.end(), 0);
    line_ready = false;
    range_max_power = 0;

    sweep.start(params);
    if (paused)
        sweep.stop();
}

void GlassView::reset_live_view() {
    max_freq_hold = 0;
    max_freq_power = -1000;

    // Clear screen in peak mode.
    const int spectrum_height = screen_height - spectrum_y;
    if (live_frequency_view == 2)
        display.fill_rectangle({{0, spectrum_y}, {screen_width, spectrum_height}}, {0, 0, 0});
}

void GlassView::on_slice(const SpecAnSliceMessage& message) {
    if (!sweep.sweeping()) return;  // late reply after a pause or hide
    const int16_t* const src = sweep.accept(message);
    if (!src) return;

    const uint16_t end = std::min<uint16_t>(message.px_end, points);
    for (uint16_t x = message.px_begin; x < end; x++) {
        const uint8_t power = sweep.power(src[x]);  // same scale as the classic spectrum
        if (power > sweep_max[x])
            sweep_max[x] = power;
        if (power > range_max_power)
            range_max_power = power;
    }
    if (message.last) {
        // Only whole sweeps reach the screen: a line taken mid sweep would show
        // the slices not yet measured as black, at a phase that drifts with the
        // frame rate (diagonal stripes in the waterfall).
        for (uint16_t x = 0; x < points; x++) {
            if (!line_ready || sweep_max[x] > line_max[x])
                line_max[x] = sweep_max[x];
            sweep_max[x] = 0;
        }
        line_ready = true;
    }
}

void GlassView::on_frame_sync() {
    sweep.on_frame();
    if (line_ready)
        draw_line();
}

void GlassView::draw_line() {
    // Sweeps can complete faster than frames: line_max holds the peak of all of
    // them. Handlers pumped below may start a new hold once line_ready is cleared.
    for (uint16_t xpos = 0; xpos < points; xpos++) {
        uint8_t power = line_max[xpos];
        if (power <= min_color_power)
            power = 0;                                                                                                    // Filtered out, show black
        spectrum_row[xpos] = gradient.lut[power];                                                                         // row of colors
        spectrum_data[xpos] = (live_frequency_integrate * spectrum_data[xpos] + power) / (live_frequency_integrate + 1);  // smoothing
    }
    line_ready = false;

    const int8_t beep_power = map(range_max_power, 0, 255, -100, 20);
    if (beep_power >= beep_squelch && chTimeNow() - last_beep >= 250) {
        baseband::request_audio_beep(map(range_max_power, 0, 256, 400, 2600), 24000, 250);
        last_beep = chTimeNow();
    }
    range_max_power = 0;

    if (live_frequency_view > 0) {
        constexpr int rssi_sample_range = 256;
        constexpr float rssi_voltage_min = 0.4;
        constexpr float rssi_voltage_max = 2.2;
        constexpr float adc_voltage_max = 3.3;
        constexpr int raw_min = 0.5f + rssi_sample_range * rssi_voltage_min / adc_voltage_max;
        constexpr int raw_max = 0.5f + rssi_sample_range * rssi_voltage_max / adc_voltage_max;
        const int spectrum_height = screen_height - spectrum_y;
        // drawing and keeping track of max freq
        for (uint16_t xpos = 0; xpos < points; xpos++) {
            // save max powerwull freq
            if (spectrum_data[xpos] > max_freq_power) {
                max_freq_power = spectrum_data[xpos];
                max_freq_hold = get_freq_from_bin_pos(xpos);
            }
            int16_t point = map(spectrum_data[xpos], 0, 255, 0, spectrum_height);
            // clear if not in peak view
            if (live_frequency_view != 2) {
                display.fill_rectangle({{xpos, spectrum_y}, {1, screen_height - point}}, {0, 0, 0});
            }
            display.fill_rectangle({{xpos, screen_height - point}, {1, point}}, gradient.lut[spectrum_data[xpos]]);
            // Let the sweep advance while we draw: retune requests wait in the queue otherwise.
            if ((xpos & 31) == 31)
                EventDispatcher::pump_application_queue();
        }
        // indicate RSSI min and max power
        if (show_rssi_guides) {
            display.fill_rectangle({{0, screen_height - map(raw_min, 0, 255, 0, spectrum_height)}, {screen_width, 1}}, -gradient.lut[raw_min]);
            display.fill_rectangle({{0, screen_height - map(raw_max, 0, 255, 0, spectrum_height)}, {screen_width, 1}}, -gradient.lut[raw_max]);
        }
        if (last_max_freq != max_freq_hold) {
            last_max_freq = max_freq_hold;
            freq_stats.set("MAX: " + to_string_short_freq(max_freq_hold));
        }
        plot_marker(marker_pixel_index);
    } else {
        display.draw_pixels({{0, display.scroll(1)}, {screen_width, 1}}, spectrum_row);  // new line at top, one less var, speedier
    }
}

void GlassView::on_hide() {
    sweep.stop();
    line_ready = false;
    display.scroll_disable();
}

void GlassView::on_show() {
    display.scroll_set_area(109, screen_height - 1);  // Restart scroll on the correct coordinates
    if (!paused) {
        std::fill(sweep_max.begin(), sweep_max.end(), 0);
        sweep.resume();
    }
}

void GlassView::on_range_changed() {
    reset_live_view();
    f_min = field_frequency_min.value();
    f_max = field_frequency_max.value();
    f_min = f_min * MHZ_DIV;  // Transpose into full frequency realm
    f_max = f_max * MHZ_DIV;
    if (f_max <= f_min)
        f_max = f_min + MHZ_DIV;
    looking_glass_range = f_max - f_min;
    search_span = looking_glass_range / MHZ_DIV;

    receiver_model.set_squelch_level(0);
    start_sweep();  // sets the sample rate and bandwidth for the range

    on_marker_change(0);  // after start_sweep: the marker frequency comes from the sweep
    update_range_field();
}

void GlassView::plot_marker(uint16_t pos) {
    uint16_t shift_y = 0;
    if (live_frequency_view > 0)  // plot one line down when in live view
    {
        shift_y = 16;
    }
    portapack::display.fill_rectangle({0, 100 + shift_y, screen_width, 8}, Theme::getInstance()->bg_darkest->background);  // Clear old marker and whole marker rectangle btw
    portapack::display.fill_rectangle({pos - 2, 100 + shift_y, 5, 3}, Theme::getInstance()->fg_red->foreground);           // Red marker top
    portapack::display.fill_rectangle({pos - 1, 103 + shift_y, 3, 3}, Theme::getInstance()->fg_red->foreground);           // Red marker middle
    portapack::display.fill_rectangle({pos, 106 + shift_y, 1, 2}, Theme::getInstance()->fg_red->foreground);               // Red marker bottom
}

void GlassView::update_min(int32_t v) {
    int32_t min_size = steps;
    if (locked_range)
        min_size = search_span;
    if (min_size < 1)
        min_size = 1;
    if (v > LOOKING_GLASS_MAX_FREQ_MHZ - min_size) {
        v = LOOKING_GLASS_MAX_FREQ_MHZ - min_size;
    }
    if (v > (field_frequency_max.value() - min_size))
        field_frequency_max.set_value(v + min_size, false);
    if (locked_range)
        field_frequency_max.set_value(v + min_size, false);
    else
        field_frequency_min.set_value(v, false);
}

void GlassView::update_max(int32_t v) {
    int32_t min_size = steps;
    if (locked_range)
        min_size = search_span;
    if (min_size < 1)
        min_size = 1;
    if (v < min_size) {
        v = min_size;
    }
    if (v > LOOKING_GLASS_MAX_FREQ_MHZ) {
        v = LOOKING_GLASS_MAX_FREQ_MHZ;
    }
    if (v < (field_frequency_min.value() + min_size))
        field_frequency_min.set_value(v - min_size, false);
    if (locked_range)
        field_frequency_min.set_value(v - min_size, false);
    else
        field_frequency_max.set_value(v, false);
}

void GlassView::update_range_field() {
    if (!locked_range) {
        field_range.set_style(Theme::getInstance()->bg_darkest);
        field_range.set_text(" " + to_string_dec_uint(search_span) + " ");
    } else {
        field_range.set_style(Theme::getInstance()->fg_red);
        field_range.set_text(">" + to_string_dec_uint(search_span) + "<");
    }
}

GlassView::GlassView(
    NavigationView& nav)
    : nav_(nav) {
    baseband::run_image(portapack::spi_flash::image_tag_wideband_spectrum);
    // The sweep waits for the M4 to discard samples until the synthesizers settle.
    radio::set_fast_retune(true);
    receiver_model.set_squelch_level(0);
    receiver_model.enable();

    points = std::min<uint16_t>(screen_width, spec_an::kPoints);
    sweep_max.resize(points);
    line_max.resize(points);
    spectrum_row.resize(screen_width);
    spectrum_data.resize(screen_width);
    if (!gradient.load_file(default_gradient_file)) {
        gradient.set_default();
    }

    add_children({&labels,
                  &field_frequency_min,
                  &field_frequency_max,
                  &field_lna,
                  &field_vga,
                  &field_range,
                  //&steps_config,
                  &scan_type,
                  &view_config,
                  &level_integration,
                  &field_volume,
                  &filter_config,
                  &field_rf_amp,
                  &range_presets,
                  &button_beep_squelch,
                  &field_marker,
                  &field_trigger,
                  &button_marker_minus,
                  &button_marker_plus,
                  &button_jump,
                  &button_rst,
                  &button_rssi,
                  &freq_stats, &button_play});

    load_presets();  // Load available presets from TXT files (or default).
    preset_index = clip<uint8_t>(preset_index, 0, presets_db.size());

    field_frequency_min.set_value(f_min / MHZ_DIV);
    field_frequency_min.on_change = [this](int32_t v) {
        range_presets.set_selected_index(0);  // Manual
        update_min(v);
        on_range_changed();
    };
    field_frequency_min.on_select = [this, &nav](NumberField& field) {
        auto new_view = nav_.push<FrequencyKeypadView>(field_frequency_min.value() * MHZ_DIV);
        new_view->on_changed = [this, &field](rf::Frequency f) {
            field_frequency_min.set_value(f / MHZ_DIV);
        };
    };

    field_frequency_max.set_value(f_max / MHZ_DIV);
    field_frequency_max.on_change = [this](int32_t v) {
        range_presets.set_selected_index(0);  // Manual
        update_max(v);
        on_range_changed();
    };
    field_frequency_max.on_select = [this, &nav](NumberField& field) {
        auto new_view = nav_.push<FrequencyKeypadView>(field_frequency_max.value() * MHZ_DIV);
        new_view->on_changed = [this, &field](rf::Frequency f) {
            field_frequency_max.set_value(f / MHZ_DIV);
        };
    };

    /*steps_config.on_change = [this](size_t, OptionsField::value_t v) {
        field_frequency_min.set_step(v);
        field_frequency_max.set_step(v);
        steps = v;
    };
    steps_config.set_selected_index(0);  // 1 Mhz step.*/

    scan_type.on_change = [this](size_t, OptionsField::value_t v) {
        mode = v;
        on_range_changed();
    };
    if (mode > LOOKING_GLASS_SLOWSCAN)
        mode = LOOKING_GLASS_FASTSCAN;  // older settings may hold the removed single pass mode
    scan_type.set_selected_index(mode);

    view_config.on_change = [this](size_t, OptionsField::value_t v) {
        reset_live_view();  // Clear between changes.
        live_frequency_view = v;

        switch (v) {
            case 0:  // SPEC
                level_integration.hidden(true);
                freq_stats.hidden(true);
                button_jump.hidden(true);
                button_rst.hidden(true);
                button_rssi.hidden(true);
                display.scroll_set_area(109, screen_height - 1);  // Restart scroll on the correct coordinates.
                break;

            case 1:  // LEVEL
                display.fill_rectangle({{0, 108}, {screen_width, 24}}, {0, 0, 0});
                display.scroll_disable();
                level_integration.hidden(false);
                freq_stats.hidden(false);
                button_jump.hidden(false);
                button_rst.hidden(false);
                button_rssi.hidden(false);
                break;

            case 2:  // PEAK
            default:
                display.fill_rectangle({{0, 108}, {screen_width, 24}}, {0, 0, 0});
                display.scroll_disable();
                level_integration.hidden(false);
                freq_stats.hidden(false);
                button_jump.hidden(false);
                button_rst.hidden(false);
                button_rssi.hidden(false);
                break;
        }

        set_dirty();
    };
    view_config.set_selected_index(live_frequency_view);

    level_integration.on_change = [this](size_t, OptionsField::value_t v) {
        reset_live_view();
        live_frequency_integrate = v;
    };
    level_integration.set_selected_index(live_frequency_integrate);

    filter_config.on_change = [this](size_t ix, OptionsField::value_t v) {
        reset_live_view();
        min_color_power = v;
        filter_index = ix;
    };
    filter_config.set_selected_index(filter_index);

    range_presets.on_change = [this](size_t ix, OptionsField::value_t v) {
        preset_index = ix;
        if (ix == 0) return;  // Don't update range for "Manual".

        // NB: Don't trigger updates, presets directly set the range
        // values without applying step or range lock.
        field_frequency_min.set_value(presets_db[v].min, false);
        field_frequency_max.set_value(presets_db[v].max, false);
        on_range_changed();
    };
    range_presets.set_selected_index(preset_index);

    field_marker.on_encoder_change = [this](TextField&, EncoderEvent delta) {
        on_marker_change(delta);
    };

    field_marker.on_select = [this](TextField&) {
        // Launch Audio with marker frequency.
        launch_audio(marker);
    };

    field_trigger.on_change = [this](int32_t v) {
        trigger = v;
        start_sweep();
    };
    field_trigger.set_value(trigger);

    field_range.on_select = [this](TextField&) {
        locked_range = !locked_range;
        update_range_field();
    };

    button_marker_minus.on_select = [this](Button&) {
        on_marker_change(-1);
    };

    button_marker_plus.on_select = [this](Button&) {
        on_marker_change(1);
    };

    button_jump.on_select = [this](Button&) {
        // Launch Audio with peak frequency.
        launch_audio(max_freq_hold);
    };

    button_rst.on_select = [this](Button&) {
        reset_live_view();
    };

    button_rssi.on_select = [this](Button& button) {
        show_rssi_guides = !show_rssi_guides;
        if (show_rssi_guides) {
            button.set_style(Theme::getInstance()->fg_green);
        } else {
            button.set_style(Theme::getInstance()->bg_darkest);
        }
        reset_live_view();
    };

    button_play.on_select = [this](ImageButton&) {
        paused = !paused;
        if (paused) {
            button_play.set_foreground(Theme::getInstance()->fg_green->foreground);
            button_play.set_background(Theme::getInstance()->fg_green->background);
            sweep.stop();
            button_play.set_bitmap(&bitmap_play);
        } else {
            button_play.set_foreground(Theme::getInstance()->fg_red->foreground);
            button_play.set_background(Theme::getInstance()->fg_red->background);
            std::fill(sweep_max.begin(), sweep_max.end(), 0);
            sweep.resume();
            button_play.set_bitmap(&bitmap_stop);
        }
    };

    display.scroll_set_area(109, screen_height - 1);  // Restart scroll on the correct coordinates

    // trigger: number of FFTs power-averaged per tune is trigger / 8 (at least 1).
    marker_pixel_index = points / 2;
    sweep_enabled = true;
    on_range_changed();  // Force a UI update, starts the sweep.

    button_beep_squelch.on_select = [this](ButtonWithEncoder& button) {
        (void)button;
        beep_enabled = 1 - beep_enabled;
        manage_beep_audio();
        update_display_beep();
    };

    button_beep_squelch.on_change = [this]() {
        int new_beep_squelch = beep_squelch + button_beep_squelch.get_encoder_delta();
        if (new_beep_squelch < -99)
            new_beep_squelch = -99;
        if (new_beep_squelch > 20)
            new_beep_squelch = 20;
        beep_squelch = new_beep_squelch;
        button_beep_squelch.set_encoder_delta(0);
        update_display_beep();
    };

    manage_beep_audio();
    update_display_beep();
}

void GlassView::on_freqchg(int64_t freq) {
    int64_t half_range = abs(field_frequency_max.value() - field_frequency_min.value()) / 2;
    if (half_range < 1) {
        half_range = 1;
    }
    const int64_t center = freq / MHZ_DIV;  // freq is in Hz, the fields in MHz
    range_presets.set_selected_index(0);    // Manual
    update_min(std::max<int64_t>(center - half_range, 0));
    update_max(center + half_range);
    on_range_changed();
}

void GlassView::load_presets() {
    presets_db.clear();

    // Add the "Manual" entry.
    presets_db.push_back({0, 0, "Manual"});

    scan_root_files(looking_glass_dir, u"*.TXT", [this](const std::filesystem::path& path) {
        if (path.empty() || path.native()[0] == u'.')
            return;

        File presets_file;
        auto error = presets_file.open(looking_glass_dir / path);
        if (!error) {
            auto reader = FileLineReader(presets_file);
            for (const auto& line : reader) {
                if (line.length() == 0 || line[0] == '#')
                    continue;

                auto cols = split_string(line, ',');
                if (cols.size() != 3)
                    continue;

                preset_entry entry{};
                parse_int(cols[0], entry.min);
                parse_int(cols[1], entry.max);
                entry.label = trimr(cols[2]);

                if (entry.max == 0 || entry.min >= entry.max)
                    continue;  // Invalid line.

                presets_db.emplace_back(std::move(entry));
            }
        }
    });

    populate_presets();
}

void GlassView::populate_presets() {
    using option_t = std::pair<std::string, int32_t>;
    using options_t = std::vector<option_t>;
    options_t entries;

    for (const auto& preset : presets_db)
        entries.emplace_back(preset.label, entries.size());

    range_presets.set_options(std::move(entries));
}

void GlassView::launch_audio(rf::Frequency center_freq) {
    receiver_model.set_target_frequency(center_freq);
    auto settings = receiver_model.settings();
    settings.frequency_step = MHZ_DIV;        // Preset a 1 MHz frequency step into RX -> AUDIO
    nav_.replace<AnalogAudioView>(settings);  // Jump into audio view
}

}  // namespace ui
