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

/* DAB mode I receiver at the native 2.048 Msps, no decimation. The front end
 * puts the block fs/4 above centre; carrier extraction absorbs it as a bin
 * offset. The FIC is always decoded, the MSC only once a subchannel is chosen.
 * Fixed-point dab::fft2048: at 76 transforms a frame the float FFT cannot keep up. */

#ifndef __PROC_DAB_H__
#define __PROC_DAB_H__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "rssi_thread.hpp"
#include "message.hpp"

#include "dab/dab_constants.hpp"
#include "dab/dab_crc.hpp"
#include "dab/dab_deinterleave.hpp"
#include "dab/dab_fft.hpp"
#include "dab/dab_fic.hpp"
#include "dab/dab_msc.hpp"
#include "dab/dab_aac_lc.hpp"
#include "dab/dab_audio_ring.hpp"
#include "dab/dab_ofdm.hpp"
#include "dab/dab_prbs.hpp"
#include "dab/dab_superframe.hpp"
#include "dab/dab_viterbi.hpp"

#include <array>
#include <cstdint>

class DabProcessor : public BasebandProcessor {
   public:
    DabProcessor();
    ~DabProcessor();

    void execute(const buffer_c8_t& buffer) override;
    void on_message(const Message* const message) override;

   private:
    enum class State : uint8_t {
        Searching,  // hunting for the null symbol
        Sync,       // null found, waiting for the frame to start
        Symbols,
    };

    /* Per-sample work for one DMA buffer. State lives in locals, written back
     * once per buffer: through `this` every field reloads and stores each sample. */
    void execute_fast(const buffer_c8_t& buffer);

    /* Advances the null envelope by one power block; true when it was quiet. */
    bool consume_power_block(uint32_t block);

    /* Samples the DMA ring's lap clamp skipped (m4_transfers_dropped), taken
     * out of the grid as a known gap instead of a re-hunt: the counters move
     * on and every symbol the gap touches is erased (DAB-IMPL 6.118). */
    void skip_samples(size_t n);
    /* The counters and block phase over a gap. */
    void advance_blocks(size_t n);
    uint32_t transfers_dropped_seen_{0};
    bool symbol_damaged_{false};
    /* After a gap the PRS shots are re-armed, and a guard still under
     * kCorrectorGate on kLapGuardBad of the next kLapWatchFrames frames means
     * the gap was not what the clamp reported: re-hunt then. */
    static constexpr uint8_t kLapWatchFrames = 6;
    static constexpr uint8_t kLapGuardBad = 3;
    uint8_t lap_watch_{0};
    uint8_t lap_guard_bad_{0};

    /* Per report: the longest execute() and what ran in it (kExec* bits), and
     * the deepest DMA backlog seen at its entry (at most the ring, 32). */
    static constexpr uint8_t kExecTransform = 1;  /* a data symbol's transform */
    static constexpr uint8_t kExecAudio = 2;      /* an AAC unit */
    static constexpr uint8_t kExecSuperframe = 4; /* a logical frame pushed (RS) */
    static constexpr uint8_t kExecCodeword = 8;   /* a CIF deinterleaved */
    uint8_t exec_tags_{0};
    uint8_t exec_worst_tags_{0};
    uint32_t exec_worst_{0};
    uint16_t backlog_max_{0};

