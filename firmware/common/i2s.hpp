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
};

struct ConfigRX {
	uint32_t dai;
	uint32_t rxrate;
	uint32_t rxbitrate;
	uint32_t rxmode;
};

struct ConfigDMA {
	uint32_t dma1;
	uint32_t dma2;
};

template<uint32_t BaseAddress>
class I2S {
public:
	static constexpr LPC_I2S_Type* Peripheral = reinterpret_cast<LPC_I2S_Type*>(BaseAddress);

	static void configure(
		const ConfigTX& config_tx,
		const ConfigRX& config_rx
	) {
		reset();

		/* I2S operates in master mode, use PLL0AUDIO as MCLK source for TX. */
		/* NOTE: Documentation of CREG6 is quite confusing. Refer to "I2S clocking and
		 * pin connections" and other I2S diagrams for more clarity.
		 */
		if( Peripheral == LPC_I2S0 ) {
			LPC_CREG->CREG6 |=
				  (1U << 12)
				| (1U << 13)
				;
		}
		if( Peripheral == LPC_I2S1 ) {
			LPC_CREG->CREG6 |=
				  (1U << 14)
				| (1U << 15)
				;
		}

		Peripheral->DAO = config_tx.dao;
		Peripheral->TXRATE = config_tx.txrate;
		Peripheral->TXBITRATE = config_tx.txbitrate;
		Peripheral->TXMODE = config_tx.txmode;

		Peripheral->DAI = config_rx.dai;
		Peripheral->RXRATE = config_rx.rxrate;
		Peripheral->RXBITRATE = config_rx.rxbitrate;
		Peripheral->RXMODE = config_rx.rxmode;
	}

	static void configure(
		const ConfigTX& config_tx,
		const ConfigRX& config_rx,
		const ConfigDMA& config_dma
	) {
		configure(config_tx, config_rx);

		Peripheral->DMA1 = config_dma.dma1;
		Peripheral->DMA2 = config_dma.dma2;
	}

	static void rx_start() {
		Peripheral->DAI &= ~(1U << 3);
	}

	static void rx_stop() {
		Peripheral->DAI |= (1U << 3);
	}

	static void tx_start() {
		Peripheral->DAO &= ~(1U << 3);
	}

	static void tx_stop() {
		Peripheral->DAO |= (1U << 3);
	}

	static void tx_mute() {
		Peripheral->DAO |= (1U << 15);
	}

	static void tx_unmute() {
		Peripheral->DAO &= ~(1U << 15);
	}

private:
	static void reset() {
		Peripheral->DAO |= (1U << 4);
		Peripheral->DAI |= (1U << 4);
	}
};

using i2s0 = I2S<LPC_I2S0_BASE>;
using i2s1 = I2S<LPC_I2S1_BASE>;

} /* namespace i2s */
} /* namespace lpc43xx */

#endif/*__I2S_H__*/
