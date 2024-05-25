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
#include "string_format.hpp"
#include "audio.hpp"
#include "file_path.hpp"

using namespace portapack;

namespace ui {
void GlassView::focus() {
    range_presets.focus();
}

GlassView::~GlassView() {
    audio::output::stop();
    receiver_model.set_sampling_rate(3072000);  // Just a hack to avoid hanging other apps
    receiver_model.disable();
    baseband::shutdown();
}

// Function to map the value from one range to another
int32_t GlassView::map(int32_t value, int32_t fromLow, int32_t fromHigh, int32_t toLow, int32_t toHigh) {
    return toLow + (value - fromLow) * (toHigh - toLow) / (fromHigh - fromLow);
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

void GlassView::get_max_power(const ChannelSpectrum& spectrum, uint8_t bin, uint8_t& max_power) {
    if (mode == LOOKING_GLASS_SINGLEPASS) {
        // <20MHz spectrum mode
        if (bin < 120) {
            if (spectrum.db[SPEC_NB_BINS - 120 + bin] > max_power)
                max_power = spectrum.db[SPEC_NB_BINS - 120 + bin];
        } else {
            if (spectrum.db[bin - 120] > max_power)
                max_power = spectrum.db[bin - 120];
        }
    } else {
        // FAST or SLOW mode
        if (bin < 120) {
            if (spectrum.db[134 + bin] > max_power)
                max_power = spectrum.db[134 + bin];
        } else {
            if (spectrum.db[bin - 118] > max_power)
                max_power = spectrum.db[bin - 118];
        }
    }
}

rf::Frequency GlassView::get_freq_from_bin_pos(uint8_t pos) {
    rf::Frequency freq_at_pos = 0;
    if (mode == LOOKING_GLASS_SINGLEPASS) {
        // starting from the middle, minus 8 ignored bin on each side. Since pos is [-120,120] after the (pos - 120), it's divided by SCREEN_W(240)/2 => 120
        freq_at_pos = f_center_ini + ((pos - 120) * ((looking_glass_range - ((16 * looking_glass_range) / SPEC_NB_BINS)) / 2)) / (SCREEN_W / 2);
    } else
        freq_at_pos = f_min + (2 * offset * each_bin_size) + (pos * looking_glass_range) / SCREEN_W;

    return freq_at_pos;
}

void GlassView::on_marker_change() {
    marker = get_freq_from_bin_pos(marker_pixel_index);
    field_marker.set_text(to_string_short_freq(marker));
    plot_marker(marker_pixel_index);  // Refresh marker on screen
}

void GlassView::retune() {
    // Start a new sweep.
    // Tune rx for this new slice directly because the model
    // saves to persistent memory which is slower.
    radio::set_tuning_frequency(f_center);
    chThdSleepMilliseconds(5);             // stabilize freq
    baseband::spectrum_streaming_start();  // Do the RX
}

void GlassView::reset_live_view() {
    max_freq_hold = 0;
    max_freq_power = -1000;

    // Clear screen in peak mode.
    if (live_frequency_view == 2)
        display.fill_rectangle({{0, 108 + 16}, {SCREEN_W, SCREEN_H - (108 + 16)}}, {0, 0, 0});
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
                    max_freq_hold = get_freq_from_bin_pos(xpos);
                }
                int16_t point = y_max_range.clip(((spectrum_data[xpos] - raw_min) * (320 - (108 + 16))) / raw_delta);
                uint8_t color_gradient = (point * 255) / 212;
                // clear if not in peak view
                if (live_frequency_view != 2) {
                    display.fill_rectangle({{xpos, 108 + 16}, {1, SCREEN_H - point}}, {0, 0, 0});
                }
                display.fill_rectangle({{xpos, SCREEN_H - point}, {1, point}}, {color_gradient, 0, uint8_t(255 - color_gradient)});
            }
            if (last_max_freq != max_freq_hold) {
                last_max_freq = max_freq_hold;
                freq_stats.set("MAX HOLD: " + to_string_short_freq(max_freq_hold));
            }
            plot_marker(marker_pixel_index);
        } else {
            display.draw_pixels({{0, display.scroll(1)}, {SCREEN_W, 1}}, spectrum_row);  // new line at top, one less var, speedier
        }
        pixel_index = 0;  // Start New cascade line
    }
}

bool GlassView::process_bins(uint8_t* powerlevel) {
    bins_hz_size += each_bin_size;          // add pixel to fulfilled bag of Hz
    if (bins_hz_size >= marker_pixel_step)  // new pixel fullfilled
    {
        if (*powerlevel > min_color_power)
            add_spectrum_pixel(*powerlevel);  // Pixel will represent max_power
        else
            add_spectrum_pixel(0);  // Filtered out, show black
        *powerlevel = 0;

        if (!pixel_index)  // Received indication that a waterfall line has been completed
        {
            bins_hz_size = 0;  // Since this is an entire pixel line, we don't carry "Pixels into next bin"
            if (mode != LOOKING_GLASS_SINGLEPASS) {
                f_center = f_center_ini;
                retune();
            } else
                baseband::spectrum_streaming_start();
            return true;  // signal a new line
        }
        bins_hz_size -= marker_pixel_step;  // reset bins size, but carrying the eventual excess Hz into next pixel
    }
    return false;
}

