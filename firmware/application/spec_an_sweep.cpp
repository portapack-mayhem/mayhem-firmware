/*
 * Copyright (C) 2026 zxkmm
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

#include "spec_an_sweep.hpp"

#include "baseband_api.hpp"
#include "portapack.hpp"
#include "radio.hpp"
#include "receiver_model.hpp"

#include <algorithm>

using namespace portapack;

namespace spec_an {

namespace {

/* Ascending. Narrow spans take the lowest rate that covers them in one tune:
 * finer bins for the same FFT size and a narrower anti-alias filter. */
constexpr uint32_t kSampleRates[] = {2'500'000, 5'000'000, 10'000'000, 20'000'000};

/* A request or reply lost on the way would stall the sweep. */
constexpr systime_t kReissueTicks = 250;

inline void compiler_barrier() {
    __asm__ volatile("" ::: "memory");
}

}  // namespace

void Sweep::compute_plan() {
    const auto layout = params_.layout;
    const uint64_t span = params_.span;
    const uint32_t points = std::clamp<uint32_t>(params_.points, 2, kPoints);
    /* Display points sit at start + x * span / (P - 1); each one owns the
     * frequencies half a point either side of it. */
    const uint64_t coverage = span + span / (points - 1);

    /* One tune covers layout_count(N) / N of fs, whatever N is. */
    uint32_t fs = kSampleRates[3];
    for (const uint32_t rate : kSampleRates) {
        const uint64_t capacity = static_cast<uint64_t>(rate) * layout_count(layout, 256) / 256;
        if (capacity >= coverage + coverage / 32) {
            fs = rate;
            break;
        }
    }

    uint8_t log2n = kMaxFftLog2;
    if (params_.rbw_idx == 0) {
        /* Auto RBW: the coarsest FFT whose bins are no wider than a point. */
        for (uint8_t l = kMinFftLog2; l <= kMaxFftLog2; l++) {
            if (static_cast<uint64_t>(fs) * (points - 1) <= (span << l)) {
                log2n = l;
                break;
            }
        }
    } else {
        log2n = std::clamp<uint8_t>(7 + params_.rbw_idx, kMinFftLog2, kMaxFftLog2);
    }

    const uint32_t n = 1u << log2n;
    const uint32_t k_max = layout_count(layout, n);
    /* +3: interpolation needs a bin either side of the outermost points. */
    const uint64_t bins_needed = (coverage * n + fs - 1) / fs + 3;
    const uint32_t slices = static_cast<uint32_t>((bins_needed + k_max - 1) / k_max);
    const uint32_t k = static_cast<uint32_t>((bins_needed + slices - 1) / slices);

    plan_.fs = fs;
    plan_.log2n = log2n;
    plan_.avg = std::max<uint16_t>(params_.avg, 1);
    plan_.bin_first = static_cast<int16_t>(layout_first(layout, n));
    plan_.bins_per_slice = static_cast<uint16_t>(k);
    plan_.slices = static_cast<uint16_t>(std::min<uint32_t>(slices, 65535));
    plan_.f_bin0 = static_cast<int64_t>(params_.start) - static_cast<int64_t>(span / (2 * (points - 1)));
    plan_.filter_bw = (fs / 4) * 3;
    plan_.rbw_hz = static_cast<uint32_t>(static_cast<uint64_t>(fs) * window_enbw_centibins(params_.window) / (100ULL * n));
    /* One DMA buffer straddles the retune, then whole buffers for settling. */
    plan_.settle_bufs = static_cast<uint8_t>(1 + (static_cast<uint64_t>(params_.settle_us) * fs + kDmaBufferSamples * 1'000'000ULL - 1) / (kDmaBufferSamples * 1'000'000ULL));
}

