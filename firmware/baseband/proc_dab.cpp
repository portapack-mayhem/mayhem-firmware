/*
 * Copyright (C) 2026 Mayhem firmware contributors
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

#include "proc_dab.hpp"

#include "dab/dab_phaseref.hpp"

#include "dab/dab_freq_interleave.hpp"
#include "dab/dab_variant.hpp"

#include "audio_dma.hpp"
#include "event_m4.hpp"
#include "portapack_shared_memory.hpp"

#include <cstring>

/* Cortex-M4 DWT by address rather than through CMSIS, so no extra headers. */
static constexpr uint32_t kDemcr = 0xE000EDFC;
static constexpr uint32_t kDwtControl = 0xE0001000;
static constexpr uint32_t kDwtCycleCount = 0xE0001004;

namespace {

inline uint32_t sample_power(int32_t i, int32_t q) {
    return static_cast<uint32_t>(i * i + q * q) >> 4;
}
constexpr uint32_t kQuietRatio = 4;

/* The envelope uses one sample in kPowerDecim. Block and reference are built
 * the same way, so the ratio the detector tests is unchanged. */
constexpr size_t kPowerDecim = 4;

/* Samples the symbol phase starts after the null detector fires; 96 measured
 * best, 640 and up lose the guard. */
constexpr uint32_t kStartOffset = 96;

/* Samples the early-late gate moves the grid per frame: large enough to close
 * the null detector's error before the next re-acquisition, small enough not
 * to jitter around the correlation peak once there. */
constexpr int32_t kTimingStep = 16;

/* Baseband backlog, in transfers, above which an audio unit that is not yet
 * needed waits (advance_audio_decode()). */
constexpr uint16_t kAudioBacklog = 4;
/* ...and above which none starts even with the ring empty: a unit (~400k
 * cycles) on top of a superframe's Reed-Solomon laps the ring (DAB-IMPL 6.118). */
constexpr uint16_t kAudioBacklogDry = 16;

/* One SSAT on the M4. */
inline int8_t sat_int8(int32_t v) {
#if defined(__ARM_ARCH_7EM__) || defined(__ARM_FEATURE_SAT)
    int32_t r;
    __asm("ssat %0, #8, %1" : "=r"(r) : "r"(v));
    return static_cast<int8_t>(r);
#else
    return static_cast<int8_t>(v > 127 ? 127 : (v < -128 ? -128 : v));
#endif
}

/* |acc| / n in thousandths, saturated at `limit`: an r4 accumulator as
 * published. */
uint32_t r4_milli(float re, float im, uint32_t n, uint32_t limit) {
    const float r4 = __builtin_sqrtf(re * re + im * im) / static_cast<float>(n);
    const uint32_t scaled = static_cast<uint32_t>(r4 * 1000.0f);
    return scaled > limit ? limit : scaled;
}

} /* namespace */

void DabProcessor::enable_cycle_counter() {
    volatile uint32_t* demcr = reinterpret_cast<volatile uint32_t*>(kDemcr);
    volatile uint32_t* control = reinterpret_cast<volatile uint32_t*>(kDwtControl);
    volatile uint32_t* counter = reinterpret_cast<volatile uint32_t*>(kDwtCycleCount);

    *demcr |= (1u << 24);  // TRCENA
    *counter = 0;
    *control |= 1u;  // CYCCNTENA
}

uint32_t DabProcessor::cycle_count() {
    return *reinterpret_cast<volatile uint32_t*>(kDwtCycleCount);
}

/* Bank 2's tail, above the code cap M4_FLASH_SIZE sets. The linker region is
 * NOLOAD and startup zeroes only bank 1's .bss, so these are filled below. */
__attribute__((section(".ram2"))) std::array<int8_t, DabProcessor::kSoftBitsPerSymbol> DabProcessor::symbol_soft_;
__attribute__((section(".ram2"))) decltype(DabProcessor::msc_in_) DabProcessor::msc_in_;
__attribute__((section(".ram2"))) std::array<uint8_t, dab::kViterbiMaxPayloadBits / 8> DabProcessor::decoded_;
__attribute__((section(".ram2"))) decltype(DabProcessor::fic_soft_) DabProcessor::fic_soft_;

namespace {
/* The audio DMA interrupt's view of the processor. */
dab::AudioRing* volatile audio_ring_isr = nullptr;

void audio_ring_refill(audio::sample_t* p, size_t count) {
    dab::AudioRing* const ring = audio_ring_isr;
    if (ring == nullptr)
        return;
    static_assert(sizeof(audio::sample_t) == sizeof(uint32_t), "packed stereo frame");
    ring->render(reinterpret_cast<uint32_t*>(p), count);
}
}  // namespace

DabProcessor::DabProcessor() {
    transfers_dropped_seen_ = shared_memory.m4_transfers_dropped;
    symbol_soft_.fill(0);
    msc_in_.fill(0);
    decoded_.fill(0);
    fic_soft_.fill(0);
    enable_cycle_counter();
    audio_ring_.attach(aac_pcm_.data());
    audio_ring_isr = &audio_ring_;
    audio::dma::set_tx_refill(audio_ring_refill);
    baseband_thread.start();
    rssi_thread.start();
}

DabProcessor::~DabProcessor() {
    audio::dma::set_tx_refill(nullptr);
    audio_ring_isr = nullptr;
}

void DabProcessor::execute(const buffer_c8_t& buffer) {
    const uint32_t started = cycle_count();
    exec_tags_ = 0;
    /* The clamp skipped these transfers just before this buffer. */
    {
        const uint32_t dropped = shared_memory.m4_transfers_dropped;
        if (dropped != transfers_dropped_seen_) {
            const uint32_t lost = dropped - transfers_dropped_seen_;
            transfers_dropped_seen_ = dropped;
            skip_samples(static_cast<size_t>(lost) * kSamplesPerDmaBuffer);
        }
    }
    if (shared_memory.m4_buffer_missed > backlog_max_)
        backlog_max_ = shared_memory.m4_buffer_missed;

    /* Copy the samples out of the ring first: the reader gets the oldest slot
     * and one transfer period to consume it, which the trellis would eat into. */
    for (size_t off = 0; off < buffer.count; off += kStageSamples) {
        const size_t take = (buffer.count - off) < kStageSamples ? (buffer.count - off) : kStageSamples;
        /* Word copy unrolled by eight: memcpy() with a runtime length compiled
         * to a byte loop. Both ends are 4-byte aligned (DMA slot, alignas, and
         * off steps in whole kStageSamples). */
        const uint32_t* __restrict s32 = reinterpret_cast<const uint32_t*>(&buffer.p[off]);
        uint32_t* __restrict d32 = reinterpret_cast<uint32_t*>(stage_.data());
        {
            const size_t words = take / 2;
            size_t w = 0;
            for (; w + 8 <= words; w += 8) {
                d32[w + 0] = s32[w + 0];
                d32[w + 1] = s32[w + 1];
                d32[w + 2] = s32[w + 2];
                d32[w + 3] = s32[w + 3];
                d32[w + 4] = s32[w + 4];
                d32[w + 5] = s32[w + 5];
                d32[w + 6] = s32[w + 6];
                d32[w + 7] = s32[w + 7];
            }
            for (; w < words; w++)
                d32[w] = s32[w];
        }
        if (take & 1)
            stage_[take - 1] = buffer.p[off + take - 1];
        const buffer_c8_t local{stage_.data(), take, buffer.sampling_rate};
        execute_fast(local);
    }

    /* A slice of each trellis per buffer, except across the subchannel's
     * symbols (msc_trellis_quiet()): it delays the next copy out of a 4 ms
     * ring, and symbols arriving meanwhile come out destroyed. */
    if (!msc_trellis_quiet()) {
        advance_msc_decode();
        advance_fic_decode();
    }

    if (fic_ready_ && !fic_viterbi_.busy())
        decode_fic_codeword();

    /* At most one audio unit per DMA buffer; one is due every 40 ms, about ten
     * buffers. */
    advance_audio_decode();

    const uint32_t spent = cycle_count() - started;
    work_cycles_ += spent;
    if (spent > exec_worst_) {
        exec_worst_ = spent;
        exec_worst_tags_ = exec_tags_;
    }
}

void DabProcessor::on_message(const Message* const message) {
    if (message->id != Message::ID::DabConfigure)
        return;

    const auto* config = static_cast<const DabConfigureMessage*>(message);
    if (config->carrier_hz != 0)
        drift_per_hz_ = static_cast<float>(kSamplesPerFrame) / static_cast<float>(config->carrier_hz);

    /* Whether this message is a release; see the end of this function. */
    const bool was_enabled = msc_enabled_;

    msc_enabled_ = false;
    msc_schedule_ = dab::MscSchedule{};
    msc_pending_ = false;
    lf_held_ = false;
    msc_steps_ = kMscStepsPerBuffer;
    fic_finish_symbol_ = kFicFinishSymbol;
    audio_tier_ = 0;
    aac_lc_.reset();
    aac_lc_configured_ = false;
    aac_lc_fail_ = 0;

    if (config->size_cu > 0) {
        dab::ProtectionProfile profile{};
        profile.kind = dab::ProtectionKind::Eep;
        profile.level = config->protection_level;
        profile.option_b = config->protection_option_b;

        const auto schedule = dab::msc_schedule(config->size_cu, profile);
        const size_t coded_bits = static_cast<size_t>(config->size_cu) * 64;

        /* Refuse a subchannel the chain cannot carry rather than overrun a
         * buffer or the decoder's payload bound. */
        if (schedule.valid &&
            dab::subchannel_placement_ok(config->start_cu, config->size_cu) &&
            coded_bits <= dab::kMaxCodedBitsPerCif &&
            schedule.payload_bits <= dab::kViterbiMaxPayloadBits &&
            schedule.mother_bits <= kMaxMotherBits &&
            deinterleaver_.configure(coded_bits, deinterleaver_storage_.data(), deinterleaver_storage_.size())) {
            msc_schedule_ = schedule;
            msc_steps_ = schedule.payload_bits > kMscWidePayloadBits ? kMscStepsWide : kMscStepsPerBuffer;
            msc_start_cu_ = config->start_cu;
            msc_size_cu_ = config->size_cu;
            msc_wanted_ = dab::subchannel_symbols(msc_start_cu_, msc_size_cu_);
            msc_window_ = dab::subchannel_window(msc_wanted_, true);
            msc_is_dab_plus_ = config->is_dab_plus;
            msc_enabled_ = true;
            {
                /* The subchannel's first transform in CIF 0 is of the symbol
                 * before its first wanted one. */
                const size_t first_work = dab::kFicSymbols + (static_cast<size_t>(msc_start_cu_) * 64) / kSoftBitsPerSymbol;
                if (first_work <= kFicFinishSymbol)
                    fic_finish_symbol_ = static_cast<uint8_t>(
                        first_work - 1 > dab::kFicSymbols + 1 ? first_work - 1 : dab::kFicSymbols + 1);
            }
            /* A correlation staged this frame would meet the subchannel's
             * transforms on its way through; drop it rather than read it. */
            if (prs_window_clash())
                prs_staged_ = false;

            if (msc_is_dab_plus_) {
                /* A superframe must be a whole number of RS code words, else
                 * refuse the subchannel rather than leave it silently dead. */
                if (superframe_.configure(schedule.payload_bits / 8)) {
                    /* The decoder is configured once the first superframe
                     * header parses and its stream parameters are known. */
                    audio_tier_ = 2;
                } else {
                    msc_enabled_ = false;
                    msc_schedule_ = dab::MscSchedule{};
                }
            } else {
                /* Classic DAB is Layer II, which this receiver does not decode;
                 * tier 1 would feed Layer II bytes to the AAC decoder. */
                audio_tier_ = 0;
            }
        }
    }

    /* A FIC stage already waiting whose finish symbol the new subchannel has
     * moved behind the current one would never finish this frame, and would
     * hold fft_ against the subchannel's own transforms until symbol 0. */
    if (fic_staged() && symbol_index_ > fic_finish_symbol_) {
        staged_is_fic_ = false;
        staged_fft_ = kStageEmpty;
    }

    /* A selection does not restart acquisition: the grid is unrelated, and
     * re-acquiring costs a couple of hundred frames. A release does: ui_dab
     * sends one only once the lock is gone, and the dead grid would free-run
     * until the periodic re-hunt. */
    if (was_enabled && config->size_cu == 0)
        restart_acquisition();
}

