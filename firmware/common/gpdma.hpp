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

#ifndef __GPDMA_H__
#define __GPDMA_H__

#include <cstdint>
#include <cstddef>
#include <array>

#include "hal.h"

#include "utility.hpp"

namespace lpc43xx {
namespace gpdma {

/* LPC43xx DMA appears to be the ARM PrimeCell(R) DMA Controller, or very
 * closely related.
 * More here: http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0196g/Chdcdaeb.html
 */

constexpr size_t buffer_words(const size_t bytes, const size_t word_size) {
	return (bytes + word_size - 1) / word_size;
}

using TCHandler = void (*)(void);
using ErrHandler = void (*)(void);

enum class FlowControl {
	MemoryToMemory_DMAControl = 0x0,
	MemoryToPeripheral_DMAControl = 0x1,
	PeripheralToMemory_DMAControl = 0x2,
	SourcePeripheralToDestinationPeripheral_DMAControl = 0x3,
	SourcePeripheralToDestinationPeripheral_DestinationControl = 0x4,
	MemoryToPeripheral_PeripheralControl = 0x5,
	PeripheralToMemory_PeripheralControl = 0x6,
	SourcePeripheralToDestinationPeripheral_SourceControl = 0x7,
};

static const uint_fast8_t flow_control_peripheral_source_map = 0b11011100;

constexpr uint_fast8_t source_endpoint_type(const FlowControl flow_control) {
	return (flow_control_peripheral_source_map >> toUType(flow_control)) & 1;
}

static const uint_fast8_t flow_control_peripheral_destination_map = 0b11111010;

constexpr uint_fast8_t destination_endpoint_type(const FlowControl flow_control) {
	return (flow_control_peripheral_destination_map >> toUType(flow_control)) & 1;
}

namespace mux {

enum class Peripheral0 {
	SPIFI = 0,
	SCT_CTOUT_2 = 1,
	SGPIO14 = 2,
	TIMER3_MATCH_1 = 3,
};

enum class Peripheral1 {
	TIMER0_MATCH_0 = 0,
	USART0_TX = 1,
};

enum class Peripheral2 {
	TIMER0_MATCH_1 = 0,
	USART0_RX = 1,
};

enum class Peripheral3 {
	TIMER1_MATCH_0 = 0,
	UART1_TX = 1,
	I2S1_DMAREQ_1 = 2,
	SSP1_TX = 3,
};

enum class Peripheral4 {
	TIMER1_MATCH_1 = 0,
	UART1_RX = 1,
	I2S1_DMAREQ_2 = 2,
	SSP1_RX = 3,
};

enum class Peripheral5 {
	TIMER2_MATCH_0 = 0,
	USART2_TX = 1,
	SSP1_TX = 2,
	SGPIO15 = 3,
};

enum class Peripheral6 {
	TIMER2_MATCH_1 = 0,
	USART2_RX = 1,
	SSP1_RX = 2,
	SGPIO14 = 3,
};

enum class Peripheral7 {
	TIMER3_MATCH_0 = 0,
	USART3_TX = 1,
	SCT_DMAREQ_0 = 2,
	ADCHS_WRITE = 3,
};

enum class Peripheral8 {
	TIMER3_MATCH_1 = 0,
	USART3_RX = 1,
	SCT_DMAREQ_1 = 2,
	ADCHS_READ = 3,
};

enum class Peripheral9 {
	SSP0_RX = 0,
	I2S0_DMAREQ_1 = 1,
	SCT_DMAREQ_1 = 2,
};

enum class Peripheral10 {
	SSP0_TX = 0,
	I2S0_DMAREQ_2 = 1,
	SCT_DMAREQ_0 = 2,
};

enum class Peripheral11 {
	SSP1_RX = 0,
	SGPIO14 = 1,
	USART0_TX = 2,
};

enum class Peripheral12 {
	SSP1_TX = 0,
	SGPIO15 = 1,
	USART0_RX = 2,
};

enum class Peripheral13 {
	ADC0 = 0,
	SSP1_RX = 2,
	USART3_RX = 3,
};

enum class Peripheral14 {
	ADC1 = 0,
	SSP1_TX = 2,
	USART3_TX = 3,
};

enum class Peripheral15 {
	DAC = 0,
	SCT_CTOUT_3 = 1,
	SGPIO15 = 2,
	TIMER3_MATCH_0 = 3,
};

struct MUX {
	Peripheral0 peripheral_0;
	Peripheral1 peripheral_1;
	Peripheral2 peripheral_2;
	Peripheral3 peripheral_3;
	Peripheral4 peripheral_4;
	Peripheral5 peripheral_5;
	Peripheral6 peripheral_6;
	Peripheral7 peripheral_7;
	Peripheral8 peripheral_8;
	Peripheral9 peripheral_9;
	Peripheral10 peripheral_10;
	Peripheral11 peripheral_11;
	Peripheral12 peripheral_12;
	Peripheral13 peripheral_13;
	Peripheral14 peripheral_14;
	Peripheral15 peripheral_15;

