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

using namespace portapack;

namespace ui {
void GlassView::focus() {
    button_marker.focus();
}

GlassView::~GlassView() {
    receiver_model.set_sampling_rate(3072000);  // Just a hack to avoid hanging other apps
    receiver_model.disable();
    baseband::shutdown();
}

void GlassView::get_max_power(const ChannelSpectrum& spectrum, uint8_t bin, uint8_t& max_power) {
    if (mode == LOOKING_GLASS_SINGLEPASS) {
        // analog audio app like view
        if (bin < 120) {
            if (spectrum.db[SPEC_NB_BINS - 120 + bin] > max_power)
                max_power = spectrum.db[SPEC_NB_BINS - 120 + bin];
        } else {
            if (spectrum.db[bin - 120] > max_power)
                max_power = spectrum.db[bin - 120];
        }
    } else if (mode == LOOKING_GLASS_FASTSCAN) {
        // view is made in multiple pass, use original bin picking
        // Center 12 bins are ignored (DC spike is blanked) Leftmost and rightmost 2 bins are ignored
        if (bin < 120) {
            if (spectrum.db[SPEC_NB_BINS - 2 - 120 + bin] > max_power)
                max_power = spectrum.db[SPEC_NB_BINS - 2 - 120 + bin];
        } else {
            if (spectrum.db[2 + bin - 120] > max_power)
                max_power = spectrum.db[2 + bin - 120];
        }
    } else  // if( mode == LOOKING_GLASS_SLOWSCAN )
    {
        if (bin < 120) {
            if (spectrum.db[SPEC_NB_BINS - offset - 120 + bin] > max_power)
                max_power = spectrum.db[SPEC_NB_BINS - offset - 120 + bin];
        } else {
            if (spectrum.db[offset + bin - 120] > max_power)
                max_power = spectrum.db[offset + bin - 120];
        }
    }
}

void GlassView::on_marker_change() {
    if (mode == LOOKING_GLASS_SINGLEPASS) {
        marker = f_min + (marker_pixel_index * looking_glass_range) / SCREEN_W;
    } else  // if( mode == LOOKING_GLASS_SLOWSCAN || mode == LOOKING_GLASS_FASTSCAN )
    {
        marker = f_min + (offset * each_bin_size) + (marker_pixel_index * looking_glass_range) / SCREEN_W;
    }
    button_marker.set_text(to_string_short_freq(marker));
    PlotMarker(marker_pixel_index);  // Refresh marker on screen
}

// Returns the next multiple of num that is a multiple of multiplier
int64_t GlassView::next_mult_of(int64_t num, int64_t multiplier) {
    return ((num / multiplier) + 1) * multiplier;
}

void GlassView::adjust_range(int64_t* f_min, int64_t* f_max, int64_t width) {
    int64_t span = *f_max - *f_min;
    int64_t num_intervals = span / width;
    if (span % width != 0) {
        num_intervals++;
    }
    int64_t new_span = num_intervals * width;
    int64_t delta_span = (new_span - span) / 2;
    *f_min -= delta_span;
    *f_max += delta_span;
}

void GlassView::retune() {
    // Start a new sweep.
    // Tune rx for this new slice directly because the model
    // saves to persistent memory which is slower.
    radio::set_tuning_frequency(f_center);
    chThdSleepMilliseconds(5);
    baseband::spectrum_streaming_start();  // Do the RX
}

void GlassView::on_lna_changed(int32_t v_db) {
    receiver_model.set_lna(v_db);
}

void GlassView::on_vga_changed(int32_t v_db) {
    receiver_model.set_vga(v_db);
}

void GlassView::reset_live_view(bool clear_screen) {
    max_freq_hold = 0;
    max_freq_power = -1000;
    if (clear_screen) {
        // only clear screen in peak mode
        if (live_frequency_view == 2) {
            display.fill_rectangle({{0, 108 + 16}, {SCREEN_W, 320 - (108 + 16)}}, {0, 0, 0});
        }
    }
}

void GlassView::add_spectrum_pixel(uint8_t power) {
    spectrum_row[pixel_index] = spectrum_rgb3_lut[power];                                                                           // row of colors
    spectrum_data[pixel_index] = (live_frequency_integrate * spectrum_data[pixel_index] + power) / (live_frequency_integrate + 1);  // smoothing
    pixel_index++;

    if (pixel_index == SCREEN_W)  // got an entire waterfall line
    {
        if (live_frequency_view > 0) {
            constexpr int rssi_sample_range = SPEC_NB_BINS;
            constexpr float rssi_voltage_min = 0.4;
            constexpr float rssi_voltage_max = 2.2;
            constexpr float adc_voltage_max = 3.3;
            constexpr int raw_min = rssi_sample_range * rssi_voltage_min / adc_voltage_max;
            constexpr int raw_max = rssi_sample_range * rssi_voltage_max / adc_voltage_max;
            constexpr int raw_delta = raw_max - raw_min;
            const range_t<int> y_max_range{0, 320 - (108 + 16)};

            // drawing and keeping track of max freq
            for (uint16_t xpos = 0; xpos < SCREEN_W; xpos++) {
                // save max powerwull freq
                if (spectrum_data[xpos] > max_freq_power) {
                    max_freq_power = spectrum_data[xpos];
                    if (mode == LOOKING_GLASS_SINGLEPASS) {
                        max_freq_hold = f_min + (xpos * looking_glass_range) / SCREEN_W;
                    } else  // if( mode == LOOKING_GLASS_SLOWSCAN || mode == LOOKING_GLASS_FASTSCAN )
                    {
                        max_freq_hold = f_min + (offset * each_bin_size) + (xpos * looking_glass_range) / SCREEN_W;
                    }
                }
                int16_t point = y_max_range.clip(((spectrum_data[xpos] - raw_min) * (320 - (108 + 16))) / raw_delta);
                uint8_t color_gradient = (point * 255) / 212;
                // clear if not in peak view
                if (live_frequency_view != 2) {
                    display.fill_rectangle({{xpos, 108 + 16}, {1, 320 - point}}, {0, 0, 0});
                }
                display.fill_rectangle({{xpos, 320 - point}, {1, point}}, {color_gradient, 0, uint8_t(255 - color_gradient)});
            }
            if (last_max_freq != max_freq_hold) {
                last_max_freq = max_freq_hold;
                freq_stats.set("MAX HOLD: " + to_string_short_freq(max_freq_hold));
            }
            PlotMarker(marker_pixel_index);
        } else {
            display.draw_pixels({{0, display.scroll(1)}, {SCREEN_W, 1}}, spectrum_row);  // new line at top, one less var, speedier
        }
        pixel_index = 0;  // Start New cascade line
    }
}

// Apparently, the spectrum object returns an array of SPEC_NB_BINS (256) bins
// Each having the radio signal power for it's corresponding frequency slot
void GlassView::on_channel_spectrum(const ChannelSpectrum& spectrum) {
    baseband::spectrum_streaming_stop();
    // Convert bins of this spectrum slice into a representative max_power and when enough, into pixels
    // we actually need SCREEN_W (240) of those bins
    for (bin = offset; bin < bin_length + offset; bin++) {
        get_max_power(spectrum, bin, max_power);
        if (ignore_dc && bin == 119) {
            uint8_t next_max_power = 0;
            get_max_power(spectrum, 120, next_max_power);
            bins_Hz_size += 12 * each_bin_size;  // add the ignored DC spike to "pixel fulfilled bag of Hz"
            max_power = (max_power + next_max_power) / 2;
        }
        bins_Hz_size += each_bin_size;  // add this bin Hz count into the "pixel fulfilled bag of Hz"

        if (bins_Hz_size >= marker_pixel_step)  // new pixel fullfilled
        {
            if (min_color_power < max_power)
                add_spectrum_pixel(max_power);  // Pixel will represent max_power
            else
                add_spectrum_pixel(0);  // Filtered out, show black

            max_power = 0;

            if (!pixel_index)  // Received indication that a waterfall line has been completed
            {
                bins_Hz_size = 0;  // Since this is an entire pixel line, we don't carry "Pixels into next bin"
                f_center = f_center_ini;
                retune();
                return;  // signal a new line
            }
            bins_Hz_size -= marker_pixel_step;  // reset bins size, but carrying the eventual excess Hz into next pixel
        }
    }
    f_center += looking_glass_step;
    retune();
}

void GlassView::on_hide() {
    baseband::spectrum_streaming_stop();
    display.scroll_disable();
}

void GlassView::on_show() {
    display.scroll_set_area(109, 319);  // Restart scroll on the correct coordinates
    baseband::spectrum_streaming_start();
}

void GlassView::on_range_changed() {
    reset_live_view(false);
    f_min = field_frequency_min.value();
    f_max = field_frequency_max.value();
    f_min = f_min * MHZ_DIV;  // Transpose into full frequency realm
    f_max = f_max * MHZ_DIV;
    looking_glass_range = f_max - f_min;
    if (looking_glass_range < LOOKING_GLASS_SLICE_WIDTH_MAX) {
        mode = LOOKING_GLASS_SINGLEPASS;
    } else {
        mode = scan_type.selected_index_value();
    }

    if (mode == LOOKING_GLASS_SINGLEPASS) {
        // if the view is done in one pass, show it like in analog_audio_app
        offset = 0;
        bin_length = SCREEN_W;
        ignore_dc = 0;
    } else if (mode == LOOKING_GLASS_FASTSCAN) {
        // view is made in multiple pass, use original bin picking
        offset = 0;
        bin_length = SCREEN_W;
        ignore_dc = 1;
    } else  // if( mode == LOOKING_GLASS_SLOWSCAN )
    {
        offset = 16;
        bin_length = 80;
        ignore_dc = 0;
    }
    each_bin_size = looking_glass_bandwidth / SPEC_NB_BINS;
    if (mode != LOOKING_GLASS_SINGLEPASS) {
        looking_glass_step = (bin_length + (ignore_dc * 12)) * each_bin_size;
    } else {
        looking_glass_step = SPEC_NB_BINS * each_bin_size;
    }
    adjust_range(&f_min, &f_max, SCREEN_W);
    looking_glass_range = f_max - f_min;
    marker_pixel_step = looking_glass_range / SCREEN_W;  // Each pixel value in Hz
    search_span = looking_glass_range / MHZ_DIV;
    button_range.set_text("      ");  // clear up to 6 chars
    if (locked_range) {
        button_range.set_text(">" + to_string_dec_uint(search_span) + "<");
    } else {
        button_range.set_text(" " + to_string_dec_uint(search_span) + " ");
    }

    pixel_index = 0;   // reset pixel counter
    max_power = 0;     // reset save max power level
    bins_Hz_size = 0;  // reset amount of Hz filled up by pixels

    if (mode == LOOKING_GLASS_SINGLEPASS) {
        looking_glass_bandwidth = looking_glass_range;
        looking_glass_sampling_rate = looking_glass_bandwidth / 2;
        each_bin_size = looking_glass_bandwidth / SCREEN_W;
    } else  // if ( mode == LOOKING_GLASS_SLOWSCAN || mode == LOOKING_GLASS_FASTSCAN )
    {
        looking_glass_sampling_rate = LOOKING_GLASS_SLICE_WIDTH_MAX;
        looking_glass_bandwidth = LOOKING_GLASS_SLICE_WIDTH_MAX;
        each_bin_size = looking_glass_bandwidth / SPEC_NB_BINS;
    }

    on_marker_change();

    // set the sample rate and bandwidth
    receiver_model.set_sampling_rate(looking_glass_sampling_rate);
    receiver_model.set_baseband_bandwidth(looking_glass_bandwidth);

    receiver_model.set_squelch_level(0);
    f_center_ini = f_min + (looking_glass_bandwidth / 2);  // Initial center frequency for sweep
    f_center = f_center_ini;                               // Reset sweep into first slice
    baseband::set_spectrum(looking_glass_sampling_rate, field_trigger.value());
    receiver_model.set_tuning_frequency(f_center);  // tune rx for this slice
}

void GlassView::PlotMarker(uint8_t pos) {
    uint8_t shift_y = 0;
    if (live_frequency_view > 0)  // plot one line down when in live view
    {
        shift_y = 16;
    }
    portapack::display.fill_rectangle({0, 100 + shift_y, SCREEN_W, 8}, Color::black());  // Clear old marker and whole marker rectangle btw
    portapack::display.fill_rectangle({pos - 2, 100 + shift_y, 5, 3}, Color::red());     // Red marker top
    portapack::display.fill_rectangle({pos - 1, 103 + shift_y, 3, 3}, Color::red());     // Red marker middle
    portapack::display.fill_rectangle({pos, 106 + shift_y, 1, 2}, Color::red());         // Red marker bottom
}

GlassView::GlassView(
    NavigationView& nav)
    : nav_(nav) {
    baseband::run_image(portapack::spi_flash::image_tag_wideband_spectrum);

    add_children({&labels,
                  &field_frequency_min,
                  &field_frequency_max,
                  &field_lna,
                  &field_vga,
                  &button_range,
                  &steps_config,
                  &scan_type,
                  &view_config,
                  &level_integration,
                  &filter_config,
                  &field_rf_amp,
                  &range_presets,
                  &button_marker,
                  &field_trigger,
                  &button_jump,
                  &button_rst,
                  &freq_stats});

    load_Presets();  // Load available presets from TXT files (or default)

    field_frequency_min.on_change = [this](int32_t v) {
        reset_live_view(true);
        int32_t min_size = steps;
        if (locked_range)
            min_size = search_span;
        if (min_size < 2)
            min_size = 2;
        if (v > 7200 - min_size) {
            v = 7200 - min_size;
            field_frequency_min.set_value(v);
        }
        if (v > (field_frequency_max.value() - min_size))
            field_frequency_max.set_value(v + min_size);
        if (locked_range)
            field_frequency_max.set_value(v + min_size);
        on_range_changed();
    };
    field_frequency_min.set_value(presets_db[0].min);  // Defaults to first preset
    field_frequency_min.set_step(steps);

    field_frequency_min.on_select = [this, &nav](NumberField& field) {
        auto new_view = nav_.push<FrequencyKeypadView>(field_frequency_min.value() * MHZ_DIV);
        new_view->on_changed = [this, &field](rf::Frequency f) {
            int32_t freq = f / MHZ_DIV;
            int32_t min_size = steps;
            if (locked_range)
                min_size = search_span;
            if (min_size < 2)
                min_size = 2;
            if (freq > (7200 - min_size))
                freq = 7200 - min_size;
            field_frequency_min.set_value(freq);
            if (field_frequency_max.value() < (freq + min_size))
                field_frequency_max.set_value(freq + min_size);
            on_range_changed();
        };
    };

    field_frequency_max.on_change = [this](int32_t v) {
        reset_live_view(true);
        int32_t min_size = steps;
        if (locked_range)
            min_size = search_span;
        if (min_size < 2)
            min_size = 2;
        if (v < min_size) {
            v = min_size;
            field_frequency_max.set_value(v);
        }
        if (v < (field_frequency_min.value() + min_size))
            field_frequency_min.set_value(v - min_size);
        if (locked_range)
            field_frequency_min.set_value(v - min_size);
        on_range_changed();
    };
    field_frequency_max.set_value(presets_db[0].max);  // Defaults to first preset
    field_frequency_max.set_step(steps);

    field_frequency_max.on_select = [this, &nav](NumberField& field) {
        auto new_view = nav_.push<FrequencyKeypadView>(field_frequency_max.value() * MHZ_DIV);
        new_view->on_changed = [this, &field](rf::Frequency f) {
            int32_t min_size = steps;
            if (locked_range)
                min_size = search_span;
            if (min_size < 2)
                min_size = 2;
            int32_t freq = f / MHZ_DIV;
            if (freq < min_size)
                freq = min_size;
            field_frequency_max.set_value(freq);
            if (field_frequency_min.value() > (freq - min_size))
                field_frequency_min.set_value(freq - min_size);
            on_range_changed();
        };
    };

    field_lna.on_change = [this](int32_t v) {
        reset_live_view(true);
        this->on_lna_changed(v);
    };
    field_lna.set_value(receiver_model.lna());

    field_vga.on_change = [this](int32_t v_db) {
        reset_live_view(true);
        this->on_vga_changed(v_db);
    };
    field_vga.set_value(receiver_model.vga());

    steps_config.on_change = [this](size_t n, OptionsField::value_t v) {
        (void)n;
        field_frequency_min.set_step(v);
        field_frequency_max.set_step(v);
        steps = v;
    };
    steps_config.set_selected_index(0);  // default of 1 Mhz steps

    scan_type.on_change = [this](size_t n, OptionsField::value_t v) {
        (void)n;
        mode = v;
        on_range_changed();
    };
    scan_type.set_selected_index(0);  // default legacy fast scan

    view_config.on_change = [this](size_t n, OptionsField::value_t v) {
        (void)n;
        // clear between changes
        reset_live_view(true);
        if (v == 0) {
            live_frequency_view = 0;
            level_integration.hidden(true);
            freq_stats.hidden(true);
            button_jump.hidden(true);
            button_rst.hidden(true);
            display.scroll_set_area(109, 319);  // Restart scroll on the correct coordinates
        } else if (v == 1) {
            display.fill_rectangle({{0, 108}, {SCREEN_W, 24}}, {0, 0, 0});
            live_frequency_view = 1;
            display.scroll_disable();
            level_integration.hidden(false);
            freq_stats.hidden(false);
            button_jump.hidden(false);
            button_rst.hidden(false);
        } else if (v == 2) {
            display.fill_rectangle({{0, 108}, {SCREEN_W, 24}}, {0, 0, 0});
            live_frequency_view = 2;
            display.scroll_disable();
            level_integration.hidden(false);
            freq_stats.hidden(false);
            button_jump.hidden(false);
            button_rst.hidden(false);
        }
        set_dirty();
    };
    view_config.set_selected_index(0);  // default spectrum

    level_integration.on_change = [this](size_t n, OptionsField::value_t v) {
        (void)n;
        reset_live_view(true);
        live_frequency_integrate = v;
    };
    level_integration.set_selected_index(3);  // default integration of ( 3 * old value + new_value ) / 4

    filter_config.on_change = [this](size_t n, OptionsField::value_t v) {
        (void)n;
        reset_live_view(true);
        min_color_power = v;
    };
    filter_config.set_selected_index(0);

    range_presets.on_change = [this](size_t n, OptionsField::value_t v) {
        (void)n;
        field_frequency_min.set_value(presets_db[v].min, false);
        field_frequency_max.set_value(presets_db[v].max, false);
        on_range_changed();
    };

    button_marker.on_change = [this]() {
        if (((int)marker_pixel_index + button_marker.get_encoder_delta()) < 0) {
            marker_pixel_index = 0;
        } else if (((int)marker_pixel_index + button_marker.get_encoder_delta()) > SCREEN_W) {
            marker_pixel_index = SCREEN_W;
        } else {
            marker_pixel_index = marker_pixel_index + button_marker.get_encoder_delta();
        }
        on_marker_change();
        button_marker.set_encoder_delta(0);
    };

    button_marker.on_select = [this](ButtonWithEncoder&) {
        receiver_model.set_tuning_frequency(marker);  // Center tune rx in marker freq.
        receiver_model.set_frequency_step(MHZ_DIV);   // Preset a 1 MHz frequency step into RX -> AUDIO
        nav_.pop();
        nav_.push<AnalogAudioView>();  // Jump into audio view
    };

    field_trigger.on_change = [this](int32_t v) {
        baseband::set_spectrum(looking_glass_sampling_rate, v);
    };
    field_trigger.set_value(32);  // Defaults to 32, as normal triggering resolution

    button_range.on_select = [this](Button&) {
        if (locked_range) {
            locked_range = false;
            button_range.set_style(&style_white);
            button_range.set_text(" " + to_string_dec_uint(search_span) + " ");
        } else {
            locked_range = true;
            button_range.set_style(&style_red);
            button_range.set_text(">" + to_string_dec_uint(search_span) + "<");
        }
    };

    button_jump.on_select = [this](Button&) {
        receiver_model.set_tuning_frequency(max_freq_hold);  // Center tune rx in marker freq.
        receiver_model.set_frequency_step(MHZ_DIV);          // Preset a 1 MHz frequency step into RX -> AUDIO
        nav_.pop();
        nav_.push<AnalogAudioView>();  // Jump into audio view
    };

    button_rst.on_select = [this](Button&) {
        reset_live_view(true);
    };

    display.scroll_set_area(109, 319);
    baseband::set_spectrum(looking_glass_sampling_rate, field_trigger.value());  // trigger:
                                                                                 // Discord User jteich:  WidebandSpectrum::on_message to set the trigger value. In WidebandSpectrum::execute ,
                                                                                 // it keeps adding the output of the fft to the buffer until "trigger" number of calls are made,
                                                                                 // at which time it pushes the buffer up with channel_spectrum.feed

    marker_pixel_index = 120;
    on_range_changed();

    receiver_model.set_modulation(ReceiverModel::Mode::SpectrumAnalysis);
    receiver_model.set_sampling_rate(looking_glass_sampling_rate);   // 20mhz
    receiver_model.set_baseband_bandwidth(looking_glass_bandwidth);  // possible values: 1.75/2.5/3.5/5/5.5/6/7/8/9/10/12/14/15/20/24/28MHz
    receiver_model.set_squelch_level(0);
    receiver_model.enable();
}

void GlassView::load_Presets() {
    File presets_file;  // LOAD /WHIPCALC/ANTENNAS.TXT from microSD
    auto result = presets_file.open("LOOKINGGLASS/PRESETS.TXT");
    presets_db.clear();  // Start with fresh db
    if (result.is_valid()) {
        presets_Default();  // There is no txt, store a default range
    } else {
        std::string line;  // There is a txt file
        char one_char[1];  // Read it char by char
        for (size_t pointer = 0; pointer < presets_file.size(); pointer++) {
            presets_file.seek(pointer);
            presets_file.read(one_char, 1);
            if ((int)one_char[0] > 31) {       // ascii space upwards
                line += one_char[0];           // Add it to the textline
            } else if (one_char[0] == '\n') {  // New Line
                txtline_process(line);         // make sense of this textline
                line.clear();                  // Ready for next textline
            }
        }
        if (line.length() > 0)
            txtline_process(line);  // Last line had no newline at end ?
        if (!presets_db.size())
            presets_Default();  // no antenna on txt, use default
    }
    populate_Presets();
}

void GlassView::txtline_process(std::string& line) {
    if (line.find("#") != std::string::npos)
        return;  // Line is just a comment

    size_t comma = line.find(",");  // Get first comma position
    if (comma == std::string::npos)
        return;  // No comma at all

    size_t previous = 0;
    preset_entry new_preset;

    new_preset.min = std::stoi(line.substr(0, comma));
    if (!new_preset.min)
        return;  // No frequency!

    previous = comma + 1;
    comma = line.find(",", previous);  // Search for next delimiter
    if (comma == std::string::npos)
        return;  // No comma at all

    new_preset.max = std::stoi(line.substr(previous, comma - previous));
    if (!new_preset.max)
        return;  // No frequency!

    new_preset.label = line.substr(comma + 1);
    if (new_preset.label.size() == 0)
        return;  // No label ?

    presets_db.push_back(new_preset);  // Add this preset.
}

void GlassView::populate_Presets() {
    using option_t = std::pair<std::string, int32_t>;
    using options_t = std::vector<option_t>;
    options_t entries;

    for (preset_entry preset : presets_db) {  // go thru all available presets
        entries.emplace_back(preset.label, entries.size());
    }
    range_presets.set_options(entries);
}

void GlassView::presets_Default() {
    presets_db.clear();
    presets_db.push_back({2320, 2560, "DEFAULT WIFI 2.4GHz"});
}

}  // namespace ui
