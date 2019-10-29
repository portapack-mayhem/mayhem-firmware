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

#include "clock_manager.hpp"

#include "portapack_io.hpp"

#include "hackrf_hal.hpp"
using namespace hackrf::one;

#include "lpc43xx_cpp.hpp"
using namespace lpc43xx;

constexpr uint32_t si5351_vco_f	= 800000000;

constexpr si5351::Inputs si5351_inputs {
	.f_xtal = si5351_xtal_f,
	.f_clkin = si5351_clkin_f,
	.clkin_div = 1,
};

static_assert(si5351_inputs.f_xtal        == si5351_xtal_f,  "XTAL output frequency wrong");
static_assert(si5351_inputs.f_clkin_out() == si5351_clkin_f, "CLKIN output frequency wrong");

constexpr si5351::PLLInputSource::Type si5351_pll_input_sources {
	  si5351::PLLInputSource::PLLA_Source_XTAL
	| si5351::PLLInputSource::PLLB_Source_CLKIN
	| si5351::PLLInputSource::CLKIN_Div1
};

constexpr si5351::PLL si5351_pll_xtal_25m {
	.f_in = si5351_inputs.f_xtal,
	.a = 32,
	.b = 0,
	.c = 1,
};
constexpr auto si5351_pll_a_xtal_reg = si5351_pll_xtal_25m.reg(0);

constexpr si5351::PLL si5351_pll_clkin_10m {
	.f_in = si5351_inputs.f_clkin_out(),
	.a = 80,
	.b = 0,
	.c = 1,
};
constexpr auto si5351_pll_b_clkin_reg = si5351_pll_clkin_10m.reg(1);

static_assert(si5351_pll_xtal_25m.f_vco() == si5351_vco_f, "PLL XTAL frequency wrong");
static_assert(si5351_pll_xtal_25m.p1()    ==         3584, "PLL XTAL P1 wrong");
static_assert(si5351_pll_xtal_25m.p2()    ==            0, "PLL XTAL P2 wrong");
static_assert(si5351_pll_xtal_25m.p3()    ==            1, "PLL XTAL P3 wrong");

static_assert(si5351_pll_clkin_10m.f_vco() == si5351_vco_f, "PLL CLKIN frequency wrong");
static_assert(si5351_pll_clkin_10m.p1()    ==         9728, "PLL CLKIN P1 wrong");
static_assert(si5351_pll_clkin_10m.p2()    ==            0, "PLL CLKIN P2 wrong");
static_assert(si5351_pll_clkin_10m.p3()    ==            1, "PLL CLKIN P3 wrong");
/*
constexpr si5351::MultisynthFractional si5351_ms_18m432 {
	.f_src = si5351_vco_f,
	.a = 43,
	.b = 29,
	.c = 72,
	.r_div = 1,
};
*/
/*
constexpr si5351::MultisynthFractional si5351_ms_0_20m {
	.f_src = si5351_vco_f,
	.a = 20,
	.b = 0,
	.c = 1,
	.r_div = 1,
};
constexpr auto si5351_ms_0_20m_reg = si5351_ms_0_20m.reg(0);
*/
constexpr si5351::MultisynthFractional si5351_ms_0_8m {
	.f_src = si5351_vco_f,
	.a = 50,
	.b = 0,
	.c = 1,
	.r_div = 1,
};
constexpr auto si5351_ms_0_8m_reg = si5351_ms_0_8m.reg(clock_generator_output_codec);

constexpr si5351::MultisynthFractional si5351_ms_group {
	.f_src = si5351_vco_f,
	.a = 80,  /* Don't care */
	.b = 0,
	.c = 1,
	.r_div = 0,
};
constexpr auto si5351_ms_1_group_reg = si5351_ms_group.reg(clock_generator_output_cpld);
constexpr auto si5351_ms_2_group_reg = si5351_ms_group.reg(clock_generator_output_sgpio);

constexpr si5351::MultisynthFractional si5351_ms_10m {
	.f_src = si5351_vco_f,
	.a = 80,
	.b = 0,
	.c = 1,
	.r_div = 0,
};
constexpr auto si5351_ms_3_10m_reg = si5351_ms_10m.reg(3);

constexpr si5351::MultisynthFractional si5351_ms_40m {
	.f_src = si5351_vco_f,
	.a = 20,
	.b = 0,
	.c = 1,
	.r_div = 0,
};

