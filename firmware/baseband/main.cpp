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

#include "ch.h"
#include "test.h"

#include "lpc43xx_cpp.hpp"

#include "portapack_shared_memory.hpp"
#include "portapack_dma.hpp"
#include "gpdma.hpp"

#include "baseband.hpp"
#include "baseband_dma.hpp"

#include "event_m4.hpp"

#include "rssi.hpp"
#include "rssi_dma.hpp"

#include "touch_dma.hpp"

#include "dsp_decimate.hpp"
#include "dsp_demodulate.hpp"
#include "dsp_fft.hpp"
#include "dsp_fir_taps.hpp"
#include "dsp_iir.hpp"

#include "baseband_stats_collector.hpp"
#include "rssi_stats_collector.hpp"
#include "channel_stats_collector.hpp"
#include "audio_stats_collector.hpp"

#include "block_decimator.hpp"
#include "clock_recovery.hpp"
#include "access_code_correlator.hpp"
#include "packet_builder.hpp"

#include "message_queue.hpp"

#include "utility.hpp"

#include "debug.hpp"

#include "audio.hpp"
#include "audio_dma.hpp"

#include "gcc.hpp"

#include <cstdint>
#include <cstddef>
#include <array>
#include <string>
#include <bitset>
#include <math.h>

constexpr auto baseband_thread_priority = NORMALPRIO + 20;
constexpr auto rssi_thread_priority = NORMALPRIO + 10;

class ChannelDecimator {
public:
	enum class DecimationFactor {
		By4,
		By8,
		By16,
		By32,
	};

	ChannelDecimator(
		DecimationFactor f
	) : decimation_factor { f }
	{
	}

	void set_decimation_factor(const DecimationFactor f) {
		decimation_factor = f;
	}

	buffer_c16_t execute(buffer_c8_t buffer) {
		auto decimated = execute_decimation(buffer);

		return decimated;
	}

private:
	std::array<complex16_t, 1024> work_baseband;

	const buffer_c16_t work_baseband_buffer {
		work_baseband.data(),
		work_baseband.size()
	};
	const buffer_s16_t work_audio_buffer {
		(int16_t*)work_baseband.data(),
		sizeof(work_baseband) / sizeof(int16_t)
	};

	//const bool fs_over_4_downconvert = true;

	dsp::decimate::TranslateByFSOver4AndDecimateBy2CIC3 translate;
	//dsp::decimate::DecimateBy2CIC3 cic_0;
	dsp::decimate::DecimateBy2CIC3 cic_1;
	dsp::decimate::DecimateBy2CIC3 cic_2;
	dsp::decimate::DecimateBy2CIC3 cic_3;
	dsp::decimate::DecimateBy2CIC3 cic_4;

	DecimationFactor decimation_factor { DecimationFactor::By32 };

	buffer_c16_t execute_decimation(buffer_c8_t buffer) {
		/* 3.072MHz complex<int8_t>[2048], [-128, 127]
		 * -> Shift by -fs/4
		 * -> 3rd order CIC: -0.1dB @ 0.028fs, -1dB @ 0.088fs, -60dB @ 0.468fs
		 *                   -0.1dB @ 86kHz,   -1dB @ 270kHz,  -60dB @ 1.44MHz
		 * -> gain of 256
		 * -> decimation by 2
		 * -> 1.544MHz complex<int16_t>[1024], [-32768, 32512] */
		const auto stage_0_out = translate.execute(buffer, work_baseband_buffer);

		//if( fs_over_4_downconvert ) {
		//	// TODO:
		//} else {
		// Won't work until cic_0 will accept input type of buffer_c8_t.
		//	stage_0_out = cic_0.execute(buffer, work_baseband_buffer);
		//}

		/* 1.536MHz complex<int16_t>[1024], [-32768, 32512]
		 * -> 3rd order CIC: -0.1dB @ 0.028fs, -1dB @ 0.088fs, -60dB @ 0.468fs
		 *                   -0.1dB @ 43kHz,   -1dB @ 136kHz,  -60dB @ 723kHz
		 * -> gain of 8
		 * -> decimation by 2
		 * -> 768kHz complex<int16_t>[512], [-8192, 8128] */
		auto cic_1_out = cic_1.execute(stage_0_out, work_baseband_buffer);
		if( decimation_factor == DecimationFactor::By4 ) {
			return cic_1_out;
		}

		/* 768kHz complex<int16_t>[512], [-32768, 32512]
		 * -> 3rd order CIC decimation by 2, gain of 1
		 * -> 384kHz complex<int16_t>[256], [-32768, 32512] */
		auto cic_2_out = cic_2.execute(cic_1_out, work_baseband_buffer);
		if( decimation_factor == DecimationFactor::By8 ) {
			return cic_2_out;
		}

		/* 384kHz complex<int16_t>[256], [-32768, 32512]
		 * -> 3rd order CIC decimation by 2, gain of 1
		 * -> 192kHz complex<int16_t>[128], [-32768, 32512] */
		auto cic_3_out = cic_3.execute(cic_2_out, work_baseband_buffer);
		if( decimation_factor == DecimationFactor::By16 ) {
			return cic_3_out;
		}

		/* 192kHz complex<int16_t>[128], [-32768, 32512]
		 * -> 3rd order CIC decimation by 2, gain of 1
		 * -> 96kHz complex<int16_t>[64], [-32768, 32512] */
		auto cic_4_out = cic_4.execute(cic_3_out, work_baseband_buffer);

		return cic_4_out;
	}
};

static constexpr iir_biquad_config_t audio_hpf_config {
	{  0.93346032f, -1.86687724f,  0.93346032f },
	{  1.0f       , -1.97730264f,  0.97773668f }
};