void Sweep::start(const Params& params) {
    params_ = params;
    params_.points = std::clamp<uint16_t>(params_.points, 2, kPoints);
    params_.span = std::max<uint64_t>(params_.span, 1);
    compute_plan();

    /* The Upper layout tunes below the range. Near 0 Hz the tuner cannot
     * follow, so start the points where the first tune can be honoured. A
     * narrower span never needs a higher rate, so one replan settles it. */
    const int64_t lo = raw_slice_lo(0);
    if (params_.raise_start && lo < 0 && static_cast<uint64_t>(-lo) < params_.span) {
        params_.start += static_cast<uint64_t>(-lo);
        params_.span -= static_cast<uint64_t>(-lo);
        compute_plan();
    }

    if (plan_.fs != cur_fs_) {
        receiver_model.set_sampling_rate(plan_.fs);
        cur_fs_ = plan_.fs;
    }
    if (plan_.filter_bw != cur_filter_) {
        receiver_model.set_baseband_bandwidth(plan_.filter_bw);
        cur_filter_ = plan_.filter_bw;
    }

    gen_++;
    const SpecAnConfigMessage message{
        &shared_,
        plan_.fs,
        gen_,
        plan_.log2n,
        plan_.avg,
        plan_.bin_first,
        plan_.bins_per_slice,
        plan_.slices,
        static_cast<uint64_t>(plan_.fs) * (params_.points - 1),
        params_.span << plan_.log2n,
        params_.window,
        params_.detector};
    baseband::set_spec_an_config(message);

    resume();
}

void Sweep::resume() {
    sweep_++;
    slice_ = 0;
    sweeping_ = true;
    issue(0, true);
}

uint64_t Sweep::point_freq(uint32_t x) const {
    return params_.start + static_cast<uint64_t>(x) * params_.span / (params_.points - 1);
}

uint8_t Sweep::power(int16_t cdb) const {
    const int32_t v = (static_cast<int32_t>(cdb) + (plan_.log2n - 8) * 301 - kPowerFloorCdb) / 20;
    return static_cast<uint8_t>(std::clamp<int32_t>(v, 0, 255));
}

int64_t Sweep::raw_slice_lo(uint16_t slice) const {
    const int64_t bins = static_cast<int64_t>(slice) * plan_.bins_per_slice - plan_.bin_first;
    return plan_.f_bin0 + ((bins * plan_.fs) >> plan_.log2n);
}

int64_t Sweep::slice_lo(uint16_t slice) const {
    return std::clamp<int64_t>(raw_slice_lo(slice), 0, kMaxSweepFreq);
}

void Sweep::issue(uint16_t slice, bool retune) {
    if (retune) radio::set_tuning_frequency(slice_lo(slice));
    shared_.req_slice = slice;
    shared_.req_sweep = sweep_;
    shared_.req_settle = retune ? plan_.settle_bufs : 0;
    compiler_barrier();
    shared_.req_seq = ++seq_;
    issued_at_ = chTimeNow();
}

void Sweep::on_captured(const SpecAnCapturedMessage& message) {
    if (message.gen != gen_ || message.seq != seq_ || !sweeping_) return;

    /* The M4 has its samples: retune now, it FFTs while we do. */
    if (slice_ + 1u < plan_.slices) {
        slice_++;
        issue(slice_, true);
    } else if (params_.continuous) {
        sweep_++;
        slice_ = 0;
        issue(0, plan_.slices > 1);
    } else {
        sweeping_ = false;
    }
}

const int16_t* Sweep::accept(const SpecAnSliceMessage& message) {
    if (message.gen != gen_) return nullptr;

    /* A timed out request that the M4 had in fact captured gets answered
     * twice; the M4 replays the same points, so drop the repeat. */
    const uint32_t id = (static_cast<uint32_t>(message.sweep) << 16) | message.slice;
    if (id == last_reply_) return nullptr;
    last_reply_ = id;

    return shared_.pix[message.sweep & 1];
}

void Sweep::watchdog() {
    if (sweeping_ && chTimeNow() - issued_at_ > kReissueTicks) issue(slice_, true);
}

} /* namespace spec_an */
