/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2018 Furrtek
 * Copyright (C) 2023 Mark Thompson
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

#include "audio.hpp"
#include "analog_audio_app.hpp"
#include "baseband_api.hpp"
#include "file.hpp"
#include "freqman.hpp"
#include "freqman_db.hpp"
#include "portapack_persistent_memory.hpp"
#include "radio_state.hpp"
#include "receiver_model.hpp"
#include "string_format.hpp"
#include "ui.hpp"
#include "ui_mictx.hpp"
#include "ui_receiver.hpp"

#define SCANNER_SLEEP_MS 50  // ms that Scanner Thread sleeps per loop
#define STATISTICS_UPDATES_PER_SEC 10
#define MAX_FREQ_LOCK 10  // # of 50ms cycles scanner locks into freq when signal detected, to verify signal is not spurious

namespace ui {

// TODO: There is too much duplicated data in these classes.
// ScannerThread should just use more from the View.
// Or perhaps ScannerThread should just be in the View.

// TODO: Too many functions mix work and UI update.
// Consolidate UI fixup to a single function.

// TODO: Just use freqman_entry.
struct scanner_entry_t {
    rf::Frequency freq;
    std::string description;
};

struct scanner_range_t {
    int64_t min;
    int64_t max;
};

class ScannerThread {
   public:
    ScannerThread(std::vector<rf::Frequency> frequency_list);
    ScannerThread(const scanner_range_t& frequency_range, size_t def_step_hz);
    ~ScannerThread();

    void set_scanning(const bool v);
    bool is_scanning();

    void set_freq_lock(const uint32_t v);
    uint32_t is_freq_lock();

    void set_freq_del(const rf::Frequency v);
    void set_index_stepper(const int32_t v);
    void set_scanning_direction(bool fwd);

    void stop();

    ScannerThread(const ScannerThread&) = delete;
    ScannerThread(ScannerThread&&) = delete;
    ScannerThread& operator=(const ScannerThread&) = delete;
    ScannerThread& operator=(ScannerThread&&) = delete;

   private:
    std::vector<rf::Frequency> frequency_list_{};
    scanner_range_t frequency_range_{0, 0};
    size_t def_step_hz_{0};
    Thread* thread{nullptr};

    bool _scanning{true};
    bool _manual_search{false};
    uint32_t _freq_lock{0};
    rf::Frequency _freq_del{0};
    uint32_t _freq_idx{0};
    int32_t _stepper{1};
    int32_t _index_stepper{0};
    static msg_t static_fn(void* arg);
    void run();
    void create_thread();
};

class ScannerView : public View {
   public:
    ScannerView(NavigationView& nav);
    ~ScannerView();

    void focus() override;

    std::string title() const override { return "Scanner"; };

   private:
    static constexpr const char* default_freqman_file = "SCANNER";

    RxRadioState radio_state_{};

    // Settings
    uint32_t browse_wait{5};
    uint32_t lock_wait{2};
    int32_t squelch{-30};
    scanner_range_t frequency_range{0, MAX_UFREQ};
    std::string freqman_file{default_freqman_file};
    app_settings::SettingsManager settings_{
        "rx_scanner"sv,
        app_settings::Mode::RX,
        {
            {"browse_wait"sv, &browse_wait},
            {"lock_wait"sv, &lock_wait},
            {"scanner_squelch"sv, &squelch},
            {"range_min"sv, &frequency_range.min},
            {"range_max"sv, &frequency_range.max},
            {"file"sv, &freqman_file},
        }};

    NavigationView& nav_;

    void start_scan_thread();
    void restart_scan();
    void change_mode(freqman_index_t mod_type);
    void show_max_index();
    void scan_pause();
    void scan_resume();
    void user_resume();
    void frequency_file_load(const std::filesystem::path& path);
    void bigdisplay_update(int32_t);
    void update_squelch_while_paused(int32_t max_db);
    void on_statistics_update(const ChannelStatistics& statistics);
    void handle_retune(int64_t freq, uint32_t freq_idx);
    void handle_encoder(EncoderEvent delta);
    std::string loaded_filename() const;

    uint32_t browse_timer{0};
    uint32_t lock_timer{0};
    uint32_t color_timer{0};
    int32_t bigdisplay_current_color{-2};
    rf::Frequency bigdisplay_current_frequency{0};

    std::vector<scanner_entry_t> entries{};
    uint32_t current_index{0};
    rf::Frequency current_frequency{0};

