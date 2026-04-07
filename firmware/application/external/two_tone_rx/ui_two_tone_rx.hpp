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

#ifndef __UI_TWO_TONE_RX_H__
#define __UI_TWO_TONE_RX_H__

#include "ui.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "ui_freq_field.hpp"
#include "ui_spectrum.hpp"
#include "app_settings.hpp"
#include "radio_state.hpp"
#include "message.hpp"
#include "string_format.hpp"
#include "log_file.hpp"
#include "recent_entries.hpp"

namespace ui::external_app::two_tone_rx {

struct TwoToneLogEntry {
    using Key = uint32_t;

    static constexpr Key invalid_key = 0;

    Key serial{};
    std::string line{};

    Key key() const { return serial; }
};

using TwoToneLogEntries = RecentEntries<TwoToneLogEntry>;

class TwoToneRxView : public View {
   public:
    TwoToneRxView(NavigationView& nav);
    ~TwoToneRxView();

    TwoToneRxView(const TwoToneRxView&) = delete;
    TwoToneRxView(TwoToneRxView&&) = delete;
    TwoToneRxView& operator=(const TwoToneRxView&) = delete;
    TwoToneRxView& operator=(TwoToneRxView&&) = delete;

    void focus() override;
    void on_hide() override;
    std::string title() const override { return "2-Tone RX"; };

   private:
    NavigationView& nav_;

    RxRadioState radio_state_{};

    bool running_{false};

    TwoToneLogEntries tone_log_entries_{};
    uint32_t next_log_serial_{0};

    uint32_t squelch_val{50};
    uint32_t ctcss_idx{0};
    bool bias_t_enabled{false};
    bool prior_antenna_bias_{false};
    bool applied_bias_t_{false};
    LogFile debug_file_{};

    app_settings::SettingsManager settings_{
        "rx_twotone",
        app_settings::Mode::RX,
        {
            {"tone_sq"sv, &squelch_val},
            {"ctcss"sv, &ctcss_idx},
            {"bias_t"sv, &bias_t_enabled},
        }};

    void start_rx();
    void stop_rx();
    void apply_bias_t(bool active);
    void debug_log(const std::string& line);
    void debug_trace_begin(const std::string& line);
    void on_tone_data(const ToneDetectDataMessage* msg);
    void log_tone_pair(uint32_t f1, uint32_t d1_ms, uint32_t f2, uint32_t d2_ms);
    void finalize_detected_pair(uint32_t t2_avg, uint32_t t2_dur, const char* reason);
    void reset_detect_state();

    // ── Two-tone detection state machine ─────────────────────────────────────
    //
    // Each 40 ms measurement window from the baseband arrives as tone_end=false.
    // tone_end=true signals that the CTCSS gate closed (end of transmission).
    //
    // Collection rule: discard first and last window of each phase.
    //   - "First" is always discarded (may contain carrier ramp-up).
    //   - "Last" is never added to the accumulator while pending; it is
    //     discarded when the next window confirms it is not the last, OR when
    //     the phase ends (tone_end=true or T1→T2 transition).
    //
    // Transition detection: two consecutive windows that snap to different MOTO
    // table entries trigger T1→T2. The transition window becomes T2's first
    // window (also discarded as T2's first).

    enum class DetectState : uint8_t { IDLE,
                                       T1_COLLECTING,
                                       T2_COLLECTING };
    DetectState detect_state_{DetectState::IDLE};

    // Per-phase window collection
    uint32_t phase_window_count_{0};   // total windows seen in this phase (including first)
    uint32_t phase_freq_accum_{0};     // sum of non-first, non-last window frequencies
    uint32_t phase_valid_windows_{0};  // count of windows in phase_freq_accum_
    uint32_t phase_last_freq_{0};      // pending window (not yet added; discarded if last)
    bool phase_last_is_first_{true};   // true when phase_last_freq_ is window 1 (discard)

    // T1 result (stored at T1→T2 transition for use when pair is logged)
    uint32_t t1_avg_freq_{0};
    uint32_t t1_window_count_{0};
    uint8_t t1_transition_candidate_windows_{0};
    uint8_t t2_zero_window_count_{0};
    bool debug_trace_active_{false};
    uint32_t debug_trace_id_{0};
    uint32_t debug_trace_counter_{0};

    // ── Row 0: frequency + RF controls ───────────────────────────────────────

    RxFrequencyField field_frequency{
        {UI_POS_X(0), UI_POS_Y(0)},
        nav_};
    RFAmpField field_rf_amp{
        {UI_POS_X(13), UI_POS_Y(0)}};
    LNAGainField field_lna{
        {UI_POS_X(15), UI_POS_Y(0)}};
    VGAGainField field_vga{
        {UI_POS_X(18), UI_POS_Y(0)}};
    RSSI rssi{
        {UI_POS_X(21), UI_POS_Y(0), UI_POS_WIDTH_REMAINING(24), 4}};
    AudioVolumeField field_volume{
        {UI_POS_X_RIGHT(2), UI_POS_Y(0)}};

    // ── Row 1: CTCSS / Squelch / Start+Stop / Clear ──────────────────────────

    Labels labels{
        {{0 * 8, 1 * 16}, "CTCSS:", Theme::getInstance()->fg_light->foreground},
        {{15 * 8, 1 * 16}, "Sq:", Theme::getInstance()->fg_light->foreground},
    };

    OptionsField options_ctcss{
        {7 * 8, 1 * 16},
        8,
        {}};

    NumberField field_squelch{
        {19 * 8, 1 * 16},
        2,
        {0, 99},
        1,
        ' '};

    Button button_startstop{
        {21 * 8, 1 * 16, 5 * 8, 18},
        "Start"};

    Button button_clear{
        {26 * 8, 1 * 16, 4 * 8, 18},
        "Clr"};

    // ── Row 2: live status (active tone in progress) + Bias-T toggle ─────────

    Text text_status{
        {0, 2 * 16 + 4, 20 * 8, 16},
        ""};

    Checkbox check_bias_t{
        {20 * 8, 2 * 16 + 2},
        9,
        "Bias-T",
        true};

    ui::RecentEntriesColumns tone_log_columns{{
        {"Tones", 0},
    }};
    ui::RecentEntriesTable<TwoToneLogEntries> tone_log_view{tone_log_entries_, tone_log_columns};

    // ── Waterfall (rest of screen) ────────────────────────────────────────────

    spectrum::WaterfallView waterfall{};

    // ── Message handler ───────────────────────────────────────────────────────

    MessageHandlerRegistration message_handler_tone{
        Message::ID::ToneDetectData,
        [this](const Message* const p) {
            const auto* msg = reinterpret_cast<const ToneDetectDataMessage*>(p);
            this->on_tone_data(msg);
        }};
};

}  // namespace ui::external_app::two_tone_rx

#endif /* __UI_TWO_TONE_RX_H__ */