void DabProcessor::restart_acquisition() {
    state_ = State::Searching;
    symbol_damaged_ = false;
    block_power_ = 0;
    block_fill_ = 0;
    /* Keep block_history_, window_power_ and the reference: they describe the
     * signal, not the frame, and rebuilding them costs a null. */
    symbol_fill_ = 0;
    symbol_index_ = 0;
    fic_fill_ = 0;
    fic_ready_ = false;
    have_reference_ = false;
    fib_errors_ = 0;
    fibs_seen_ = 0;
    idle_samples_ = 0;
    preroll_write_ = 0;
    corrector_.reset();
    /* Re-prime the frequency estimate too: corrector_.reset() clears only the
     * rotator, so a re-hunt would keep applying the offset the previous grid
     * settled on, and guard_quality_ is a magnitude, blind to a bad prime. */
    corrector_primed_ = false;
    corrector_offset_ = 0.0f;
    deinterleaver_.reset();
    superframe_.reset();
    /* Pending units pointed into the reset assembler. Not counted as stale:
     * nothing a working receiver would have played was dropped. */
    pending_unit_count_ = 0;
    pending_unit_next_ = 0;
    /* Also reset for a re-hunt: both trellises (advance_msc_decode() gates on
     * busy(), not msc_enabled_), timing_adjust_ (Sync zeroes only the total),
     * the frame-long frequency accumulator, and use_a_, which pairs carriers_a_
     * and carriers_b_ by toggle. */
    viterbi_.reset();
    fic_viterbi_.reset();
    msc_pending_ = false;
    lf_held_ = false;
    timing_adjust_ = 0;
    timing_total_ = 0;
    freq_acc_re_ = 0;
    freq_acc_im_ = 0;
    freq_acc_n_ = 0;
    use_a_ = true;
    audio_decode_hold_ = false;
    prs_staged_ = false;
    prs_frame_ = 0;
    prs_timing_valid_ = false;
    staged_fft_ = kStageEmpty;
    staged_buf_ = kStageEmpty;
    staged_is_fic_ = false;
    /* Not frames_since_fib_: keep-lock wants it below kFramesBeforeRehunt, so
     * zeroing it would hold a fresh grid on r4 alone, the misnumbered grid
     * kFibHuntFrames exists to catch. */
}

bool DabProcessor::consume_power_block(uint32_t block) {
    window_power_ -= block_history_[history_write_];
    block_history_[history_write_] = block;
    window_power_ += block;

    history_write_ = history_write_ + 1;
    if (history_write_ >= kNullWindowBlocks) {
        history_write_ = 0;
        window_filled_ = true;
    }

    /* The reference must average over frames: a shift of 12 spans ~1.3 frames
     * of power blocks, so the null (41 blocks a frame) barely moves it, where a
     * short one would track the null down and never fire. */
    /* Same IIR, accumulated at full precision: `window_power_ >> 12` is zero
     * for any window under 4,096, which pins the reference at 1. */
    reference_acc_ = reference_acc_ - (reference_acc_ >> 12) + window_power_;
    reference_power_ = static_cast<uint32_t>(reference_acc_ >> 12);
    if (reference_power_ == 0)
        reference_power_ = 1;

    if (!window_filled_)
        return false;

    /* Deepest point of the frame so far. Recorded only, so it cannot move the
     * timing it measures. */
    if (window_power_ < frame_dip_window_) {
        frame_dip_window_ = window_power_;
        frame_dip_reference_ = reference_power_;
        frame_dip_offset_ = since_null_;
    }

    return window_power_ * kQuietRatio < reference_power_;
}

void DabProcessor::advance_blocks(size_t n) {
    idle_samples_ += n;
    since_null_ += n;
    if (state_ != State::Symbols)
        hunting_ += n;
    /* Hunting, restart the block so a short one cannot pass for a null; else
     * keep the block phase, whose part-filled verdict nothing reads. */
    if (state_ == State::Searching) {
        block_fill_ = 0;
        block_power_ = 0;
    } else {
        block_fill_ = (block_fill_ + n) % kPowerBlock;
    }
}

/* Moves the receiver on by n samples it never saw, state by state, as
 * execute_fast() would have. A gap of a frame or more leaves nothing to hold
 * on to, so that re-acquires. */
void DabProcessor::skip_samples(size_t n) {
    if (n >= kSamplesPerFrame) {
        restart_acquisition();
        return;
    }
    prs_timing_shots_ = 0;
    lap_watch_ = kLapWatchFrames;
    lap_guard_bad_ = 0;
    while (n > 0) {
        if (state_ == State::Symbols) {
            symbol_damaged_ = true;
            const size_t room = dab::kSymbolSamples - symbol_fill_;
            const size_t take = n < room ? n : room;
            advance_blocks(take);
            symbol_fill_ += take;
            n -= take;
            if (symbol_fill_ >= dab::kSymbolSamples) {
                symbol_fill_ = 0;
                on_symbol_complete();
                symbol_damaged_ = n > 0;
            }
        } else if (state_ == State::Sync) {
            /* Sync -> Symbols fires on the first block boundary once the
             * countdown is out; if that falls in the gap, the frame starts there. */
            const size_t to_boundary = (kPowerBlock - (block_fill_ + null_remaining_) % kPowerBlock) % kPowerBlock;
            const size_t start = null_remaining_ + to_boundary;
            if (n < start) {
                advance_blocks(n);
                null_remaining_ = null_remaining_ > n ? static_cast<uint32_t>(null_remaining_ - n) : 0;
                symbol_damaged_ = false;
                n = 0;
            } else {
                advance_blocks(start);
                null_remaining_ = 0;
                n -= start;
                on_power_block(false);
                /* Its pre-roll was in the gap. */
                symbol_damaged_ = true;
            }
        } else {
            advance_blocks(n);
            symbol_damaged_ = false;
            n = 0;
        }
    }
}

void DabProcessor::execute_fast(const buffer_c8_t& buffer) {
    size_t i = 0;

    while (i < buffer.count) {
        if (state_ != State::Symbols) {
            /* Hunting: accumulate the envelope over a chunk and enter the
             * state machine only when a power block completes. */
            size_t n = buffer.count - i;
            const size_t block_room = kPowerBlock - block_fill_;
            if (n > block_room)
                n = block_room;

            const complex8_t* __restrict src = &buffer.p[i];
            uint32_t power = block_power_;
            size_t phase = block_fill_ % kPowerDecim;

            for (size_t k = 0; k < n; k++) {
                if (phase == 0)
                    power += sample_power(src[k].real(), src[k].imag());
                if (++phase == kPowerDecim)
                    phase = 0;
            }

            block_power_ = power;
            block_fill_ += n;
            idle_samples_ += n;
            since_null_ += n;
            hunting_ += n;
            if (null_remaining_ > 0)
                null_remaining_ = null_remaining_ > n ? null_remaining_ - n : 0;

            /* The pre-roll matters only when the symbol phase starts, so keep
             * just the tail of the chunk. */
            const size_t keep = n < kPreRollSamples ? n : kPreRollSamples;
            for (size_t k = 0; k < keep; k++) {
                const auto& sm = src[n - keep + k];
                preroll_[preroll_write_] = dab::Iq16{static_cast<int16_t>(sm.real() >> kInputShift),
                                                     static_cast<int16_t>(sm.imag() >> kInputShift)};
                preroll_write_ = (preroll_write_ + 1) % kPreRollSamples;
            }

            i += n;

            if (block_fill_ >= kPowerBlock) {
                const bool quiet = consume_power_block(block_power_);
                block_power_ = 0;
                block_fill_ = 0;
                on_power_block(quiet);
            }

            if (state_ != State::Symbols && idle_samples_ >= kSamplesPerFrame)
                publish_status(false);

            continue;
        }

        /* Chunk bounded by the buffer, the symbol and the next power block, so
         * the inner loop carries no conditionals. */
        size_t n = buffer.count - i;
        const size_t symbol_room = dab::kSymbolSamples - symbol_fill_;
        if (n > symbol_room)
            n = symbol_room;
        const size_t block_room = kPowerBlock - block_fill_;
        if (n > block_room)
            n = block_room;

        const complex8_t* __restrict src = &buffer.p[i];
        dab::Iq16* __restrict dst = &symbol_[symbol_fill_];

        uint32_t power = block_power_;
        size_t k = 0;

        /* The envelope keeps running inside symbols: hunting-only, it would see
         * little but the null and drag the reference down. Its samples sit at
         * multiples of kPowerDecim within the block, and a chunk never crosses
         * a block, so they get their own loop. */
        {
            const size_t first = (kPowerDecim - (block_fill_ % kPowerDecim)) % kPowerDecim;
            for (size_t kk = first; kk < n; kk += kPowerDecim)
                power += sample_power(src[kk].real() >> kInputShift, src[kk].imag() >> kInputShift);
        }

        /* A complex8_t pair is one word, i0 q0 i1 q1. SXTB16 sign-extends bytes
         * 0,2 (with ror #8, bytes 1,3) into (i0, i1) and (q0, q1); PKHBT/PKHTB
         * interleave them into Iq16 pairs. Word loads must be 4-byte aligned or
         * the compiler emits byte loads, so an odd first sample is stepped over. */
#if defined(__ARM_FEATURE_SIMD32)
        static_assert(kInputShift == 0, "the packed copy assumes native 8-bit scale");

        if ((reinterpret_cast<uintptr_t>(src) & 3u) != 0 && n > 0) {
            dst[0] = dab::Iq16{static_cast<int16_t>(src[0].real()),
                               static_cast<int16_t>(src[0].imag())};
            k = 1;
        }
        for (; k + 4 <= n; k += 4) {
            uint32_t w0, w1, ii, qq, d0, d1, d2, d3;
            __builtin_memcpy(&w0, &src[k], sizeof(w0));
            __builtin_memcpy(&w1, &src[k + 2], sizeof(w1));
            __asm("sxtb16 %0, %1" : "=r"(ii) : "r"(w0));
            __asm("sxtb16 %0, %1, ror #8" : "=r"(qq) : "r"(w0));
            __asm("pkhbt %0, %1, %2, lsl #16" : "=r"(d0) : "r"(ii), "r"(qq));
            __asm("pkhtb %0, %1, %2, asr #16" : "=r"(d1) : "r"(qq), "r"(ii));
            __asm("sxtb16 %0, %1" : "=r"(ii) : "r"(w1));
            __asm("sxtb16 %0, %1, ror #8" : "=r"(qq) : "r"(w1));
            __asm("pkhbt %0, %1, %2, lsl #16" : "=r"(d2) : "r"(ii), "r"(qq));
            __asm("pkhtb %0, %1, %2, asr #16" : "=r"(d3) : "r"(qq), "r"(ii));
            __builtin_memcpy(static_cast<void*>(&dst[k]), &d0, sizeof(d0));
            __builtin_memcpy(static_cast<void*>(&dst[k + 1]), &d1, sizeof(d1));
            __builtin_memcpy(static_cast<void*>(&dst[k + 2]), &d2, sizeof(d2));
            __builtin_memcpy(static_cast<void*>(&dst[k + 3]), &d3, sizeof(d3));
        }
        for (; k + 2 <= n; k += 2) {
            uint32_t w, ii, qq, d0, d1;
            __builtin_memcpy(&w, &src[k], sizeof(w));
            __asm("sxtb16 %0, %1" : "=r"(ii) : "r"(w));
            __asm("sxtb16 %0, %1, ror #8" : "=r"(qq) : "r"(w));
            __asm("pkhbt %0, %1, %2, lsl #16" : "=r"(d0) : "r"(ii), "r"(qq));
            __asm("pkhtb %0, %1, %2, asr #16" : "=r"(d1) : "r"(qq), "r"(ii));
            __builtin_memcpy(static_cast<void*>(&dst[k]), &d0, sizeof(d0));
            __builtin_memcpy(static_cast<void*>(&dst[k + 1]), &d1, sizeof(d1));
        }
#endif
        /* Tail, and the whole loop on a host without SIMD32. */
        for (; k < n; k++)
            dst[k] = dab::Iq16{static_cast<int16_t>(src[k].real() >> kInputShift),
                               static_cast<int16_t>(src[k].imag() >> kInputShift)};

        block_power_ = power;
        block_fill_ += n;
        symbol_fill_ += n;
        idle_samples_ += n;
        since_null_ += n;
        i += n;

        if (block_fill_ >= kPowerBlock) {
            /* The verdict matters only while hunting. */
            (void)consume_power_block(block_power_);
            block_power_ = 0;
            block_fill_ = 0;
        }

        if (symbol_fill_ >= dab::kSymbolSamples) {
            symbol_fill_ = 0;
            on_symbol_complete();
        }
    }
}

