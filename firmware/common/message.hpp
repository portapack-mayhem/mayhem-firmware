/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
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

#include "acars_packet.hpp"
#include "adsb_frame.hpp"
#include "ert_packet.hpp"
#include "pocsag_packet.hpp"
#include "sonde_packet.hpp"
#include "tpms_packet.hpp"
#include "jammer.hpp"
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
		Shutdown = 5,
		TPMSPacket = 6,
		ACARSPacket = 7,
		AISPacket = 8,
		ERTPacket = 9,
		SondePacket = 10,
		UpdateSpectrum = 11,
		NBFMConfigure = 12,
		WFMConfigure = 13,
		AMConfigure = 14,
		ChannelSpectrumConfig = 15,
		SpectrumStreamingConfig = 16,
		DisplaySleep = 17,
		CaptureConfig = 18,
		CaptureThreadDone = 19,
		ReplayConfig = 20,
		ReplayThreadDone = 21,
		AFSKRxConfigure = 22,
		StatusRefresh = 23,
		SamplerateConfig = 24,
		BTLERxConfigure = 25,
		NRFRxConfigure = 26,

		TXProgress = 30,
		Retune = 31,
		
		TonesConfigure = 32,
		AFSKTxConfigure = 33,
		PitchRSSIConfigure = 34,
		OOKConfigure = 35,
		RDSConfigure = 36,
		AudioTXConfig = 37,
		POCSAGConfigure = 38,
		DTMFTXConfig = 39,
		ADSBConfigure = 40,
		JammerConfigure = 41,
		WidebandSpectrumConfig = 42,
		FSKConfigure = 43,
		SSTVConfigure = 44,
		SigGenConfig = 43,
		SigGenTone = 44,
		
		POCSAGPacket = 45,
		ADSBFrame = 46,
		AFSKData = 47,
		TestAppPacket = 48,
		
		RequestSignal = 49,
		FIFOData = 50,
		
		AudioLevelReport = 51,
		CodedSquelch = 52,
		AudioSpectrum = 53,
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

class StatusRefreshMessage : public Message {
public:
	constexpr StatusRefreshMessage(
	) : Message { ID::StatusRefresh }
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

class WidebandSpectrumConfigMessage : public Message {
public:
	constexpr WidebandSpectrumConfigMessage (
		size_t sampling_rate,
		size_t trigger
	) : Message { ID::WidebandSpectrumConfig },
		sampling_rate { sampling_rate },
		trigger { trigger }
	{
	}

	size_t sampling_rate { 0 };
	size_t trigger { 0 };
};

struct AudioSpectrum {
	std::array<uint8_t, 128> db { { 0 } };
	//uint32_t sampling_rate { 0 };
};

class AudioSpectrumMessage : public Message {
public:
	constexpr AudioSpectrumMessage(
		AudioSpectrum* data
	) : Message { ID::AudioSpectrum },
		data { data }
	{
	}

	AudioSpectrum* data { nullptr };
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

class POCSAGPacketMessage : public Message {
public:
	constexpr POCSAGPacketMessage(
		const pocsag::POCSAGPacket& packet
	) : Message { ID::POCSAGPacket },
		packet { packet }
	{
	}
	
	pocsag::POCSAGPacket packet;
};

class ACARSPacketMessage : public Message {
public:
	constexpr ACARSPacketMessage(
		const baseband::Packet& packet
	) : Message { ID::ACARSPacket },
		packet { packet }
	{
	}

	baseband::Packet packet;
};

class ADSBFrameMessage : public Message {
public:
	constexpr ADSBFrameMessage(
		const adsb::ADSBFrame& frame
	) : Message { ID::ADSBFrame },
		frame { frame }
	{
	}
	
	adsb::ADSBFrame frame;
};

class AFSKDataMessage : public Message {
public:
	constexpr AFSKDataMessage(
		const bool is_data,
		const uint32_t value
	) : Message { ID::AFSKData },
		is_data { is_data },
		value { value }
	{
	}
	
	bool is_data;
	uint32_t value;
};

class CodedSquelchMessage : public Message {
public:
	constexpr CodedSquelchMessage(
		const uint32_t value
	) : Message { ID::CodedSquelch },
		value { value }
	{
	}
	
