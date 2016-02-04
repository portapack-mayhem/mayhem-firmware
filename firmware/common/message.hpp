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

#include "baseband_packet.hpp"
#include "ert_packet.hpp"
#include "dsp_fir_taps.hpp"
#include "dsp_iir.hpp"
#include "fifo.hpp"

#include "utility.hpp"

#include "ch.h"

class Message {
public:
	static constexpr size_t MAX_SIZE = 512;

	enum class ID : uint32_t {
		/* Assign consecutive IDs. IDs are used to index array. */
		RSSIStatistics = 0,
		BasebandStatistics = 1,
		ChannelStatistics = 2,
		DisplayFrameSync = 3,
		AudioStatistics = 4,
		BasebandConfiguration = 5,
		TPMSPacket = 6,
		Shutdown = 8,
		AISPacket = 7,
		ERTPacket = 9,
		UpdateSpectrum = 10,
		NBFMConfigure = 11,
		WFMConfigure = 12,
		AMConfigure = 13,
		ChannelSpectrumConfig = 14,
		SpectrumStreamingConfig = 15,
		DisplaySleep = 16,
		TXDone = 17,
		Retune = 18,
		ReadyForSwitch = 19,
		AFSKData = 20,
		ModuleID = 21,
		FIFOSignal = 22,
		FIFOData = 23,
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
		const ChannelStatistics& statistics
	) : Message { ID::ChannelStatistics },
		statistics { statistics }
	{
	}

	ChannelStatistics statistics;
};

class DisplayFrameSyncMessage : public Message {
public:
	constexpr DisplayFrameSyncMessage(
	) : Message { ID::DisplayFrameSync }
	{
	}
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

class DisplaySleepMessage : public Message {
public:
	constexpr DisplaySleepMessage(
	) : Message { ID::DisplaySleep }
	{
	}
};

class AudioStatisticsMessage : public Message {
public:
	constexpr AudioStatisticsMessage(
		const AudioStatistics& statistics
	) : Message { ID::AudioStatistics },
		statistics { statistics }
	{
	}

	AudioStatistics statistics;
};

struct BasebandConfiguration {
	int32_t mode;
	uint32_t sampling_rate;
	size_t decimation_factor;

	constexpr BasebandConfiguration(
		int32_t mode,
		uint32_t sampling_rate,
		size_t decimation_factor = 1
	) : mode { mode },
		sampling_rate { sampling_rate },
		decimation_factor { decimation_factor }
	{
	}

	constexpr BasebandConfiguration(
	) : BasebandConfiguration { -1, 0, 1 }
	{
	}
};

class BasebandConfigurationMessage : public Message {
public:
	constexpr BasebandConfigurationMessage(
		const BasebandConfiguration& configuration
	) : Message { ID::BasebandConfiguration },
		configuration { configuration }
	{
	}

	BasebandConfiguration configuration;
};

class SpectrumStreamingConfigMessage : public Message {
public:
	enum class Mode : uint32_t {
		Stopped = 0,
		Running = 1,
	};

	constexpr SpectrumStreamingConfigMessage(
		Mode mode
	) : Message { ID::SpectrumStreamingConfig },
		mode { mode }
	{
	}

	Mode mode { Mode::Stopped };
};

struct ChannelSpectrum {
	std::array<uint8_t, 256> db { { 0 } };
	uint32_t sampling_rate { 0 };
	uint32_t channel_filter_pass_frequency { 0 };
	uint32_t channel_filter_stop_frequency { 0 };
};

using ChannelSpectrumFIFO = FIFO<ChannelSpectrum, 2>;

class ChannelSpectrumConfigMessage : public Message {
public:
	constexpr ChannelSpectrumConfigMessage(
		ChannelSpectrumFIFO* fifo
	) : Message { ID::ChannelSpectrumConfig },
		fifo { fifo }
	{
	}

	ChannelSpectrumFIFO* fifo { nullptr };
};

class AISPacketMessage : public Message {
public:
	constexpr AISPacketMessage(
		const baseband::Packet& packet
	) : Message { ID::AISPacket },
		packet { packet }
	{
	}

	baseband::Packet packet;
};

class TPMSPacketMessage : public Message {
public:
	constexpr TPMSPacketMessage(
		const baseband::Packet& packet
	) : Message { ID::TPMSPacket },
		packet { packet }
	{
	}

	baseband::Packet packet;
};

class ShutdownMessage : public Message {
public:
	constexpr ShutdownMessage(
	) : Message { ID::Shutdown }
	{
	}
};

class ERTPacketMessage : public Message {
public:
	constexpr ERTPacketMessage(
		const ert::Packet::Type type,
		const baseband::Packet& packet
	) : Message { ID::ERTPacket },
		type { type },
		packet { packet }
	{
	}