constexpr auto si5351_ms_rffc5072 = si5351_ms_40m;
constexpr auto si5351_ms_max2837 = si5351_ms_40m;

constexpr auto si5351_ms_4_reg = si5351_ms_rffc5072.reg(clock_generator_output_first_if);
constexpr auto si5351_ms_5_reg = si5351_ms_max2837.reg(clock_generator_output_second_if);

static_assert(si5351_ms_10m.f_out() == 10000000, "MS 10MHz f_out wrong");
static_assert(si5351_ms_10m.p1()    ==     9728, "MS 10MHz p1 wrong");
static_assert(si5351_ms_10m.p2()    ==        0, "MS 10MHz p2 wrong");
static_assert(si5351_ms_10m.p3()    ==        1, "MS 10MHz p3 wrong");

static_assert(si5351_ms_rffc5072.f_out() == rffc5072_reference_f, "RFFC5072 reference f_out wrong");
// static_assert(si5351_ms_50m.p1()    ==     2048, "MS 50MHz P1 wrong");
// static_assert(si5351_ms_50m.p2()    ==        0, "MS 50MHz P2 wrong");
// static_assert(si5351_ms_50m.p3()    ==        1, "MS 50MHz P3 wrong");

static_assert(si5351_ms_max2837.f_out() == max2837_reference_f, "MAX2837 reference f_out wrong");
// static_assert(si5351_ms_50m.p1()    ==     2048, "MS 40MHz P1 wrong");
// static_assert(si5351_ms_50m.p2()    ==        0, "MS 40MHz P2 wrong");
// static_assert(si5351_ms_50m.p3()    ==        1, "MS 40MHz P3 wrong");

constexpr si5351::MultisynthInteger si5351_ms_int_off {
	.f_src = si5351_vco_f,
	.a = 255,
	.r_div = 0,
};

constexpr si5351::MultisynthInteger si5351_ms_int_mcu_clkin {
	.f_src = si5351_vco_f,
	.a = 20,
	.r_div = 0,
};

constexpr auto si5351_ms6_7_off_mcu_clkin_reg = si5351::ms6_7_reg(si5351_ms_int_off, si5351_ms_int_mcu_clkin);

static_assert(si5351_ms_int_off.f_out() == 3137254, "MS int off f_out wrong");
static_assert(si5351_ms_int_off.p1()    ==     255, "MS int off P1 wrong");

static_assert(si5351_ms_int_mcu_clkin.f_out() == mcu_clkin_f, "MS int MCU CLKIN f_out wrong");
// static_assert(si5351_ms_int_mcu_clkin.p1()    ==       20, "MS int MCU CLKIN P1 wrong");

using namespace si5351;

static constexpr ClockControl::MultiSynthSource get_reference_clock_generator_pll(const ClockManager::ReferenceSource reference_source) {
	return (reference_source == ClockManager::ReferenceSource::Xtal)
		? ClockControl::MultiSynthSource::PLLA
		: ClockControl::MultiSynthSource::PLLB
		;
}

