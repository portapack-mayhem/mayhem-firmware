/*
 * Copyright (C) 2024 PortaPack Mayhem
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

#include "ui_two_tone_pager.hpp"

#include "portapack.hpp"
#include "baseband_api.hpp"
#include "portapack_shared_memory.hpp"
#include "string_format.hpp"
#include "theme.hpp"

#include <algorithm>
#include <cstring>

using namespace portapack;

namespace ui::external_app::two_tone_pager {

// ---------------------------------------------------------------------------
// Tone tables — store only freq×10 values; names are generated at runtime
// to keep static data small and avoid const-char-pointer patching issues.
// ---------------------------------------------------------------------------

static const uint32_t MOTO_FREQS[45] = {
    2885,
    3047,
    3217,
    3396,
    3586,
    3786,
    3998,
    4221,
    4457,
    4705,
    4968,
    5246,
    5539,
    5848,
    6174,
    6519,
    6883,
    7268,
    7674,
    8102,
    8555,
    9032,
    9537,
    10073,
    10642,
    11225,
    11247,
    11534,
    11852,
    11885,
    12178,
    12514,
    12555,
    12858,
    13258,
    13576,
    13950,
    13996,
    14768,
    15579,
    16430,
    17325,
    18262,
    19245,
    20275,
};

// Index 0 = None (0), indices 1-50 = standard CTCSS tones
static const uint32_t CTCSS_FREQS[51] = {
    0,
    670,
    719,
    744,
    770,
    797,
    825,
    854,
    885,
    915,
    948,
    974,
    1000,
    1035,
    1072,
    1109,
    1148,
    1188,
    1230,
    1273,
    1318,
    1365,
    1413,
    1462,
    1500,
    1514,
    1567,
    1598,
    1622,
    1655,
    1679,
    1713,
    1738,
    1773,
    1799,
    1835,
    1862,
    1899,
    1928,
    1966,
    1995,
    2035,
    2065,
    2107,
    2181,
    2257,
    2291,
    2336,
    2418,
    2503,
    2541,
};

// Generate a display name from a freq×10 value ("288.5Hz", "None" for 0)
static std::string freq_name(uint32_t freq_x10) {
    if (freq_x10 == 0) return "None";
    return to_string_dec_uint(freq_x10 / 10) + "." +
           to_string_dec_uint(freq_x10 % 10) + "Hz";
}

// Common timing profiles used in the field
struct TimingPreset {
    uint32_t dur_a;  // ms
    uint32_t dur_b;  // ms
    uint32_t gap;    // ms
};

static const TimingPreset TIMING_PRESETS[4] = {
    {1000, 3000, 0},  // Moto Std
    {700, 1000, 0},   // Short Alert
    {2000, 1000, 0},  // Fire Std
    {3000, 3000, 0},  // Long Alert
};

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Parse one unsigned decimal integer from *p, advancing p past the digits and
// an optional trailing comma.
static uint32_t parse_uint_field(const char*& p) {
    uint32_t v = 0;
    while (*p >= '0' && *p <= '9')
        v = v * 10 + (*p++ - '0');
    if (*p == ',')
        ++p;
    return v;
}

// Phase-delta for the 32-bit sine-table accumulator used by proc_tones.
// delta = freq_hz * 2^32 / sample_rate
// Using freq_x10 to avoid floating-point: delta = (freq_x10 * 2^32) / (sample_rate * 10)
uint32_t TwoTonePagerView::tone_delta(uint32_t freq_x10) const {
    if (freq_x10 == 0) return 0;
    return static_cast<uint32_t>(
        (static_cast<uint64_t>(freq_x10) << 32) /
        (static_cast<uint64_t>(SAMPLE_RATE) * 10ULL));
}

uint32_t TwoTonePagerView::ms_to_samples(uint32_t ms) const {
    return static_cast<uint32_t>(
        (static_cast<uint64_t>(ms) * SAMPLE_RATE) / 1000ULL);
}

// ---------------------------------------------------------------------------
// Preset encode / decode
// ---------------------------------------------------------------------------

std::string TwoTonePagerView::encode_preset() const {
    return to_string_dec_uint(ctcss_idx) + "," +
           to_string_dec_uint(tone_a_idx) + "," +
           to_string_dec_uint(tone_b_idx) + "," +
           to_string_dec_uint(dur_a) + "," +
           to_string_dec_uint(dur_b) + "," +
           to_string_dec_uint(gap_ms) + "," +
           to_string_dec_uint(custom_freq_a_hz) + "," +
           to_string_dec_uint(custom_freq_b_hz);
}

void TwoTonePagerView::decode_preset(const std::string& s) {
    const char* p = s.c_str();
    uint32_t ci = parse_uint_field(p);
    uint32_t ai = parse_uint_field(p);
    uint32_t bi = parse_uint_field(p);
    uint32_t da = parse_uint_field(p);
    uint32_t db = parse_uint_field(p);
    uint32_t gp = parse_uint_field(p);

    uint32_t cfa = (*p != '\0') ? parse_uint_field(p) : 405;
    uint32_t cfb = (*p != '\0') ? parse_uint_field(p) : 814;

    ctcss_idx = std::min(ci, static_cast<uint32_t>(CTCSS_COUNT - 1));
    tone_a_idx = std::min(ai, CUSTOM_TONE_IDX);
    tone_b_idx = std::min(bi, CUSTOM_TONE_IDX);
    dur_a = std::max(uint32_t{100}, std::min(da, uint32_t{9900}));
    dur_b = std::max(uint32_t{100}, std::min(db, uint32_t{9900}));
    gap_ms = std::min(gp, uint32_t{9900});
    custom_freq_a_hz = std::max(uint32_t{100}, std::min(cfa, uint32_t{9999}));
    custom_freq_b_hz = std::max(uint32_t{100}, std::min(cfb, uint32_t{9999}));
}

std::string& TwoTonePagerView::slot_ref(uint32_t slot) {
    switch (slot) {
        case 2:
            return preset_2;
        case 3:
            return preset_3;
        case 4:
            return preset_4;
        case 5:
            return preset_5;
        default:
            return preset_1;
    }
}

std::string& TwoTonePagerView::slot_name_ref(uint32_t slot) {
    switch (slot) {
        case 2:
            return preset_name_2;
        case 3:
            return preset_name_3;
        case 4:
            return preset_name_4;
        case 5:
            return preset_name_5;
        default:
            return preset_name_1;
    }
}

void TwoTonePagerView::update_slot_name_display() {
    const auto& name = slot_name_ref(preset_slot);
    std::string display = name.empty() ? "(none)" : name;
    if (display.size() > 8) display = display.substr(0, 8);
    text_slot_name.set(display);
}

void TwoTonePagerView::save_preset(uint32_t slot) {
    slot_ref(slot) = encode_preset();
    text_status.set("Saved to slot " + to_string_dec_uint(slot));
}

void TwoTonePagerView::load_preset(uint32_t slot) {
    decode_preset(slot_ref(slot));

    // Update all UI fields without firing their on_change callbacks
    options_ctcss.set_selected_index(ctcss_idx, false);
    options_tone_a.set_selected_index(tone_a_idx, false);
    options_tone_b.set_selected_index(tone_b_idx, false);
    field_dur_a.set_value(static_cast<int32_t>(dur_a), false);
    field_dur_b.set_value(static_cast<int32_t>(dur_b), false);
    field_gap.set_value(static_cast<int32_t>(gap_ms), false);
    symfield_custom_a.set_value(custom_freq_a_hz);
    symfield_custom_b.set_value(custom_freq_b_hz);
    options_timing.set_selected_index(detect_timing_preset(), false);

    update_tx_time();
    update_slot_name_display();
    text_status.set("Loaded slot " + to_string_dec_uint(slot));
}

// ---------------------------------------------------------------------------
// Timing presets
// ---------------------------------------------------------------------------

size_t TwoTonePagerView::detect_timing_preset() const {
    for (size_t i = 0; i < 4; i++) {
        if (TIMING_PRESETS[i].dur_a == dur_a &&
            TIMING_PRESETS[i].dur_b == dur_b &&
            TIMING_PRESETS[i].gap == gap_ms)
            return i;
    }
    return 4;  // "Custom"
}

void TwoTonePagerView::apply_timing_preset(size_t idx) {
    if (idx >= 4) return;  // "Custom" — do not override current values
    dur_a = TIMING_PRESETS[idx].dur_a;
    dur_b = TIMING_PRESETS[idx].dur_b;
    gap_ms = TIMING_PRESETS[idx].gap;
    field_dur_a.set_value(static_cast<int32_t>(dur_a), false);
    field_dur_b.set_value(static_cast<int32_t>(dur_b), false);
    field_gap.set_value(static_cast<int32_t>(gap_ms), false);
    update_tx_time();
}

// ---------------------------------------------------------------------------
// UI helpers
// ---------------------------------------------------------------------------

void TwoTonePagerView::update_tx_time() {
    uint32_t total_ms = dur_a + gap_ms + dur_b;
    std::string name_a = (tone_a_idx == CUSTOM_TONE_IDX)
                             ? to_string_dec_uint(custom_freq_a_hz) + "Hz"
                             : freq_name(MOTO_FREQS[tone_a_idx]);
    std::string name_b = (tone_b_idx == CUSTOM_TONE_IDX)
                             ? to_string_dec_uint(custom_freq_b_hz) + "Hz"
                             : freq_name(MOTO_FREQS[tone_b_idx]);
    text_time.set("TX time: " +
                  to_string_dec_uint(total_ms / 1000) + "." +
                  to_string_dec_uint((total_ms / 100) % 10) + "s" +
                  "  A:" + name_a + " B:" + name_b);
}

// ---------------------------------------------------------------------------
// Transmission
// ---------------------------------------------------------------------------

bool TwoTonePagerView::start_tx() {
    auto& td = shared_memory.bb_data.tones_data;

    const uint32_t freq_a_x10 = (tone_a_idx == CUSTOM_TONE_IDX)
                                    ? custom_freq_a_hz * 10
                                    : MOTO_FREQS[tone_a_idx];
    const uint32_t freq_b_x10 = (tone_b_idx == CUSTOM_TONE_IDX)
                                    ? custom_freq_b_hz * 10
                                    : MOTO_FREQS[tone_b_idx];
    const uint32_t delta_a = tone_delta(freq_a_x10);
    const uint32_t delta_b = tone_delta(freq_b_x10);
    const uint32_t delta_c = tone_delta(CTCSS_FREQS[ctcss_idx]);
    const uint32_t samp_a = ms_to_samples(dur_a);
    const uint32_t samp_b = ms_to_samples(dur_b);
    const uint32_t samp_g = ms_to_samples(gap_ms);
    const bool with_ctcss = (ctcss_idx > 0);

    // Clear tone defs we'll use
    memset(&td, 0, sizeof(td));

    uint8_t tone_count;

    if (!with_ctcss) {
        // Single-tone sequential mode
        // tone_defs[0] = Tone A,  tone_defs[1] = Tone B
        td.tone_defs[0].delta = delta_a;
        td.tone_defs[0].duration = samp_a;
        td.tone_defs[1].delta = delta_b;
        td.tone_defs[1].duration = samp_b;

        if (gap_ms > 0) {
            td.silence = samp_g;
            td.message[0] = 0;    // Tone A
            td.message[1] = 255;  // Silence (any index ≥ 32 triggers silence)
            td.message[2] = 1;    // Tone B
            tone_count = 3;
        } else {
            td.message[0] = 0;  // Tone A
            td.message[1] = 1;  // Tone B
            tone_count = 2;
        }
    } else {
        // Dual-tone mode: CTCSS mixed simultaneously with pager tones.
        //
        // proc_tones dual-tone indexing for digit n:
        //   main delta = tone_deltas[n * 2]   = tone_defs[n*2].delta
        //   sub  delta = tone_deltas[n*2 + 1] = tone_defs[n*2+1].delta
        //   duration   = tone_durations[n]    = tone_defs[n].duration
        //
        // digit 0 → Tone A (main) + CTCSS (sub),  duration from tone_defs[0]
        // digit 1 → Tone B (main) + CTCSS (sub),  duration from tone_defs[1]
        td.tone_defs[0].delta = delta_a;  // digit 0 main
        td.tone_defs[0].duration = samp_a;
        td.tone_defs[1].delta = delta_c;  // digit 0 sub (CTCSS); also digit 1 duration source
        td.tone_defs[1].duration = samp_b;
        td.tone_defs[2].delta = delta_b;  // digit 1 main (Tone B)
        td.tone_defs[2].duration = 0;     // duration for digit 1 comes from tone_defs[1]
        td.tone_defs[3].delta = delta_c;  // digit 1 sub (CTCSS)
        td.tone_defs[3].duration = 0;

        if (gap_ms > 0) {
            td.silence = samp_g;
            td.message[0] = 0;
            td.message[1] = 255;
            td.message[2] = 1;
            tone_count = 3;
        } else {
            td.message[0] = 0;
            td.message[1] = 1;
            tone_count = 2;
        }
    }

    progressbar.set_max(tone_count);
    progressbar.set_value(0);

    transmitter_model.set_baseband_bandwidth(1'750'000);
    transmitter_model.enable();

    baseband::set_tones_config(
        transmitter_model.channel_bandwidth(),
        0,  // no pre-silence
        tone_count,
        with_ctcss,  // dual_tone flag
        false);      // no audio monitor output

    return true;
}

void TwoTonePagerView::stop_tx() {
    transmitter_model.disable();
    baseband::kill_tone();
    tx_view.set_transmitting(false);
    text_status.set("Stopped.");
}

void TwoTonePagerView::on_tx_progress(uint32_t progress, bool done) {
    if (done) {
        transmitter_model.disable();
        progressbar.set_value(0);
        tx_view.set_transmitting(false);
        text_status.set("Done.");
    } else {
        progressbar.set_value(progress);
    }
}

// ---------------------------------------------------------------------------
// View lifecycle
// ---------------------------------------------------------------------------

void TwoTonePagerView::focus() {
    options_tone_a.focus();
}

TwoTonePagerView::~TwoTonePagerView() {
    transmitter_model.disable();
    baseband::shutdown();
}

TwoTonePagerView::TwoTonePagerView(NavigationView& nav)
    : nav_(nav) {
    baseband::run_prepared_image(portapack::memory::map::m4_code.base());

    // Clamp restored settings to valid ranges before touching UI
    ctcss_idx = std::min(ctcss_idx, static_cast<uint32_t>(CTCSS_COUNT - 1));
    tone_a_idx = std::min(tone_a_idx, CUSTOM_TONE_IDX);
    tone_b_idx = std::min(tone_b_idx, CUSTOM_TONE_IDX);
    dur_a = std::max(uint32_t{100}, std::min(dur_a, uint32_t{9900}));
    dur_b = std::max(uint32_t{100}, std::min(dur_b, uint32_t{9900}));
    gap_ms = std::min(gap_ms, uint32_t{9900});
    preset_slot = std::max(uint32_t{1}, std::min(preset_slot, uint32_t{5}));
    custom_freq_a_hz = std::max(uint32_t{100}, std::min(custom_freq_a_hz, uint32_t{9999}));
    custom_freq_b_hz = std::max(uint32_t{100}, std::min(custom_freq_b_hz, uint32_t{9999}));

    // Build CTCSS options list
    {
        std::vector<std::pair<std::string, int32_t>> opts;
        opts.reserve(CTCSS_COUNT);
        for (size_t i = 0; i < CTCSS_COUNT; i++)
            opts.push_back({freq_name(CTCSS_FREQS[i]), static_cast<int32_t>(i)});
        options_ctcss.set_options(std::move(opts));
    }

    // Build Motorola tone options (same pool for both A and B); "Custom" appended
    {
        std::vector<std::pair<std::string, int32_t>> opts;
        opts.reserve(MOTO_TONE_COUNT + 1);
        for (size_t i = 0; i < MOTO_TONE_COUNT; i++)
            opts.push_back({freq_name(MOTO_FREQS[i]), static_cast<int32_t>(i)});
        opts.push_back({"Custom", static_cast<int32_t>(CUSTOM_TONE_IDX)});
        options_tone_a.set_options(opts);
        options_tone_b.set_options(std::move(opts));
    }

    add_children({&labels,
                  &options_ctcss,
                  &options_tone_a,
                  &options_tone_b,
                  &symfield_custom_a,
                  &symfield_custom_b,
                  &field_dur_a,
                  &field_dur_b,
                  &field_gap,
                  &options_timing,
                  &field_slot,
                  &button_save,
                  &button_load,
                  &text_slot_name,
                  &text_time,
                  &text_status,
                  &progressbar,
                  &tx_view});

    // Restore saved indices into the options / number fields (no callbacks)
    options_ctcss.set_selected_index(ctcss_idx, false);
    options_tone_a.set_selected_index(tone_a_idx, false);
    options_tone_b.set_selected_index(tone_b_idx, false);
    symfield_custom_a.set_value(custom_freq_a_hz);
    symfield_custom_b.set_value(custom_freq_b_hz);
    field_dur_a.set_value(static_cast<int32_t>(dur_a), false);
    field_dur_b.set_value(static_cast<int32_t>(dur_b), false);
    field_gap.set_value(static_cast<int32_t>(gap_ms), false);
    options_timing.set_selected_index(detect_timing_preset(), false);
    field_slot.set_value(static_cast<int32_t>(preset_slot), false);

    update_tx_time();
    update_slot_name_display();

    // --- Callbacks ---

    options_ctcss.on_change = [this](size_t i, int32_t) {
        ctcss_idx = static_cast<uint32_t>(i);
    };

    options_tone_a.on_change = [this](size_t i, int32_t) {
        tone_a_idx = static_cast<uint32_t>(i);
        update_tx_time();
    };

    options_tone_b.on_change = [this](size_t i, int32_t) {
        tone_b_idx = static_cast<uint32_t>(i);
        update_tx_time();
    };

    symfield_custom_a.on_change = [this](SymField&) {
        uint32_t v = static_cast<uint32_t>(symfield_custom_a.to_integer());
        custom_freq_a_hz = std::max(uint32_t{100}, std::min(v, uint32_t{9999}));
        update_tx_time();
    };

    symfield_custom_b.on_change = [this](SymField&) {
        uint32_t v = static_cast<uint32_t>(symfield_custom_b.to_integer());
        custom_freq_b_hz = std::max(uint32_t{100}, std::min(v, uint32_t{9999}));
        update_tx_time();
    };

    field_dur_a.on_change = [this](int32_t v) {
        dur_a = static_cast<uint32_t>(v);
        options_timing.set_selected_index(detect_timing_preset(), false);
        update_tx_time();
    };

    field_dur_b.on_change = [this](int32_t v) {
        dur_b = static_cast<uint32_t>(v);
        options_timing.set_selected_index(detect_timing_preset(), false);
        update_tx_time();
    };

    field_gap.on_change = [this](int32_t v) {
        gap_ms = static_cast<uint32_t>(v);
        options_timing.set_selected_index(detect_timing_preset(), false);
        update_tx_time();
    };

    options_timing.on_change = [this](size_t i, int32_t) {
        apply_timing_preset(i);
    };

    field_slot.on_change = [this](int32_t v) {
        preset_slot = static_cast<uint32_t>(v);
        update_slot_name_display();
    };

    button_save.on_select = [this](Button&) {
        text_prompt(nav_, slot_name_ref(preset_slot), 12, ENTER_KEYBOARD_MODE_ALPHA,
                    [this](std::string&) {
                        save_preset(preset_slot);
                        update_slot_name_display();
                    });
    };

    button_load.on_select = [this](Button&) {
        load_preset(preset_slot);
    };

    tx_view.on_edit_frequency = [this, &nav]() {
        auto new_view = nav.push<FrequencyKeypadView>(transmitter_model.target_frequency());
        new_view->on_changed = [this](rf::Frequency f) {
            transmitter_model.set_target_frequency(f);
        };
    };

    tx_view.on_start = [this]() {
        if (start_tx()) {
            tx_view.set_transmitting(true);
            text_status.set("Transmitting...");
        }
    };

    tx_view.on_stop = [this]() {
        stop_tx();
    };
}

}  // namespace ui::external_app::two_tone_pager
