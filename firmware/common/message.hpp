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
		FSKConfiguration = 6,
		FSKPacket = 7,
		Shutdown = 8,
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
	) : Message { ID::RSSIStatistics },
		statistics { }
	{
	}

	RSSIStatistics statistics;
};

struct BasebandStatistics {
	uint32_t idle_ticks { 0 };
	uint32_t baseband_ticks { 0 };
	bool saturation { false };
};

class BasebandStatisticsMessage : public Message {
public:
	constexpr BasebandStatisticsMessage(
	) : Message { ID::BasebandStatistics },
		statistics { }
	{
	}

	BasebandStatistics statistics;
};

struct ChannelStatistics {
	int32_t max_db;
	size_t count;

	constexpr ChannelStatistics(
	) : max_db { -120 },
		count { 0 }
	{
	}

	constexpr ChannelStatistics(
		int32_t max_db,
		size_t count
	) : max_db { max_db },
		count { count }
	{
	}
};

class ChannelStatisticsMessage : public Message {
public:
	constexpr ChannelStatisticsMessage(
	) : Message { ID::ChannelStatistics },
		statistics { }
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

struct FSKConfiguration {
	uint32_t symbol_rate;
	uint32_t access_code;
	size_t access_code_length;
	size_t access_code_tolerance;
	size_t packet_length;
};

class FSKConfigurationMessage : public Message {
public:
	constexpr FSKConfigurationMessage(
		FSKConfiguration configuration
	) : Message { ID::FSKConfiguration },
		configuration(configuration)
	{
	}

	FSKConfiguration configuration;
};

#include <bitset>

struct FSKPacket {
	std::bitset<256> payload;
	size_t bits_received;

	FSKPacket(
	) : bits_received { 0 }
	{
	}
};

class FSKPacketMessage : public Message {
public:
	FSKPacketMessage(
	) : Message { ID::FSKPacket }
	{
	}

	FSKPacket packet;
};

class ShutdownMessage : public Message {
public:
	constexpr ShutdownMessage(
	) : Message { ID::Shutdown }
	{
	}
};

class MessageHandlerMap {
public:
	using MessageHandler = std::function<void(const Message* const p)>;

	void register_handler(const Message::ID id, MessageHandler&& handler) {
		map_[toUType(id)] = std::move(handler);
	}

	void unregister_handler(const Message::ID id) {
		map_[toUType(id)] = nullptr;
	}

	void send(const Message* const message) {
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
