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

#ifndef _UI_GLASS
#define _UI_GLASS

#include "ui.hpp"
#include "portapack.hpp"
#include "app_settings.hpp"
#include "baseband_api.hpp"
#include "radio_state.hpp"
#include "receiver_model.hpp"
#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "string_format.hpp"
#include "analog_audio_app.hpp"
#include "gradient.hpp"

namespace ui {

#define LOOKING_GLASS_SLICE_WIDTH_MAX 20000000
#define LOOKING_GLASS_MAX_SAMPLERATE 20000000
#define MHZ_DIV 1000000

// blanked DC (16 centered bins ignored ) and top left and right (2 bins ignored on each side )
#define LOOKING_GLASS_FASTSCAN 0
// only first half used (so DC spike is not ignored, it's stopped before the DC spike) minus the 2 first bins
#define LOOKING_GLASS_SLOWSCAN 1
// analog audio view like
#define LOOKING_GLASS_SINGLEPASS 2
// one spectrum line number of bins
#define SPEC_NB_BINS 256

class GlassView : public View {
   public:
    GlassView(NavigationView& nav);

    GlassView(const GlassView&);
    GlassView& operator=(const GlassView& nav);

    ~GlassView();
    std::string title() const override { return "Looking Glass"; };

    void on_show() override;
    void on_hide() override;
    void focus() override;

   private:
    NavigationView& nav_;
    Gradient gradient{};
    RxRadioState radio_state_{ReceiverModel::Mode::SpectrumAnalysis};
    // Settings
    rf::Frequency f_min = 260 * MHZ_DIV;  // Default to 315/433 remote range.
    rf::Frequency f_max = 500 * MHZ_DIV;
    uint8_t preset_index = 0;  // Manual
    uint8_t filter_index = 0;  // OFF
    uint8_t trigger = 32;
    uint8_t mode = LOOKING_GLASS_FASTSCAN;
    uint8_t live_frequency_view = 0;       // Spectrum
    uint8_t live_frequency_integrate = 3;  // Default (3 * old value + new_value) / 4
    int32_t beep_squelch = 20;             // range from -100 to +20, >=20 disabled
    bool beep_enabled = false;             // activate on bip button click
    app_settings::SettingsManager settings_{
        "rx_glass"sv,
        app_settings::Mode::RX,
        {
            {"min"sv, &f_min},
            {"max"sv, &f_max},
            {"preset"sv, &preset_index},
            {"filter"sv, &filter_index},
            {"trigger"sv, &trigger},
            {"scan_mode"sv, &mode},
            {"freq_view"sv, &live_frequency_view},
            {"freq_integrate"sv, &live_frequency_integrate},
            {"beep_squelch"sv, &beep_squelch},
            {"beep_enabled"sv, &beep_enabled},
        }};

    struct preset_entry {
        rf::Frequency min{};
        rf::Frequency max{};
        std::string label{};
    };

    void on_freqchg(int64_t freq);
    // Function to map the value from one range to another
    int32_t map(int32_t value, int32_t fromLow, int32_t fromHigh, int32_t toLow, int32_t toHigh);
    std::vector<preset_entry> presets_db{};
    void manage_beep_audio();
    void update_display_beep();
    void update_min(int32_t v);
    void update_max(int32_t v);
    void update_range_field();
    void get_max_power(const ChannelSpectrum& spectrum, uint16_t bin, uint8_t& max_power);
    rf::Frequency get_freq_from_bin_pos(uint16_t pos);
    void on_marker_change(int8_t delta);
    void retune();
    bool process_bins(uint8_t* powerlevel);
    void on_channel_spectrum(const ChannelSpectrum& spectrum);
    void do_timers();
    int64_t next_mult_of(int64_t num, int64_t multiplier);
    void adjust_range(int64_t* f_min, int64_t* f_max, int64_t width);
    void on_range_changed();
    void reset_live_view();
    void add_spectrum_pixel(uint8_t power);
    void plot_marker(uint16_t pos);
    void load_presets();
    void populate_presets();
    void launch_audio(rf::Frequency center_freq);

    rf::Frequency search_span{0};
    rf::Frequency f_center{0};
    rf::Frequency f_center_ini{0};

    rf::Frequency marker{0};
    uint16_t marker_pixel_index{0};
    rf::Frequency marker_pixel_step{0};

    // Size of one spectrum bin in Hz.
    rf::Frequency each_bin_size{0};
    // Bandwidth of a single spectrum bin.
    rf::Frequency bins_hz_size{0};
    rf::Frequency looking_glass_sampling_rate{0};
    rf::Frequency looking_glass_bandwidth{0};
    rf::Frequency looking_glass_range{0};
    rf::Frequency looking_glass_step{0};
    uint8_t min_color_power{0};  // Filter cutoff level.
    uint32_t pixel_index{0};

    std::vector<Color> spectrum_row{};
    std::vector<uint8_t> spectrum_data{};
    ChannelSpectrumFIFO* fifo{};

    int32_t steps = 1;
    bool locked_range = false;

    uint8_t range_max_power = 0;
    uint8_t range_max_power_counter = 0;
    uint8_t max_power = 0;
    rf::Frequency max_freq_hold = 0;
    rf::Frequency last_max_freq = 0;
    int16_t max_freq_power = -1000;
    uint8_t bin_length = screen_width;
    uint8_t offset = 0;
    uint8_t ignore_dc = 0;
    bool show_rssi_guides = false;

