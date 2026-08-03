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
Config low_band(const rf::Frequency target_frequency, const uint32_t afe_rate, const bool transmit);
Config mid_band(const rf::Frequency target_frequency, const uint32_t afe_rate, const bool transmit);
Config high_band(const rf::Frequency target_frequency);

#ifdef PRALINE
/*
 * PRALINE Tuning Configuration
 * ============================
 *
 * These tables are copied verbatim from the reference firmware,
 * hackrf/firmware/common/tune_config.h (praline_tune_config_rx /
 * praline_tune_config_tx), and the selection and offset maths below reproduce
 * hackrf/firmware/common/radio.c radio_update_frequency() /
 * analog_from_digital_rf() / compute_offset().
 *
 * Each entry gives, for target frequencies up to rf_range_end_mhz:
 *   if_mhz    the IF the MAX2831 tunes to (0 = mixer bypassed, IF = RF)
 *   high_lo   true  -> LO = IF + analogue RF (mixer inverts the spectrum)
 *             false -> LO = IF - analogue RF (no inversion)
 *   shift     the FPGA quarter-rate shift mode used for this entry
 *
 * The quarter-rate shift is the part Mayhem was previously missing. RX entries
 * deliberately place the analogue passband a quarter of the ADC rate away from
 * the requested frequency (+8 MHz at the usual 32 Msps AFE rate) so that the
 * wanted signal never sits on the DC offset / LO leakage, and then ask the FPGA
 * to rotate it back down to DC. Both halves have to be programmed together:
 *   - tuning to target + offset without asking the FPGA to rotate leaves the
 *     signal 8 MHz out and the decimation filter deletes it;
 *   - tuning to target with no offset (what Mayhem did) parks the signal on DC,
 *     under the LO leakage and the gateware's adaptive DC block.
 *
 * "up" and "down" are named for the direction the FPGA rotates, so
 * FPGA_QUARTER_SHIFT_MODE_UP means the analogue centre is placed ABOVE the
 * requested frequency, and DOWN below it (radio.c analog_from_digital_rf()).
 */

