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
#include "spectrum_color_lut.hpp"

namespace ui {

#define LOOKING_GLASS_SLICE_WIDTH_MAX 20000000
#define MHZ_DIV 1000000

// blanked DC (16 centered bins ignored ) and top left and right (2 bins ignored on each side )
#define LOOKING_GLASS_FASTSCAN 0
// only first half used (so DC spike is not ignored, it's stopped before the DC spike) minus the 2 first bins
#define LOOKING_GLASS_SLOWSCAN 1
// analog audio view like
#define LOOKING_GLASS_SINGLEPASS 2
// one spectrum line number of bins
#define SPEC_NB_BINS 256
// screen dimensions
#define SCREEN_W 240
#define SCREEN_H 320

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

    uint8_t get_spec_iq_phase_calibration_value();
    void set_spec_iq_phase_calibration_value(uint8_t cal_value);

   private:
    NavigationView& nav_;
    RxRadioState radio_state_{ReceiverModel::Mode::SpectrumAnalysis};
    // Settings
    rf::Frequency f_min = 260 * MHZ_DIV;  // Default to 315/433 remote range.
    rf::Frequency f_max = 500 * MHZ_DIV;
    uint8_t preset_index = 0;  // Manual
    uint8_t filter_index = 0;  // OFF
    uint8_t trigger = 32;
    uint8_t mode = LOOKING_GLASS_FASTSCAN;
    uint8_t live_frequency_view = 0;         // Spectrum
    uint8_t live_frequency_integrate = 3;    // Default (3 * old value + new_value) / 4
    uint8_t iq_phase_calibration_value{15};  // initial default RX IQ phase calibration value , used for both max2837 & max2839
    int32_t beep_squelch = 20;               // range from -100 to +20, >=20 disabled
    bool beep_enabled = false;               // activate on bip button click
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
            {"iq_phase_calibration"sv, &iq_phase_calibration_value},  // we are saving and restoring that CAL from Settings.
            {"beep_squelch"sv, &beep_squelch},
            {"beep_enabled"sv, &beep_enabled},
        }};

    struct preset_entry {
        rf::Frequency min{};
        rf::Frequency max{};
        std::string label{};
    };

    int32_t map(int32_t value, int32_t fromLow, int32_t fromHigh, int32_t toLow, int32_t toHigh);
    std::vector<preset_entry> presets_db{};
    void manage_beep_audio();
    void update_display_beep();
    void update_min(int32_t v);
    void update_max(int32_t v);
    void update_range_field();
    void get_max_power(const ChannelSpectrum& spectrum, uint8_t bin, uint8_t& max_power);
    rf::Frequency get_freq_from_bin_pos(uint8_t pos);
    void on_marker_change();
    void retune();
    bool process_bins(uint8_t* powerlevel);
    void on_channel_spectrum(const ChannelSpectrum& spectrum);
    void do_timers();
    int64_t next_mult_of(int64_t num, int64_t multiplier);
    void adjust_range(int64_t* f_min, int64_t* f_max, int64_t width);
    void on_range_changed();
    void reset_live_view();
    void add_spectrum_pixel(uint8_t power);
    void plot_marker(uint8_t pos);
    void load_presets();
    void populate_presets();
    void launch_audio(rf::Frequency center_freq);

    rf::Frequency search_span{0};
    rf::Frequency f_center{0};
    rf::Frequency f_center_ini{0};

    rf::Frequency marker{0};
    uint8_t marker_pixel_index{0};
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

    std::array<Color, SCREEN_W> spectrum_row{};
    std::array<uint8_t, SCREEN_W> spectrum_data{};
    ChannelSpectrumFIFO* fifo{};

    int32_t steps = 1;
    bool locked_range = false;

    uint8_t range_max_power = 0;
    uint8_t range_max_power_counter = 0;
    uint8_t max_power = 0;
    rf::Frequency max_freq_hold = 0;
    rf::Frequency last_max_freq = 0;
    int16_t max_freq_power = -1000;
    uint8_t bin_length = SCREEN_W;
    uint8_t offset = 0;
    uint8_t ignore_dc = 0;

    Labels labels{
        {{0, 0 * 16}, "MIN:     MAX:     LNA   VGA  ", Theme::getInstance()->fg_light->foreground},
        {{0, 1 * 16}, "RANGE:       FILTER:     AMP:", Theme::getInstance()->fg_light->foreground},
        {{0, 2 * 16}, "P:", Theme::getInstance()->fg_light->foreground},
        {{0, 3 * 16}, "MARKER:          MHz RXIQCAL", Theme::getInstance()->fg_light->foreground},
        //{{0, 4 * 16}, "RES:    STEPS:", Theme::getInstance()->fg_light->foreground}};
        {{0, 4 * 16}, "RES:     VOL:", Theme::getInstance()->fg_light->foreground}};

    NumberField field_frequency_min{
        {4 * 8, 0 * 16},
        4,
        {0, 7199},
        1,  // number of steps by encoder delta
        ' '};

    NumberField field_frequency_max{
        {13 * 8, 0 * 16},
        4,
        {1, 7200},
        1,  // number of steps by encoder delta
        ' '};

    LNAGainField field_lna{
        {21 * 8, 0 * 16}};

    VGAGainField field_vga{
        {27 * 8, 0 * 16}};

    TextField field_range{
        {6 * 8, 1 * 16, 6 * 8, 16},
        ""};

    OptionsField filter_config{
        {20 * 8, 1 * 16},
        4,
        {
            {"OFF ", 0},
            {"MID ", 118},  // 85 + 25 (110) + a bit more to kill all blue
            {"HIGH", 202},  // 168 + 25 (193)
        }};

    RFAmpField field_rf_amp{
        {28 * 8, 1 * 16}};

    OptionsField range_presets{
        {2 * 8, 2 * 16},
        20,
        {}};

    ButtonWithEncoder button_beep_squelch{
        {240 - 8 * 8, 2 * 16 + 4, 8 * 8, 1 * 8},
        ""};

    TextField field_marker{
        {7 * 8, 3 * 16, 9 * 8, 16},
        ""};

    NumberField field_rx_iq_phase_cal{
        {28 * 8, 3 * 16},
        2,
        {0, 63},  // 5 or 6 bits IQ CAL phase adjustment (range updated later)
        1,
        ' ',
    };

    NumberField field_trigger{
        {4 * 8, 4 * 16},
        3,
        {2, 128},
        2,
        ' '};

    AudioVolumeField field_volume{
        {13 * 8, 4 * 16}};

    /*OptionsField steps_config{
        {13 * 8, 4 * 16},
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
        {17 * 8, 4 * 16},
        2,
        {
            {"F-", LOOKING_GLASS_FASTSCAN},
            {"S-", LOOKING_GLASS_SLOWSCAN},
        }};

    OptionsField view_config{
        {19 * 8, 4 * 16},
        7,
        {
            {"SPCTR-V", 0},
            {"LEVEL-V", 1},
            {"PEAK-V", 2},
        }};

    OptionsField level_integration{
        {27 * 8, 4 * 16},
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
        {SCREEN_W - 4 * 8, 5 * 16, 4 * 8, 16},
        "JMP"};

    Button button_rst{
        {SCREEN_W - 9 * 8, 5 * 16, 4 * 8, 16},
        "RST"};

    Text freq_stats{
        {0 * 8, 5 * 16, SCREEN_W - 10 * 8, 8},
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
};
}  // namespace ui
#endif