static constexpr iir_biquad_config_t non_audio_hpf_config {
	{  0.51891061f, -0.95714180f,  0.51891061f },
	{  1.0f       , -0.79878302f,  0.43960231f }
};

class FMSquelch {
public:
	bool execute(buffer_s16_t audio) {
		// TODO: No hard-coded array size.
		std::array<int16_t, N> squelch_energy_buffer;
		const buffer_s16_t squelch_energy {
			squelch_energy_buffer.data(),
			squelch_energy_buffer.size()
		};
		non_audio_hpf.execute(audio, squelch_energy);

		uint64_t max_squared = 0;
		for(const auto sample : squelch_energy_buffer) {
			const uint64_t sample_squared = sample * sample;
			if( sample_squared > max_squared ) {
				max_squared = sample_squared;
			}
		}

		return (max_squared < (threshold * threshold));
	}

private:
	static constexpr size_t N = 32;
	static constexpr int16_t threshold = 3072;

	// nyquist = 48000 / 2.0
	// scipy.signal.iirdesign(wp=8000 / nyquist, ws= 4000 / nyquist, gpass=1, gstop=18, ftype='ellip')
	IIRBiquadFilter non_audio_hpf { non_audio_hpf_config };
};

static volatile bool channel_spectrum_request_update { false };
static std::array<complex16_t, 256> channel_spectrum;
static uint32_t channel_spectrum_sampling_rate { 0 };
static uint32_t channel_filter_pass_frequency { 0 };
static uint32_t channel_filter_stop_frequency { 0 };

class BasebandProcessor {
public:
	virtual ~BasebandProcessor() = default;

	virtual void execute(buffer_c8_t buffer) = 0;

protected:
	void feed_channel_stats(const buffer_c16_t channel) {
		channel_stats.feed(
			channel,
			[this](const ChannelStatistics statistics) {
				this->post_channel_stats_message(statistics);
			}
		);
	}

	void feed_channel_spectrum(
		const buffer_c16_t channel,
		const uint32_t filter_pass_frequency,
		const uint32_t filter_stop_frequency
	) {
		channel_filter_pass_frequency = filter_pass_frequency;
		channel_filter_stop_frequency = filter_stop_frequency;
		channel_spectrum_decimator.feed(
			channel,
			[this](const buffer_c16_t data) {
				this->post_channel_spectrum_message(data);
			}
		);
	}

	void fill_audio_buffer(const buffer_s16_t audio) {
		auto audio_buffer = audio::dma::tx_empty_buffer();
		for(size_t i=0; i<audio_buffer.count; i++) {
			audio_buffer.p[i].left = audio_buffer.p[i].right = audio.p[i];
		}
		i2s::i2s0::tx_unmute();

		feed_audio_stats(audio);
	}

private:
	BlockDecimator<256> channel_spectrum_decimator { 4 };

	ChannelStatsCollector channel_stats;
	ChannelStatisticsMessage channel_stats_message;

	AudioStatsCollector audio_stats;
	AudioStatisticsMessage audio_stats_message;

	void post_channel_stats_message(const ChannelStatistics statistics) {
		if( channel_stats_message.is_free() ) {
			channel_stats_message.statistics = statistics;
			shared_memory.application_queue.push(&channel_stats_message);
		}
	}

	void post_channel_spectrum_message(const buffer_c16_t data) {
		if( !channel_spectrum_request_update ) {
			channel_spectrum_request_update = true;
			std::copy(&data.p[0], &data.p[data.count], channel_spectrum.begin());
			channel_spectrum_sampling_rate = data.sampling_rate;
			events_flag(EVT_MASK_SPECTRUM);
		}
	}

	void feed_audio_stats(const buffer_s16_t audio) {
		audio_stats.feed(
			audio,
			[this](const AudioStatistics statistics) {
				this->post_audio_stats_message(statistics);
			}
		);
	}

	void post_audio_stats_message(const AudioStatistics statistics) {
		if( audio_stats_message.is_free() ) {
			audio_stats_message.statistics = statistics;
			shared_memory.application_queue.push(&audio_stats_message);
		}
	}
};

class NarrowbandAMAudio : public BasebandProcessor {
public:
	void execute(buffer_c8_t buffer) override {
		auto decimator_out = decimator.execute(buffer);

		const buffer_c16_t work_baseband_buffer {
			(complex16_t*)decimator_out.p,
			sizeof(*decimator_out.p) * decimator_out.count
		};

		/* 96kHz complex<int16_t>[64]
		 * -> FIR filter, <?kHz (0.???fs) pass, gain 1.0
		 * -> 48kHz int16_t[32] */
		auto channel = channel_filter.execute(decimator_out, work_baseband_buffer);

		// TODO: Feed channel_stats post-decimation data?
		feed_channel_stats(channel);
		feed_channel_spectrum(
			channel,
			decimator_out.sampling_rate * channel_filter_taps.pass_frequency_normalized,
			decimator_out.sampling_rate * channel_filter_taps.stop_frequency_normalized
		);

		const buffer_s16_t work_audio_buffer {
			(int16_t*)decimator_out.p,
			sizeof(*decimator_out.p) * decimator_out.count
		};

		/* 48kHz complex<int16_t>[32]
		 * -> AM demodulation
		 * -> 48kHz int16_t[32] */
		auto audio = demod.execute(channel, work_audio_buffer);

		audio_hpf.execute_in_place(audio);
		fill_audio_buffer(audio);
	}

private:
	ChannelDecimator decimator { ChannelDecimator::DecimationFactor::By32 };
	const fir_taps_real<64>& channel_filter_taps = taps_64_lp_031_070_tfilter;
	dsp::decimate::FIRAndDecimateBy2Complex<64> channel_filter { channel_filter_taps.taps };
	dsp::demodulate::AM demod;
	IIRBiquadFilter audio_hpf { audio_hpf_config };
};

