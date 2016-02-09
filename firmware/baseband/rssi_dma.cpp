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

#include "rssi_dma.hpp"

#include <cstdint>
#include <cstddef>
#include <array>

#include "hal.h"
#include "gpdma.hpp"

using namespace lpc43xx;

#include "portapack_dma.hpp"
#include "portapack_adc.hpp"

#include "thread_wait.hpp"

namespace rf {
namespace rssi {
namespace dma {

/* TODO: SO MUCH REPEATED CODE IN touch_dma.cpp!!! */

static constexpr auto& gpdma_channel = gpdma::channels[portapack::adc1_gpdma_channel_number];

constexpr uint32_t gpdma_ahb_master_peripheral = 1;
constexpr uint32_t gpdma_ahb_master_memory = 0;
constexpr uint32_t gpdma_ahb_master_lli_fetch = 0;

constexpr uint32_t gpdma_peripheral = 0xe;
constexpr uint32_t gpdma_src_peripheral = gpdma_peripheral;
constexpr uint32_t gpdma_dest_peripheral = gpdma_peripheral;

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
		.sbsize = 0,  /* Burst size: 1 transfer */
		.dbsize = 0,  /* Burst size: 1 transfer */
		.swidth = 0,  /* Source transfer width: byte (8 bits) */
		.dwidth = 2,  /* Destination transfer width: word (32 bits) */
		.s = gpdma_ahb_master_peripheral,
		.d = gpdma_ahb_master_memory,
		.si = 0,
		.di = 1,
		.prot1 = 0,
		.prot2 = 0,
		.prot3 = 0,
		.i = 1,
	};
}

constexpr gpdma::channel::Config config() {
	return {
		.e = 0,
		.srcperipheral = gpdma_src_peripheral,
		.destperipheral = gpdma_dest_peripheral,
		.flowcntrl = gpdma::FlowControl::PeripheralToMemory_DMAControl,
		.ie = 1,
		.itc = 1,
		.l = 0,
		.a = 0,
		.h = 0,
	};
}

struct buffers_config_t {
	size_t count;
	size_t items_per_buffer;
};

static buffers_config_t			buffers_config;

static sample_t				*samples	{ nullptr };
static gpdma::channel::LLI	*lli		{ nullptr };

static ThreadWait thread_wait;

static void transfer_complete() {
	const auto next_lli_index = gpdma_channel.next_lli() - &lli[0];
	thread_wait.wake_from_interrupt(next_lli_index);
}

static void dma_error() {
	thread_wait.wake_from_interrupt(-1);
	disable();
}

void init() {
	gpdma_channel.set_handlers(transfer_complete, dma_error);

	// LPC_GPDMA->SYNC |= (1 << gpdma_peripheral);
}

void allocate(size_t buffer_count, size_t items_per_buffer) {
	buffers_config = {
		.count = buffer_count,
		.items_per_buffer = items_per_buffer,
	};

	const auto peripheral = reinterpret_cast<uint32_t>(&LPC_ADC1->DR[portapack::adc1_rssi_input]) + 1;
	const auto control_value = control(gpdma::buffer_words(buffers_config.items_per_buffer, 1));

	samples = new sample_t[buffers_config.count * buffers_config.items_per_buffer];
	lli = new gpdma::channel::LLI[buffers_config.count];

	for(size_t i=0; i<buffers_config.count; i++) {
		const auto memory = reinterpret_cast<uint32_t>(&samples[i * buffers_config.items_per_buffer]);
		lli[i].srcaddr = peripheral;
		lli[i].destaddr = memory;
		lli[i].lli = lli_pointer(&lli[(i + 1) % buffers_config.count]);
		lli[i].control = control_value;
	}
}

void free() {
	delete samples;
	delete lli;
}

void enable() {
	const auto gpdma_config = config();
	gpdma_channel.configure(lli[0], gpdma_config);
	gpdma_channel.enable();
}

bool is_enabled() {
	return gpdma_channel.is_enabled();
}

void disable() {
	gpdma_channel.disable();
}

rf::rssi::buffer_t wait_for_buffer() {
	const auto next_index = thread_wait.sleep();

	if( next_index >= 0 ) {
		const size_t free_index = (next_index + buffers_config.count - 2) % buffers_config.count;
		return { reinterpret_cast<sample_t*>(lli[free_index].destaddr), buffers_config.items_per_buffer };
	} else {
		// TODO: Should I return here, or loop if RDY_RESET?
		return { nullptr, 0 };
	}
}

} /* namespace dma */
} /* namespace rssi */
} /* namespace rf */
