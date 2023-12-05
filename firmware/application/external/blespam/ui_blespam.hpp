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

// Code from https://github.com/Flipper-XFW/Xtreme-Apps/tree/04c3a60093e2c2378e79498b4505aa8072980a42/ble_spam/protocols
// Thanks for the work of the original creators!
// Saying thanks in the main view!

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
        {0, 3 * 16, 96, 24},
        "Start"};
    Checkbox chk_randdev{{100, 16}, 10, "Rnd device", false};

    Console console{
        {0, 70, 240, 220}};

    OptionsField options_atkmode{
        {0 * 8, 2 * 8},
        10,
        {{"Android", 0},
         {"iOs", 1},
         {"iOs crash", 2},
         {"Windows", 3},
         {"Samsung", 4}}};

    bool is_running{false};

    uint8_t counter = 0;         // for packet change
    uint8_t displayCounter = 0;  // for packet display

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

    typedef enum {
        PayloadModeRandom,
        PayloadModeValue,
        PayloadModeBruteforce,
    } PayloadMode;

    typedef enum {
        ContinuityTypeAirDrop = 0x05,
        ContinuityTypeProximityPair = 0x07,
        ContinuityTypeAirplayTarget = 0x09,
        ContinuityTypeHandoff = 0x0C,
        ContinuityTypeTetheringSource = 0x0E,
        ContinuityTypeNearbyAction = 0x0F,
        ContinuityTypeNearbyInfo = 0x10,

        ContinuityTypeCustomCrash,
        ContinuityTypeCOUNT
    } ContinuityType;

    typedef enum {
        ContinuityPpBruteforceModel,
        ContinuityPpBruteforceColor,
    } ContinuityPpBruteforce;

    typedef struct {
        uint8_t value;
        // const char* name;
    } ContinuityColor;

    static const uint8_t pp_prefixes_count;

    static const uint8_t na_actions_count;
    static const uint8_t pp_models_count;
    static const ContinuityColor colors_beats_studio_buds_[];
    static const ContinuityColor colors_beats_fit_pro[];
    static const ContinuityColor colors_beats_studio_pro[];
    static const ContinuityColor colors_beats_studio_3[];
    static const ContinuityColor colors_beats_x[];
    static const ContinuityColor colors_beats_studio_buds[];
    static const ContinuityColor colors_beats_solo_pro[];
    static const ContinuityColor colors_powerbeats_pro[];
    static const ContinuityColor colors_powerbeats_3[];
    static const ContinuityColor colors_beats_solo_3[];
    static const ContinuityColor colors_beats_flex[];
    static const ContinuityColor colors_airpods_max[];
    static const ContinuityColor colors_white[];
    typedef struct {
        uint16_t value;
        // const char* name;
        const ContinuityColor* colors;
        const uint8_t colors_count;
    } contiModels;
    static const contiModels pp_models[];
    typedef struct {
        uint8_t value;
    } contiU8;
    static const contiU8 pp_prefixes[];
    static const contiU8 na_actions[];
};
};  // namespace ui::external_app::blespam

#endif /*__UI_BLESPAM_H__*/
