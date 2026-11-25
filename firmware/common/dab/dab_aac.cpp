/*
 * Copyright (C) 2026 Mayhem firmware contributors
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

#include "dab_aac.hpp"

namespace dab {

namespace {

/* MPEG-4 sampling frequency index 0..12. */
constexpr uint32_t kSamplingFrequencies[13] = {
    96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350};

}  // namespace

uint8_t aac_sampling_frequency_index(uint32_t rate) {
    for (uint8_t i = 0; i < 13; i++) {
        if (kSamplingFrequencies[i] == rate)
            return i;
    }
    return 0x0F;  // "explicit rate follows", which this config never uses
}

AacConfig aac_config_from_superframe(const SuperframeHeader& header) {
    AacConfig config{};

    if (header.audio_unit_count == 0)
        return config;

    config.output_rate = header.output_sample_rate();
    config.core_rate = header.core_sample_rate();
    config.sbr = (header.sbr_flag != 0);
    config.parametric_stereo = (header.ps_flag != 0);
    config.channels = header.aac_channel_mode ? 2 : 1;

    const uint8_t frequency_index = aac_sampling_frequency_index(config.core_rate);
    if (frequency_index > 12)
        return config;

    /* AudioSpecificConfig, MSB first: audioObjectType 5 | samplingFrequencyIndex 4
     * | channelConfiguration 4 | frameLengthFlag 1 (1 = 960 transform) |
     * dependsOnCoreCoder 1 | extensionFlag 1 = 16 bits. */
    const uint16_t packed = static_cast<uint16_t>(
        (static_cast<uint16_t>(kAacObjectTypeLc & 0x1F) << 11) |
        (static_cast<uint16_t>(frequency_index & 0x0F) << 7) |
        (static_cast<uint16_t>(config.channels & 0x0F) << 3) |
        (static_cast<uint16_t>(1) << 2));

    config.bytes[0] = static_cast<uint8_t>(packed >> 8);
    config.bytes[1] = static_cast<uint8_t>(packed & 0xFF);
    config.length = 2;
    config.valid = true;
    return config;
}

} /* namespace dab */

