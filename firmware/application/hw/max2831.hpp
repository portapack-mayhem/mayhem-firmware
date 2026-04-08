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
 * MAX2831 driver ported from GSG HackRF reference implementation.
 * Register definitions match max2831_regs.def from hackrf firmware.
 */

#ifndef __MAX2831_H__
#define __MAX2831_H__

#include "max283x.hpp"
#include "gpio.hpp"
#include "spi_arbiter.hpp"

#include <cstdint>
#include <array>

namespace max2831 {

using namespace max283x;

// Global debug tracking for MAX2831
struct MAX2831Info {
    uint32_t requested_freq_mhz;
    uint32_t calculated_n;
    uint32_t calculated_frac;
    bool set_frequency_called;
    bool frequency_valid;
};

MAX2831Info get_max2831_info();

/* minumum and maximum lo_frequencies supported by max2831 */
constexpr rf::Frequency MAX2831_MIN_LO_FREQUENCY_HZ = 2300000000LL;
constexpr rf::Frequency MAX2831_MAX_LO_FREQUENCY_HZ = 2600000000LL;

/* MAX2831 has 16 registers, each containing 14 bits of data */
constexpr size_t reg_count = 16;

/* Default register values from GSG HackRF reference (max2831.c) */
constexpr std::array<uint16_t, reg_count> default_regs = {
    0x1740, /* 0: enable fractional mode (Table 16 recommends 0x0740, clearing unknown bit) */
    0x119a, /* 1 */
    0x1003, /* 2 */
    0x0079, /* 3: PLL divider settings for 2437 MHz */
    0x3666, /* 4: PLL divider settings for 2437 MHz */
    0x00a4, /* 5: divide reference frequency by 2 */
    0x0060, /* 6: enable TX power detector */
    0x1022, /* 7: 110% TX LPF bandwidth */
    0x2021, /* 8: pin control of RX gain, 11 MHz LPF bandwidth */
    0x03b5, /* 9: pin control of TX gain */
    0x1d80, /* 10: 3.5 us PA enable delay, zero PA bias */
    0x0074, /* 11: LNA high gain, RX VGA moderate gain (Table 27 recommends 0x007f, maximum gain) */
    0x0140, /* 12: TX VGA minimum */
    0x0e92, /* 13 */
    0x0100, /* 14: reference clock output disabled */
    0x0145, /* 15: RX IQ common mode 1.1 V */
};

/*
 * Register bit field definitions from max2831_regs.def
 * Format: REG<num>_<field>_<info>
 */

/* REG 0: PLL Mode */
constexpr uint16_t REG0_PLL_MODE_SHIFT = 10;
constexpr uint16_t REG0_PLL_MODE_MASK = (1 << REG0_PLL_MODE_SHIFT);
constexpr uint16_t REG0_PLL_MODE_INTEGER = 0;
constexpr uint16_t REG0_PLL_MODE_FRACTIONAL = 1;

/* REG 3: Synthesizer Integer and Fractional Low */
constexpr uint16_t REG3_SYN_INT_SHIFT = 0;
constexpr uint16_t REG3_SYN_INT_MASK = 0x00FF; /* D7:D0 - Integer divider (8 bits) */
constexpr uint16_t REG3_SYN_FRAC_LO_SHIFT = 8;
constexpr uint16_t REG3_SYN_FRAC_LO_MASK = 0x3F00; /* D13:D8 - Low 6 bits of fractional divider */

/* REG 4: Synthesizer Fractional High */
constexpr uint16_t REG4_SYN_FRAC_HI_MASK = 0x3FFF; /* D13:D0 - High 14 bits of fractional divider */

/* REG 5: Reference Divider and Lock Detect */
constexpr uint16_t REG5_SYN_REF_DIV_SHIFT = 2;
constexpr uint16_t REG5_SYN_REF_DIV_1 = (0 << REG5_SYN_REF_DIV_SHIFT);
constexpr uint16_t REG5_SYN_REF_DIV_2 = (1 << REG5_SYN_REF_DIV_SHIFT);

/* REG 6: Calibration Mode */
constexpr uint16_t REG6_RX_CAL_MODE_EN_SHIFT = 0;
constexpr uint16_t REG6_RX_CAL_MODE_EN = (1 << REG6_RX_CAL_MODE_EN_SHIFT);
constexpr uint16_t REG6_TX_CAL_MODE_EN_SHIFT = 1;
constexpr uint16_t REG6_TX_CAL_MODE_EN = (1 << REG6_TX_CAL_MODE_EN_SHIFT);
constexpr uint16_t REG6_TX_POWER_DETECT_EN_SHIFT = 6;
constexpr uint16_t REG6_TX_POWER_DETECT_EN = (1 << REG6_TX_POWER_DETECT_EN_SHIFT);

/* REG 7: LPF Fine Adjustment and RX HPF */
constexpr uint16_t REG7_RX_LPF_FINE_SHIFT = 0;
constexpr uint16_t REG7_RX_LPF_FINE_MASK = 0x0007; /* D2:D0 */
constexpr uint16_t REG7_RX_LPF_FINE_90 = 0;
constexpr uint16_t REG7_RX_LPF_FINE_95 = 1;
constexpr uint16_t REG7_RX_LPF_FINE_100 = 2;
constexpr uint16_t REG7_RX_LPF_FINE_105 = 3;
constexpr uint16_t REG7_RX_LPF_FINE_110 = 4;

constexpr uint16_t REG7_TX_LPF_FINE_SHIFT = 3;
constexpr uint16_t REG7_TX_LPF_FINE_MASK = 0x0038; /* D5:D3 */
constexpr uint16_t REG7_TX_LPF_FINE_90 = (0 << REG7_TX_LPF_FINE_SHIFT);
constexpr uint16_t REG7_TX_LPF_FINE_95 = (1 << REG7_TX_LPF_FINE_SHIFT);
constexpr uint16_t REG7_TX_LPF_FINE_100 = (2 << REG7_TX_LPF_FINE_SHIFT);
constexpr uint16_t REG7_TX_LPF_FINE_105 = (3 << REG7_TX_LPF_FINE_SHIFT);
constexpr uint16_t REG7_TX_LPF_FINE_110 = (4 << REG7_TX_LPF_FINE_SHIFT);
constexpr uint16_t REG7_TX_LPF_FINE_115 = (5 << REG7_TX_LPF_FINE_SHIFT);

constexpr uint16_t REG7_RX_HPF_SEL_SHIFT = 12;
constexpr uint16_t REG7_RX_HPF_SEL_MASK = 0x3000; /* D13:D12 */
constexpr uint16_t REG7_RX_HPF_100HZ = (0 << REG7_RX_HPF_SEL_SHIFT);
constexpr uint16_t REG7_RX_HPF_4KHZ = (1 << REG7_RX_HPF_SEL_SHIFT);
constexpr uint16_t REG7_RX_HPF_30KHZ = (2 << REG7_RX_HPF_SEL_SHIFT);

/* REG 8: LPF Coarse, RSSI MUX, and RX VGA SPI Enable */
constexpr uint16_t REG8_LPF_COARSE_SHIFT = 0;
constexpr uint16_t REG8_LPF_COARSE_MASK = 0x0003; /* D1:D0 */
/* RX and TX share the same coarse LPF setting bits */
constexpr uint16_t REG8_RX_LPF_7_5M = 0;
constexpr uint16_t REG8_RX_LPF_8_5M = 1;
constexpr uint16_t REG8_RX_LPF_15M = 2;
constexpr uint16_t REG8_RX_LPF_18M = 3;
constexpr uint16_t REG8_TX_LPF_8M = 0;
constexpr uint16_t REG8_TX_LPF_11M = 1;
constexpr uint16_t REG8_TX_LPF_16_5M = 2;
constexpr uint16_t REG8_TX_LPF_22_5M = 3;

constexpr uint16_t REG8_RSSI_MUX_SHIFT = 8;
constexpr uint16_t REG8_RSSI_MUX_MASK = 0x0300; /* D9:D8 */
constexpr uint16_t REG8_RSSI_MUX_RSSI = (0 << REG8_RSSI_MUX_SHIFT);
constexpr uint16_t REG8_RSSI_MUX_TEMP = (1 << REG8_RSSI_MUX_SHIFT);
constexpr uint16_t REG8_RSSI_MUX_TX_POWER = (2 << REG8_RSSI_MUX_SHIFT);
constexpr uint16_t REG8_RSSI_EN = (1 << 10);

constexpr uint16_t REG8_RXVGA_GAIN_SPI_EN_SHIFT = 12;
constexpr uint16_t REG8_RXVGA_GAIN_SPI_EN = (1 << REG8_RXVGA_GAIN_SPI_EN_SHIFT);

/* REG 9: TX VGA SPI Enable */
constexpr uint16_t REG9_TXVGA_GAIN_SPI_EN_SHIFT = 10;
constexpr uint16_t REG9_TXVGA_GAIN_SPI_EN = (1 << REG9_TXVGA_GAIN_SPI_EN_SHIFT);

/* REG 11: RX Gain */
constexpr uint16_t REG11_RXVGA_GAIN_SHIFT = 0;
constexpr uint16_t REG11_RXVGA_GAIN_MASK = 0x001F; /* D4:D0 - 5 bits */

constexpr uint16_t REG11_LNA_GAIN_SHIFT = 5;
constexpr uint16_t REG11_LNA_GAIN_MASK = 0x0060;                     /* D6:D5 - 2 bits */
constexpr uint16_t REG11_LNA_GAIN_M33 = (0 << REG11_LNA_GAIN_SHIFT); /* -33 dB from max (min) */
constexpr uint16_t REG11_LNA_GAIN_M16 = (2 << REG11_LNA_GAIN_SHIFT); /* -16 dB from max */
constexpr uint16_t REG11_LNA_GAIN_MAX = (3 << REG11_LNA_GAIN_SHIFT); /* Maximum LNA gain */

/* REG 12: TX VGA Gain */
constexpr uint16_t REG12_TXVGA_GAIN_SHIFT = 0;
constexpr uint16_t REG12_TXVGA_GAIN_MASK = 0x003F; /* D5:D0 - 6 bits */

/* REG 14: Clock Output */
constexpr uint16_t REG14_CLKOUT_PIN_EN_SHIFT = 9;
constexpr uint16_t REG14_CLKOUT_PIN_EN = (1 << REG14_CLKOUT_PIN_EN_SHIFT);

/* REG 15: RX IQ Common Mode */
constexpr uint16_t REG15_RXIQ_VCM_SHIFT = 10;
constexpr uint16_t REG15_RXIQ_VCM_MASK = 0x0C00;                      /* D11:D10 - 2 bits */
constexpr uint16_t REG15_RXIQ_VCM_1_1 = (0 << REG15_RXIQ_VCM_SHIFT);  /* 1.1V */
constexpr uint16_t REG15_RXIQ_VCM_1_2 = (1 << REG15_RXIQ_VCM_SHIFT);  /* 1.2V */
constexpr uint16_t REG15_RXIQ_VCM_1_3 = (2 << REG15_RXIQ_VCM_SHIFT);  /* 1.3V */
constexpr uint16_t REG15_RXIQ_VCM_1_45 = (3 << REG15_RXIQ_VCM_SHIFT); /* 1.45V */

class MAX2831 : public MAX283x {
   public:
    constexpr MAX2831(
        spi::arbiter::Target& target)
        : _target(target) {
    }

