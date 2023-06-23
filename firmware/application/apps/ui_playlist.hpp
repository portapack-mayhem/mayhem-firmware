/*
 * Copyright (C) 2016 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
 * Copyright (C) 2023 Kyle Reed, zxkmm
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

#define SHORT_UI true
#define NORMAL_UI false

#include "app_settings.hpp"
#include "metadata_file.hpp"
#include "radio_state.hpp"
#include "replay_thread.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "ui_spectrum.hpp"
#include "ui_transmitter.hpp"
#include "ui_widget.hpp"

#include <string>
#include <memory>
#include <vector>

namespace ui {

class PlaylistView : public View {
   public:
    PlaylistView(NavigationView& nav);
    ~PlaylistView();

    // Disable copy to make -Weffc++ happy.
    PlaylistView(const PlaylistView&) = delete;
    PlaylistView& operator=(const PlaylistView&) = delete;

    void set_parent_rect(Rect new_parent_rect) override;
    void on_hide() override;
    void focus() override;

    std::string title() const override { return "Playlist"; };

   private:
    NavigationView& nav_;
    TxRadioState radio_state_{};
    app_settings::SettingsManager settings_{
        "tx_playlist", app_settings::Mode::TX};

    // More header == less spectrum view.
    static constexpr ui::Dim header_height = 4 * 16;
    static constexpr uint32_t baseband_bandwidth = 2500000;

    struct playlist_entry {
        std::filesystem::path capture_path{};
        capture_metadata metadata{};
        uint32_t ms_delay{};
    };

    std::unique_ptr<ReplayThread> replay_thread_{};
    bool ready_signal_{};  // Used to signal the ReplayThread.

    size_t current_track_{0};
    const playlist_entry* current_entry_{};
    size_t current_entry_size_{0};
    std::vector<playlist_entry> playlist_db_{};
    std::filesystem::path playlist_path_{};

    void load_file(const std::filesystem::path& path);
    void on_file_changed(const std::filesystem::path& path);
    void reset();
    void show_file_error(
        const std::filesystem::path& path,
        const std::string& message);

    bool is_active() const;
    bool loop() const;
    bool is_done() const;

    void toggle();
    void start();
    bool next_track();
    void send_current_track();
    void stop();

    void update_ui();

    /* There are called by Message handlers. */
    void on_tx_progress(uint32_t progress);
    void handle_replay_thread_done(uint32_t return_code);

    Button button_open{
        {0 * 8, 0 * 16, 10 * 8, 2 * 16},
        "Open PPL"};

    Text text_filename{
        {11 * 8, 0 * 16, 12 * 8, 16}};

    Text text_sample_rate{
        {24 * 8, 0 * 16, 6 * 8, 16}};

    Text text_duration{
        {11 * 8, 1 * 16, 6 * 8, 16}};

    ProgressBar progressbar_track{
        {18 * 8, 1 * 16, 12 * 8, 8}};

    ProgressBar progressbar_transmit{
        {18 * 8, 3 * 8, 12 * 8, 8}};

    // TODO: TxFrequencyField to set TX freq
    // when metadata file not found.
    Text text_frequency{
        {0 * 8, 2 * 16, 9 * 8, 16}};

    TransmitterView2 tx_view{
        74, 1 * 8, SHORT_UI};

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

    Text text_track{
        {0 * 8, 3 * 16, 30 * 8, 16}};

    spectrum::WaterfallWidget waterfall{};

    MessageHandlerRegistration message_handler_replay_thread_error{
        Message::ID::ReplayThreadDone,
        [this](const Message* p) {
            auto message = *reinterpret_cast<const ReplayThreadDoneMessage*>(p);
            handle_replay_thread_done(message.return_code);
        }};

    MessageHandlerRegistration message_handler_fifo_signal{
        Message::ID::RequestSignal,
        [this](const Message* p) {
            auto message = static_cast<const RequestSignalMessage*>(p);
            if (message->signal == RequestSignalMessage::Signal::FillRequest) {
                ready_signal_ = true;
            }
        }};

    MessageHandlerRegistration message_handler_tx_progress{
        Message::ID::TXProgress,
        [this](const Message* p) {
            auto message = *reinterpret_cast<const TXProgressMessage*>(p);
            on_tx_progress(message.progress);
        }};
};

} /* namespace ui */
