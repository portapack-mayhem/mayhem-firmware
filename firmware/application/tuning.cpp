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

#ifdef PRALINE
/*
 * PRALINE Tuning Configuration
 * ============================
 *
 * Reference: hackrf_usb/common/tune_config.h praline_tune_config_rx[]
 *
 * The hackrf_usb firmware uses a table-driven approach where each entry
 * specifies:
 *   - rf_range_end_mhz: Upper frequency limit for this config
 *   - if_mhz: IF frequency (what MAX2831 tunes to)
 *   - high_lo: true = high-side injection, false = low-side
 *   - shift: FPGA quarter-shift mode (not implemented in Mayhem yet)
 *
 * Key insight: The IF frequency varies to keep the RFFC5072 VCO in a
 * safe operating range (ideally 3500-5000 MHz, avoiding extremes).
 *
 * RFFC5072 VCO calculation:
 *   High-side injection: LO = IF + RF, VCO = LO × lodiv
 *   Low-side injection:  LO = IF - RF, VCO = LO × lodiv
 *   Where lodiv = 2 for frequencies where VCO > 2700 MHz
 *
 * From hackrf_usb tune_config_rx (simplified):
 *   0-2100 MHz:  IF=2375, high_lo=true  → VCO = (2375+RF)×2
 *   2105-2115:   IF=2375, high_lo=false → VCO = (2375-RF)×2
 *   2115-2130:   IF=2425, high_lo=false → VCO = (2425-RF)×2
 *   ... (more entries for fine-grained control)
 *   2320-2580:   IF=0 (bypass mode, no mixer)
 *   2580+:       High-pass mode
 */

// Simplified tune_config lookup for Mayhem
// Returns the IF frequency in Hz for a given target frequency
constexpr rf::Frequency praline_get_if_frequency(const rf::Frequency target_frequency) {
    const uint32_t freq_mhz = target_frequency / 1'000'000;

    // Based on hackrf_usb tune_config_rx table
    if (freq_mhz < 2100) {
        // Most low-band frequencies: use 2375 MHz IF
        // This keeps VCO around 4750-4950 MHz for FM band
        return 2375'000'000;
    } else if (freq_mhz < 2320) {
        // Transition zone: use varying IF to avoid VCO edges
        // These frequencies are tricky - near MAX2831 minimum
        // Use 2425 MHz to give some margin
        return 2425'000'000;
    } else {
        // Bypass mode or high-band - IF not used for mixer
        return 0;
    }
}

// Returns true for high-side injection, false for low-side
constexpr bool praline_use_high_side_injection(const rf::Frequency target_frequency) {
    const uint32_t freq_mhz = target_frequency / 1'000'000;

    // Based on hackrf_usb tune_config_rx table
    if (freq_mhz < 2100) {
        // Standard low-band: high-side injection
        // LO = IF + RF, mixer inverts spectrum
        return true;
    } else if (freq_mhz < 2105) {
        // Narrow transition: still high-side
        return true;
    } else if (freq_mhz < 2320) {
        // Near MAX2831 minimum: use low-side injection
        // LO = IF - RF, no spectrum inversion
        return false;
    } else {
        // Bypass/high-band - doesn't matter, mixer bypassed
        return false;
    }
}
#endif  // PRALINE

// Low band <2170 Mhz (HackRF One) or <2320 MHz (PRALINE):
constexpr rf::Frequency low_band_second_lo_frequency(const rf::Frequency target_frequency) {
#ifdef PRALINE
    // Use the tune_config lookup for PRALINE
    return praline_get_if_frequency(target_frequency);
#else
    return 2650'000'000 - (target_frequency / 7);
#endif
}

Config low_band(const rf::Frequency target_frequency) {
    const rf::Frequency second_lo_frequency = low_band_second_lo_frequency(target_frequency);

#ifdef PRALINE
    rf::Frequency first_lo_frequency;
    bool mixer_invert;

    if (praline_use_high_side_injection(target_frequency)) {
        // High-side injection: LO = IF + RF
        first_lo_frequency = second_lo_frequency + target_frequency;
        mixer_invert = true;
    } else {
        // Low-side injection: LO = IF - RF
        first_lo_frequency = second_lo_frequency - target_frequency;
        mixer_invert = false;
    }

    return {first_lo_frequency, second_lo_frequency, rf::path::Band::Low, mixer_invert};
#else
    const rf::Frequency first_lo_frequency = target_frequency + second_lo_frequency;
    const bool mixer_invert = true;
    return {first_lo_frequency, second_lo_frequency, rf::path::Band::Low, mixer_invert};
#endif
}

// Mid band 2170-2740 Mhz (HackRF One) or 2320-2580 MHz (PRALINE):
Config mid_band(const rf::Frequency target_frequency) {
#ifdef PRALINE
    // For Praline with MAX2831 (2.3-2.6 GHz range)
    // Frequencies 2170-2300 MHz need upconversion since they're below MAX2831 minimum
    if (target_frequency < 2300'000'000) {
        // Treat as low band - need mixer
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

// High band >2740 Mhz (HackRF One) or >2580 MHz (PRALINE):
constexpr rf::Frequency high_band_second_lo_frequency(const rf::Frequency target_frequency) {
#ifdef PRALINE
    // Praline formula tuned for MAX2831 (2.3-2.6 GHz range)
    // Keep second_lo in MAX2831's range while allowing RFFC5072 to work
    //
    // For high-band, we use LOW-side injection: LO = RF - IF
    // So IF should be chosen to keep LO (and thus VCO) in a good range
    //
    // Based on hackrf_usb tune_config_tx patterns:
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
