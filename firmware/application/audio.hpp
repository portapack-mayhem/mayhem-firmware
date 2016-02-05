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

#ifndef __AUDIO_H__
#define __AUDIO_H__

#include "i2s.hpp"
using namespace lpc43xx;

namespace audio {

constexpr i2s::ConfigTX i2s0_config_tx {
	.dao = i2s::DAO {
		.wordwidth = i2s::WordWidth::Bits16,
		.mono = 0,
		.stop = 1,
		.reset = 0,
		.ws_sel = 0,
		.ws_halfperiod = 0x0f,
		.mute = 1,
	},
	.txrate = i2s::MCLKRate {
		.x_divider = 0,
		.y_divider = 0,
	},
	.txbitrate = i2s::BitRate {
		.bitrate = 7,
	},
	.txmode = i2s::Mode {
		.clksel = i2s::ClockSelect::BaseAudioClkOrExternalMCLK,
		.four_pin = 0,
		.mclk_out_en = 1,
	},
};

constexpr i2s::ConfigRX i2s0_config_rx {
	.dai = i2s::DAI {
		.wordwidth = i2s::WordWidth::Bits16,
		.mono = 0,
		.stop = 1,
		.reset = 0,
		.ws_sel = 1,
		.ws_halfperiod = 0x0f,
	},
	.rxrate = i2s::MCLKRate {
		.x_divider = 0,
		.y_divider = 0,
	},
	.rxbitrate = i2s::BitRate {
		.bitrate = 7,
	},
	.rxmode = i2s::Mode {
		.clksel = i2s::ClockSelect::BaseAudioClkOrExternalMCLK,
		.four_pin = 1,
		.mclk_out_en = 0,
	},
};

constexpr i2s::ConfigDMA i2s0_config_dma {
	.dma1 = i2s::DMA {
		.rx_enable = 1,
		.tx_enable = 0,
		.rx_depth = 4,
		.tx_depth = 0,
	},
	.dma2 = i2s::DMA {
		.rx_enable = 0,
		.tx_enable = 1,
		.rx_depth = 0,
		.tx_depth = 4,
	},
};

} /* namespace audio */

#endif/*__AUDIO_H__*/