// Apparently, the spectrum object returns an array of SPEC_NB_BINS (256) bins
// Each having the radio signal power for its corresponding frequency slot
void GlassView::on_channel_spectrum(const ChannelSpectrum& spectrum) {
    baseband::spectrum_streaming_stop();
    // Convert bins of this spectrum slice into a representative max_power and when enough, into pixels
    // we actually need SCREEN_W (240) of those bins
    for (uint8_t bin = 0; bin < bin_length; bin++) {
        get_max_power(spectrum, bin, max_power);
        if (max_power > range_max_power)
            range_max_power = max_power;
        // process dc spike if enable
        if (bin == 119) {
            uint8_t next_max_power = 0;
            get_max_power(spectrum, bin + 1, next_max_power);
            for (uint8_t it = 0; it < ignore_dc; it++) {
                uint8_t med_max_power = (max_power + next_max_power) / 2;  // due to the way process_bins works we have to keep resetting the color
                if (process_bins(&med_max_power) == true)
                    return;  // new line signaled, return
            }
        }
        // process actual bin
        if (process_bins(&max_power)) {
            int8_t power = map(range_max_power, 0, 255, -100, 20);
            if (power >= beep_squelch) {
                baseband::request_audio_beep(map(range_max_power, 0, 256, 400, 2600), 24000, 250);
            }
            range_max_power = 0;
            return;  // new line signaled, return
        }
    }
    if (mode != LOOKING_GLASS_SINGLEPASS) {
        f_center += looking_glass_step;
        retune();
    } else {
        baseband::spectrum_streaming_start();
    }
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
    reset_live_view();
    f_min = field_frequency_min.value();
    f_max = field_frequency_max.value();
    f_min = f_min * MHZ_DIV;  // Transpose into full frequency realm
    f_max = f_max * MHZ_DIV;
    looking_glass_range = f_max - f_min;
    if (looking_glass_range <= LOOKING_GLASS_SLICE_WIDTH_MAX) {
        // if the view is done in one pass, show it like in analog_audio_app
        mode = LOOKING_GLASS_SINGLEPASS;
        offset = 2;
        bin_length = SCREEN_W;
        ignore_dc = 0;
        looking_glass_bandwidth = looking_glass_range;
        looking_glass_sampling_rate = looking_glass_bandwidth;
        each_bin_size = looking_glass_bandwidth / SCREEN_W;
        looking_glass_step = looking_glass_bandwidth;
        f_center_ini = f_min + (looking_glass_bandwidth / 2);  // Initial center frequency for sweep
    } else {
        // view is made in multiple pass, use original bin picking
        mode = scan_type.selected_index_value();
        looking_glass_bandwidth = LOOKING_GLASS_SLICE_WIDTH_MAX;
        looking_glass_sampling_rate = LOOKING_GLASS_SLICE_WIDTH_MAX;
        each_bin_size = LOOKING_GLASS_SLICE_WIDTH_MAX / SPEC_NB_BINS;
        if (mode == LOOKING_GLASS_FASTSCAN) {
            offset = 2;
            ignore_dc = 4;
            bin_length = SCREEN_W;
        } else {  // if( mode == LOOKING_GLASS_SLOWSCAN )
            offset = 2;
            bin_length = 80;
            ignore_dc = 0;
        }
        looking_glass_step = (bin_length + ignore_dc) * each_bin_size;
        f_center_ini = f_min - (offset * each_bin_size) + (looking_glass_bandwidth / 2);  // Initial center frequency for sweep
    }
    search_span = looking_glass_range / MHZ_DIV;
    marker_pixel_step = looking_glass_range / SCREEN_W;  // Each pixel value in Hz

    pixel_index = 0;
    max_power = 0;
    bins_hz_size = 0;

    on_marker_change();
    update_range_field();

    // set the sample rate and bandwidth
    receiver_model.set_sampling_rate(looking_glass_sampling_rate);
    receiver_model.set_baseband_bandwidth(looking_glass_bandwidth);

    receiver_model.set_squelch_level(0);
    f_center = f_center_ini;  // Reset sweep into first slice
    baseband::set_spectrum(looking_glass_bandwidth, trigger);
    receiver_model.set_target_frequency(f_center);  // tune rx for this slice
}

