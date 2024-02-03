/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2023 Great Scott Gadgets
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

#include "max2839.hpp"

#include "hackrf_hal.hpp"
#include "hackrf_gpio.hpp"
using namespace hackrf::one;

#include "ch.h"
#include "hal.h"

#include <algorithm>

namespace max2839 {

namespace lna {

using namespace max283x::lna;

constexpr std::array<uint8_t, 8> lookup_8db_steps{
    0b11, 0b11, 0b10, 0b10,
    0b01, 0b00, 0b00, 0b00};

static uint_fast8_t gain_ordinal(const int8_t db) {
    const auto db_sat = gain_db_range.clip(db);
    return lna::lookup_8db_steps[(db_sat >> 3) & 7];
}

} /* namespace lna */

namespace vga {

using namespace max283x::vga;

constexpr range_t<int8_t> gain_db_range_internal{0, 63};

static uint_fast8_t gain_ordinal(const int8_t db) {
    const auto db_sat = gain_db_range_internal.clip(db);
    return (db_sat & 0b111111) ^ 0b111111;
}

} /* namespace vga */

namespace tx {

using namespace max283x::tx;

static uint_fast8_t gain_ordinal(const int8_t db) {
    const auto db_sat = gain_db_range.clip(db);
    return 47 - db_sat;
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

static int_fast8_t requested_rx_lna_gain = 0;
static int_fast8_t requested_rx_vga_gain = 0;

void MAX2839::init() {
    set_mode(Mode::Shutdown);

    gpio_max283x_enable.output();
    gpio_max2839_rxtx.output();

    _map.r.rxrf_1.MIMOmode = 1; /* enable RXINB */

    _map.r.pa_drv.TXVGA_GAIN_SPI_EN = 1;
    _map.r.tx_gain.TXVGA_GAIN_SPI = 0x00;

    _map.r.hpfsm_3.HPC_STOP = 1; /* 1kHz */

    _map.r.rxrf_2.LNAgain_SPI_EN = 1; /* control LNA gain from SPI */
    _map.r.lpf_vga_1.L = 0b000;
    _map.r.lpf_vga_2.L = 0b000;

    _map.r.rx_top_1.VGAgain_SPI_EN = 1; /* control VGA gain from SPI */
    _map.r.lpf_vga_1.VGA = 0b000000;
    _map.r.lpf_vga_2.VGA = 0b010101;

    _map.r.lpf_vga_2.BUFF_VCM = 0b11; /* maximum RX output common-mode voltage */

    _map.r.lpf_vga_1.ModeCtrl = 0b01; /* Rx LPF */
    _map.r.lpf.FT = 0b0000;           /* 1.75 MHz LPF */

    _map.r.spi_en.EN_SPI = 1; /* enable chip functions when ENABLE pin set */

    _map.r.lo_gen.LOGEN_2GM = 0;

    _map.r.rssi_vga.RSSI_MODE = 1; /* RSSI independent of RXHP */

    /*
     * There are two LNA band settings, but we only use one of them.
     * Switching to the other one doesn't make the overall spectrum any
     * flatter but adds a surprise step in the middle.
     */
    _map.r.rxrf_1.LNAband = 0; /* 2.3 - 2.5GHz */

    _dirty.set();
    flush();

    set_mode(Mode::Standby);
}

void MAX2839::set_tx_LO_iq_phase_calibration(const size_t v) {
    /* IQ phase deg CAL adj (+4 ...-4)  This IC  in 64 steps (6 bits), 000000 = +4deg (Q lags I by 94degs, default), 011111 = +0deg, 111111 = -4deg (Q lags I by 86degs) */

    // TX calibration , 2 x Logic pins , ENABLE, RXENABLE = 1,0, (2dec), and  Reg address 16, D1 (CAL mode 1):DO (CHIP ENABLE 1)
    set_mode(Mode::Tx_Calibration);  // write to ram 3 LOGIC Pins .

    gpio_max283x_enable.output();  // max2839 has only 2 x pins + regs to decide mode.
    gpio_max2839_rxtx.output();    // Here is combined rx & tx pin in one port.

    _map.r.spi_en.CAL_SPI = 1;  // Register Settings reg address 16,  D1 (CAL mode 1)
    _map.r.spi_en.EN_SPI = 1;   // Register Settings reg address 16,  DO (CHIP ENABLE 1)
    flush_one(Register::SPI_EN);

    _map.r.pa_drv.TXLO_IQ_SPI_EN = 1;  // reg 27 D6, TX LO I/Q Phase SPI Adjust. Active when Address 27 D6 (TXLO_IQ_SPI_EN) = 1.
    _map.r.pa_drv.TXLO_IQ_SPI = v;     // reg 27 D5:D0 6 bits, TX LO I/Q Phase SPI Adjust.
    flush_one(Register::PA_DRV);

    // Exit Calibration mode,  Go back to reg 16, D1:D0 , Out of CALIBRATION , back to default conditions, but keep CS activated.
    _map.r.spi_en.CAL_SPI = 0;  // Register Settings reg address 16,  D1 (0 = Normal operation (default)
    _map.r.spi_en.EN_SPI = 1;   // Register Settings reg address 16,  DO (1 = Chip select enable )
    flush_one(Register::SPI_EN);

    set_mode(Mode::Standby);  // Back 3  logic pins CALIBRATION mode -> Standby.
}

enum class Mask {
    Enable = 0b01,
    RxTx = 0b10,
    Shutdown = 0b00,
    Standby = RxTx,
    Receive = Enable | RxTx,
    Transmit = Enable,
    Rx_calibration = Enable | RxTx,  // sets the same 2 x logic pins to the Receive operating mode.
    Tx_calibration = Enable,         // sets the same 2 x logic pins to the Transmit operating mode.
};

Mask mode_mask(const Mode mode) {
    switch (mode) {
        case Mode::Standby:
            return Mask::Standby;
        case Mode::Receive:
            return Mask::Receive;
        case Mode::Transmit:
            return Mask::Transmit;
        case Mode::Rx_Calibration:        // Let's add this not used previously Rx calibration mode.
            return Mask::Rx_calibration;  // same logic pins as  Receive mode = Enable | RxTx, ,(the difference is in Regs )
        case Mode::Tx_Calibration:        // Let's add this not used previously Tx calibration mode.
            return Mask::Tx_calibration;  // same logic pins as  Transmit = Enable , (the difference is in Reg add 16 D1:DO)
        default:
            return Mask::Shutdown;
    }
}

void MAX2839::set_mode(const Mode mode) {
    Mask mask = mode_mask(mode);
    gpio_max283x_enable.write(toUType(mask) & toUType(Mask::Enable));
    gpio_max2839_rxtx.write(toUType(mask) & toUType(Mask::RxTx));
}

void MAX2839::flush() {
    if (_dirty) {
        for (size_t n = 0; n < reg_count; n++) {
            if (_dirty[n]) {
                write(n, _map.w[n]);
            }
        }
        _dirty.clear();
    }
}

void MAX2839::flush_one(const Register reg) {
    const auto reg_num = toUType(reg);
    write(reg_num, _map.w[reg_num]);
    _dirty.clear(reg_num);
}

void MAX2839::write(const address_t reg_num, const reg_t value) {
    uint16_t t = (0U << 15) | (reg_num << 10) | (value & 0x3ffU);
    _target.transfer(&t, 1);
}

reg_t MAX2839::read(const address_t reg_num) {
    uint16_t t = (1U << 15) | (reg_num << 10);
    _target.transfer(&t, 1U);
    return t & 0x3ffU;
}

void MAX2839::write(const Register reg, const reg_t value) {
    write(toUType(reg), value);
}

reg_t MAX2839::read(const Register reg) {
    return read(toUType(reg));
}

void MAX2839::set_tx_vga_gain(const int_fast8_t db) {
    _map.r.tx_gain.TXVGA_GAIN_SPI = tx::gain_ordinal(db);
    _dirty[Register::TX_GAIN] = 1;
    flush();
}

/*
 * MAX2839 gain rain ranges differ slightly from MAX2837's but are close
 * enough that it makes sense to emulate MAX2837 gain ranges for a consistent
 * user experience.
 */
void MAX2839::configure_rx_gain() {
    /* Apply MAX2837 restrictions to requested gain settings. */
    int_fast8_t lna_gain = lna::gain_db_range.clip(requested_rx_lna_gain);
    lna_gain &= 0x38;
    int_fast8_t vga_gain = vga::gain_db_range.clip(requested_rx_vga_gain);
    vga_gain &= 0x3e;

    /*
     * MAX2839 has lower full-scale RX output voltage than MAX2837, so we
     * adjust the VGA (baseband) gain to compensate.
     */
    vga_gain += 3;

    /*
     * If that adjustment puts VGA gain out of range, use LNA gain to
     * compensate.  MAX2839 VGA gain can be any number from 0 through 63.
     */
    if (vga_gain > 63) {
        if (lna_gain <= 32) {
            vga_gain -= 8;
            lna_gain += 8;
        } else {
            vga_gain = 63;
        }
    }

    /*
     * MAX2839 lacks max-24 dB (16 dB) and max-40 dB (0 dB) LNA gain
     * settings, so we use VGA gain to compensate.
     */
    if (lna_gain == 0) {
        lna_gain = 8;
        vga_gain = (vga_gain >= 8) ? vga_gain - 8 : 0;
    }
    if (lna_gain == 16) {
        if (vga_gain > 32) {
            vga_gain -= 8;
            lna_gain += 8;
        } else {
            vga_gain += 8;
            lna_gain -= 8;
        }
    }

    _map.r.lpf_vga_2.L = lna::gain_ordinal(lna_gain);
    _dirty[Register::RXRF_2] = 1;
    _map.r.lpf_vga_2.VGA = vga::gain_ordinal(vga_gain);
    _dirty[Register::LPF_VGA_2] = 1;
    flush();
}

void MAX2839::set_lna_gain(const int_fast8_t db) {
    requested_rx_lna_gain = db;
    configure_rx_gain();
}

void MAX2839::set_vga_gain(const int_fast8_t db) {
    requested_rx_vga_gain = db;
    configure_rx_gain();
}

void MAX2839::set_lpf_rf_bandwidth_rx(const uint32_t bandwidth_minimum) {
    _map.r.lpf_vga_1.ModeCtrl = 0b01; /* Address reg 5, D9-D8, Set mode lowpass filter block to Rx LPF . Active when Address 8 D<2> = 1 */
    flush_one(Register::LPF_VGA_1);

    _map.r.rx_top_1.LPF_MODE_SEL = 1; /* Address 8 reg, D2 bit:LPF mode mux, LPF_MODE_SEL 0 = Normal operation, 1 = Operating mode is programmed Address 5 D<9:8> */
    flush_one(Register::RX_TOP_1);

    _map.r.lpf.FT = filter::bandwidth_ordinal(bandwidth_minimum);
    flush_one(Register::LPF);

    _map.r.rx_top_1.LPF_MODE_SEL = 0; /* Address 8 reg, D2 bit:LPF mode mux, LPF_MODE_SEL 0 = Normal operation, 1 = Operating mode is programmed Address 5 D<9:8> */
    flush_one(Register::RX_TOP_1);    /* Leave LPF_MODE_SEL 0 = Normal operation */
}

void MAX2839::set_lpf_rf_bandwidth_tx(const uint32_t bandwidth_minimum) {
    _map.r.lpf_vga_1.ModeCtrl = 0b10; /* Address reg 5, D9-D8, Set mode lowpass filter block to Tx LPF . Active when Address 8 D<2> = 1 */
    flush_one(Register::LPF_VGA_1);

    _map.r.rx_top_1.LPF_MODE_SEL = 1; /* Address 8 reg, D2 bit:LPF mode mux, LPF_MODE_SEL 0 = Normal operation, 1 = Operating mode is programmed Address 5 D<9:8> */
    flush_one(Register::RX_TOP_1);

    _map.r.lpf.FT = filter::bandwidth_ordinal(bandwidth_minimum);
    flush_one(Register::LPF);

    _map.r.rx_top_1.LPF_MODE_SEL = 0; /* Address 8 reg, D2 bit:LPF mode mux, LPF_MODE_SEL 0 = Normal operation, 1 = Operating mode is programmed Address 5 D<9:8> */
    flush_one(Register::RX_TOP_1);    /* Leave LPF_MODE_SEL 0 = Normal operation */
}

bool MAX2839::set_frequency(const rf::Frequency lo_frequency) {
    /* TODO: This is a sad implementation. Refactor. */
    if (lo::band[0].contains(lo_frequency)) {
        _map.r.syn_int_div.LOGEN_BSW = 0b00; /* 2300 - 2399.99MHz */
    } else if (lo::band[1].contains(lo_frequency)) {
        _map.r.syn_int_div.LOGEN_BSW = 0b01; /* 2400 - 2499.99MHz */
    } else if (lo::band[2].contains(lo_frequency)) {
        _map.r.syn_int_div.LOGEN_BSW = 0b10; /* 2500 - 2599.99MHz */
    } else if (lo::band[3].contains(lo_frequency)) {
        _map.r.syn_int_div.LOGEN_BSW = 0b11; /* 2600 - 2700Hz */
    } else {
        return false;
    }
    _dirty[Register::SYN_INT_DIV] = 1;

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

void MAX2839::set_rx_lo_iq_calibration(const size_t v) {
    _map.r.rxrf_2.RX_IQERR_SPI_EN = 1;
    _dirty[Register::RXRF_2] = 1;
    _map.r.rxrf_1.iqerr_trim = v;
    _dirty[Register::RXRF_1] = 1;
    flush();
}

void MAX2839::set_rx_buff_vcm(const size_t v) {
    _map.r.lpf_vga_2.BUFF_VCM = v;
    _dirty[Register::LPF_VGA_2] = 1;
    flush();
}

int8_t MAX2839::temp_sense() {
    if (!_map.r.rx_top_2.ts_en) {
        _map.r.rx_top_2.ts_en = 1;
        flush_one(Register::RX_TOP_2);

        chThdSleepMilliseconds(1);
    }

    _map.r.rx_top_2.ts_adc_trigger = 1;
    flush_one(Register::RX_TOP_2);

    halPolledDelay(ticks_for_temperature_sense_adc_conversion);

    /*
     * Conversion to degrees C determined by testing - does not match data sheet.
     */
    reg_t value = read(Register::TEMP_SENSE) & 0x1F;

    _map.r.rx_top_2.ts_adc_trigger = 0;
    flush_one(Register::RX_TOP_2);

    return std::min(127, (int)(value * 4.31 - 40));  // reg value is 0 to 31; possible return range is -40 C to 127 C
}

}  // namespace max2839