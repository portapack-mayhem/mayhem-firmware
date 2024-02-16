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

// Low band <2170 Mhz:
constexpr rf::Frequency low_band_second_lo_frequency(const rf::Frequency target_frequency) {
    return 2650'000'000 - (target_frequency / 7);
}

Config low_band(const rf::Frequency target_frequency) {
    const rf::Frequency second_lo_frequency = low_band_second_lo_frequency(target_frequency);
    const rf::Frequency first_lo_frequency = target_frequency + second_lo_frequency;
    const bool mixer_invert = true;
    return {first_lo_frequency, second_lo_frequency, rf::path::Band::Low, mixer_invert};
}

// Mid band 2170-2740 Mhz:
Config mid_band(const rf::Frequency target_frequency) {
    const rf::Frequency second_lo_frequency = target_frequency;
    const rf::Frequency first_lo_frequency = 0;
    const bool mixer_invert = false;
    return {first_lo_frequency, second_lo_frequency, rf::path::Band::Mid, mixer_invert};
}

// High band >2740 Mhz:
constexpr rf::Frequency high_band_second_lo_frequency(const rf::Frequency target_frequency) {
    if (target_frequency < 3600'000'000)
        return (2170'000'000 + (((target_frequency - 2740'000'000) * 57) / 86));
    else if (target_frequency < 5100'000'000)
        return (2350'000'000 + ((target_frequency - 3600'000'000) / 5));
    else
        return (2500'000'000 + ((target_frequency - 5100'000'000) / 9));
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