class NarrowbandFMAudio : public BasebandProcessor {
public:
	void execute(buffer_c8_t buffer) override {
		/* Called every 2048/3072000 second -- 1500Hz. */

		auto decimator_out = decimator.execute(buffer);

		const buffer_c16_t work_baseband_buffer {
			(complex16_t*)decimator_out.p,
			sizeof(*decimator_out.p) * decimator_out.count
		};

		/* 96kHz complex<int16_t>[64]
		 * -> FIR filter, <6kHz (0.063fs) pass, gain 1.0
		 * -> 48kHz int16_t[32] */
		auto channel = channel_filter.execute(decimator_out, work_baseband_buffer);

		// TODO: Feed channel_stats post-decimation data?
		feed_channel_stats(channel);
		feed_channel_spectrum(
			channel,
			decimator_out.sampling_rate * channel_filter_taps.pass_frequency_normalized,
			decimator_out.sampling_rate * channel_filter_taps.stop_frequency_normalized
		);

		const buffer_s16_t work_audio_buffer {
			(int16_t*)decimator_out.p,
			sizeof(*decimator_out.p) * decimator_out.count
		};

		/* 48kHz complex<int16_t>[32]
		 * -> FM demodulation
		 * -> 48kHz int16_t[32] */
		auto audio = demod.execute(channel, work_audio_buffer);

		static uint64_t audio_present_history = 0;
		const auto audio_present_now = squelch.execute(audio);
		audio_present_history = (audio_present_history << 1) | (audio_present_now ? 1 : 0);
		const bool audio_present = (audio_present_history != 0);

		if( !audio_present ) {
			// Zero audio buffer.
			for(size_t i=0; i<audio.count; i++) {
				audio.p[i] = 0;
			}
		}

		audio_hpf.execute_in_place(audio);
		fill_audio_buffer(audio);
	}

private:
	ChannelDecimator decimator { ChannelDecimator::DecimationFactor::By32 };
	const fir_taps_real<64>& channel_filter_taps = taps_64_lp_042_078_tfilter;
	dsp::decimate::FIRAndDecimateBy2Complex<64> channel_filter { channel_filter_taps.taps };
	dsp::demodulate::FM demod { 48000, 7500 };

	IIRBiquadFilter audio_hpf { audio_hpf_config };
	FMSquelch squelch;
};

class WidebandFMAudio : public BasebandProcessor {
public:
	void execute(buffer_c8_t buffer) override {
		auto decimator_out = decimator.execute(buffer);

		const buffer_s16_t work_audio_buffer {
			(int16_t*)decimator_out.p,
			sizeof(*decimator_out.p) * decimator_out.count
		};

		auto channel = decimator_out;

		// TODO: Feed channel_stats post-decimation data?
		feed_channel_stats(channel);
		//feed_channel_spectrum(channel);

		/* 768kHz complex<int16_t>[512]
		 * -> FM demodulation
		 * -> 768kHz int16_t[512] */
		/* TODO: To improve adjacent channel rejection, implement complex channel filter:
		 *		pass < +/- 100kHz, stop > +/- 200kHz
		 */

		auto audio_oversampled = demod.execute(decimator_out, work_audio_buffer);

		/* 768kHz int16_t[512]
		 * -> 4th order CIC decimation by 2, gain of 1
		 * -> 384kHz int16_t[256] */
		auto audio_8fs = audio_dec_1.execute(audio_oversampled, work_audio_buffer);

		/* 384kHz int16_t[256]
		 * -> 4th order CIC decimation by 2, gain of 1
		 * -> 192kHz int16_t[128] */
		auto audio_4fs = audio_dec_2.execute(audio_8fs, work_audio_buffer);

		/* 192kHz int16_t[128]
		 * -> 4th order CIC decimation by 2, gain of 1
		 * -> 96kHz int16_t[64] */
		auto audio_2fs = audio_dec_3.execute(audio_4fs, work_audio_buffer);

		/* 96kHz int16_t[64]
		 * -> FIR filter, <15kHz (0.156fs) pass, >19kHz (0.198fs) stop, gain of 1
		 * -> 48kHz int16_t[32] */
		auto audio = audio_filter.execute(audio_2fs, work_audio_buffer);

		/* -> 48kHz int16_t[32] */
		audio_hpf.execute_in_place(audio);
		fill_audio_buffer(audio);
	}

private:
	ChannelDecimator decimator { ChannelDecimator::DecimationFactor::By4 };

	//dsp::decimate::FIRAndDecimateBy2Complex<64> channel_filter { taps_64_lp_031_070_tfilter };
	dsp::demodulate::FM demod { 768000, 75000 };
	dsp::decimate::DecimateBy2CIC4Real audio_dec_1;
	dsp::decimate::DecimateBy2CIC4Real audio_dec_2;
	dsp::decimate::DecimateBy2CIC4Real audio_dec_3;
	const fir_taps_real<64>& audio_filter_taps = taps_64_lp_156_198;
	dsp::decimate::FIR64AndDecimateBy2Real audio_filter { audio_filter_taps.taps };

	IIRBiquadFilter audio_hpf { audio_hpf_config };
};

class FSKProcessor : public BasebandProcessor {
public:
	FSKProcessor(
		MessageHandlerMap& message_handlers
	) : message_handlers(message_handlers)
	{
		message_handlers[Message::ID::FSKConfiguration] = [this](const Message* const p) {
			auto m = reinterpret_cast<const FSKConfigurationMessage*>(p);
			this->configure(m->configuration);
		};
	}

	~FSKProcessor() {
		message_handlers[Message::ID::FSKConfiguration] = nullptr;
	}

	void configure(const FSKConfiguration new_configuration) {
		clock_recovery.configure(new_configuration.symbol_rate, 76800);
		access_code_correlator.configure(
			new_configuration.access_code,
			new_configuration.access_code_length,
			new_configuration.access_code_tolerance
		);
		packet_builder.configure(new_configuration.packet_length);
	}

