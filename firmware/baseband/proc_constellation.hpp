/*
 * Copyleft zxkmm (>) 2026
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

#ifndef __PROC_CONSTELLATION_H__
#define __PROC_CONSTELLATION_H__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"

#include "message.hpp"

#include <cstddef>
#include <cstdint>

/* Carrier-recovery constellation source.
 *
 * Runs entirely on the M4 (it has a hardware FPU; the M0 application core does
 * not, so all the floating point lives here -- see the float warning in the
 * project). The chain is the classic FLL-assisted PLL:
 *
 *   - A numerically controlled oscillator (NCO) de-rotates every working
 *     sample. The NCO phase lives in a uint32 accumulator (one turn = 2^32)
 *     and the sin/cos come from the shared int8 sine table, so there is no
 *     transcendental call in the per-sample loop.
 *   - The modulation is stripped with an M=4 (QPSK) non-linearity (z^4). The
 *     frequency-locked loop and the phase-locked loop both run on z^4:
 *       * FLL: cross-product frequency error of consecutive z^4 samples drives
 *         the NCO frequency word -> removes the frequency offset.
 *       * PLL: residual phase of z^4 drives a phase accumulator -> removes the
 *         static phase offset.
 *   - Each stage can be toggled independently so the operator can see the
 *     effect of each algorithm.
 *
 * Corrected I/Q points are packed as interleaved int8 (offset +128) into the
 * 256-byte ChannelSpectrum payload and shipped over the existing spectrum FIFO,
 * exactly like the Time Sink app, so the M0 side stays integer-only.
 */
class ConstellationProcessor : public BasebandProcessor {
   public:
    void execute(const buffer_c8_t& buffer) override;
    void on_message(const Message* const message) override;

   private:
    static constexpr size_t points_per_frame = 128;  // 256-byte payload / 2

    bool configured = false;
    size_t baseband_fs = 4000000;

    void execute_constellation(const buffer_c8_t& buffer);
    void set_streaming_state(const SpectrumStreamingConfigMessage& message);
    void update_fifo();
    void reset_loops();
    void process_sample(int32_t i_in, int32_t q_in);

    ChannelSpectrum points{};
    ChannelSpectrum fifo_data[1 << ChannelSpectrumConfigMessage::fifo_k]{};
    ChannelSpectrumFIFO fifo{fifo_data, ChannelSpectrumConfigMessage::fifo_k};

    // Configuration.
    size_t decimation = 16;
    bool correct_frequency = false;
    bool correct_phase = false;

    // Working state.
    size_t decim_counter = 0;
    size_t point_index = 0;

    // NCO / loops.
    uint32_t nco_phase = 0;     // running phase accumulator, 2^32 == one turn
    int32_t nco_freq = 0;       // FLL output: phase increment per working sample
    uint32_t phase_acc = 0;     // PLL output: static phase correction
    float prev_r4 = 0.0f;
    float prev_i4 = 0.0f;
    bool have_prev = false;

    // Loop gains (turn units per unit error, applied to the 2^32 accumulators).
    static constexpr float fll_gain = 1.5e6f;
    static constexpr float pll_gain = 8.0e6f;

    bool streaming = false;
    volatile bool request_update = false;

    /* NB: Threads should be the last members in the class definition. */
    BasebandThread baseband_thread{baseband_fs, this, baseband::Direction::Receive};
};

#endif /*__PROC_CONSTELLATION_H__*/