/* The acquisition state machine, driven once per completed power block. */
void DabProcessor::on_power_block(bool quiet) {
    if (state_ == State::Searching) {
        /* One quiet window decides: it is full of null by then, so the null
         * began kNullWindowBlocks blocks ago and ends a known distance on.
         * Waiting for the window to clear lags by its own length. */
        if (quiet) {
            state_ = State::Sync;
            /* A fresh placement from the null; the trim budget starts over. */
            timing_total_ = 0;
            fine_phase_ = 0;
            search_samples_ = hunting_;

            const size_t seen = kNullWindowBlocks * kPowerBlock;
            null_remaining_ = dab::kNullSamples > seen ? dab::kNullSamples - seen : 0;

            /* The detector fires on the power dip, about half a guard interval
             * early; kStartOffset sets where in the guard the symbol phase
             * lands. Once per acquisition. */
            null_remaining_ += kStartOffset;
        }
        return;
    }

    /* Sync: the rest of the null is counted out, not watched; the phase
     * reference is the first symbol after it. */
    if (null_remaining_ == 0) {
        state_ = State::Symbols;

        /* Replay the pre-roll so the symbol starts before the detected edge
         * rather than after it. */
        symbol_fill_ = 0;
        for (size_t n = fine_phase_; n < kPreRollSamples; n++)
            symbol_[symbol_fill_++] = preroll_[(preroll_write_ + n) % kPreRollSamples];

        /* A frame is exactly 196608 samples; anything else means the detector
         * is missing the null and the lock is an artefact of differential
         * demodulation, which works on any consecutive pair of symbols. */
        null_interval_ = since_null_;

        /* A boundary in the right place puts the frame's deepest dip just under
         * 196608, in the null that closes it; anything else is the error in
         * samples, which free-running hides (since_null_ counts by arithmetic). */
        null_dip_offset_ = frame_dip_offset_;
        const uint32_t dip_window = frame_dip_window_ ? frame_dip_window_ : 1;
        const uint32_t dip_ratio = frame_dip_reference_ / dip_window;
        null_dip_ratio_ = static_cast<uint16_t>(dip_ratio > 0xFFFF ? 0xFFFF : dip_ratio);
        frame_dip_window_ = 0xFFFFFFFF;
        frame_dip_reference_ = 1;
        frame_dip_offset_ = 0;
        sync_samples_ = hunting_ > search_samples_ ? hunting_ - search_samples_ : 0;
        since_null_ = 0;
        hunting_ = 0;

        symbol_index_ = 0;
        fic_fill_ = 0;
        fic_rotate_ = static_cast<uint8_t>((fic_rotate_ + 1) % dab::kFicCodewordsPerFrame);
        have_reference_ = false;
        fib_errors_ = 0;
        /* Not idle_samples_: publish_status() divides the cycle measurement by
         * it, and clearing it here would hide the cost of acquiring. */
    }
}

/* Partial guard correlation at a shifted window, in arbitrary units. Only the
 * magnitude relative to its neighbours matters, so this skips the
 * normalisation and every fourth sample. */
static uint32_t guard_energy(const dab::Iq16* symbol, int32_t shift, int32_t bias) {
    /* The far tap, base + t + kFftSize, stays inside kSymbolSamples (2552) for
     * shifts in [-64, +64]: at bias 0 it reaches 2428. The static_assert on
     * kGateBias bounds `bias`. */
    const size_t base = static_cast<size_t>(64 + bias + shift);
    int64_t re = 0, im = 0;
    for (size_t t = 0; t < 256; t += 4) {
        const auto& a = symbol[base + t];
        const auto& b = symbol[base + t + dab::kFftSize];
        re += static_cast<int32_t>(b.i) * a.i + static_cast<int32_t>(b.q) * a.q;
        im += static_cast<int32_t>(b.q) * a.i - static_cast<int32_t>(b.i) * a.q;
    }
    const int64_t mag2 = re * re + im * im;
    return static_cast<uint32_t>(mag2 >> 20);
}

void DabProcessor::measure_spectrum(uint16_t& dc_out, uint16_t& used_out, uint16_t& unused_out) {
    uint64_t dc = 0, used = 0, unused = 0;
    for (size_t bin = 0; bin < dab::kFftSize; bin++) {
        const uint64_t p = static_cast<uint64_t>(static_cast<int32_t>(fft_[bin].re) * fft_[bin].re +
                                                 static_cast<int32_t>(fft_[bin].im) * fft_[bin].im);
        /* The block sits a quarter of the transform up; nothing rotates it
         * to DC. */
        const size_t centred = (bin + dab::kFftSize - dab::kFftSize / 4) & (dab::kFftSize - 1);
        if (centred == 0)
            dc += p;
        else if (centred <= 768 || centred >= 1280)
            used += p;
        else
            unused += p;
    }
    const uint64_t total = dc + used + unused;
    if (total == 0)
        return;
    dc_out = static_cast<uint16_t>(dc * 1000 / total);
    used_out = static_cast<uint16_t>(used * 1000 / total);
    unused_out = static_cast<uint16_t>(unused * 1000 / total);
}

void DabProcessor::transform_symbol() {
    transform_stage();
    transform_finish();
}

/* The cheap half: remove the residual offset and copy the window into fft_,
 * at the symbol's own boundary before symbol_ is overwritten. The copy fills
 * all of fft_, so staging needs no other buffer. */
void DabProcessor::transform_stage() {
    const uint32_t transform_started = cycle_count();

    /* The residual offset turns the differential phase by 2*pi*f*T_symbol (50 Hz
     * is over 20 degrees a symbol), which r4 cannot see. The phase accumulates
     * across calls, so consecutive symbols keep their differential. It also
     * applies fft_boost_. */
    corrector_.apply(symbol_.data(), symbol_.size());

    /* Start kFftWindowBias inside the guard: early only reaches into the cyclic
     * guard, late crosses into the next symbol. */
    for (size_t n = 0; n < dab::kFftSize; n++) {
        const auto& s = symbol_[dab::kGuardSamples - kFftWindowBias + n];
        fft_[n].re = s.i;
        fft_[n].im = s.q;
    }

    transform_work_cycles_ += cycle_count() - transform_started;
}

/* Copy symbol_'s transform window into defer_stage_, saturating: a rotated
 * or boosted sample can pass int8, though fft_boost_ leaves only rare peaks. */
void DabProcessor::window_to_stage() {
    for (size_t n = 0; n < dab::kFftSize; n++) {
        const auto& v = symbol_[dab::kGuardSamples - kFftWindowBias + n];
        defer_stage_[2 * n] = sat_int8(v.i);
        defer_stage_[2 * n + 1] = sat_int8(v.q);
    }
}

/* Stage a symbol in defer_stage_ for the third transform of a burst; fft_
 * holds only one. */
void DabProcessor::stage_to_buf() {
    const uint32_t transform_started = cycle_count();
    /* At the symbol's own boundary, so the phase accumulator still advances
     * one symbol per transformed symbol and the differential is unchanged. */
    corrector_.apply(symbol_.data(), symbol_.size());
    window_to_stage();
    transform_work_cycles_ += cycle_count() - transform_started;
}

void DabProcessor::buf_to_fft() {
    for (size_t n = 0; n < dab::kFftSize; n++) {
        fft_[n].re = defer_stage_[2 * n];
        fft_[n].im = defer_stage_[2 * n + 1];
    }
}

/* Phase reference correlation against EN 300 401 table 44, the only measure
 * that sees an integer carrier offset. Spectrum times the reference's
 * conjugate, transformed back, is the impulse response: peak-to-mean says
 * whether the carriers are where assumed, the peak's position the window error. */
void DabProcessor::prs_forward() {
    buf_to_fft();
    uint8_t sh = 0;
    dab::fft2048(fft_.data(), &sh);
    /* The block sits kFftSize/4 bins up (nothing rotates it to DC); the
     * candidate carrier shift rides on top. */
    const size_t shift = (dab::kFftSize / 4 + dab::kFftSize +
                          static_cast<size_t>(static_cast<int32_t>(dab::kFftSize) + prs_shift_)) %
                         dab::kFftSize;
    dab::prs_conjugate_multiply(fft_.data(), shift);
    /* Conjugate for the inverse transform, which is the forward one with the
     * input and the output conjugated. */
    for (size_t n = 0; n < dab::kFftSize; n++)
        fft_[n].im = static_cast<int16_t>(-fft_[n].im);
}