	uint32_t value;
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

class SondePacketMessage : public Message {
public:
	constexpr SondePacketMessage(
		const sonde::Packet::Type type,
		const baseband::Packet& packet
	) : Message { ID::SondePacket },
		type { type },
		packet { packet }
	{
	}

	sonde::Packet::Type type;

	baseband::Packet packet;
};

class TestAppPacketMessage : public Message {
public:
	constexpr TestAppPacketMessage(
		const baseband::Packet& packet
	) : Message { ID::TestAppPacket },
		packet { packet }
	{
	}

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
		const iir_biquad_config_t audio_deemph_config,
		const uint8_t squelch_level
	) : Message { ID::NBFMConfigure },
		decim_0_filter(decim_0_filter),
		decim_1_filter(decim_1_filter),
		channel_filter(channel_filter),
		channel_decimation { channel_decimation },
		deviation { deviation },
		audio_hpf_config(audio_hpf_config),
		audio_deemph_config(audio_deemph_config),
		squelch_level(squelch_level)
	{
	}

	const fir_taps_real<24> decim_0_filter;
	const fir_taps_real<32> decim_1_filter;
	const fir_taps_real<32> channel_filter;
	const size_t channel_decimation;
	const size_t deviation;
	const iir_biquad_config_t audio_hpf_config;
	const iir_biquad_config_t audio_deemph_config;
	const uint8_t squelch_level;
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
	
	size_t read(void* p, const size_t count) {
		const auto copy_size = std::min(used_, count);
		memcpy(p, &data_[capacity_ - used_], copy_size);
		used_ -= copy_size;
		return copy_size;
	}

	bool is_full() const {
		return used_ >= capacity_;
	}
	
	bool is_empty() const {
		return used_ == 0;
	}

	void* data() const {
		return data_;
	}

	size_t size() const {
		return used_;
	}
	
	size_t capacity() const {
		return capacity_;
	}

