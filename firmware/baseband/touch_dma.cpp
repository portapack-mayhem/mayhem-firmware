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

#include "touch_dma.hpp"

#include <cstdint>
#include <cstddef>
#include <array>

#include "hal.h"
#include "gpdma.hpp"
using namespace lpc43xx;

#include "portapack_dma.hpp"
#include "portapack_adc.hpp"
#include "portapack_shared_memory.hpp"

namespace touch {
namespace dma {


/* TODO: SO MUCH REPEATED CODE FROM rssi_dma.cpp!!! */

static constexpr auto& gpdma_channel = gpdma::channels[portapack::adc0_gpdma_channel_number];

constexpr uint32_t gpdma_ahb_master_peripheral = 1;
constexpr uint32_t gpdma_ahb_master_memory = 0;
constexpr uint32_t gpdma_ahb_master_lli_fetch = 0;

constexpr uint32_t gpdma_src_peripheral = 0xd;
constexpr uint32_t gpdma_dest_peripheral = 0xd;

constexpr gpdma::channel::LLIPointer lli_pointer(const void* lli) {
	return {
		.lm = gpdma_ahb_master_lli_fetch,
		.r = 0,
		.lli = reinterpret_cast<uint32_t>(lli),
	};
}

constexpr gpdma::channel::Control control(const size_t number_of_transfers) {
	return {
		.transfersize = number_of_transfers,
		.sbsize = 2,  /* Burst size: 8 transfers */
		.dbsize = 2,  /* Burst size: 8 transfers */
		.swidth = 2,  /* Source transfer width: word (32 bits) */
		.dwidth = 2,  /* Destination transfer width: word (32 bits) */
		.s = gpdma_ahb_master_peripheral,
		.d = gpdma_ahb_master_memory,
		.si = 1,
		.di = 1,
		.prot1 = 0,
		.prot2 = 0,
		.prot3 = 0,
		.i = 0,
	};
}

constexpr gpdma::channel::Config config() {
	return {
		.e = 0,
		.srcperipheral = gpdma_src_peripheral,
		.destperipheral = gpdma_dest_peripheral,
		.flowcntrl = gpdma::FlowControl::PeripheralToMemory_DMAControl,
		.ie = 0,
		.itc = 0,
		.l = 0,
		.a = 0,
		.h = 0,
	};
}

static gpdma::channel::LLI	lli;

constexpr size_t channels_per_sample = 8;
//constexpr size_t samples_per_frame = 40;
//constexpr size_t channel_samples_per_frame = channels_per_sample * samples_per_frame;

void init() {
}

void allocate() {
	//samples = new sample_t[channel_samples_per_frame];
	//lli = new gpdma::channel::LLI;
	lli.srcaddr = reinterpret_cast<uint32_t>(&LPC_ADC0->DR[0]);
	lli.destaddr = reinterpret_cast<uint32_t>(&shared_memory.touch_adc_frame.dr[0]);
	lli.lli = lli_pointer(&lli);
	lli.control = control(channels_per_sample);
}

void free() {
	//delete samples;
	//delete lli;
}

void enable() {
	const auto gpdma_config = config();
	gpdma_channel.configure(lli, gpdma_config);
	gpdma_channel.enable();
}

bool is_enabled() {
	return gpdma_channel.is_enabled();
}

void disable() {
	gpdma_channel.disable();
}

} /* namespace dma */
} /* namespace touch */
