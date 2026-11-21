/*
 * Copyright (C) 2026 zxkmm
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

#ifndef __UI_GPS_SCAN_H__
#define __UI_GPS_SCAN_H__

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "ui_freq_field.hpp"
#include "ui_rssi.hpp"
#include "radio_state.hpp"
#include "event_m0.hpp"
#include "gps_scan_types.hpp"

#include <array>
#include <string>

namespace ui {

// Shared state rendered by GPSScanPanel (avoids 20+ child Text widgets & stack bloat).
struct GPSScanModel {
    gps_scan::SignalStats stats{};
    gps_scan::CorrProfile profile{};
    gps_scan::TrackStatus track{};
    std::array<gps_scan::PRNResult, gps_scan::NUM_PRNS> results{};
    std::array<uint8_t, gps_scan::NUM_PRNS> sorted_prns{};
    size_t result_count{0};
    size_t selected_rank{0};
    uint8_t selected_prn{0};
    uint16_t scan_ms{0};
    uint32_t ifft_count{0};
    uint32_t scan_id{0};
    bool has_profile{false};
    bool has_track{false};
    uint8_t view_mode{0};  // 0 = List+Plot, 1 = Track, 2 = Diag
};

class GPSScanPanel : public Widget {
   public:
    GPSScanPanel(Rect parent_rect, const GPSScanModel& model)
        : Widget{parent_rect},
          model_{model} {
        invalidate_full();
    }

    void on_show() override {
        invalidate_full();
        set_dirty();
    }

    void invalidate_full() {
        needs_clear_ = true;
        iq_chrome_drawn_ = false;
        plot_chrome_drawn_ = false;
        prev_iq_count_ = 0;
        prev_sf_mask_ = 0xFF;
        for (auto& slot : line_cache_) {
            slot.text[0] = '\x7F';
            slot.style = nullptr;
        }
    }

    void paint(Painter& painter) override;

   private:
    static constexpr size_t kCacheSlots = 14;

    struct LineCache {
        const Style* style{nullptr};
        char text[32]{};
    };

    void draw_cached(Painter& painter, Coord x, Coord y, size_t width_chars, const Style* style, const char* text, size_t slot);
    void paint_list_and_plot(Painter& painter, const Rect& r);
    void paint_tracking(Painter& painter, const Rect& r);
    void paint_diagnostics(Painter& painter, const Rect& r);

    const GPSScanModel& model_;
    bool needs_clear_{true};
    bool iq_chrome_drawn_{false};
    bool plot_chrome_drawn_{false};
    uint8_t prev_view_mode_{0xFF};
    bool prev_has_track_{false};
    uint8_t prev_sf_mask_{0xFF};
    size_t prev_iq_count_{0};
    Point prev_iq_[gps_scan::TRACK_IQ_HISTORY]{};
    LineCache line_cache_[kCacheSlots]{};
};

class GPSScanView : public View {
   public:
    GPSScanView(NavigationView& nav);
    ~GPSScanView();

    GPSScanView(const GPSScanView&) = delete;
    GPSScanView& operator=(const GPSScanView&) = delete;

    void focus() override;
    std::string title() const override { return "GPS L1 Rx"; }

   private:
    void on_stats(const GPSScanStatsMessage& msg);
    void on_result(const GPSScanResultMessage& msg);
    void on_profile(const GPSScanProfileMessage& msg);
    void on_track(const GPSScanTrackMessage& msg);

    void send_config(gps_scan::Mode mode, uint8_t prn_override = 0, int16_t dopp_hint = 0);
    void toggle_bias_tee();
    void update_bias_button();
    void update_view_button();
    void update_action_buttons();
    void cycle_integ();
    void cycle_dopp();
    void sort_results();
    void refresh_adc_strip();
    void select_rank(int delta);
    void request_selected_profile();
    void start_tracking_selected(bool selftest);

    NavigationView& nav_;

    // Direct-conversion 1575.42 MHz, 2.5 MHz LPF, 2.048 Msps, Mode::SpectrumAnalysis (0 Hz offset!)
    RxRadioState radio_state_{
        gps_scan::GPS_L1_FREQ_HZ,
        gps_scan::BASEBAND_BW_HZ,
        gps_scan::SAMPLING_RATE_HZ,
        ReceiverModel::Mode::SpectrumAnalysis};

    GPSScanModel model_{};

    gps_scan::Integration integ_{gps_scan::Integration::Ms2x4};
    uint8_t dopp_idx_{0};  // 0=5k, 1=10k, 2=20k, 3=35k
    bool scanning_{false};
    bool tracking_{false};
    bool auto_rescan_{false};
    bool selftest_active_{false};

    // Row 0: RF Header (y = 0)
    RxFrequencyField field_frequency{
        {UI_POS_X(0), UI_POS_Y(0)},
        nav_};
    Labels labels_rf{
        {{UI_POS_X(10), UI_POS_Y(0)}, "A", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(13), UI_POS_Y(0)}, "L", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(17), UI_POS_Y(0)}, "V", Theme::getInstance()->fg_light->foreground},
        {{146, 16}, "P", Theme::getInstance()->fg_light->foreground},
    };
    RFAmpField field_rf_amp{
        {UI_POS_X(11), UI_POS_Y(0)}};
    LNAGainField field_lna{
        {UI_POS_X(14), UI_POS_Y(0)}};
    VGAGainField field_vga{
        {UI_POS_X(18), UI_POS_Y(0)}};
    RSSI rssi{
        {UI_POS_X(21), 2, UI_POS_WIDTH(9), 12}};

    // Row 1: Bias-Tee + Integration + Doppler Span + PRN (00=All) + Auto (y = 16)
    Button button_bias{
        {0, 16, 56, 16},
        "ANT:OFF"};
    Button button_integ{
        {58, 16, 42, 16},
        "2msx4"};
    Button button_dopp{
        {102, 16, 42, 16},
        "+-5k"};
    NumberField field_prn{
        {154, 16},
        2,
        {0, 32},
        1,
        '0'};
    Checkbox check_auto{
        {176, 16},
        4,
        "Auto",
        true};

    // Row 2: Action buttons (y = 34..54)
    Button button_scan{
        {0, 34, 52, 20},
        "Scan"};
    Button button_track{
        {54, 34, 54, 20},
        "Track"};
    Button button_test{
        {110, 34, 42, 20},
        "Test"};
    Button button_view{
        {154, 34, 40, 20},
        "Trk"};
    Button button_prev{
        {196, 34, 21, 20},
        "<"};
    Button button_next{
        {219, 34, 21, 20},
        ">"};

    // Row 3..5: Status, Progress Bar, ADC Health Strip (y = 56..97)
    Text text_status{
        {0, 56, UI_POS_MAXWIDTH, 16},
        "IDLE  Ready (Scan or Track)"};
    ProgressBar progress_bar{
        {0, 73, UI_POS_MAXWIDTH, 7}};
    Text text_adc_strip{
        {0, 82, UI_POS_MAXWIDTH, 16},
        "ADC: waiting for DMA..."};

    // Unified List+Plot / Tracking / Diagnostics panel (y = 98..304)
    GPSScanPanel panel{
        {0, 98, UI_POS_MAXWIDTH, 206},
        model_};

    MessageHandlerRegistration message_handler_stats{
        Message::ID::GPSScanStats,
        [this](Message* const p) {
            const auto message = static_cast<const GPSScanStatsMessage*>(p);
            this->on_stats(*message);
        }};

    MessageHandlerRegistration message_handler_result{
        Message::ID::GPSScanResult,
        [this](Message* const p) {
            const auto message = static_cast<const GPSScanResultMessage*>(p);
            this->on_result(*message);
        }};

    MessageHandlerRegistration message_handler_profile{
        Message::ID::GPSScanProfile,
        [this](Message* const p) {
            const auto message = static_cast<const GPSScanProfileMessage*>(p);
            this->on_profile(*message);
        }};

    MessageHandlerRegistration message_handler_track{
        Message::ID::GPSScanTrack,
        [this](Message* const p) {
            const auto message = static_cast<const GPSScanTrackMessage*>(p);
            this->on_track(*message);
        }};
};

} /* namespace ui */

#endif /* __UI_GPS_SCAN_H__ */
