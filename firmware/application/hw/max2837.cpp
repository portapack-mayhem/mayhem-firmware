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

#include "max2837.hpp"

#include "hackrf_hal.hpp"
#include "hackrf_gpio.hpp"
using namespace hackrf::one;

#include "ch.h"
#include "hal.h"

#include <algorithm>

namespace max2837 {

using namespace max283x;

namespace lna {

using namespace max283x::lna;

constexpr std::array<uint8_t, 8> lookup_8db_steps{
    0b111, 0b011, 0b110, 0b010,
    0b100, 0b000, 0b000, 0b000};

static uint_fast8_t gain_ordinal(const int8_t db) {
    const auto db_sat = gain_db_range.clip(db);
    return lna::lookup_8db_steps[(db_sat >> 3) & 7];
}

} /* namespace lna */

namespace vga {

using namespace max283x::vga;

static uint_fast8_t gain_ordinal(const int8_t db) {
    const auto db_sat = gain_db_range.clip(db);
    return ((db_sat >> 1) & 0b11111) ^ 0b11111;
}

} /* namespace vga */

namespace tx {

using namespace max283x::tx;

static uint_fast8_t gain_ordinal(const int8_t db) {
    const auto db_sat = gain_db_range.clip(db);
    uint8_t value = db_sat & 0x0f;
    value = (db_sat >= 16) ? (value | 0x20) : value;
    value = (db_sat >= 32) ? (value | 0x10) : value;
    return (value & 0b111111) ^ 0b111111;
}

} /* namespace tx */

namespace filter {

using namespace max283x::filter;

static uint_fast8_t bandwidth_ordinal(const uint32_t bandwidth) {
    /* Determine filter setting that will provide bandwidth greater than or
     * equal to requested bandwidth.
     */
    return std::lower_bound(bandwidths.cbegin(), bandwidths.cend(), bandwidth) - bandwidths.cbegin();
}

} /* namespace filter */

/* Empirical testing indicates about 25us is necessary to get a valid
 * temperature sense conversion from the ADC.
 */
constexpr float seconds_for_temperature_sense_adc_conversion = 30.0e-6;
constexpr halrtcnt_t ticks_for_temperature_sense_adc_conversion = (base_m4_clk_f * seconds_for_temperature_sense_adc_conversion + 1);

constexpr uint32_t reference_frequency = max283x_reference_f;
constexpr uint32_t pll_factor = 1.0 / (4.0 / 3.0 / reference_frequency) + 0.5;

void MAX2837::init() {
    set_mode(Mode::Shutdown);

    gpio_max283x_enable.output();
    gpio_max2837_rxenable.output();
    gpio_max2837_txenable.output();

    _map.r.tx_gain.TXVGA_GAIN_SPI_EN = 1;
    _map.r.tx_gain.TXVGA_GAIN_MSB_SPI_EN = 1;
    _map.r.tx_gain.TXVGA_GAIN_SPI = 0x00;

    _map.r.lpf_3_vga_1.VGAMUX_enable = 1;
    _map.r.lpf_3_vga_1.VGA_EN = 1;

    _map.r.hpfsm_3.HPC_STOP = 1; /* 1kHz */

    _map.r.rx_top_rx_bias.LNAgain_SPI_EN = 1; /* control LNA gain from SPI */
    _map.r.rxrf_2.L = 0b000;

    _map.r.rx_top_rx_bias.VGAgain_SPI_EN = 1; /* control VGA gain from SPI */
    _map.r.vga_2.VGA = 0b01010;

    _map.r.lpf_3_vga_1.BUFF_VCM = 0b00; /* TODO: Check values out of ADC */

    _map.r.lpf_1.LPF_EN = 1;      /* Enable low-pass filter */
    _map.r.lpf_1.ModeCtrl = 0b01; /* Rx LPF */
    _map.r.lpf_1.FT = 0b0000;     /* 1,75MHz LPF */

    _map.r.spi_en.EN_SPI = 1; /* enable chip functions when ENABLE pin set */

    _map.r.lo_gen.LOGEN_2GM = 0;

#if 0
	_map.r.rxrf_1.LNA_EN = 1;
	_map.r.rxrf_1.Mixer_EN = 1;
	_map.r.rxrf_1.RxLO_EN = 1;

	_map.r.rx_top.DOUT_DRVH = 0;			/* slow down DOUT edges */

	_map.r.hpfsm_4.DOUT_CSB_SEL	= 0;		/* DOUT not tri-stated, is independent of CSB */

	_map.r.xtal_cfg.XTAL_CLKOUT_EN = 0;		/* CLKOUT pin disabled. (Seems to have no effect.) */
#endif

    _map.r.vga_3_rx_top.RSSI_EN_SPIenables = 1;
    _map.r.vga_3_rx_top.RSSI_MODE = 1; /* RSSI independent of RXHP */

    _dirty.set();
    flush();

    set_mode(Mode::Standby);
}

void MAX2837::set_tx_LO_iq_phase_calibration(const size_t v) {
    /*  IQ phase deg CAL adj  (+4 ...-4)  in 32 steps (5 bits), 00000 = +4deg (Q lags I by 94degs, default), 01111 = +0deg, 11111 = -4deg (Q lags I by 86degs) */

    // TX calibration , Logic pins , ENABLE, RXENABLE, TXENABLE = 1,0,1 (5dec), and  Reg address 16, D1 (CAL mode 1):DO (CHIP ENABLE 1)
    set_mode(Mode::Tx_Calibration);  // write to ram 3 LOGIC Pins .

    gpio_max283x_enable.output();
    gpio_max2837_rxenable.output();
    gpio_max2837_txenable.output();

    _map.r.spi_en.CAL_SPI = 1;  // Register Settings reg address 16,  D1 (CAL mode 1)
    _map.r.spi_en.EN_SPI = 1;   // Register Settings reg address 16,  DO (CHIP ENABLE 1)
    flush_one(Register::SPI_EN);

    _map.r.tx_lo_iq.TXLO_IQ_SPI_EN = 1;  // reg 30 D5, TX LO I/Q Phase SPI Adjust. Active when Address 30 D5 (TXLO_IQ_SPI_EN) = 1.
    _map.r.tx_lo_iq.TXLO_IQ_SPI = v;     // reg 30  D4:D0, TX LO I/Q Phase SPI Adjust.
    flush_one(Register::TX_LO_IQ);

    // Exit Calibration mode,  Go back to reg 16, D1:D0 , Out of CALIBRATION , back to default conditions, but keep CS activated.
    _map.r.spi_en.CAL_SPI = 0;  // Register Settings reg address 16,  D1 (0 = Normal operation (default)
    _map.r.spi_en.EN_SPI = 1;   // Register Settings reg address 16,  DO (1 = Chip select enable )
    flush_one(Register::SPI_EN);

    set_mode(Mode::Standby);  // Back 3  logic pins CALIBRATION mode -> Standby.
}

enum class Mask {  // There are class Mask ,and class mode with same names, but they are not same.
    Enable = 0b001,
    RxEnable = 0b010,
    TxEnable = 0b100,
    Shutdown = 0b000,
    Standby = Enable,
    Receive = Enable | RxEnable,
    Transmit = Enable | TxEnable,
    Rx_calibration = Enable | RxEnable,  // sets the same 3 x logic pins to the Receive operating mode.
    Tx_calibration = Enable | TxEnable,  // sets the same 3 x logic pins to the Transmit operating mode.
};

Mask mode_mask(const Mode mode) {  // based on enum Mode cases, we set up the correct  3 logic PINS .
    switch (mode) {
        case Mode::Standby:
            return Mask::Standby;
        case Mode::Receive:
            return Mask::Receive;
        case Mode::Transmit:
            return Mask::Transmit;
        case Mode::Rx_Calibration:        // Let's add those two CAL logic pin settings- Rx and Tx calibration modes.
            return Mask::Rx_calibration;  // same logic pins as Receive mode = Enable | RxEnable, (the difference is in Reg add 16 D1:DO)
        case Mode::Tx_Calibration:        // Let's add this CAL Tx calibration mode = Transmit.
            return Mask::Tx_calibration;  // same logic pins as Transmit = Enable | TxEnable,(the difference is in Reg add 16 D1:DO)
        default:
            return Mask::Shutdown;
    }
}

void MAX2837::set_mode(const Mode mode) {  // We set up the 3 Logic Pins ENABLE, RXENABLE, TXENABLE accordingly to the max2837 mode case,  that we want to set up .
    Mask mask = mode_mask(mode);
    gpio_max283x_enable.write(toUType(mask) & toUType(Mask::Enable));
    gpio_max2837_rxenable.write(toUType(mask) & toUType(Mask::RxEnable));
    gpio_max2837_txenable.write(toUType(mask) & toUType(Mask::TxEnable));
}

void MAX2837::flush() {
    if (_dirty) {
        for (size_t n = 0; n < reg_count; n++) {
            if (_dirty[n]) {
                write(n, _map.w[n]);
            }
        }
        _dirty.clear();
    }
}

void MAX2837::flush_one(const Register reg) {
    const auto reg_num = toUType(reg);
    write(reg_num, _map.w[reg_num]);
    _dirty.clear(reg_num);
}

void MAX2837::write(const address_t reg_num, const reg_t value) {
    uint16_t t = (0U << 15) | (reg_num << 10) | (value & 0x3ffU);
    _target.transfer(&t, 1);
}

reg_t MAX2837::read(const address_t reg_num) {
    uint16_t t = (1U << 15) | (reg_num << 10);
    _target.transfer(&t, 1U);
    return t & 0x3ffU;
}

void MAX2837::write(const Register reg, const reg_t value) {
    write(toUType(reg), value);
}

reg_t MAX2837::read(const Register reg) {
    return read(toUType(reg));
}

void MAX2837::set_tx_vga_gain(const int_fast8_t db) {
    _map.r.tx_gain.TXVGA_GAIN_SPI = tx::gain_ordinal(db);
    _dirty[Register::TX_GAIN] = 1;
    flush();
}

void MAX2837::set_lna_gain(const int_fast8_t db) {
    _map.r.rxrf_2.L = lna::gain_ordinal(db);
    _dirty[Register::RXRF_2] = 1;
    flush();
}

void MAX2837::set_vga_gain(const int_fast8_t db) {
    _map.r.vga_2.VGA = vga::gain_ordinal(db);
    _dirty[Register::VGA_2] = 1;
    flush();
}

void MAX2837::set_lpf_rf_bandwidth_rx(const uint32_t bandwidth_minimum) {
    _map.r.lpf_1.ModeCtrl = 0b01; /* Address reg 2, D3-D2, Set mode lowpass filter block to Rx LPF . Active when Address 6 D<9> = 1 */
    _map.r.lpf_1.FT = filter::bandwidth_ordinal(bandwidth_minimum);
    flush_one(Register::LPF_1);

    _map.r.vga_3_rx_top.LPF_MODE_SEL = 1; /* Address 6 reg, D9 bit:LPF mode mux, LPF_MODE_SEL 0 = Normal operation, 1 = Operating mode is programmed Address 2 D3:D2*/
    flush_one(Register::VGA_3_RX_TOP);

    _map.r.vga_3_rx_top.LPF_MODE_SEL = 0; /* Leave LPF_MODE_SEL 0 = Normal operation */
    flush_one(Register::VGA_3_RX_TOP);
}

void MAX2837::set_lpf_rf_bandwidth_tx(const uint32_t bandwidth_minimum) {
    _map.r.lpf_1.ModeCtrl = 0b10; /* Address 2 reg, D3-D2, Set mode lowpass filter block to Tx LPF . Active when Address 6 D<9> = 1 */
    _map.r.lpf_1.FT = filter::bandwidth_ordinal(bandwidth_minimum);
    flush_one(Register::LPF_1);

    _map.r.vga_3_rx_top.LPF_MODE_SEL = 1; /* Address 6 reg, D9 bit:LPF mode mux, LPF_MODE_SEL 0 = Normal operation, 1 = Operating mode is programmed Address 2 D3:D2*/
    flush_one(Register::VGA_3_RX_TOP);

    _map.r.vga_3_rx_top.LPF_MODE_SEL = 0; /* Leave LPF_MODE_SEL 0 = Normal operation */
    flush_one(Register::VGA_3_RX_TOP);
}

bool MAX2837::set_frequency(const rf::Frequency lo_frequency) {
    /* TODO: This is a sad implementation. Refactor. */
    if (lo::band[0].contains(lo_frequency)) {
        _map.r.syn_int_div.LOGEN_BSW = 0b00; /* 2300 - 2399.99MHz */
        _map.r.rxrf_1.LNAband = 0;           /* 2.3 - 2.5GHz */
    } else if (lo::band[1].contains(lo_frequency)) {
        _map.r.syn_int_div.LOGEN_BSW = 0b01; /* 2400 - 2499.99MHz */
        _map.r.rxrf_1.LNAband = 0;           /* 2.3 - 2.5GHz */
    } else if (lo::band[2].contains(lo_frequency)) {
        _map.r.syn_int_div.LOGEN_BSW = 0b10; /* 2500 - 2599.99MHz */
        _map.r.rxrf_1.LNAband = 1;           /* 2.5 - 2.7GHz */
    } else if (lo::band[3].contains(lo_frequency)) {
        _map.r.syn_int_div.LOGEN_BSW = 0b11; /* 2600 - 2700Hz */
        _map.r.rxrf_1.LNAband = 1;           /* 2.5 - 2.7GHz */
    } else {
        return false;
    }
    _dirty[Register::SYN_INT_DIV] = 1;
    _dirty[Register::RXRF_1] = 1;

    const uint64_t div_q20 = (lo_frequency * (1 << 20)) / pll_factor;

    _map.r.syn_int_div.SYN_INTDIV = div_q20 >> 20;
    _dirty[Register::SYN_INT_DIV] = 1;
    _map.r.syn_fr_div_2.SYN_FRDIV_19_10 = (div_q20 >> 10) & 0x3ff;
    _dirty[Register::SYN_FR_DIV_2] = 1;
    /* flush to commit high FRDIV first, as low FRDIV commits the change */
    flush();

    _map.r.syn_fr_div_1.SYN_FRDIV_9_0 = (div_q20 & 0x3ff);
    _dirty[Register::SYN_FR_DIV_1] = 1;
    flush();

    return true;
}

void MAX2837::set_rx_lo_iq_calibration(const size_t v) {
    _map.r.rx_top_rx_bias.RX_IQERR_SPI_EN = 1;
    _dirty[Register::RX_TOP_RX_BIAS] = 1;
    _map.r.rxrf_2.iqerr_trim = v;
    _dirty[Register::RXRF_2] = 1;
    flush();
}

void MAX2837::set_rx_bias_trim(const size_t v) {
    _map.r.rx_top_rx_bias.EN_Bias_Trim = 1;
    _map.r.rx_top_rx_bias.BIAS_TRIM_SPI = v;
    _dirty[Register::RX_TOP_RX_BIAS] = 1;
    flush();
}

void MAX2837::set_vco_bias(const size_t v) {
    _map.r.vco_cfg.VCO_BIAS_SPI_EN = 1;
    _map.r.vco_cfg.VCO_BIAS_SPI = v;
    _dirty[Register::VCO_CFG] = 1;
    flush();
}

void MAX2837::set_rx_buff_vcm(const size_t v) {
    _map.r.lpf_3_vga_1.BUFF_VCM = v;
    _dirty[Register::LPF_3_VGA_1] = 1;
    flush();
}

int8_t MAX2837::temp_sense() {
    if (!_map.r.rx_top.ts_en) {
        _map.r.rx_top.ts_en = 1;
        flush_one(Register::RX_TOP);

        chThdSleepMilliseconds(1);
    }

    _map.r.rx_top.ts_adc_trigger = 1;
    flush_one(Register::RX_TOP);

    halPolledDelay(ticks_for_temperature_sense_adc_conversion);

    /*
     * Conversion to degrees C determined by testing - does not match data sheet.
     */
    reg_t value = read(Register::TEMP_SENSE) & 0x1F;

    _map.r.rx_top.ts_adc_trigger = 0;
    flush_one(Register::RX_TOP);

    return std::min(127, (int)(value * 4.31 - 40));  // reg value is 0 to 31; possible return range is -40 C to 127 C
}

}  // namespace max2837
