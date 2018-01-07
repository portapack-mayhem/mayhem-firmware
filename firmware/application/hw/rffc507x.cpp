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

#include "rffc507x.hpp"

#include <array>

#include "utility.hpp"

#include "hackrf_hal.hpp"
#include "hackrf_gpio.hpp"
using namespace hackrf::one;

#include "hal.h"

namespace rffc507x {

/* Empirical tests indicate no minimum reset pulse width, but the speed
 * of the processor and GPIO probably produce at least 20ns pulse width.
 */
constexpr float seconds_during_reset = 1.0e-6;
constexpr halrtcnt_t ticks_during_reset = (base_m4_clk_f * seconds_during_reset + 1);

/* Empirical testing indicates >3.5us delay required after reset, before
 * registers can be reliably written. Make it 5us, just for fun. Tests were
 * conducted at high temperatures (with a hair dryer) increased room
 * temperature minimum delay of 2.9us to the requirement above.
 */
constexpr float seconds_after_reset = 5.0e-6;
constexpr halrtcnt_t ticks_after_reset = (base_m4_clk_f * seconds_after_reset + 1);

constexpr auto reference_frequency = rffc5072_reference_f;

namespace vco {

constexpr rf::FrequencyRange range { 2700000000, 5400000000 };

} /* namespace vco */

namespace lo {

constexpr size_t divider_log2_min = 0;
constexpr size_t divider_log2_max = 5;

constexpr size_t divider_min = 1U << divider_log2_min;
constexpr size_t divider_max = 1U << divider_log2_max;

constexpr rf::FrequencyRange range { vco::range.minimum / divider_max, vco::range.maximum / divider_min };

size_t divider_log2(const rf::Frequency lo_frequency) {
	/* TODO: Error */
	/*
	if( lo::range.out_of_range(lo_frequency) ) {
		return;
	}
	*/
	/* Compute LO divider. */
	auto lo_divider_log2 = lo::divider_log2_min;
	auto vco_frequency = lo_frequency;
	while( vco::range.below_range(vco_frequency) ) {
		vco_frequency <<= 1;
		lo_divider_log2 += 1;
	}

	return lo_divider_log2;
}

} /* namespace lo */

namespace prescaler {

constexpr rf::Frequency max_frequency = 1600000000U;

constexpr size_t divider_log2_min = 1;
constexpr size_t divider_log2_max = 2;

constexpr size_t divider_min = 1U << divider_log2_min;
constexpr size_t divider_max = 1U << divider_log2_max;

constexpr size_t divider_log2(const rf::Frequency vco_frequency) {
	return (vco_frequency > (prescaler::divider_min * prescaler::max_frequency))
		? prescaler::divider_log2_max
		: prescaler::divider_log2_min
		;
}

} /* namespace prescaler */

struct SynthConfig {
	const size_t lo_divider_log2;
	const size_t prescaler_divider_log2;
	const uint64_t n_divider_q24;