    bool userpause{false};
    bool manual_search{false};
    bool fwd{true};  // to preserve direction setting even if scan_thread restarted

    enum bigdisplay_color_type {
        BDC_GREY,
        BDC_YELLOW,
        BDC_GREEN,
        BDC_RED
    };

    Labels labels{
        {{0 * 8, 0 * 16}, "LNA:   VGA:   AMP:  VOL:", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 1 * 16}, "BW:       SQ:    Wsa:   Wsl:", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 10 * 16}, "SRCH START  SEARCH END  SWITCH", Theme::getInstance()->fg_light->foreground},

        {{0 * 8, (26 * 8) + 4}, "MODE:", Theme::getInstance()->fg_light->foreground},
        {{11 * 8, (26 * 8) + 4}, "STEP:", Theme::getInstance()->fg_light->foreground},
    };

    LNAGainField field_lna{
        {4 * 8, 0 * 16}};

    VGAGainField field_vga{
        {11 * 8, 0 * 16}};

    RFAmpField field_rf_amp{
        {18 * 8, 0 * 16}};

    AudioVolumeField field_volume{
        {24 * 8, 0 * 16}};

    OptionsField field_bw{
        {3 * 8, 1 * 16},
        6,
        {}};

    NumberField field_squelch{
        {13 * 8, 1 * 16},
        3,
        {-90, 20},
        1,
        ' ',
    };

    NumberField field_browse_wait{
        // Signal-Active wait timer - time to wait before moving on even when signal locked
        {21 * 8, 1 * 16},
        2,
        {0, 99},
        1,
        ' ',
    };

    NumberField field_lock_wait{
        // Signal-Lost wait timer - time to wait before moving on after losing signal lock
        {28 * 8, 1 * 16},
        2,
        {0, 99},
        1,
        ' ',
    };

    RSSI rssi{
        {0 * 16, 2 * 16, 15 * 16, 8},
    };

    TextField field_current_index{
        {0, 3 * 16, 3 * 8, 16},
        {},
    };

    Text text_max_index{
        {4 * 8, 3 * 16, 18 * 8, 16},
    };

    Text text_current_desc{
        {0, 4 * 16, 240 - 6 * 8, 16},
    };

    BigFrequency big_display{
        {4, 6 * 16, 28 * 8, 52},
        0};

    Button button_manual_start{
        {0 * 8, 11 * 16, 11 * 8, 28},
        ""};

    Button button_manual_end{
        {12 * 8, 11 * 16, 11 * 8, 28},
        ""};

    Button button_manual_search{
        {24 * 8, 11 * 16, 6 * 8, 28},
        ""};

    OptionsField field_mode{
        {5 * 8, (26 * 8) + 4},
        6,
        {}  // Text strings get filled by freqman_set_modulation_option()
    };

    OptionsField field_step{
        {17 * 8, (26 * 8) + 4},
        12,
        {}  // Text strings get filled by freqman_set_step_option()
    };

    ButtonWithEncoder button_pause{
        {0, (15 * 16) - 4, 72, 28},
        "<PAUSE>"};

    Button button_dir{
        {0, (35 * 8) - 4, 72, 28},
        "REVERSE"};

    Button button_audio_app{
        {84, (15 * 16) - 4, 72, 28},
        "AUDIO"};

    Button button_mic_app{
        {84, (35 * 8) - 4, 72, 28},
        "MIC TX"};

    Button button_add{
        {168, (15 * 16) - 4, 72, 28},
        "ADD FQ"};

    Button button_load{
        {24 * 8, 3 * 16 - 10, 6 * 8, 22},
        "LOAD"};

    Button button_clear{
        {24 * 8, 4 * 16, 6 * 8, 22},
        "MCLR"};

    Button button_remove{
        {168, (35 * 8) - 4, 72, 28},
        "DEL FQ"};

    std::unique_ptr<ScannerThread> scan_thread{};

    MessageHandlerRegistration message_handler_retune{
        Message::ID::Retune,
        [this](const Message* const p) {
            const auto message = *reinterpret_cast<const RetuneMessage*>(p);
            this->handle_retune(message.freq, message.range);
        }};

    MessageHandlerRegistration message_handler_stats{
        Message::ID::ChannelStatistics,
        [this](const Message* const p) {
            this->on_statistics_update(static_cast<const ChannelStatisticsMessage*>(p)->statistics);
        }};
};

} /* namespace ui */
