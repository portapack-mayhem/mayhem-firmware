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

#ifndef __MESSAGE_H__
#define __MESSAGE_H__

#include <cstdint>
#include <cstddef>
#include <array>
#include <functional>

#include "utility.hpp"

#include "ch.h"

class Message {
public:
	static constexpr size_t MAX_SIZE = 276;

	enum class ID : uint32_t {
		/* Assign consecutive IDs. IDs are used to index array. */
		RSSIStatistics = 0,
		BasebandStatistics = 1,
		ChannelStatistics = 2,
		ChannelSpectrum = 3,
		AudioStatistics = 4,
		BasebandConfiguration = 5,
		TPMSPacket = 6,
		Shutdown = 8,
		AISPacket = 7,
		ERTPacket = 9,
		MAX
	};

	constexpr Message(
		ID id
	) : id { id }
	{
	}

	const ID id;
};

struct RSSIStatistics {
	uint32_t accumulator { 0 };
	uint32_t min { 0 };
	uint32_t max { 0 };
	uint32_t count { 0 };
};

class RSSIStatisticsMessage : public Message {
public:
	constexpr RSSIStatisticsMessage(
		const RSSIStatistics& statistics
	) : Message { ID::RSSIStatistics },
		statistics { statistics }
	{
	}

	RSSIStatistics statistics;
};

struct BasebandStatistics {
	uint32_t idle_ticks { 0 };
	uint32_t main_ticks { 0 };
	uint32_t rssi_ticks { 0 };
	uint32_t baseband_ticks { 0 };
	bool saturation { false };
};

class BasebandStatisticsMessage : public Message {
public:
	constexpr BasebandStatisticsMessage(
		const BasebandStatistics& statistics
	) : Message { ID::BasebandStatistics },
		statistics { statistics }
	{
	}

	BasebandStatistics statistics;
};

struct ChannelStatistics {
	int32_t max_db;
	size_t count;

	constexpr ChannelStatistics(
		int32_t max_db = -120,
		size_t count = 0
	) : max_db { max_db },
		count { count }
	{
	}
};

class ChannelStatisticsMessage : public Message {
public:
	constexpr ChannelStatisticsMessage(
	) : Message { ID::ChannelStatistics }
	{
	}

	ChannelStatistics statistics;
};

struct AudioStatistics {
	int32_t rms_db;
	int32_t max_db;
	size_t count;

	constexpr AudioStatistics(
	) : rms_db { -120 },
		max_db { -120 },
		count { 0 }
	{
	}

	constexpr AudioStatistics(
		int32_t rms_db,
		int32_t max_db,
		size_t count
	) : rms_db { rms_db },
		max_db { max_db },
		count { count }
	{
	}
};

class AudioStatisticsMessage : public Message {
public:
	constexpr AudioStatisticsMessage(
	) : Message { ID::AudioStatistics },
		statistics { }
	{
	}

	AudioStatistics statistics;
};

struct BasebandConfiguration {
	int32_t mode;
	uint32_t sampling_rate;
	size_t decimation_factor;

	constexpr BasebandConfiguration(
		int32_t mode = -1,
		uint32_t sampling_rate = 0,
		size_t decimation_factor = 1
	) : mode { mode },
		sampling_rate { sampling_rate },
		decimation_factor { decimation_factor }
	{
	}
};

class BasebandConfigurationMessage : public Message {
public:
	constexpr BasebandConfigurationMessage(
		BasebandConfiguration configuration
	) : Message { ID::BasebandConfiguration },
		configuration(configuration)
	{
	}

	BasebandConfiguration configuration;
};

struct ChannelSpectrum {
	std::array<uint8_t, 256> db { { 0 } };
	size_t db_count { 256 };
	uint32_t sampling_rate { 0 };
	uint32_t channel_filter_pass_frequency { 0 };
	uint32_t channel_filter_stop_frequency { 0 };
};

class ChannelSpectrumMessage : public Message {
public:
	constexpr ChannelSpectrumMessage(
	) : Message { ID::ChannelSpectrum }
	{
	}

	ChannelSpectrum spectrum;
};

#include <bitset>

struct AISPacket {
	std::bitset<1024> payload;
	size_t bits_received { 0 };
};

class AISPacketMessage : public Message {
public:
	constexpr AISPacketMessage(
	) : Message { ID::AISPacket }
	{
	}

	AISPacket packet;
};

struct TPMSPacket {
	std::bitset<1024> payload;
	size_t bits_received { 0 };
};

class TPMSPacketMessage : public Message {
public:
	constexpr TPMSPacketMessage(
	) : Message { ID::TPMSPacket }
	{
	}

	TPMSPacket packet;
};

class ShutdownMessage : public Message {
public:
	constexpr ShutdownMessage(
	) : Message { ID::Shutdown }
	{
	}
};

struct ERTPacket {
	uint64_t preamble { 0 };
	std::bitset<1024> payload;
	size_t bits_received { 0 };
};

class ERTPacketMessage : public Message {
public:
	constexpr ERTPacketMessage(
	) : Message { ID::ERTPacket }
	{
	}

	ERTPacket packet;
};

class MessageHandlerMap {
public:
	using MessageHandler = std::function<void(Message* const p)>;

	void register_handler(const Message::ID id, MessageHandler&& handler) {
		if( map_[toUType(id)] != nullptr ) {
			chDbgPanic("MsgDblReg");
		}
		map_[toUType(id)] = std::move(handler);
	}

	void unregister_handler(const Message::ID id) {
		map_[toUType(id)] = nullptr;
	}

	void send(Message* const message) {
		if( message->id < Message::ID::MAX ) {
			auto& fn = map_[toUType(message->id)];
			if( fn ) {
				fn(message);
			}
		}
	}

private:
	using MapType = std::array<MessageHandler, toUType(Message::ID::MAX)>;
	MapType map_;
};

#endif/*__MESSAGE_H__*/