    void on_power_block(bool quiet);
    void on_symbol_complete();
    void transform_symbol();
    /* transform_symbol() split in two, so the MSC can pay the cheap half at
     * the symbol's own boundary and the expensive half on an idle symbol. */
    void transform_stage();
    void transform_finish();
    /* The phase reference correlation, split so the two transforms it needs
     * land on idle symbols instead of on top of symbol 0. */
    void prs_forward();
    void prs_inverse();
    static constexpr int16_t kPrsSweep = 4;
    /* Symbols the two transforms run on: inside msc_trellis_quiet()'s window. */
    static constexpr size_t kPrsForwardSymbol = 8;
    static constexpr size_t kPrsInverseSymbol = 10;
    /* Frames between correlations. The offset being looked for does not move. */
    static constexpr uint8_t kPrsFrameDivider = 4;
    uint8_t prs_frame_{0};
    /* Peak-to-mean, in tenths, below which the correlation is not trusted to
     * steer the frame. The noise floor of a max over 2048 bins is near 11x. */
    static constexpr uint32_t kPrsTrustPeak = 400;
    /* Largest single correction; kTimingTrimLimit bounds the sum. 500 is worse:
     * two shots reach the limit and the third is rejected whole (DAB-IMPL 6.63). */
    static constexpr int32_t kPrsMaxStep = 400;
    int32_t prs_timing_error_{0};
    bool prs_timing_valid_{false};
    /* The measured window error is applied a few times per acquisition rather
     * than as a servo, which saturates at kTimingTrimLimit. Errors reach ~1,100
     * samples, so three shots of kPrsMaxStep (DAB-IMPL 6.57). */
    static constexpr uint8_t kPrsTimingShots = 3;
    uint8_t prs_timing_shots_{0};
    /* A subchannel clashing with the phase reference (prs_window_clash())
     * blocks the correlation. While shots are owed, a correlation frame writes
     * its CIF 0 as erasures (the time interleaver spreads them over 16 logical
     * frames), at most kPrsClashFrames per hunt. */
    static constexpr uint8_t kPrsClashFrames = 2 * kPrsTimingShots;
    uint8_t prs_clash_frames_{0};
    bool msc_cif0_erased_{false};
    bool prs_staged_{false};
    int16_t prs_shift_{-kPrsSweep};
    uint16_t prs_sweep_best_{0};
    int16_t prs_sweep_best_shift_{0};
    uint16_t prs_sweep_best_at_{0};
    /* Re-roll the grid when the correlation peak stays under kPrsHuntMin for
     * kPrsHuntSweeps sweeps: symbol 0 is not the phase reference. Misnumbered
     * grids peak near 100, working ones at 1,800+, and guard and r4 cannot tell
     * them apart (DAB-IMPL 6.54). */
    static constexpr uint16_t kPrsHuntMin = 500;
    static constexpr uint8_t kPrsHuntSweeps = 2;
    uint8_t prs_low_sweeps_{0};
    bool count_prs_low();

    /* Once a sweep wins at shift 0 above kPrsHuntMin, stop sweeping, so the
     * hunt gate gets a verdict every correlation (4 frames), not every sweep (36). */
    bool prs_pinned_{false};

    /* The pin survives a re-hunt (the carrier offset belongs to the tuner, not
     * the grid); after kPrsPinRetries re-hunts without a good correlation the
     * offset is the suspect, so unpin. */
    static constexpr uint8_t kPrsPinRetries = 2;
    uint8_t prs_pinned_hunts_{0};

    /* Published: best peak-to-mean over a complete sweep, and its shift. */
    uint16_t prs_peak_{0};
    int16_t prs_best_shift_{0};
    /* Which step of the subchannel's transform burst a symbol is. */
    enum class BurstStep : uint8_t { kNone, kNow, kStageFft, kStageBuf, kFinishFft, kFinishBuf };
    static constexpr uint8_t kStageEmpty = 0xFF;
    uint8_t staged_fft_{kStageEmpty};
    uint8_t staged_buf_{kStageEmpty};
    /* fft_ holds the FIC's last burst symbol, not the subchannel's. They share
     * the slot because they never overlap (dab::subchannel_placement_ok()). */
    bool staged_is_fic_{false};
    bool fic_staged() const { return staged_is_fic_ && staged_fft_ != kStageEmpty; }
    /* Where the FIC's deferred transform finishes: inside the trellis-quiet
     * window, since the Viterbi runs on the symbols no transform uses. */
    static constexpr size_t kFicFinishSymbol = 6;
    /* ...or one symbol before the subchannel's first transform (its
     * differential reference) if that comes earlier, never inside the frame
     * head. Set with the subchannel; stagecheck.py checks the pair. */
    uint8_t fic_finish_symbol_{kFicFinishSymbol};
    void window_to_stage();
    void stage_to_buf();
    void buf_to_fft();