namespace {

struct PralineTuneConfig {
    uint16_t rf_range_end_mhz;
    uint16_t if_mhz;
    bool high_lo;
    uint8_t shift; /* 0b00 none, 0b11 up, 0b01 down */
};

/* tuning table optimized for RX */
constexpr PralineTuneConfig praline_tune_config_rx[] = {
    {    0,  2360,   true, 0b00},
    {   50,  2320,   true, 0b11},
    {  100,  2320,   true, 0b01},
    {  140,  2320,   true, 0b11},
    {  406,  2560,   true, 0b11},
    {  511,  2380,   true, 0b11},
    {  578,  2560,   true, 0b01},
    {  741,  2340,   true, 0b11},
    {  861,  2560,   true, 0b01},
    {  921,  2560,   true, 0b11},
    { 1049,  2340,   true, 0b01},
    { 1169,  2380,   true, 0b11},
    { 1360,  2340,   true, 0b11},
    { 1544,  2560,   true, 0b01},
    { 1675,  2560,   true, 0b11},
    { 1992,  2380,   true, 0b01},
    { 2070,  2340,   true, 0b01},
    { 2150,  2360,   true, 0b01},
    { 2168,  2560,  false, 0b11},
    { 2185,  2580,  false, 0b11},
    { 2202,  2580,  false, 0b01},
    { 2205,  2520,  false, 0b11},
    { 2216,  2560,  false, 0b11},
    { 2223,  2540,  false, 0b11},
    { 2234,  2580,  false, 0b11},
    { 2240,  2560,  false, 0b11},
    { 2251,  2580,  false, 0b01},
    { 2258,  2580,  false, 0b11},
    { 2265,  2540,  false, 0b01},
    { 2271,  2580,  false, 0b11},
    { 2273,  2560,  false, 0b11},
    { 2275,  2580,  false, 0b01},
    { 2280,  2500,  false, 0b01},
    { 2284,  2540,  false, 0b11},
    { 2289,  2580,  false, 0b01},
    { 2293,  2540,  false, 0b01},
    { 2298,  2520,  false, 0b01},
    { 2300,  2580,  false, 0b11},
    { 2302,  2540,  false, 0b01},
    { 2309,  2560,  false, 0b01},
    { 2311,  2580,  false, 0b01},
    { 2314,  2540,  false, 0b11},
    { 2315,  2540,  false, 0b01},
    { 2320,  2580,  false, 0b11},
    { 2380,     0,  false, 0b11},
    { 2440,     0,  false, 0b01},
    { 2500,     0,  false, 0b11},
    { 2580,     0,  false, 0b01},
    { 2583,  2360,  false, 0b11},
    { 2584,  2380,  false, 0b11},
    { 2587,  2340,  false, 0b11},
    { 2593,  2340,  false, 0b01},
    { 2607,  2340,  false, 0b11},
    { 2609,  2360,  false, 0b11},
    { 2615,  2360,  false, 0b01},
    { 2627,  2340,  false, 0b01},
    { 2629,  2360,  false, 0b01},
    { 2631,  2380,  false, 0b11},
    { 2644,  2340,  false, 0b11},
    { 2649,  2380,  false, 0b11},
    { 2651,  2380,  false, 0b01},
    { 2654,  2500,  false, 0b11},
    { 2665,  2360,  false, 0b11},
    { 2669,  2380,  false, 0b01},
    { 2672,  2360,  false, 0b01},
    { 2682,  2340,  false, 0b11},
    { 2687,  2380,  false, 0b11},
    { 2692,  2340,  false, 0b11},
    { 2695,  2500,  false, 0b11},
    { 2705,  2360,  false, 0b11},
    { 2707,  2380,  false, 0b01},
    { 2712,  2340,  false, 0b01},
    { 2717,  2520,  false, 0b11},
    { 2728,  2380,  false, 0b11},
    { 2730,  2560,  false, 0b11},
    { 2734,  2500,  false, 0b11},
    { 2758,  2340,  false, 0b11},
    { 2780,  2360,  false, 0b11},
    { 2787,  2520,  false, 0b11},
    { 2802,  2380,  false, 0b11},
    { 2809,  2540,  false, 0b11},
    { 2822,  2380,  false, 0b01},
    { 2831,  2560,  false, 0b11},
    { 2854,  2340,  false, 0b11},
    { 2875,  2360,  false, 0b11},
    { 2898,  2380,  false, 0b11},
    { 2918,  2380,  false, 0b01},
    { 2936,  2520,  false, 0b01},
    { 2944,  2380,  false, 0b01},
    { 2959,  2560,  false, 0b11},
    { 2976,  2340,  false, 0b11},
    { 2985,  2500,  false, 0b01},
    { 3003,  2340,  false, 0b11},
    { 3009,  2540,  false, 0b11},
    { 3027,  2380,  false, 0b11},
    { 3034,  2560,  false, 0b11},
    { 3050,  2380,  false, 0b01},
    { 3069,  2500,  false, 0b11},
    { 3094,  2520,  false, 0b11},
    { 3119,  2540,  false, 0b11},
    { 3144,  2560,  false, 0b11},
    { 3169,  2560,  false, 0b01},
    { 3180,  2500,  false, 0b11},
    { 3204,  2340,  false, 0b11},
    { 3232,  2360,  false, 0b11},
    { 3292,  2340,  false, 0b01},
    { 3340,  2380,  false, 0b01},
    { 3369,  2340,  false, 0b11},
    { 3399,  2360,  false, 0b11},
    { 3429,  2380,  false, 0b11},
    { 3464,  2500,  false, 0b11},
    { 3489,  2520,  false, 0b11},
    { 3512,  2540,  false, 0b11},
    { 3551,  2500,  false, 0b01},
    { 3582,  2540,  false, 0b11},
    { 3611,  2560,  false, 0b11},
    { 3639,  2520,  false, 0b11},
    { 3729,  2340,  false, 0b11},
    { 3817,  2380,  false, 0b01},
    { 3942,  2360,  false, 0b01},
    { 4049,  2540,  false, 0b11},
    { 4134,  2500,  false, 0b01},
    { 4194,  2560,  false, 0b11},
    { 4353,  2520,  false, 0b11},
    { 4449,  2360,  false, 0b01},
    { 4562,  2500,  false, 0b11},
    { 4672,  2560,  false, 0b11},
    { 4769,  2540,  false, 0b11},
    { 4849,  2560,  false, 0b01},
    { 4889,  2560,  false, 0b11},
    { 4929,  2560,  false, 0b11},
    { 4969,  2560,  false, 0b11},
    { 5009,  2560,  false, 0b11},
    { 5049,  2560,  false, 0b11},
    { 5092,  2360,  false, 0b11},
    { 5209,  2340,  false, 0b01},
    { 5298,  2380,  false, 0b01},
    { 5468,  2340,  false, 0b01},
    { 5582,  2520,  false, 0b11},
    { 5702,  2340,  false, 0b11},
    { 5888,  2520,  false, 0b01},
    { 6092,  2340,  false, 0b01},
    { 6240,  2560,  false, 0b11},
    { 6609,  2340,  false, 0b11},
    { 6752,  2380,  false, 0b01},
    { 6930,  2520,  false, 0b01},
    { 7000,  2560,  false, 0b11},
    { 7070,  2560,  false, 0b01},
    { 7251,  2580,  false, 0b01},
    {    0,     0,  false, 0b00},
};

/* tuning table optimized for TX */
constexpr PralineTuneConfig praline_tune_config_tx[] = {
    { 2100,  2375,   true, 0b00},
    { 2105,  2375,  false, 0b00},
    { 2115,  2425,  false, 0b00},
    { 2130,  2375,  false, 0b00},
    { 2150,  2425,  false, 0b00},
    { 2160,  2475,  false, 0b00},
    { 2175,  2425,  false, 0b00},
    { 2190,  2475,  false, 0b00},
    { 2195,  2425,  false, 0b00},
    { 2210,  2375,  false, 0b00},
    { 2248,  2425,  false, 0b00},
    { 2265,  2525,  false, 0b00},
    { 2300,  2425,  false, 0b00},
    { 2320,  2525,  false, 0b00},
    { 2580,     0,  false, 0b00},
    { 3000,  2325,  false, 0b00},
    { 3140,  2375,  false, 0b00},
    { 3200,  2425,  false, 0b00},
    { 3280,  2375,  false, 0b00},
    { 3340,  2425,  false, 0b00},
    { 3420,  2475,  false, 0b00},
    { 3480,  2525,  false, 0b00},
    { 3500,  2475,  false, 0b00},
    { 3595,  2425,  false, 0b00},
    { 3625,  2375,  false, 0b00},
    { 3670,  2475,  false, 0b00},
    { 3710,  2425,  false, 0b00},
    { 3760,  2525,  false, 0b00},
    { 3790,  2475,  false, 0b00},
    { 3860,  2425,  false, 0b00},
    { 3915,  2375,  false, 0b00},
    { 4000,  2425,  false, 0b00},
    { 4055,  2375,  false, 0b00},
    { 4125,  2425,  false, 0b00},
    { 4700,  2375,  false, 0b00},
    { 4800,  2425,  false, 0b00},
    { 5000,  2375,  false, 0b00},
    { 5260,  2475,  false, 0b00},
    { 5465,  2525,  false, 0b00},
    { 5560,  2375,  false, 0b00},
    { 5720,  2425,  false, 0b00},
    { 5860,  2475,  false, 0b00},
    { 5970,  2575,  false, 0b00},
    { 6000,  2375,  false, 0b00},
    { 6500,  2325,  false, 0b00},
    { 6750,  2375,  false, 0b00},
    { 6850,  2425,  false, 0b00},
    { 6950,  2475,  false, 0b00},
    { 7000,  2525,  false, 0b00},
    { 7251,  2575,  false, 0b00},
    {    0,     0,  false, 0b00},
};

/* radio.c select_tune_config(): first entry whose range end is above the
 * requested frequency. The list is terminated by an all-zero entry, which is
 * also what a frequency past the end of the table lands on. */
const PralineTuneConfig* select_tune_config(const rf::Frequency target_frequency, const bool transmit) {
    const PralineTuneConfig* entry = transmit ? praline_tune_config_tx : praline_tune_config_rx;
    const uint32_t freq_mhz = static_cast<uint32_t>(target_frequency / 1'000'000);

    while ((entry->rf_range_end_mhz != 0) || (entry->if_mhz != 0)) {
        if ((target_frequency == 0) || (entry->rf_range_end_mhz > freq_mhz))
            break;
        entry++;
    }
    return entry;
}

/* radio.c compute_offset(): a quarter of the AFE (ADC) sample rate, or zero if
 * no shift is in use or the AFE rate isn't known yet. */
constexpr uint32_t quarter_shift_offset(const uint8_t shift, const uint32_t afe_rate) {
    return (shift == 0) ? 0 : (afe_rate / 4);
}

/* radio.c analog_from_digital_rf(). */
rf::Frequency analog_from_digital_rf(const rf::Frequency target_frequency, const uint8_t shift, const uint32_t afe_rate) {
    const rf::Frequency offset = quarter_shift_offset(shift, afe_rate);

    if (shift == 0b11)
        return target_frequency + offset;

    if (shift == 0b01)
        return (offset > target_frequency) ? (offset - target_frequency)
                                           : (target_frequency - offset);

    return target_frequency;
}

}  // namespace
#endif  // PRALINE

