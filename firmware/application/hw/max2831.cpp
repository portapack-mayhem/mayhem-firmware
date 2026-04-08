/*
 * Copyright (C) 2025 Great Scott Gadgets
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

/*
 * MAX2831 driver ported from GSG HackRF reference implementation (max2831.c).
 * Adapted to work with Mayhem's MAX283x abstraction layer.
 */

#ifdef PRALINE

#include "max2831.hpp"

#include "hackrf_hal.hpp"
#include "hackrf_gpio.hpp"
using namespace hackrf::one;

#include "ch.h"
#include "hal.h"

#include <algorithm>
#include <cstring>

namespace max2831 {

using namespace max283x;

static MAX2831Info max2831_info = {0, 0, 0, false, false};

MAX2831Info get_max2831_info() {
    return {
        max2831_info.requested_freq_mhz,
        max2831_info.calculated_n,
        max2831_info.calculated_frac,
        max2831_info.set_frequency_called,
        max2831_info.frequency_valid};
}

/*
 * MAX2831 uses 9-bit SPI transfers.
 * An 18-bit word is sent as two 9-bit transfers:
 *   Word format: [VALUE:14][REG:4]
 *   First transfer: bits 17:9 (high 9 bits)
 *   Second transfer: bits 8:0 (low 9 bits)
 *
 * This matches the GSG reference implementation exactly.
 */
void MAX2831::write_reg(const uint8_t reg, const uint16_t value) {
    uint32_t word = (((uint32_t)value & 0x3fff) << 4) | (reg & 0xf);
    uint16_t values[2] = {
        static_cast<uint16_t>(word >> 9),
        static_cast<uint16_t>(word & 0x1ff)};
    _target.transfer(values, 2);
}

void MAX2831::set_reg_field(const uint8_t reg, const uint16_t mask, const uint16_t value) {
    _regs[reg] = (_regs[reg] & ~mask) | (value & mask);
    mark_dirty(reg);
}

uint16_t MAX2831::get_reg_field(const uint8_t reg, const uint16_t mask, const uint8_t shift) {
    return (_regs[reg] & mask) >> shift;
}

void MAX2831::mark_dirty(const uint8_t reg) {
    _regs_dirty |= (1 << reg);
}

void MAX2831::mark_clean(const uint8_t reg) {
    _regs_dirty &= ~(1 << reg);
}

void MAX2831::flush_reg(const uint8_t reg) {
    write_reg(reg, _regs[reg]);
    mark_clean(reg);
}

void MAX2831::flush_dirty() {
    for (size_t r = 0; r < reg_count; r++) {
        if ((_regs_dirty >> r) & 0x1) {
            flush_reg(r);
        }
    }
}

void MAX2831::init() {
    set_mode(Mode::Shutdown);

    /* Configure GPIO pins for MAX2831 control */
    gpio_max283x_enable.output();
    gpio_max2831_rx_enable.output();
    gpio_max2831_rxhp.output();
    gpio_max2831_rxhp.write(0); /* RXHP low = 100 Hz HPF (default) */

    /* Reset to default register values */
    std::memcpy(_regs.data(), default_regs.data(), sizeof(_regs));
    _regs_dirty = 0xFFFF;

    /* Write default register values to chip */
    flush_dirty();

    /* Use SPI control instead of B1-B7 pins for gain settings.
     * This matches the GSG reference: max2831_setup() */
    set_reg_field(8, REG8_RXVGA_GAIN_SPI_EN, REG8_RXVGA_GAIN_SPI_EN);
    set_reg_field(9, REG9_TXVGA_GAIN_SPI_EN, REG9_TXVGA_GAIN_SPI_EN);

    /* Set initial gains - matches GSG reference */
    set_reg_field(12, REG12_TXVGA_GAIN_MASK, 0x00); /* Minimum TX gain */
    set_reg_field(7, REG7_RX_HPF_SEL_MASK, REG7_RX_HPF_30KHZ);
    set_reg_field(11, REG11_LNA_GAIN_MASK, REG11_LNA_GAIN_MAX);
    set_reg_field(11, REG11_RXVGA_GAIN_MASK, 0x18);  // Moderate RX VGA gain

    /* FORCE MAXIMUM GAIN FOR TESTING */
    // set_reg_field(11, REG11_RXVGA_GAIN_MASK, 0x1F);  // 62 dB VGA = MAX

    /* Configure baseband filter for 8 MHz TX - matches GSG reference */
    // set_reg_field(8, REG8_LPF_COARSE_MASK, REG8_RX_LPF_7_5M);
    set_reg_field(8, REG8_LPF_COARSE_MASK, REG8_RX_LPF_15M);
    set_reg_field(7, REG7_RX_LPF_FINE_MASK, REG7_RX_LPF_FINE_100);
    set_reg_field(7, REG7_TX_LPF_FINE_MASK, REG7_TX_LPF_FINE_100);

    /* Disable clock output */
    set_reg_field(14, REG14_CLKOUT_PIN_EN, 0);

    /* Write all modified registers */
    flush_dirty();

    set_mode(Mode::Standby);
}

void MAX2831::set_mode(const Mode mode) {
    _mode = mode;

    /*
     * MAX2831 mode control via ENABLE and RXTX pins.
     * From GSG hackrf max2831_target.c:
     *
     *   Shutdown: ENABLE=0, RXTX=0
     *   Standby:  ENABLE=0, RXTX=1  (PLL/VCO/LO on, ready for quick TX/RX)
     *   RX:       ENABLE=1, RXTX=0
     *   TX:       ENABLE=1, RXTX=1
     *
     * Note: gpio_max2831_rx_enable is the RXTX mode select pin.
     * RXTX=0 selects RX, RXTX=1 selects TX.
     */

    /* Handle calibration mode bits if needed */
    bool tx_cal = (mode == Mode::Tx_Calibration);
    bool rx_cal = (mode == Mode::Rx_Calibration);

    uint16_t current_tx_cal = get_reg_field(6, REG6_TX_CAL_MODE_EN, REG6_TX_CAL_MODE_EN_SHIFT);
    uint16_t current_rx_cal = get_reg_field(6, REG6_RX_CAL_MODE_EN, REG6_RX_CAL_MODE_EN_SHIFT);

    if (current_tx_cal != (tx_cal ? 1 : 0)) {
        set_reg_field(6, REG6_TX_CAL_MODE_EN, tx_cal ? REG6_TX_CAL_MODE_EN : 0);
        flush_dirty();
    }
    if (current_rx_cal != (rx_cal ? 1 : 0)) {
        set_reg_field(6, REG6_RX_CAL_MODE_EN, rx_cal ? REG6_RX_CAL_MODE_EN : 0);
        flush_dirty();
    }

    switch (mode) {
        default:
        case Mode::Shutdown:
            gpio_max2831_rx_enable.write(0); /* RXTX=0 */
            gpio_max283x_enable.write(0);    /* ENABLE=0 */
            set_rssi_mux(0);
            break;
        case Mode::Standby:
            gpio_max2831_rx_enable.write(1); /* RXTX=1 */
            gpio_max283x_enable.write(0);    /* ENABLE=0 */
            set_rssi_mux(0);
            break;
        case Mode::Transmit:
        case Mode::Tx_Calibration:
            gpio_max2831_rx_enable.write(1); /* RXTX=1 for TX */
            gpio_max283x_enable.write(1);    /* ENABLE=1 */
            set_rssi_mux(2);                 // transmit power
            break;
        case Mode::Receive:
        case Mode::Rx_Calibration:
            gpio_max2831_rx_enable.write(0); /* RXTX=0 for RX */
            gpio_max283x_enable.write(1);    /* ENABLE=1 */
            set_rssi_mux(1);                 // RSSI
            break;
    }

    /* Update LPF bandwidth for current mode */
    if (_desired_lpf_bw > 0) {
        set_lpf_bandwidth_internal(_desired_lpf_bw);
    }
}

void MAX2831::set_tx_vga_gain(const int_fast8_t db) {
    /* TX VGA gain: 0-31 dB in ~1 dB steps
     * Register value: gain * 2 | 1, max 0x3F
     * This matches GSG reference: max2831_set_txvga_gain() */
    int_fast8_t db_clipped = std::max(0, std::min(31, (int)db));
    uint16_t value = std::min((db_clipped << 1) | 1, 0x3f);
    set_reg_field(12, REG12_TXVGA_GAIN_MASK, value);
    flush_reg(12);
}

void MAX2831::set_lna_gain(const int_fast8_t db) {
    /*
     * LNA gain has 3 settings (from GSG reference):
     *   MAX (33 dB), -16 dB from max (17 dB), -33 dB from max (0 dB)
     * Map from MAX2837 8 dB steps for compatibility
     */
    uint16_t gain_val;
    if (db >= 32) {
        gain_val = REG11_LNA_GAIN_MAX;
    } else if (db >= 16) {
        gain_val = REG11_LNA_GAIN_M16;
    } else {
        gain_val = REG11_LNA_GAIN_M33;
    }
    set_reg_field(11, REG11_LNA_GAIN_MASK, gain_val);
    flush_reg(11);
}

void MAX2831::set_vga_gain(const int_fast8_t db) {
    /* VGA gain: 0-62 dB in 2 dB steps
     * This matches GSG reference: max2831_set_vga_gain() */
    if ((db & 0x1) || db > 62) {
        return; /* Invalid: must be even and <= 62 */
    }
    int_fast8_t db_clipped = std::max(0, std::min(62, (int)db));
    uint16_t value = (db_clipped >> 1) & 0x1f;
    set_reg_field(11, REG11_RXVGA_GAIN_MASK, value);
    flush_reg(11);
}

/*
 * LPF bandwidth tables from GSG reference max2831.c
 */
struct lpf_ft_t {
    uint32_t bandwidth_hz;
    uint8_t ft;
};

struct lpf_ft_fine_t {
    uint8_t percent;
    uint8_t ft_fine;
};

/* Measured -0.5 dB complex baseband bandwidth for each register setting */
static constexpr lpf_ft_t rx_lpf_ft[] = {
    {11600000, REG8_RX_LPF_7_5M},
    {15100000, REG8_RX_LPF_8_5M},
    {22600000, REG8_RX_LPF_15M},
    {28300000, REG8_RX_LPF_18M},
    {0, 0},
};

static constexpr lpf_ft_fine_t rx_lpf_ft_fine[] = {
    {90, REG7_RX_LPF_FINE_90},
    {95, REG7_RX_LPF_FINE_95},
    {100, REG7_RX_LPF_FINE_100},
    {105, REG7_RX_LPF_FINE_105},
    {110, REG7_RX_LPF_FINE_110},
    {0, 0},
};

static constexpr lpf_ft_t tx_lpf_ft[] = {
    {11900000, REG8_TX_LPF_8M},
    {15800000, REG8_TX_LPF_11M},
    {23600000, REG8_TX_LPF_16_5M},
    {31300000, REG8_TX_LPF_22_5M},
    {0, 0},
};

static constexpr lpf_ft_fine_t tx_lpf_ft_fine[] = {
    {90, REG7_TX_LPF_FINE_90},
    {95, REG7_TX_LPF_FINE_95},
    {100, REG7_TX_LPF_FINE_100},
    {105, REG7_TX_LPF_FINE_105},
    {110, REG7_TX_LPF_FINE_110},
    {115, REG7_TX_LPF_FINE_115},
    {0, 0},
};

uint32_t MAX2831::set_lpf_bandwidth_internal(const uint32_t bandwidth_hz) {
    const lpf_ft_t* coarse;
    const lpf_ft_fine_t* fine;

    if (_mode == Mode::Receive || _mode == Mode::Rx_Calibration) {
        coarse = rx_lpf_ft;
        fine = rx_lpf_ft_fine;
    } else {
        coarse = tx_lpf_ft;
        fine = tx_lpf_ft_fine;
    }

    /* Find coarse and fine settings for LPF - matches GSG reference */
    bool found = false;
    const lpf_ft_fine_t* f = fine;
    for (; coarse->bandwidth_hz != 0; coarse++) {
        uint32_t coarse_aux = coarse->bandwidth_hz / 100;
        for (f = fine; f->percent != 0; f++) {
            if ((coarse_aux * f->percent) >= bandwidth_hz) {
                found = true;
                break;
            }
        }
        if (found) break;
    }

    /* Use the widest setting if a wider bandwidth than our maximum is requested */
    if (!found) {
        coarse--;
        f--;
    }

    /* Program found settings */
    set_reg_field(8, REG8_LPF_COARSE_MASK, coarse->ft);
    if (_mode == Mode::Receive || _mode == Mode::Rx_Calibration) {
        set_reg_field(7, REG7_RX_LPF_FINE_MASK, f->ft_fine);
    } else {
        /* TX fine values are already shifted in the constants (REG7_TX_LPF_FINE_*) */
        set_reg_field(7, REG7_TX_LPF_FINE_MASK, f->ft_fine);
    }
    flush_dirty();

    return coarse->bandwidth_hz * f->percent / 100;
}

void MAX2831::set_lpf_rf_bandwidth_rx(const uint32_t bandwidth_minimum) {
    _desired_lpf_bw = bandwidth_minimum;
#ifdef PRALINE
    uint32_t actual_bw = bandwidth_minimum;

    _desired_lpf_bw = actual_bw;
    if (_mode == Mode::Receive || _mode == Mode::Rx_Calibration) {
        set_lpf_bandwidth_internal(actual_bw);
    }
#else
    if (_mode == Mode::Receive || _mode == Mode::Rx_Calibration) {
        set_lpf_bandwidth_internal(bandwidth_minimum);
    }
#endif
}

void MAX2831::set_lpf_rf_bandwidth_tx(const uint32_t bandwidth_minimum) {
    _desired_lpf_bw = bandwidth_minimum;
    if (_mode == Mode::Transmit || _mode == Mode::Tx_Calibration) {
        set_lpf_bandwidth_internal(bandwidth_minimum);
    }
}

bool MAX2831::set_frequency(const rf::Frequency lo_frequency) {
    /*
     * MAX2831 frequency synthesis from GSG reference max2831_set_frequency():
     *   F_LO = F_REF * (N + F/2^20) / R
     * Where:
     *   F_REF = 40 MHz reference
     *   R = reference divider (1 or 2), we use R=2
     *   N = integer divider (8 bits)
     *   F = fractional divider (20 bits)
     *
     * Using R=2: F_LO = 40M * (N + F/2^20) / 2 = 20M * (N + F/2^20)
     */

    /* MAX2831 supports 2.3-2.6 GHz */
    // if (lo_frequency < MAX2831_MIN_LO_FREQUENCY_HZ || lo_frequency > MAX2831_MAX_LO_FREQUENCY_HZ) {
    //     return false;
    // }

    bool valid = (lo_frequency >= MAX2831_MIN_LO_FREQUENCY_HZ && lo_frequency <= MAX2831_MAX_LO_FREQUENCY_HZ);

    // TRACK REQUEST IMMEDIATELY
    max2831_info.requested_freq_mhz = lo_frequency / 1000000;
    max2831_info.set_frequency_called = true;
    max2831_info.frequency_valid = valid;

    if (!valid) {
        max2831_info.calculated_n = 0;
        max2831_info.calculated_frac = 0;
        return false;
    }

    /* From GSG reference: ASSUME 40MHz PLL. Ratio = F*R/40,000,000.
     * TODO: fixed to R=2. Check if it's worth exploring R=1. */
    uint32_t freq = lo_frequency;
    freq += (20000000 >> 21); /* Round to nearest frequency */
    uint32_t div_int = freq / 20000000;
    uint32_t div_rem = freq % 20000000;
    uint32_t div_frac = 0;
    uint32_t div_cmp = 20000000;

    for (int i = 0; i < 20; i++) {
        div_frac <<= 1;
        div_rem <<= 1;
        if (div_rem >= div_cmp) {
            div_frac |= 0x1;
            div_rem -= div_cmp;
        }
    }

    // TRACK CALCULATED VALUES
    max2831_info.calculated_n = div_int;
    max2831_info.calculated_frac = div_frac;

    /* Write order matters - matches GSG reference */
    /* REG 3: SYN_INT (bits 7:0) and SYN_FRAC_LO (bits 13:8) */
    uint16_t reg3_val = (div_int & 0xFF) | ((div_frac & 0x3F) << 8);
    _regs[3] = reg3_val;
    mark_dirty(3);

    /* REG 4: SYN_FRAC_HI (bits 13:0) - upper 14 bits of 20-bit fractional */
    uint16_t reg4_val = (div_frac >> 6) & 0x3FFF;
    _regs[4] = reg4_val;
    mark_dirty(4);

    flush_dirty();

    return true;
}

void MAX2831::set_rx_LO_iq_phase_calibration(const size_t v) {
    /* MAX2831 doesn't have the same IQ calibration as MAX2837 */
    (void)v;
}

void MAX2831::set_tx_LO_iq_phase_calibration(const size_t v) {
    /* MAX2831 doesn't have the same IQ calibration as MAX2837 */
    (void)v;
}

void MAX2831::set_rx_buff_vcm(const size_t v) {
    /* MAX2831 RX IQ common mode voltage is in register 15
     * Values: 0=1.1V, 1=1.2V, 2=1.3V, 3=1.45V */
    uint16_t vcm = std::min(v, (size_t)3) << REG15_RXIQ_VCM_SHIFT;
    set_reg_field(15, REG15_RXIQ_VCM_MASK, vcm);
    flush_reg(15);
}

int8_t MAX2831::temp_sense() {
    /* MAX2831 temperature sensor can be read via RSSI MUX.
     * This would require:
     * 1. Switch RSSI_MUX to temperature mode
     * 2. Read the ADC
     * 3. Switch back to RSSI mode
     * For now, return a placeholder value. */
    return 25; /* Room temperature placeholder */
}

reg_t MAX2831::read(const address_t reg_num) {
    /* MAX2831 doesn't support SPI read, return cached value */
    if (reg_num < reg_count) {
        return _regs[reg_num];
    }
    return 0;
}

void MAX2831::write(const address_t reg_num, const reg_t value) {
    if (reg_num < reg_count) {
        _regs[reg_num] = value & 0x3FFF; /* 14-bit registers */
        write_reg(reg_num, _regs[reg_num]);
        mark_clean(reg_num);
    }
}

void MAX2831::set_rssi_mux(const uint8_t mode) {
    /* RSSI MUX allows switching the RSSI output between different internal signals.
     * 0 = disable mux
     * 1 = RSSI
     * 2 = TX_POWER
     * 3 = TEMP
     */
    uint16_t mux_val = 0;

    // Select the appropriate constant based on the input mode.
    if (mode == 0) {
        mux_val = 0;
    } else {
        // Select the appropriate constant based on the input mode.
        switch (mode) {
            case 3:
                mux_val = REG8_RSSI_MUX_TEMP;
                break;
            case 2:
                mux_val = REG8_RSSI_MUX_TX_POWER;
                break;
            case 1:
            default:
                mux_val = REG8_RSSI_MUX_RSSI;
                break;
        }
        mux_val |= REG8_RSSI_EN;
    }

    set_reg_field(8, REG8_RSSI_MUX_MASK | REG8_RSSI_EN, mux_val);
    flush_reg(8);
}

}  // namespace max2831
#endif
