/*
 * Copyright (C) 2016 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __REPLAY_APP_HPP__
#define __REPLAY_APP_HPP__

#include "app_settings.hpp"
#include "radio_state.hpp"
#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "ui_freq_field.hpp"
#include "replay_thread.hpp"
#include "ui_spectrum.hpp"
#include "ui_transmitter.hpp"

#include <string>
#include <memory>

namespace ui {

class ReplayAppView : public View {
   public:
    ReplayAppView(NavigationView& nav);
    ~ReplayAppView();

    void on_hide() override;
    void set_parent_rect(const Rect new_parent_rect) override;
    void focus() override;

    std::string title() const override { return "Replay"; };

   private:
    NavigationView& nav_;
    TxRadioState radio_state_{};
    app_settings::SettingsManager settings_{
        "tx_replay", app_settings::Mode::TX};

    static constexpr ui::Dim header_height = 3 * 16;

    uint32_t sample_rate = 0;
    int32_t tx_gain{47};
    bool rf_amp{true};  // aux private var to store temporal, Replay App rf_amp user selection.
    static constexpr uint32_t baseband_bandwidth = 2500000;
    const size_t read_size{16384};
    const size_t buffer_count{3};

    void on_file_changed(const std::filesystem::path& new_file_path);
    void on_tx_progress(const uint32_t progress);

    void toggle();
    void start();
    void stop(const bool do_loop);
    bool is_active() const;
    void set_ready();
    void handle_replay_thread_done(const uint32_t return_code);
    void file_error();

    std::filesystem::path file_path{};
    std::unique_ptr<ReplayThread> replay_thread{};
    bool ready_signal{false};

    Button button_open{
        {0 * 8, 0 * 16, 10 * 8, 2 * 16},
        "Open file"};

    Text text_filename{
        {11 * 8, 0 * 16, 12 * 8, 16},
        "-"};
    Text text_sample_rate{
        {24 * 8, 0 * 16, 6 * 8, 16},
        "-"};

    Text text_duration{
        {11 * 8, 1 * 16, 6 * 8, 16},
        "-"};
    ProgressBar progressbar{
        {18 * 8, 1 * 16, 12 * 8, 16}};

    TxFrequencyField field_frequency{
        {0 * 8, 2 * 16},
        nav_};

    TransmitterView2 tx_view{
        {11 * 8, 2 * 16},
        /*short_ui*/ true};

    Checkbox check_loop{
        {21 * 8, 2 * 16},
        4,
        "Loop",
        true};
    ImageButton button_play{
        {28 * 8, 2 * 16, 2 * 8, 1 * 16},
        &bitmap_play,
        Color::green(),
        Color::black()};

    spectrum::WaterfallView waterfall{};

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

} /* namespace ui */

#endif /*__REPLAY_APP_HPP__*/
