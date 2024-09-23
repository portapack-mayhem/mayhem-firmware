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

#include "app_settings.hpp"
#include "receiver_model.hpp"
#include "recent_entries.hpp"
#include "radio_state.hpp"
#include "spectrum_color_lut.hpp"
#include "ui_receiver.hpp"

namespace ui {

#define SEARCH_SLICE_WIDTH 2500000                // Search slice bandwidth
#define SEARCH_BIN_NB 256                         // FFT power bins
#define SEARCH_BIN_NB_NO_DC (SEARCH_BIN_NB - 16)  // Bins after trimming
#define SEARCH_BIN_WIDTH (SEARCH_SLICE_WIDTH / SEARCH_BIN_NB)

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
    RxRadioState radio_state_{
        100'000'000 /* frequency */,
        2500000 /* bandwidth */,
        SEARCH_SLICE_WIDTH /* sampling rate */,
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

    struct slice_t {
        rf::Frequency center_frequency;
        uint8_t max_power;
        int16_t max_index;
        uint8_t power;
        int16_t index;
    } slices[32];

    uint32_t bin_skip_acc = 0;
    uint32_t bin_skip_frac = 0;
    uint32_t pixel_index = 0;
    std::array<Color, 240> spectrum_row{};
    ChannelSpectrumFIFO* fifo = nullptr;

    uint8_t detect_timer = 0;
    uint8_t release_timer = 0;
    uint8_t timing_div = 0;
    uint8_t overall_power_max{0};
    uint32_t mean_power = 0;
    uint32_t mean_acc = 0;
    uint32_t duration = 0;

    rf::Frequency slice_start = 0;
    uint8_t slices_nb = 0;
    uint8_t slice_counter = 0;
    int16_t last_bin = 0;
    uint32_t last_slice = 0;
    Coord last_tick_pos = 0;
    rf::Frequency search_span = 0;
    rf::Frequency resolved_frequency = 0;
    uint16_t locked_bin = 0;
    uint8_t search_counter = 0;
    bool locked = false;

    void do_detection();
    void do_timers();
    void on_channel_spectrum(const ChannelSpectrum& spectrum);
    void on_range_changed();
    void add_spectrum_pixel(Color color);

    const RecentEntriesColumns columns{{{"Frequency", 9},
                                        {"Time", 8},
                                        {"Duration", 11}}};
    SearchRecentEntries recent{};
    RecentEntriesView<RecentEntries<SearchRecentEntry>> recent_entries_view{columns, recent};

    Labels labels{
        {{1 * 8, 0}, "Min:      Max:       LNA VGA", Theme::getInstance()->fg_light->foreground},
        {{1 * 8, 4 * 8}, "Trig:   /255    Mean:   /255", Theme::getInstance()->fg_light->foreground},
        {{1 * 8, 6 * 8}, "Slices:  /32      Rate:   Hz", Theme::getInstance()->fg_light->foreground},
        {{6 * 8, 10 * 8}, "Timer  Status", Theme::getInstance()->fg_light->foreground},
        {{1 * 8, 25 * 8}, "Accuracy +/-4.9kHz", Theme::getInstance()->fg_light->foreground},
        {{26 * 8, 25 * 8}, "MHz", Theme::getInstance()->fg_light->foreground}};

    FrequencyField field_frequency_min{
        {1 * 8, 1 * 16}};
    FrequencyField field_frequency_max{
        {11 * 8, 1 * 16}};

    LNAGainField field_lna{
        {22 * 8, 1 * 16}};
    VGAGainField field_vga{
        {26 * 8, 1 * 16}};

    NumberField field_threshold{
        {6 * 8, 2 * 16},
        3,
        {5, 255},
        5,
        ' '};
    Text text_mean{
        {22 * 8, 2 * 16, 3 * 8, 16},
        "---"};
    Text text_slices{
        {8 * 8, 3 * 16, 2 * 8, 16},
        "--"};
    Text text_rate{
        {24 * 8, 3 * 16, 3 * 8, 16},
        "---"};

    VuMeter vu_max{
        {1 * 8, 11 * 8 - 4, 3 * 8, 48},
        18,
        false};

    ProgressBar progress_timers{
        {6 * 8, 12 * 8, 6 * 8, 16}};
    Text text_infos{
        {13 * 8, 12 * 8, 15 * 8, 16},
        "Listening"};

    Checkbox check_snap{
        {6 * 8, 15 * 8},
        7,
        "Snap to:",
        true};
    OptionsField options_snap{
        {17 * 8, 15 * 8},  // Position
        7,                 // Length
        {                  // Options
         {"25kHz  ", 25'000},
         {"12.5kHz", 12'500},
         {"8.33kHz", 8'333},
         {"2.5kHz", 2'500},
         {"500Hz", 500}}};

    BigFrequency big_display{
        {4, 9 * 16, 28 * 8, 52},
        0};

    MessageHandlerRegistration message_handler_spectrum_config{
        Message::ID::ChannelSpectrumConfig,
        [this](const Message* const p) {
            const auto message = *reinterpret_cast<const ChannelSpectrumConfigMessage*>(p);
            this->fifo = message.fifo;
        }};

    MessageHandlerRegistration message_handler_frame_sync{
        Message::ID::DisplayFrameSync,
        [this](const Message* const) {
            if (this->fifo) {
                ChannelSpectrum channel_spectrum;
                while (fifo->out(channel_spectrum)) {
                    this->on_channel_spectrum(channel_spectrum);
                }
            }
            this->do_timers();
        }};
};

} /* namespace ui */
