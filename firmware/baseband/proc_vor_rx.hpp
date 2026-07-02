/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2026 PortaPack Mayhem
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

#ifndef __PROC_VOR_RX_H__
#define __PROC_VOR_RX_H__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "rssi_thread.hpp"

#include "dsp_decimate.hpp"
#include "dsp_demodulate.hpp"
#include "audio_compressor.hpp"

#include "audio_output.hpp"
#include "spectrum_collector.hpp"

#include <array>
#include <cstdint>
#include <cmath>

// Standalone VOR receiver baseband.
// Demodulates the AM channel at 48 kHz (so the 9960 Hz subcarrier stays
// representable) and decodes the 30 Hz reference vs. variable phase to derive
// the radial.
class VorRx : public BasebandProcessor {
   public:
    VorRx();

    void execute(const buffer_c8_t& buffer) override;
    void on_message(const Message* const message) override;

   private:
    static constexpr size_t baseband_fs = 3072000;
    // VOR keeps the channel at 48 kHz instead of the AM audio 12 kHz, so the
    // 9960 Hz subcarrier stays below Nyquist. The standard AM decim_2 stage is
    // therefore omitted (its ~4.5 kHz low-pass would remove the subcarrier).
    static constexpr size_t channel_filter_decimation_factor = 1;

    std::array<complex16_t, 512> dst{};
    const buffer_c16_t dst_buffer{
        dst.data(),
        dst.size()};
    std::array<float, 32> audio{};
    const buffer_f32_t audio_buffer{
        audio.data(),
        audio.size()};

    dsp::decimate::FIRC8xR16x24FS4Decim8 decim_0{};
    dsp::decimate::FIRC16xR16x32Decim8 decim_1{};
    dsp::decimate::FIRAndDecimateComplex channel_filter{};
    int32_t channel_filter_low_f = 0;
    int32_t channel_filter_high_f = 0;
    int32_t channel_filter_transition = 0;
    bool configured{false};
    bool vor_enabled{false};

    static constexpr float vor_reference_hz{30.0f};
    static constexpr float vor_subcarrier_hz{9960.0f};
    // 100 ms at 48 kHz (exactly 3 cycles of 30 Hz)
    static constexpr uint32_t vor_window_samples{4800};
    // One-pole low-pass to isolate the +/-480 Hz FM subcarrier baseband
    // after quadrature down-conversion (corner ~600 Hz at 48 kHz).
    static constexpr float vor_sub_lp_alpha{0.9245f};
    // Fixed phase lag the receive chain adds to the recovered 30 Hz reference
    // tone, expressed in degrees at 30 Hz. It comes almost entirely from the
    // one-pole subcarrier low-pass above (~2.75 deg of lag at 30 Hz for
    // alpha = 0.9245) plus a ~0.1 deg half-sample delay in the FM
    // differentiator. Because the reference tone is delayed, the raw radial
    // (reference - variable) reads low by this amount, so we add it back.
    // Value verified by a TX->RX loopback (proc_vor_tx fed through this
    // decoder): every transmitted radial decoded ~3.2 deg low before this
    // correction, and on-bearing afterwards.
    static constexpr float vor_ref_phase_lag_deg{3.2f};

    struct PhaseOscillator {
        float sin_v{0.0f};
        float cos_v{1.0f};
        float step_sin{0.0f};
        float step_cos{1.0f};

        void configure(float frequency_hz, float sample_rate_hz);
        float cosine() const { return cos_v; }
        float sine() const { return sin_v; }
        void advance();
        void renormalize();
    } vor_reference_osc{}, vor_subcarrier_osc{};
    // 30 Hz DFT accumulators for the variable (AM) and reference (FM) tones.
    float vor_ref_i{0.0f};
    float vor_ref_q{0.0f};
    float vor_var_i{0.0f};
    float vor_var_q{0.0f};
    float vor_dc_sum{0.0f};
    // Subcarrier quadrature down-converter / FM-demodulator state.
    float vor_sub_lp_i{0.0f};
    float vor_sub_lp_q{0.0f};
    float vor_sub_prev_i{0.0f};
    float vor_sub_prev_q{0.0f};
    uint32_t vor_sample_count{0};

    dsp::demodulate::AM demod_am{};
    FeedForwardCompressor audio_compressor{};
    AudioOutput audio_output{};

    SpectrumCollector channel_spectrum{};

    /* NB: Threads should be the last members in the class definition. */
#ifdef PRALINE
    BasebandThread baseband_thread{baseband_fs, this, baseband::Direction::Receive,
                                   /*auto_start*/ false};
    RSSIThread rssi_thread{/*auto_start*/ false};
#else
    BasebandThread baseband_thread{baseband_fs, this, baseband::Direction::Receive};
    RSSIThread rssi_thread{};
#endif

    void configure(const AMConfigureMessage& message);
    void configure_vor(const VorRxConfigureMessage& message);
    void process_vor_metrics(const buffer_f32_t& audio);
    void send_vor_status(uint16_t phase_deg, uint16_t radial_deg, uint16_t ref_level, uint16_t var_level, uint8_t quality, bool valid, bool to_from);
    static uint16_t normalize_degrees(float degrees);
};

#endif /*__PROC_VOR_RX_H__*/
