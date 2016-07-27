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
#include <cstring>
#include <array>
#include <functional>
#include <algorithm>

#include "baseband_packet.hpp"
#include "ert_packet.hpp"
#include "tpms_packet.hpp"
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
		CaptureConfig = 17,
		CaptureThreadDone = 18,

		TXDone = 20,
		Retune = 21,
		XylosConfigure = 22,
		AFSKConfigure = 23,
		ModuleID = 24,
		FIFOSignal = 25,
		FIFOData = 26,
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

using ChannelSpectrumFIFO = FIFO<ChannelSpectrum>;

class ChannelSpectrumConfigMessage : public Message {
public:
	static constexpr size_t fifo_k = 2;
	
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
		const tpms::SignalType signal_type,
		const baseband::Packet& packet
	) : Message { ID::TPMSPacket },
		signal_type { signal_type },
		packet { packet }
	{
	}

	tpms::SignalType signal_type;
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

// TODO: Put this somewhere else, or at least the implementation part.
class StreamBuffer {
	uint8_t* data_;
	size_t used_;
	size_t capacity_;

public:
	constexpr StreamBuffer(
		void* const data = nullptr,
		const size_t capacity = 0
	) : data_ { static_cast<uint8_t*>(data) },
		used_ { 0 },
		capacity_ { capacity }
	{
	}

	size_t write(const void* p, const size_t count) {
		const auto copy_size = std::min(capacity_ - used_, count);
		memcpy(&data_[used_], p, copy_size);
		used_ += copy_size;
		return copy_size;
	}

	bool is_full() const {
		return used_ >= capacity_;
	}

	const void* data() const {
		return data_;
	}

	size_t size() const {
		return used_;
	}

	void empty() {
		used_ = 0;
	}
};

struct CaptureConfig {
	const size_t write_size;
	const size_t buffer_count;
	uint64_t baseband_bytes_received;
	uint64_t baseband_bytes_dropped;
	FIFO<StreamBuffer*>* fifo_buffers_empty;
	FIFO<StreamBuffer*>* fifo_buffers_full;

	constexpr CaptureConfig(
		const size_t write_size,
		const size_t buffer_count
	) : write_size { write_size },
		buffer_count { buffer_count },
		baseband_bytes_received { 0 },
		baseband_bytes_dropped { 0 },
		fifo_buffers_empty { nullptr },
		fifo_buffers_full { nullptr }
	{
	}

	size_t dropped_percent() const {
		if( baseband_bytes_dropped == 0 ) {
			return 0;
		} else {
			const size_t percent = baseband_bytes_dropped * 100U / baseband_bytes_received;
			return std::max(1U, percent);
		}
	}
};

class CaptureConfigMessage : public Message {
public:
	constexpr CaptureConfigMessage(
		CaptureConfig* const config
	) : Message { ID::CaptureConfig },
		config { config }
	{
	}

	CaptureConfig* const config;
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

class XylosConfigureMessage : public Message {
public:
	XylosConfigureMessage(
		const char data[]
	) : Message { ID::XylosConfigure }
	{
		memcpy(ccir_message, data, 21);
	}

	char ccir_message[21];
};

class RetuneMessage : public Message {
public:
	RetuneMessage(
	) : Message { ID::Retune }
	{
	}
	
	int64_t freq = 0;
};

class AFSKConfigureMessage : public Message {
public:
	AFSKConfigureMessage(
		const char data[],
		const uint32_t afsk_samples_per_bit,
		const uint32_t afsk_phase_inc_mark,
		const uint32_t afsk_phase_inc_space,
		const uint8_t afsk_repeat,
		const uint32_t afsk_bw,
		const bool afsk_alt_format
	) : Message { ID::AFSKConfigure },
		afsk_samples_per_bit(afsk_samples_per_bit),
		afsk_phase_inc_mark(afsk_phase_inc_mark),
		afsk_phase_inc_space(afsk_phase_inc_space),
		afsk_repeat(afsk_repeat),
		afsk_bw(afsk_bw),
		afsk_alt_format(afsk_alt_format)
	{
		memcpy(message_data, data, 256);
	}

	uint32_t afsk_samples_per_bit;
	uint32_t afsk_phase_inc_mark;
	uint32_t afsk_phase_inc_space;
	uint8_t afsk_repeat;
	uint32_t afsk_bw;
	bool afsk_alt_format;
	char message_data[256];
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

class CaptureThreadDoneMessage : public Message {
public:
	constexpr CaptureThreadDoneMessage(
		uint32_t error = 0
	) : Message { ID::CaptureThreadDone },
		error { error }
	{
	}

	uint32_t error;
};

#endif/*__MESSAGE_H__*/