Config low_band(const rf::Frequency target_frequency, const uint32_t afe_rate, const bool transmit) {
#ifdef PRALINE
    const PralineTuneConfig* entry = select_tune_config(target_frequency, transmit);

    /* Past the end of the table: no usable configuration. */
    if ((entry->rf_range_end_mhz == 0) && (entry->if_mhz == 0))
        return {};

    const uint8_t shift = entry->shift;
    const rf::Frequency analog_rf = analog_from_digital_rf(target_frequency, shift, afe_rate);

    /* if_mhz == 0 means the mixer is bypassed and the transceiver tunes the RF
     * directly; there is no first LO in that case. */
    const rf::Frequency second_lo_frequency =
        (entry->if_mhz == 0) ? analog_rf : (static_cast<rf::Frequency>(entry->if_mhz) * 1'000'000);

    if (entry->if_mhz == 0)
        return {0, second_lo_frequency, rf::path::Band::Low, false, shift};

    /* The low band always runs through the low-pass image-reject filter, so
     * the spectrum is inverted exactly when the first LO ends up above the IF,
     * i.e. for high-side injection. This is hackrf_usb.c radio_changed():
     *   invert = (img_reject == RF_PATH_FILTER_LOW_PASS) && (freq_lo > freq_if)
     */
    rf::Frequency first_lo_frequency;
    bool mixer_invert;

    if (entry->high_lo) {
        first_lo_frequency = second_lo_frequency + analog_rf;
        mixer_invert = true;
    } else {
        first_lo_frequency = second_lo_frequency - analog_rf;
        mixer_invert = false;
    }

    return {first_lo_frequency, second_lo_frequency, rf::path::Band::Low, mixer_invert, shift};
#else
    (void)afe_rate;
    (void)transmit;
    const rf::Frequency second_lo_frequency = 2650'000'000 - (target_frequency / 7);
    const rf::Frequency first_lo_frequency = target_frequency + second_lo_frequency;
    const bool mixer_invert = true;
    return {first_lo_frequency, second_lo_frequency, rf::path::Band::Low, mixer_invert};
#endif
}