void DabProcessor::prs_inverse() {
    uint8_t sh = 0;
    dab::fft2048(fft_.data(), &sh);

    /* Peak of the impulse response over the mean as its noise floor: the ratio
     * says whether the reference matched, the height only the signal strength. */
    uint32_t peak = 0;
    size_t peak_at = 0;
    uint64_t total = 0;
    for (size_t n = 0; n < dab::kFftSize; n++) {
        const int32_t re = fft_[n].re;
        const int32_t im = fft_[n].im;
        const uint32_t mag = static_cast<uint32_t>(re * re + im * im);
        total += mag;
        if (mag > peak) {
            peak = mag;
            peak_at = n;
        }
    }
    const uint32_t mean = static_cast<uint32_t>(total / dab::kFftSize);
    /* Peak over mean in tenths, saturated. */
    uint32_t ratio = mean > 0 ? static_cast<uint32_t>((static_cast<uint64_t>(peak) * 10) / mean) : 0;
    if (ratio > 65535)
        ratio = 65535;

    if (ratio > prs_sweep_best_) {
        prs_sweep_best_ = static_cast<uint16_t>(ratio);
        prs_sweep_best_shift_ = prs_shift_;
        prs_sweep_best_at_ = static_cast<uint16_t>(peak_at);
    }

    /* Close the timing loop: a correctly timed window puts the impulse at
     * kFftWindowBias, and any other position is that many samples of error,
     * which the early-late gate cannot see on a ~640-sample guard plateau. */
    if (ratio >= kPrsTrustPeak && prs_shift_ == 0) {
        int32_t error = static_cast<int32_t>(peak_at) - static_cast<int32_t>(kFftWindowBias);
        if (error > static_cast<int32_t>(dab::kFftSize) / 2)
            error -= static_cast<int32_t>(dab::kFftSize);
        else if (error < -static_cast<int32_t>(dab::kFftSize) / 2)
            error += static_cast<int32_t>(dab::kFftSize);
        /* Bounded, so one bad measurement cannot throw the frame away. */
        if (error > kPrsMaxStep)
            error = kPrsMaxStep;
        else if (error < -kPrsMaxStep)
            error = -kPrsMaxStep;
        prs_timing_error_ = error;
        prs_timing_valid_ = true;
    }

    /* Pinned: this correlation is the verdict, so publish it and skip the
     * other candidates. */
    if (prs_pinned_) {
        prs_peak_ = static_cast<uint16_t>(ratio);
        prs_best_shift_ = static_cast<int16_t>(static_cast<int16_t>(peak_at << 4));
        if (count_prs_low())
            prs_pinned_hunts_ = 0;
        prs_sweep_best_ = 0;
        return;
    }

    /* Next candidate; the sweep publishes its winner when it wraps, so the
     * reported pair is always from a complete sweep. */
    if (++prs_shift_ > kPrsSweep) {
        prs_shift_ = -kPrsSweep;
        prs_peak_ = prs_sweep_best_;

        /* Pin the offset once a sweep finds it zero with a real correlation. */
        if (prs_sweep_best_shift_ == 0 && prs_sweep_best_ >= kPrsHuntMin) {
            prs_pinned_ = true;
            prs_shift_ = 0;
        }

        count_prs_low();
        /* The impulse position (the timing error, see above) rides above the
         * winning shift, which needs only four bits. */
        prs_best_shift_ = static_cast<int16_t>((prs_sweep_best_shift_ & 0x0F) |
                                               static_cast<int16_t>(prs_sweep_best_at_ << 4));
        prs_sweep_best_ = 0;
    }
}

/* A published peak judges the grid's numbering: under kPrsHuntMin, symbol 0
 * is not the phase reference whatever guard and r4 say. Counts those in a
 * row; true when this one cleared the threshold. */
bool DabProcessor::count_prs_low() {
    if (prs_peak_ >= kPrsHuntMin) {
        prs_low_sweeps_ = 0;
        return true;
    }
    if (prs_low_sweeps_ < 0xFF)
        ++prs_low_sweeps_;
    return false;
}

/* From every other sample of symbol 0's useful part, before any transform of
 * the frame (symbol_ is still uncorrected): as many doublings as keep the mean
 * power at most kFftBoostPower. */
void DabProcessor::set_fft_boost() {
    uint32_t sum = 0;
    for (size_t n = dab::kGuardSamples; n < dab::kSymbolSamples; n += 2) {
        const int32_t i = symbol_[n].i;
        const int32_t q = symbol_[n].q;
        sum += static_cast<uint32_t>(i * i + q * q);
    }
    const uint32_t mean = sum / (dab::kFftSize / 2);
    uint8_t boost = 0;
    while (boost < kFftBoostMax && (mean << (2 * (boost + 1))) <= kFftBoostPower)
        ++boost;
    fft_boost_ = boost;
    corrector_.set_boost(boost);
}

/* The expensive half: the 2048-point transform and the carrier extraction,
 * over whatever is already sitting in fft_. */
void DabProcessor::transform_finish() {
    const uint32_t transform_started = cycle_count();

    exec_tags_ |= kExecTransform;
    dab::fft2048(fft_.data(), nullptr);

    /* Spectrum occupancy, once a frame: power outside the 1536 carriers means
     * the block is not centred, and power in bin 0 is the front end's DC, which
     * correlates at every lag and blinds the guard measurement. */
    if (symbol_index_ == 0)
        measure_spectrum(spectrum_dc_, spectrum_used_, spectrum_unused_);

    /* Pull the active carriers out in frequency-deinterleaved order. Adding
     * kFftSize/4 to the bin is the whole of the fs/4 down-conversion. */
    constexpr size_t kFsOver4Bins = dab::kFftSize / 4;
    auto& current = use_a_ ? carriers_a_ : carriers_b_;
    const int16_t* map = dab::variant_freq_inverse() ? dab::frequency_map_inverse()
                                                     : dab::frequency_map();

    /* An integer carrier offset moves every carrier by whole bins and is
     * invisible to the guard correlation; see dab_variant.hpp. Not a table:
     * it would cost 3 KB of heap to save an add. */
    const int16_t shift = static_cast<int16_t>(dab::carrier_shift) - dab::kCarrierShiftBias;

    for (size_t k = 0; k < dab::kCarriers; k++) {
        const size_t bin =
            (dab::carrier_to_fft_bin(static_cast<int16_t>(map[k] + shift)) + kFsOver4Bins) &
            (dab::kFftSize - 1);
        current[k] = dab::Iq16{fft_[bin].re, fft_[bin].im};
    }

    transform_work_cycles_ += cycle_count() - transform_started;
}