	void execute(buffer_c8_t buffer) override {
		/* 2.4576MHz, 2048 samples */

		auto decimator_out = decimator.execute(buffer);

		/* 153.6kHz, 128 samples */

		const buffer_c16_t work_baseband_buffer {
			(complex16_t*)decimator_out.p,
			decimator_out.count
		};

		/* 153.6kHz complex<int16_t>[128]
		 * -> FIR filter, <?kHz (?fs) pass, gain 1.0
		 * -> 76.8kHz int16_t[64] */
		auto channel = channel_filter.execute(decimator_out, work_baseband_buffer);

		/* 76.8kHz, 64 samples */
		feed_channel_stats(channel);
		feed_channel_spectrum(
			channel,
			decimator_out.sampling_rate * channel_filter_taps.pass_frequency_normalized,
			decimator_out.sampling_rate * channel_filter_taps.stop_frequency_normalized
		);

		const auto symbol_handler_fn = [this](const float value) {
			const uint_fast8_t symbol = (value >= 0.0f) ? 1 : 0;
			const bool access_code_found = this->access_code_correlator.execute(symbol);
			this->consume_symbol(symbol, access_code_found);
		};

		// 76.8k

		const buffer_s16_t work_demod_buffer {
			(int16_t*)decimator_out.p,
			decimator_out.count * sizeof(*decimator_out.p) / sizeof(int16_t)
		};

		auto demodulated = demod.execute(channel, work_demod_buffer);

		i2s::i2s0::tx_mute();

		for(size_t i=0; i<demodulated.count; i++) {
			clock_recovery.execute(demodulated.p[i], symbol_handler_fn);
		}
	}

private:
	ChannelDecimator decimator { ChannelDecimator::DecimationFactor::By16 };
	const fir_taps_real<64>& channel_filter_taps = taps_64_lp_031_070_tfilter;
	dsp::decimate::FIRAndDecimateBy2Complex<64> channel_filter { channel_filter_taps.taps };
	dsp::demodulate::FM demod { 76800, 9600 * 2 };

	ClockRecovery clock_recovery;
	AccessCodeCorrelator access_code_correlator;
	PacketBuilder packet_builder;

	FSKPacketMessage message;
	MessageHandlerMap& message_handlers;

	void consume_symbol(
		const uint_fast8_t symbol,
		const bool access_code_found
	) {
		const auto payload_handler_fn = [this](
			const std::bitset<256>& payload,
			const size_t bits_received
		) {
			this->payload_handler(payload, bits_received);
		};

		packet_builder.execute(
			symbol,
			access_code_found,
			payload_handler_fn
		);
	}

	void payload_handler(
		const std::bitset<256>& payload,
		const size_t bits_received
	) {
		if( message.is_free() ) {
			message.packet.payload = payload;
			message.packet.bits_received = bits_received;
			shared_memory.application_queue.push(&message);
		}
	}
};

