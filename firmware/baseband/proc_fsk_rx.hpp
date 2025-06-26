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
    void execute(const buffer_c8_t& buffer) override;
    void on_message(const Message* const message) override;

   private:
    static constexpr int ROLLING_WINDOW{32};
    static constexpr uint16_t MAX_BUFFER_SIZE{512};

    enum Parse_State {
        Parse_State_Wait_For_Peak = 0,
        Parse_State_Preamble,
        Parse_State_Sync,
        Parse_State_PDU_Payload,
        Parse_State_Parsing_Data
    };

    size_t baseband_fs = 960000;
    uint8_t stat_update_interval = 10;
    uint32_t stat_update_threshold = baseband_fs / stat_update_interval;

    float detect_peak_power(const buffer_c8_t& buffer, int N);
    void agc_correct_iq(const buffer_c8_t& buffer, int N, float measured_power);
    float get_phase_diff(const complex16_t& sample0, const complex16_t& sample1);
    void demodulateFSKBits(const buffer_c16_t& decimator_out, int num_demod_byte);
    void resetPreambleTracking();
    void resetBitPacketIndex();
    void resetToDefaultState();

    void handlePreambleState(const buffer_c16_t& decimator_out);
    void handleSyncWordState(const buffer_c16_t& decimator_out);
    void handlePDUPayloadState(const buffer_c16_t& decimator_out);

    void configure(const FSKRxConfigureMessage& message);
    void sample_rate_config(const SampleRateConfigMessage& message);

    std::array<complex16_t, MAX_BUFFER_SIZE> dst{};
    const buffer_c16_t dst_buffer{
        dst.data(),
        dst.size()};

    uint8_t rb_buf[MAX_BUFFER_SIZE];

    dsp::demodulate::FM demod{};
    int rb_head{-1};
    int32_t g_threshold{0};
    uint8_t channel_number{0};

    uint16_t process = 0;

    bool configured{false};
    FskPacketData fskPacketData{};

    Parse_State parseState{Parse_State_Wait_For_Peak};

    int sample_idx{0};
    int samples_eaten{0};

    int32_t max_dB{0};
    int8_t real{0};
    int8_t imag{0};

    uint16_t peak_timeout{0};
    float noise_floor{12.0};  // Using LNA 40 and VGA 20. 10.0 was 40/0 ratio.
    float target_power_db{5.0};
    float agc_power{0.0f};

    float frequency_offset_estimate{0.0f};
    float frequency_offset{0.0f};
    float phase_buffer[ROLLING_WINDOW] = {0.0f};
    int phase_buffer_index = 0;

    uint16_t packet_index{0};
    uint8_t bit_index{0};
    size_t demod_input_fs{0};

    uint8_t SAMPLE_PER_SYMBOL{1};
    uint32_t DEFAULT_PREAMBLE{0xAAAAAAAA};
    uint32_t DEFAULT_SYNC_WORD{0xFFFFFFFF};
    uint8_t NUM_SYNC_WORD_BYTE{4};
    uint8_t NUM_PREAMBLE_BYTE{4};
    uint16_t NUM_DATA_BYTE = MAX_BUFFER_SIZE - NUM_SYNC_WORD_BYTE - NUM_PREAMBLE_BYTE;

    SpectrumCollector channel_spectrum{};
    size_t spectrum_interval_samples = 0;
    size_t spectrum_samples = 0;
    static constexpr auto spectrum_rate_hz = 50.0f;

    MultiDecimator<
        dsp::decimate::FIRC8xR16x24FS4Decim4,
        dsp::decimate::FIRC8xR16x24FS4Decim8>
        decim_0{};
    MultiDecimator<
        dsp::decimate::FIRC16xR16x16Decim2,
        dsp::decimate::FIRC16xR16x32Decim8,
        NoopDecim>
        decim_1{};

    dsp::decimate::FIRAndDecimateComplex channel_filter{};
    // fir_taps_real<32> channel_filter_taps = 0;
    size_t channel_decimation = 2;
    int32_t channel_filter_low_f = 0;
    int32_t channel_filter_high_f = 0;
    int32_t channel_filter_transition = 0;

    /* Squelch to ignore noise. */
    FMSquelch squelch{};
    uint64_t squelch_history = 0;

    /* NB: Threads should be the last members in the class definition. */
    BasebandThread baseband_thread{baseband_fs, this, baseband::Direction::Receive};
    RSSIThread rssi_thread{};
};

#endif
