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

#include "hackrf_hal.hpp"
using namespace hackrf::one;

#include "lpc43xx_cpp.hpp"
using namespace lpc43xx;

static void set_clock(LPC_CGU_BASE_CLK_Type& clk, const cgu::CLK_SEL clock_source) {
	clk.AUTOBLOCK = 1;
	clk.CLK_SEL = toUType(clock_source);
}

static constexpr uint32_t systick_count(const uint32_t clock_source_f) {
	return clock_source_f / CH_FREQUENCY;
}

static constexpr uint32_t systick_load(const uint32_t clock_source_f) {
	return systick_count(clock_source_f) - 1;
}

constexpr uint32_t clock_source_irc_f		=  12000000;
//constexpr uint32_t clock_source_gp_clkin	=  20000000;
constexpr uint32_t clock_source_pll1_step_f	= 100000000;
constexpr uint32_t clock_source_pll1_f		= 200000000;

constexpr auto systick_count_irc = systick_load(clock_source_irc_f);
constexpr auto systick_count_pll1 = systick_load(clock_source_pll1_f);
constexpr auto systick_count_pll1_step = systick_load(clock_source_pll1_step_f);

constexpr uint32_t si5351_vco_f	= 800000000;

constexpr uint32_t i2c0_bus_f			= 400000;
constexpr uint32_t i2c0_high_period_ns	= 900;

constexpr I2CClockConfig i2c_clock_config_400k_slow_clock {
	.clock_source_f = clock_source_irc_f,
	.bus_f = i2c0_bus_f,
	.high_period_ns = i2c0_high_period_ns,
};

constexpr I2CClockConfig i2c_clock_config_400k_fast_clock {
	.clock_source_f = clock_source_pll1_f,
	.bus_f = i2c0_bus_f,
	.high_period_ns = i2c0_high_period_ns,
};

constexpr I2CConfig i2c_config_slow_clock {
	.high_count = i2c_clock_config_400k_slow_clock.i2c_high_count(),
	.low_count = i2c_clock_config_400k_slow_clock.i2c_low_count(),
};

constexpr I2CConfig i2c_config_fast_clock {
	.high_count = i2c_clock_config_400k_fast_clock.i2c_high_count(),
	.low_count = i2c_clock_config_400k_fast_clock.i2c_low_count(),
};

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

constexpr ClockControl::Type si5351_clock_control_ms_src_xtal = ClockControl::MS_SRC_PLLA;
constexpr ClockControl::Type si5351_clock_control_ms_src_clkin = ClockControl::MS_SRC_PLLB;

constexpr ClockControls si5351_clock_control_common {
	ClockControl::CLK_IDRV_6mA | ClockControl::CLK_SRC_MS_Self  | ClockControl::CLK_INV_Normal | ClockControl::MS_INT_Fractional | ClockControl::CLK_PDN_Power_Off,
	ClockControl::CLK_IDRV_6mA | ClockControl::CLK_SRC_MS_Group | ClockControl::CLK_INV_Invert | ClockControl::MS_INT_Integer    | ClockControl::CLK_PDN_Power_Off,
	ClockControl::CLK_IDRV_6mA | ClockControl::CLK_SRC_MS_Group | ClockControl::CLK_INV_Normal | ClockControl::MS_INT_Integer    | ClockControl::CLK_PDN_Power_Off,
	ClockControl::CLK_IDRV_8mA | ClockControl::CLK_SRC_MS_Self  | ClockControl::CLK_INV_Normal | ClockControl::MS_INT_Integer    | ClockControl::CLK_PDN_Power_Off,
	ClockControl::CLK_IDRV_8mA | ClockControl::CLK_SRC_MS_Self  | ClockControl::CLK_INV_Normal | ClockControl::MS_INT_Integer    | ClockControl::CLK_PDN_Power_Off,
	ClockControl::CLK_IDRV_6mA | ClockControl::CLK_SRC_MS_Self  | ClockControl::CLK_INV_Normal | ClockControl::MS_INT_Integer    | ClockControl::CLK_PDN_Power_Off,
	ClockControl::CLK_IDRV_2mA | ClockControl::CLK_SRC_MS_Self  | ClockControl::CLK_INV_Normal | ClockControl::MS_INT_Fractional | ClockControl::CLK_PDN_Power_Off,
	ClockControl::CLK_IDRV_6mA | ClockControl::CLK_SRC_MS_Self  | ClockControl::CLK_INV_Normal | ClockControl::MS_INT_Integer    | ClockControl::CLK_PDN_Power_Off,
};