static const int8_t sintab[1024] = {
0, 1, 2, 2, 3, 4, 5, 5, 6, 7, 8, 9, 9, 10, 11, 12, 12, 13, 14, 15, 16, 16, 17, 18, 19, 19, 20, 21, 22, 22, 23, 24, 25, 26, 26, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36, 37, 38, 38, 39, 40, 41, 41, 42, 43, 44, 44, 45, 46, 46, 47, 48, 49, 49, 50, 51, 51, 52, 53, 54, 54, 55, 56, 56, 57, 58, 58, 59, 60, 61, 61, 62, 63, 63, 64, 65, 65, 66, 67, 67, 68, 69, 69, 70, 71, 71, 72, 72, 73, 74, 74, 75, 76, 76, 77, 78, 78, 79, 79, 80, 81, 81, 82, 82, 83, 84, 84, 85, 85, 86, 86, 87, 88, 88, 89, 89, 90, 90, 91, 91, 92, 93, 93, 94, 94, 95, 95, 96, 96, 97, 97, 98, 98, 99, 99, 100, 100, 101, 101, 102, 102, 102, 103, 103, 104, 104, 105, 105, 106, 106, 106, 107, 107, 108, 108, 109, 109, 109, 110, 110, 111, 111, 111, 112, 112, 112, 113, 113, 113, 114, 114, 114, 115, 115, 115, 116, 116, 116, 117, 117, 117, 118, 118, 118, 118, 119, 119, 119, 120, 120, 120, 120, 121, 121, 121, 121, 122, 122, 122, 122, 122, 123, 123, 123, 123, 123, 124, 124, 124, 124, 124, 124, 125, 125, 125, 125, 125, 125, 125, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 125, 125, 125, 125, 125, 125, 125, 124, 124, 124, 124, 124, 124, 123, 123, 123, 123, 123, 122, 122, 122, 122, 122, 121, 121, 121, 121, 120, 120, 120, 120, 119, 119, 119, 118, 118, 118, 118, 117, 117, 117, 116, 116, 116, 115, 115, 115, 114, 114, 114, 113, 113, 113, 112, 112, 112, 111, 111, 111, 110, 110, 109, 109, 109, 108, 108, 107, 107, 106, 106, 106, 105, 105, 104, 104, 103, 103, 102, 102, 102, 101, 101, 100, 100, 99, 99, 98, 98, 97, 97, 96, 96, 95, 95, 94, 94, 93, 93, 92, 91, 91, 90, 90, 89, 89, 88, 88, 87, 86, 86, 85, 85, 84, 84, 83, 82, 82, 81, 81, 80, 79, 79, 78, 78, 77, 76, 76, 75, 74, 74, 73, 72, 72, 71, 71, 70, 69, 69, 68, 67, 67, 66, 65, 65, 64, 63, 63, 62, 61, 61, 60, 59, 58, 58, 57, 56, 56, 55, 54, 54, 53, 52, 51, 51, 50, 49, 49, 48, 47, 46, 46, 45, 44, 44, 43, 42, 41, 41, 40, 39, 38, 38, 37, 36, 35, 35, 34, 33, 32, 32, 31, 30, 29, 29, 28, 27, 26, 26, 25, 24, 23, 22, 22, 21, 20, 19, 19, 18, 17, 16, 16, 15, 14, 13, 12, 12, 11, 10, 9, 9, 8, 7, 6, 5, 5, 4, 3, 2, 2, 1, 0, -1, -2, -2, -3, -4, -5, -5, -6, -7, -8, -9, -9, -10, -11, -12, -12, -13, -14, -15, -16, -16, -17, -18, -19, -19, -20, -21, -22, -22, -23, -24, -25, -26, -26, -27, -28, -29, -29, -30, -31,
-32, -32, -33, -34, -35, -35, -36, -37, -38, -38, -39, -40, -41, -41, -42, -43, -44, -44, -45, -46, -46, -47, -48, -49, -49, -50, -51, -51, -52, -53, -54, -54, -55, -56, -56, -57, -58, -58, -59, -60, -61, -61, -62, -63, -63, -64, -65, -65, -66, -67, -67, -68, -69, -69, -70, -71, -71, -72, -72, -73, -74, -74, -75, -76, -76, -77, -78, -78, -79, -79, -80, -81, -81, -82, -82, -83, -84, -84, -85, -85, -86, -86, -87, -88, -88, -89, -89, -90, -90, -91, -91, -92, -93, -93, -94, -94, -95, -95, -96, -96, -97, -97, -98, -98, -99, -99, -100, -100, -101, -101, -102, -102, -102, -103, -103, -104, -104, -105, -105, -106, -106, -106, -107, -107, -108, -108, -109, -109, -109, -110, -110, -111, -111, -111, -112, -112, -112, -113, -113, -113, -114, -114, -114, -115, -115, -115, -116, -116, -116, -117, -117, -117, -118, -118, -118, -118, -119, -119, -119, -120, -120, -120, -120, -121, -121, -121, -121, -122, -122, -122, -122, -122, -123, -123, -123, -123, -123, -124, -124, -124, -124, -124, -124, -125, -125, -125, -125, -125, -125, -125, -126, -126, -126, -126, -126, -126, -126, -126, -126, -126, -126, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127,
-126, -126, -126, -126, -126, -126, -126, -126, -126, -126, -126, -125, -125, -125, -125, -125, -125, -125, -124, -124, -124, -124, -124, -124, -123, -123, -123, -123, -123, -122, -122, -122, -122, -122, -121, -121, -121, -121, -120, -120, -120, -120, -119, -119, -119, -118, -118, -118, -118, -117, -117, -117, -116, -116, -116, -115, -115, -115, -114, -114, -114, -113, -113, -113, -112, -112, -112, -111, -111, -111, -110, -110, -109, -109, -109, -108, -108, -107, -107, -106, -106, -106, -105, -105, -104, -104, -103, -103, -102, -102, -102, -101, -101, -100, -100, -99, -99, -98, -98, -97, -97, -96, -96, -95, -95, -94, -94, -93, -93, -92, -91, -91, -90, -90, -89, -89, -88, -88, -87, -86, -86, -85, -85, -84, -84, -83, -82, -82, -81, -81, -80, -79, -79, -78, -78, -77, -76, -76, -75, -74, -74, -73, -72, -72, -71, -71, -70, -69, -69, -68, -67, -67, -66, -65, -65, -64, -63, -63, -62, -61, -61, -60, -59, -58, -58, -57, -56, -56, -55, -54, -54, -53, -52, -51, -51, -50, -49, -49, -48, -47, -46, -46, -45, -44, -44, -43, -42, -41, -41, -40, -39, -38, -38, -37, -36, -35, -35, -34, -33, -32, -32, -31, -30, -29, -29, -28, -27, -26, -26, -25, -24, -23, -22, -22, -21, -20, -19, -19, -18, -17, -16, -16, -15, -14, -13, -12, -12, -11, -10, -9, -9, -8, -7, -6, -5, -5, -4, -3, -2, -2, -1
};

#define SAMPLES_PER_BIT 192
#define FILTER_SIZE 576
#define SAMPLE_BUFFER_SIZE SAMPLES_PER_BIT + FILTER_SIZE

