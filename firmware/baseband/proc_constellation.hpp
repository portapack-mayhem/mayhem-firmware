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
 *   - The modulation is stripped by multiplying the sample phase by the
 *     constellation's rotational symmetry M (1=CW, 2=BPSK, 4=QPSK, 8=8PSK). The
 *     phase is read with the shared fixed-point atan2 (fast, no float library
 *     call), so M*phase wraps the symbol rotation away and leaves the signed
 *     carrier phase error e_p.
 *   - e_p drives a single 2nd-order PLL with two paths into one NCO:
 *       * Integral path (ki) accumulates into the NCO frequency -> removes the
 *         frequency offset. Toggled by the frequency-correction control.
 *       * Proportional path (kp), recomputed each sample (never accumulated, so
 *         the loop stays stable) -> removes the static phase offset. Toggled by
 *         the phase-correction control.
 *   - A magnitude gate skips low-amplitude inter-symbol transition samples
 *     (there is no symbol-timing recovery) so they neither smear the display
 *     nor bias the loops.
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
    size_t decimation = 8;
    uint32_t order = 4;
    bool correct_frequency = false;
    bool correct_phase = false;

    // 2nd-order PLL loop-filter gains (accumulator units per unit int16 error).
    // kp = proportional path (phase), ki = integral path (frequency). Selected
    // by preset; ki << kp so the loop is well damped.
    float kp_gain = 800.0f;
    float ki_gain = 4.0f;

    // Working state.
    size_t decim_counter = 0;
    size_t point_index = 0;
    int32_t env2 = 0;           // running mean of |z|^2 for the magnitude gate

    // NCO / loop. Single phase accumulator (2^32 == one turn) with one
    // frequency integrator; the proportional term is recomputed per sample (not
    // accumulated), which is what keeps the 2nd-order loop stable.
    uint32_t nco_phase = 0;
    int32_t nco_freq = 0;       // integral state: phase increment per working sample

    bool streaming = false;
    volatile bool request_update = false;

    /* NB: Threads should be the last members in the class definition. */
    BasebandThread baseband_thread{baseband_fs, this, baseband::Direction::Receive};
};

#endif /*__PROC_CONSTELLATION_H__*/
