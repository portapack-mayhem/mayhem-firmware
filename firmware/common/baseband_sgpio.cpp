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

#include "baseband_sgpio.hpp"

#include "baseband.hpp"
#include "utility.hpp"

namespace baseband {

/*

struct PinConfig {
	P_OUT_CFG p_out_cfg;
	P_OE_CFG p_oe_cfg { P_OE_CFG::GPIO_OE };

	constexpr SGPIOPinConfig(
		P_OUT_CFG p_out_cfg
	) :
		p_out_cfg(p_out_cfg)
	{
	}
};

static constexpr bool slice_mode_multislice = false;

static constexpr P_OUT_CFG output_multiplexing_mode =
	slice_mode_multislice ? P_OUT_CFG::DOUT_DOUTM8C : P_OUT_CFG::DOUT_DOUTM8A;

static constexpr std::array<PinConfig, 16> pin_config { {
	[PIN_D0]		= { output_multiplexing_mode, SLICE_A },
	[PIN_D1]		= { output_multiplexing_mode, SLICE_I },
	[PIN_D2]		= { output_multiplexing_mode, },
	[PIN_D3]		= { output_multiplexing_mode, },
	[PIN_D4]		= { output_multiplexing_mode, },
	[PIN_D5]		= { output_multiplexing_mode, },
	[PIN_D6]		= { output_multiplexing_mode, },
	[PIN_D7]		= { output_multiplexing_mode, },
	[PIN_CLKIN]		= { P_OUT_CFG::DOUT_DOUTM1, },
	[PIN_CAPTURE]	= { P_OUT_CFG::DOUT_DOUTM1, },
	[PIN_DISABLE]	= { P_OUT_CFG::GPIO_OUT, },
	[PIN_DIRECTION]	= { P_OUT_CFG::GPIO_OUT, },
	[PIN_INVERT]	= { P_OUT_CFG::GPIO_OUT, },
	[PIN_DECIM0]	= { P_OUT_CFG::GPIO_OUT, },
	[PIN_DECIM1]	= { P_OUT_CFG::DOUT_DOUTM1, },
	[PIN_DECIM2]	= { P_OUT_CFG::GPIO_OUT, },
} };
*/
/*
static constexpr std::array<LPC_SGPIO_OUT_MUX_CFG_Type, 16> out_mux_cfg_receive {
	{ },
};

static constexpr std::array<LPC_SGPIO_OUT_MUX_CFG_Type, 16> out_mux_cfg_transmit {
	{ },
};
*/

enum class P_OUT_CFG : uint8_t {
	DOUT_DOUTM1 = 0x0,
	DOUT_DOUTM2A = 0x1,
	DOUT_DOUTM2B = 0x2,
	DOUT_DOUTM2C = 0x3,
	GPIO_OUT = 0x4,
	DOUT_DOUTM4A = 0x5,
	DOUT_DOUTM4B = 0x6,
	DOUT_DOUTM4C = 0x7,
	CLK_OUT = 0x8,
	DOUT_DOUTM8A = 0x9,
	DOUT_DOUTM8B = 0xa,
	DOUT_DOUTM8C = 0xb,
};

enum class P_OE_CFG : uint8_t {
	GPIO_OE = 0x0,
	DOUT_OEM1 = 0x4,
	DOUT_OEM2 = 0x5,
	DOUT_OEM4 = 0x6,
	DOUT_OEM8 = 0x7,
};

enum class CONCAT_ORDER : uint8_t {
	SELF_LOOP = 0x0,
	TWO_SLICES = 0x1,
	FOUR_SLICES = 0x2,
	EIGHT_SLICES = 0x3,
};

enum class CONCAT_ENABLE : uint8_t {
	EXTERNAL_DATA_PIN = 0x0,
	CONCATENATE_DATA = 0x1,
};

enum class CLK_CAPTURE_MODE : uint8_t {
	RISING_CLOCK_EDGE = 0,
	FALLING_CLOCK_EDGE = 1,
};

enum class PARALLEL_MODE : uint8_t {
	SHIFT_1_BIT_PER_CLOCK = 0x0,
	SHIFT_2_BITS_PER_CLOCK = 0x1,
	SHIFT_4_BITS_PER_CLOCK = 0x2,
	SHIFT_1_BYTE_PER_CLOCK = 0x3,
};

enum {
	PIN_D0 = 0,
	PIN_D1 = 1,
	PIN_D2 = 2,
	PIN_D3 = 3,
	PIN_D4 = 4,
	PIN_D5 = 5,
	PIN_D6 = 6,
	PIN_D7 = 7,
	PIN_CLKIN = 8,
	PIN_CAPTURE = 9,
	PIN_DISABLE = 10,
	PIN_DIRECTION = 11,
	PIN_INVERT = 12,
	PIN_SYNC_EN = 13,
	PIN_P81 = 14,
	PIN_P78 = 15,
};

enum class Slice : uint8_t {
	A =  0,
	B =  1,
	C =  2,
	D =  3,
	E =  4,
	F =  5,
	G =  6,
	H =  7,
	I =  8,
	J =  9,
	K = 10,
	L = 11,
	M = 12,
	N = 13,
	O = 14,
	P = 15,
};

constexpr bool slice_mode_multislice = false;

constexpr uint8_t pos_count_multi_slice = 0x1f;
constexpr uint8_t pos_count_single_slice = 0x03;

constexpr Slice slice_order[] {
	Slice::A,
	Slice::I,
	Slice::E,
	Slice::J,
	Slice::C,
	Slice::K,
	Slice::F,
	Slice::L,
	Slice::B,
	Slice::M,
	Slice::G,
	Slice::N,
	Slice::D,
	Slice::O,
	Slice::H,
	Slice::P,
};

constexpr uint32_t gpio_outreg(const Direction direction) {
	return ((direction == Direction::Transmit) ? (1U << PIN_DIRECTION) : 0U) | (1U << PIN_DISABLE);
}

constexpr uint32_t gpio_oenreg(const Direction direction) {
	return
		  (0U << PIN_P78)
		| (0U << PIN_P81)
		| (0U << PIN_SYNC_EN)
		| (0U << PIN_INVERT)
		| (1U << PIN_DIRECTION)
		| (1U << PIN_DISABLE)
		| (0U << PIN_CAPTURE)
		| (0U << PIN_CLKIN)
		| ((direction == Direction::Transmit) ? 0xffU : 0x00U)
		;
}

constexpr uint32_t out_mux_cfg(const P_OUT_CFG out, const P_OE_CFG oe) {
	return
		  (toUType(out) << 0)
		| (toUType(oe) << 4)
		;
}

constexpr uint32_t data_sgpio_mux_cfg(
	const CONCAT_ENABLE concat_enable,
	const CONCAT_ORDER concat_order
) {
	return
		  (1U << 0)
		| (0U << 1)
		| (0U << 3)
		| (3U << 5)
		| (1U << 7)
		| (0U << 9)
		| (toUType(concat_enable) << 11)
		| (toUType(concat_order) << 12)
		;
}

constexpr uint32_t data_slice_mux_cfg(
	const PARALLEL_MODE parallel_mode,
	const CLK_CAPTURE_MODE clk_capture_mode
) {
	return
		  (0U << 0)
		| (toUType(clk_capture_mode) << 1)
		| (1U << 2)
		| (0U << 3)
		| (0U << 4)
		| (toUType(parallel_mode) << 6)
		| (0U << 8)
		;
}

constexpr uint32_t pos(
	const uint32_t pos,
	const uint32_t pos_reset
) {
	return
		  (pos << 0)
		| (pos_reset << 8)
		;
}
constexpr uint32_t data_pos(
	const bool multi_slice
) {
	return pos(
		(multi_slice ? pos_count_multi_slice : pos_count_single_slice),
		(multi_slice ? pos_count_multi_slice : pos_count_single_slice)
	);
}

constexpr CONCAT_ENABLE data_concat_enable(
	const bool input_slice,
	const bool single_slice
) {
	return (input_slice || single_slice)
		? CONCAT_ENABLE::EXTERNAL_DATA_PIN
		: CONCAT_ENABLE::CONCATENATE_DATA
		;
}

constexpr CONCAT_ORDER data_concat_order(
	const bool input_slice,
	const bool single_slice
) {
	return (input_slice || single_slice)
		? CONCAT_ORDER::SELF_LOOP
		: CONCAT_ORDER::EIGHT_SLICES
		;
}

constexpr CLK_CAPTURE_MODE data_clk_capture_mode(
	const Direction direction
) {
	return (direction == Direction::Transmit)
		? CLK_CAPTURE_MODE::RISING_CLOCK_EDGE
		: CLK_CAPTURE_MODE::RISING_CLOCK_EDGE
		;
}

constexpr P_OUT_CFG data_p_out_cfg(
	const bool multi_slice
) {
	return (multi_slice)
		? P_OUT_CFG::DOUT_DOUTM8C
		: P_OUT_CFG::DOUT_DOUTM8A
		;
}

static const sgpio_resources_t sgpio_resources = {
	.base = { .clk = &LPC_CGU->BASE_PERIPH_CLK, .stat = &LPC_CCU1->BASE_STAT, .stat_mask = (1 << 6) },
	.branch = { .cfg = &LPC_CCU1->CLK_PERIPH_SGPIO_CFG, .stat = &LPC_CCU1->CLK_PERIPH_SGPIO_STAT },
	.reset = { .output_index = 57 },
};

void SGPIO::init() {
	base_clock_enable(&sgpio_resources.base);
	branch_clock_enable(&sgpio_resources.branch);
	peripheral_reset(&sgpio_resources.reset);
}

void SGPIO::configure(const Direction direction) {
	disable_all_slice_counters();

	// Set data pins as input, temporarily.
	LPC_SGPIO->GPIO_OENREG = gpio_oenreg(Direction::Receive);

	// Now that data pins are inputs, safe to change CPLD direction.
	LPC_SGPIO->GPIO_OUTREG = gpio_outreg(direction);

	LPC_SGPIO->OUT_MUX_CFG[ 8] = out_mux_cfg(P_OUT_CFG::DOUT_DOUTM1,	P_OE_CFG::GPIO_OE);
	LPC_SGPIO->OUT_MUX_CFG[ 9] = out_mux_cfg(P_OUT_CFG::DOUT_DOUTM1,	P_OE_CFG::GPIO_OE);
	LPC_SGPIO->OUT_MUX_CFG[10] = out_mux_cfg(P_OUT_CFG::GPIO_OUT,		P_OE_CFG::GPIO_OE);
	LPC_SGPIO->OUT_MUX_CFG[11] = out_mux_cfg(P_OUT_CFG::GPIO_OUT,		P_OE_CFG::GPIO_OE);
	LPC_SGPIO->OUT_MUX_CFG[12] = out_mux_cfg(P_OUT_CFG::GPIO_OUT,		P_OE_CFG::GPIO_OE);
	LPC_SGPIO->OUT_MUX_CFG[13] = out_mux_cfg(P_OUT_CFG::GPIO_OUT,		P_OE_CFG::GPIO_OE);
	LPC_SGPIO->OUT_MUX_CFG[14] = out_mux_cfg(P_OUT_CFG::DOUT_DOUTM1,	P_OE_CFG::GPIO_OE);
	LPC_SGPIO->OUT_MUX_CFG[15] = out_mux_cfg(P_OUT_CFG::GPIO_OUT,		P_OE_CFG::GPIO_OE);

	const auto data_out_mux_cfg = out_mux_cfg(data_p_out_cfg(slice_mode_multislice), P_OE_CFG::GPIO_OE);
	for(size_t i=0; i<8; i++) {
		LPC_SGPIO->OUT_MUX_CFG[i] = data_out_mux_cfg;
	}

	// Now that output enable sources are set, enable data bus in correct direction.
	LPC_SGPIO->GPIO_OENREG = gpio_oenreg(direction);

	const auto slice_gpdma = Slice::H;

	const size_t slice_count = slice_mode_multislice ? 8 : 1;
	const auto clk_capture_mode = data_clk_capture_mode(direction);
	const auto single_slice = !slice_mode_multislice;

	uint32_t slice_enable_mask = 0;
	for(size_t i=0; i<slice_count; i++) {
		const auto slice = slice_order[i];
		const auto slice_index = toUType(slice);
		const auto input_slice = (i == 0) && (direction != Direction::Transmit);
		const auto concat_order = data_concat_order(input_slice, single_slice);
		const auto concat_enable = data_concat_enable(input_slice, single_slice);

		LPC_SGPIO->SGPIO_MUX_CFG[slice_index] = data_sgpio_mux_cfg(
			concat_enable,
			concat_order
		);
		LPC_SGPIO->SLICE_MUX_CFG[slice_index] = data_slice_mux_cfg(
			PARALLEL_MODE::SHIFT_1_BYTE_PER_CLOCK,
			clk_capture_mode
		);

		LPC_SGPIO->PRESET[slice_index] = 0;
		LPC_SGPIO->COUNT[slice_index] = 0;
		LPC_SGPIO->POS[slice_index] = data_pos(slice_mode_multislice);
		LPC_SGPIO->REG[slice_index] = 0;
		LPC_SGPIO->REG_SS[slice_index] = 0;

		slice_enable_mask |= (1U << slice_index);
	}

	if( !slice_mode_multislice ) {
		const auto slice_index = toUType(slice_gpdma);

		LPC_SGPIO->SGPIO_MUX_CFG[slice_index] = data_sgpio_mux_cfg(
			CONCAT_ENABLE::CONCATENATE_DATA,
			CONCAT_ORDER::SELF_LOOP
		);
		LPC_SGPIO->SLICE_MUX_CFG[slice_index] = data_slice_mux_cfg(
			PARALLEL_MODE::SHIFT_1_BIT_PER_CLOCK,
			clk_capture_mode
		);

		LPC_SGPIO->PRESET[slice_index] = 0;
		LPC_SGPIO->COUNT[slice_index] = 0;
		LPC_SGPIO->POS[slice_index] = pos(0x1f, 0x1f);
		LPC_SGPIO->REG[slice_index] = 0x11111111;
		LPC_SGPIO->REG_SS[slice_index] = 0x11111111;

		slice_enable_mask |= (1 << slice_index);
	}

	set_slice_counter_enables(slice_enable_mask);
}

} /* namespace baseband */
