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

#ifndef __I2S_H__
#define __I2S_H__

#include "hal.h"

#include "utility.hpp"

namespace lpc43xx {
namespace i2s {

enum class WordWidth {
	Bits8 = 0x0,
	Bits16 = 0x1,
	Bits32 = 0x3,
};

enum class ClockSelect {
	FractionalDivider = 0x0,
	BaseAudioClkOrExternalMCLK = 0x01,
	OtherMCLK = 0x2,
};

struct DAO {
	WordWidth wordwidth;
	uint32_t mono;
	uint32_t stop;
	uint32_t reset;
	uint32_t ws_sel;
	uint32_t ws_halfperiod;
	uint32_t mute;

	constexpr operator uint32_t() const {
		return
			  ((toUType(wordwidth) & 3) << 0)
			| ((mono & 1) << 2)
			| ((stop & 1) << 3)
			| ((reset & 1) << 4)
			| ((ws_sel & 1) << 5)
			| ((ws_halfperiod & 0x1ff) << 6)
			| ((mute & 1) << 15)
			;
	}
};

struct DAI {
	WordWidth wordwidth;
	uint32_t mono;
	uint32_t stop;
	uint32_t reset;
	uint32_t ws_sel;
	uint32_t ws_halfperiod;

	constexpr operator uint32_t() const {
		return
			  ((toUType(wordwidth) & 3) << 0)
			| ((mono & 1) << 2)
			| ((stop & 1) << 3)
			| ((reset & 1) << 4)
			| ((ws_sel & 1) << 5)
			| ((ws_halfperiod & 0x1ff) << 6)
			;
	}
};

struct MCLKRate {
	uint32_t x_divider;
	uint32_t y_divider;

	constexpr operator uint32_t() const {
		return
			  ((y_divider & 0xff) << 0)
			| ((x_divider & 0xff) << 8)
			;
	}
};

struct BitRate {
	uint32_t bitrate;

	constexpr operator uint32_t() const {
		return ((bitrate & 0x3f) << 0);
	}
};

struct Mode {
	ClockSelect clksel;
	uint32_t four_pin;
	uint32_t mclk_out_en;

	constexpr operator uint32_t() const {
		return
			  ((toUType(clksel) & 3) << 0)
			| ((four_pin & 1) << 2)
			| ((mclk_out_en & 1) << 3)
			;
	}
};

struct DMA {
	uint32_t rx_enable;
	uint32_t tx_enable;
	size_t rx_depth;
	size_t tx_depth;

	constexpr operator uint32_t() const {
		return
			  ((rx_enable & 1) << 0)
			| ((tx_enable & 1) << 1)
			| ((rx_depth & 0xf) << 8)
			| ((tx_depth & 0xf) << 16)
			;
	}
};

struct ConfigTX {
	uint32_t dao;
	uint32_t txrate;
	uint32_t txbitrate;
	uint32_t txmode;
	uint32_t sck_in_sel;
};

struct ConfigRX {
	uint32_t dai;
	uint32_t rxrate;
	uint32_t rxbitrate;
	uint32_t rxmode;
	uint32_t sck_in_sel;
};

struct ConfigDMA {
	uint32_t dma1;
	uint32_t dma2;
};

static const audio_clock_resources_t audio_clock_resources = {
	.base = { .clk = &LPC_CGU->BASE_AUDIO_CLK, .stat = &LPC_CCU2->BASE_STAT, .stat_mask = 0 },
	.branch = { .cfg = &LPC_CCU2->CLK_AUDIO_CFG, .stat = &LPC_CCU2->CLK_AUDIO_STAT },
};

static const i2s_resources_t i2s_resources = {
	.base = { .clk = &LPC_CGU->BASE_APB1_CLK, .stat = &LPC_CCU1->BASE_STAT, .stat_mask = (1 << 1) },
	.branch = { .cfg = &LPC_CCU1->CLK_APB1_I2S_CFG, .stat = &LPC_CCU1->CLK_APB1_I2S_STAT },
	.reset = { { .output_index = 52 }, { .output_index = 53 } },
};

template<uint32_t BaseAddress>
class I2S {
public:
	static void configure(
		const ConfigTX& config_tx,
		const ConfigRX& config_rx
	) {
		base_clock_enable(&i2s_resources.base);
		branch_clock_enable(&i2s_resources.branch);

		base_clock_enable(&audio_clock_resources.base);
		branch_clock_enable(&audio_clock_resources.branch);

		if( &p() == LPC_I2S0 ) {
			peripheral_reset(&i2s_resources.reset[0]);
		}
		if( &p() == LPC_I2S1 ) {
			peripheral_reset(&i2s_resources.reset[1]);
		}

		reset();

		if( &p() == LPC_I2S0 ) {
			LPC_CREG->CREG6.I2S0_TX_SCK_IN_SEL = config_tx.sck_in_sel;
			LPC_CREG->CREG6.I2S0_RX_SCK_IN_SEL = config_rx.sck_in_sel;
		}
		if( &p() == LPC_I2S1 ) {
			LPC_CREG->CREG6.I2S1_TX_SCK_IN_SEL = config_tx.sck_in_sel;
			LPC_CREG->CREG6.I2S1_RX_SCK_IN_SEL = config_rx.sck_in_sel;
		}

		p().DAO = config_tx.dao;
		p().TXRATE = config_tx.txrate;
		p().TXBITRATE = config_tx.txbitrate;
		p().TXMODE = config_tx.txmode;

		p().DAI = config_rx.dai;
		p().RXRATE = config_rx.rxrate;
		p().RXBITRATE = config_rx.rxbitrate;
		p().RXMODE = config_rx.rxmode;
	}

	static void configure(
		const ConfigTX& config_tx,
		const ConfigRX& config_rx,
		const ConfigDMA& config_dma
	) {
		configure(config_tx, config_rx);

		p().DMA1 = config_dma.dma1;
		p().DMA2 = config_dma.dma2;
	}

	static void shutdown() {
		if( &p() == LPC_I2S0 ) {
			peripheral_reset(&i2s_resources.reset[0]);
		}
		if( &p() == LPC_I2S1 ) {
			peripheral_reset(&i2s_resources.reset[1]);
		}

		branch_clock_disable(&audio_clock_resources.branch);
		base_clock_disable(&audio_clock_resources.base);

		branch_clock_disable(&i2s_resources.branch);
		base_clock_disable(&i2s_resources.base);
	}

	static void rx_start() {
		p().DAI &= ~(1U << 3);
	}

	static void rx_stop() {
		p().DAI |= (1U << 3);
	}

	static void tx_start() {
		p().DAO &= ~(1U << 3);
	}

	static void tx_stop() {
		p().DAO |= (1U << 3);
	}

	static void tx_mute() {
		p().DAO |= (1U << 15);
	}

	static void tx_unmute() {
		p().DAO &= ~(1U << 15);
	}

private:
	static void reset() {
		p().DAO |= (1U << 4);
		p().DAI |= (1U << 4);
	}

	static LPC_I2S_Type& p() {
		return *reinterpret_cast<LPC_I2S_Type*>(BaseAddress);
	}
};

using i2s0 = I2S<LPC_I2S0_BASE>;
using i2s1 = I2S<LPC_I2S1_BASE>;

} /* namespace i2s */
} /* namespace lpc43xx */

#endif/*__I2S_H__*/
