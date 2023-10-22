/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __PROC_FSK_RX_H__
#define __PROC_FSK_RX_H__

#include "audio_output.hpp"
#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "rssi_thread.hpp"

#include "dsp_decimate.hpp"
#include "dsp_demodulate.hpp"
#include "dsp_iir_config.hpp"
#include "dsp_fir_taps.hpp"

#include "spectrum_collector.hpp"
#include "stream_input.hpp"

#include "message.hpp"
#include "portapack_shared_memory.hpp"

#include <array>
#include <memory>
#include <tuple>
#include <variant>
#include <cstdint>
#include <functional>

/* Normalizes audio stream to +/-1.0f */
class AudioNormalizer {
   public:
    void execute_in_place(const buffer_f32_t& audio);

   private:
    void calculate_thresholds();

    uint32_t counter_ = 0;
    float min_ = 99.0f;
    float max_ = -99.0f;
    float t_hi_ = 1.0;
    float t_lo_ = 1.0;
};

/* A decimator that just returns the source buffer. */
class NoopDecim {
   public:
    static constexpr int decimation_factor = 1;

    template <typename Buffer>
    Buffer execute(const Buffer& src, const Buffer&) {
        // TODO: should this copy to 'dst'?
        return {src.p, src.count, src.sampling_rate};
    }
};

/* Decimator wrapper that can hold one of a set of decimators and dispatch at runtime. */
template <typename... Args>
class MultiDecimator {
   public:
    /* Dispatches to the underlying type's execute. */
    template <typename Source, typename Destination>
    Destination execute(
        const Source& src,
        const Destination& dst) {
        return std::visit(
            [&src, &dst](auto&& arg) -> Destination {
                return arg.execute(src, dst);
            },
            decimator_);
    }

    size_t decimation_factor() const {
        return std::visit(
            [](auto&& arg) -> size_t {
                return arg.decimation_factor;
            },
            decimator_);
    }

    /* Sets this decimator to a new instance of the specified decimator type.
     * NB: The instance is returned by-ref so 'configure' can easily be called. */
    template <typename Decimator>
    Decimator& set() {
        decimator_ = Decimator{};
        return std::get<Decimator>(decimator_);
    }

   private:
    std::variant<Args...> decimator_{};
};

class FSKRxProcessor : public BasebandProcessor {
   public:
    FSKRxProcessor();
    void execute(const buffer_c8_t& buffer) override;
    void on_message(const Message* const message) override;

   private:
    size_t baseband_fs = 1024000;  // aka: sample_rate
    uint8_t stat_update_interval = 10;
    uint32_t stat_update_threshold = baseband_fs / stat_update_interval;
    static constexpr auto spectrum_rate_hz = 50.0f;

    void configure(const FSKRxConfigureMessage& message);
    void capture_config(const CaptureConfigMessage& message);
    void sample_rate_config(const SampleRateConfigMessage& message);
    void flush();
    void reset();
    void send_packet(uint32_t data);
    void process_bits(const buffer_c8_t& buffer);

    void clear_data_bits();
    void handle_sync(bool inverted);

    /* Returns true if the batch has as sync frame. */
    bool has_sync() const { return has_sync_; }

    /* Set once app is ready to receive messages. */
    bool configured = false;

    /* Buffer for decimated IQ data. */
    std::array<complex16_t, 512> dst{};
    const buffer_c16_t dst_buffer{
        dst.data(),
        dst.size()};

    /* Buffer for demodulated audio. */
    std::array<float, 16> audio{};
    const buffer_f32_t audio_buffer{audio.data(), audio.size()};

    /* The actual type will be configured depending on the sample rate. */
    MultiDecimator<
        dsp::decimate::FIRC8xR16x24FS4Decim4,
        dsp::decimate::FIRC8xR16x24FS4Decim8>
        decim_0{};
    MultiDecimator<
        dsp::decimate::FIRC16xR16x16Decim2,
        dsp::decimate::FIRC16xR16x32Decim8,
        NoopDecim>
        decim_1{};

    /* Filter to 24kHz and demodulate. */
    dsp::decimate::FIRAndDecimateComplex channel_filter{};
    size_t deviation = 3750;
    // fir_taps_real<32> channel_filter_taps = 0;
    size_t channel_decimation = 2;
    int32_t channel_filter_low_f = 0;
    int32_t channel_filter_high_f = 0;
    int32_t channel_filter_transition = 0;

    /* Squelch to ignore noise. */
    FMSquelch squelch{};
    uint64_t squelch_history = 0;

    // /* LPF to reduce noise. POCSAG supports 2400 baud, but that falls
    //  * nicely into the transition band of this 1800Hz filter.
    //  * scipy.signal.butter(2, 1800, "lowpass", fs=24000, analog=False) */
    // IIRBiquadFilter lpf{{{0.04125354f, 0.082507070f, 0.04125354f},
    //                      {1.00000000f, -1.34896775f, 0.51398189f}}};

    /* Attempts to de-noise and normalize signal. */
    AudioNormalizer normalizer{};

    /* Handles writing audio stream to hardware. */
    AudioOutput audio_output{};

    /* Holds the data sent to the app. */
    AFSKDataMessage data_message{false, 0};

    /* Used to keep track of how many samples were processed
     * between status update messages. */
    uint32_t samples_processed = 0;

    /* Number of bits in 'data_' member. */
    static constexpr uint8_t data_bit_count = sizeof(uint32_t) * 8;

    /* Sync frame codeword. */
    static constexpr uint32_t sync_codeword = 0x12345678;

    /* When true, sync frame has been received. */
    bool has_sync_ = false;

    /* When true, bit vales are flipped in the codewords. */
    bool inverted = false;

    uint32_t data = 0;
    uint8_t bit_count = 0;
    uint8_t word_count = 0;

    /* LPF to reduce noise. POCSAG supports 2400 baud, but that falls
     * nicely into the transition band of this 1800Hz filter.
     * scipy.signal.butter(2, 1800, "lowpass", fs=24000, analog=False) */
    IIRBiquadFilter lpf{{{0.04125354f, 0.082507070f, 0.04125354f},
                         {1.00000000f, -1.34896775f, 0.51398189f}}};

    SpectrumCollector channel_spectrum{};
    size_t spectrum_interval_samples = 0;
    size_t spectrum_samples = 0;

    /* NB: Threads should be the last members in the class definition. */
    BasebandThread baseband_thread{baseband_fs, this, baseband::Direction::Receive};
    RSSIThread rssi_thread{};
};

#endif