constexpr ClockControls si5351_clock_control_common { {
	{ ClockControl::ClockCurrentDrive::_8mA, ClockControl::ClockSource::MS_Self,  ClockControl::ClockInvert::Normal, get_reference_clock_generator_pll(ClockManager::ReferenceSource::Xtal), ClockControl::MultiSynthMode::Fractional, ClockControl::ClockPowerDown::Power_Off },
	{ ClockControl::ClockCurrentDrive::_2mA, ClockControl::ClockSource::MS_Group, ClockControl::ClockInvert::Invert, get_reference_clock_generator_pll(ClockManager::ReferenceSource::Xtal), ClockControl::MultiSynthMode::Integer,    ClockControl::ClockPowerDown::Power_Off },
	{ ClockControl::ClockCurrentDrive::_2mA, ClockControl::ClockSource::MS_Group, ClockControl::ClockInvert::Normal, get_reference_clock_generator_pll(ClockManager::ReferenceSource::Xtal), ClockControl::MultiSynthMode::Integer,    ClockControl::ClockPowerDown::Power_Off },
	{ ClockControl::ClockCurrentDrive::_8mA, ClockControl::ClockSource::MS_Self,  ClockControl::ClockInvert::Normal, get_reference_clock_generator_pll(ClockManager::ReferenceSource::Xtal), ClockControl::MultiSynthMode::Integer,    ClockControl::ClockPowerDown::Power_Off },
	{ ClockControl::ClockCurrentDrive::_6mA, ClockControl::ClockSource::MS_Self,  ClockControl::ClockInvert::Invert, get_reference_clock_generator_pll(ClockManager::ReferenceSource::Xtal), ClockControl::MultiSynthMode::Integer,    ClockControl::ClockPowerDown::Power_Off },
	{ ClockControl::ClockCurrentDrive::_4mA, ClockControl::ClockSource::MS_Self,  ClockControl::ClockInvert::Normal, get_reference_clock_generator_pll(ClockManager::ReferenceSource::Xtal), ClockControl::MultiSynthMode::Integer,    ClockControl::ClockPowerDown::Power_Off },
	{ ClockControl::ClockCurrentDrive::_2mA, ClockControl::ClockSource::MS_Self,  ClockControl::ClockInvert::Normal, get_reference_clock_generator_pll(ClockManager::ReferenceSource::Xtal), ClockControl::MultiSynthMode::Fractional, ClockControl::ClockPowerDown::Power_Off },
	{ ClockControl::ClockCurrentDrive::_2mA, ClockControl::ClockSource::MS_Self,  ClockControl::ClockInvert::Normal, get_reference_clock_generator_pll(ClockManager::ReferenceSource::Xtal), ClockControl::MultiSynthMode::Integer,    ClockControl::ClockPowerDown::Power_Off },
} };

ClockManager::Reference ClockManager::get_reference() const {
	return reference;
}

static void portapack_tcxo_enable() {
	portapack::io.reference_oscillator(true);

	/* Delay >10ms at 96MHz clock speed for reference oscillator to start. */
	/* Delay an additional 1ms (arbitrary) for the clock generator to detect a signal. */
	volatile uint32_t delay = 240000 + 24000;
	while(delay--);
}

static void portapack_tcxo_disable() {
	portapack::io.reference_oscillator(false);
}

#include "hackrf_gpio.hpp"
using namespace hackrf::one;

void ClockManager::init_clock_generator() {
	clock_generator.reset();
	clock_generator.set_crystal_internal_load_capacitance(CrystalInternalLoadCapacitance::XTAL_CL_8pF);
	clock_generator.enable_fanout();
	clock_generator.set_pll_input_sources(si5351_pll_input_sources);

	clock_generator.set_clock_control(
		clock_generator_output_mcu_clkin,
		si5351_clock_control_common[clock_generator_output_mcu_clkin].clk_src(ClockControl::ClockSource::CLKIN).clk_pdn(ClockControl::ClockPowerDown::Power_On)
	);
	clock_generator.enable_output(clock_generator_output_mcu_clkin);

	reference = choose_reference();

	clock_generator.disable_output(clock_generator_output_mcu_clkin);

	const auto ref_pll = get_reference_clock_generator_pll(reference.source);
	const ClockControls si5351_clock_control = ClockControls { {
		si5351_clock_control_common[0].ms_src(ref_pll),
		si5351_clock_control_common[1].ms_src(ref_pll),
		si5351_clock_control_common[2].ms_src(ref_pll),
		si5351_clock_control_common[3].ms_src(ref_pll),
		si5351_clock_control_common[4].ms_src(ref_pll),
		si5351_clock_control_common[5].ms_src(ref_pll),
		si5351_clock_control_common[6].ms_src(ref_pll),
		si5351_clock_control_common[7].ms_src(ref_pll),
	} };
	clock_generator.set_clock_control(si5351_clock_control);

	clock_generator.write(si5351_pll_a_xtal_reg);
	clock_generator.write(si5351_pll_b_clkin_reg);
	clock_generator.write(si5351_ms_0_8m_reg);
	clock_generator.write(si5351_ms_1_group_reg);
	clock_generator.write(si5351_ms_2_group_reg);
	clock_generator.write(si5351_ms_3_10m_reg);
	clock_generator.write(si5351_ms_4_reg);
	clock_generator.write(si5351_ms_5_reg);
	clock_generator.write(si5351_ms6_7_off_mcu_clkin_reg);

	clock_generator.reset_plls();

	// Wait for both PLLs to lock.
	// TODO: Disable the unused PLL?
	const uint8_t device_status_mask = (ref_pll == ClockControl::MultiSynthSource::PLLB) ? 0x40 : 0x20;
	while((clock_generator.device_status() & device_status_mask) != 0);

	clock_generator.set_clock_control(
		clock_generator_output_mcu_clkin,
		si5351_clock_control_common[clock_generator_output_mcu_clkin].ms_src(ref_pll).clk_pdn(ClockControl::ClockPowerDown::Power_On)
	);
	clock_generator.enable_output(clock_generator_output_mcu_clkin);
}