void DabProcessor::on_symbol_complete() {
    const uint32_t symbol_started = cycle_count();
    ++shared_memory.m4_symbols_total;
    /* Cleared here, not after the transform: the thread returns for a buffer
     * only once the symbol's work is done, so a drop a transform causes is
     * always detected after it. */
    shared_memory.m4_in_burst = 0;

    /* A symbol the DMA lap cut (skip_samples()): no transform reads it, and its
     * subchannel bits go in as erasures. Symbol 0's estimates run on it, which
     * one damaged frame cannot steer (low guard, and PRS needs two low peaks). */
    const bool erased = symbol_damaged_;
    symbol_damaged_ = false;

    /* Before anything touches symbol_: transform_stage() corrects it in place,
     * so accumulating later would measure the residual on some symbols and the
     * absolute offset on others. Symbol 0 is skipped, as the references do. */
    if (symbol_index_ != 0) {
        dab::accumulate_frequency_offset(symbol_.data(), &freq_acc_re_, &freq_acc_im_);
        if (freq_acc_n_ < 0xFFFF)
            ++freq_acc_n_;
    }

    /* A deferred transform finishes only inside the CIF it was staged in. On
     * a frame restart or a release, drop whatever is held rather than demap
     * it against another frame's reference. */
    if (symbol_index_ == 0) {
        staged_fft_ = kStageEmpty;
        staged_buf_ = kStageEmpty;
        staged_is_fic_ = false;
    } else if (!msc_enabled_ && !staged_is_fic_) {
        staged_fft_ = kStageEmpty;
        staged_buf_ = kStageEmpty;
    }

    if (symbol_index_ == 0) {
        /* Timing trim while acquiring, frozen once a subchannel runs: the gate
         * reads symbol 0, which the subchannel's load makes unreliable, and a
         * subchannel is selected only at a good r4, so the grid was right then. */
        bool prs_drove = false;
        /* The phase reference measures the error outright. Consumed once
         * applied, so a stale measurement is never re-applied. */
        if (prs_timing_valid_ && prs_timing_shots_ < kPrsTimingShots) {
            prs_timing_valid_ = false;
            ++prs_timing_shots_;
            timing_adjust_ = prs_timing_error_;
            const int32_t next = timing_total_ + prs_timing_error_;
            if (next <= kTimingTrimLimit && next >= -kTimingTrimLimit)
                timing_total_ = next;
            else
                timing_adjust_ = 0;
            prs_drove = true;
        }
        const bool trim_settled = (kTimingFreezeR4 != 0 && constellation_r4_ >= kTimingFreezeR4) || msc_enabled_;
        if (prs_drove) {
            /* already steered from the impulse response */
        } else if (trim_settled) {
            timing_adjust_ = 0;
        } else {
        /* Early-late gate on the guard correlation (plateau ~640 samples). */
        const uint32_t early = guard_energy(symbol_.data(), -64, kGateBias);
        const uint32_t centre = guard_energy(symbol_.data(), 0, kGateBias);
        const uint32_t late = guard_energy(symbol_.data(), +64, kGateBias);
        int32_t step = 0;
        if (late > centre && late > early)
            step = kTimingStep;
        else if (early > centre && early > late)
            step = -kTimingStep;

        /* Never let the trim walk off the null: the guard correlation repeats
         * every symbol, so the gate could settle a whole symbol out. The null
         * sets the frame; this only trims inside it. */
        const int32_t next = timing_total_ + step;
        if (next > kTimingTrimLimit || next < -kTimingTrimLimit)
            step = 0;
        else
            timing_total_ = next;

        timing_adjust_ = step;
        }

        /* Follow the sample clock's drift once the PRS shots are spent and the
         * trim is frozen. Tuner and ADC share a crystal, so an offset of f Hz at
         * carrier C walks the frame -f/C * kSamplesPerFrame samples a frame;
         * whole samples steer now, the rest is carried (DAB-IMPL 6.107). */
        if (corrector_primed_ && drift_per_hz_ != 0.0f) {
            drift_owed_ -= corrector_offset_ * drift_per_hz_;
            const int32_t whole = static_cast<int32_t>(drift_owed_);
            timing_adjust_ += whole;
            drift_owed_ -= static_cast<float>(whole);
        }

        /* guard_quality_ from symbol 0 says where the window sits. The offset
         * comes from the sum over the frame's data symbols when there are
         * enough: symbol 0 alone is far noisier and the likeliest damaged. */
        float error = dab::estimate_frequency_offset(symbol_.data(), &guard_quality_);
        if (freq_acc_n_ >= kFreqAccMinSymbols)
            error = dab::frequency_offset_from_sum(freq_acc_re_, freq_acc_im_);
        freq_acc_re_ = 0;
        freq_acc_im_ = 0;
        freq_acc_n_ = 0;
        frequency_error_ = static_cast<int16_t>(error);
        /* A raw per-frame estimate's median is within a couple of Hz but its
         * tail reaches hundreds, which destroys the frame. The true offset is a
         * TCXO's and barely moves: gate on guard quality and smooth slowly. An
         * erased symbol 0 reads a low guard and so moves nothing. */
        if (guard_quality_ >= kCorrectorGate) {
            if (!corrector_primed_) {
                corrector_offset_ = error;
                corrector_primed_ = true;
            } else {
                corrector_offset_ += (error - corrector_offset_) * (1.0f / kCorrectorIir);
            }
        }
        if (corrector_primed_)
            corrector_.set_offset(corrector_offset_);

        set_fft_boost();
    }

    /* The reference is the symbol at fic_offset - 1 and the FIC the three after
     * it; offset 1 is the specification's layout (dab_variant.hpp). */
    const size_t fic_first = fic_start_;

    /* Only the symbols this frame's codeword is made of. A codeword is 2304
     * bits and a symbol pair yields 3072, so codeword r spans pairs
     * floor(r*2304/3072) to floor(((r+1)*2304-1)/3072): {0}, {0,1}, {1,2},
     * {2} for r = 0..3. */
    const size_t cw_first_bit = static_cast<size_t>(fic_rotate_) * dab::kFicPuncturedBits;
    const size_t cw_last_bit = cw_first_bit + dab::kFicPuncturedBits - 1;
    const size_t pair_first = cw_first_bit / kSoftBitsPerSymbol;
    const size_t pair_last = cw_last_bit / kSoftBitsPerSymbol;

    /* Once a subchannel runs the FIC is only a watch for reconfigurations,
     * which are announced seconds ahead: decode one frame in kFicDutyFrames
     * and leave the transforms and the trellis to the MSC. */
    const bool fic_this_frame = !msc_enabled_ || (fic_duty_ == 0);
    const bool want_fic = fic_this_frame &&
                          symbol_index_ + 1 >= fic_first + pair_first &&
                          symbol_index_ <= fic_first + pair_last;
    /* The subchannel's symbols plus the one before each, the differential
     * reference; never the FIC's symbols 1 to 3. */
    bool want_msc = msc_enabled_ && symbol_index_ > dab::kFicSymbols &&
                    (msc_symbol_wanted(symbol_index_) || msc_symbol_wanted(symbol_index_ + 1));
    /* CIF 0 given to the phase reference: nothing of the subchannel's is
     * transformed, and the whole slice is written as erasures on the CIF's last
     * symbol, before its decode_cif(), no earlier than its own finishes. */
    if (msc_cif0_erased_ && symbol_index_ <= dab::kFicSymbols + kMscSymbolsPerCif) {
        if (msc_enabled_ && symbol_index_ == dab::kFicSymbols + kMscSymbolsPerCif)
            for (size_t i = 0; i < kMscSymbolsPerCif; ++i)
                accumulate_msc(i, true);
        want_msc = false;
    }

    /* Stage the phase reference's window before symbol_ is overwritten; the
     * transforms run later, on symbols the subchannel leaves alone. */
    if (symbol_index_ == 0) {
        msc_cif0_erased_ = false;
        if (++prs_frame_ >= kPrsFrameDivider) {
            prs_frame_ = 0;
            bool stage = !prs_window_clash();
            if (!stage && prs_timing_shots_ < kPrsTimingShots && prs_clash_frames_ < kPrsClashFrames) {
                ++prs_clash_frames_;
                msc_cif0_erased_ = true;
                stage = true;
            }
            if (stage) {
                window_to_stage();
                prs_staged_ = true;
            }
        }
    } else if (prs_staged_ && symbol_index_ == kPrsForwardSymbol) {
        prs_forward();
    } else if (prs_staged_ && symbol_index_ == kPrsInverseSymbol) {
        prs_inverse();
        prs_staged_ = false;
    }

    /* Spread the subchannel's three transforms (its pair and the reference
     * before it) over five symbols: back to back they are ~6 ms of work against
     * 3.7 ms of arrival, more than the 4 ms ring absorbs. E.g. 12 now, 13 staged
     * in fft_, 14 in defer_stage_; 15 and 16 finish them and demap the pairs. */
    BurstStep step = BurstStep::kNone;
    if (want_fic) {
        /* The FIC defers only the last symbol of its burst, into fft_: two
         * transforms in two symbol times fit the ring, and the last of a
         * burst is the one that gets destroyed. */
        const bool last_of_burst = symbol_index_ >= fic_first + pair_first &&
                                   symbol_index_ == fic_first + pair_last &&
                                   pair_last > pair_first;
        step = last_of_burst ? BurstStep::kStageFft : BurstStep::kNow;
        if (last_of_burst && !erased)
            staged_is_fic_ = true;
    } else if (want_msc) {
        /* The reference symbol is transformed at once: it is alone, so it
         * fits, and it has to be in carriers_ before the pair is demapped. */
        if (!msc_symbol_wanted(symbol_index_))
            step = BurstStep::kNow;
        else if (msc_symbol_wanted(symbol_index_ + 1))
            step = BurstStep::kStageFft;
        else
            step = BurstStep::kStageBuf;
    } else if (fic_staged() && symbol_index_ == fic_finish_symbol_) {
        step = BurstStep::kFinishFft;
    } else if (msc_enabled_ && !staged_is_fic_ && staged_fft_ != kStageEmpty) {
        step = BurstStep::kFinishFft;
    } else if (msc_enabled_ && staged_buf_ != kStageEmpty) {
        step = BurstStep::kFinishBuf;
    }
    /* An erased symbol neither transforms nor stages; finishing a symbol
     * staged before the gap still runs. */
    if (erased && (step == BurstStep::kNow || step == BurstStep::kStageFft || step == BurstStep::kStageBuf)) {
        if (want_msc && msc_symbol_wanted(symbol_index_))
            accumulate_msc((symbol_index_ - dab::kFicSymbols - 1) % kMscSymbolsPerCif, true);
        step = BurstStep::kNone;
    }
    const bool produces_carriers = (step == BurstStep::kNow || step == BurstStep::kFinishFft ||
                                    step == BurstStep::kFinishBuf);
    /* The CIF symbol the carriers about to be produced belong to; with deferred
     * transforms it does not follow from the current symbol. */
    size_t pair_in_cif = 0;
    if (symbol_index_ > dab::kFicSymbols && symbol_index_ < dab::kSymbolsPerFrame)
        pair_in_cif = (symbol_index_ - dab::kFicSymbols - 1) % kMscSymbolsPerCif;
    /* Likewise which FIC pair, and whether they are the FIC's at all. */
    bool fic_carriers = want_fic;
    size_t fic_pair = symbol_index_ >= dab::fic_offset ? symbol_index_ - dab::fic_offset : 0;

    if (step != BurstStep::kNone) {
        /* Lets the DMA clamp attribute a lapped transfer to the burst or to
         * everything else (m4_in_burst). */
        shared_memory.m4_in_burst = 1;
        ++shared_memory.m4_symbols_in_burst;
        switch (step) {
            case BurstStep::kNow:
                transform_symbol();
                break;
            case BurstStep::kStageFft:
                transform_stage();
                staged_fft_ = static_cast<uint8_t>(staged_is_fic_ ? fic_pair : pair_in_cif);
                break;
            case BurstStep::kStageBuf:
                stage_to_buf();
                staged_buf_ = static_cast<uint8_t>(pair_in_cif);
                break;
            case BurstStep::kFinishFft:
                transform_finish();
                if (staged_is_fic_) {
                    fic_carriers = true;
                    fic_pair = staged_fft_;
                    staged_is_fic_ = false;
                } else {
                    pair_in_cif = staged_fft_;
                }
                staged_fft_ = kStageEmpty;
                break;
            case BurstStep::kFinishBuf:
                buf_to_fft();
                transform_finish();
                pair_in_cif = staged_buf_;
                staged_buf_ = kStageEmpty;
                break;
            default:
                break;
        }

        if (produces_carriers) {
        const auto& current = use_a_ ? carriers_a_ : carriers_b_;
        const auto& previous = use_a_ ? carriers_b_ : carriers_a_;

        if (have_reference_) {
            /* r4 folds the four DQPSK points onto one: (z/|z|)^4 = z^4/|z|^4, no
             * atan2. MSC pairs are gated on !want_fic, since a deferred demap
             * lands after want_msc goes false. Two loops, not a helper, which GCC
             * would fuse into vfma and so change the MSC sum's rounding. */
            /* Only the subchannel's first pair: CIF symbol s holds CUs 48s to
             * 48s+47, so that is the symbol its start CU falls in. */
            const size_t first_pair =
                (static_cast<size_t>(msc_start_cu_) * dab::kBitsPerCu) / kSoftBitsPerSymbol;
            if (!want_fic && pair_in_cif == first_pair) {
                for (size_t k = 0; k < dab::kCarriers; k += 4) {
                    const float re = static_cast<float>(static_cast<int32_t>(current[k].i) * previous[k].i +
                                                        static_cast<int32_t>(current[k].q) * previous[k].q);
                    const float im = static_cast<float>(static_cast<int32_t>(current[k].q) * previous[k].i -
                                                        static_cast<int32_t>(current[k].i) * previous[k].q);
                    const float m2 = re * re + im * im;
                    if (m2 <= 0.0f)
                        continue;
                    const float z2re = re * re - im * im;
                    const float z2im = 2.0f * re * im;
                    const float inv = 1.0f / (m2 * m2);
                    msc_r4_re_ += (z2re * z2re - z2im * z2im) * inv;
                    msc_r4_im_ += (2.0f * z2re * z2im) * inv;
                    ++msc_r4_n_;
                }
            }

            if (want_fic) {
                for (size_t k = 0; k < dab::kCarriers; k++) {
                    const float re = static_cast<float>(static_cast<int32_t>(current[k].i) * previous[k].i +
                                                        static_cast<int32_t>(current[k].q) * previous[k].q);
                    const float im = static_cast<float>(static_cast<int32_t>(current[k].q) * previous[k].i -
                                                        static_cast<int32_t>(current[k].i) * previous[k].q);
                    const float m2 = re * re + im * im;
                    if (m2 <= 0.0f)
                        continue;
                    const float z2re = re * re - im * im;
                    const float z2im = 2.0f * re * im;
                    const float inv = 1.0f / (m2 * m2);
                    r4_acc_re_ += (z2re * z2re - z2im * z2im) * inv;
                    r4_acc_im_ += (2.0f * z2re * z2im) * inv;
                    ++r4_count_;
                }
            }

            dab::dqpsk_demap(previous.data(), current.data(), dab::kCarriers, symbol_soft_.data());

            if (fic_carriers)
                accumulate_fic(fic_pair);
            else
                accumulate_msc(pair_in_cif);
        } else if (!fic_carriers) {
            /* Its reference was erased: erasures, not the last CIF's bits. */
            accumulate_msc(pair_in_cif, true);
        }

        have_reference_ = true;
        use_a_ = !use_a_;
        }
    } else if (fic_staged()) {
        /* A deferred FIC transform's reference is its predecessor, still in
         * carriers_a_/carriers_b_; clearing have_reference_ would skip that
         * pair's demap, and no codeword spanning two pairs could complete. */
        (void)0;
    } else {
        /* No carriers this symbol: the differential chain is broken. */
        have_reference_ = false;
    }

    /* End of a CIF: decode it whether or not its final symbol happened to
     * carry any of the subchannel. */
    if (msc_enabled_ && symbol_index_ > dab::kFicSymbols && symbol_index_ < dab::kSymbolsPerFrame) {
        const size_t msc_index = symbol_index_ - dab::kFicSymbols - 1;
        if ((msc_index % kMscSymbolsPerCif) + 1 == kMscSymbolsPerCif)
            decode_cif();
    }

    ++symbol_index_;
    symbol_work_cycles_ += cycle_count() - symbol_started;

    if (symbol_index_ >= dab::kSymbolsPerFrame) {
        publish_status(true);

        /* Free-run to the next frame: it is exactly 196608 samples from a
         * 0.5 ppm TCXO. Re-hunt every kFramesBeforeRehunt frames unless r4 is
         * good and a FIB passed recently; r4 alone is not enough, as a grid one
         * symbol out has a perfect constellation and no FIB passing CRC. */
        /* constellation_r4_ is refreshed on FIC frames only, so one low reading
         * stands for several frames: hold until it has stayed low for
         * kRehuntGrace frames (DAB-IMPL 6.105). */
        if (constellation_r4_ < kRehuntKeepR4) {
            if (r4_low_frames_ < 0xFF)
                ++r4_low_frames_;
        } else {
            r4_low_frames_ = 0;
        }
        const bool r4_holds = constellation_r4_ >= kRehuntKeepR4 || r4_low_frames_ <= kRehuntGrace;
        const bool keep_lock = kRehuntKeepR4 != 0 && r4_holds &&
                               frames_since_fib_ < kFramesBeforeRehunt;

        if (frames_since_fib_ < 0xFFFF)
            ++frames_since_fib_;

        /* No FIB for kFibHuntFrames, at any r4: a misnumbered grid, or one a
         * sample loss moved, which the PRS gate takes two sweeps to see (DAB-IMPL
         * 6.117). Only on a hunt that old: frames_since_fib_ survives a re-hunt. */
        const bool grid_never_decoded = kFibHuntFrames != 0 &&
                                        frames_since_fib_ >= kFibHuntFrames &&
                                        frames_since_hunt_ >= kFibHuntFrames;

        /* A gap skip_samples() took that left the window off a symbol. */
        bool grid_lap_dead = false;
        if (lap_watch_ > 0) {
            --lap_watch_;
            if (guard_quality_ < kCorrectorGate && ++lap_guard_bad_ >= kLapGuardBad) {
                grid_lap_dead = true;
                lap_watch_ = 0;
            }
        }

        /* A window not on a symbol: r4 is at the noise floor, so the gate above
         * cannot fire. Healthy runs dip under kGuardHuntMin for single frames. */
        if (kGuardHuntMin != 0 && guard_quality_ < kGuardHuntMin) {
            if (guard_dead_frames_ < 0xFFFF)
                ++guard_dead_frames_;
        } else {
            guard_dead_frames_ = 0;
        }

        const bool grid_guard_dead = kGuardHuntMin != 0 &&
                                     kGuardHuntFrames != 0 &&
                                     guard_dead_frames_ >= kGuardHuntFrames;

        /* A grid on a symbol but the wrong one, which neither guard nor r4
         * can see. */
        const bool grid_prs_dead = prs_low_sweeps_ >= kPrsHuntSweeps;

        if (!grid_never_decoded && !grid_guard_dead && !grid_prs_dead && !grid_lap_dead &&
            (frames_since_hunt_ < kFramesBeforeRehunt || keep_lock)) {
            if (frames_since_hunt_ < kFramesBeforeRehunt)
                ++frames_since_hunt_;
            state_ = State::Sync;

            /* One frame from symbol start to symbol start; the pre-roll replays
             * samples already taken, so add kPreRollSamples back. */
            int32_t wait = static_cast<int32_t>(dab::kNullSamples + kPreRollSamples);

            /* Sync -> Symbols happens only on a power block boundary: whole
             * blocks go into the wait and the remainder is skipped off the
             * pre-roll replay. The wait also gives back the previous frame's
             * skip, which its symbols consumed (DAB-IMPL 6.108). */
            const int32_t phase = static_cast<int32_t>(fine_phase_) + timing_adjust_;
            wait += (phase & ~static_cast<int32_t>(kPowerBlock - 1)) - fine_phase_;
            fine_phase_ = static_cast<uint8_t>(phase & static_cast<int32_t>(kPowerBlock - 1));

            null_remaining_ = static_cast<uint32_t>(wait > 0 ? wait : 0);
            return;
        }

        frames_since_hunt_ = 0;
        /* The new grid's guard has not been judged yet, so the fast re-hunt
         * cannot fire again until the new grid has had kGuardHuntFrames. */
        guard_dead_frames_ = 0;
        prs_low_sweeps_ = 0;
        /* The pin survives the hunt (the carrier offset is the tuner's), saving
         * re-sweep frames just when the new grid needs judging; unless
         * kPrsPinRetries hunts have failed. */
        if (prs_pinned_ && ++prs_pinned_hunts_ >= kPrsPinRetries) {
            prs_pinned_hunts_ = 0;
            prs_pinned_ = false;
            prs_shift_ = -kPrsSweep;
        }
        /* The new grid gets its own one-shot correction. */
        prs_timing_shots_ = 0;
        prs_clash_frames_ = 0;
        restart_acquisition();
    }
}

