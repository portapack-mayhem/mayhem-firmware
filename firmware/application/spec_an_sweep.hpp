/*
 * Copyright (C) 2026 zxkmm
 * Copyright (C) 2026 gullradriel, Nilorea Studio Inc.
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

/* M0 side of the swept spectrum protocol (see spec_an_shared.hpp): plans a
 * sweep over [start, start + span] and drives the tuner slice by slice from
 * the M4's SpecAnCaptured replies. The baseband image must run a
 * spec_an::Engine (PSAN, or PSPE in sweep mode).
 *
 * The owner forwards SpecAnCaptured to on_captured(), SpecAnSlice to
 * accept(), calls on_frame() from DisplayFrameSync (also needed unpaced: it
 * re-asks for a request or reply lost on the way), and should have called
 * radio::set_fast_retune(true). Display point x sits at
 * start + x * span / (points - 1). Integer math only. */

#ifndef __SPEC_AN_SWEEP_H__
#define __SPEC_AN_SWEEP_H__

#include "ch.h"
#include "message.hpp"
#include "spec_an_shared.hpp"

#include <cstdint>

namespace spec_an {

/* Synthesizer settling after each retune. HackRF One moves both PLLs on
 * every slice (its second LO tracks the RF, 2650 MHz - f/7) and smears a CW
 * across the slice below ~350 us. PRALINE keeps the MAX2831 on a fixed IF
 * across most bands and is clean even at 0. */
#ifdef PRALINE
constexpr uint32_t kSettleMinUs = 0;
constexpr uint32_t kSettleDefaultUs = 100;
constexpr BinLayout kDefaultLayout = BinLayout::Both;
#else
constexpr uint32_t kSettleMinUs = 350;
constexpr uint32_t kSettleDefaultUs = 400;
constexpr BinLayout kDefaultLayout = BinLayout::Upper;
#endif

constexpr uint64_t kMaxSweepFreq = 7'250'000'000ULL;

/* Where power() puts 0. Chosen so the noise of a 256 point FFT lands where
 * the classic spectrum (Looking Glass, RES 32) put it: classic noise reads
 * 255 + 5 * 10 log10(256 * 66 * 0.397 * s2 / 2^30) = -5 + 50 log10(s2), this
 * one 50 log10(s2) - 315.75 - floor / 20 (Blackman-Harris ENBW 2 bins, full
 * scale tone 127), s2 being the noise power of the int8 samples. */
constexpr int32_t kPowerFloorCdb = -6200;

class Sweep {
   public:
    struct Params {
        uint64_t start{0};
        uint64_t span{20'000'000};
        uint16_t points{kPoints}; /* <= kPoints */
        BinLayout layout{kDefaultLayout};
        uint8_t rbw_idx{0}; /* 0 auto, else N = 128 << idx */
        uint16_t avg{1};    /* FFTs power-averaged per slice */
        Window window{Window::BlackmanHarris};
        Detector detector{Detector::Peak};
        uint32_t settle_us{kSettleDefaultUs};
        bool continuous{true};
        /* Near 0 Hz, raise the start so the first tune can be honoured (see
         * start()). Off keeps the points where asked, the first tune is then
         * clamped at 0 Hz and the lowest points are off by up to fs / 32. */
        bool raise_start{true};
        /* Issue requests only early in each display frame (see
         * spec_an_sweep.cpp). Keeps a busy UI responsive, at the cost of
         * sweep rate: off sweeps as fast as the M4 replies, all the time. */
        bool frame_paced{false};
        /* Start a sweep only once the points it overwrites (pix is double
         * buffered by sweep) have been read, see start_ready(). Off keeps the
         * capture overlapping the FFT of the sweep before, at the risk of a
         * sweep showing points of a later one while the M0 falls behind. */
        bool wait_for_read{true};
    };

    struct Plan {
        uint32_t fs{20000000};
        uint8_t log2n{8};
        uint16_t avg{1};
        int16_t bin_first{8};
        uint16_t bins_per_slice{88};
        uint16_t slices{1};
        int64_t f_bin0{0}; /* frequency of global bin 0 */
        uint32_t filter_bw{15000000};
        uint32_t rbw_hz{0};
        uint8_t settle_bufs{2};
    };

    /* Plans for `params`, sets the sample rate and filter, configures the M4
     * and starts a new sweep. The start may move up a little near 0 Hz:
     * params() and point_freq() give the points actually swept. */
    void start(const Params& params);
    /* Stops issuing requests; resume() starts a fresh sweep. */
    void stop() { sweeping_ = false; }
    void resume();
    /* Takes effect at the end of the sweep in progress. */
    void set_continuous(bool continuous) { params_.continuous = continuous; }

    void on_captured(const SpecAnCapturedMessage& message);
    /* The display points of a valid reply, or nullptr for a stale or
     * repeated one. [px_begin, px_end) may be empty (a slice that completes
     * no point) or reach past `points` (tail fill up to kPoints): bound it.
     * Also issues a sweep start that waited for the points an earlier call
     * handed out to be read. */
    const int16_t* accept(const SpecAnSliceMessage& message);
    /* After accept() of a last slice: every slice of that sweep arrived. The
     * M4 drops a reply when the queue stays full, and the points of a sweep
     * missing one are partly zero or stale. */
    bool sweep_complete() const { return sweep_complete_; }
    /* Once per display frame: paces a frame_paced sweep (see Params) and
     * re-asks for a request or reply lost on the way. */
    void on_frame();

    /* Forget the rates last applied, e.g. after another app changed them. */
    void invalidate_rates() {
        cur_fs_ = 0;
        cur_filter_ = 0;
    }

    bool sweeping() const { return sweeping_; }
    const Params& params() const { return params_; }
    const Plan& plan() const { return plan_; }
    uint64_t point_freq(uint32_t x) const;
    /* The M4's waterfall history (PSAN only), nullptr if it has none. */
    uint8_t* waterfall() const { return shared_.waterfall; }
    /* A display point (centi-dBFS, 0 = full scale tone) on the 0..255 scale
     * of the classic spectrum, 5 units per dB. As on any spectrum analyzer a
     * tone keeps its level whatever the resolution, and the noise floor drops
     * as the bins get narrower. */
    uint8_t power(int16_t cdb) const;

   private:
    Params params_{};
    Plan plan_{};
    Shared shared_{};
    uint32_t cur_fs_{0};
    uint32_t cur_filter_{0};
    uint16_t gen_{0};
    uint32_t seq_{0};
    uint8_t sweep_{0};
    uint16_t slice_{0};
    bool sweeping_{false};
    systime_t issued_at_{0};
    systime_t frame_at_{0};
    bool deferred_{false}; /* slice_ is due, issued on the next frame */
    bool deferred_retune_{false};
    uint32_t last_reply_{UINT32_MAX}; /* (sweep << 16) | slice of the last reply taken */
    uint8_t reply_sweep_{0};          /* sweep of the replies being counted */
    uint16_t reply_next_{0};          /* slice expected next in it */
    uint8_t done_sweep_{0};           /* sweep of the last last slice taken */
    bool sweep_complete_{false};

    void compute_plan();
    int64_t raw_slice_lo(uint16_t slice) const;
    int64_t slice_lo(uint16_t slice) const;
    void issue(uint16_t slice, bool retune);
    void request(uint16_t slice, bool retune);
    void defer(bool retune);
    bool start_ready() const;
};

} /* namespace spec_an */

#endif /*__SPEC_AN_SWEEP_H__*/
