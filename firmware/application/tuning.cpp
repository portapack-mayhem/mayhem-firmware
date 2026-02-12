/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
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

#include "tuning.hpp"

#include "utility.hpp"

namespace tuning {
namespace config {

// Forward declarations
Config low_band(const rf::Frequency target_frequency);
Config mid_band(const rf::Frequency target_frequency);
Config high_band(const rf::Frequency target_frequency);

// Low band <2170 Mhz:
constexpr rf::Frequency low_band_second_lo_frequency(const rf::Frequency target_frequency) {
#ifdef PRALINE
    // Praline-specific formula for MAX2831 (2.3-2.6 GHz range)
    // Use a fixed second_lo that:
    // 1. Falls in MAX2831's sweet spot (2.3-2.6 GHz)
    // 2. Gives RFFC5072 a VCO frequency in its range (2700-5400 MHz)

    // For most low-band frequencies, use 2500 MHz as second_lo
    // This gives RFFC5072 plenty of headroom
    (void)target_frequency;  // Unused in fixed formula
    return 2500'000'000;
#else
    return 2650'000'000 - (target_frequency / 7);
#endif
}

Config low_band(const rf::Frequency target_frequency) {
    const rf::Frequency second_lo_frequency = low_band_second_lo_frequency(target_frequency);
    const rf::Frequency first_lo_frequency = target_frequency + second_lo_frequency;
    const bool mixer_invert = true;
    return {first_lo_frequency, second_lo_frequency, rf::path::Band::Low, mixer_invert};
}

// Mid band 2170-2740 Mhz:
Config mid_band(const rf::Frequency target_frequency) {
#ifdef PRALINE
    // For Praline with MAX2831 (2.3-2.6 GHz range)
    // Frequencies 2170-2300 MHz need upconversion since they're below MAX2831 minimum
    if (target_frequency < 2300'000'000) {
        // Treat as low band
        return low_band(target_frequency);
    }
    // Frequencies 2300-2600 MHz can go direct (no RFFC5072)
    else if (target_frequency <= 2600'000'000) {
        const rf::Frequency second_lo_frequency = target_frequency;
        const rf::Frequency first_lo_frequency = 0;
        const bool mixer_invert = false;
        return {first_lo_frequency, second_lo_frequency, rf::path::Band::Mid, mixer_invert};
    }
    // Frequencies 2600-2740 MHz need downconversion since they're above MAX2831 maximum
    else {
        // Treat as high band
        return high_band(target_frequency);
    }
#else
    const rf::Frequency second_lo_frequency = target_frequency;
    const rf::Frequency first_lo_frequency = 0;
    const bool mixer_invert = false;
    return {first_lo_frequency, second_lo_frequency, rf::path::Band::Mid, mixer_invert};
#endif
}

// High band >2740 Mhz:
constexpr rf::Frequency high_band_second_lo_frequency(const rf::Frequency target_frequency) {
#ifdef PRALINE
    // Praline formula tuned for MAX2831 (2.3-2.6 GHz range)
    // Keep second_lo in MAX2831's range while allowing RFFC5072 to work
    if (target_frequency < 3600'000'000)
        return 2400'000'000 + ((target_frequency - 2740'000'000) / 4);
    else if (target_frequency < 5100'000'000)
        return 2500'000'000 + ((target_frequency - 3600'000'000) / 6);
    else
        return 2550'000'000 + ((target_frequency - 5100'000'000) / 10);
#else
    if (target_frequency < 3600'000'000)
        return (2170'000'000 + (((target_frequency - 2740'000'000) * 57) / 86));
    else if (target_frequency < 5100'000'000)
        return (2350'000'000 + ((target_frequency - 3600'000'000) / 5));
    else
        return (2500'000'000 + ((target_frequency - 5100'000'000) / 9));
#endif
}

Config high_band(const rf::Frequency target_frequency) {
    const rf::Frequency second_lo_frequency = high_band_second_lo_frequency(target_frequency);
    const rf::Frequency first_lo_frequency = target_frequency - second_lo_frequency;
    const bool mixer_invert = false;
    return {first_lo_frequency, second_lo_frequency, rf::path::Band::High, mixer_invert};
}

Config create(const rf::Frequency target_frequency) {
    /* TODO: This is some lame code. */
    if (rf::path::band_low.contains(target_frequency)) {
        return low_band(target_frequency);
    } else if (rf::path::band_mid.contains(target_frequency)) {
        return mid_band(target_frequency);
    } else if (rf::path::band_high.contains(target_frequency)) {
        return high_band(target_frequency);
    } else {
        return {};
    }
}

} /* namespace config */
} /* namespace tuning */