/* `pair` is the FIC pair whose demap is in symbol_soft_; with a deferred
 * transform it is not the one symbol_index_ implies. */
void DabProcessor::accumulate_fic(size_t pair) {
    /* Only the pairs the rotating codeword needs are transformed, so bits
     * arrive at an offset: pair p covers frame bits [p*3072, p*3072+3072) and
     * codeword r wants [r*2304, r*2304+2304). */
    const size_t pair_first_bit = pair * kSoftBitsPerSymbol;
    const size_t cw_first_bit = static_cast<size_t>(fic_rotate_) * dab::kFicPuncturedBits;
    const size_t cw_end = cw_first_bit + dab::kFicPuncturedBits;

    const size_t from = pair_first_bit > cw_first_bit ? pair_first_bit : cw_first_bit;
    const size_t pair_end = pair_first_bit + kSoftBitsPerSymbol;
    const size_t to = pair_end < cw_end ? pair_end : cw_end;

    if (from >= to)
        return;

    std::memcpy(&fic_soft_[from - cw_first_bit], &symbol_soft_[from - pair_first_bit], to - from);
    fic_fill_ += to - from;

    if (fic_fill_ < dab::kFicPuncturedBits)
        return;

    /* Complete: hand it to the decoder, which drains the queue between DMA
     * buffers rather than stalling the sample path. */
    fic_ready_ = true;
    fic_fill_ = 0;
}

/* `index_in_cif` is the CIF index of the symbol whose demap is in
 * symbol_soft_, up to two symbols behind symbol_index_ with a deferred
 * transform. `erase` writes erasures instead of the soft bits. */
void DabProcessor::accumulate_msc(size_t index_in_cif, bool erase) {
    /* The CIF is the 18 symbols concatenated; the subchannel is a contiguous
     * slice of it, so only the overlap with this symbol needs copying. */
    const size_t symbol_start = index_in_cif * kSoftBitsPerSymbol;
    const size_t symbol_end = symbol_start + kSoftBitsPerSymbol;

    const size_t sub_start = static_cast<size_t>(msc_start_cu_) * 64;
    const size_t sub_end = sub_start + static_cast<size_t>(msc_size_cu_) * 64;

    const size_t from = (symbol_start > sub_start) ? symbol_start : sub_start;
    const size_t to = (symbol_end < sub_end) ? symbol_end : sub_end;

    if (from < to && msc_pending_) {
        msc_pending_ = false;
        note_msc_overrun(msc_pending_cif_);
    }
    /* Reduced to codes on the way in. Both ends are multiples of 64 bits, so
     * the run always starts on a byte and covers whole bytes. Code 0 is level
     * 0, an erasure. */
    if (from < to && (to - sub_start) / 2 <= dab::kMaxCodedBitsPerCif / 2) {
        uint8_t* const dst = &msc_in_[(from - sub_start) / 2];
        if (erase)
            std::memset(dst, 0, (to - from) / 2);
        else
            dab::pack_codes(&symbol_soft_[from - symbol_start], dst, to - from);
    }
}

/* The phase reference holds defer_stage_ from symbol 0 to kPrsForwardSymbol
 * and fft_ to kPrsInverseSymbol; the subchannel uses both from the symbol
 * before its first wanted one to two past its last, so a wanted symbol up to
 * kPrsInverseSymbol clashes (stagecheck.py checks every placement). */
bool DabProcessor::prs_window_clash() const {
    if (!msc_enabled_)
        return false;
    for (size_t i = dab::kFicSymbols + 1; i <= kPrsInverseSymbol; ++i)
        if (msc_symbol_wanted(i))
            return true;
    return false;
}

/* Whether the sample path is about to need the whole core. */
bool DabProcessor::msc_trellis_quiet() const {
    if (!msc_enabled_)
        return false;
    /* The frame head: FIC transforms, null hunt, frequency estimate, timing
     * gate and frame-boundary work make it the busiest moment of the frame. */
    if (symbol_index_ <= dab::kFicSymbols + 1)
        return true;
    /* The phase reference's two transforms, only in frames that staged one
     * (one in kPrsFrameDivider). */
    if (prs_staged_ && (symbol_index_ == kPrsForwardSymbol || symbol_index_ == kPrsInverseSymbol))
        return true;
    /* Likewise the FIC's deferred finish, only when staged: reserving it every
     * frame starved CIF 0, whose span between decode_cif() calls is the short
     * one (cifsched.py). */
    if (fic_staged() && symbol_index_ == fic_finish_symbol_)
        return true;
    /* The symbol, the next two, the one before and the one before that (the
     * deferred finishes land one and two after the last wanted symbol). */
    return msc_window_.test(symbol_index_);
}

void DabProcessor::decode_fic_codeword() {
    fic_ready_ = false;

    if (dab::depuncture_fic_packed(fic_soft_.data(), fic_mother_.data(), fic_mother_.size()) != dab::kFicMotherBits)
        return;

    /* Start the trellis; execute() walks it a few steps at a time. Run to
     * completion here it would stall capture far beyond the 4 ms DMA ring. */
    fic_viterbi_.begin(fic_mother_.data(), dab::kFicMotherBits, fic_decoded_.data(), fic_decoded_.size());
}

/* Called once per DMA buffer with a bounded step budget, and finishes the
 * codeword off when the trellis runs out. */
