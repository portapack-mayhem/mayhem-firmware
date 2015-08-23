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

static float waveform_biphase[] = {0.00253265133022, 0.00255504491037, 0.00256667102126, 0.0025672385497, 0.00255649674667, 0.00253423716573, 0.00250029547253, 0.00245455311551, 0.00239693884806, 0.00232743009314, 0.00224605414143, 0.00215288917468, 0.00204806510656, 0.00193176423352, 0.00180422168917, 0.00166572569587, 0.00151661760823, 0.00135729174364, 0.00118819499588, 0.00100982622839, 0.000822735444702, 0.000627522734284, 0.000424836992883, 0.000215374417197, -1.23225298158e-07, -0.000220870549765, -0.000446040728168, -0.000674767880774, -0.000906149680707, -0.00113925016637, -0.00137310275567, -0.00160671345499, -0.00183906425517, -0.00206911670572, -0.00229581565752, -0.00251809316382, -0.00273487252813, -0.00294507248686, -0.0031476115141, -0.00334141223473, -0.0035254059317, -0.00369853713255, -0.00385976825946, -0.00400808432674, -0.00414249766903, -0.00426205268297, -0.00436583056466, -0.00445295402495, -0.00452259196407, -0.00457396408696, -0.00460634544047, -0.00461907085337, -0.00461153926002, -0.00458321788861, -0.00453364629481, -0.00446244022186, -0.0043692952683, -0.00425399034471, -0.0041163909013, -0.00395645190841, -0.00377422057251, -0.0035698387709, -0.00334354518872, -0.00309567714275, -0.00282667207705, -0.00253706871632, -0.00222750786389, -0.00189873283191, -0.00155158949247, -0.0011870259394, -0.000806091751623, -0.000409936849938, 1.90059383236e-07, 0.00042294345989, 0.000856883423268, 0.00130047843362, 0.00175210863919, 0.00221006953169, 0.00267257605183, 0.00313776711824, 0.0036037105756, 0.00406840855594, 0.00452980324611, 0.00498578305229, 0.00543418915128, 0.00587282241677, 0.00629945070701, 0.00671181649912, 0.00710764485348, 0.00748465169059, 0.00784055236082, 0.00817307048659, 0.00847994705483, 0.00875894973632, 0.00900788240743, 0.00922459484812, 0.00940699258958, 0.00955304688301, 0.00966080476069, 0.00972839915915, 0.00975405907344, 0.00973611971083, 0.00967303261139, 0.00956337570235, 0.00940586325271, 0.00919935569384, 0.00894286927184, 0.00863558549687, 0.00827686035476, 0.00786623324593, 0.00740343561706, 0.00688839925073, 0.00632126417893, 0.00570238618641, 0.00503234387065, 0.0043119452256, 0.00354223371723, 0.00272449381977, 0.0018602559824, 0.000951300997274, -3.36259008787e-07, -0.000992363737284, -0.0020222298062, -0.00308712154498, -0.00418396376074, -0.00530941875093, -0.0064598868296, -0.00763150763453, -0.00882016223, -0.0100214760183, -0.011230822471, -0.0124433276877, -0.0136538757908, -0.0148571151577, -0.0160474654948, -0.0172191257514, -0.0183660828705, -0.0194821213717, -0.0205608337587, -0.0215956317392, -0.0225797582461, -0.0235063002445, -0.0243682023052, -0.0251582809259, -0.0258692395759, -0.0264936844398, -0.0270241408326, -0.0274530702548, -0.0277728880563, -0.0279759816737, -0.0280547294041, -0.0280015196764, -0.0278087707783, -0.0274689509968, -0.026974599126, -0.0263183452962, -0.0254929320749, -0.02449123579, -0.0233062880235, -0.0219312972225, -0.0203596703733, -0.0185850346832, -0.0166012592143, -0.0144024764108, -0.0119831034641, -0.00933786345535, -0.00646180621834, -0.0033503288631, 8.04098443393e-07, 0.00359544108347, 0.0074370242845, 0.0115285710889, 0.0158726561293, 0.0204713940206, 0.0253264228417, 0.0304388884133, 0.0358094294279, 0.041438163483, 0.0473246740673, 0.0534679985499, 0.0598666172177, 0.0665184434076, 0.0734208147742, 0.0805704857345, 0.0879636211267, 0.0955957911196, 0.103461967402, 0.111556520688, 0.119873219553, 0.128405230643, 0.137145120254, 0.146084857323, 0.155215817824, 0.164528790593, 0.174013984585, 0.18366103756, 0.193459026213, 0.203396477728, 0.213461382761, 0.223641209834, 0.233922921126, 0.244292989649, 0.254737417779, 0.265241757122, 0.275791129685, 0.286370250309, 0.296963450356, 0.307554702567, 0.318127647097, 0.328665618632, 0.339151674584, 0.349568624272, 0.359899059063, 0.37012538339, 0.380229846614, 0.39019457563, 0.400001608191, 0.40963292682, 0.419070493361, 0.428296283903, 0.437292324199, 0.446040725381, 0.45452371993, 0.462723697829, 0.470623242797, 0.478205168549, 0.485452554966, 0.492348784132, 0.498877576118, 0.505023024448, 0.510769631171, 0.516102341434, 0.521006577488, 0.525468272051, 0.529473900929, 0.533010514831, 0.536065770289, 0.538627959622, 0.540686039841, 0.542229660459, 0.543249190098, 0.543735741853, 0.543681197332, 0.543078229311, 0.541920322948, 0.540201795495, 0.537917814453, 0.535064414122, 0.5316385105, 0.527637914479, 0.523061343309, 0.51790843029, 0.512179732652, 0.505876737611, 0.499001866561, 0.491558477398, 0.483550864942, 0.474984259467, 0.46586482332, 0.456199645625, 0.445996735085, 0.43526501088, 0.424014291681, 0.412255282783, 0.399999561397, 0.387259560081, 0.374048548451, 0.360380613021, 0.346270635398, 0.331734268754, 0.316787912671, 0.301448686393, 0.28573440054, 0.269663527349, 0.253255169494, 0.236529027553, 0.219505366195, 0.202204979147, 0.184649153022, 0.166859630089, 0.148858570049, 0.13066851092, 0.112312329091, 0.0938131986527, 0.0751945500785, 0.0564800283494, 0.0376934506144, 0.018858763477, 0.0, -0.018858763477, -0.0376934506144, -0.0564800283494, -0.0751945500785, -0.0938131986527, -0.112312329091, -0.13066851092, -0.148858570049, -0.166859630089, -0.184649153022, -0.202204979147, -0.219505366195, -0.236529027553, -0.253255169494, -0.269663527349, -0.28573440054, -0.301448686393, -0.316787912671, -0.331734268754, -0.346270635398, -0.360380613021, -0.374048548451, -0.387259560081, -0.399999561397, -0.412255282783, -0.424014291681, -0.43526501088, -0.445996735085, -0.456199645625, -0.46586482332, -0.474984259467, -0.483550864942, -0.491558477398, -0.499001866561, -0.505876737611, -0.512179732652, -0.51790843029, -0.523061343309, -0.527637914479, -0.5316385105, -0.535064414122, -0.537917814453, -0.540201795495, -0.541920322948, -0.543078229311, -0.543681197332, -0.543735741853, -0.543249190098, -0.542229660459, -0.540686039841, -0.538627959622, -0.536065770289, -0.533010514831, -0.529473900929, -0.525468272051, -0.521006577488, -0.516102341434, -0.510769631171, -0.505023024448, -0.498877576118, -0.492348784132, -0.485452554966, -0.478205168549, -0.470623242797, -0.462723697829, -0.45452371993, -0.446040725381, -0.437292324199, -0.428296283903, -0.419070493361, -0.40963292682, -0.400001608191, -0.39019457563, -0.380229846614, -0.37012538339, -0.359899059063, -0.349568624272, -0.339151674584, -0.328665618632, -0.318127647097, -0.307554702567, -0.296963450356, -0.286370250309, -0.275791129685, -0.265241757122, -0.254737417779, -0.244292989649, -0.233922921126, -0.223641209834, -0.213461382761, -0.203396477728, -0.193459026213, -0.18366103756, -0.174013984585, -0.164528790593, -0.155215817824, -0.146084857323, -0.137145120254, -0.128405230643, -0.119873219553, -0.111556520688, -0.103461967402, -0.0955957911196, -0.0879636211267, -0.0805704857345, -0.0734208147742, -0.0665184434076, -0.0598666172177, -0.0534679985499, -0.0473246740673, -0.041438163483, -0.0358094294279, -0.0304388884133, -0.0253264228417, -0.0204713940206, -0.0158726561293, -0.0115285710889, -0.0074370242845, -0.00359544108347, -8.04098443393e-07, 0.0033503288631, 0.00646180621834, 0.00933786345535, 0.0119831034641, 0.0144024764108, 0.0166012592143, 0.0185850346832, 0.0203596703733, 0.0219312972225, 0.0233062880235, 0.02449123579, 0.0254929320749, 0.0263183452962, 0.026974599126, 0.0274689509968, 0.0278087707783, 0.0280015196764, 0.0280547294041, 0.0279759816737, 0.0277728880563, 0.0274530702548, 0.0270241408326, 0.0264936844398, 0.0258692395759, 0.0251582809259, 0.0243682023052, 0.0235063002445, 0.0225797582461, 0.0215956317392, 0.0205608337587, 0.0194821213717, 0.0183660828705, 0.0172191257514, 0.0160474654948, 0.0148571151577, 0.0136538757908, 0.0124433276877, 0.011230822471, 0.0100214760183, 0.00882016223, 0.00763150763453, 0.0064598868296, 0.00530941875093, 0.00418396376074, 0.00308712154498, 0.0020222298062, 0.000992363737284, 3.36259008787e-07, -0.000951300997274, -0.0018602559824, -0.00272449381977, -0.00354223371723, -0.0043119452256, -0.00503234387065, -0.00570238618641, -0.00632126417893, -0.00688839925073, -0.00740343561706, -0.00786623324593, -0.00827686035476, -0.00863558549687, -0.00894286927184, -0.00919935569384, -0.00940586325271, -0.00956337570235, -0.00967303261139, -0.00973611971083, -0.00975405907344, -0.00972839915915, -0.00966080476069, -0.00955304688301, -0.00940699258958, -0.00922459484812, -0.00900788240743, -0.00875894973632, -0.00847994705483, -0.00817307048659, -0.00784055236082, -0.00748465169059, -0.00710764485348, -0.00671181649912, -0.00629945070701, -0.00587282241677, -0.00543418915128, -0.00498578305229, -0.00452980324611, -0.00406840855594, -0.0036037105756, -0.00313776711824, -0.00267257605183, -0.00221006953169, -0.00175210863919, -0.00130047843362, -0.000856883423268, -0.00042294345989, -1.90059383236e-07, 0.000409936849938, 0.000806091751623, 0.0011870259394, 0.00155158949247, 0.00189873283191, 0.00222750786389, 0.00253706871632, 0.00282667207705, 0.00309567714275, 0.00334354518872, 0.0035698387709, 0.00377422057251, 0.00395645190841, 0.0041163909013, 0.00425399034471, 0.0043692952683, 0.00446244022186, 0.00453364629481, 0.00458321788861, 0.00461153926002, 0.00461907085337, 0.00460634544047, 0.00457396408696, 0.00452259196407, 0.00445295402495, 0.00436583056466, 0.00426205268297, 0.00414249766903, 0.00400808432674, 0.00385976825946, 0.00369853713255, 0.0035254059317, 0.00334141223473, 0.0031476115141, 0.00294507248686, 0.00273487252813, 0.00251809316382, 0.00229581565752, 0.00206911670572, 0.00183906425517, 0.00160671345499, 0.00137310275567, 0.00113925016637, 0.000906149680707, 0.000674767880774, 0.000446040728168, 0.000220870549765, 1.23225298158e-07, -0.000215374417197, -0.000424836992883, -0.000627522734284, -0.000822735444702, -0.00100982622839, -0.00118819499588, -0.00135729174364, -0.00151661760823, -0.00166572569587, -0.00180422168917, -0.00193176423352, -0.00204806510656, -0.00215288917468, -0.00224605414143, -0.00232743009314, -0.00239693884806, -0.00245455311551, -0.00250029547253, -0.00253423716573, -0.00255649674667, -0.0025672385497, -0.00256667102126, -0.00255504491037};