uint32_t ClockManager::measure_gp_clkin_frequency() {
	// Measure Si5351B CLKIN frequency against LPC43xx IRC oscillator
	start_frequency_monitor_measurement(cgu::CLK_SEL::GP_CLKIN);
	wait_For_frequency_monitor_measurement_done();
	return get_frequency_monitor_measurement_in_hertz();
}

ClockManager::ReferenceSource ClockManager::detect_reference_source() {
	if( clock_generator.clkin_loss_of_signal() ) {
		// No external reference. Turn on PortaPack reference (if present).
		portapack_tcxo_enable();

		if( clock_generator.clkin_loss_of_signal() ) {
			// No PortaPack reference was detected. Choose the HackRF crystal as the reference.
			return ReferenceSource::Xtal;
		} else {
			return ReferenceSource::PortaPack;
		}
	} else {
		return ReferenceSource::External;
	}
}

ClockManager::Reference ClockManager::choose_reference() {
	const auto detected_reference = detect_reference_source();

	if( (detected_reference == ReferenceSource::External) ||
	    (detected_reference == ReferenceSource::PortaPack) ) {
		const auto frequency = measure_gp_clkin_frequency();
		if( (frequency >= 9850000) && (frequency <= 10150000) ) {

			return { detected_reference, 10000000 };
		}
	}

	portapack_tcxo_disable();
	return { ReferenceSource::Xtal, 10000000 };
}

void ClockManager::shutdown() {
	clock_generator.reset();
}

void ClockManager::enable_codec_clocks() {
	clock_generator.enable_clock(clock_generator_output_codec);
	clock_generator.enable_clock(clock_generator_output_cpld);
	clock_generator.enable_clock(clock_generator_output_sgpio);
	/* Turn on all outputs at the same time. This probably doesn't ensure
	 * their phase relationships. For example, clocks that output frequencies
	 * in a 2:1 relationship may start with the slower clock high or low?
	 */
	clock_generator.enable_output_mask(
		  (1U << clock_generator_output_codec)
		| (1U << clock_generator_output_cpld)
		| (1U << clock_generator_output_sgpio)
	);
}

void ClockManager::disable_codec_clocks() {
	/* Turn off outputs before disabling clocks. It seems the clock needs to
	 * be enabled for the output to come to rest at the state specified by
	 * CLKx_DISABLE_STATE.
	 */
	clock_generator.disable_output_mask(
		  (1U << clock_generator_output_codec)
		| (1U << clock_generator_output_cpld)
		| (1U << clock_generator_output_sgpio)
	);
	clock_generator.disable_clock(clock_generator_output_codec);
	clock_generator.disable_clock(clock_generator_output_cpld);
	clock_generator.disable_clock(clock_generator_output_sgpio);
}

void ClockManager::enable_first_if_clock() {
	clock_generator.enable_clock(clock_generator_output_first_if);
	clock_generator.enable_output_mask(1U << clock_generator_output_first_if);
}

void ClockManager::disable_first_if_clock() {
	clock_generator.disable_output_mask(1U << clock_generator_output_first_if);
	clock_generator.disable_clock(clock_generator_output_first_if);
}

void ClockManager::enable_second_if_clock() {
	clock_generator.enable_clock(clock_generator_output_second_if);
	clock_generator.enable_output_mask(1U << clock_generator_output_second_if);
}

void ClockManager::disable_second_if_clock() {
	clock_generator.disable_output_mask(1U << clock_generator_output_second_if);
	clock_generator.disable_clock(clock_generator_output_second_if);
}

void ClockManager::set_sampling_frequency(const uint32_t frequency) {
	/* Codec clock is at sampling frequency, CPLD and SGPIO clocks are at
	 * twice the frequency, and derived from the MS0 synth. So it's only
	 * necessary to change the MS0 synth frequency, and ensure the output
	 * is divided by two.
	 */
	clock_generator.set_ms_frequency(clock_generator_output_codec, frequency * 2, si5351_vco_f, 1);
}