    /* The third symbol of a burst while fft_ holds the second: three transforms
     * back to back slightly outrun arrival, and staging keeps that inside the
     * 4 ms ring. int8 is all the front end delivers. */
    std::array<int8_t, 2 * dab::kFftSize> defer_stage_{};
    void accumulate_fic(size_t pair);
    void accumulate_msc(size_t index_in_cif, bool erase = false);
    void decode_fic_codeword();
    void advance_fic_decode();

    /* The ring geometry CMake gives both baseband_dma.cpp
     * (BASEBAND_SHARED_DAB_FLAGS) and this header (MODE_FLAGS); the fallbacks
     * match baseband_dma.cpp's. */
#ifndef BASEBAND_BUFFER_LOG2N
#define BASEBAND_BUFFER_LOG2N 13
#endif
#ifndef BASEBAND_TRANSFERS_LOG2N
#define BASEBAND_TRANSFERS_LOG2N 2
#endif
    /* Trellis steps per DMA buffer, as rates per 512 samples. A FIC codeword is
     * 774 steps; eight a buffer finish one in a quarter frame with small spikes. */
    static constexpr size_t kSamplesPerDmaBuffer =
        (size_t{1} << BASEBAND_BUFFER_LOG2N) >> BASEBAND_TRANSFERS_LOG2N;
    static constexpr size_t kViterbiStepsPerBuffer = 8 * kSamplesPerDmaBuffer / 512;
    void decode_cif();

    /* The quiet window keeps the trellis out of 5 symbols in 18, so a CIF's
     * 1,542 steps must fit in ~65 of its ~90 buffers; 28 drops CIFs. */
    static constexpr size_t kMscStepsPerBuffer = 40 * kSamplesPerDmaBuffer / 512;
    /* Rate for codewords above kMscWidePayloadBits: at 72 CU one is 2,310 steps
     * against ~2,190 the quiet window gives at 40, and averages out only just
     * (cifsched.py, slipsim.py). */
    static constexpr size_t kMscStepsWide = 44 * kSamplesPerDmaBuffer / 512;
    static constexpr uint32_t kMscWidePayloadBits = 1536;
    size_t msc_steps_{kMscStepsPerBuffer};

    /* A CIF completing while the trellis is busy waits in msc_in_ until
     * advance_msc_decode() can start it, which must happen before the
     * subchannel's next symbol is copied in (slipsim.py budgets it) or
     * accumulate_msc() counts an overrun. */
    bool msc_pending_{false};
    uint8_t msc_pending_cif_{0};
    bool start_msc_codeword();
    /* A logical frame whose push completes a superframe runs Reed-Solomon over
     * it, about 1M cycles at 72 CU in one call: over kRsBacklog transfers of
     * DMA backlog it is copied to held_ and pushed from a buffer with room,
     * the next codeword running meanwhile (DAB-IMPL 6.118). */
    static constexpr uint16_t kRsBacklog = 6;
    bool lf_held_{false};
    std::array<uint8_t, dab::kViterbiMaxPayloadBits / 8> held_;
    void push_held_frame();
    uint8_t current_cif() const;
    void note_msc_overrun(uint8_t cif);
    void advance_msc_decode();

    /* The FIC has its own trellis (~4.4 KB): the MSC's is busy ~80% of a frame,
     * and a FIC starved of FIBs starves the keep-lock gate too. */
    dab::ViterbiDecoder fic_viterbi_{};
    std::array<dab::ViterbiDecoder::Soft, dab::kFicMotherBits / 4 * dab::ViterbiDecoder::kSoftPerStep> fic_mother_{};
    std::array<uint8_t, dab::kFicPayloadBits / 8> fic_decoded_{};

    void handle_logical_frame(const uint8_t* data, size_t length);
    /* True when the unit reached the audio path. */
    bool decode_audio(const uint8_t* frame, size_t length);
    /* One pending audio unit per playout slot. */
    void advance_audio_decode();
    void emit_pad(const uint8_t* frame, size_t length);
    void emit_aac_pad(const uint8_t* unit, size_t length);
    void publish_status(bool locked);
    bool msc_symbol_wanted(size_t index) const { return msc_wanted_.test(index); }
    /* The subchannel's symbols and the trellis's keep-out window (dab_msc.hpp). */
    dab::FrameSymbolMask msc_wanted_{};
    dab::FrameSymbolMask msc_window_{};
    bool prs_window_clash() const;
    bool msc_trellis_quiet() const;
    void measure_spectrum(uint16_t& dc_out, uint16_t& used_out, uint16_t& unused_out);
    uint8_t report_fib_errors() const;
    void restart_acquisition();
    static void enable_cycle_counter();
    static uint32_t cycle_count();