static int32_t waveform_biphase[] = {
	165,167,168,168,167,166,163,160,
	157,152,147,141,134,126,118,109,
	99,88,77,66,53,41,27,14,
	0,-14,-29,-44,-59,-74,-89,-105,
	-120,-135,-150,-165,-179,-193,-206,-218,
	-231,-242,-252,-262,-271,-279,-286,-291,
	-296,-299,-301,-302,-302,-300,-297,-292,
	-286,-278,-269,-259,-247,-233,-219,-202,
	-185,-166,-145,-124,-101,-77,-52,-26,
	0,27,56,85,114,144,175,205,
	236,266,296,326,356,384,412,439,
	465,490,513,535,555,574,590,604,
	616,626,633,637,639,638,633,626,
	616,602,586,565,542,515,485,451,
	414,373,329,282,232,178,121,62,
	0,-65,-132,-202,-274,-347,-423,-500,
	-578,-656,-736,-815,-894,-973,-1051,-1128,
	-1203,-1276,-1347,-1415,-1479,-1540,-1596,-1648,
	-1695,-1736,-1771,-1799,-1820,-1833,-1838,-1835,
	-1822,-1800,-1767,-1724,-1670,-1605,-1527,-1437,
	-1334,-1217,-1087,-943,-785,-611,-423,-219,
	0,235,487,755,1040,1341,1659,1994,
	2346,2715,3101,3504,3923,4359,4811,5280,
	5764,6264,6780,7310,7856,8415,8987,9573,
	10172,10782,11404,12036,12678,13329,13989,14656,
	15330,16009,16694,17382,18074,18767,19461,20155,
	20848,21539,22226,22909,23586,24256,24918,25571,
	26214,26845,27464,28068,28658,29231,29787,30325,
	30842,31339,31814,32266,32694,33097,33473,33823,
	34144,34437,34699,34931,35131,35299,35434,35535,
	35602,35634,35630,35591,35515,35402,35252,35065,
	34841,34579,34279,33941,33566,33153,32702,32214,
	31689,31128,30530,29897,29228,28525,27788,27017,
	26214,25379,24513,23617,22693,21740,20761,19755,
	18725,17672,16597,15501,14385,13251,12101,10935,
	9755,8563,7360,6148,4927,3701,2470,1235,
	0,-1235,-2470,-3701,-4927,-6148,-7360,-8563,
	-9755,-10935,-12101,-13251,-14385,-15501,-16597,-17672,
	-18725,-19755,-20761,-21740,-22693,-23617,-24513,-25379,
	-26214,-27017,-27788,-28525,-29228,-29897,-30530,-31128,
	-31689,-32214,-32702,-33153,-33566,-33941,-34279,-34579,
	-34841,-35065,-35252,-35402,-35515,-35591,-35630,-35634,
	-35602,-35535,-35434,-35299,-35131,-34931,-34699,-34437,
	-34144,-33823,-33473,-33097,-32694,-32266,-31814,-31339,
	-30842,-30325,-29787,-29231,-28658,-28068,-27464,-26845,
	-26214,-25571,-24918,-24256,-23586,-22909,-22226,-21539,
	-20848,-20155,-19461,-18767,-18074,-17382,-16694,-16009,
	-15330,-14656,-13989,-13329,-12678,-12036,-11404,-10782,
	-10172,-9573,-8987,-8415,-7856,-7310,-6780,-6264,
	-5764,-5280,-4811,-4359,-3923,-3504,-3101,-2715,
	-2346,-1994,-1659,-1341,-1040,-755,-487,-235,
	0,219,423,611,785,943,1087,1217,
	1334,1437,1527,1605,1670,1724,1767,1800,
	1822,1835,1838,1833,1820,1799,1771,1736,
	1695,1648,1596,1540,1479,1415,1347,1276,
	1203,1128,1051,973,894,815,736,656,
	578,500,423,347,274,202,132,65,
	0,-62,-121,-178,-232,-282,-329,-373,
	-414,-451,-485,-515,-542,-565,-586,-602,
	-616,-626,-633,-638,-639,-637,-633,-626,
	-616,-604,-590,-574,-555,-535,-513,-490,
	-465,-439,-412,-384,-356,-326,-296,-266,
	-236,-205,-175,-144,-114,-85,-56,-27,
	0,26,52,77,101,124,145,166,
	185,202,219,233,247,259,269,278,
	286,292,297,300,302,302,301,299,
	296,291,286,279,271,262,252,242,
	231,218,206,193,179,165,150,135,
	120,105,89,74,59,44,29,14,
	0,-14,-27,-41,-53,-66,-77,-88,
	-99,-109,-118,-126,-134,-141,-147,-152,
	-157,-160,-163,-166,-167,-168,-168,-167
};

class RDSProcessor : public BasebandProcessor {
public:
	void execute(buffer_c8_t buffer) override {
        
		for (size_t i = 0; i<buffer.count; i++) {
			
			//Sample generation 2.28M/10=228kHz
			if(s >= 9) {
				s = 0;
				if(sample_count >= SAMPLES_PER_BIT) {
					cur_bit = (shared_memory.rdsdata[(bit_pos / 26) & 15]>>(25-(bit_pos % 26))) & 1;
					prev_output = cur_output;
					cur_output = prev_output ^ cur_bit;

					int32_t *src = waveform_biphase;
					int idx = in_sample_index;

					for(int j=0; j<FILTER_SIZE; j++) {
						val = (*src++);
						if (cur_output) val = -val;
						sample_buffer[idx++] += val;
						if (idx >= SAMPLE_BUFFER_SIZE) idx = 0;
					}

					in_sample_index += SAMPLES_PER_BIT;
					if (in_sample_index >= SAMPLE_BUFFER_SIZE) in_sample_index -= SAMPLE_BUFFER_SIZE;
					
					bit_pos++;
					sample_count = 0;
				}
				
				sample = sample_buffer[out_sample_index];
				sample_buffer[out_sample_index] = 0;
				out_sample_index++;
				if (out_sample_index >= SAMPLE_BUFFER_SIZE) out_sample_index = 0;
				
				//AM @ 228k/4=57kHz
				switch (mphase) {
					case 0:
					case 2: sample = 0; break;
					case 1: break;
					case 3: sample = -sample; break;
				}
				mphase++;
				if (mphase >= 4) mphase = 0;
				
				sample_count++;
			} else {
				s++;
			}
			
			//FM
			frq = (sample>>16) * 386760;
			
			phase = (phase + frq);
			sphase = phase + (256<<16);

			re = sintab[(sphase & 0x03FF0000)>>16];
			im = sintab[(phase & 0x03FF0000)>>16];
			
			buffer.p[i] = {(int8_t)re,(int8_t)im};
		}
	}

private:
	int8_t re, im;
	uint8_t mphase, s;
    uint32_t bit_pos;
    int32_t sample_buffer[SAMPLE_BUFFER_SIZE] = {0};
    int32_t val;
    uint8_t prev_output = 0;
    uint8_t cur_output = 0;
    uint8_t cur_bit = 0;
    int sample_count = SAMPLES_PER_BIT;
    int in_sample_index = 0;
    int32_t sample;
    int out_sample_index = SAMPLE_BUFFER_SIZE-1;
	uint32_t phase, sphase;
	int32_t sig, frq, frq_im, rdsc;
	int32_t k;
};

