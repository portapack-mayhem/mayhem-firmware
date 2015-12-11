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

#include "baseband_dma.hpp"

#include <cstdint>
#include <cstddef>
#include <array>

#include "hal.h"
#include "gpdma.hpp"

using namespace lpc43xx;

#include "portapack_dma.hpp"

namespace baseband {
namespace dma {

constexpr uint32_t gpdma_ahb_master_sgpio = 0;
constexpr uint32_t gpdma_ahb_master_memory = 1;
constexpr uint32_t gpdma_ahb_master_lli_fetch = 0;

constexpr uint32_t gpdma_src_peripheral = 0x0;
constexpr uint32_t gpdma_dest_peripheral = 0x0;

constexpr gpdma::channel::LLIPointer lli_pointer(const void* lli) {
	return {
		.lm = gpdma_ahb_master_lli_fetch,
		.r = 0,
		.lli = reinterpret_cast<uint32_t>(lli),
	};
}

constexpr gpdma::channel::Control control(const baseband::Direction direction, const size_t buffer_words) {
	return {
		.transfersize = buffer_words,
		.sbsize = 0,  /* Burst size: 1 */
		.dbsize = 0,  /* Burst size: 1 */
		.swidth = 2,  /* Source transfer width: word (32 bits) */
		.dwidth = 2,  /* Destination transfer width: word (32 bits) */
		.s = (direction == baseband::Direction::Transmit) ? gpdma_ahb_master_memory : gpdma_ahb_master_sgpio,
		.d = (direction == baseband::Direction::Transmit) ? gpdma_ahb_master_sgpio : gpdma_ahb_master_memory,
		.si = (direction == baseband::Direction::Transmit) ? 1U : 0U,
		.di = (direction == baseband::Direction::Transmit) ? 0U : 1U,
		.prot1 = 0,
		.prot2 = 0,
		.prot3 = 0,
		.i = 1,
	};
}

constexpr gpdma::channel::Config config(const baseband::Direction direction) {
	return {
		.e = 0,
		.srcperipheral = gpdma_src_peripheral,
		.destperipheral = gpdma_dest_peripheral,
		.flowcntrl = (direction == baseband::Direction::Transmit)
			? gpdma::FlowControl::MemoryToPeripheral_DMAControl
			: gpdma::FlowControl::PeripheralToMemory_DMAControl,
		.ie = 1,
		.itc = 1,
		.l = 0,
		.a = 0,
		.h = 0,
	};
}

constexpr size_t buffer_samples_log2n = 13;
constexpr size_t buffer_samples = (1 << buffer_samples_log2n);
constexpr size_t transfers_per_buffer_log2n = 2;
constexpr size_t transfers_per_buffer = (1 << transfers_per_buffer_log2n);
constexpr size_t transfer_samples = buffer_samples / transfers_per_buffer;
constexpr size_t transfers_mask = transfers_per_buffer - 1;

constexpr size_t buffer_bytes = buffer_samples * sizeof(baseband::sample_t);
constexpr size_t transfer_bytes = transfer_samples * sizeof(baseband::sample_t);

constexpr size_t msg_count = transfers_per_buffer - 1;

static std::array<gpdma::channel::LLI, transfers_per_buffer> lli_loop;
static constexpr auto& gpdma_channel_sgpio = gpdma::channels[portapack::sgpio_gpdma_channel_number];

static Semaphore semaphore;

static volatile const gpdma::channel::LLI* next_lli = nullptr;

static void transfer_complete() {
	next_lli = gpdma_channel_sgpio.next_lli();
	chSemSignalI(&semaphore);
}

static void dma_error() {
	disable();
}

void init() {
	chSemInit(&semaphore, 0);
	gpdma_channel_sgpio.set_handlers(transfer_complete, dma_error);

	// LPC_GPDMA->SYNC |= (1 << gpdma_src_peripheral);
	// LPC_GPDMA->SYNC |= (1 << gpdma_dest_peripheral);
}

void configure(
	baseband::sample_t* const buffer_base,
	const baseband::Direction direction
) {
	const auto peripheral = reinterpret_cast<uint32_t>(&LPC_SGPIO->REG_SS[0]);
	const auto control_value = control(direction, gpdma::buffer_words(transfer_bytes, 4));
	for(size_t i=0; i<lli_loop.size(); i++) {
		const auto memory = reinterpret_cast<uint32_t>(&buffer_base[i * transfer_samples]);
		lli_loop[i].srcaddr = (direction == Direction::Transmit) ? memory : peripheral;
		lli_loop[i].destaddr = (direction == Direction::Transmit) ? peripheral : memory;
		lli_loop[i].lli = lli_pointer(&lli_loop[(i + 1) % lli_loop.size()]);
		lli_loop[i].control = control_value;
	}
}

void enable(const baseband::Direction direction) {
	const auto gpdma_config = config(direction);
	gpdma_channel_sgpio.configure(lli_loop[0], gpdma_config);

	chSemReset(&semaphore, 0);

	gpdma_channel_sgpio.enable();
}

bool is_enabled() {
	return gpdma_channel_sgpio.is_enabled();
}

void disable() {
	gpdma_channel_sgpio.disable_force();
}

baseband::buffer_t wait_for_rx_buffer() {
	const auto status = chSemWait(&semaphore);
	if( status == RDY_OK ) {
		const auto next = next_lli;
		if( next ) {
			const size_t next_index = next - &lli_loop[0];
			const size_t free_index = (next_index + transfers_per_buffer - 2) & transfers_mask;
			return { reinterpret_cast<sample_t*>(lli_loop[free_index].destaddr), transfer_samples };
		} else {
			return { };
		}
	} else {
		return { };
	}
}

} /* namespace dma */
} /* namespace baseband */