    static constexpr size_t baseband_fs = dab::kSampleRate;

    /* The null is 2656 samples of near silence. A 196608-sample frame will not
     * fit in RAM, so the detector looks for a sustained dip in a decimated
     * power envelope. */
    static constexpr size_t kPowerBlock = 64;
    static constexpr size_t kNullBlocks = dab::kNullSamples / kPowerBlock;

    static constexpr size_t kMscSymbolsPerCif = dab::kMscSymbols / dab::kCifsPerFrame;
    static constexpr size_t kSoftBitsPerSymbol = 2 * dab::kCarriers;

    State state_{State::Searching};

    /* idle_samples_ paces status while unsynchronised. */
    static constexpr size_t kSamplesPerFrame = (dab::kSampleRate * dab::kFrameDurationMs) / 1000;
    size_t idle_samples_{0};

    uint32_t block_power_{0};
    size_t block_fill_{0};
    uint32_t reference_power_{1};

    /* reference = reference - (reference >> 12) + (window >> 12) in a scaled
     * accumulator so neither shift truncates, else a window under 4,096 adds
     * nothing and the quiet test depends on input level. uint64: the scaled
     * sum (~2.6e9) is too near uint32's limit. */
    uint64_t reference_acc_{1u << 12};

    /* One 64-sample block of OFDM is near Rayleigh and says little, so the
     * detector sums a window of half the null: it then sits wholly inside the
     * null for ~1,400 samples, where a full-length one may never fit at all. */
    static constexpr size_t kNullWindowBlocks = kNullBlocks / 2;

    std::array<uint32_t, kNullWindowBlocks> block_history_{};
    size_t history_write_{0};
    uint32_t window_power_{0};
    bool window_filled_{false};

    /* The deepest envelope dip this frame and its offset: the null is the only
     * 20 dB dip, so this shows where the frame really starts. */
    uint32_t frame_dip_window_{0xFFFFFFFF};
    uint32_t frame_dip_reference_{1};
    uint32_t frame_dip_offset_{0};
    uint32_t null_dip_offset_{0};
    uint16_t null_dip_ratio_{0};

    /* Constellation tightness of the last FIC symbol pair. */
    float r4_acc_re_{0.0f};
    float r4_acc_im_{0.0f};
    uint32_t r4_count_{0};

    uint16_t constellation_r4_{0};
    /* The same over the subchannel's first symbol pair in each CIF. */
    float msc_r4_re_{0.0f};
    float msc_r4_im_{0.0f};
    uint32_t msc_r4_n_{0};
    int16_t msc_r4_milli_{0};

    uint16_t guard_quality_{0};

    /* Which symbol the FIC starts on (dab::fic_offset). */
    uint8_t fic_start_ = 1;

    /* Doublings the corrector applies to each data symbol, set per frame from
     * symbol 0's power: corrector and transform truncate in fixed LSBs, so a
     * weak window lost its SNR to them (DAB-IMPL 6.115). 0 above a mean power
     * of kFftBoostPower / 4, where the receiver runs bit for bit as before. */
    static constexpr uint32_t kFftBoostPower = 1024; /* mean i^2 + q^2 */
    static constexpr uint8_t kFftBoostMax = 4;
    uint8_t fft_boost_{0};
    void set_fft_boost();
    uint16_t spectrum_dc_{0};
    uint16_t spectrum_used_{0};
    uint16_t spectrum_unused_{0};

    /* How far inside the guard the transform window sits, in samples. Decoding
     * holds for window errors of about -600..+100 and falls off a cliff late,
     * so aim early; the gate settles the boundary at +32..+192. */
    static constexpr size_t kFftWindowBias = 192;
    static_assert(kFftWindowBias <= dab::kGuardSamples,
                  "the transform window would start before the symbol");

