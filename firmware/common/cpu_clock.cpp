/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#include "cpu_clock.hpp"

#include <ch.h>
#include <hal.h>

#include "lpc43xx_cpp.hpp"
using namespace lpc43xx;

constexpr uint32_t systick_count(const uint32_t clock_source_f) {
	return clock_source_f / CH_FREQUENCY;
}

constexpr uint32_t systick_load(const uint32_t clock_source_f) {
	return systick_count(clock_source_f) - 1;
}

constexpr auto systick_count_irc = systick_load(clock_source_irc_f);
constexpr auto systick_count_pll1 = systick_load(clock_source_pll1_f);
constexpr auto systick_count_pll1_step = systick_load(clock_source_pll1_step_f);

static void set_clock(LPC_CGU_BASE_CLK_Type& clk, const cgu::CLK_SEL clock_source) {
  clk.AUTOBLOCK = 1;
  clk.CLK_SEL = toUType(clock_source);
}

void cpu_clock_irc() {
	/* Set M4 clock to safe default speed (~12MHz IRC) */
	set_clock(LPC_CGU->BASE_M4_CLK, cgu::CLK_SEL::IRC);
	systick_adjust_period(systick_count_irc);
	halLPCSetSystemClock(clock_source_irc_f);
}

void cpu_xtal_start() {
	LPC_CGU->XTAL_OSC_CTRL.BYPASS = 0;
	LPC_CGU->XTAL_OSC_CTRL.HF = 0;
	LPC_CGU->XTAL_OSC_CTRL.ENABLE = 0;
	halPolledDelay(US2RTT(250));
}

void cpu_clock_max_speed() {
	/* Incantation from LPC43xx UM10503 section 12.2.1.1, to bring the M4
	 * core clock speed to the 110 - 204MHz range.
	 */
	cpu_clock_irc();

	cpu_xtal_start();

	/* Step into the 90-110MHz M4 clock range */
	cgu::pll1::ctrl({
		.pd = 0,
		.bypass = 0,
		.fbsel = 0,
		.direct = 0,
		.psel = 0,
		.autoblock = 1,
		.nsel = 0,
		.msel = 16,
		.clk_sel = cgu::CLK_SEL::XTAL,
	});
	while( !cgu::pll1::is_locked() );

	/* Switch M4 clock to PLL1 running at intermediate rate */
	set_clock(LPC_CGU->BASE_M4_CLK, cgu::CLK_SEL::PLL1);
	systick_adjust_period(systick_count_pll1_step);
	halLPCSetSystemClock(clock_source_pll1_step_f);

	/* Delay >50us at 90-110MHz clock speed */
	halPolledDelay(US2RTT(50));

	/* Remove /2P divider from PLL1 output to achieve full speed */
	cgu::pll1::direct();
	systick_adjust_period(systick_count_pll1);
	halLPCSetSystemClock(clock_source_pll1_f);
}

void cpu_start_audio_pll() {
	cgu::pll0audio::ctrl({
		.pd = 1,
		.bypass = 0,
		.directi = 0,
		.directo = 0,
		.clken = 0,
		.frm = 0,
		.autoblock = 1,
		.pllfract_req = 1,
		.sel_ext = 0,
		.mod_pd = 0,
		.clk_sel = cgu::CLK_SEL::XTAL,
	});

	/* For 12MHz clock source, 48kHz audio rate, 256Fs MCLK:
	 * 		Fout=12.288MHz, Fcco=417.792MHz
	 *		PDEC=3, NDEC=1, PLLFRACT=0x1a1cac
	 */
	cgu::pll0audio::mdiv({
		.mdec = 0x5B6A,
	});
	cgu::pll0audio::np_div({
		.pdec = 3,
		.ndec = 1,
	});

	cgu::pll0audio::frac({
		.pllfract_ctrl = 0x1a1cac,
	});

	cgu::pll0audio::power_up();
	while( !cgu::pll0audio::is_locked() );
	cgu::pll0audio::clock_enable();

	set_clock(LPC_CGU->BASE_AUDIO_CLK, cgu::CLK_SEL::PLL0AUDIO);
}