	ert::Packet::Type type;

	baseband::Packet packet;
};

class UpdateSpectrumMessage : public Message {
public:
	constexpr UpdateSpectrumMessage(
	) : Message { ID::UpdateSpectrum }
	{
	}
};

class NBFMConfigureMessage : public Message {
public:
	constexpr NBFMConfigureMessage(
		const fir_taps_real<24> decim_0_filter,
		const fir_taps_real<32> decim_1_filter,
		const fir_taps_real<32> channel_filter,
		const size_t channel_decimation,
		const size_t deviation,
		const iir_biquad_config_t audio_hpf_config,
		const iir_biquad_config_t audio_deemph_config
	) : Message { ID::NBFMConfigure },
		decim_0_filter(decim_0_filter),
		decim_1_filter(decim_1_filter),
		channel_filter(channel_filter),
		channel_decimation { channel_decimation },
		deviation { deviation },
		audio_hpf_config(audio_hpf_config),
		audio_deemph_config(audio_deemph_config)
	{
	}

	const fir_taps_real<24> decim_0_filter;
	const fir_taps_real<32> decim_1_filter;
	const fir_taps_real<32> channel_filter;
	const size_t channel_decimation;
	const size_t deviation;
	const iir_biquad_config_t audio_hpf_config;
	const iir_biquad_config_t audio_deemph_config;
};

class WFMConfigureMessage : public Message {
public:
	constexpr WFMConfigureMessage(
		const fir_taps_real<24> decim_0_filter,
		const fir_taps_real<16> decim_1_filter,
		const fir_taps_real<64> audio_filter,
		const size_t deviation,
		const iir_biquad_config_t audio_hpf_config,
		const iir_biquad_config_t audio_deemph_config
	) : Message { ID::WFMConfigure },
		decim_0_filter(decim_0_filter),
		decim_1_filter(decim_1_filter),
		audio_filter(audio_filter),
		deviation { deviation },
		audio_hpf_config(audio_hpf_config),
		audio_deemph_config(audio_deemph_config)
	{
	}

	const fir_taps_real<24> decim_0_filter;
	const fir_taps_real<16> decim_1_filter;
	const fir_taps_real<64> audio_filter;
	const size_t deviation;
	const iir_biquad_config_t audio_hpf_config;
	const iir_biquad_config_t audio_deemph_config;
};

class AMConfigureMessage : public Message {
public:
	enum class Modulation : int32_t {
		DSB = 0,
		SSB = 1,
	};

	constexpr AMConfigureMessage(
		const fir_taps_real<24> decim_0_filter,
		const fir_taps_real<32> decim_1_filter,
		const fir_taps_real<32> decim_2_filter,
		const fir_taps_complex<64> channel_filter,
		const Modulation modulation,
		const iir_biquad_config_t audio_hpf_config
	) : Message { ID::AMConfigure },
		decim_0_filter(decim_0_filter),
		decim_1_filter(decim_1_filter),
		decim_2_filter(decim_2_filter),
		channel_filter(channel_filter),
		modulation { modulation },
		audio_hpf_config(audio_hpf_config)
	{
	}

	const fir_taps_real<24> decim_0_filter;
	const fir_taps_real<32> decim_1_filter;
	const fir_taps_real<32> decim_2_filter;
	const fir_taps_complex<64> channel_filter;
	const Modulation modulation;
	const iir_biquad_config_t audio_hpf_config;
};

class TXDoneMessage : public Message {
public:
	TXDoneMessage(
	) : Message { ID::TXDone }
	{
	}
	
	int n = 0;
};

class ModuleIDMessage : public Message {
public:
	ModuleIDMessage(
	) : Message { ID::ModuleID }
	{
	}
	
	bool query;
	char md5_signature[16];
};

class ReadyForSwitchMessage : public Message {
public:
	ReadyForSwitchMessage(
	) : Message { ID::ReadyForSwitch }
	{
	}
};

class RetuneMessage : public Message {
public:
	RetuneMessage(
	) : Message { ID::Retune }
	{
	}
	
	int64_t freq = 0;
};

class AFSKDataMessage : public Message {
public:
	constexpr AFSKDataMessage(
	) : Message { ID::AFSKData }
	{
	}

	int16_t data[128] = {0};
};

class FIFOSignalMessage : public Message {
public:
	FIFOSignalMessage(
	) : Message { ID::FIFOSignal }
	{
	}

	char signaltype = 0;
};

class FIFODataMessage : public Message {
public:
	FIFODataMessage(
	) : Message { ID::FIFOData }
	{
	}

	int8_t * data;
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