void ClockManager::set_reference_ppb(const int32_t ppb) {
	/* NOTE: This adjustment only affects PLLA, which is derived from the 25MHz crystal.
	 * It is assumed an external clock coming in to PLLB is sufficiently accurate as to not need adjustment.
	 * TODO: Revisit the above policy. It may be good to allow adjustment of the external reference too.
	 */
	constexpr uint32_t pll_multiplier = si5351_pll_xtal_25m.a;
	constexpr uint32_t denominator = 1000000 / pll_multiplier;
	const uint32_t new_a = (ppb >= 0) ? pll_multiplier : (pll_multiplier - 1);
	const uint32_t new_b = (ppb >= 0) ? (ppb / 1000) : (denominator + (ppb / 1000));
	const uint32_t new_c = (ppb == 0) ? 1 : denominator;

	const si5351::PLL pll {
		.f_in = si5351_inputs.f_xtal,
		.a = new_a,
		.b = new_b,
		.c = new_c,
	};
	const auto pll_a_reg = pll.reg(0);
	clock_generator.write(pll_a_reg);
}

void ClockManager::start_frequency_monitor_measurement(const cgu::CLK_SEL clk_sel) {
	// Measure a clock input for 480 cycles of the LPC43xx IRC.
	LPC_CGU->FREQ_MON = LPC_CGU_FREQ_MON_Type {
		.RCNT = 480,
		.FCNT = 0,
		.MEAS = 0,
		.CLK_SEL = toUType(clk_sel),
		.RESERVED0 = 0
	};
	LPC_CGU->FREQ_MON.MEAS = 1;
}

void ClockManager::wait_For_frequency_monitor_measurement_done() {
	// FREQ_MON mechanism fails to finish if there's no clock present on selected input?!
	while(LPC_CGU->FREQ_MON.MEAS == 1);
}

uint32_t ClockManager::get_frequency_monitor_measurement_in_hertz() {
	// Measurement is only as accurate as the LPC43xx IRC oscillator,
	// which is +/- 1.5%. Measurement is for 480 IRC clcocks. Scale
	// the cycle count to get a value in Hertz.
	return LPC_CGU->FREQ_MON.FCNT * 25000;
}

void ClockManager::start_audio_pll() {
	cgu::pll0audio::ctrl({
		.pd = 1,
		.bypass = 0,
		.directi = 0,
		.directo = 0,
		.clken = 0,
		.frm = 0,
		.autoblock = 1,
		.pllfract_req = 0,
		.sel_ext = 1,
		.mod_pd = 1,
		.clk_sel = cgu::CLK_SEL::GP_CLKIN,
	});

	/* For 40MHz clock source, 48kHz audio rate, 256Fs MCLK:
	 * 		Fout=12.288MHz, Fcco=491.52MHz
	 *		PSEL=20, NSEL=125, MSEL=768
	 *		PDEC=31, NDEC=45, MDEC=30542
	 */
	cgu::pll0audio::mdiv({
		.mdec = 30542,
	});
	cgu::pll0audio::np_div({
		.pdec = 31,
		.ndec = 45,
	});

	cgu::pll0audio::frac({
		.pllfract_ctrl = 0,
	});

	cgu::pll0audio::power_up();
	while( !cgu::pll0audio::is_locked() );
	cgu::pll0audio::clock_enable();

	set_base_audio_clock_divider(1);

	LPC_CGU->BASE_AUDIO_CLK.AUTOBLOCK = 1;
	LPC_CGU->BASE_AUDIO_CLK.CLK_SEL = toUType(cgu::CLK_SEL::IDIVD);
}

void ClockManager::set_base_audio_clock_divider(const size_t divisor) {
	LPC_CGU->IDIVD_CTRL.word =
		  (0 <<  0)
		| ((divisor - 1) <<  2)
		| (1 << 11)
		| (toUType(cgu::CLK_SEL::PLL0AUDIO) << 24)
		;
}

void ClockManager::stop_audio_pll() {
	cgu::pll0audio::clock_disable();
	cgu::pll0audio::power_down();
	while( cgu::pll0audio::is_locked() );
}