void DabProcessor::advance_fic_decode() {
    if (!fic_viterbi_.busy())
        return;

    const bool done = fic_viterbi_.advance(kViterbiStepsPerBuffer);

    if (!done)
        return;

    if (fic_viterbi_.payload_bits() != dab::kFicPayloadBits)
        return;

    /* Energy dispersal runs continuously over the whole 768-bit codeword, not
     * restarted per FIB. Restarting it would leave FIBs 1 and 2 at the wrong
     * PRBS phase and only the first of every three would pass CRC. */
    dab::prbs_descramble(fic_decoded_.data(), dab::kFicPayloadBits / 8);

    for (size_t f = 0; f < 3; f++) {
        const uint8_t* fib = &fic_decoded_[f * dab::kFibBytes];

        const bool ok = dab::fib_crc_ok(fib);
        if (ok)
            frames_since_fib_ = 0;
        if (fibs_seen_ < dab::kFibsPerFrame)
            ++fibs_seen_;
        if (!ok && fib_errors_ < 255)
            ++fib_errors_;

        /* A valid FIB always goes up; failed ones are limited to one a frame,
         * enough for the host to sample them. */
        if (!ok) {
            if (raw_fib_sent_this_frame_)
                continue;
            raw_fib_sent_this_frame_ = true;
        }

        const DabFibMessage message{fib, ok};
        shared_memory.application_queue.push(message);
    }
}

void DabProcessor::decode_cif() {
    exec_tags_ |= kExecCodeword;
    if (!msc_enabled_ || !msc_schedule_.valid)
        return;

    const uint32_t cif0 = cycle_count();

    /* Deinterleaves in place; msc_in_ then holds the deinterleaved CIF.
     * Nothing comes out until the 15-CIF delay lines have filled. */
    const bool deinterleaved = deinterleaver_.push_packed(msc_in_.data());
    if (!deinterleaved)
        return;

    /* A CIF still pending here was never reached and msc_in_ has since been
     * deinterleaved over it; accumulate_msc() normally catches this first. */
    if (msc_pending_) {
        msc_pending_ = false;
        note_msc_overrun(msc_pending_cif_);
    }
    /* The running trellis reads mother_ a slice at a time, so a busy one
     * leaves this CIF pending for advance_msc_decode() to depuncture. */
    if (viterbi_.busy()) {
        msc_pending_ = true;
        msc_pending_cif_ = current_cif();
        cif_cycles_ += cycle_count() - cif0;
        return;
    }

    if (!start_msc_codeword())
        return;

    cif_cycles_ += cycle_count() - cif0;
}

uint8_t DabProcessor::current_cif() const {
    const size_t mi = symbol_index_ > dab::kFicSymbols
                          ? (symbol_index_ - dab::kFicSymbols - 1) / kMscSymbolsPerCif
                          : 0;
    return static_cast<uint8_t>(mi & 3);
}

void DabProcessor::note_msc_overrun(uint8_t cif) {
    ++msc_overruns_;
    msc_overrun_cif_ = cif;
}

/* Depuncture the deinterleaved CIF in msc_in_ and start the trellis on it;
 * execute() walks it between DMA buffers. Run to completion here it would
 * lap the 4 ms ring every CIF. */
void DabProcessor::push_held_frame() {
    lf_held_ = false;
    handle_logical_frame(held_.data(), msc_schedule_.payload_bits / 8);
}

bool DabProcessor::start_msc_codeword() {
    if (dab::depuncture_msc_packed(msc_schedule_, msc_in_.data(), mother_.data(), mother_.size()) != msc_schedule_.mother_bits)
        return false;
    return viterbi_.begin(mother_.data(), msc_schedule_.mother_bits, decoded_.data(), decoded_.size());
}

/* A slice of the outstanding CIF, once per DMA buffer. */
void DabProcessor::advance_msc_decode() {
    if (lf_held_ && shared_memory.m4_buffer_missed <= kRsBacklog) {
        push_held_frame();
        return;
    }
    if (!viterbi_.busy()) {
        /* The depuncture is this buffer's whole slice: no steps on top. */
        if (msc_pending_) {
            msc_pending_ = false;
            const uint32_t mv0 = cycle_count();
            start_msc_codeword();
            msc_viterbi_cycles_ += cycle_count() - mv0;
        }
        return;
    }

    const uint32_t mv0 = cycle_count();
    const bool done = viterbi_.advance(msc_steps_);
    msc_viterbi_cycles_ += cycle_count() - mv0;

    if (!done)
        return;

    if (viterbi_.payload_bits() != msc_schedule_.payload_bits)
        return;

    const size_t payload_bytes = viterbi_.payload_bits() / 8;
    dab::prbs_descramble(decoded_.data(), payload_bytes);
    /* In order: a frame still held goes first. */
    if (lf_held_)
        push_held_frame();
    if (msc_is_dab_plus_ && superframe_.parse_due() && shared_memory.m4_buffer_missed > kRsBacklog) {
        std::memcpy(held_.data(), decoded_.data(), payload_bytes);
        lf_held_ = true;
        return;
    }
    handle_logical_frame(decoded_.data(), payload_bytes);
}

void DabProcessor::handle_logical_frame(const uint8_t* data, size_t length) {
    if (msc_frames_ < 0xFF)
        ++msc_frames_;

    if (!msc_is_dab_plus_) {
        /* Classic DAB: the logical frame is one MPEG Layer II frame, 1152
         * samples of 48 kHz audio, with its PAD at the end. */
        decode_audio(data, length);
        emit_pad(data, length);
        return;
    }

    exec_tags_ |= kExecSuperframe;
    if (!superframe_.push_logical_frame(data))
        return;

    if (msc_superframes_ < 0xFF)
        ++msc_superframes_;

    if (!aac_lc_configured_) {
        const auto& h = superframe_.header();
        /* The decoder borrows fft_, dead between symbols and the only place
         * with 7.7 KB to spare. */
        aac_lc_configured_ = aac_lc_.configure(
            h.core_sample_rate(), h.aac_channel_mode ? 2 : 1,
            reinterpret_cast<float*>(fft_.data()),
            (fft_.size() * sizeof(dab::FftComplex)) / sizeof(float));
    }

    for (size_t i = 0; i < superframe_.audio_unit_count(); i++) {
        const auto& unit = superframe_.audio_units()[i];
        if (!unit.crc_ok || unit.length == 0) {
            if (msc_units_bad_ < 0xFF)
                ++msc_units_bad_;
            continue;
        }

        /* Units that passed their CRC, whatever the decoder then makes of
         * them; msc_units_dec_ counts the ones it accepted. */
        if (msc_units_ok_ < 0xFF)
            ++msc_units_ok_;
    }

    /* Decoded one per playout slot by advance_audio_decode(): three back to
     * back would be a burst longer than the DMA ring holds. Units still pending
     * are lost, as try_parse() has just rewritten the buffer they point into. */
    pending_unit_next_ = 0;
    pending_unit_count_ = static_cast<uint8_t>(superframe_.audio_unit_count());
    audio_decode_hold_ = true;

    /* DAB+ PAD is taken per unit from the AAC data_stream_element, in
     * decode_audio(): the trailing bytes here are AAC payload, not PAD. */
}

/* One audio unit per playout slot, once per DMA buffer: a 40 ms unit has 40 ms
 * of slack. Units are referenced in place in the assembler's buffer, valid
 * until the next superframe parses 120 ms later: exactly three slots. */
void DabProcessor::advance_audio_decode() {
    if (pending_unit_next_ >= pending_unit_count_)
        return;

    /* Not in the call that parsed the superframe; one buffer's wait is 4 ms of
     * a 40 ms slot. */
    if (audio_decode_hold_) {
        audio_decode_hold_ = false;
        return;
    }

    /* The ring is the pacer: decode whenever a slot is free. The other one is
     * playing, so the unit is ready a whole frame before the codec needs it. */
    if (audio_ring_.free_slots() == 0)
        return;
    /* ...and with a frame still playing there are 40 ms to choose from, so do
     * not add the decoder's ~2 ms on top of a baseband backlog (audiosim.cpp). */
    if (audio_ring_.queued() > 0 && shared_memory.m4_buffer_missed > kAudioBacklog)
        return;
    if (shared_memory.m4_buffer_missed > kAudioBacklogDry)
        return;

    /* filterbank() borrows fft_ as scratch, safe only while nothing is staged
     * there by the deferred MSC, FIC and PRS transforms. */
    if (staged_fft_ != kStageEmpty)
        return;

    /* Not on the symbols msc_trellis_quiet() reserves for the sample path.
     * Anything new that runs on a symbol has to declare itself there: the
     * symbols without a transform are where the trellis is spread, not idle. */
    if (msc_trellis_quiet())
        return;

    /* Nor while a deferred transform is staged in either buffer.
     * msc_trellis_quiet() already keeps it off; this holds if that window
     * is narrowed. */
    if (staged_fft_ != kStageEmpty || staged_buf_ != kStageEmpty)
        return;
    /* The phase reference holds fft_ from its forward transform to its
     * inverse, and msc_trellis_quiet() reserves only those two symbols. */
    if (prs_staged_ && symbol_index_ >= kPrsForwardSymbol)
        return;

    const auto& unit = superframe_.audio_units()[pending_unit_next_++];
    /* Already counted in msc_units_bad_ when the superframe parsed. */
    if (!unit.crc_ok || unit.length == 0)
        return;

    if (decode_audio(unit.data, unit.length) && msc_units_dec_ < 0xFF)
        ++msc_units_dec_;
}

bool DabProcessor::decode_audio(const uint8_t* frame, size_t length) {
    exec_tags_ |= kExecAudio;
    if (audio_tier_ == 0)
        return false;

    if (!aac_lc_configured_)
        return false;

    /* Straight into the free slot. No expansion here: the interrupt makes the
     * 48 kHz samples as it hands them to the codec. */
    int16_t* const slot = audio_ring_.writable();
    if (slot == nullptr)
        return false;
    bool parsed_r = false;
    const size_t nr = aac_lc_.decode(frame, length, slot, &parsed_r);
    if (!parsed_r) {
        if (aac_lc_fail_ < 0xFF)
            ++aac_lc_fail_;
        return false;
    }
    emit_aac_pad(frame, length);
    if (nr != dab::AudioRing::kCore)
        return true; /* the first unit only primes the overlap */
    audio_ring_.commit();
    if (pcm_queued_ < 0xFF)
        ++pcm_queued_;
    return true;
}

/* DAB+ carries the PAD in the audio unit's data_stream_element, laid out as
 * classic DAB lays it at the end of a frame but not necessarily byte aligned,
 * so the last kPadRegion bytes are realigned before emit_pad() takes them. */
void DabProcessor::emit_aac_pad(const uint8_t* unit, size_t length) {
    constexpr size_t kPadRegion = 128;
    const size_t bytes = aac_lc_.dse_bytes();
    if (bytes < 3)
        return;
    const size_t take = (bytes < kPadRegion) ? bytes : kPadRegion;
    const size_t bit = aac_lc_.dse_bit() + (bytes - take) * 8;
    const unsigned shift = bit & 7;
    const uint8_t* const p = &unit[bit >> 3];
    if ((bit >> 3) + take + (shift ? 1 : 0) > length)
        return;
    uint8_t pad[kPadRegion];
    for (size_t i = 0; i < take; i++)
        pad[i] = shift ? static_cast<uint8_t>((p[i] << shift) | (p[i + 1] >> (8 - shift))) : p[i];
    emit_pad(pad, take);
}

