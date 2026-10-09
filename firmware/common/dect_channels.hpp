/*
 * DECT channel tables and timing constants for the PortaPack Mayhem DECT RX.
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
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
 *
 * DECT channel layout and the decode pipeline are ported from DeDECTive
 * (SarahRose), which itself ports the DECT receiver from gr-dect2 by
 * Pavel Yazev (GPLv3). Attribution retained as required by the GPL.
 */

#ifndef __DECT_CHANNELS_H__
#define __DECT_CHANNELS_H__

#include <cstdint>
#include <cstddef>

namespace dect {

/* DECT symbol rate (GFSK, 1.152 Mbaud). */
inline constexpr uint32_t DECT_SYMBOL_RATE = 1152000;

/* Decode at exactly 4x the symbol rate: 4 IQ samples per bit, no resampler. */
inline constexpr uint32_t SAMPLE_RATE = 4 * DECT_SYMBOL_RATE; /* 4,608,000 Hz */

/* DECT channel spacing (1.728 MHz). */
inline constexpr uint32_t DECT_CHAN_SPACING = 1728000;

inline constexpr size_t NUM_DECT_CHANNELS = 10;

struct DectChannel {
    uint8_t number;
    uint64_t freq_hz;
};

/* US DECT 6.0 - FCC Part 15 Subpart D: 1921.536-1937.088 MHz. */
inline constexpr DectChannel US_DECT_CHANNELS[NUM_DECT_CHANNELS] = {
    {0, 1921536000ULL},
    {1, 1923264000ULL},
    {2, 1924992000ULL},
    {3, 1926720000ULL},
    {4, 1928448000ULL},
    {5, 1930176000ULL},
    {6, 1931904000ULL},
    {7, 1933632000ULL},
    {8, 1935360000ULL},
    {9, 1937088000ULL},
};

/* EU DECT - ETSI EN 300 175: 1897.344 - n * 1.728 MHz. */
inline constexpr DectChannel EU_DECT_CHANNELS[NUM_DECT_CHANNELS] = {
    {0, 1897344000ULL},
    {1, 1895616000ULL},
    {2, 1893888000ULL},
    {3, 1892160000ULL},
    {4, 1890432000ULL},
    {5, 1888704000ULL},
    {6, 1886976000ULL},
    {7, 1885248000ULL},
    {8, 1883520000ULL},
    {9, 1881792000ULL},
};

enum class DectBand : uint8_t {
    US = 0,
    EU = 1,
};

inline const DectChannel* dect_channels(DectBand band) {
    return (band == DectBand::US) ? US_DECT_CHANNELS : EU_DECT_CHANNELS;
}

inline uint64_t dect_center_freq(DectBand band) {
    const DectChannel* ch = dect_channels(band);
    return (ch[0].freq_hz + ch[NUM_DECT_CHANNELS - 1].freq_hz) / 2;
}

}  // namespace dect

#endif /*__DECT_CHANNELS_H__*/
