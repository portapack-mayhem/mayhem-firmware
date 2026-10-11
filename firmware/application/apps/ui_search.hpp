/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
 * Copyright (C) 2026 gullradriel, Nilorea Studio Inc.
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

#include "app_settings.hpp"
#include "receiver_model.hpp"
#include "recent_entries.hpp"
#include "radio_state.hpp"
#include "gradient.hpp"
#include "ui_receiver.hpp"
#include "log_file.hpp"
#include "file_path.hpp"
#include "spec_an_sweep.hpp"

namespace ui {

#define DETECT_DELAY 5  // In 100ms units
#define RELEASE_DELAY 6

struct SearchRecentEntry {
    using Key = rf::Frequency;

    static constexpr Key invalid_key = 0xffffffff;

    rf::Frequency frequency;
    uint32_t duration{0};  // In 100ms units
    std::string time{""};

    SearchRecentEntry()
        : SearchRecentEntry{0} {
    }

    SearchRecentEntry(
        const rf::Frequency frequency)
        : frequency{frequency} {
    }

    Key key() const {
        return frequency;
    }

    void set_time(std::string& new_time) {
        time = new_time;
    }

    void set_duration(uint32_t new_duration) {
        duration = new_duration;
    }
};

using SearchRecentEntries = RecentEntries<SearchRecentEntry>;

class SearchLogger {
   public:
    Optional<File::Error> append(const std::filesystem::path& filename) {
        return log_file.append(filename);
    }

    void log_data(SearchRecentEntry& data);
    void write_header() {
        log_file.write_raw("Time;Freq;Duration;");
    }

   private:
    LogFile log_file{};
};

class SearchView : public View {
   public:
    SearchView(NavigationView& nav);
    ~SearchView();

    SearchView(const SearchView&) = delete;
    SearchView(SearchView&&) = delete;
    SearchView& operator=(const SearchView&) = delete;
    SearchView& operator=(SearchView&&) = delete;

    void on_show() override;
    void on_hide() override;
    void focus() override;

    std::string title() const override { return "Search"; };

   private:
    NavigationView& nav_;
    Gradient gradient{};
    RxRadioState radio_state_{
        100'000'000 /* frequency */,
        2500000 /* bandwidth */,
        2500000 /* sampling rate */,
        ReceiverModel::Mode::SpectrumAnalysis};

    // Settings
    struct SearchSettings {
        uint32_t power_threshold = 80;
        rf::Frequency freq_min = 100'000'000;
        rf::Frequency freq_max = 400'000'000;
        bool snap_search = true;
        uint32_t snap_step = 12'500;
    };
    SearchSettings settings_{};
    app_settings::SettingsManager app_settings_{
        "rx_search"sv,
        app_settings::Mode::RX,
        {
            {"power_threshold"sv, &settings_.power_threshold},
            {"freq_min"sv, &settings_.freq_min},
            {"freq_max"sv, &settings_.freq_max},
            {"snap_search"sv, &settings_.snap_search},
            {"snap_step"sv, &settings_.snap_step},
        }};

    /* Sweeps complete much faster than signals come and go (TDMA bursts,
     * fading): they are max-held over each 100 ms timer period and looked at
     * once per period.
     * Scan: sweeps the whole range, looking for a point that stays over the
     *       threshold for DETECT_DELAY.
     * Resolve: sweeps narrower and narrower spans around it until the points
     *       are no more than kResolvedSpacing apart. A zoom with no peak (a
     *       wideband signal fills it) settles for the best estimate so far.
     * Locked: sweeps the whole range again, the lock holds while the peak
     *       stays around the locked point. */
    enum class Stage : uint8_t {
        Scan,
        Resolve,
        Locked
    };

    static constexpr uint16_t points = 240;              // spectrum line width
    static constexpr uint16_t kAverage = 4;              // FFTs power-averaged per tune
    static constexpr uint32_t kResolvedSpacing = 2'500;  // Hz between points once resolved
    static constexpr uint32_t kZoomMinSpan = 240'000;    // narrowest zoom, room for some drift
    static constexpr uint8_t kZoomPoints = 6;            // zoom span, in points of the span before

    spec_an::Sweep sweep{};
    Stage stage{Stage::Scan};
    bool shown{false};
    std::vector<uint8_t> sweep_power{};  // the sweep in progress
    std::vector<uint8_t> hold_power{};   // max hold of the sweeps completed this period
    uint16_t hold_count{0};              // sweeps in hold_power
    std::vector<Color> spectrum_row{};
    bool row_ready{false};      // spectrum_row holds a period not drawn yet
    int16_t row_tick{-1};       // its point over the threshold, -1 if none
    int16_t last_index{-1};     // point over the threshold at the previous period
    int16_t locked_index{-1};   // range point of the signal being resolved or locked
    rf::Frequency estimate{0};  // best frequency found while resolving
    Coord last_tick_pos = 0;