class LCRFSKProcessor : public BasebandProcessor {
public:
	void BasebandProcessor() {
		afsk_samples_per_bit = shared_memory.afsk_samples_per_bit;
		phase_inc_mark = shared_memory.afsk_phase_inc_mark;
		phase_inc_space = shared_memory.afsk_phase_inc_space;
	}
	
	void execute(buffer_c8_t buffer) override {
        
		for (size_t i = 0; i<buffer.count; i++) {
			
			//Sample generation 2.28M/10 = 228kHz
			if (s >= 9) {
				s = 0;
				
				if (sample_count >= afsk_samples_per_bit) {
					cur_byte = shared_memory.lcrdata[byte_pos];
					if (!cur_byte) {
						bit_pos = 0;
						byte_pos = 0;
						cur_byte = shared_memory.lcrdata[0];
					}
					cur_byte = (0x55<<1);	//DEBUG
					
					//SdddddddpD
					//0dddddddp1
					
					gbyte = 0;
					gbyte = cur_byte << 1;
					gbyte |= 1;
					
					cur_bit = gbyte >> (9-bit_pos) & 1;

					if (bit_pos == 9) {
						bit_pos = 0;
						byte_pos++;
					} else {
						bit_pos++;
					}
					
					//aphase = 0x2FFFFFF;
					
					sample_count = 0;
				} else {
					sample_count++;
				}
				if (cur_bit)
					aphase += phase_inc_mark; //(353205)
				else
					aphase += phase_inc_space; //(647542)
					
				sample = sintab[(aphase & 0x03FF0000)>>16];
			} else {
				s++;
			}
			
			sample = sintab[(aphase & 0x03FF0000)>>16];
			
			//FM
			frq = sample * 850; //TODO: Put in config (channel bandwidth)
			
			phase = (phase + frq);
			sphase = phase + (256<<16);

			re = sintab[(sphase & 0x03FF0000)>>16];
			im = sintab[(phase & 0x03FF0000)>>16];
			
			buffer.p[i] = {(int8_t)re,(int8_t)im};
		}
	}

private:
	uint32_t afsk_samples_per_bit;
	uint32_t phase_inc_mark, phase_inc_space;
	int8_t re, im;
	uint8_t s;
    uint8_t bit_pos, byte_pos;
    uint8_t cur_byte = 0;
    uint16_t gbyte;
    uint8_t cur_bit = 0;
    uint32_t sample_count;
	uint32_t aphase, phase, sphase;
	int32_t sample, sig, frq;
};

static BasebandProcessor* baseband_processor { nullptr };
static BasebandConfiguration baseband_configuration;

static baseband::Direction direction = baseband::Direction::Transmit;
		
static WORKING_AREA(baseband_thread_wa, 8192);

static __attribute__((noreturn)) msg_t baseband_fn(void *arg) {
	(void)arg;
	chRegSetThreadName("baseband");

	BasebandStatsCollector stats;
	BasebandStatisticsMessage message;
	
	while(true) {
		if (direction == baseband::Direction::Transmit) {
			const auto buffer_tmp = baseband::dma::wait_for_tx_buffer();
			
			const buffer_c8_t buffer {
				buffer_tmp.p, buffer_tmp.count, baseband_configuration.sampling_rate
			};

			if( baseband_processor ) {
				baseband_processor->execute(buffer);
			}

			stats.process(buffer,
				[&message](const BasebandStatistics statistics) {
					if( message.is_free() ) {
						message.statistics = statistics;
						shared_memory.application_queue.push(&message);
					}
				}
			);
		} else {
			const auto buffer_tmp = baseband::dma::wait_for_rx_buffer();
			
			const buffer_c8_t buffer {
				buffer_tmp.p, buffer_tmp.count, baseband_configuration.sampling_rate
			};

			if( baseband_processor ) {
				baseband_processor->execute(buffer);
			}

			stats.process(buffer,
				[&message](const BasebandStatistics statistics) {
					if( message.is_free() ) {
						message.statistics = statistics;
						shared_memory.application_queue.push(&message);
					}
				}
			);
		}
	}
}

static WORKING_AREA(rssi_thread_wa, 128);
static __attribute__((noreturn)) msg_t rssi_fn(void *arg) {
	(void)arg;
	chRegSetThreadName("rssi");

	RSSIStatisticsCollector stats;
	RSSIStatisticsMessage message;

	while(true) {
		// TODO: Place correct sampling rate into buffer returned here:
		const auto buffer_tmp = rf::rssi::dma::wait_for_buffer();
		const rf::rssi::buffer_t buffer {
			buffer_tmp.p, buffer_tmp.count, 400000
		};

		stats.process(
			buffer,
			[&message](const RSSIStatistics statistics) {
				if( message.is_free() ) {
					message.statistics = statistics;
					shared_memory.application_queue.push(&message);
				}
			}
		);
	}
}

extern "C" {

void __late_init(void) {
	/* After this call, scheduler, systick, heap, etc. are available. */
	/* By doing chSysInit() here, it runs before C++ constructors, which may
	 * require the heap.
	 */
	chSysInit();
}

}

