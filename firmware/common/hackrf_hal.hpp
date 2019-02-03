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

#ifndef __HACKRF_HAL_H__
#define __HACKRF_HAL_H__

#include <cstdint>

#include "adc.hpp"

using namespace lpc43xx;

namespace hackrf {
namespace one {

/* Clocks */

using ClockFrequency = uint32_t;

constexpr ClockFrequency si5351_xtal_f		= 25000000U;
constexpr ClockFrequency si5351_clkin_f		= 10000000U;

/* TODO: Use this many other places. */
/* TODO: M4/M0 and peripheral rates may be more PortaPack-specific? Move out
 * of HackRF header? */
constexpr ClockFrequency base_m4_clk_f		= 200000000U;
constexpr ClockFrequency base_m0_clk_f		= base_m4_clk_f;
constexpr ClockFrequency base_apb3_clk_f	= base_m4_clk_f;
constexpr ClockFrequency ssp1_pclk_f		= base_m4_clk_f;

constexpr ClockFrequency max5864_spi_f		= 20000000U;
constexpr ClockFrequency max2837_spi_f 		= 20000000U;

constexpr ClockFrequency rffc5072_reference_f	= 40000000U;
constexpr ClockFrequency max2837_reference_f	= 40000000U;
constexpr ClockFrequency mcu_clkin_f			= 40000000U;

constexpr uint8_t si5351_i2c_address = 0x60;

/* Clock Generator */

constexpr size_t clock_generator_output_codec		= 0;
constexpr size_t clock_generator_output_cpld		= 1;
constexpr size_t clock_generator_output_sgpio		= 2;
constexpr size_t clock_generator_output_clkout		= 3;
constexpr size_t clock_generator_output_first_if	= 4;
constexpr size_t clock_generator_output_second_if	= 5;
constexpr size_t clock_generator_output_mcu_clkin	= 7;

/* ADC0 */

using adc0 = adc::ADC<LPC_ADC0_BASE>;

/* ADC1 */

using adc1 = adc::ADC<LPC_ADC1_BASE>;

} /* namespace one */
} /* namespace hackrf */

#endif/*__HACKRF_HAL_H__*/