constexpr ClockControls si5351_clock_control_xtal {
	si5351_clock_control_common[0] | si5351_clock_control_ms_src_xtal,
	si5351_clock_control_common[1] | si5351_clock_control_ms_src_xtal,
	si5351_clock_control_common[2] | si5351_clock_control_ms_src_xtal,
	si5351_clock_control_common[3] | si5351_clock_control_ms_src_xtal,
	si5351_clock_control_common[4] | si5351_clock_control_ms_src_xtal,
	si5351_clock_control_common[5] | si5351_clock_control_ms_src_xtal,
	si5351_clock_control_common[6] | si5351_clock_control_ms_src_xtal,
	si5351_clock_control_common[7] | si5351_clock_control_ms_src_xtal,
};

constexpr ClockControls si5351_clock_control_clkin {
	si5351_clock_control_common[0] | si5351_clock_control_ms_src_clkin,
	si5351_clock_control_common[1] | si5351_clock_control_ms_src_clkin,
	si5351_clock_control_common[2] | si5351_clock_control_ms_src_clkin,
	si5351_clock_control_common[3] | si5351_clock_control_ms_src_clkin,
	si5351_clock_control_common[4] | si5351_clock_control_ms_src_clkin,
	si5351_clock_control_common[5] | si5351_clock_control_ms_src_clkin,
	si5351_clock_control_common[6] | si5351_clock_control_ms_src_clkin,
	si5351_clock_control_common[7] | si5351_clock_control_ms_src_xtal,
};

void ClockManager::init(const bool use_clkin) {
	/* Must be sure to run the M4 core from IRC when messing with the signal
	 * generator that sources the GP_CLKIN signal that drives the micro-
	 * controller's PLL1 input.
	 */
	/* When booting from SPIFI, PLL1 is already running at 96MHz. */
	//run_from_irc();
	/* TODO: Refactor this blob, there's too much knowledge about post-boot
	 * state, which can change depending on where we're running from -- SPIFI
	 * or RAM or ???
	 */
	update_peripheral_clocks(cgu::CLK_SEL::IRC);
	start_peripherals(cgu::CLK_SEL::IRC);

	clock_generator.reset();
	clock_generator.set_crystal_internal_load_capacitance(CrystalInternalLoadCapacitance::XTAL_CL_8pF);
	clock_generator.enable_fanout();
	clock_generator.set_pll_input_sources(si5351_pll_input_sources);

	//const bool use_clkin = false;
	clock_generator.set_clock_control(
		use_clkin ?
			si5351_clock_control_clkin
		: si5351_clock_control_xtal
	);

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
}

void ClockManager::shutdown() {
	run_from_irc();
	clock_generator.reset();
}

void ClockManager::run_from_irc() {
	change_clock_configuration(cgu::CLK_SEL::IRC);
}