    /* Added to guard_energy()'s base, moving the early-late gate's fixed point
     * earlier by about as much: the gate stops at the late edge of a 252-sample
     * plateau, not the peak. The far tap is 64 + kGateBias + 64 + 252 + kFftSize,
     * so 123, the measured best (DAB-IMPL 6.29), is the largest that stays
     * inside symbol_. */
    static constexpr int32_t kGateBias = 123;
    static_assert(kGateBias >= 0 && 64 + kGateBias + 64 + 252 + static_cast<int32_t>(dab::kFftSize) <
                                        static_cast<int32_t>(dab::kSymbolSamples),
                  "the early-late gate would read past the end of symbol_");

    /* Constellation tightness above which the timing gate stops trimming. */
    static constexpr uint16_t kTimingFreezeR4 = 350;
    /* Samples the grid is nudged by at the end of the next frame, and the
     * total trim since the null last placed the frame. */
    int32_t timing_adjust_{0};
    int32_t timing_total_{0};
    static constexpr int32_t kTimingTrimLimit = 1000;  /* under half a symbol, so it cannot reach the neighbour */

    /* The frequency estimate is gated on the guard correlation that produced
     * it and then smoothed: its median is right and its tail is not. */
    static constexpr uint16_t kCorrectorGate = 300;
    static constexpr float kCorrectorIir = 32.0f;
    float corrector_offset_{0.0f};
    bool corrector_primed_{false};

    /* Guard correlation summed over a frame's data symbols, as the reference
     * receivers estimate the offset. */
    int32_t freq_acc_re_{0};
    int32_t freq_acc_im_{0};
    uint16_t freq_acc_n_{0};
    static constexpr uint16_t kFreqAccMinSymbols = 32;

    /* The DMA slot is copied here first: execute_fast() holds its buffer across
     * on_symbol_complete(), up to 4.7 ms of transforms, and the ring wraps every
     * 4 ms. */
    static constexpr size_t kStageSamples = kSamplesPerDmaBuffer;
    alignas(4) std::array<complex8_t, kStageSamples> stage_{};

    /* Bits to drop from the input samples: 0 at the native 8-bit scale. */
    static constexpr int kInputShift = 0;

    size_t symbol_fill_{0};
    size_t symbol_index_{0};

    /* The null ends inside a power block, so the transition is always a little
     * late. Late timing eats the next symbol and cannot be undone; early only
     * reaches into the guard, a phase slope the differential cancels. */
    static constexpr size_t kPreRollSamples = 2 * kPowerBlock;
    std::array<dab::Iq16, kPreRollSamples> preroll_{};
    size_t preroll_write_{0};

    std::array<dab::Iq16, dab::kSymbolSamples> symbol_{};

    /* Transform working array, natural order in and out. */
    std::array<dab::FftComplex, dab::kFftSize> fft_{};

    std::array<dab::Iq16, dab::kCarriers> carriers_a_{};
    std::array<dab::Iq16, dab::kCarriers> carriers_b_{};
    bool use_a_{true};
    bool have_reference_{false};

    /* Soft bits of the symbol just demapped, before the FIC/subchannel split.
     * In bank 2; see msc_in_. */
    static std::array<int8_t, kSoftBitsPerSymbol> symbol_soft_;

    /* FIC codewords are decoded as they complete rather than buffering all
     * four (9 KB). In bank 2, leaving bank 1 for Reed-Solomon's tables; +4
     * because the packed depuncturer fetches groups as words. */
    static std::array<int8_t, dab::kFicPuncturedBits + 4> fic_soft_;
    size_t fic_fill_{0};

    /* A completed FIC codeword awaiting decode between DMA buffers. No queue:
     * the depuncturer copies it out first and fic_soft_ refills next frame. */
    bool fic_ready_{false};
    uint8_t fic_rotate_{0};

    /* Subchannel soft bits of the CIF being assembled, deinterleaved in place,
     * as packed codes (dab_soft.hpp); +4: the depuncturer fetches groups as
     * words. In .ram2, off bank 1 which the DMA ring competes for; static
     * because a section attribute cannot apply to a member. .ram2 is NOLOAD,
     * so the constructor fills these. */
    static std::array<uint8_t, dab::kMaxCodedBitsPerCif / 2 + 4> msc_in_;

