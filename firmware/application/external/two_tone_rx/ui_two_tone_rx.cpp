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

#include "ui_two_tone_rx.hpp"
#include "audio.hpp"
#include "baseband_api.hpp"
#include "portapack.hpp"
#include "string_format.hpp"

using namespace portapack;

namespace ui::external_app::two_tone_rx {

static constexpr uint32_t DETECT_WINDOW_MS = 40;
static constexpr Coord screen_width_px = 240;
static constexpr Coord screen_height_px = 320;
static constexpr Coord tone_log_top = 4 * 16;
static constexpr Coord waterfall_top = 13 * 16;
static constexpr Coord waterfall_height = 6 * 16;
static constexpr Rect tone_log_rect{0, tone_log_top, 30 * 8, waterfall_top - tone_log_top};

// ---------------------------------------------------------------------------
// Tone tables
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

// Index 0 = None (0), indices 1–50 = standard CTCSS tones (freq × 10)
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

static std::string ctcss_name(uint32_t freq_x10) {
    if (freq_x10 == 0) return "None";
    return to_string_dec_uint(freq_x10 / 10) + "." +
           to_string_dec_uint(freq_x10 % 10) + "Hz";
}

// ---------------------------------------------------------------------------
// MOTO table helpers
// ---------------------------------------------------------------------------

static constexpr uint32_t MOTO_NONE = 255;
static constexpr uint32_t MOTO_TRANSITION_SNAP_HZ = 30;   // stricter live transition tolerance
static constexpr uint32_t MOTO_FINAL_MATCH_HZ = 60;       // looser phase-end/logging tolerance
static constexpr uint32_t MOTO_TRANSITION_DELTA_HZ = 25;  // raw shift needed to confirm close-in A→B handoff

static uint32_t abs_diff_u32(uint32_t a, uint32_t b) {
    return (a > b) ? (a - b) : (b - a);
}

// Return the nearest MOTO table index within match_hz, or MOTO_NONE.
static uint32_t moto_index(uint32_t freq_hz, uint32_t match_hz = MOTO_TRANSITION_SNAP_HZ) {
    if (freq_hz == 0) return MOTO_NONE;
    uint32_t best_idx = MOTO_NONE;
    uint32_t best_diff = match_hz + 1;
    for (size_t i = 0; i < 45; i++) {
        const uint32_t hz = MOTO_FREQS[i] / 10;
        const uint32_t diff = (freq_hz > hz) ? (freq_hz - hz) : (hz - freq_hz);
        if (diff < best_diff) {
            best_diff = diff;
            best_idx = i;
        }
    }
    return best_idx;
}

// Format a detected tone for display, snapping to nearest MOTO table entry.
static std::string format_tone(uint32_t freq_hz, uint32_t duration_ms) {
    const uint32_t idx = moto_index(freq_hz, MOTO_FINAL_MATCH_HZ);
    std::string freq_str;
    if (idx != MOTO_NONE) {
        const uint32_t matched_x10 = MOTO_FREQS[idx];
        freq_str = to_string_dec_uint(matched_x10 / 10) + "." +
                   to_string_dec_uint(matched_x10 % 10) + "Hz";
    } else {
        freq_str = to_string_dec_uint(freq_hz) + "Hz";
    }
    return freq_str + " " + to_string_dec_uint(duration_ms) + "ms";
}

// ---------------------------------------------------------------------------
// Waterfall rect
// ---------------------------------------------------------------------------
static constexpr ui::Rect waterfall_rect{0, waterfall_top, screen_width_px, waterfall_height};

// ---------------------------------------------------------------------------
// TwoToneRxView
// ---------------------------------------------------------------------------

void TwoToneRxView::focus() {
    button_startstop.focus();
}

void TwoToneRxView::on_hide() {
    if (running_) stop_rx();
}

TwoToneRxView::TwoToneRxView(NavigationView& nav)
    : nav_{nav} {
    prior_antenna_bias_ = get_antenna_bias();
    debug_file_.append(u"DEBUG/TWOTONERX.TXT");
    debug_file_.write_entry("==== two_tone_rx session start ====");

    add_children({
        &field_frequency,
        &field_rf_amp,
        &field_lna,
        &field_vga,
        &rssi,
        &field_volume,
        &labels,
        &options_ctcss,
        &field_squelch,
        &button_startstop,
        &button_clear,
        &text_status,
        &check_bias_t,
        &tone_log_view,
    });

    // Populate CTCSS options
    using opt_t = std::pair<std::string, int32_t>;
    options_ctcss.set_options([&]() {
        std::vector<opt_t> opts;
        opts.reserve(51);
        for (size_t i = 0; i < 51; i++)
            opts.push_back({ctcss_name(CTCSS_FREQS[i]), (int32_t)i});
        return opts;
    }());
    options_ctcss.set_by_value((int32_t)ctcss_idx);
    options_ctcss.on_change = [this](size_t, int32_t v) {
        ctcss_idx = (uint32_t)v;
        if (running_) baseband::set_tonedetect_config((uint8_t)squelch_val, CTCSS_FREQS[ctcss_idx]);
    };

    field_squelch.set_value((int32_t)squelch_val);
    field_squelch.on_change = [this](int32_t v) {
        squelch_val = (uint32_t)v;
        if (running_) baseband::set_tonedetect_config((uint8_t)squelch_val, CTCSS_FREQS[ctcss_idx]);
    };

    button_startstop.on_select = [this](Button&) {
        if (running_)
            stop_rx();
        else
            start_rx();
    };

    button_clear.on_select = [this](Button&) {
        tone_log_entries_.clear();
        tone_log_view.set_dirty();
        text_status.set("");
        reset_detect_state();
    };

    check_bias_t.set_value(bias_t_enabled);
    check_bias_t.on_select = [this](Checkbox&, bool v) {
        bias_t_enabled = v;
        apply_bias_t(running_);
    };

    field_frequency.set_step(12500);
    tone_log_view.set_parent_rect(tone_log_rect);
}

TwoToneRxView::~TwoToneRxView() {
    if (running_) stop_rx();
}

void TwoToneRxView::reset_detect_state() {
    detect_state_ = DetectState::IDLE;
    phase_window_count_ = 0;
    phase_freq_accum_ = 0;
    phase_valid_windows_ = 0;
    phase_last_freq_ = 0;
    phase_last_is_first_ = true;
    t1_avg_freq_ = 0;
    t1_window_count_ = 0;
    t1_transition_candidate_windows_ = 0;
    t2_zero_window_count_ = 0;
    debug_trace_active_ = false;
    debug_trace_id_ = 0;
}

void TwoToneRxView::start_rx() {
    baseband::run_prepared_image(portapack::memory::map::m4_code.base());

    audio::set_rate(audio::Rate::Hz_24000);
    audio::output::start();

    receiver_model.set_hidden_offset(0);
    receiver_model.set_sampling_rate(3072000);
    receiver_model.set_baseband_bandwidth(1750000);
    receiver_model.enable();
    apply_bias_t(true);

    baseband::set_tonedetect_config((uint8_t)squelch_val, CTCSS_FREQS[ctcss_idx]);

    add_child(&waterfall);
    waterfall.set_parent_rect(waterfall_rect);

    running_ = true;
    button_startstop.set_text("Stop");
    text_status.set("");
}

void TwoToneRxView::stop_rx() {
    remove_child(&waterfall);

    apply_bias_t(false);
    receiver_model.disable();
    baseband::shutdown();
    audio::output::stop();

    running_ = false;
    button_startstop.set_text("Start");
    text_status.set("");
    reset_detect_state();
}

void TwoToneRxView::apply_bias_t(bool active) {
    if (active && bias_t_enabled) {
        set_antenna_bias(true);
        receiver_model.set_antenna_bias();
        applied_bias_t_ = true;
        return;
    }

    if (!applied_bias_t_) return;

    set_antenna_bias(prior_antenna_bias_);
    receiver_model.set_antenna_bias();
    applied_bias_t_ = false;
}

void TwoToneRxView::debug_log(const std::string& line) {
    const std::string trace_prefix = debug_trace_active_
                                         ? ("#" + to_string_dec_uint(debug_trace_id_) + " ")
                                         : "";
    const std::string entry = "[D] " + trace_prefix + line;
    debug_file_.write_entry(entry);
}

void TwoToneRxView::debug_trace_begin(const std::string& line) {
    if (!debug_trace_active_) {
        debug_trace_id_ = ++debug_trace_counter_;
        debug_trace_active_ = true;
    }
    debug_log(line);
}

void TwoToneRxView::finalize_detected_pair(uint32_t t2_avg, uint32_t t2_dur, const char* reason) {
    const uint32_t t1_dur = t1_window_count_ * DETECT_WINDOW_MS;
    const std::string suffix = std::string(" via ") + reason;

    if (t1_avg_freq_ > 0 && t2_avg > 0 &&
        t1_dur >= 500 && t2_dur >= 500 &&
        moto_index(t1_avg_freq_, MOTO_FINAL_MATCH_HZ) != MOTO_NONE &&
        moto_index(t2_avg, MOTO_FINAL_MATCH_HZ) != MOTO_NONE) {
        debug_trace_begin("END ok T1=" + format_tone(t1_avg_freq_, t1_dur) +
                          " T2=" + format_tone(t2_avg, t2_dur) + suffix);
        log_tone_pair(t1_avg_freq_, t1_dur, t2_avg, t2_dur);
    } else {
        debug_trace_begin("END drop T1=" + format_tone(t1_avg_freq_, t1_dur) +
                          " T2=" + format_tone(t2_avg, t2_dur) + suffix);
    }
}

// ---------------------------------------------------------------------------
// on_tone_data — window-based two-tone detection state machine
//
// Every 40 ms the baseband sends tone_end=false with a raw frequency estimate
// for that window (freq_hz=0 if no stable QCII candidate).
// When the CTCSS gate closes it sends tone_end=true (freq_hz=0).
//
// First and last windows of each phase are discarded:
//   - "First" is tracked with phase_last_is_first_.
//   - "Last" is phase_last_freq_, never added to the accumulator while
//     pending; discarded when the phase ends.
//
// T1→T2 transition: consecutive windows that snap to different MOTO table
// entries. The transition window becomes T2's first window.
//
// Final matching is done from the phase-average raw estimates collected here,
// not from the per-window nearest-table snap used for transition detection.
// ---------------------------------------------------------------------------
void TwoToneRxView::on_tone_data(const ToneDetectDataMessage* msg) {
    if (!running_) return;

    if (msg->tone_end) {
        // Gate closed — finalize whatever phase we're in
        if (detect_state_ == DetectState::T2_COLLECTING) {
            // phase_last_freq_ is the last T2 window → discard it
            uint32_t t2_avg = (phase_valid_windows_ > 0)
                                  ? (phase_freq_accum_ / phase_valid_windows_)
                                  : phase_last_freq_;
            uint32_t t2_dur = phase_window_count_ * DETECT_WINDOW_MS;
            finalize_detected_pair(t2_avg, t2_dur, "tone_end");
        } else if (debug_trace_active_) {
            debug_log("END before T2");
        }
        reset_detect_state();
        text_status.set("");
        return;
    }

    // tone_end=false: this is a 40 ms measurement window
    const uint32_t freq = msg->freq_hz;
    if (freq == 0) {
        if (detect_state_ == DetectState::T2_COLLECTING) {
            const uint32_t t2_avg = (phase_valid_windows_ > 0)
                                        ? (phase_freq_accum_ / phase_valid_windows_)
                                        : phase_last_freq_;
            const uint32_t t2_dur = phase_window_count_ * DETECT_WINDOW_MS;

            if (t2_avg > 0 && t2_dur >= 2000) {
                finalize_detected_pair(t2_avg, t2_dur, "t2_break");
                reset_detect_state();
                text_status.set("");
                return;
            }
        }

        // A single weak/noisy dropout is tolerated during T2 so standard 3 s
        // B tones are not lost to one marginal 40 ms estimate window.
        if (detect_state_ == DetectState::T2_COLLECTING && t2_zero_window_count_ == 0) {
            t2_zero_window_count_++;
            if (debug_trace_active_) {
                debug_log("T2 zero dropout");
            }
            text_status.set("T1:" + format_tone(t1_avg_freq_, t1_window_count_ * DETECT_WINDOW_MS) +
                            " T2 ?");
            return;
        }

        // Otherwise, treat the missing estimate as a break in the sequence.
        if (debug_trace_active_) {
            debug_log("RESET zero");
        }
        reset_detect_state();
        text_status.set("");
        return;
    }

    t2_zero_window_count_ = 0;

    switch (detect_state_) {
        case DetectState::IDLE:
            detect_state_ = DetectState::T1_COLLECTING;
            phase_window_count_ = 1;
            phase_freq_accum_ = 0;
            phase_valid_windows_ = 0;
            phase_last_freq_ = freq;
            phase_last_is_first_ = true;
            text_status.set("T1 ...");
            break;

        case DetectState::T1_COLLECTING: {
            // Check for T1→T2 MOTO index transition.
            // Skip if phase_last_freq_ is the first (noisy) window — it's
            // marked for discard and must not trigger a false transition.
            if (freq > 0 && phase_last_freq_ > 0 && !phase_last_is_first_) {
                const uint32_t t1_ref_freq = (phase_valid_windows_ > 0)
                                                 ? (phase_freq_accum_ / phase_valid_windows_)
                                                 : phase_last_freq_;
                const uint32_t idx_ref = moto_index(t1_ref_freq, MOTO_FINAL_MATCH_HZ);
                const uint32_t idx_last = moto_index(phase_last_freq_);
                const uint32_t idx_cur = moto_index(freq);
                const uint32_t idx_cur_final = moto_index(freq, MOTO_FINAL_MATCH_HZ);

                bool transition_detected = false;
                if (idx_last != MOTO_NONE && idx_cur != MOTO_NONE && idx_last != idx_cur) {
                    if (!debug_trace_active_) {
                        debug_trace_begin("T1 start " + format_tone(t1_ref_freq, phase_window_count_ * DETECT_WINDOW_MS));
                    }
                    debug_log("T1->T2 snap " + format_tone(phase_last_freq_, DETECT_WINDOW_MS) +
                              " -> " + format_tone(freq, DETECT_WINDOW_MS));
                    transition_detected = true;
                } else if (idx_ref != MOTO_NONE &&
                           idx_cur_final != MOTO_NONE &&
                           idx_cur_final != idx_ref &&
                           abs_diff_u32(freq, t1_ref_freq) >= MOTO_TRANSITION_DELTA_HZ) {
                    t1_transition_candidate_windows_++;
                    if (debug_trace_active_) {
                        debug_log("T1 cand " + to_string_dec_uint(t1_transition_candidate_windows_) +
                                  " ref=" + to_string_dec_uint(t1_ref_freq) +
                                  " cur=" + to_string_dec_uint(freq));
                    }
                    transition_detected = (t1_transition_candidate_windows_ >= 2);
                } else {
                    if (debug_trace_active_ && t1_transition_candidate_windows_ > 0) {
                        debug_log("T1 cand reset");
                    }
                    t1_transition_candidate_windows_ = 0;
                }

                if (transition_detected) {
                    // Transition detected.
                    // phase_last_freq_ is the last T1 window → discard.
                    t1_avg_freq_ = (phase_valid_windows_ > 0)
                                       ? (phase_freq_accum_ / phase_valid_windows_)
                                       : phase_last_freq_;
                    t1_window_count_ = phase_window_count_;

                    // Start T2 with current window as first (also discarded)
                    detect_state_ = DetectState::T2_COLLECTING;
                    phase_window_count_ = 1;
                    phase_freq_accum_ = 0;
                    phase_valid_windows_ = 0;
                    phase_last_freq_ = freq;
                    phase_last_is_first_ = true;
                    t1_transition_candidate_windows_ = 0;
                    t2_zero_window_count_ = 0;

                    if (!debug_trace_active_) {
                        debug_trace_begin("T1 start " + format_tone(t1_avg_freq_, t1_window_count_ * DETECT_WINDOW_MS));
                    }
                    debug_log("T2 start " + format_tone(freq, DETECT_WINDOW_MS));
                    text_status.set("T1:" + format_tone(t1_avg_freq_, t1_window_count_ * DETECT_WINDOW_MS) + " T2...");
                    break;
                }
            }

            // No transition: promote pending window into accumulator (unless first)
            if (!phase_last_is_first_ && phase_last_freq_ > 0) {
                phase_freq_accum_ += phase_last_freq_;
                phase_valid_windows_++;
            }
            phase_last_freq_ = freq;
            phase_last_is_first_ = false;
            phase_window_count_++;

            if (!debug_trace_active_ &&
                phase_window_count_ >= 3 &&
                moto_index(phase_last_freq_, MOTO_FINAL_MATCH_HZ) != MOTO_NONE) {
                const uint32_t t1_ref_freq = (phase_valid_windows_ > 0)
                                                 ? (phase_freq_accum_ / phase_valid_windows_)
                                                 : phase_last_freq_;
                debug_trace_begin("T1 start " + format_tone(t1_ref_freq, phase_window_count_ * DETECT_WINDOW_MS));
            }

            text_status.set("T1 " + to_string_dec_uint(phase_window_count_ * DETECT_WINDOW_MS) + "ms...");
            break;
        }

        case DetectState::T2_COLLECTING:
            // Promote pending window into accumulator (unless first)
            if (!phase_last_is_first_ && phase_last_freq_ > 0) {
                phase_freq_accum_ += phase_last_freq_;
                phase_valid_windows_++;
            }
            phase_last_freq_ = freq;
            phase_last_is_first_ = false;
            phase_window_count_++;

            text_status.set("T1:" + format_tone(t1_avg_freq_, t1_window_count_ * DETECT_WINDOW_MS) +
                            " T2 " + to_string_dec_uint(phase_window_count_ * DETECT_WINDOW_MS) + "ms");
            break;
    }
}

void TwoToneRxView::log_tone_pair(uint32_t f1, uint32_t d1_ms, uint32_t f2, uint32_t d2_ms) {
    // Keep the oldest entry at the top so newer pairs appear below it.
    tone_log_entries_.push_back({++next_log_serial_, format_tone(f1, d1_ms) + " " + format_tone(f2, d2_ms)});
    tone_log_view.set_dirty();
}

}  // namespace ui::external_app::two_tone_rx

namespace ui {

template <>
void RecentEntriesTable<ui::external_app::two_tone_rx::TwoToneLogEntries>::draw(
    const Entry& entry,
    const Rect& target_rect,
    Painter& painter,
    const Style& style,
    ui::RecentEntriesColumns&) {
    std::string line = entry.line;
    line.resize(target_rect.width() / 8, ' ');
    painter.draw_string(target_rect.location(), style, line);
}

}  // namespace ui