void ClockManager::run_at_full_speed() {
	change_clock_configuration(cgu::CLK_SEL::PLL1);
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

void ClockManager::change_clock_configuration(const cgu::CLK_SEL clk_sel) {
	/* If starting PLL1, turn on the clock feeding GP_CLKIN */
	if( clk_sel == cgu::CLK_SEL::PLL1 ) {
		enable_gp_clkin_source();
	}

	if( clk_sel == cgu::CLK_SEL::XTAL ) {
		enable_xtal_oscillator();
	}

	stop_peripherals();

	set_m4_clock_to_irc();

	update_peripheral_clocks(clk_sel);

	if( clk_sel == cgu::CLK_SEL::PLL1 ) {
		set_m4_clock_to_pll1();
	} else {
		power_down_pll1();
	}

	start_peripherals(clk_sel);

	if( clk_sel != cgu::CLK_SEL::XTAL ) {
		disable_xtal_oscillator();
	}

	/* If not using PLL1, disable clock feeding GP_CLKIN */
	if( clk_sel != cgu::CLK_SEL::PLL1 ) {
		stop_audio_pll();
		disable_gp_clkin_source();
	}
}

void ClockManager::enable_gp_clkin_source() {
	clock_generator.enable_clock(clock_generator_output_mcu_clkin);
	clock_generator.enable_output(clock_generator_output_mcu_clkin);
}

void ClockManager::disable_gp_clkin_source() {
	clock_generator.disable_clock(clock_generator_output_mcu_clkin);
	clock_generator.disable_output(clock_generator_output_mcu_clkin);
}

void ClockManager::enable_xtal_oscillator() {
	LPC_CGU->XTAL_OSC_CTRL.BYPASS = 0;
	LPC_CGU->XTAL_OSC_CTRL.ENABLE = 1;
}

void ClockManager::disable_xtal_oscillator() {
	LPC_CGU->XTAL_OSC_CTRL.ENABLE = 0;
}

void ClockManager::set_m4_clock_to_irc() {
	/* Set M4 clock to safe default speed (~12MHz IRC) */
	set_clock(LPC_CGU->BASE_M4_CLK, cgu::CLK_SEL::IRC);
	systick_adjust_period(systick_count_irc);
	//_clock_f = clock_source_irc_f;
	halLPCSetSystemClock(clock_source_irc_f);
}

void ClockManager::set_m4_clock_to_pll1() {
	/* Incantation from LPC43xx UM10503 section 12.2.1.1, to bring the M4
	 * core clock speed to the 110 - 204MHz range.
	 */

	/* Step into the 90-110MHz M4 clock range */
	cgu::pll1::ctrl({
		.pd = 0,
		.bypass = 0,
		.fbsel = 0,
		.direct = 0,
		.psel = 0,
		.autoblock = 1,
		.nsel = 0,
		.msel = 4,
		.clk_sel = cgu::CLK_SEL::GP_CLKIN,
	});
	while( !cgu::pll1::is_locked() );

	/* Switch M4 clock to PLL1 running at intermediate rate */
	set_clock(LPC_CGU->BASE_M4_CLK, cgu::CLK_SEL::PLL1);
	systick_adjust_period(systick_count_pll1_step);
	//_clock_f = clock_source_pll1_step_f;
	halLPCSetSystemClock(clock_source_pll1_step_f);

	/* Delay >50us at 90-110MHz clock speed */
	volatile uint32_t delay = 1400;
	while(delay--);

	/* Remove /2P divider from PLL1 output to achieve full speed */
	cgu::pll1::direct();
	systick_adjust_period(systick_count_pll1);
	//_clock_f = clock_source_pll1_f;
	halLPCSetSystemClock(clock_source_pll1_f);
}

void ClockManager::power_down_pll1() {
	/* Power down PLL1 if not needed */
	cgu::pll1::disable();
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
	set_clock(LPC_CGU->BASE_AUDIO_CLK, cgu::CLK_SEL::IDIVC);
}

void ClockManager::set_base_audio_clock_divider(const size_t divisor) {
	LPC_CGU->IDIVC_CTRL =
		  (0 <<  1)
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

void ClockManager::stop_peripherals() {
	i2c0.stop();
}

void ClockManager::update_peripheral_clocks(const cgu::CLK_SEL clk_sel) {
	/* TODO: Extract a structure to represent clock settings for different
	 * modes.
	 */
	set_clock(LPC_CGU->BASE_PERIPH_CLK, clk_sel);
	LPC_CGU->IDIVB_CTRL =
		  (0 <<  1)
		| (1 <<  2)
		| (1 << 11)
		| (toUType(clk_sel) << 24)
		;
	set_clock(LPC_CGU->BASE_APB1_CLK, clk_sel);
	set_clock(LPC_CGU->BASE_APB3_CLK, clk_sel);
	set_clock(LPC_CGU->BASE_SDIO_CLK, clk_sel);
	set_clock(LPC_CGU->BASE_SSP1_CLK, clk_sel);
}

void ClockManager::start_peripherals(const cgu::CLK_SEL clk_sel) {
	/* Start APB1 peripherals considering new clock */
	i2c0.start((clk_sel == cgu::CLK_SEL::PLL1)
		? i2c_config_fast_clock
		: i2c_config_slow_clock
	);
}