    /* Deinterleaver history. Not in the AHB bank the linker calls the USB
     * buffer: that region overlaps the M0's RAM. */
    std::array<uint8_t, dab::TimeDeinterleaver::storage_bytes_for(dab::kMaxCodedBitsPerCif)> deinterleaver_storage_{};

    static constexpr size_t kMaxMotherBits =
        (dab::kFicMotherBits > 4 * (dab::kViterbiMaxPayloadBits + 6))
            ? dab::kFicMotherBits
            : 4 * (dab::kViterbiMaxPayloadBits + 6);
    std::array<dab::ViterbiDecoder::Soft, kMaxMotherBits / 4 * dab::ViterbiDecoder::kSoftPerStep> mother_{};

    /* Trellis output, in bank 2 too. */
    static std::array<uint8_t, dab::kViterbiMaxPayloadBits / 8> decoded_;

    dab::FrequencyCorrector corrector_{};
    dab::ViterbiDecoder viterbi_{};
    dab::TimeDeinterleaver deinterleaver_{};
    /* Produces the 24 kHz core, played at 48 kHz with each sample twice:
     * nothing above 12 kHz until SBR exists. */
    dab::AacLcDecoder aac_lc_{};
    bool aac_lc_configured_{false};
    std::array<int16_t, 2 * dab::kAacFrameLength> aac_pcm_{};
    /* aac_pcm_ as two 24 kHz core slots, drained by the audio DMA interrupt. */
    dab::AudioRing audio_ring_{};
    uint32_t audio_ring_dry_last_{0};
    static_assert(dab::AudioRing::kCore == dab::kAacFrameLength &&
                      dab::AudioRing::kStorage == 2 * dab::kAacFrameLength,
                  "the ring lives in aac_pcm_");
    dab::SuperframeAssembler superframe_{};

    /* From DabConfigureMessage; size_cu 0 means the FIC only, no audio. */
    bool msc_enabled_{false};
    bool msc_is_dab_plus_{false};

    /* MSC health per report, then cleared, so each fits a byte. No logical
     * frames: Viterbi or deinterleaver; frames but no superframes: sync or RS;
     * failing units: wrong superframe boundary. */
    uint8_t msc_frames_{0};
    uint8_t msc_superframes_{0};
    /* Indices into superframe_.audio_units(), one per playout slot, valid
     * until the next superframe parses 120 ms later. */
    uint8_t pending_unit_count_{0};
    uint8_t pending_unit_next_{0};
    /* Set when a superframe parses, so the first unit is not decoded in the
     * same execute() call that ran Reed-Solomon over it. */
    bool audio_decode_hold_{false};
    uint8_t msc_units_ok_{0};
    uint8_t msc_units_bad_{0};
    uint16_t msc_start_cu_{0};
    uint16_t msc_size_cu_{0};
    dab::MscSchedule msc_schedule_{};

    /* Frames committed to the ring per report. Unit counters cannot stand in:
     * a unit that only primes the overlap yields no PCM. */
    uint8_t pcm_queued_{0};
    /* Published in the r4b slot: the Rx line's "Lost" figure. */
    uint16_t pcm_report_{0};

    /* Transfers the interrupt concealed per report, because the ring was dry. */
    uint8_t pcm_dry_{0};

    /* DWT cycle counts, so the budget is measured on the device. */
    uint32_t work_cycles_{0};
    uint32_t frame_cycles_{0};

    /* Split of work_cycles_: everything on_symbol_complete costs, so the
     * per-sample loop can be priced as the difference. */
    uint32_t symbol_work_cycles_{0};
    uint32_t symbol_cycles_{0};
    uint32_t transform_work_cycles_{0};
    uint32_t transform_cycles_{0};

    uint8_t fib_errors_{0};

    /* FIBs put to the CRC this frame; one not evaluated counts as failed. */
    uint8_t fibs_seen_{0};

    /* Samples since the last null, and the previous interval; a frame is
     * exactly 196608. */
    uint32_t since_null_{0};
    uint32_t null_interval_{0};
    uint32_t hunting_{0};

