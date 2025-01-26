/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2017 Furrtek
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

#include "ui.hpp"
#include "ui_navigation.hpp"
#include "io_wave.hpp"
#include "spectrum_color_lut.hpp"
#include "ui_receiver.hpp"
#include "replay_thread.hpp"

namespace ui {

class ViewWavView : public View {
   public:
    ViewWavView(NavigationView& nav);
    ~ViewWavView();

    void focus() override;
    void paint(Painter&) override;

    std::string title() const override { return "WAV viewer"; };

   private:
    app_settings::SettingsManager settings_{
        "wav_viewer", app_settings::Mode::NO_RF};

    NavigationView& nav_;
    static constexpr uint32_t subsampling_factor = 8;

    void update_scale(int32_t new_scale);
    void refresh_waveform();
    void refresh_measurements();
    void on_pos_time_changed();
    void on_pos_sample_changed();
    void load_wav(std::filesystem::path file_path);
    void reset_controls();
    bool is_active();
    void stop();
    void handle_replay_thread_done(const uint32_t return_code);
    void set_ready();
    void file_error();
    void start_playback();
    void on_playback_progress(const uint32_t progress);

    std::filesystem::path wav_file_path{};
    std::unique_ptr<ReplayThread> replay_thread{};
    bool ready_signal{false};
    const size_t read_size{2048};
    const size_t buffer_count{3};
    const uint32_t progress_interval_samples{1536000 / 20};

    std::unique_ptr<WAVFileReader> wav_reader{};

    int16_t waveform_buffer[240]{};
    uint8_t amplitude_buffer[240]{};
    int32_t scale{1};
    uint64_t ns_per_pixel{};
    uint64_t position{};
    bool updating_position{false};
    bool playback_in_progress{false};
    bool waveform_update_needed{false};

    Labels labels{
        {{0 * 8, 0 * 16}, "File:", Theme::getInstance()->fg_light->foreground},
        {{2 * 8, 1 * 16}, "-bit mono", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 2 * 16}, "Title:", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 3 * 16}, "Duration:", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 12 * 16}, "Position:    .   s Scale:", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 13 * 16}, "  Sample:", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 14 * 16}, "Cursor A:", Theme::getInstance()->fg_darkcyan->foreground},
        {{0 * 8, 15 * 16}, "Cursor B:", Theme::getInstance()->fg_magenta->foreground},
        {{0 * 8, 16 * 16}, "Delta:", Theme::getInstance()->fg_light->foreground},
        {{24 * 8, 18 * 16}, "Vol:", Theme::getInstance()->fg_light->foreground}};

    Text text_filename{
        {5 * 8, 0 * 16, 18 * 8, 16},
        ""};
    Text text_samplerate{
        {12 * 8, 1 * 16, 10 * 8, 16},
        ""};
    Text text_title{
        {6 * 8, 2 * 16, 17 * 8, 16},
        ""};
    Text text_duration{
        {9 * 8, 3 * 16, 20 * 8, 16},
        ""};
    Text text_bits_per_sample{
        {0 * 8, 1 * 16, 2 * 8, 16},
        "16"};
    Button button_open{
        {24 * 8, 8, 6 * 8, 2 * 16},
        "Open"};
    ImageButton button_play{
        {24 * 8, 17 * 16, 2 * 8, 1 * 16},
        &bitmap_play,
        Theme::getInstance()->fg_green->foreground,
        Theme::getInstance()->fg_green->background};
    AudioVolumeField field_volume{
        {28 * 8, 18 * 16}};

    Waveform waveform{
        {0, 5 * 16, 240, 64},
        waveform_buffer,
        240,
        0,
        false,
        Theme::getInstance()->bg_darkest->foreground};

    ProgressBar progressbar{
        {0 * 8, 11 * 16, 30 * 8, 4}};

    NumberField field_pos_seconds{
        {9 * 8, 12 * 16},
        4,
        {0, 0},
        1,
        ' ',
        true};
    NumberField field_pos_milliseconds{
        {14 * 8, 12 * 16},
        3,
        {0, 999},
        1,
        '0',
        true};
    NumberField field_pos_samples{
        {9 * 8, 13 * 16},
        9,
        {0, 0},
        1,
        '0',
        true};
    NumberField field_scale{
        {25 * 8, 12 * 16},
        5,
        {1, 1},
        1,
        ' ',
        true};

    NumberField field_cursor_a{
        {9 * 8, 14 * 16},
        3,
        {0, 239},
        1,
        ' ',
        true};

    NumberField field_cursor_b{
        {9 * 8, 15 * 16},
        3,
        {0, 239},
        1,
        ' ',
        true};

    Text text_delta{
        {7 * 8, 16 * 16, 30 * 8, 16},
        "-"};

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
            this->on_playback_progress(message.progress);
        }};
};

} /* namespace ui */