// Mid band 2170-2740 Mhz (HackRF One) or 2320-2580 MHz (PRALINE):
Config mid_band(const rf::Frequency target_frequency, const uint32_t afe_rate, const bool transmit) {
#ifdef PRALINE
    // For Praline with MAX2831 (2.3-2.6 GHz range)
    // Frequencies 2170-2300 MHz need upconversion since they're below MAX2831 minimum
    if (target_frequency < 2300'000'000) {
        // Treat as low band - need mixer
        return low_band(target_frequency, afe_rate, transmit);
    }
    // Frequencies 2300-2600 MHz can go direct (no RFFC5072)
    else if (target_frequency <= 2600'000'000) {
        const rf::Frequency second_lo_frequency = target_frequency;
        const rf::Frequency first_lo_frequency = 0;
        const bool mixer_invert = false;
        return {first_lo_frequency, second_lo_frequency, rf::path::Band::Mid, mixer_invert, 0};
    }
    // Frequencies 2600-2740 MHz need downconversion since they're above MAX2831 maximum
    else {
        // Treat as high band
        return high_band(target_frequency);
    }
#else
    (void)afe_rate;
    (void)transmit;
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

Config create(const rf::Frequency target_frequency, const uint32_t afe_rate, const bool transmit) {
    /* TODO: This is some lame code. */
    if (rf::path::band_low.contains(target_frequency)) {
        return low_band(target_frequency, afe_rate, transmit);
    } else if (rf::path::band_mid.contains(target_frequency)) {
        return mid_band(target_frequency, afe_rate, transmit);
    } else if (rf::path::band_high.contains(target_frequency)) {
        return high_band(target_frequency);
    } else {
        return {};
    }
}

} /* namespace config */
} /* namespace tuning */
