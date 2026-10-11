/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
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

#include "ui_search.hpp"

#include "baseband_api.hpp"
#include "binder.hpp"
#include "string_format.hpp"
#include "ui_freqman.hpp"
#include "audio.hpp"
#include "radio.hpp"

#include <algorithm>

using namespace portapack;
namespace pmem = portapack::persistent_memory;

namespace ui {

void SearchLogger::log_data(SearchRecentEntry& data) {
    log_file.write_entry(";" + to_string_short_freq(data.frequency) + ";" + std::to_string(data.duration));
}

template <>
void RecentEntriesTable<SearchRecentEntries>::draw(
    const Entry& entry,
    const Rect& target_rect,
    Painter& painter,
    const Style& style,
    RecentEntriesColumns& columns) {
    std::string str_duration = "";

    if (entry.duration < 600)
        str_duration = to_string_dec_uint(entry.duration / 10) + "." + to_string_dec_uint(entry.duration % 10) + "s";
    else
        str_duration = to_string_dec_uint(entry.duration / 600) + "m" + to_string_dec_uint((entry.duration / 10) % 60) + "s";

    str_duration.resize(11, ' ');
    std::string freq = to_string_short_freq(entry.frequency);
    freq.resize(columns.at(0).second, ' ');
    painter.draw_string(target_rect.location(), style, freq + " " + entry.time + " " + str_duration);
}

/* SearchView ********************************************/

SearchView::SearchView(
    NavigationView& nav)
    : nav_(nav) {
    sweep_power.resize(points);
    hold_power.resize(points);
    spectrum_row.resize(points);
    baseband::run_image(portapack::spi_flash::image_tag_wideband_spectrum);
    // The sweep waits for the M4 to discard samples until the synthesizers settle.
    radio::set_fast_retune(true);
    receiver_model.enable();

    if (!gradient.load_file(default_gradient_file)) {
        gradient.set_default();
    }

    add_children({&labels,
                  &field_frequency_min,
                  &field_frequency_max,
                  &field_lna,
                  &field_vga,
                  &field_threshold,
                  &text_mean,
                  &text_slices,
                  &text_rate,
                  &text_infos,
                  &vu_max,
                  &progress_timers,
                  &check_snap,
                  &options_snap,
                  &big_display,
                  &check_log,
                  &recent_entries_view});

    recent_entries_view.set_parent_rect({0, 28 * 8, screen_width, screen_height - 28 * 8});
    recent_entries_view.on_select = [this, &nav](const SearchRecentEntry& entry) {
        nav.push<FrequencySaveView>(entry.frequency);
    };

    text_mean.set_style(Theme::getInstance()->fg_medium);
    text_slices.set_style(Theme::getInstance()->fg_medium);
    text_rate.set_style(Theme::getInstance()->fg_medium);
    progress_timers.set_style(Theme::getInstance()->fg_medium);
    big_display.set_style(Theme::getInstance()->fg_medium);

    field_frequency_min.set_step(100'000);
    bind(field_frequency_min, settings_.freq_min, nav, [this](auto) {
        on_range_changed();
    });

    field_frequency_max.set_step(100'000);
    bind(field_frequency_max, settings_.freq_max, nav, [this](auto) {
        on_range_changed();
    });

    check_log.on_select = [this](Checkbox&, bool v) {
        logging = v;
        if (logging) {
            logger.append(logs_dir.string() + "/SEARCH_" + to_string_timestamp(rtc_time::now()) + ".CSV");
            logger.write_header();
        }
    };

    bind(field_threshold, settings_.power_threshold);
    bind(check_snap, settings_.snap_search);
    bind(options_snap, settings_.snap_step);

    progress_timers.set_max(DETECT_DELAY);

    on_range_changed();

    if (pmem::beep_on_packets()) {
        audio::set_rate(audio::Rate::Hz_24000);
        audio::output::start();
    }
}

SearchView::~SearchView() {
    sweep.stop();
    audio::output::stop();
    radio::set_fast_retune(false);
    receiver_model.set_sampling_rate(3072000);  // as Looking Glass: the sweep may leave 20 MHz
    receiver_model.disable();
    baseband::shutdown();
}

void SearchView::on_show() {
    shown = true;
    std::fill(sweep_power.begin(), sweep_power.end(), 0);
    sweep.resume();
}

void SearchView::on_hide() {
    shown = false;
    sweep.stop();
    row_ready = false;
}

void SearchView::focus() {
    field_frequency_min.focus();
}

void SearchView::start_sweep(rf::Frequency start, rf::Frequency span) {
    spec_an::Sweep::Params params{};
    params.start = start;
    params.span = span;
    params.points = points;
    params.avg = kAverage;

    std::fill(sweep_power.begin(), sweep_power.end(), 0);
    std::fill(hold_power.begin(), hold_power.end(), 0);
    hold_count = 0;
    sweep.start(params);
    if (!shown)
        sweep.stop();  // on_show() resumes it
}

void SearchView::sweep_range() {
    const rf::Frequency f_lo = std::min(settings_.freq_min, settings_.freq_max);
    const rf::Frequency f_hi = std::max(settings_.freq_min, settings_.freq_max);
    start_sweep(f_lo, f_hi - f_lo);

    const uint16_t slices = sweep.plan().slices;
    text_slices.set(slices > 999 ? "!!!" : to_string_dec_uint(slices, 3, ' '));
}

void SearchView::start_scan() {
    stage = Stage::Scan;
    detect_timer = 0;
    last_index = -1;
    sweep_range();
}

void SearchView::zoom(uint16_t x) {
    // A few points of the current span either side of x: the signal may sit
    // anywhere in the point, and drift a little until the next period.
    const rf::Frequency spacing = sweep.params().span / (points - 1);
    const rf::Frequency span = std::max<rf::Frequency>(kZoomPoints * spacing, kZoomMinSpan);
    const rf::Frequency center = sweep.point_freq(x);

    stage = Stage::Resolve;
    start_sweep(std::max<rf::Frequency>(center - span / 2, 0), span);
}

void SearchView::lock(rf::Frequency frequency) {
    rtc::RTC datetime;
    std::string str_timestamp;

    resolved_frequency = frequency;
    if (check_snap.value()) {
        const rf::Frequency snap_value = options_snap.selected_index_value();
        resolved_frequency = ((resolved_frequency + snap_value / 2) / snap_value) * snap_value;  // nearest step
    }
    big_display.set(resolved_frequency);

    // Check range
    if ((resolved_frequency < std::min(settings_.freq_min, settings_.freq_max)) ||
        (resolved_frequency > std::max(settings_.freq_min, settings_.freq_max))) {
        text_infos.set("Out of range");
        start_scan();
        return;
    }

    duration = 0;
    release_timer = 0;

    auto& entry = ::on_packet(recent, resolved_frequency);

    rtcGetTime(&RTCD1, &datetime);
    str_timestamp = to_string_dec_uint(datetime.hour(), 2, '0') + ":" +
                    to_string_dec_uint(datetime.minute(), 2, '0') + ":" +
                    to_string_dec_uint(datetime.second(), 2, '0');
    entry.set_time(str_timestamp);
    recent_entries_view.set_dirty();

    text_infos.set("Locked ! ");
    big_display.set_style(Theme::getInstance()->fg_green);

    if (pmem::beep_on_packets()) {
        baseband::request_audio_beep(1000, 24000, 60);
    }
    // TODO: open Audio.

    // Watch it, and the rest of the range, from the range sweep.
    stage = Stage::Locked;
    sweep_range();
}

void SearchView::unlock() {
    auto& entry = ::on_packet(recent, resolved_frequency);
    entry.set_duration(duration);
    if (logging) logger.log_data(entry);
    recent_entries_view.set_dirty();

    text_infos.set("Listening");
    big_display.set_style(Theme::getInstance()->fg_medium);

    // Already sweeping the range.
    stage = Stage::Scan;
    detect_timer = 0;
    last_index = -1;
}

void SearchView::on_slice(const SpecAnSliceMessage& message) {
    if (!sweep.sweeping()) return;  // late reply after a hide
    const int16_t* const src = sweep.accept(message);
    if (!src) return;

    const uint16_t end = std::min<uint16_t>(message.px_end, points);
    for (uint16_t x = message.px_begin; x < end; x++)
        sweep_power[x] = std::max(sweep_power[x], sweep.power(src[x]));

    if (message.last) {
        // Only whole sweeps go into the hold, the slices of the next one are still to come.
        for (uint16_t x = 0; x < points; x++) {
            hold_power[x] = std::max(hold_power[x], sweep_power[x]);
            sweep_power[x] = 0;
        }
        hold_count++;
        search_counter++;
    }
}

void SearchView::evaluate() {
    uint32_t sum = 0;
    uint8_t power_max = 0;
    uint16_t index_max = 0;

    for (uint16_t x = 0; x < points; x++) {
        const uint8_t power = hold_power[x];
        sum += power;
        if (power > power_max) {
            power_max = power;
            index_max = x;
        }
    }
    const uint32_t mean = sum / points;
    const bool over = power_max >= mean + settings_.power_threshold;
    const int16_t index = over ? index_max : -1;

    if (stage != Stage::Resolve) {
        // A period of the whole range: show it.
        for (uint16_t x = 0; x < points; x++)
            spectrum_row[x] = gradient.lut[hold_power[x]];
        row_tick = index;
        row_ready = true;
        mean_power = mean;
        overall_power_max = power_max;
    }

    std::fill(hold_power.begin(), hold_power.end(), 0);
    hold_count = 0;

    switch (stage) {
        case Stage::Scan:
            // Staying around the same point
            if ((index > -1) && (last_index > -1) && (abs(index - last_index) <= 2)) {
                if (detect_timer >= DETECT_DELAY) {
                    locked_index = index;
                    estimate = sweep.point_freq(index);
                    zoom(index);
                }
            } else {
                detect_timer = 0;
            }
            last_index = index;
            break;

        case Stage::Resolve:
            if (index < 0) {
                lock(estimate);  // no peak: a wideband signal fills the zoom, keep what we have
            } else {
                estimate = sweep.point_freq(index);
                if (sweep.params().span / (points - 1) > kResolvedSpacing)
                    zoom(index);
                else
                    lock(estimate);
            }
            break;

        case Stage::Locked:
            if ((index > -1) && (abs(index - locked_index) <= 2)) {
                locked_index = index;  // follows a slow drift
                release_timer = 0;
            } else if (release_timer >= RELEASE_DELAY) {
                unlock();
            }
            break;
    }
}

void SearchView::draw_row() {
    const uint16_t center_align_start = (screen_width - points) / 2;
    display.draw_pixels({{center_align_start, 88}, {points, 1}}, spectrum_row);
    row_ready = false;

    // Refresh red tick
    portapack::display.fill_rectangle({last_tick_pos, 90, 1, 6}, Theme::getInstance()->fg_red->background);
    if (row_tick > -1) {
        last_tick_pos = row_tick + center_align_start;
        portapack::display.fill_rectangle({last_tick_pos, 90, 1, 6}, Theme::getInstance()->fg_red->foreground);
    }
}

void SearchView::do_timers() {
    if (timing_div >= 60) {
        // ~1Hz

        timing_div = 0;

        // Update scan rate (whole sweeps per second)
        text_rate.set(to_string_dec_uint(std::min<uint16_t>(search_counter, 999), 3));
        search_counter = 0;
    }

    if (timing_div % 12 == 0) {
        // ~5Hz

        // Update power levels
        text_mean.set(to_string_dec_uint(mean_power, 3));

        vu_max.set_value(overall_power_max);
        vu_max.set_mark(mean_power + settings_.power_threshold);
    }

    if (timing_div % 6 == 0) {
        // ~10Hz

        // Look at the sweeps of this period (none yet: a wide range takes longer)
        if (hold_count)
            evaluate();

        // Update timing indicator
        if (stage == Stage::Locked) {
            progress_timers.set_max(RELEASE_DELAY);
            progress_timers.set_value(RELEASE_DELAY - release_timer);
        } else {
            progress_timers.set_max(DETECT_DELAY);
            progress_timers.set_value(detect_timer);
        }

        // Increment timers
        if (detect_timer < DETECT_DELAY) detect_timer++;
        if (release_timer < RELEASE_DELAY) release_timer++;

        if (stage == Stage::Locked) duration++;
    }

    timing_div++;
}

void SearchView::on_range_changed() {
    if (stage == Stage::Locked)
        unlock();  // logs the signal
    start_scan();
}

} /* namespace ui */