    void init() override;
    void set_mode(const Mode mode) override;

    void set_tx_vga_gain(const int_fast8_t db) override;
    void set_lna_gain(const int_fast8_t db) override;
    void set_vga_gain(const int_fast8_t db) override;
    void set_lpf_rf_bandwidth_rx(const uint32_t bandwidth_minimum) override;
    void set_lpf_rf_bandwidth_tx(const uint32_t bandwidth_minimum) override;

    bool set_frequency(const rf::Frequency lo_frequency) override;

    void set_rx_LO_iq_phase_calibration(const size_t v) override;
    void set_tx_LO_iq_phase_calibration(const size_t v) override;

    void set_rx_buff_vcm(const size_t v) override;

    int8_t temp_sense() override;

    reg_t read(const address_t reg_num) override;
    void write(const address_t reg_num, const reg_t value) override;

    void set_rssi_mux(const uint8_t mode);

   private:
    spi::arbiter::Target& _target;
    Mode _mode{Mode::Standby};
    std::array<uint16_t, reg_count> _regs{default_regs};
    uint16_t _regs_dirty{0xFFFF}; /* Track which registers need to be written */
    uint32_t _desired_lpf_bw{0};  /* Desired LPF bandwidth in Hz */

    void write_reg(const uint8_t reg, const uint16_t value);
    void set_reg_field(const uint8_t reg, const uint16_t mask, const uint16_t value);
    uint16_t get_reg_field(const uint8_t reg, const uint16_t mask, const uint8_t shift);
    void mark_dirty(const uint8_t reg);
    void mark_clean(const uint8_t reg);
    void flush_reg(const uint8_t reg);
    void flush_dirty();
    uint32_t set_lpf_bandwidth_internal(const uint32_t bandwidth_hz);
};

}  // namespace max2831

#endif /*__MAX2831_H__*/
