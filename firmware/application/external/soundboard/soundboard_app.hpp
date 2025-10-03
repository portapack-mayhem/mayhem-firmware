/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
 * Copyright (C) 2024 Mark Thompson
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

#ifndef __UI_SOUNDBOARD_H__
#define __UI_SOUNDBOARD_H__

#include "ui_widget.hpp"
#include "ui_transmitter.hpp"
#include "replay_thread.hpp"
#include "baseband_api.hpp"
#include "lfsr_random.hpp"
#include "io_wave.hpp"
#include "app_settings.hpp"
#include "radio_state.hpp"

namespace ui::external_app::soundboard {

class SoundBoardView : public View {
   public:
    SoundBoardView(NavigationView& nav);
    ~SoundBoardView();

    SoundBoardView(const SoundBoardView&) = delete;
    SoundBoardView(SoundBoardView&&) = delete;
    SoundBoardView& operator=(const SoundBoardView&) = delete;
    SoundBoardView& operator=(SoundBoardView&&) = delete;

    void focus() override;

    std::string title() const override { return "Soundbrd TX"; };

   private:
    TxRadioState radio_state_{
        0 /* frequency */,
        1750000 /* bandwidth */,
        1536000 /* sampling rate */
    };
    app_settings::SettingsManager settings_{
        "tx_soundboard", app_settings::Mode::TX};

    NavigationView& nav_;

    enum tx_modes {
        NORMAL = 0,
        RANDOM
    };

    tx_modes tx_mode = NORMAL;

    uint32_t playing_id{};
    uint32_t page = 1;
    uint32_t c_page = 1;
    uint32_t tone_key_index = 1;
    uint8_t bits_per_sample = 1;

    std::vector<std::filesystem::path> file_list{};

    const size_t read_size{2048};  // Less ?
    const size_t buffer_count{3};
    std::unique_ptr<ReplayThread> replay_thread{};
    bool ready_signal{false};
    lfsr_word_t lfsr_v = 1;

    // void show_infos();
    void start_tx(const uint32_t id);
    // void on_ctcss_changed(uint32_t v);
    void stop();
    bool is_active() const;
    void set_ready();
    void handle_replay_thread_done(const uint32_t return_code);
    void file_error();
    void on_tx_progress(const uint32_t progress);
    void refresh_list();
    void on_select_entry();
    void update_config();

    Labels labels{
        {{24 * 8, UI_POS_Y_BOTTOM(9)}, "Vol:", Theme::getInstance()->fg_light->foreground},
        {{0, UI_POS_Y_BOTTOM(9)}, "Key:", Theme::getInstance()->fg_light->foreground}};

    Button button_next_page{
        {UI_POS_X_RIGHT(4), UI_POS_Y_BOTTOM(8), 10 * 3, 2 * 14},
        "=>"};

    Button button_prev_page{
        {UI_POS_X_RIGHT(8), UI_POS_Y_BOTTOM(8), 10 * 3, 2 * 14},
        "<="};

    Text page_info{
        {0, UI_POS_Y_BOTTOM(6), screen_width, 16}};

    MenuView menu_view{
        {0, 0, screen_width, UI_POS_Y_BOTTOM(9)},
        true};
    Text text_empty{
        {UI_POS_X_CENTER(16), 12 * 8, 16 * 8, 16},
        "Empty directory !",
    };

    /*Text text_title {
                { 6 * 8, 20 * 8 + 4, 15 * 8, 16 }
        };*/

    /*Text text_duration {
                { 22 * 8, 20 * 8 + 4, 6 * 8, 16 }
        };*/

    OptionsField options_tone_key{
        {4 * 8, UI_POS_Y_BOTTOM(9)},
        18,
        {}};

    AudioVolumeField field_volume{
        {screen_width - 2 * 8, UI_POS_Y_BOTTOM(9)}};
    Text text_volume_disabled{
        {screen_width - 2 * 8, UI_POS_Y_BOTTOM(9), 3 * 8, 16},
        "--"};

    Checkbox check_loop{
        {0, UI_POS_Y_BOTTOM(8)},
        4,
        "Loop"};

    Checkbox check_random{
        {10 * 7, UI_POS_Y_BOTTOM(8)},
        6,
        "Random"};

    ProgressBar progressbar{
        {UI_POS_X(0), UI_POS_Y_BOTTOM(4.5) + 2, screen_width, 4}};

    TransmitterView tx_view{
        (int16_t)UI_POS_Y_BOTTOM(4),
        5000,
        12};

    MessageHandlerRegistration message_handler_replay_thread_error{
        Message::ID::ReplayThreadDone,
        [this](const Message* const p) {
            const auto message = *reinterpret_cast<const ReplayThreadDoneMessage*>(p);
            this->handle_replay_thread_done(message.return_code);
        }};

    MessageHandlerRegistration message_handler_fifo_signal{
        Message::ID::RequestSignal,
        [this](const Message* const p) {
            const auto message = static_cast<const RequestSignalMessage*>(p);
            if (message->signal == RequestSignalMessage::Signal::FillRequest) {
                this->set_ready();
            }
        }};

    MessageHandlerRegistration message_handler_tx_progress{
        Message::ID::TXProgress,
        [this](const Message* const p) {
            const auto message = *reinterpret_cast<const TXProgressMessage*>(p);
            this->on_tx_progress(message.progress);
        }};
};

} /* namespace ui::external_app::soundboard */

#endif /*__UI_SOUNDBOARD_H__*/
