/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2017 Furrtek
 * Copyright (C) 2023 TJ Baginski
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

#ifndef __BLE_TX_APP_H__
#define __BLE_TX_APP_H__

#include "ui.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "ui_transmitter.hpp"
#include "ui_freq_field.hpp"
#include "ui_record_view.hpp"
#include "app_settings.hpp"
#include "radio_state.hpp"
#include "log_file.hpp"
#include "utility.hpp"

#include "recent_entries.hpp"

class BLELoggerTx {
   public:
    Optional<File::Error> append(const std::string& filename) {
        return log_file.append(filename);
    }

    void log_raw_data(const std::string& data);

   private:
    LogFile log_file{};
};

namespace ui {

class BLETxView : public View {
   public:
    BLETxView(NavigationView& nav);
    ~BLETxView();

    void set_parent_rect(const Rect new_parent_rect) override;
    void paint(Painter&) override{};

    void focus() override;

    bool is_active() const;
    void toggle();
    void start();
    void stop(const bool do_loop);
    void handle_replay_thread_done(const uint32_t return_code);

    std::string title() const override { return "BLE TX"; };

   private:
    void on_data(uint32_t value, bool is_data);
    void on_tx_progress(const uint32_t progress, const bool done);

    NavigationView& nav_;
    TxRadioState radio_state_{
        2'402'000'000 /* frequency */,
        4'000'000 /* bandwidth */,
        4'000'000 /* sampling rate */
    };
    app_settings::SettingsManager settings_{
        "ble_tx_app", app_settings::Mode::TX};

    uint8_t console_color{0};
    uint32_t prev_value{0};
    uint8_t channel_number = 37;
    bool is_running = false;

    static constexpr auto header_height = 12 + 3 * 16;

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

    Console console{
    {0, 3 * 16, 240, 240}};

    std::string str_log{""};
    bool logging{true};
    bool logging_done{false};

    std::unique_ptr<BLELoggerTx> logger{};

    MessageHandlerRegistration message_handler_packet{
        Message::ID::AFSKData,
        [this](Message* const p) {
            const auto message = static_cast<const AFSKDataMessage*>(p);
            this->on_data(message->value, message->is_data);
        }};

    MessageHandlerRegistration message_handler_tx_progress{
    Message::ID::TXProgress,
    [this](const Message* const p) {
        const auto message = *reinterpret_cast<const TXProgressMessage*>(p);
        this->on_tx_progress(message.progress, message.done);
    }};
};

} /* namespace ui */

#endif /*__UI_AFSK_RX_H__*/
