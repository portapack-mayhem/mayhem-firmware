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

#ifndef __PORTAPACK_DMA_H__
#define __PORTAPACK_DMA_H__

#include <cstddef>

#include "gpdma.hpp"
using namespace lpc43xx;

namespace portapack {

/* GPDMA */

/* DMA channel 0 has the highest priority and DMA channel 7 has the lowest
 * priority.
 */
/* Reserve channels 6 and 7 for memory-memory transfers. These channels
 * can't saturate AHB by design. */

constexpr size_t sgpio_gpdma_channel_number = 0;
constexpr size_t i2s0_tx_gpdma_channel_number = 2;
constexpr size_t i2s0_rx_gpdma_channel_number = 3;
constexpr size_t adc1_gpdma_channel_number = 4;
constexpr size_t adc0_gpdma_channel_number = 5;

constexpr gpdma::mux::MUX gpdma_mux {
	.peripheral_0  = gpdma::mux::Peripheral0::SGPIO14,
	.peripheral_1  = gpdma::mux::Peripheral1::TIMER0_MATCH_0,
	.peripheral_2  = gpdma::mux::Peripheral2::TIMER0_MATCH_1,
	.peripheral_3  = gpdma::mux::Peripheral3::TIMER1_MATCH_0,
	.peripheral_4  = gpdma::mux::Peripheral4::TIMER1_MATCH_1,
	.peripheral_5  = gpdma::mux::Peripheral5::TIMER2_MATCH_0,
	.peripheral_6  = gpdma::mux::Peripheral6::TIMER2_MATCH_1,
	.peripheral_7  = gpdma::mux::Peripheral7::TIMER3_MATCH_0,
	.peripheral_8  = gpdma::mux::Peripheral8::TIMER3_MATCH_1,
	.peripheral_9  = gpdma::mux::Peripheral9::I2S0_DMAREQ_1,
	.peripheral_10 = gpdma::mux::Peripheral10::I2S0_DMAREQ_2,
	.peripheral_11 = gpdma::mux::Peripheral11::SSP1_RX,
	.peripheral_12 = gpdma::mux::Peripheral12::SSP1_TX,
	.peripheral_13 = gpdma::mux::Peripheral13::ADC0,
	.peripheral_14 = gpdma::mux::Peripheral14::ADC1,
	.peripheral_15 = gpdma::mux::Peripheral15::DAC,
};

} /* namespace portapack */

#endif/*__PORTAPACK_DMA_H__*/