void DabProcessor::emit_pad(const uint8_t* frame, size_t length) {
    /* F-PAD is the frame's last two bytes and X-PAD immediately precedes it;
     * the M0 parses them. 128 bytes covers every X-PAD seen in practice (the
     * maximum is four 48-byte subfields) and bounds the queue traffic. */
    constexpr size_t kPadRegion = 128;

    if (length < 3)
        return;

    /* Frames with no X-PAD carry nothing worth sending, and skipping them
     * keeps this off the message queue most of the time. */
    if (((frame[length - 2] >> 4) & 0x03) == 0)
        return;

    const size_t take = (length < kPadRegion) ? length : kPadRegion;
    const DabPadMessage message{&frame[length - take], static_cast<uint8_t>(take)};
    shared_memory.application_queue.push(message);
}

void DabProcessor::publish_status(bool locked) {
    /* idle_samples_ counts the samples since the last report, so clearing both
     * here keeps the cycle figure and its interval on the same window. */
    frame_cycles_ = work_cycles_;
    symbol_cycles_ = symbol_work_cycles_;
    transform_cycles_ = transform_work_cycles_;
    work_cycles_ = 0;
    symbol_work_cycles_ = 0;
    transform_work_cycles_ = 0;
    idle_samples_ = 0;

    /* MSC counters once a subchannel is selected, per report so each fits a
     * byte; before that the null-to-null interval and the split of the hunt
     * (Searching and Sync, in units of 64 power blocks). */
    uint32_t decoder_bytes;
    /* Filled below, before the `if (reported)` block clears msc_units_dec_
     * and aac_lc_fail_. */
    uint16_t decoder_counts = 0;
    if (msc_enabled_) {
        /* Bit 31 marks counters: the baseband may refuse a selected subchannel,
         * and the diagnostic below must not read as a decode. */
        decoder_bytes = 0x80000000u |
                        static_cast<uint32_t>(msc_frames_) |
                        (static_cast<uint32_t>(msc_superframes_) << 8) |
                        (static_cast<uint32_t>(msc_units_ok_) << 16) |
                        (static_cast<uint32_t>(msc_units_bad_ & 0x7F) << 24);
        /* Why the superframe did not parse, in the spectrum slots: RS failures
         * in the low byte, code words the last superframe recovered (of 16) in
         * the high byte. */
        spectrum_unused_ = static_cast<uint16_t>((superframe_.rs_failures() & 0xFF) |
                                                 (static_cast<uint16_t>(dab::rs_last_good_words()) << 8));
        /* Captured here, before clear_failures() below. */
        hdr_parse_ = static_cast<uint16_t>((superframe_.header_failures() & 0xFF) |
                                           ((superframe_.parse_calls() & 0xFF) << 8));
        /* Low byte rs_zero(), bits 8-13 logical_frame_bytes()/8 (24 for the
         * 192-byte frame of a 48 CU EEP-3-A subchannel), bits 14-15 which
         * guard turned the last call back. */
        asm_shape_ = static_cast<uint16_t>(
            (superframe_.rs_zero() & 0xFF) |
            (((superframe_.logical_frame_bytes() / 8) & 0x3F) << 8) |
            ((dab::rs_zero_why() & 3) << 14));
        /* Firecode failures, and in the high byte the dropped-CIF count in six
         * bits and the index of the last dropped CIF in the top two. A dropped
         * CIF stops any superframe assembling, not just one. */
        spectrum_dc_ = static_cast<uint16_t>(
            superframe_.firecode_failures() |
            (static_cast<uint16_t>((msc_overruns_ > 63 ? 63 : msc_overruns_) |
                                   (msc_overrun_cif_ << 6))
             << 8));
        decoder_counts = static_cast<uint16_t>(msc_units_dec_ | (aac_lc_fail_ << 8));
        /* Transfers the interrupt had to conceal, since the last report. */
        {
            const uint32_t dry = audio_ring_.dry_transfers();
            const uint32_t fresh = dry - audio_ring_dry_last_;
            audio_ring_dry_last_ = dry;
            const uint32_t total = pcm_dry_ + fresh;
            pcm_dry_ = static_cast<uint8_t>(total > 0xFF ? 0xFF : total);
        }
        /* Low byte the frames the codec accepted, high byte the fills it had
         * to be given instead. */
        pcm_report_ = static_cast<uint16_t>(pcm_queued_ | (pcm_dry_ << 8));
        superframe_.clear_failures();
        msc_overruns_ = 0;
        msc_frames_ = 0;
        msc_superframes_ = 0;
        msc_units_ok_ = 0;
        msc_units_bad_ = 0;
        msc_units_dec_ = 0;
        aac_lc_fail_ = 0;
        pcm_queued_ = 0;
        pcm_dry_ = 0;
    } else {
        /* Bit 31 stays clear: it is the counters flag above. */
        const uint32_t search_u = (search_samples_ / 64 / 64) & 0x1F;
        const uint32_t sync_u = (sync_samples_ / 64 / 64) & 0x3F;
        decoder_bytes = (null_interval_ & 0xFFFFF) | (search_u << 26) | (sync_u << 20);
    }

    if (r4_count_ > 0) {
        constellation_r4_ = static_cast<uint16_t>(r4_milli(r4_acc_re_, r4_acc_im_, r4_count_, 0xFFFF));
    }
    r4_acc_re_ = 0.0f;
    r4_acc_im_ = 0.0f;
    r4_count_ = 0;

    if (msc_r4_n_ > 0) {
        msc_r4_milli_ = static_cast<int16_t>(r4_milli(msc_r4_re_, msc_r4_im_, msc_r4_n_, 32767));
    }
    msc_r4_re_ = 0.0f;
    msc_r4_im_ = 0.0f;
    msc_r4_n_ = 0;

    const DabStatusMessage message{
        locked ? DabStatusMessage::Sync::Locked : DabStatusMessage::Sync::Searching,
        locked ? frequency_error_ : static_cast<int16_t>(0),
        locked ? report_fib_errors() : static_cast<uint8_t>(dab::kFibsPerFrame),
        audio_tier_,
        frame_cycles_,
        decoder_bytes,
        symbol_cycles_,
        transform_cycles_,
        /* Bit 7 carries prs_pinned_; decode_variant never exceeds 15. */
        static_cast<uint8_t>(dab::decode_variant | (prs_pinned_ ? 0x80u : 0u)),
        null_dip_offset_,
        null_dip_ratio_,
        constellation_r4_,
        /* const_angle: 0 without a subchannel (the angle is not computed). */
        msc_enabled_ ? msc_r4_milli_ : static_cast<int16_t>(0),
        guard_quality_,
        spectrum_dc_,
        /* While a subchannel runs, decode_cif()'s cost: deinterleaving and
         * depuncturing, without the trellis. */
        msc_enabled_ ? static_cast<uint16_t>(cif_cycles_ / 1000 > 65535 ? 65535 : cif_cycles_ / 1000)
                     : spectrum_used_,
        spectrum_unused_,
        /* The audio path's counters in the r4b slot (the Rx line's "Lost"). */
        pcm_report_,
        /* The MSC trellis, in kcycles per report. */
        static_cast<uint16_t>(msc_viterbi_cycles_ / 1000 > 65535 ? 65535 : msc_viterbi_cycles_ / 1000),
        /* The transform's shift is always 4, so the low byte (S-log fft_shift)
         * carries the deepest DMA backlog this report, in transfers; the high
         * byte fft_boost_. */
        static_cast<uint16_t>(backlog_max_ | (fft_boost_ << 8)),
        /* msc_angles: the longest execute() this report in units of 1,024
         * cycles (12 bits) and what ran in it (kExec* bits, top 4). */
        static_cast<uint16_t>(((exec_worst_ >> 10) > 0xFFF ? 0xFFF : (exec_worst_ >> 10)) |
                              (static_cast<uint32_t>(exec_worst_tags_) << 12)),
        prs_peak_,
        prs_best_shift_,
        /* DabStatusMessage::msc_state: the only field saying which of the two
         * logical-frame consumers runs. */
        static_cast<uint16_t>((msc_enabled_ ? 1u : 0u) |
                              (msc_is_dab_plus_ ? 2u : 0u) |
                              ((audio_tier_ & 3u) << 2) |
                              ((msc_size_cu_ & 0x7Fu) << 4) |
                              (((msc_start_cu_ / 32u) & 0x1Fu) << 11)),
        decoder_counts,
        /* Low byte: boundaries the firecode passed and a header check threw out;
         * high byte: try_parse() entries. Per report. */
        hdr_parse_,
        asm_shape_};
    shared_memory.application_queue.push(message);
    fib_errors_ = 0;
    fibs_seen_ = 0;
    msc_viterbi_cycles_ = 0;
    cif_cycles_ = 0;
    exec_worst_ = 0;
    exec_worst_tags_ = 0;
    backlog_max_ = 0;

    if (msc_enabled_) {
        fic_duty_ = static_cast<uint8_t>((fic_duty_ + 1) % kFicDutyFrames);
    } else {
        fic_duty_ = 0;
    }
    raw_fib_sent_this_frame_ = false;
}

uint8_t DabProcessor::report_fib_errors() const {
    /* FIBs not evaluated this frame count as failures. */
    constexpr uint8_t expected = dab::kFibsPerFrame;
    const uint8_t missing = fibs_seen_ >= expected
                                ? 0
                                : static_cast<uint8_t>(expected - fibs_seen_);
    const uint32_t total = static_cast<uint32_t>(fib_errors_) + missing;
    return static_cast<uint8_t>(total > 255 ? 255 : total);
}

/* The processor is heap allocated, outside the linker's RAM figure. A fast
 * fence: heapcheck.py, run by build.sh on every image, computes the real
 * ceiling from the ELF; 97,344 is it rounded down to 64. Past it, lower
 * dab::kMaxCodedBitsPerCif and with it the highest accepted bit rate. */
static_assert(sizeof(DabProcessor) <= 97344,
              "DabProcessor no longer fits in the M4 bank alongside statics and stacks -- "
              "run tools-local/dab/heapcheck.py for the real figure and the deficit");

int main() {
    shared_memory.set_m4_boot_stage(M4BootStage::main_entered);

    /* The audio DMA is set up per image rather than in baseband.cpp, so a
     * processor that produces audio has to ask for it. */
    audio::dma::init_audio_out();
    shared_memory.set_m4_boot_stage(M4BootStage::audio_ready);

    /* DabProcessor is most of the heap, so recording its allocation separates
     * "out of memory" from anything afterwards. */
    auto processor = std::make_unique<DabProcessor>();
    shared_memory.set_m4_boot_stage(M4BootStage::processor);

    EventDispatcher event_dispatcher{std::move(processor)};

    event_dispatcher.run();
    return 0;
}