	constexpr operator uint32_t() const {
		return
			  (toUType(peripheral_0 ) <<  0)
			| (toUType(peripheral_1 ) <<  2)
			| (toUType(peripheral_2 ) <<  4)
			| (toUType(peripheral_3 ) <<  6)
			| (toUType(peripheral_4 ) <<  8)
			| (toUType(peripheral_5 ) << 10)
			| (toUType(peripheral_6 ) << 12)
			| (toUType(peripheral_7 ) << 14)
			| (toUType(peripheral_8 ) << 16)
			| (toUType(peripheral_9 ) << 18)
			| (toUType(peripheral_10) << 20)
			| (toUType(peripheral_11) << 22)
			| (toUType(peripheral_12) << 24)
			| (toUType(peripheral_13) << 26)
			| (toUType(peripheral_14) << 28)
			| (toUType(peripheral_15) << 30)
			;
	}
};

} /* namespace mux */

namespace channel {

struct LLI {
	uint32_t srcaddr;
	uint32_t destaddr;
	uint32_t lli;
	uint32_t control;
};

struct LLIPointer {
	uint32_t lm;
	uint32_t r;
	uint32_t lli;

	constexpr operator uint32_t() const {
		return
			  ((lm & 1) << 0)
			| ((r & 1) << 1)
			| (lli & 0xfffffffc)
			;
	}
};

struct Control {
	uint32_t transfersize;
	uint32_t sbsize;
	uint32_t dbsize;
	uint32_t swidth;
	uint32_t dwidth;
	uint32_t s;
	uint32_t d;
	uint32_t si;
	uint32_t di;
	uint32_t prot1;
	uint32_t prot2;
	uint32_t prot3;
	uint32_t i;

	constexpr operator uint32_t() const {
		return
			  ((transfersize & 0xfff) << 0)
			| ((sbsize & 7) << 12)
			| ((dbsize & 7) << 15)
			| ((swidth & 7) << 18)
			| ((dwidth & 7) << 21)
			| ((s & 1) << 24)
			| ((d & 1) << 25)
			| ((si & 1) << 26)
			| ((di & 1) << 27)
			| ((prot1 & 1) << 28)
			| ((prot2 & 1) << 29)
			| ((prot3 & 1) << 30)
			| ((i & 1) << 31)
			;
	}
};

struct Config {
	uint32_t e;
	uint32_t srcperipheral;
	uint32_t destperipheral;
	FlowControl flowcntrl;
	uint32_t ie;
	uint32_t itc;
	uint32_t l;
	uint32_t a;
	uint32_t h;

	constexpr operator uint32_t() const {
		return
			  ((e & 1) << 0)
			| ((srcperipheral & 0x1f) << 1)
			| ((destperipheral & 0x1f) << 6)
			| ((toUType(flowcntrl) & 7) << 11)
			| ((ie & 1) << 14)
			| ((itc & 1) << 15)
			| ((l & 1) << 16)
			| ((a & 1) << 17)
			| ((h & 1) << 18)
			;
	}
};

class Channel {
public:
	constexpr Channel(
		const size_t number
	) : number(number)
	{
	}

	void enable() const {
		LPC_GPDMA->CH[number].CONFIG |= (1U << 0);
	}

	bool is_enabled() const {
		return LPC_GPDMA->CH[number].CONFIG & (1U << 0);
	}

	void disable() const {
		LPC_GPDMA->CH[number].CONFIG &= ~(1U << 0);
	}

	void clear_interrupts() const {
		LPC_GPDMA->INTTCCLR = (1U << number);
		LPC_GPDMA->INTERRCLR = (1U << number);
	}

	void set_handlers(const TCHandler tc_handler, const ErrHandler err_handler) const;

	void configure(const LLI& first_lli, const uint32_t config) const;

	const LLI* next_lli() const {
		return reinterpret_cast<LLI*>(LPC_GPDMA->CH[number].LLI);
	}

private:
	const size_t number;
};

} /* namespace channel */

constexpr std::array<channel::Channel, 8> channels { {
	{ 0 }, { 1 }, { 2 }, { 3 },
	{ 4 }, { 5 }, { 6 }, { 7 },
} };

static const gpdma_resources_t gpdma_resources = {
  .base = { .clk = &LPC_CGU->BASE_M4_CLK, .stat = &LPC_CCU1->BASE_STAT, .stat_mask = (1 << 3) },
  .branch = { .cfg = &LPC_CCU1->CLK_M4_DMA_CFG, .stat = &LPC_CCU1->CLK_M4_DMA_STAT },
  .reset = { .output_index = 19 },
};

class Controller {
public:
	void enable() const {
		base_clock_enable(&gpdma_resources.base);
		branch_clock_enable(&gpdma_resources.branch);
		peripheral_reset(&gpdma_resources.reset);
		LPC_GPDMA->CONFIG |= (1U << 0);
	}

	void disable() const {
		for(const auto& channel : channels) {
			channel.disable();
		}
		LPC_GPDMA->CONFIG &= ~(1U << 0);
		peripheral_reset(&gpdma_resources.reset);
		branch_clock_disable(&gpdma_resources.branch);
		base_clock_disable(&gpdma_resources.base);
	}
};

constexpr Controller controller;

} /* namespace gpdma */
} /* namespace lpc43xx */

#endif/*__GPDMA_H__*/