    // start of spectrum area
    static constexpr int spectrum_y = (108 + 16);

    Labels labels{
        {{0, UI_POS_Y(0)}, "MIN:     MAX:     LNA   VGA  ", Theme::getInstance()->fg_light->foreground},
        {{0, UI_POS_Y(1)}, "RANGE:       FILTER:     AMP:", Theme::getInstance()->fg_light->foreground},
        {{0, UI_POS_Y(2)}, "P:", Theme::getInstance()->fg_light->foreground},
        {{0, UI_POS_Y(3)}, "MARKER( / ):          MHz", Theme::getInstance()->fg_light->foreground},
        {{0, UI_POS_Y(4)}, "RES:     VOL:", Theme::getInstance()->fg_light->foreground}};

    NumberField field_frequency_min{
        {UI_POS_X(4), UI_POS_Y(0)},
        4,
        {0, 7199},
        1,  // number of steps by encoder delta
        ' '};

    NumberField field_frequency_max{
        {UI_POS_X(13), UI_POS_Y(0)},
        4,
        {1, 7200},
        1,  // number of steps by encoder delta
        ' '};

    LNAGainField field_lna{
        {UI_POS_X(21), UI_POS_Y(0)}};

    VGAGainField field_vga{
        {UI_POS_X(27), UI_POS_Y(0)}};

    TextField field_range{
        {UI_POS_X(6), UI_POS_Y(1), UI_POS_WIDTH(6), UI_POS_HEIGHT(1)},
        ""};

    OptionsField filter_config{
        {UI_POS_X(20), UI_POS_Y(1)},
        4,
        {
            {"OFF ", 0},
            {"MID ", 118},  // 85 + 25 (110) + a bit more to kill all blue
            {"HIGH", 202},  // 168 + 25 (193)
        }};

    RFAmpField field_rf_amp{
        {UI_POS_X(28), UI_POS_Y(1)}};

    OptionsField range_presets{
        {UI_POS_X(2), UI_POS_Y(2)},
        20,
        {}};

    ButtonWithEncoder button_beep_squelch{
        {UI_POS_X_RIGHT(8), UI_POS_Y(2.25), UI_POS_WIDTH(8), UI_POS_HEIGHT(0.5)},
        ""};

    TextField field_marker{
        {UI_POS_X(12), UI_POS_Y(3), UI_POS_WIDTH(9), UI_POS_HEIGHT(1)},
        ""};

    Button button_marker_minus{
        {UI_POS_X(7), UI_POS_Y(3.25), UI_POS_WIDTH(1), UI_POS_HEIGHT(0.5)},
        "-"};

    Button button_marker_plus{
        {UI_POS_X(9), UI_POS_Y(3.25), UI_POS_WIDTH(1), UI_POS_HEIGHT(0.5)},
        "+"};

    NumberField field_trigger{
        {UI_POS_X(4), UI_POS_Y(4)},
        3,
        {2, 128},
        2,
        ' '};

    AudioVolumeField field_volume{
        {UI_POS_X(13), UI_POS_Y(4)}};

    /*OptionsField steps_config{
        {UI_POS_X(13), UI_POS_Y(4)},
        3,
        {
            {"1", 1},
            {"25", 25},
            {"50", 50},
            {"100", 100},
            {"250", 250},
            {"500", 500},
        }};*/

    OptionsField scan_type{
        {UI_POS_X(17), UI_POS_Y(4)},
        2,
        {
            {"F-", LOOKING_GLASS_FASTSCAN},
            {"S-", LOOKING_GLASS_SLOWSCAN},
        }};

    OptionsField view_config{
        {UI_POS_X(19), UI_POS_Y(4)},
        7,
        {
            {"SPCTR-V", 0},
            {"LEVEL-V", 1},
            {"PEAK-V", 2},
        }};

    OptionsField level_integration{
        {UI_POS_X(27), UI_POS_Y(4)},
        2,
        {
            {"x0", 0},
            {"x1", 1},
            {"x2", 2},
            {"x3", 3},
            {"x4", 4},
            {"x5", 5},
            {"x6", 6},
            {"x7", 7},
            {"x8", 8},
            {"x9", 9},
        }};

    Button button_jump{
        {UI_POS_X_RIGHT(4), UI_POS_Y(5), UI_POS_WIDTH(4), UI_POS_HEIGHT(1)},
        "JMP"};

    Button button_rst{
        {UI_POS_X_RIGHT(9), UI_POS_Y(5), UI_POS_WIDTH(4), UI_POS_HEIGHT(1)},
        "RST"};

    Button button_rssi{
        {UI_POS_X_RIGHT(15), UI_POS_Y(5), UI_POS_WIDTH(5), UI_POS_HEIGHT(1)},
        "RSSI"};

    Text freq_stats{
        {UI_POS_X(0), UI_POS_Y(5), UI_POS_WIDTH(14), UI_POS_HEIGHT(1)},
        ""};

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
        }};

    MessageHandlerRegistration message_handler_freqchg{
        Message::ID::FreqChangeCommand,
        [this](Message* const p) {
            const auto message = static_cast<const FreqChangeCommandMessage*>(p);
            this->on_freqchg(message->freq);
        }};
};
}  // namespace ui
#endif
