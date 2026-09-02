/*
 * Copyright (C) 2026 Alexey Verhogladov
 *
 * This file is part of PortaPack.
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef __MESH_REGIONS_H__
#define __MESH_REGIONS_H__

#include <cstdint>
#include <array>

// Meshtastic regional frequency plans and LoRa modem presets.
// Source: https://meshtastic.org/docs/overview/radio-settings

namespace meshtastic {

struct LoRaPreset {
    const char* name;
    uint8_t sf;      // Spreading Factor 7-12
    uint32_t bw_hz;  // Bandwidth in Hz
    uint8_t cr;      // Coding Rate denominator (5=4/5 .. 8=4/8)
};

struct RegionConfig {
    const char* name;
    const char* code;
    uint64_t freq_start_hz;  // Band start (freqStart in meshtastic RadioInterface.cpp)
    uint64_t freq_end_hz;    // Band end   (freqEnd)
    int8_t max_power_dbm;
};

// LoRa modem presets (matches Meshtastic ModemPreset enum, verified against
// meshtastic-device firmware RadioInterface.cpp LONG_FAST = SF11/BW250/CR45)
static constexpr LoRaPreset MODEM_PRESETS[] = {
    {"LONG_FAST", 11, 250000, 5},       // Default: SF11, BW250k, CR4/5
    {"LONG_SLOW", 12, 125000, 5},       // SF12, BW125k, CR4/5 - the node reports 4/5, and so does the air
    {"VERY_LONG_SLOW", 12, 125000, 8},  // SF12, BW125k, CR4/8 (legacy alias)
    {"MEDIUM_SLOW", 10, 250000, 5},     // SF10, BW250k, CR4/5
    {"MEDIUM_FAST", 9, 250000, 5},      // SF9,  BW250k, CR4/5
    {"SHORT_SLOW", 8, 250000, 5},       // SF8,  BW250k, CR4/5
    {"SHORT_FAST", 7, 250000, 5},       // SF7,  BW250k, CR4/5
    {"SHORT_TURBO", 7, 500000, 5},      // SF7,  BW500k, CR4/5
    {"LONG_MODERATE", 11, 125000, 5},   // SF11, BW125k, CR4/5 - first BW125 preset
};
static constexpr uint8_t NUM_MODEM_PRESETS = sizeof(MODEM_PRESETS) / sizeof(MODEM_PRESETS[0]);
static constexpr uint8_t PRESET_LONG_FAST = 0;
static constexpr uint8_t PRESET_LONG_SLOW = 1;
// Stock Meshtastic no longer implements this one: a node told to use it comes back
// reporting SF11/250/CR4/5 - LongFast's parameters - so it is not a preset any more,
// it is a way to end up on a different modem from everyone you are trying to reach.
// Measured against a Heltec on 2026-08-26, both directions dead. The entry stays so
// the indices after it keep their meaning in saved settings; the picker skips it.
static constexpr uint8_t PRESET_VERY_LONG_SLOW = 2;
static constexpr uint8_t PRESET_SHORT_TURBO = 7;
static constexpr uint8_t PRESET_LONG_MODERATE = 8;

// Regional band edges - verbatim from meshtastic-device RadioInterface.cpp
// regions[] (RDEF freqStart/freqEnd/power). Multi-band countries use the
// sub-band the stock firmware maps the bare code to (MY->MY_919, SG->SG_923,
// PH->PH_915, NZ->NZ_865).
static constexpr RegionConfig REGIONS[] = {
    {"United States", "US", 902000000, 928000000, 30},
    {"EU 868 MHz", "EU", 869400000, 869650000, 27},
    {"EU 433 MHz", "EU4", 433000000, 434000000, 10},
    {"Australia", "ANZ", 915000000, 928000000, 30},
    {"New Zealand", "NZ", 864000000, 868000000, 36},
    {"Korea", "KR", 920000000, 923000000, 23},
    {"Taiwan", "TW", 920000000, 925000000, 27},
    {"Russia", "RU", 868700000, 869200000, 20},
    {"India", "IN", 865000000, 867000000, 30},
    {"Japan", "JP", 920500000, 923500000, 13},
    {"Malaysia", "MY", 919000000, 924000000, 27},
    {"Singapore", "SG", 917000000, 925000000, 20},
    {"Philippines", "PH", 915000000, 918000000, 24},
    {"China", "CN", 470000000, 510000000, 19},
    {"Unset", "NONE", 902000000, 928000000, 30},
};
static constexpr uint8_t NUM_REGIONS = sizeof(REGIONS) / sizeof(REGIONS[0]);

// Default channel name per preset - Channels.cpp hashes the modem-preset
// display name (DisplayFormatters long form) when the primary channel has
// no explicit name. Order must match MODEM_PRESETS.
static constexpr const char* PRESET_CHANNEL_NAMES[] = {
    "LongFast", "LongSlow", "VLongSlow", "MediumSlow",
    "MediumFast", "ShortSlow", "ShortFast", "ShortTurbo", "LongMod"};

// Our table order is not the wire enum. config.proto ModemPreset agrees with us for
// LONG_FAST..SHORT_FAST (0..6) but puts LONG_MODERATE at 7, so SHORT_TURBO is 8.
inline uint8_t meshtastic_modem_preset(uint8_t preset_idx) {
    if (preset_idx == PRESET_SHORT_TURBO) return 8;
    if (preset_idx == PRESET_LONG_MODERATE) return 7;
    return preset_idx;
}

// config.proto RegionCode, indexed by our REGIONS[] order (0 = UNSET on overflow).
inline uint8_t meshtastic_region_code(uint8_t region_idx) {
    static constexpr uint8_t CODES[] = {
        1,   // United States -> US
        3,   // EU 868        -> EU_868
        2,   // EU 433        -> EU_433
        6,   // Australia     -> ANZ
        11,  // New Zealand   -> NZ_865
        7,   // Korea         -> KR
        8,   // Taiwan        -> TW
        9,   // Russia        -> RU
        10,  // India         -> IN
        5,   // Japan         -> JP
        17,  // Malaysia      -> MY_919
        18,  // Singapore     -> SG_923
        21,  // Philippines   -> PH_915
        4,   // China         -> CN
        0,   // Unset         -> UNSET
    };
    return (region_idx < sizeof(CODES)) ? CODES[region_idx] : 0;
}

// Meshtastic channel hash (the 1-byte channel id in every packet header):
// generateHash() = xorHash(channel_name) ^ xorHash(psk), xorHash = XOR of all bytes.
// The header byte MUST equal this or MeshRouter::on_raw_rx drops the packet - the app
// had it hardcoded to ShortTurbo's 0x0E, so it silently rejected every LongFast (0x08)
// packet even though the M4 decoded it byte-exact. Compute it from the active preset.
inline uint8_t channel_hash(const char* name, const uint8_t* psk, size_t psk_len) {
    uint8_t h = 0;
    for (const char* s = name; s && *s; ++s) h ^= static_cast<uint8_t>(*s);
    for (size_t i = 0; i < psk_len; ++i) h ^= psk[i];
    return h;
}

// The same rule for a preset's default channel name. One rule, one implementation:
// it used to be written out separately here and at each of the three places the app
// hashes a channel, which is the shape of every bug this file's comments describe.
inline uint8_t preset_channel_hash(uint8_t preset_idx, const uint8_t* psk, size_t psk_len) {
    if (preset_idx >= NUM_MODEM_PRESETS) preset_idx = 0;
    return channel_hash(PRESET_CHANNEL_NAMES[preset_idx], psk, psk_len);
}

// djb2 (Dan Bernstein) - the hash RadioInterface.cpp uses to pick the
// default frequency slot from the channel name.
inline uint32_t name_hash(const char* s) {
    uint32_t h = 5381;
    while (*s)
        h = (h << 5) + h + static_cast<uint8_t>(*s++);
    return h;
}

// Frequency-slot count of a band at a given preset bandwidth.
// numChannels = floor((freqEnd - freqStart) / bw); region spacing is 0 for
// every region in the stock table. Clamped to 1 so presets wider than the
// band (SHORT_TURBO in EU 868) don't divide by zero.
inline uint32_t num_channels(uint8_t region_idx, uint8_t preset_idx) {
    const auto& r = REGIONS[region_idx];
    const auto& p = MODEM_PRESETS[preset_idx];
    const uint32_t n = static_cast<uint32_t>((r.freq_end_hz - r.freq_start_hz) / p.bw_hz);
    return n ? n : 1;
}

// Center frequency exactly as RadioInterface::applyModemConfig computes it:
// freq = freqStart + bw/2 + slot*bw, slot = hash(channelName) % numChannels.
// RU + SHORT_TURBO -> 1 slot -> 868.950 MHz (measured on-air from stock nodes).
// slot_override mirrors Meshtastic LoRaConfig.channel_num: 0 = derive the slot from the
// channel-name hash (default); 1..N = use that fixed 1-based slot in the region's band.
// Which slot in the band we sit on. Meshtastic derives it from the name of the PRIMARY
// channel; ours is always the preset's default channel, so the preset name is the name.
// Split out from channel_frequency() so a shared QR can state the slot outright.
// `name` is the primary channel's actual name; empty falls back to the preset's, which
// is what a node with an unset name does. It matters: Meshtastic derives the frequency
// slot from the channel NAME, and a node that has ever been configured keeps an explicit
// one across preset changes. At BW250 there are two slots and both "LongFast" and
// "ShortFast" happen to land on the same one, so this went unnoticed - at BW125 there
// are four, and "LongMod" and "LongFast" sit 250 kHz apart, which is a whole bandwidth.
inline uint32_t channel_slot(uint8_t region_idx, uint8_t preset_idx, uint8_t slot_override = 0, const char* name = nullptr) {
    if (region_idx >= NUM_REGIONS) region_idx = 0;
    if (preset_idx >= NUM_MODEM_PRESETS) preset_idx = 0;
    const uint32_t nch = num_channels(region_idx, preset_idx);
    if (slot_override) return static_cast<uint32_t>(slot_override - 1) % nch;
    const char* n = (name && *name) ? name : PRESET_CHANNEL_NAMES[preset_idx];
    return name_hash(n) % nch;
}

inline uint64_t channel_frequency(uint8_t region_idx, uint8_t preset_idx, uint8_t slot_override = 0, const char* name = nullptr) {
    if (region_idx >= NUM_REGIONS) region_idx = 0;
    if (preset_idx >= NUM_MODEM_PRESETS) preset_idx = 0;
    const auto& r = REGIONS[region_idx];
    const auto& p = MODEM_PRESETS[preset_idx];
    return r.freq_start_hz + p.bw_hz / 2 +
           static_cast<uint64_t>(channel_slot(region_idx, preset_idx, slot_override, name)) * p.bw_hz;
}

// A fresh install transmits nothing until somebody chooses where it is. Meshtastic
// itself ships this way, and for the same two reasons: the region decides the band,
// which is a matter of local law rather than of a sensible guess, and a node quietly
// working on somebody else's frequency is worse than one that says it is not
// configured. These two used to read "Russia" and "SHORT_TURBO to match the Heltec
// monitor" - the bench this was built on, left in by accident.
static constexpr uint8_t DEFAULT_REGION = 14;                // Unset
static constexpr uint8_t DEFAULT_PRESET = PRESET_LONG_FAST;  // what most meshes run

}  // namespace meshtastic

#endif /* __MESH_REGIONS_H__ */