    uint8_t detect_timer = 0;
    uint8_t release_timer = 0;
    uint8_t timing_div = 0;
    uint8_t overall_power_max{0};
    uint32_t mean_power = 0;
    uint32_t duration = 0;

    rf::Frequency resolved_frequency = 0;
    uint16_t search_counter = 0;
    bool logging = false;
    SearchLogger logger{};

    void start_sweep(rf::Frequency start, rf::Frequency span);
    void sweep_range();
    void start_scan();
    void zoom(uint16_t x);
    void lock(rf::Frequency frequency);
    void unlock();
    void on_slice(const SpecAnSliceMessage& message);
    void evaluate();
    void draw_row();
    void do_timers();
    void on_range_changed();

    RecentEntriesColumns columns{{{"Frequency", 0},
                                  {"Time", 8},
                                  {"Duration", 11}}};
    SearchRecentEntries recent{};
    RecentEntriesView<RecentEntries<SearchRecentEntry>> recent_entries_view{columns, recent};

    Labels labels{
        {{UI_POS_X(1), UI_POS_Y(0)}, "Min:      Max:       ", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X_RIGHT(7), UI_POS_Y(0)}, "LNA VGA", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(1), UI_POS_Y(2)}, "Trig:   /255", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X_RIGHT(12), UI_POS_Y(2)}, "Mean:   /255", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(1), UI_POS_Y(3)}, "Slices:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X_RIGHT(10), UI_POS_Y(3)}, "Rate:   Hz", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(6), UI_POS_Y(5)}, "Timer  Status", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(1), 25 * 8}, "Accuracy +/-1.3kHz", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X_RIGHT(4), 25 * 8}, "MHz", Theme::getInstance()->fg_light->foreground}};

    Checkbox check_log{
        {UI_POS_X_RIGHT(6), UI_POS_Y(5)},
        3,
        "LOG",
        true};

    FrequencyField field_frequency_min{
        {UI_POS_X(1), UI_POS_Y(1)}};
    FrequencyField field_frequency_max{
        {UI_POS_X(11), UI_POS_Y(1)}};

    LNAGainField field_lna{
        {UI_POS_X_RIGHT(7), UI_POS_Y(1)}};
    VGAGainField field_vga{
        {UI_POS_X_RIGHT(3), UI_POS_Y(1)}};

    NumberField field_threshold{
        {UI_POS_X(6), UI_POS_Y(2)},
        3,
        {5, 255},
        5,
        ' '};
    Text text_mean{
        {UI_POS_X_RIGHT(7), UI_POS_Y(2), UI_POS_WIDTH(3), UI_POS_HEIGHT(1)},
        "---"};
    Text text_slices{
        {UI_POS_X(8), UI_POS_Y(3), UI_POS_WIDTH(3), UI_POS_HEIGHT(1)},
        "---"};
    Text text_rate{
        {UI_POS_X_RIGHT(5), UI_POS_Y(3), UI_POS_WIDTH(3), UI_POS_HEIGHT(1)},
        "---"};

    VuMeter vu_max{
        {UI_POS_X(1), 11 * 8 - 4, 3 * 8, 48},
        18,
        false};

    ProgressBar progress_timers{
        {UI_POS_X(6), UI_POS_Y(6), UI_POS_WIDTH(6), UI_POS_HEIGHT(1)}};
    Text text_infos{
        {UI_POS_X(13), UI_POS_Y(6), UI_POS_WIDTH(15), UI_POS_HEIGHT(1)},
        "Listening"};

    Checkbox check_snap{
        {UI_POS_X(6), 15 * 8},
        7,
        "Snap to:",
        true};
    OptionsField options_snap{
        {UI_POS_X(17), 15 * 8},  // Position
        7,                       // Length
        {                        // Options
         {"25kHz  ", 25'000},
         {"12.5kHz", 12'500},
         {"8.33kHz", 8'333},
         {"2.5kHz", 2'500},
         {"500Hz", 500}}};

    BigFrequency big_display{{UI_POS_X_CENTER(28), UI_POS_Y(9), UI_POS_WIDTH(28), 52}, 0};

    // Swept on the M4 by the spectrum analyzer engine, see spec_an_shared.hpp.
    MessageHandlerRegistration message_handler_captured{
        Message::ID::SpecAnCaptured,
        [this](const Message* const p) {
            this->sweep.on_captured(*reinterpret_cast<const SpecAnCapturedMessage*>(p));
        }};

    MessageHandlerRegistration message_handler_slice{
        Message::ID::SpecAnSlice,
        [this](const Message* const p) {
            this->on_slice(*reinterpret_cast<const SpecAnSliceMessage*>(p));
        }};

    MessageHandlerRegistration message_handler_frame_sync{
        Message::ID::DisplayFrameSync,
        [this](const Message* const) {
            this->sweep.on_frame();
            if (this->row_ready)
                this->draw_row();
            this->do_timers();
        }};
};

} /* namespace ui */
