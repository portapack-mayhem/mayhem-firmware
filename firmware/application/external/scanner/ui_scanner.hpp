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

using namespace ui;

namespace ui::external_app::scanner {

#define SCANNER_SLEEP_MS 50  // ms that Scanner Thread sleeps per loop
#define STATISTICS_UPDATES_PER_SEC 10
#define MAX_FREQ_LOCK 10  // # of 50ms cycles scanner locks into freq when signal detected, to verify signal is not spurious

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
        {{UI_POS_X(0), UI_POS_Y(0)}, "LNA:   VGA:   AMP:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X_RIGHT(6), UI_POS_Y(0)}, "VOL:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), UI_POS_Y(1)}, "BW:       SQ:    Wsa:   Wsl:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X_RIGHT(13), UI_POS_Y(1)}, "Wsa:   Wsl:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), UI_POS_Y(10)}, "SRCH START", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X_CENTER(10) + 8, UI_POS_Y(10)}, "SEARCH END", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X_RIGHT(6), UI_POS_Y(10)}, "SWITCH", Theme::getInstance()->fg_light->foreground},

        {{UI_POS_X(0), (26 * 8) + 4}, "MODE:", Theme::getInstance()->fg_light->foreground},
        {{11 * 8, (26 * 8) + 4}, "STEP:", Theme::getInstance()->fg_light->foreground},
    };

    LNAGainField field_lna{
        {UI_POS_X(4), UI_POS_Y(0)}};

    VGAGainField field_vga{
        {UI_POS_X(11), UI_POS_Y(0)}};

    RFAmpField field_rf_amp{
        {UI_POS_X(18), UI_POS_Y(0)}};

    AudioVolumeField field_volume{
        {UI_POS_X_RIGHT(2), UI_POS_Y(0)}};

    OptionsField field_bw{
        {UI_POS_X(3), UI_POS_Y(1)},
        6,
        {}};

    NumberField field_squelch{
        {UI_POS_X(13), UI_POS_Y(1)},
        3,
        {-90, 20},
        1,
        ' ',
    };

    NumberField field_browse_wait{
        // Signal-Active wait timer - time to wait before moving on even when signal locked
        {UI_POS_X_RIGHT(9), UI_POS_Y(1)},
        2,
        {0, 99},
        1,
        ' ',
    };

    NumberField field_lock_wait{
        // Signal-Lost wait timer - time to wait before moving on after losing signal lock
        {UI_POS_X_RIGHT(2), UI_POS_Y(1)},
        2,
        {0, 99},
        1,
        ' ',
    };

    RSSI rssi{
        {UI_POS_X(0), UI_POS_Y(2), UI_POS_WIDTH_REMAINING(6), 8},
    };

    TextField field_current_index{
        {UI_POS_X(0), UI_POS_Y(3), 3 * 8, 16},
        {},
    };

    Text text_max_index{
        {UI_POS_X(4), UI_POS_Y(3), 18 * 8, 16},
    };

    Text text_current_desc{
        {UI_POS_X(0), UI_POS_Y(4), UI_POS_WIDTH_REMAINING(6), UI_POS_HEIGHT(1)},
    };

    BigFrequency big_display{
        {4, 6 * 16, 28 * 8, 52},
        0};

    Button button_manual_start{
        {UI_POS_X(0), UI_POS_Y(11), UI_POS_WIDTH(10), 28},
        ""};

    Button button_manual_end{
        {UI_POS_X_CENTER(10) + 8, UI_POS_Y(11), UI_POS_WIDTH(10), 28},
        ""};

    Button button_manual_search{
        {UI_POS_X_RIGHT(6), UI_POS_Y(11), UI_POS_WIDTH(6), 28},
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

    Button button_audio_app{
        {UI_POS_X_CENTER(9), (15 * 16) - 4, UI_POS_WIDTH(9), 28},
        "AUDIO"};

    Button button_add{
        {UI_POS_X_RIGHT(9), (15 * 16) - 4, UI_POS_WIDTH(9), 28},
        "ADD FQ"};

    Button button_dir{
        {0, (35 * 8) - 4, 72, 28},
        "REVERSE"};

    Button button_mic_app{
        {UI_POS_X_CENTER(9), (35 * 8) - 4, UI_POS_WIDTH(9), 28},
        "MIC TX"};

    Button button_remove{
        {UI_POS_X_RIGHT(9), (35 * 8) - 4, UI_POS_WIDTH(9), 28},
        "DEL FQ"};

    Button button_load{
        {UI_POS_X_RIGHT(6), UI_POS_Y(3) - 10, UI_POS_WIDTH(6), 22},
        "LOAD"};

    Button button_clear{
        {UI_POS_X_RIGHT(6), UI_POS_Y(4), UI_POS_WIDTH(6), 22},
        "MCLR"};

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
}  // namespace ui::external_app::scanner