    /* The hunt split in two: Searching waits for a quiet window, Sync for
     * the signal to return. */
    uint32_t search_samples_{0};
    uint32_t sync_samples_{0};

    /* Null samples still to run when the window went quiet. Counted down:
     * waiting for the window to clear lags by its own length. */
    uint32_t null_remaining_{0};

    /* Frames free-run since the last search. A periodic re-hunt bounds how long
     * a wrong lock persists without a search every frame, and needs nothing to
     * have decoded, which a wrong timing never does. Long enough for the
     * timing trim to converge and stay converged. */
    static constexpr uint16_t kFramesBeforeRehunt = 256;
    uint16_t frames_since_hunt_{0};

    /* Frames since a FIB last passed CRC: tells a real lock from one that is
     * merely tight, for the keep-lock test. */
    uint16_t frames_since_fib_{0};

    /* Frames between FIC decodes while a subchannel runs. Higher saves cycles
     * but fills the service list slower (FIG 0/1). Not 0: r4 is measured only
     * on FIC symbols and frames_since_fib_ would never reset. */
    static constexpr uint8_t kFicDutyFrames = 4;
    uint8_t fic_duty_{0};

    uint32_t msc_viterbi_cycles_{0};
    uint32_t cif_cycles_{0};

    /* CIFs dropped because the previous one had not finished decoding. */
    uint16_t msc_overruns_{0};
    /* Which CIF of the frame the last drop was, 0..3. */
    uint8_t msc_overrun_cif_{0};

    /* Keep a lock the constellation says is real at the periodic re-hunt,
     * which can turn a good lock bad. Noise reads r4 15-25, a working receiver
     * 600+. */
    static constexpr uint16_t kRehuntKeepR4 = 250;
    /* Frames r4 must stay under kRehuntKeepR4 before an expired grid is
     * re-hunted. */
    static constexpr uint8_t kRehuntGrace = 12;
    /* Samples a frame drifts per Hz of carrier offset (kSamplesPerFrame /
     * carrier), 0 until configured; and the fraction of a sample owed. */
    float drift_per_hz_{0.0f};
    float drift_owed_{0.0f};
    /* The accumulated steer below a power block: pre-roll samples the next
     * symbol phase skips. */
    uint8_t fine_phase_{0};
    uint8_t r4_low_frames_{0};

    /* Re-roll the grid after this many frames with no FIB, on a hunt at least
     * as old: a working grid decodes within a frame or two. */
    static constexpr uint16_t kFibHuntFrames = 48;

    /* Re-roll the grid when the guard correlation stays under kGuardHuntMin
     * for kGuardHuntFrames frames: the window is not on a symbol, which needs
     * no good r4 to see. Working runs dip for single frames, hence the run
     * length. Overrides keep-lock. Off (kGuardHuntMin 0): 150 is the value to
     * try, never tried on air. */
    static constexpr uint16_t kGuardHuntMin = 0;
    static constexpr uint16_t kGuardHuntFrames = 32;
    /* Frames in a row under kGuardHuntMin; reset by the re-hunt itself. */
    uint16_t guard_dead_frames_{0};

    /* A failed FIB already forwarded this frame: a rate limit, not a cap, so
     * samples keep coming. */
    bool raw_fib_sent_this_frame_{false};

    uint8_t msc_units_dec_{0};
    /* Published in decoder_counts. */
    uint8_t aac_lc_fail_{0};

    /* Captured from the assembler before clear_failures() runs, like
     * spectrum_dc_; read at message time it would always be 0. Low byte
     * header_fail_, high byte parse_calls_. */
    uint16_t hdr_parse_{0};
    uint16_t asm_shape_{0};
    uint8_t audio_tier_{0};
    int16_t frequency_error_{0};

    /* Threads must be the last members, started from the constructor body:
     * started from their own constructors they run before EventDispatcher::run()
     * sets shared_memory.baseband_ready ("Baseband Sync Fail" on the M0). */
    BasebandThread baseband_thread{baseband_fs, this, baseband::Direction::Receive,
                                   /*auto_start*/ false};
    RSSIThread rssi_thread{/*auto_start*/ false};
};

#endif /*__PROC_DAB_H__*/
