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

// Meteor-M N2-3 / N2-4 LRPT, receive side up to the CADU:
// OQPSK at 72000 or 80000 symbols/s (RRC, alpha 0.5), convolutional code r1/2 K7 (0x4F,
// 0x6D), NRZ-M, CADUs of 1024 bytes behind the ASM 0x1ACFFC1D. Frames come out still
// randomized and without Reed-Solomon, the M0 does the rest. In 80k mode the code bits are
// interleaved; the M0 undoes that through the SD card (see lrpt_shared.hpp).
// No dependency on the firmware, so the same code runs in a PC test harness.

#include <cstddef>
#include <cstdint>

#include "lrpt_shared.hpp"

namespace lrpt {

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

// 80k: finds the markers, which also tell how the softs come out of the demodulator, puts
// the code bits into input blocks for the M0 and takes them back deinterleaved from the output
// blocks, one out for each one in. Never waits: an output block that is not ready in time is
// decoded as erasures. Which bit of a code pair comes first the markers can't tell; the
// decoder finds that out as in 72k mode.
class Interleaved {
   public:
    using SoftHandler = void (*)(void* context, int8_t s);
    using BlockHandler = void (*)(void* context);

    Interleaved(Shared* shared, SoftHandler soft, void* soft_context, BlockHandler block, void* block_context);
    // carrier: the carrier loop is locked, markers found before that are not believed
    void soft(int8_t s, bool carrier);

    bool locked() const { return state_ == State::Track; }
    uint32_t late() const { return late_; }  // output blocks not ready in time
    uint32_t lost() const { return lost_; }  // input blocks without a free slot

   private:
    static constexpr size_t SEARCH_PERIODS = 32;
    static constexpr uint8_t RESYNC_SHIFT = 4;            // softs a slip may move the markers by
    static constexpr int CHECK_SHIFT = 2;                 // neighbours looked at while tracking
    static constexpr uint32_t RESYNC_SOFTS = 2 * 160000;  // then start over, about 2 s

    enum class State : uint8_t {
        Search,  // from scratch, new generation
        Track,
        Resync,  // lost the markers, keeps the blocks going on the old grid meanwhile
    };

    Shared* shared_;
    SoftHandler soft_;
    void* soft_context_;
    BlockHandler block_;
    void* block_context_;
    State state_{State::Search};
    bool carrier_{false};

    // search
    int8_t pattern_[4][INTER_MARKER_BITS]{};  // the marker as each variant shows it
    int8_t window_[INTER_MARKER_BITS]{};
    uint8_t window_index_{0};
    uint8_t position_{0};  // of the incoming soft, modulo the period
    int16_t acc_[4][INTER_PERIOD]{};
    uint32_t search_softs_{0};
    uint32_t abs_sum_{0};
    uint32_t resync_softs_{0};

    // track
    uint8_t start_{0};    // where the markers start, modulo the period
    uint8_t phase_{0};    // of the incoming soft from the marker start
    uint8_t variant_{0};  // bit 0: odd softs negated, bit 1: pairs swapped
    int8_t sign_{1};
    int32_t marker_corr_{0};
    int32_t marker_threshold_{0};
    uint8_t marker_misses_{0};  // the last 8 markers, a bit set for each bad one
    bool begun_{false};         // the blocks start at a marker
    bool align_{false};         // put the next marker on a period boundary of the blocks
    int8_t held_{0};

    // input block
    uint32_t seq_{0};
    size_t index_{0};  // in the block
    uint8_t branch_{0};
    size_t depth_{0};
    bool stored_{false};

    // output block being used
    bool using_{false};
    uint32_t use_seq_{0};
    bool use_ready_{false};
    bool use_stored_{false};  // its input block, for branch 35
    uint64_t use_valid_{0};
    size_t use_index_{0};
    uint8_t use_branch_{0};
    size_t use_depth_{0};

    uint32_t late_{0};
    uint32_t lost_{0};

    void restart();
    void search(int8_t s);
    void check(int8_t s);
    void found(uint8_t start, uint8_t variant, int8_t sign, int32_t threshold);
    void realign();
    void data(int8_t s);
    void store(uint8_t code);
    void use();
    void complete();
};

class Demodulator {
   public:
    using FrameHandler = Deframer::FrameHandler;

    using BlockHandler = Interleaved::BlockHandler;

    // shared and block are only used in 80k mode
    Demodulator(Mode mode, FrameHandler handler, void* context, Shared* shared = nullptr, BlockHandler block = nullptr);
    Demodulator(const Demodulator&) = delete;
    Demodulator& operator=(const Demodulator&) = delete;
    void execute(const Cf& in);  // one sample at input_rate(mode)
    // a soft symbol as it leaves the demodulator, I then Q (for tests)
    void soft_in(int8_t s, bool carrier);

    bool carrier_locked() const { return carrier_locked_; }
    bool code_locked() const { return code_locked_; }
    bool marker_locked() const { return interleaved_.locked(); }
    uint32_t late_blocks() const { return interleaved_.late(); }
    uint32_t lost_blocks() const { return interleaved_.lost(); }
    bool frame_synced() const { return deframer_.synced(); }
    int32_t frequency_offset_hz() const;
    // permille of the code symbols that disagree with the decoded data
    uint16_t ber_permille() const { return ber_permille_; }
    // the last points, pairs of I and Q
    void constellation(int8_t* out) const;

   private:
    static constexpr size_t RRC_TAPS = 33;
    static const float rrc_taps_[RRC_TAPS];

    const Mode mode_;
    const float fll_near_;

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
    float omega_{0.0f};  // rad/sample at the input rate
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
    Interleaved interleaved_;

    static void on_bit(void* context, uint8_t bit);
    static void on_soft(void* context, int8_t s);
    bool tracking() const;
    void decode(int8_t a, int8_t b);

    void sample(Cf y);
    void interpolant(const Cf& y);
    void symbol(const Cf& y);
    void soft(int8_t s);
    void set_omega(float omega);
    void next_hypothesis();
};

}  // namespace lrpt

#endif /*__LRPT_DEMOD_H__*/