	static SynthConfig calculate(
		const rf::Frequency lo_frequency
	) {
		/* RFFC507x frequency synthesizer is is accurate to about 2ppb (two parts
		 * per BILLION). There's not much point to worrying about rounding and
		 * tuning error, when it amounts to 8Hz at 5GHz!
		 */
		const size_t lo_divider_log2 = lo::divider_log2(lo_frequency);
		const size_t lo_divider = 1U << lo_divider_log2;

		const rf::Frequency vco_frequency = lo_frequency * lo_divider;

		const size_t prescaler_divider_log2 = prescaler::divider_log2(vco_frequency);

		const uint64_t prescaled_lo_q24 = vco_frequency << (24 - prescaler_divider_log2);
		const uint64_t n_divider_q24 = prescaled_lo_q24 / reference_frequency;

		return {
			lo_divider_log2,
			prescaler_divider_log2,
			n_divider_q24,
		};
	}
};

/* Readback values, RFFC5072 rev A:
 * 0000: 0x8a01 => dev_id=1000101000000 mrev_id=001
 * 0001: 0x3f7c => lock=0 ct_cal=0111111 cp_cal=011111 ctfail=0 0
 * 0010: 0x806f => v0_cal=10000000 v1_cal=01101111
 * 0011: 0x0000 => rsm_state=00000 f_errflag=00
 * 0100: 0x0000 => vco_count_l=0
 * 0101: 0x0000 => vco_count_h=0
 * 0110: 0xc000 => cal_fbi=1 cal_fbq=1
 * 0111: 0x0000 => vco_sel=0 vco_tc_curve=0
 */

void RFFC507x::init() {
	gpio_rffc5072_resetx.set();
	gpio_rffc5072_resetx.output();
	reset();

	_bus.init();

	_dirty.set();
	flush();
}

void RFFC507x::reset() {
	/* TODO: Is RESETB pin ignored if sdi_ctrl.sipin=1? Programming guide
	 * description of sdi_ctrl.sipin suggests the pin is not ignored.
	 */
	gpio_rffc5072_resetx.clear();
	halPolledDelay(ticks_during_reset);
	gpio_rffc5072_resetx.set();
	halPolledDelay(ticks_after_reset);
}

void RFFC507x::flush() {
	if( _dirty ) {
		for(size_t i=0; i<_map.w.size(); i++) {
			if( _dirty[i] ) {
				write(i, _map.w[i]);
			}
		}
		_dirty.clear();
	}
}

void RFFC507x::write(const address_t reg_num, const spi::reg_t value) {
	_bus.write(reg_num, value);
}

spi::reg_t RFFC507x::read(const address_t reg_num) {
	return _bus.read(reg_num);
}

void RFFC507x::write(const Register reg, const spi::reg_t value) {
	write(toUType(reg), value);
}

spi::reg_t RFFC507x::read(const Register reg) {
	return read(toUType(reg));
}

void RFFC507x::flush_one(const Register reg) {
	const auto reg_num = toUType(reg);
	write(reg_num, _map.w[reg_num]);
	_dirty.clear(reg_num);
}

void RFFC507x::enable() {
	_map.r.sdi_ctrl.enbl = 1;
	flush_one(Register::SDI_CTRL);

	/* TODO: Reset PLLCPL after CT_CAL? */

	/* TODO: After device is enabled and CT_cal is complete and VCO > 3.2GHz,
	 * change prescaler divider to 2, update synthesizer ratio, change
	 * lf.pllcpl from 3 to 2.
	 */
}

void RFFC507x::disable() {
	_map.r.sdi_ctrl.enbl = 0;
	flush_one(Register::SDI_CTRL);
}

void RFFC507x::set_mixer_current(const uint8_t value) {
	/* MIX IDD = 0b000 appears to turn the mixer completely off */
	/* TODO: Adjust mixer current. Graphs in datasheet suggest:
	 * MIX_IDD=1 has lowest noise figure (10.1dB vs 13dB @ MIX_IDD=7).
	 * MIX_IDD=5 has highest IP3 (24dBm vs 10.3dBm @ MIX_IDD=1).
	 * MIX_IDD=5 has highest P1dB (11.8dBm vs 1.5dBm @ MIX_IDD=1).
	 * Mixer input impedance ~85 Ohms at MIX_IDD=4.
	 * Mixer input impedance inversely proportional to MIX_IDD.
	 * Balun balanced (mixer) side is 100 Ohms. Perhaps reduce MIX_IDD
	 * a bit to get 100 Ohms from mixer.
	 */
	_map.r.mix_cont.p1mixidd = value;
	_map.r.mix_cont.p2mixidd = value;
	flush_one(Register::MIX_CONT);
}

void RFFC507x::set_frequency(const rf::Frequency lo_frequency) {
	const SynthConfig synth_config = SynthConfig::calculate(lo_frequency);

	/* Boost charge pump leakage if VCO frequency > 3.2GHz, indicated by
	 * prescaler divider set to 4 (log2=2) instead of 2 (log2=1).
	 */
	if( synth_config.prescaler_divider_log2 == 2 ) {
		_map.r.lf.pllcpl = 3;
	} else {
		_map.r.lf.pllcpl = 2;
	}
	flush_one(Register::LF);

	_map.r.p2_freq1.p2n = synth_config.n_divider_q24 >> 24;
	_map.r.p2_freq1.p2lodiv = synth_config.lo_divider_log2;
	_map.r.p2_freq1.p2presc = synth_config.prescaler_divider_log2;
	_map.r.p2_freq2.p2nmsb = (synth_config.n_divider_q24 >> 8) & 0xffff;
	_map.r.p2_freq3.p2nlsb = synth_config.n_divider_q24 & 0xff;
	_dirty[Register::P2_FREQ1] = 1;
	_dirty[Register::P2_FREQ2] = 1;
	_dirty[Register::P2_FREQ3] = 1;
	flush();
}

void RFFC507x::set_gpo1(const bool new_value) {
	if( new_value ) {
		_map.r.gpo.p2gpo |= 1;
		_map.r.gpo.p1gpo |= 1;
	} else {
		_map.r.gpo.p2gpo &= ~1;
		_map.r.gpo.p1gpo &= ~1;
	}

	flush_one(Register::GPO);
}

spi::reg_t RFFC507x::readback(const Readback readback) {
	/* TODO: This clobbers the rest of the DEV_CTRL register
	 * Time to implement bitfields for registers.
	 */
	_map.r.dev_ctrl.readsel = toUType(readback);
	flush_one(Register::DEV_CTRL);

	return read(Register::READBACK);
}

} /* namespace rffc507x */
