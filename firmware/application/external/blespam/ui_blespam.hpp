/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2017 Furrtek
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

#ifndef __UI_BLESPAM_H__
#define __UI_BLESPAM_H__

#include "ui.hpp"
#include "ui_navigation.hpp"
#include "ui_transmitter.hpp"
#include "ui_freq_field.hpp"
#include "ui_record_view.hpp"
#include "app_settings.hpp"
#include "radio_state.hpp"
#include "log_file.hpp"
#include "utility.hpp"

using namespace ui;

namespace ui::external_app::blespam {

#include "fastpair.hpp"
#include "continuity.hpp"
#include "easysetup.hpp"

enum ATK_TYPE {
    ATK_ANDROID,
    ATK_IOS,
    ATK_IOS_CRASH,
    ATK_WINDOWS,
    ATK_SAMSUNG
};

class BLESpamView : public View {
   public:
    BLESpamView(NavigationView& nav);
    ~BLESpamView();

    void focus() override;

    std::string title() const override {
        return "BLESpam";
    };

   private:
    NavigationView& nav_;
    TxRadioState radio_state_{
        2'402'000'000 /* frequency */,
        4'000'000 /* bandwidth */,
        4'000'000 /* sampling rate */
    };
    TxFrequencyField field_frequency{
        {0 * 8, 0 * 16},
        nav_};
    TransmitterView2 tx_view{
        {11 * 8, 0 * 16},
        /*short_ui*/ true};
    app_settings::SettingsManager settings_{
        "tx_blespam", app_settings::Mode::TX};

    Button button_startstop{
        {0, 2 * 16, 96, 24},
        "Start"};
    Checkbox chk_randdev{{100, 16}, 10, "Rnd device", true};

    Console console{
        {0, 4 * 16, 240, 224}};

    OptionsField options_atkmode{
        {0 * 8, 2 * 8},
        10,
        {{"Android", 0},
         {"iOs", 1},
         {"iOs crash", 2},
         {"Windows", 3},
         {"Samsung", 4}}};

    void on_tx_progress(const bool done);
    bool is_running{false};

    uint8_t counter = 0;  // for packet

    int16_t timer_count{0};
    int16_t timer_period{2};
    ATK_TYPE attackType = ATK_ANDROID;

    bool randomMac{true};
    bool randomDev{true};

    uint8_t channel_number = 37;
    char mac[13] = "010203040507";
    char advertisementData[63] = {"03032CFE06162CFED5A59E020AB4\0"};
    PKT_TYPE pduType = {PKT_TYPE_DISCOVERY};

    void start();
    void stop();
    void reset();
    void createFastPairPacket();
    void createIosPacket(bool crash);
    void createSamsungPacket();
    void createWindowsPacket();
    void changePacket(bool forced);
    void on_timer();
    uint64_t get_freq_by_channel_number(uint8_t channel_number);
    void randomizeMac();
    void randomChn();

    void furi_hal_random_fill_buf(uint8_t* buf, uint32_t len);

    MessageHandlerRegistration message_handler_frame_sync{
        Message::ID::DisplayFrameSync,
        [this](const Message* const) {
            this->on_timer();
        }};
};

}  // namespace ui::external_app::blespam

#endif /*__UI_BLESPAM_H__*/
