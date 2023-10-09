/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2018 Furrtek
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

#ifndef _UI_RECON
#define _UI_RECON

#include "ui.hpp"
#include "receiver_model.hpp"
#include "ui_receiver.hpp"
#include "ui_styles.hpp"
#include "freqman.hpp"
#include "analog_audio_app.hpp"
#include "audio.hpp"
#include "ui_mictx.hpp"
#include "ui_level.hpp"
#include "ui_looking_glass_app.hpp"
#include "portapack_persistent_memory.hpp"
#include "baseband_api.hpp"
#include "string_format.hpp"
#include "file.hpp"
#include "app_settings.hpp"
#include "radio_state.hpp"
#include "ui_recon_settings.hpp"

namespace ui {

#define RECON_CFG_FILE "SETTINGS/recon.cfg"

enum class recon_mode : uint8_t {
    Recon,
    Scanner,
    Manual
};

class ReconView : public View {
   public:
    ReconView(NavigationView& nav);
    ~ReconView();

    void focus() override;

    std::string title() const override { return "Recon"; };

    // void set_parent_rect(const Rect new_parent_rect) override;

   private:
    NavigationView& nav_;

    RxRadioState radio_state_{};
    app_settings::SettingsManager settings_{
        "rx_recon"sv, app_settings::Mode::RX};

    void check_update_ranges_from_current();
    void set_loop_config(bool v);
    void clear_freqlist_for_ui_action();
    void reset_indexes();
    void update_description();
    void audio_output_start();
    bool check_sd_card();
    size_t change_mode(freqman_index_t mod_type);
    void show_max(bool refresh_display = false);
    void recon_pause();
    void recon_resume();
    void frequency_file_load(bool stop_all_before = false);
    void on_statistics_update(const ChannelStatistics& statistics);
    void on_index_delta(int32_t v);
    void on_stepper_delta(int32_t v);
    void colorize_waits();
    void recon_redraw();
    void handle_retune();
    void handle_coded_squelch(const uint32_t value);
    void handle_remove_current_item();
    void load_persisted_settings();
    bool recon_save_freq(const std::filesystem::path& path, size_t index, bool warn_if_exists);
    // placeholder for possible void recon_start_recording();
    void recon_stop_recording();

    // Returns true if 'current_index' is in bounds of frequency_list.
    bool current_is_valid();
    freqman_entry& current_entry();

    // TODO: consolidate mode bools and use recon_mode.
    recon_mode mode() const {
        if (manual_mode) return recon_mode::Manual;
        if (scanner_mode) return recon_mode::Scanner;
        return recon_mode::Recon;
    }

    jammer::jammer_range_t frequency_range{false, 0, MAX_UFREQ};  // perfect for manual recon task too...
    int32_t squelch{RECON_DEF_SQUELCH};
    int32_t db{0};
    int32_t timer{0};
    int32_t wait{RECON_DEF_WAIT_DURATION};  // in msec. if > 0 wait duration after a lock, if < 0 duration is set to 'wait' unless there is no more activity
    freqman_db frequency_list{};
    int32_t current_index{0};
    bool continuous_lock{false};
    bool freqlist_cleared_for_ui_action{false};  // flag positioned by ui widgets to manage freqlist unload/load
    std::string input_file{"RECON"};
    std::string output_file{"RECON_RESULTS"};
    std::string description{"...no description..."};
    bool autosave{true};
    bool autostart{true};
    bool continuous{true};
    bool filedelete{true};
    bool load_freqs{true};
    bool load_ranges{true};
    bool load_hamradios{true};
    bool update_ranges{true};
    bool fwd{true};
    bool recon{true};
    bool user_pause{false};
    bool auto_record_locked{false};
    bool is_recording{false};
    uint32_t recon_lock_nb_match{RECON_DEF_NB_MATCH};
    uint32_t recon_lock_duration{RECON_MIN_LOCK_DURATION};
    uint32_t recon_match_mode{RECON_MATCH_CONTINUOUS};
    bool scanner_mode{false};
    bool manual_mode{false};
    bool sd_card_mounted{false};
    int32_t stepper{0};
    int32_t index_stepper{0};
    int64_t freq{0};
    uint32_t step{0};
    freqman_index_t def_step{0};
    freqman_entry last_entry{};
    bool entry_has_changed{false};
    uint32_t freq_lock{0};
    int64_t minfreq{0};
    int64_t maxfreq{0};
    bool has_looped{false};
    int8_t status{-1};  // 0 recon , 1 locking , 2 locked
    int32_t last_timer{-1};
    int8_t last_db{-127};
    uint16_t last_nb_match{999};
    uint16_t last_freq_lock{999};
    size_t last_list_size{0};
    int8_t last_rssi_min{-127};
    int8_t last_rssi_med{-127};
    int8_t last_rssi_max{-127};
    int32_t last_index{-1};
    int64_t last_freq{0};
    std::string freq_file_path{};
    systime_t chrono_start{};
    systime_t chrono_end{};

    // Persisted settings.
    SettingsStore ui_settings{
        "recon"sv,
        {
            {"input_file"sv, &input_file},
            {"output_file"sv, &output_file},
            {"lock_duration"sv, &recon_lock_duration},
            {"lock_nb_match"sv, &recon_lock_nb_match},
            {"squelch_level"sv, &squelch},
            {"match_mode"sv, &recon_match_mode},
            {"match_wait"sv, &wait},
            {"range_min"sv, &frequency_range.min},
            {"range_max"sv, &frequency_range.max},
        }};

    std::unique_ptr<RecordView> record_view{};