int cnt, re, im;

class RDSProcessor : public BasebandProcessor {
public:
	void execute(buffer_c8_t buffer) override {
        
		for (size_t i = 0; i<buffer.count; i++) {
			
			if(s >= 9) {
				s = 0;
				if(sample_count >= SAMPLES_PER_BIT) {
					cur_bit = (shared_memory.rdsdata[(bit_pos / 26) & 15]>>(25-(bit_pos % 26))) & 1;
					prev_output = cur_output;
					cur_output = prev_output ^ cur_bit;
					
					inverting = (cur_output == 1);

					float *src = waveform_biphase;
					int idx = in_sample_index;

					for(int j=0; j<FILTER_SIZE; j++) {
						val = (*src++) * 10 * 2;
						if (inverting) val = -val;
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
				
				switch(mphase) {
					case 0:
					case 2: sample = 0; break;
					case 1: break;
					case 3: sample = -sample; break;
				}
				mphase++;
				if (mphase >= 4) mphase = 0;
				//sample *= 16;
				
				sample_count++;
			} else {
				s++;
			}

			frq = sample * 586 * 33; // / 8;
			
			phase = (phase + frq);
			sphase = phase + (256<<16);

			re = sintab[(sphase & 0x03FF0000)>>16];
			im = sintab[(phase & 0x03FF0000)>>16];
			
			buffer.p[i] = {(int8_t)re,(int8_t)im};
		}
	}

private:
    int bit_pos;
    float sample_buffer[SAMPLE_BUFFER_SIZE] = {0};
    float val;
    int prev_output = 0;
    int cur_output = 0;
    int cur_bit = 0;
    int sample_count = SAMPLES_PER_BIT;
    int inverting = 0;
    int phase;

    int in_sample_index = 0;
    float sample;
    int out_sample_index = SAMPLE_BUFFER_SIZE-1;
	int8_t s;
	uint32_t sphase, mphase;
	int32_t sig, frq, frq_im, rdsc;
	int32_t k;
};

static BasebandProcessor* baseband_processor { nullptr };
static BasebandConfiguration baseband_configuration;

static baseband::Direction direction = baseband::Direction::Transmit;
		
static WORKING_AREA(baseband_thread_wa, 8192);
/*static __attribute__((noreturn)) msg_t basebandrx_fn(void *arg) {
	(void)arg;
	chRegSetThreadName("baseband");

	BasebandStatsCollector stats;
	BasebandStatisticsMessage message;
	
	while(true) {

	}
}*/

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
		} else if (direction == baseband::Direction::Receive) {
			// TODO: Place correct sampling rate into buffer returned here:
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

	//if( direction == baseband::Direction::Receive ) {
		rf::rssi::dma::allocate(4, 400);
	//}

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