void GlassView::plot_marker(uint8_t pos) {
    uint8_t shift_y = 0;
    if (live_frequency_view > 0)  // plot one line down when in live view
    {
        shift_y = 16;
    }
    portapack::display.fill_rectangle({0, 100 + shift_y, SCREEN_W, 8}, Theme::getInstance()->bg_darkest->background);  // Clear old marker and whole marker rectangle btw
    portapack::display.fill_rectangle({pos - 2, 100 + shift_y, 5, 3}, Theme::getInstance()->fg_red->foreground);       // Red marker top
    portapack::display.fill_rectangle({pos - 1, 103 + shift_y, 3, 3}, Theme::getInstance()->fg_red->foreground);       // Red marker middle
    portapack::display.fill_rectangle({pos, 106 + shift_y, 1, 2}, Theme::getInstance()->fg_red->foreground);           // Red marker bottom
}

void GlassView::update_min(int32_t v) {
    int32_t min_size = steps;
    if (locked_range)
        min_size = search_span;
    if (min_size < 2)
        min_size = 2;
    if (v > 7200 - min_size) {
        v = 7200 - min_size;
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
    if (min_size < 2)
        min_size = 2;
    if (v < min_size) {
        v = min_size;
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
                  &button_jump,
                  &button_rst,
                  &field_rx_iq_phase_cal,
                  &freq_stats});

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
                display.scroll_set_area(109, 319);  // Restart scroll on the correct coordinates.
                break;

            case 1:  // LEVEL
                display.fill_rectangle({{0, 108}, {SCREEN_W, 24}}, {0, 0, 0});
                display.scroll_disable();
                level_integration.hidden(false);
                freq_stats.hidden(false);
                button_jump.hidden(false);
                button_rst.hidden(false);
                break;

            case 2:  // PEAK
            default:
                display.fill_rectangle({{0, 108}, {SCREEN_W, 24}}, {0, 0, 0});
                display.scroll_disable();
                level_integration.hidden(false);
                freq_stats.hidden(false);
                button_jump.hidden(false);
                button_rst.hidden(false);
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
        marker_pixel_index = clip<uint8_t>(marker_pixel_index + delta, 0, SCREEN_W);
        on_marker_change();
    };

    field_marker.on_select = [this](TextField&) {
        // Launch Audio with marker frequency.
        launch_audio(marker);
    };

    field_trigger.on_change = [this](int32_t v) {
        trigger = v;
        baseband::set_spectrum(looking_glass_bandwidth, trigger);
    };
    field_trigger.set_value(trigger);

    field_range.on_select = [this](TextField&) {
        locked_range = !locked_range;
        update_range_field();
    };

    button_jump.on_select = [this](Button&) {
        // Launch Audio with peak frequency.
        launch_audio(max_freq_hold);
    };

    button_rst.on_select = [this](Button&) {
        reset_live_view();
    };

    field_rx_iq_phase_cal.set_range(0, hackrf_r9 ? 63 : 31);                 // max2839 has 6 bits [0..63],  max2837 has 5 bits [0..31]
    field_rx_iq_phase_cal.set_value(get_spec_iq_phase_calibration_value());  // using  accessor function of AnalogAudioView to read iq_phase_calibration_value from rx_audio.ini
    field_rx_iq_phase_cal.on_change = [this](int32_t v) {
        set_spec_iq_phase_calibration_value(v);  // using  accessor function of AnalogAudioView to write inside SPEC submenu, register value to max283x and save it to rx_audio.ini
    };
    set_spec_iq_phase_calibration_value(get_spec_iq_phase_calibration_value());  // initialize iq_phase_calibration in radio

    display.scroll_set_area(109, 319);

    // trigger:
    // Discord User jteich:  WidebandSpectrum::on_message to set the trigger value. In WidebandSpectrum::execute,
    // it keeps adding the output of the fft to the buffer until "trigger" number of calls are made,
    // at which time it pushes the buffer up with channel_spectrum.feed
    baseband::set_spectrum(looking_glass_bandwidth, trigger);

    marker_pixel_index = SCREEN_W / 2;
    on_range_changed();  // Force a UI update.

    receiver_model.set_sampling_rate(looking_glass_sampling_rate);   // 20mhz
    receiver_model.set_baseband_bandwidth(looking_glass_bandwidth);  // possible values: 1.75/2.5/3.5/5/5.5/6/7/8/9/10/12/14/15/20/24/28MHz
    receiver_model.set_squelch_level(0);
    receiver_model.enable();

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

uint8_t GlassView::get_spec_iq_phase_calibration_value() {  // define accessor functions inside AnalogAudioView to read & write real iq_phase_calibration_value
    return iq_phase_calibration_value;
}

void GlassView::set_spec_iq_phase_calibration_value(uint8_t cal_value) {  // define accessor functions
    iq_phase_calibration_value = cal_value;
    radio::set_rx_max283x_iq_phase_calibration(iq_phase_calibration_value);
}

void GlassView::load_presets() {
    File presets_file;
    auto error = presets_file.open(looking_glass_dir / u"PRESETS.TXT");
    presets_db.clear();

    // Add the "Manual" entry.
    presets_db.push_back({0, 0, "Manual"});

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

            if (entry.min == 0 || entry.max == 0 || entry.min >= entry.max)
                continue;  // Invalid line.

            presets_db.emplace_back(std::move(entry));
        }
    }

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