	void set_size(const size_t value) {
		used_ = value;
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

struct ReplayConfig {
	const size_t read_size;
	const size_t buffer_count;
	uint64_t baseband_bytes_received;
	FIFO<StreamBuffer*>* fifo_buffers_empty;
	FIFO<StreamBuffer*>* fifo_buffers_full;

	constexpr ReplayConfig(
		const size_t read_size,
		const size_t buffer_count
	) : read_size { read_size },
		buffer_count { buffer_count },
		baseband_bytes_received { 0 },
		fifo_buffers_empty { nullptr },
		fifo_buffers_full { nullptr }
	{
	}
};

class ReplayConfigMessage : public Message {
public:
	constexpr ReplayConfigMessage(
		ReplayConfig* const config
	) : Message { ID::ReplayConfig },
		config { config }
	{
	}

	ReplayConfig* const config;
};

class TXProgressMessage : public Message {
public:
	constexpr TXProgressMessage(
	) : Message { ID::TXProgress }
	{
	}
	
	uint32_t progress = 0;
	bool done = false;
};

class AFSKRxConfigureMessage : public Message {
public:
	constexpr AFSKRxConfigureMessage(
		const uint32_t baudrate,
		const uint32_t word_length,
		const uint32_t trigger_value,
		const bool trigger_word
	) : Message { ID::AFSKRxConfigure },
		baudrate(baudrate),
		word_length(word_length),
		trigger_value(trigger_value),
		trigger_word(trigger_word)
	{
	}
	
	const uint32_t baudrate;
	const uint32_t word_length;
	const uint32_t trigger_value;
	const bool trigger_word;
};

class BTLERxConfigureMessage : public Message {
public:
	constexpr BTLERxConfigureMessage(
		const uint32_t baudrate,
		const uint32_t word_length,
		const uint32_t trigger_value,
		const bool trigger_word
	) : Message { ID::BTLERxConfigure },
		baudrate(baudrate),
		word_length(word_length),
		trigger_value(trigger_value),
		trigger_word(trigger_word)
	{
    }
	const uint32_t baudrate;
	const uint32_t word_length;
	const uint32_t trigger_value;
	const bool trigger_word;
};

class NRFRxConfigureMessage : public Message {
public:
	constexpr NRFRxConfigureMessage(
		const uint32_t baudrate,
		const uint32_t word_length,
		const uint32_t trigger_value,
		const bool trigger_word
	) : Message { ID::NRFRxConfigure },
		baudrate(baudrate),
		word_length(word_length),
		trigger_value(trigger_value),
		trigger_word(trigger_word)
	{
    }
	const uint32_t baudrate;
	const uint32_t word_length;
	const uint32_t trigger_value;
	const bool trigger_word;
};

class PitchRSSIConfigureMessage : public Message {
public:
	constexpr PitchRSSIConfigureMessage(
		const bool enabled,
		const int32_t rssi
	) : Message { ID::PitchRSSIConfigure },
		enabled(enabled),
		rssi(rssi)
	{
	}
	
	const bool enabled;
	const int32_t rssi;
};

class TonesConfigureMessage : public Message {
public:
	constexpr TonesConfigureMessage(
		const uint32_t fm_delta,
		const uint32_t pre_silence,
		const uint16_t tone_count,
		const bool dual_tone,
		const bool audio_out
	) : Message { ID::TonesConfigure },
		fm_delta(fm_delta),
		pre_silence(pre_silence),
		tone_count(tone_count),
		dual_tone(dual_tone),
		audio_out(audio_out)
	{
	}

	const uint32_t fm_delta;
	const uint32_t pre_silence;
	const uint16_t tone_count;
	const bool dual_tone;
	const bool audio_out;
};

class RDSConfigureMessage : public Message {
public:
	constexpr RDSConfigureMessage(
		const uint16_t length
	) : Message { ID::RDSConfigure },
		length(length)
	{
	}
	
	const uint16_t length = 0;
};

class RetuneMessage : public Message {
public:
	constexpr RetuneMessage(
	) : Message { ID::Retune }
	{
	}
	
	int64_t freq = 0;
	uint32_t range = 0;
};

class SamplerateConfigMessage : public Message {
public:
	constexpr SamplerateConfigMessage(
		const uint32_t sample_rate
	) : Message { ID::SamplerateConfig },
		sample_rate(sample_rate)
	{
	}
	
	const uint32_t sample_rate = 0;
};

class AudioLevelReportMessage : public Message {
public:
	constexpr AudioLevelReportMessage(
	) : Message { ID::AudioLevelReport }
	{
	}
	
	uint32_t value = 0;
};

class AudioTXConfigMessage : public Message {
public:
	constexpr AudioTXConfigMessage(
		const uint32_t divider,
		const float deviation_hz,
		const float audio_gain,
		const uint32_t tone_key_delta,
		const float tone_key_mix_weight
	) : Message { ID::AudioTXConfig },
		divider(divider),
		deviation_hz(deviation_hz),
		audio_gain(audio_gain),
		tone_key_delta(tone_key_delta),
		tone_key_mix_weight(tone_key_mix_weight)
	{
	}

	const uint32_t divider;
	const float deviation_hz;
	const float audio_gain;
	const uint32_t tone_key_delta;
	const float tone_key_mix_weight;
};

class SigGenConfigMessage : public Message {
public:
	constexpr SigGenConfigMessage(
		const uint32_t bw,
		const uint32_t shape,
		const uint32_t duration
	) : Message { ID::SigGenConfig },
		bw(bw),
		shape(shape),
		duration(duration)
	{
	}

	const uint32_t bw;
	const uint32_t shape;
	const uint32_t duration;
};

class SigGenToneMessage : public Message {
public:
	constexpr SigGenToneMessage(
		const uint32_t tone_delta
	) : Message { ID::SigGenTone },
		tone_delta(tone_delta)
	{
	}

	const uint32_t tone_delta;
};

class AFSKTxConfigureMessage : public Message {
public:
	constexpr AFSKTxConfigureMessage(
		const uint32_t samples_per_bit,
		const uint32_t phase_inc_mark,
		const uint32_t phase_inc_space,
		const uint8_t repeat,
		const uint32_t fm_delta,
		const uint8_t symbol_count
	) : Message { ID::AFSKTxConfigure },
		samples_per_bit(samples_per_bit),
		phase_inc_mark(phase_inc_mark),
		phase_inc_space(phase_inc_space),
		repeat(repeat),
		fm_delta(fm_delta),
		symbol_count(symbol_count)
	{
	}

	const uint32_t samples_per_bit;
	const uint32_t phase_inc_mark;
	const uint32_t phase_inc_space;
	const uint8_t repeat;
	const uint32_t fm_delta;
	const uint8_t symbol_count;
};

class OOKConfigureMessage : public Message {
public:
	constexpr OOKConfigureMessage(
		const uint32_t stream_length,
		const uint32_t samples_per_bit,
		const uint8_t repeat,
		const uint32_t pause_symbols
	) : Message { ID::OOKConfigure },
		stream_length(stream_length),
		samples_per_bit(samples_per_bit),
		repeat(repeat),
		pause_symbols(pause_symbols)
	{
	}

	const uint32_t stream_length;
	const uint32_t samples_per_bit;
	const uint8_t repeat;
	const uint32_t pause_symbols;
};

class SSTVConfigureMessage : public Message {
public:
	constexpr SSTVConfigureMessage(
		const uint8_t vis_code,
		const uint32_t pixel_duration
	) : Message { ID::SSTVConfigure },
		vis_code(vis_code),
		pixel_duration(pixel_duration)
	{
	}

	const uint8_t vis_code;
	const uint32_t pixel_duration;
};

class FSKConfigureMessage : public Message {
public:
	constexpr FSKConfigureMessage(
		const uint32_t stream_length,
		const uint32_t samples_per_bit,
		const uint32_t shift,
		const uint32_t progress_notice
	) : Message { ID::FSKConfigure },
		stream_length(stream_length),
		samples_per_bit(samples_per_bit),
		shift(shift),
		progress_notice(progress_notice)
	{
	}

	const uint32_t stream_length;
	const uint32_t samples_per_bit;
	const uint32_t shift;
	const uint32_t progress_notice;
};

class POCSAGConfigureMessage : public Message {
public:
	constexpr POCSAGConfigureMessage(
		const pocsag::BitRate bitrate,
		const bool phase
	) : Message { ID::POCSAGConfigure },
		bitrate(bitrate),
		phase(phase)
	{
	}

	const pocsag::BitRate bitrate;
	const bool phase;
};

class ADSBConfigureMessage : public Message {
public:
	constexpr ADSBConfigureMessage(
		const uint32_t test
	) : Message { ID::ADSBConfigure },
		test(test)
	{
	}

	const uint32_t test;
};

class JammerConfigureMessage : public Message {
public:
	constexpr JammerConfigureMessage(
		const bool run,
		const jammer::JammerType type,
		const uint32_t speed
	) : Message { ID::JammerConfigure },
		run(run),
		type(type),
		speed(speed)
	{
	}

	const bool run;
	const jammer::JammerType type;
	const uint32_t speed;
};

class DTMFTXConfigMessage : public Message {
public:
	constexpr DTMFTXConfigMessage(
		const uint32_t bw,
		const uint32_t tone_length,
		const uint32_t pause_length
	) : Message { ID::DTMFTXConfig },
		bw(bw),
		tone_length(tone_length),
		pause_length(pause_length)
	{
	}

	const uint32_t bw;
	const uint32_t tone_length;
	const uint32_t pause_length;
};

// TODO: use streaming buffer instead
// TODO: rename (not only used for requests)
class RequestSignalMessage : public Message {
public:
	enum class Signal : char {
		FillRequest = 1,
		BeepRequest = 2,
		Squelched = 3
	};

	constexpr RequestSignalMessage(
		Signal signal
	) : Message { ID::RequestSignal },
		signal ( signal )
	{
	}

	Signal signal;
};

class FIFODataMessage : public Message {
public:
	constexpr FIFODataMessage(
		const int8_t * data
	) : Message { ID::FIFOData },
		data ( data )
	{
	}

	const int8_t * data;
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

class ReplayThreadDoneMessage : public Message {
public:
	constexpr ReplayThreadDoneMessage(
		uint32_t return_code = 0
	) : Message { ID::ReplayThreadDone },
		return_code { return_code }
	{
	}

	uint32_t return_code;
};

#endif/*__MESSAGE_H__*/
