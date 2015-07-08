/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#include <cstdint>
#include <cstddef>

#include <vector>
#include <memory>

#include "gpdma.hpp"

namespace lpc43xx {
namespace gpdma {
namespace lli {

enum class ChainType : uint8_t {
	Loop = 0,
	OneShot = 1,
};

enum class Interrupt : uint8_t {
	All = 0,
	Last = 1,
};

struct ChainConfig {
	ChainType type;
	size_t length;
	Interrupt interrupt;
};

enum class BurstSize : uint8_t {
	Transfer1   = 0,
	Transfer4   = 1,
	Transfer8   = 2,
	Transfer16  = 3,
	Transfer32  = 4,
	Transfer64  = 5,
	Transfer128 = 6,
	Transfer256 = 7,
};

enum class TransferWidth : uint8_t {
	Byte = 0,
	HalfWord = 1,
	Word = 2,
};

enum class Increment : uint8_t {
	No = 0,
	Yes = 1,
};

using PeripheralIndex = uint8_t;

struct Endpoint {
	PeripheralIndex peripheral;
	BurstSize burst_size;
	TransferWidth transfer_size;
	Increment increment;
};

struct ChannelConfig {
	ChainConfig chain;
	FlowControl flow_control;
	Endpoint source;
	Endpoint destination;

	constexpr gpdma::channel::Control control(
		const size_t transfer_size,
		const bool last
	) {
		return {
			.transfersize = transfer_size,
			.sbsize = toUType(source.burst_size),
			.dbsize = toUType(destination.burst_size),
			.swidth = toUType(source.transfer_size),
			.dwidth = toUType(destination.transfer_size),
			.s = source_endpoint_type(flow_control),
			.d = destination_endpoint_type(flow_control),
			.si = toUType(source.increment),
			.di = toUType(destination.increment),
			.prot1 = 0,
			.prot2 = 0,
			.prot3 = 0,
			.i = ((chain.interrupt == Interrupt::All) || last) ? 1U : 0U,
		};
	}

	constexpr gpdma::channel::Config config() {
		return {
			.e = 0,
			.srcperipheral = source.peripheral,
			.destperipheral = destination.peripheral,
			.flowcntrl = flow_control,
			.ie = 1,
			.itc = 1,
			.l = 0,
			.a = 0,
			.h = 0,
		};
	};
};

constexpr ChannelConfig channel_config_baseband_tx {
	{ ChainType::Loop, 4, Interrupt::All },
	gpdma::FlowControl::MemoryToPeripheral_DMAControl,
	{ 0x00, BurstSize::Transfer1, TransferWidth::Word, Increment::Yes },
	{ 0x00, BurstSize::Transfer1, TransferWidth::Word, Increment::No  },
};

constexpr ChannelConfig channel_config_baseband_rx {
	{ ChainType::Loop, 4, Interrupt::All },
	gpdma::FlowControl::PeripheralToMemory_DMAControl,
	{ 0x00, BurstSize::Transfer1, TransferWidth::Word, Increment::No  },
	{ 0x00, BurstSize::Transfer1, TransferWidth::Word, Increment::Yes },
};

constexpr ChannelConfig channel_config_audio_tx {
	{ ChainType::Loop, 4, Interrupt::All },
	gpdma::FlowControl::MemoryToPeripheral_DMAControl,
	{ 0x0a, BurstSize::Transfer32, TransferWidth::Word, Increment::Yes },
	{ 0x0a, BurstSize::Transfer32, TransferWidth::Word, Increment::No  },
};

constexpr ChannelConfig channel_config_audio_rx {
	{ ChainType::Loop, 4, Interrupt::All },
	gpdma::FlowControl::PeripheralToMemory_DMAControl,
	{ 0x09, BurstSize::Transfer32, TransferWidth::Word, Increment::No  },
	{ 0x09, BurstSize::Transfer32, TransferWidth::Word, Increment::Yes },
};

constexpr ChannelConfig channel_config_rssi {
	{ ChainType::Loop, 4, Interrupt::All },
	gpdma::FlowControl::PeripheralToMemory_DMAControl,
	{ 0x0e, BurstSize::Transfer1, TransferWidth::Byte, Increment::No  },
	{ 0x0e, BurstSize::Transfer1, TransferWidth::Word, Increment::Yes },
};

class Chain {
public:
	using chain_t = std::vector<gpdma::channel::LLI>;
	using chain_p = std::unique_ptr<chain_t>;

	Chain(const ChannelConfig& cc) :
		chain(std::make_unique<chain_t>(cc.chain.length))
	{
		set_lli_sequential(cc.chain_type);
		set_source_address()...
	}

private:
	chain_p chain;

	void set_source_peripheral(void* const address) {
		set_source_address(address, 0);
	}

	void set_destination_peripheral(void* const address) {
		set_destination_address(address, 0);
	}

	void set_source_address(void* const address, const size_t increment) {
		size_t offset = 0;
		for(auto& item : *chain) {
			item.srcaddr = (uint32_t)address + offset;
			offset += increment;
		}
	}

	void set_destination_address(void* const address, const size_t increment) {
		size_t offset = 0;
		for(auto& item : *chain) {
			item.destaddr = (uint32_t)address + offset;
			offset += increment;
		}
	}

	void set_control(const gpdma::channel::Control control) {
		for(auto& item : *chain) {
			item.control = control;
		}
	}

	void set_lli_sequential(ChainType chain_type) {
		for(auto& item : *chain) {
			item.lli = lli_pointer(&item + 1);
		}
		if( chain_type == ChainType::Loop ) {
			chain[chain->size() - 1].lli = lli_pointer(&chain[0]);
		} else {
			chain[chain->size() - 1].lli = lli_pointer(nullptr);
		}
	}

	gpdma::channel::LLIPointer lli_pointer(const void* lli) {
		return {
			.lm = 0,
			.r = 0,
			.lli = reinterpret_cast<uint32_t>(lli),
		};
	}
};

} /* namespace lli */
} /* namespace gpdma */
} /* namespace lpc43xx */