static void init() {
	i2s::i2s0::configure(
		audio::i2s0_config_tx,
		audio::i2s0_config_rx,
		audio::i2s0_config_dma
	);

	audio::dma::init();
	audio::dma::configure();
	audio::dma::enable();

	i2s::i2s0::tx_start();
	i2s::i2s0::rx_start();

	LPC_CREG->DMAMUX = portapack::gpdma_mux;
	gpdma::controller.enable();
	nvicEnableVector(DMA_IRQn, CORTEX_PRIORITY_MASK(LPC_DMA_IRQ_PRIORITY));

	baseband::dma::init();

	rf::rssi::init();
	touch::dma::init();

	chThdCreateStatic(baseband_thread_wa, sizeof(baseband_thread_wa),
		baseband_thread_priority, baseband_fn,
		nullptr
	);
	
	chThdCreateStatic(rssi_thread_wa, sizeof(rssi_thread_wa),
		rssi_thread_priority, rssi_fn,
		nullptr
	);
}

class EventDispatcher {
public:
	MessageHandlerMap& message_handlers() {
		return message_map;
	}

	eventmask_t wait() {
		return chEvtWaitAny(ALL_EVENTS);
	}

	void dispatch(const eventmask_t events) {
		if( events & EVT_MASK_BASEBAND ) {
			handle_baseband_queue();
		}

		if( events & EVT_MASK_SPECTRUM ) {
			handle_spectrum();
		}
	}

private:
	MessageHandlerMap message_map;

	ChannelSpectrumMessage spectrum_message;
	std::array<uint8_t, 256> spectrum_db;

	void handle_baseband_queue() {
		while( !shared_memory.baseband_queue.is_empty() ) {
			auto message = shared_memory.baseband_queue.pop();

			auto& fn = message_map[message->id];
			if( fn ) {
				fn(message);
			}

			message->state = Message::State::Free;
		}
	}

	void handle_spectrum() {
		if( channel_spectrum_request_update ) {
			/* Decimated buffer is full. Compute spectrum. */
			std::array<std::complex<float>, 256> samples_swapped;
			fft_swap(channel_spectrum, samples_swapped);
			channel_spectrum_request_update = false;
			fft_c_preswapped(samples_swapped);
			if( spectrum_message.is_free() ) {
				for(size_t i=0; i<spectrum_db.size(); i++) {
					const auto mag2 = magnitude_squared(samples_swapped[i]);
					const float db = complex16_mag_squared_to_dbv_norm(mag2);
					constexpr float mag_scale = 5.0f;
					const unsigned int v = (db * mag_scale) + 255.0f;
					spectrum_db[i] = std::max(0U, std::min(255U, v));
				}

				/* TODO: Rename .db -> .magnitude, or something more (less!) accurate. */
				spectrum_message.spectrum.db = &spectrum_db;
				spectrum_message.spectrum.db_count = spectrum_db.size();
				spectrum_message.spectrum.sampling_rate = channel_spectrum_sampling_rate;
				spectrum_message.spectrum.channel_filter_pass_frequency = channel_filter_pass_frequency;
				spectrum_message.spectrum.channel_filter_stop_frequency = channel_filter_stop_frequency;
				shared_memory.application_queue.push(&spectrum_message);
			}
		}
	}
};

static void m0apptxevent_interrupt_enable() {
	nvicEnableVector(M0CORE_IRQn, CORTEX_PRIORITY_MASK(LPC43XX_M0APPTXEVENT_IRQ_PRIORITY));
}

extern "C" {

CH_IRQ_HANDLER(MAPP_IRQHandler) {
	CH_IRQ_PROLOGUE();

	chSysLockFromIsr();
	events_flag_isr(EVT_MASK_BASEBAND);
	chSysUnlockFromIsr();

	creg::m0apptxevent::clear();

	CH_IRQ_EPILOGUE();
}

}

std::array<baseband::sample_t, 8192> baseband_buffer;

int main(void) {
	init();

	events_initialize(chThdSelf());
	m0apptxevent_interrupt_enable();

	EventDispatcher event_dispatcher;
	auto& message_handlers = event_dispatcher.message_handlers();
	
	//const auto baseband_buffer = new std::array<baseband::sample_t, 8192>();

	message_handlers[Message::ID::BasebandConfiguration] = [&message_handlers](const Message* const p) {
		auto message = reinterpret_cast<const BasebandConfigurationMessage*>(p);
		if( message->configuration.mode != baseband_configuration.mode ) {

			// TODO: Timing problem around disabling DMA and nulling and deleting old processor
			auto old_p = baseband_processor;
			baseband_processor = nullptr;
			delete old_p;

			switch(message->configuration.mode) {
			case 1:
				direction = baseband::Direction::Receive;
				baseband_processor = new NarrowbandAMAudio();
				break;

			case 2:
				direction = baseband::Direction::Receive;
				baseband_processor = new NarrowbandFMAudio();
				break;

			case 3:
				direction = baseband::Direction::Receive;
				baseband_processor = new WidebandFMAudio();
				break;

			case 4:
				direction = baseband::Direction::Receive;
				baseband_processor = new FSKProcessor(message_handlers);
				break;
				
			case 15:
				direction = baseband::Direction::Transmit;
				baseband_processor = new RDSProcessor();
				break;
				
			case 16:
				direction = baseband::Direction::Transmit;
				baseband_processor = new LCRFSKProcessor();
				break;

			default:
				break;
			}

			if( baseband_processor ) {
				if( direction == baseband::Direction::Receive ) {
					rf::rssi::start();
				}
				baseband::dma::enable(direction);
			} else {
				baseband::dma::disable();
				rf::rssi::stop();
			}
		}
		
		baseband::dma::configure(
			baseband_buffer.data(),
			direction
		);

		baseband_configuration = message->configuration;
	};

	/* TODO: Ensure DMAs are configured to point at first LLI in chain. */

	rf::rssi::dma::allocate(4, 400);

	touch::dma::allocate();
	touch::dma::enable();

	baseband::dma::configure(
		baseband_buffer.data(),
		direction
	);
	//baseband::dma::allocate(4, 2048);d

	while(true) {
		const auto events = event_dispatcher.wait();
		event_dispatcher.dispatch(events);
	}
	
	return 0;
}