    Labels labels{
        {{0 * 8, 0 * 16}, "LNA:   VGA:   AMP:  VOL:     ", Color::light_grey()},
        {{3 * 8, 8 * 16}, "START       END", Color::light_grey()},
        {{0 * 8, (22 * 8)}, "                S:          ", Color::light_grey()},
        {{0 * 8, (24 * 8) + 4}, "NBLCKS:x      W,L:      ,     ", Color::light_grey()},
        {{0 * 8, (26 * 8) + 4}, "MODE:     ,      SQUELCH:    ", Color::light_grey()}};

    LNAGainField field_lna{
        {4 * 8, 0 * 16}};

    VGAGainField field_vga{
        {11 * 8, 0 * 16}};

    RFAmpField field_rf_amp{
        {18 * 8, 0 * 16}};

    AudioVolumeField field_volume{
        {24 * 8, 0 * 16}};

    Text file_name{
        // show file used
        {0, 1 * 16, SCREEN_W, 16},
    };

    Text desc_cycle{
        {0, 2 * 16, SCREEN_W, 16},
    };

    RSSI rssi{
        {0 * 16, 3 * 16 + 2, SCREEN_W - 8 * 8 + 4, 12},
    };

    ButtonWithEncoder text_cycle{
        {0, 4 * 16, 4 * 8, 16},
        ""};

    // "/XXX -XXX db" =>  12 chars max
    Text text_max{
        {4 * 8, 4 * 16, 12 * 8, 16},
    };

    // "XX/XX" =>  5 chars max
    Text text_nb_locks{
        {16 * 8, 4 * 16, 5 * 8, 16},
    };

    Text big_display{
        // Show frequency in text mode
        {0, 5 * 16, 21 * 8, 16},
    };

    Text freq_stats{
        // Show frequency stats in text mode
        {0, 6 * 16, 21 * 8, 16},
    };

    // TIMER: 9999
    Text text_timer{
        // Show frequency stats in text mode
        {0, 7 * 16, 11 * 8, 16},
    };

    // T: Senn. 32.000k
    Text text_ctcss{
        {14 * 8, 7 * 16, 8 * 8, 1 * 8},
        ""};

    // Button can be RECON or SCANNER
    Button button_scanner_mode{
        {SCREEN_W - 7 * 8, 3 * 16, 7 * 8, 28},
        "RECON"};

    Button button_loop_config{
        {SCREEN_W - 7 * 8, 5 * 16, 7 * 8, 28},
        "[LOOP]"};

    Button button_config{
        {SCREEN_W - 7 * 8, 7 * 16, 7 * 8, 28},
        "CONFIG"};

    ButtonWithEncoder button_manual_start{
        {0 * 8, 9 * 16, 11 * 8, 28},
        ""};

    ButtonWithEncoder button_manual_end{
        {12 * 8 - 6, 9 * 16, 11 * 8, 28},
        ""};

    OptionsField field_recon_match_mode{
        {0 * 8, 11 * 16},
        16,  // CONTINUOUS MATCH MODE / SPARSE TIMED MATCH MODE
        {
            {"MATCH:CONTINOUS", 0},
            {"MATCH:SPARSE", 1}}};

    OptionsField step_mode{
        {18 * 8, 11 * 16},
        12,
        {}};

    Button button_manual_recon{
        {23 * 8, 9 * 16, 7 * 8, 28},
        "SEARCH"};

    NumberField field_nblocks{
        {8 * 8, 24 * 8 + 4},
        2,
        {1, 99},
        1,
        ' ',
    };

    NumberField field_wait{
        {19 * 8, 24 * 8 + 4},
        5,
        {-RECON_MAX_LOCK_DURATION, RECON_MAX_LOCK_DURATION},
        STATS_UPDATE_INTERVAL,
        ' ',
    };

    NumberField field_lock_wait{
        {25 * 8, 24 * 8 + 4},
        4,
        {STATS_UPDATE_INTERVAL, RECON_MAX_LOCK_DURATION},
        STATS_UPDATE_INTERVAL,
        ' ',
    };

    OptionsField field_mode{
        {6 * 8, (26 * 8) + 4},
        4,
        {}};

    OptionsField field_bw{
        {11 * 8, (26 * 8) + 4},
        6,
        {}};

    NumberField field_squelch{
        {26 * 8, (26 * 8) + 4},
        3,
        {-90, 20},
        1,
        ' ',
    };

    ButtonWithEncoder button_pause{
        {0, (15 * 16) - 4, 72, 28},
        "PAUSE"};

    Button button_audio_app{
        {84, (15 * 16) - 4, 72, 28},
        "AUDIO"};

    ButtonWithEncoder button_add{
        {168, (15 * 16) - 4, 72, 28},
        "<STORE>"};

    Button button_dir{
        {0, (35 * 8) - 4, 34, 28},
        "FW>"};

    Button button_restart{
        {38, (35 * 8) - 4, 34, 28},
        "RST"};

    Button button_mic_app{
        {84, (35 * 8) - 4, 72, 28},
        "MIC TX"};

    ButtonWithEncoder button_remove{
        {168, (35 * 8) - 4, 72, 28},
        "<REMOVE>"};

    MessageHandlerRegistration message_handler_coded_squelch{
        Message::ID::CodedSquelch,
        [this](const Message* const p) {
            const auto message = *reinterpret_cast<const CodedSquelchMessage*>(p);
            this->handle_coded_squelch(message.value);
        }};

    MessageHandlerRegistration message_handler_stats{
        Message::ID::ChannelStatistics,
        [this](const Message* const p) {
            this->on_statistics_update(static_cast<const ChannelStatisticsMessage*>(p)->statistics);
        }};
};

} /* namespace ui */

#endif
