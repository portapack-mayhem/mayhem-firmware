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

namespace lna {

constexpr std::array<uint8_t, 8> lookup_8db_steps {
	0b111, 0b011, 0b110, 0b010,
	0b100, 0b000, 0b000, 0b000
};

static uint_fast8_t gain_ordinal(const int8_t db) {
	const auto db_sat = gain_db_range.clip(db);
	return lna::lookup_8db_steps[(db_sat >> 3) & 7];
}

} /* namespace lna */

namespace vga {

static uint_fast8_t gain_ordinal(const int8_t db) {
	const auto db_sat = gain_db_range.clip(db);
	return ((db_sat >> 1) & 0b11111) ^ 0b11111;
}

} /* namespace vga */

namespace tx {

static uint_fast8_t gain_ordinal(const int8_t db) {
	const auto db_sat = gain_db_range.clip(db);
	uint8_t value = db_sat & 0x0f;
	value = (db_sat >= 16) ? (value | 0x20) : value;
	value = (db_sat >= 32) ? (value | 0x10) : value;
	return (value & 0b111111) ^ 0b111111;
}

} /* namespace tx */

namespace filter {

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

constexpr uint32_t reference_frequency = max2837_reference_f;
constexpr uint32_t pll_factor = 1.0 / (4.0 / 3.0 / reference_frequency) + 0.5;

void MAX2837::init() {
	set_mode(Mode::Shutdown);

	gpio_max2837_enable.output();
	gpio_max2837_rxenable.output();
	gpio_max2837_txenable.output();

	_map.r.tx_gain.TXVGA_GAIN_SPI_EN = 1;
	_map.r.tx_gain.TXVGA_GAIN_MSB_SPI_EN = 1;
	_map.r.tx_gain.TXVGA_GAIN_SPI = 0x00;

	_map.r.lpf_3_vga_1.VGAMUX_enable = 1;
	_map.r.lpf_3_vga_1.VGA_EN = 1;

	_map.r.hpfsm_3.HPC_STOP = 1;	/* 1kHz */

	_map.r.rx_top_rx_bias.LNAgain_SPI_EN = 1;	/* control LNA gain from SPI */
	_map.r.rxrf_2.L = 0b000;

	_map.r.rx_top_rx_bias.VGAgain_SPI_EN = 1;	/* control VGA gain from SPI */
	_map.r.vga_2.VGA = 0b01010;

	_map.r.lpf_3_vga_1.BUFF_VCM = 0b00;		/* TODO: Check values out of ADC */

	_map.r.lpf_1.LPF_EN = 1;				/* Enable low-pass filter */
	_map.r.lpf_1.ModeCtrl = 0b01;			/* Rx LPF */
	_map.r.lpf_1.FT = 0b0000;				/* 5MHz LPF */

	_map.r.spi_en.EN_SPI = 1;				/* enable chip functions when ENABLE pin set */

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
	_map.r.vga_3_rx_top.RSSI_MODE = 1;		/* RSSI independent of RXHP */

	_dirty.set();
	flush();

	set_mode(Mode::Standby);
}

void MAX2837::set_mode(const Mode mode) {
	gpio_max2837_enable.write(toUType(mode) & toUType(Mode::Mask_Enable));
	gpio_max2837_rxenable.write(toUType(mode) & toUType(Mode::Mask_RxEnable));
	gpio_max2837_txenable.write(toUType(mode) & toUType(Mode::Mask_TxEnable));
}

void MAX2837::flush() {
	if( _dirty ) {
		for(size_t n=0; n<reg_count; n++) {
			if( _dirty[n] ) {
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

reg_t MAX2837::read(const address_t reg_num) {	//16 bit registers
	uint16_t t = (1U << 15) | (reg_num << 10);	// bits: 15 1=read mode 14,13,12,11,10 = register address
	_target.transfer(&t, 1U);	//read 
	return t & 0x3ffU;	//Return bits 9,8,7,6,5,4,3,2,1,0 (masked)
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

void MAX2837::set_lpf_rf_bandwidth(const uint32_t bandwidth_minimum) {
	_map.r.lpf_1.FT = filter::bandwidth_ordinal(bandwidth_minimum);
	_dirty[Register::LPF_1] = 1;
	flush();
}

bool MAX2837::set_frequency(const rf::Frequency lo_frequency) {
	/* TODO: This is a sad implementation. Refactor. */
	if( lo::band[0].contains(lo_frequency) ) {
		_map.r.syn_int_div.LOGEN_BSW = 0b00;	/* 2300 - 2399.99MHz */
		_map.r.rxrf_1.LNAband = 0;				/* 2.3 - 2.5GHz */
	} else if( lo::band[1].contains(lo_frequency)  ) {
		_map.r.syn_int_div.LOGEN_BSW = 0b01;	/* 2400 - 2499.99MHz */
		_map.r.rxrf_1.LNAband = 0;				/* 2.3 - 2.5GHz */
	} else if( lo::band[2].contains(lo_frequency) ) {
		_map.r.syn_int_div.LOGEN_BSW = 0b10;	/* 2500 - 2599.99MHz */
		_map.r.rxrf_1.LNAband = 1;				/* 2.5 - 2.7GHz */
	} else if( lo::band[3].contains(lo_frequency) ) {
		_map.r.syn_int_div.LOGEN_BSW = 0b11;	/* 2600 - 2700Hz */
		_map.r.rxrf_1.LNAband = 1;				/* 2.5 - 2.7GHz */
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

reg_t MAX2837::temp_sense() {
	if( !_map.r.rx_top.ts_en ) {
		_map.r.rx_top.ts_en = 1;
		flush_one(Register::RX_TOP);

		chThdSleepMilliseconds(1);
	}

	_map.r.rx_top.ts_adc_trigger = 1;
	flush_one(Register::RX_TOP);

	halPolledDelay(ticks_for_temperature_sense_adc_conversion);

	const auto value = read(Register::TEMP_SENSE);

	_map.r.rx_top.ts_adc_trigger = 0;
	flush_one(Register::RX_TOP);

	return value;
}

}
