/*
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

#ifndef __LRPT_DEMOD_H__
#define __LRPT_DEMOD_H__

// Meteor-M N2-3 / N2-4 LRPT, 72k mode, receive side up to the CADU:
// OQPSK at 72000 symbols/s (RRC, alpha 0.5), convolutional code r1/2 K7 (0x4F, 0x6D),
// NRZ-M, CADUs of 1024 bytes behind the ASM 0x1ACFFC1D. Frames come out still randomized
// and without Reed-Solomon, the M0 does the rest.
// No dependency on the firmware, so the same code runs in a PC test harness.

#include <cstddef>
#include <cstdint>

#include "lrpt_shared.hpp"

namespace lrpt {

constexpr uint32_t INPUT_RATE = SYMBOL_RATE * 4;  // samples in, 4 a symbol

struct Cf {
    float re;
    float im;
};

// Convolutional decoder, 64 states, soft decisions, sliding traceback.
class Viterbi {
   public:
    using BitHandler = void (*)(void* context, uint8_t bit);

    Viterbi(BitHandler handler, void* context);
    void reset();
    // one code symbol pair (G1, G2), positive soft value is a 1
    void step(int8_t s0, int8_t s1);

    // re-encoded decisions against the received ones, over the last window
    uint32_t window_errors() const { return window_errors_; }
    bool window_done() {
        const bool d = window_ready_;
        window_ready_ = false;
        return d;
    }

    static constexpr size_t WINDOW_BITS = 1024;

   private:
    static constexpr size_t RING = 128;
    static constexpr size_t CHUNK = 64;

    BitHandler handler_;
    void* context_;
    int32_t metrics_[2][64]{};
    int32_t* metric_{metrics_[0]};
    uint32_t decision_[RING][2]{};
    int8_t received_[RING][2]{};
    size_t step_{0};
    uint8_t encoder_{0};
    uint32_t errors_{0};
    uint32_t bits_{0};
    uint32_t window_errors_{0};
    bool window_ready_{false};

    void traceback();
};

// Code bits after the Viterbi decoder in, randomized frames out.
class Deframer {
   public:
    using FrameHandler = void (*)(void* context, const uint8_t* frame);

    Deframer(FrameHandler handler, void* context)
        : handler_{handler}, context_{context} {}
    void reset();
    void bit(uint8_t b);
    bool synced() const { return state_ != State::Search; }

   private:
    static constexpr uint32_t ASM = 0x1ACFFC1D;
    enum class State : uint8_t { Search,
                                 Frame,
                                 Marker };

    FrameHandler handler_;
    void* context_;
    State state_{State::Search};
    uint8_t last_{0};
    uint32_t shift_{0};
    size_t count_{0};
    uint8_t misses_{0};
    uint8_t frame_[FRAME_BYTES]{};
};

class Demodulator {
   public:
    using FrameHandler = Deframer::FrameHandler;

    Demodulator(FrameHandler handler, void* context);
    Demodulator(const Demodulator&) = delete;
    Demodulator& operator=(const Demodulator&) = delete;
    void execute(const Cf& in);  // one sample at INPUT_RATE

    bool carrier_locked() const { return carrier_locked_; }
    bool code_locked() const { return code_locked_; }
    bool frame_synced() const { return deframer_.synced(); }
    int32_t frequency_offset_hz() const;
    // permille of the code symbols that disagree with the decoded data
    uint16_t ber_permille() const { return ber_permille_; }
    // the last points, pairs of I and Q
    void constellation(int8_t* out) const;

   private:
    static constexpr size_t RRC_TAPS = 33;
    static const float rrc_taps_[RRC_TAPS];

    // RRC matched filter, decimation by 2: 4 to 2 samples a symbol
    Cf rrc_history_[RRC_TAPS * 2]{};
    size_t rrc_index_{0};
    bool rrc_phase_{false};

    // AGC
    float power_{1.0f};
    float gain_{1.0f};
    uint8_t gain_count_{0};

    // carrier NCO, e^-j(phase) as a phasor, advanced by step_ each sample
    Cf nco_{1.0f, 0.0f};
    Cf nco_step_{1.0f, 0.0f};
    float omega_{0.0f};  // rad/sample at INPUT_RATE
    float delayed_i_{0.0f};

    // symbol timing (Gardner) with a cubic interpolator, at 2 samples a symbol
    Cf history_[4]{};
    float next_{1.0f};
    float period_adjust_{0.0f};
    bool strobe_next_{true};
    Cf mid_{};
    Cf last_symbol_{};

    // FLL for acquisition
    Cf fll_last_{};
    Cf fll_acc_{};
    uint16_t fll_count_{0};

    // lock detector
    float lock_num_{0.0f};
    float lock_den_{1.0f};
    bool carrier_locked_{false};

    int8_t points_[CONSTELLATION_POINTS][2]{};
    uint8_t point_index_{0};

    // the 8 ways the code pairs can come out of the demodulator
    uint8_t hypothesis_{0};
    bool shift_{false};
    bool skip_{false};
    bool have_first_{false};
    int8_t first_{0};
    bool code_locked_{false};
    uint8_t bad_windows_{0};
    uint16_t ber_permille_{500};

    Deframer deframer_;
    Viterbi viterbi_;

    static void on_bit(void* context, uint8_t bit);

    void sample(Cf y);
    void interpolant(const Cf& y);
    void symbol(const Cf& y);
    void soft(int8_t s);
    void set_omega(float omega);
    void next_hypothesis();
};

}  // namespace lrpt

#endif /*__LRPT_DEMOD_H__*/
