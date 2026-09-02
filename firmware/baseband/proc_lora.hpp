/*
 * Copyright (C) 2026 Alexey Verhogladov
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
 */

#ifndef __PROC_LORA_H__
#define __PROC_LORA_H__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "rssi_thread.hpp"
#include "dsp_decimate.hpp"
#include "message.hpp"

#include <cstdint>
#include <array>
#include <complex>

// LoRa CSS (Chirp Spread Spectrum) demodulator for HackRF/PortaPack.
// Implements the algorithm described by Robyns et al. (2016).
// Full RX chain: dechirp+FFT -> deinterleave -> dewhiten -> Hamming decode.
// Half-duplex only - HackRF cannot RX and TX simultaneously.

class LoRaProcessor : public BasebandProcessor {
   public:
    void execute(const buffer_c8_t& buffer) override;
    void on_message(const Message* const message) override;

   private:
    // LoRa parameters (set via LoRaConfigureMessage)
    uint8_t spreading_factor{7};  // SF7-SF12
    uint32_t bandwidth{125000};   // 125/250/500 kHz
    uint8_t coding_rate{5};       // denominator: 5=CR4/5, 8=CR4/8 (TX, and the RX default)
    // The receiver's block length, taken from the CR field of the header actually
    // received. The preset table said 4/8 for LONG_MODERATE while the air carried 4/5,
    // and the payload blocks were cut three symbols too late from the second block on:
    // the first block survived (a nibble depends only on a block's first four symbols,
    // which are the same either way) and everything after it was noise. Reading the
    // header's own answer costs nothing and cannot disagree with the transmitter.
    uint8_t rx_cw_len_{5};

    // Derived parameters
    uint32_t chips_per_symbol{0};
    uint32_t samples_per_symbol{0};
    bool rx_active{false};

    // Low Data Rate Optimization: mandatory for SF>=11 in Meshtastic.
    // When true, interleaver uses (SF-2) codes per block, not SF.
    bool use_ldro_{false};

    // --- RX state machine ----------------------------------------------------
    // HUNT    -> count >=8 near-zero upchirps (preamble)
    // PRE_END -> wait for first non-zero symbol (sync word) or downchirp (SFD)
    // SKIP    -> skip sync + SFD + header symbols
    // PAYLOAD -> accumulate symbols into interleaver blocks, decode, send
    enum class RxState : uint8_t { HUNT,
                                   PRE_END,
                                   SKIP,
                                   HEADER,
                                   PAYLOAD };
    RxState rx_state_{RxState::HUNT};

    // Explicit-header decode (reduced-rate block of 8 symbols -> packet length).
    uint16_t hdr_sym_[8]{};
    uint8_t hdr_count_{0};
    uint8_t payload_len_target_{0};  // bytes to collect (from header), 0 = unknown
    bool header_valid_{false};       // last decode_header() checksum result (debug/gate)
    // The LoRa header block carries SF-2 nibbles = 5 header + (SF-7) PAYLOAD nibbles.
    // At SF7 there are none (SF-2 == 5); at SF11 the first 4 payload nibbles (2 bytes)
    // live in the header block and MUST be fed to the payload or the whole payload
    // mis-aligns.  decode_header() stashes nib[5..SF-2) here; the PAYLOAD transition
    // feeds them before the first payload block.
    uint8_t hdr_extra_nib_[8]{};
    uint8_t hdr_extra_cnt_{0};

    uint8_t preamble_run_{0};  // consecutive near-zero upchirps seen
    uint8_t skip_remain_{0};   // symbols left to skip before payload
    uint8_t new_pre_run_{0};   // near-zero run in PAYLOAD (new preamble detector)
    int stored_realign_{0};    // fine-align realign computed at sync-detection (moved out of SKIP)

    // --- Interleaver block accumulator ---------------------------------------
    // One block = coding_rate symbols of cpb bits -> cpb codewords of coding_rate bits
    // Max coding_rate = 8, max cpb (SF-2 for LDRO) = 10 (SF12-2).
    std::array<uint16_t, 8> sym_block_{};
    uint8_t sym_in_block_{0};
    uint16_t payload_sym_count_{0};

    // --- Nibble assembly (2 x 4-bit nibbles -> 1 byte) ------------------------
    bool nibble_lo_valid_{false};
    uint8_t nibble_lo_{0};

    // --- Whitening LFSR -------------------------------------------------------
    // Polynomial x^8+x^6+x^5+x^4+1, initial state 0xFF.
    // Sequence: 0xFF, 0xFE, 0xFC, 0xF8, 0xF0, 0xE1, 0xC2, 0x85, 0x0B, ...
    uint8_t whiten_state_{0xFF};

    // --- Decoded payload -----------------------------------------------------
    static constexpr size_t MAX_PAYLOAD{255};
    std::array<uint8_t, MAX_PAYLOAD> payload_buf{};  // decoded bytes
    uint8_t decoded_len_{0};

    // --- End-of-packet detection via FFT peak magnitude -----------------------
    float last_peak_mag_{0.0f};
    float ref_peak_mag_{0.0f};
    float ref_sharp_acc_{0.0f};  // sum of per-symbol peak/total power, for the SNR estimate
    uint32_t ref_sharp_n_{0};
    uint8_t weak_sym_count_{0};
    size_t last_peak_bin_{0};  // FFT peak bin from last dechirp_symbol() call

    // --- Symbol timing recovery -----------------------------------------------
    // HUNT scans 8 phase offsets (0, N/8, 2N/8, ...) to cover all timing offsets.
    // First near-zero upchirp locks timing_corr_ = peak_bin so PAYLOAD can
    // correct decoded chips: chip = (raw_bin - timing_corr_ + N) % N.
    uint32_t phase_offset_{0};  // chip-domain phase applied in execute()
    uint32_t timing_corr_{0};   // chip offset correction for payload decode

    // --- SFD-based CFO / timing sync (matches the harness-validated method) ----
    // Preamble up-dechirp peaks at 1+cfo+tau, SFD down-dechirp at 1+cfo-tau, so the
    // CFO (bin) = (up+down)/2 and tau = (up-down)/2*os.  This SEPARATES CFO (a bin
    // shift -> timing_corr_) from timing (tau -> sample realign), where the old
    // bin-snap conflated them and corrupted the no-FEC explicit header.
    float last_peak_frac_{0.0f};  // parabolic sub-bin of the last up-dechirp
    uint32_t up_bin_sfd_{0};      // preamble up-peak bin (reference)
    float up_frac_sfd_{0.0f};
    double up_acc_{0.0};  // averaged preamble up-position (noise reduction)
    int up_cnt_{0};
    uint32_t preamble_idx_{0};  // ring index of the most recent preamble window
    uint32_t down_bin_sfd_{0};  // best (strongest) SFD down-peak bin
    float down_frac_sfd_{0.0f};
    bool have_down_{false};
    float best_down_mag_{0.0f};
    uint32_t down_peak_bin_{0};  // last dechirp_down() result
    float down_peak_mag_{0.0f}, down_peak_frac_{0.0f};
    uint32_t sharp_peak_bin_{0};  // argmax bin from the last sharpness_*()
    uint32_t pre_buf_base_{0};    // ring index that pre_buf_[0] corresponds to

    // --- Preamble detection threshold ----------------------------------------
    // Detect preamble after this many consecutive near-zero upchirps.
    // Meshtastic sends 16; we trigger at 8 to allow early detection.
    static constexpr uint8_t PREAMBLE_DETECT{4};  // aligned-preamble confirmations after the timing snap
    // Trigger new-preamble-in-payload send after this many near-zero symbols.
    static constexpr uint8_t NEW_PRE_DETECT{8};
    // Hard limit: send after this many payload symbols regardless.
    static constexpr uint16_t PAYLOAD_SYM_LIMIT{220};

    // --- Sample decimation ---------------------------------------------------
    // 2 MHz baseband, /2 -> 1 MHz (integer oversampling per chip: OS=2 for BW500).
    // Running at 2 MHz (not 4) DOUBLES the per-buffer M4 deadline and makes the
    // decimator a cheap CIC3 - giving the M4 real-time margin so the fine-align
    // FFT burst no longer drops samples (the chronic live-RX failure).
    static constexpr size_t baseband_fs{2000000};
    static constexpr uint32_t DECIM{2};
    // Decimator output <= input(2048)/2 = 1024 samples per call.
    std::array<complex16_t, 1024> decim_buf{};
    const buffer_c16_t decim_buffer{decim_buf.data(), decim_buf.size()};
    // Translate by -fs/4 (cancels receiver -fs/4 tuning offset) + decimate by 2 -> 1 MHz.
    dsp::decimate::TranslateByFSOver4AndDecimateBy2CIC3 decim_0{};
    // LONG_FAST (SF11/BW250): /8 -> 250 kHz (OS=1, sps=2048).  Separate STREAMING path
    // (a whole SF11 packet won't fit the ring); selected by SF/BW in configure().
    dsp::decimate::FIRC8xR16x24FS4Decim8 decim8_{};
    // BW125 needs 125 kHz, so a second /2 runs behind the /8. Its output lands in the
    // upper half of the same scratch buffer - the /8 stage only ever fills 256 of the
    // 1024 slots.
    dsp::decimate::FIRC16xR16x16Decim2 decim2_{};
    const buffer_c16_t decim_buffer2{decim_buf.data() + 512, 512};
    bool bw125_{false};               // second decimation stage in use
    uint32_t sf11_out_per_buf_{256};  // decimated samples one baseband buffer yields

    // Max supported samples/symbol (sps) at 1 MHz.  256 = SHORT_TURBO SF7/BW500 (OS=2).
    // Kept at 256 (not 512) to free RAM for a packet-sized ring; drops BW250/BW125
    // support (not used by SHORT_TURBO) - revisit if wider BW is needed.
    static constexpr size_t MAX_SPS{256};

    // --- FFT buffer (radix-2, sps pts - our own fft, not dsp_fft.hpp) ----------
    std::array<std::complex<float>, MAX_SPS> fft_buf{};

    // Linear snapshot of the preamble (3 symbols: [idx-sps, idx+2*sps)) so the SFD
    // fine-align searches contiguous samples in either tau direction - the ring read
    // was racing the writer, corrupting timing_corr live.
    std::array<complex8_t, 3 * MAX_SPS> pre_buf_{};

    // Precomputed conjugate reference chirp (one symbol, sps samples). Filled in
    // configure(); the fold demod multiplies by it (no per-sample trig on M4).
    uint32_t os_{2};  // oversampling = sps/chips
    std::array<std::complex<float>, MAX_SPS> ref_chirp_{};

    // Decimated-sample ring buffer, stored as complex8 (int8 I/Q - validated to
    // decode byte-exact at the real signal level; halves RAM so a whole packet fits).
    // STORE-AND-DECODE: sized to hold an entire packet (~=64 symbols) so that, once
    // sync is found, execute() can just COPY samples during the packet (no per-symbol
    // FFT -> always under the M4 deadline even while M0 hogs the bus) and defer the
    // FFT decode to AFTER the packet - decoupling decode from real time.  This is the
    // fix for the chronic live-RX header-window sample drops (M0/M4 bus contention).
    // Sized to hold a whole real Meshtastic packet (text ~= 77, NodeInfo ~= 100 symbols)
    // so the offline decode never truncates.  NON-power-of-2 (fits the M4 heap with a
    // safe margin where 32768 would not); wrap via compare-subtract (real-time store)
    // or % (post-packet reads), never a mask.
    // 108 symbols @ sps=256 = 54 KiB. The decode runs after the packet ends,
    // so the ring must hold the whole packet: TELEMETRY ~98 sym, POSITION
    // ~104 sym. 100 dropped both (too tight); 112 OOM'd the M4 heap; 108 fits
    // with a small margin. (Long text / NodeInfo ~110-123 sym still overrun.)
    static constexpr size_t RING{108 * 256};

    // --- LONG_FAST (SF11/BW250) streaming buffers - UNION with the SF7 ring ------
    // SF7 (store-and-decode) uses ring_; SF11 (streaming) uses sf11_.  They are never
    // active together (SF is fixed per session), so they share the same 54 KiB - RAM
    // stays exactly as before (no OOM).  The float member makes the union 4-aligned,
    // so both views are safe.  See the proc_lora.cpp SF11 section for the algorithm.
    static constexpr size_t SF11_SPS{2048};  // MAX sps = buffer/layout size (ref/fft/win alloc)
    uint32_t sf11_sps_{2048};                // RUNTIME samples/symbol = 2^SF (<=2048): 128/256/512/1024/2048
    uint32_t sf11_stages_{11};               // log2(sps) FFT butterfly stages (8/9/10/11)
    uint32_t sf11_nsteps_{8};                // demod steps = sps/256 = buffers/symbol (1/2/4/8)
    // Rolling sample window.  5 symbols, not 3: FINE consumes exactly one demod per symbol
    // and the budget delivers exactly one, so it has no real-time margin - whenever a job
    // can't start the instant its samples land it slips a buffer, and over FINE's 14 symbols
    // that slip accumulated past a 3-symbol window and tripped sf11_win_stale(), silently
    // killing most packets. 5 symbols buys ~4 symbols of lag. Fits the ring: ref 16K + fft
    // 16K + win 20K = 52 KiB of the 54 KiB.
    static constexpr size_t SF11_WIN{11 * SF11_SPS / 2};  // 5.5-symbol rolling window (uses the ring's spare 2 KiB -> more FINE staleness margin after the RESOLVE latency)
    // The SF7 ring and the SF11 streaming buffers share this 54 KiB (never active
    // together - SF is fixed per session), so RAM is unchanged (no OOM).  Done via
    // reinterpret views (not a union: std::complex<float>'s non-trivial ctor deletes a
    // union's default ctor).  alignas(8) makes the complex<float> view properly aligned.
    // Layout: [ref11 16 KiB][fft11 16 KiB][win11 12 KiB] within the 54 KiB (spare tail).
    alignas(8) std::array<complex8_t, RING> ring_{};
    std::complex<float>* sf11_ref() { return reinterpret_cast<std::complex<float>*>(ring_.data()); }
    // At SF12 a symbol is 4096 samples and the stored reference no longer fits, so it
    // is generated as the dechirp runs and the FFT takes its place at the start of the
    // ring. The window keeps both its offset and its length either way - the reference
    // frees exactly as much as the doubled FFT wants - so none of the wrap arithmetic
    // below changes, which is the only reason this is a small change instead of a big one.
    std::complex<float>* sf11_fft() { return reinterpret_cast<std::complex<float>*>(
        ring_.data() + (sf11_gen_ref_ ? 0u : SF11_SPS * 4)); }
    complex8_t* sf11_win() { return ring_.data() + SF11_SPS * 8; }
    uint32_t write_idx_{0};     // monotonic write count (for spans)
    uint32_t read_idx_{0};      // next symbol start to process
    bool snapped_{false};       // timing snap done this packet
    uint32_t skip_samples_{0};  // one-shot sample skip (snap / SFD realign)

    // --- Store-and-decode phase (energy-triggered, ZERO real-time FFT) ---------
    // ACQUIRE: execute() only decimates, copies to the ring, and sums energy - all
    //          cheap (~100 us, 10x under the deadline), so M0 bus contention can no
    //          longer starve it.  Packet detected purely by ENERGY (a run of buffers
    //          >= ~4x the noise floor) - no FFT.
    // DECODE : packet ended -> freeze storing, run the ENTIRE pipeline (HUNT scan +
    //          sync + fine-align + SKIP/HEADER/PAYLOAD) offline over the buffered ring,
    //          chunked across execute() calls, then back to ACQUIRE.
    enum class RxPhase : uint8_t { ACQUIRE,
                                   DECODE };
    RxPhase phase_{RxPhase::ACQUIRE};
    float noise_ema_{0.0f};  // running noise-floor energy estimate (per decim batch)
    bool in_packet_{false};  // energy currently above the packet threshold
    uint32_t low_run_{0};    // consecutive below-threshold batches inside a packet
    uint32_t hi_run_{0};     // high-energy batches so far in this packet (min-run gate)
    uint32_t pkt_start_{0};  // ring index where the current packet began
    // A real SF7/BW500 packet spans ~15 decim batches; require this many high-energy
    // batches before decoding so brief noise spikes don't cause false DECODE bursts.
    static constexpr uint32_t MIN_PKT_BATCHES{6};

    // --- LONG_FAST (SF11) STREAMING state ------------------------------------
    // Store-and-decode can't fit a 58-symbol SF11 packet, so decode STREAMING (one
    // symbol at a time - affordable because SF11 symbols are 8 ms).  Sub-sample lock
    // (the SF7 "last-mile") is solved WITHOUT a whole-packet buffer: in the FINE phase
    // demod each symbol at all N_DA da-offsets (N_DA FFTs spread across the symbol's
    // execute() calls), buffer only the demodded BINS (~1 KB), then decode every
    // (da, off) candidate from the bins and keep the valid-checksum BROADCAST frame.
    // Validated 8/8 byte-exact on real captures (tools/lora_bench/lf_sf11_stream_finealign.py).
    bool sf11_mode_{false};
    // Our Meshtastic node id (from LoRaConfigureMessage). A candidate header is validated
    // by its 4-byte destination: broadcast, or unicast to us (direct messages / ACKs).
    // 0 = not configured -> broadcast only.
    uint32_t local_node_id_{0};
    // da=0 wins on every real burst (off0=up_bin-tau + off+/-2 absorbs the residual), so a
    // small da radius suffices; off search is +/-2 (in sf11_try_decode).
    // The da search is now 0-radius: it cost N_DA demods per FINE symbol, and the real-time
    // budget below allows exactly ONE.  Measured on real captures, da=0 always won anyway,
    // and at OS=1 a da-sample window shift and an off-bin shift are near-degenerate, so the
    // +/-2 off search in sf11_try_decode already absorbs what da used to cover.
    static constexpr int SF11_DA{0};                 // da search radius (samples)
    static constexpr int SF11_NDA{2 * SF11_DA + 1};  // 1 da candidate
    static constexpr int SF11_OFF_RAD{16};           // integer-bin off search radius (samples)
    // Widened from 8: a symbol-timing error of d samples shifts every dechirped bin by d
    // (chirp property), a CONSTANT shift the off search absorbs.  On-air the SFD tau
    // estimate is noisy by tens of samples (the downchirp peak is noisier than the clean
    // preamble), so h0 lands tens of samples off; a d-sample error decodes iff |d| < the
    // off radius AND below the ~96-sample ISI limit (measured: h_pw offline, +-96 decodes,
    // +-112 does not).  Free: the search is bin-domain (deinterleave+hamming), no FFT.
    // 8 header + 6 payload + 1 for the h0/h0+SPS multi-start. Enough to rebuild the four
    // destination bytes the header search checks - AT SF8 AND UP. A payload block yields
    // SF nibbles and the header block hands over (SF-2)-5 of its own, so SF11 has 4+11
    // and SF7 has 0+7: three bytes, one short of the test, which is exactly what the
    // instrumentation measured (best candidate = 3 leading 0xFF, 0 accepted). SF7 needs a
    // second payload block, so the buffer is sized for it and the count is per-SF below.
    static constexpr uint8_t SF11_FINE_SYMS{25};  // 8 + 2*8 + 1: three blocks, so the
                                                  // buffered search can decode the same
                                                  // block the stream gets wrong (BW125
                                                  // only; other rates still use 19)
    static constexpr int SF11_HS_MAX{1};          // header-start hypotheses: h0 (0) and h0+SPS (1)
    static constexpr uint8_t SF11_PRE_MIN{6};     // preamble upchirps before SFD accept

    // --- Real-time budget (measured on hardware, do NOT regress) --------------
    // A buffer is 2048 samples @2 MHz = 1.024 ms, and wait_for_buffer() does NOT queue:
    // it returns the DMA's current position, so overrunning the deadline silently SKIPS
    // buffers (counted in shared_memory.m4_buffer_missed, readable via the `sysinfo`
    // console command).  A whole 2048-pt demod measured ~4.5 ms => running it in one call
    // dropped 3.4 buffers each time: 413 drops/s of 976.6 = 42% of the stream gone.  Every
    // window stayed internally contiguous (sharpness 0.7) but consecutive windows were no
    // longer adjacent in time, so the preamble read as non-periodic (correlation 8 permille
    // vs 950 with the FFT disabled) and its bins as random.
    // Average load is only ~23% (122 symbols/s x ~1.9 ms), so the work is SPREAD, not cut:
    // one demod is split into sf11_nsteps_ (= sps/256 = buffers/symbol) bounded steps, one per
    // buffer, sized so a symbol's buffers complete exactly one demod for EVERY BW250 SF (SF8
    // nsteps=1 ... SF11 nsteps=8).  The log2(sps) butterfly stages are distributed across those
    // steps (see sf11_demod_step); each step stays well under the 1.024 ms buffer deadline.
    // Steps per call.  Measured: 2 steps (4 FFT stages) overrun the deadline and bring the
    // drops straight back (8857 in 49 s), so ONE step per call is the ceiling - i.e. exactly
    // one demod per symbol's 8 buffers.  Every stage must therefore live within that: the
    // SFD search used to spend two demods per symbol (up-ref then down-ref) and so fell
    // behind real time; it is now a single down-ref demod (see sf11_consume).
    // ...but "one" is the ceiling for a STEP THE SIZE OF SF11's, not a property of the
    // core. A step costs about sps*(log2(sps)+3): 3584 units at SF11 (~0.56 ms, proven),
    // against 1280 for a WHOLE demod at SF7. SF7's symbol is 128 samples, so two of them
    // arrive in every 256-sample buffer and two whole demods per call - 2560 units, still
    // under one SF11 step - are what it takes to see both. With the fixed 1 the
    // demodulator read every other symbol, which is exactly how SHORT_FAST behaved:
    // occasionally right, mostly not.
    uint32_t sf11_steps_per_call_{1};
    static constexpr float SF11_SHARP_THR{0.02f};  // peak^2/total gate for a "sharp" chirp
    // ...which is an ABSOLUTE gate, while the thing it has to clear is not. Sharpness is
    // peak^2/total over N bins, so pure noise already reads about ln(N)/N: 0.0037 at
    // SF11's 2048 bins - the 0.02 gate sits a healthy 5x above it - but 0.038 at SF7's
    // 128, which is ABOVE the gate. Every noise symbol therefore read as a sharp chirp,
    // the preamble run latched onto whatever bins the noise threw up, and the state
    // machine spent its time in false SFD searches instead of hunting for the packet.
    // Rescaled for SF7 only; SF8 and up keep the exact gate they were tuned against.
    float sf11_sharp_thr_{SF11_SHARP_THR};
    static constexpr float SF11_SFD_THR{0.15f};  // stronger gate for the SFD downchirp (fire in its pure middle)
    static constexpr int SF11_STORE_SHIFT{7};    // starting point for store_shift_
    // How far the decimator output is shifted down before it is kept as int8, chosen
    // from the signal level between packets: a fixed shift crushed weak signals to a
    // couple of levels and cost real sensitivity.
    int store_shift_{SF11_STORE_SHIFT};
    int agc_peak_{0};
    enum class Sf11State : uint8_t { HUNT,
                                     RESOLVE,
                                     FINE,
                                     PAYLOAD };
    Sf11State sf11_state_{Sf11State::HUNT};
    uint32_t win_w_{0};      // win11 monotonic write count
    uint32_t sf11_read_{0};  // next symbol start (win index) in HUNT
    // preamble / SFD detection
    uint8_t sf11_pre_run_{0};
    int32_t sf11_prev_bin_{-99};
    uint32_t sf11_up_bin_{0};
    // Timing tracking: the win11 grid slips relative to the signal (M0-contention
    // sample drops), so a fixed sf11_read_ += SPS makes consecutive preamble bins
    // drift -> never 6-in-a-row.  Re-center each preamble window to sf11_pre_ref_ by
    // advancing SPS - (bin - ref).  No-op when stable; corrective when it slips.
    uint32_t sf11_pre_ref_{0};
    static constexpr int SF11_DRIFT_TOL{48};        // max per-symbol bin slip counted as preamble
    uint32_t sf11_pre_pos_{0};                      // win index of the last preamble window
    std::complex<float> sf11_cfo_acc_{0.0f, 0.0f};  // sum pk*conj(prev_pk) for phase-slope CFO
    std::complex<float> sf11_prev_pk_{0.0f, 0.0f};
    bool sf11_confirmed_{false};
    uint8_t sf11_sfd_step_{0};  // SFD search: 0=do up demod, 1=do down
    bool sf11_in_sfd_{false};   // (unused since down-only revert)
    // tau mod-N/2 ambiguity resolution: (up-down)/2 fixes tau only mod N/2, so h0 has a
    // half-symbol ambiguity. RESOLVE demods the first header symbol at h0 and h0+-N/2 and
    // keeps the sharpest (a half-symbol-off window straddles two header symbols -> dull).
    uint8_t sf11_res_idx_{0};  // 0,1,2 -> h0-N/2, h0, h0+N/2
    long sf11_res_besth_{0};
    int32_t sf11_res_bestbin_{0};
    float sf11_res_bestfrac_{0.0f};
    uint16_t sf11_miss0_{0};  // m4_buffer_missed snapshot at FINE start (drop detection)
    float sf11_res_bestsh_{-1.0f};
    float sf11_up_sharp_cache_{0.0f};  // up-sharpness cached between the 2 steps
    float sf11_cfo_ramp_{0.0f};        // fractional CFO (rad/sample)
    // Generated-reference state (SF12): the same recurrence sf11_recompute_ref uses to
    // fill the buffer, carried between dechirp chunks instead. Starting it afresh in the
    // middle would mean evaluating a quadratic phase of ~13000 radians in single
    // precision, which is worth about a milliradian - continuing the recurrence is both
    // cheaper and exact.
    bool sf11_gen_ref_{false};
    uint32_t sf11_dechirp_steps_{1};                // chunks the dechirp is cut into
    float sf11_gz_r_{1.0f}, sf11_gz_i_{0.0f};       // z = exp(-j*ph(n))
    float sf11_gd_r_{1.0f}, sf11_gd_i_{0.0f};       // d = exp(-j*(ph(n+1)-ph(n)))
    float sf11_ref_ramp_{0.0f};                     // the ramp the reference is built with
    uint32_t sf11_off0_{0};                         // measured integer offset (preamble bin)
    uint32_t sf11_h0_{0};                           // header start (win index)
    float sf11_last_sharp_{0.0f};                   // peak^2/total of the last sf11_demod
    std::complex<float> sf11_last_pk_{0.0f, 0.0f};  // complex peak (for phase-slope CFO)
    float sf11_last_frac_{0.0f};                    // parabolic sub-bin of the last demod peak
    // Mid-packet sub-bin refinement: average this many payload symbols, then fold the
    // bias into the reference once. Enough to average the parabolic estimate's noise
    // down, few enough to still fix most of the packet.
    static constexpr uint32_t SF11_FRAC_SYMS{6};
    // Sub-bin tracking through the payload. A single estimate taken at the header cannot
    // follow a residual that drifts, and at BW125 - 61 Hz to a bin against BW250's 122 -
    // it does not have to drift far to matter. Accumulate each symbol's residual and step
    // the working offset by one bin whenever half a bin has built up.
    static constexpr float SF11_TRACK_GAIN{0.25f};
    float sf11_pre_frac_{0.0f};    // preamble peak sub-bin (fractional-CFO correction)
    bool sf11_job_active_{false};  // a staged demod is in flight
    uint8_t sf11_job_step_{0};     // next step index (0..SF11_STEPS-1)
    uint32_t sf11_job_start_{0};   // win11 index being demodded
    bool sf11_job_up_{true};       // up- or down-chirp reference
    // FINE fine-align: per-da bins (tiny), processed a few FFTs per execute() call
    int16_t sf11_bins_[SF11_NDA][SF11_FINE_SYMS]{};
    // How many of those bins this SF actually collects before the header search runs:
    // 14 for SF8..SF11 exactly as before, 19 at SF7. Waiting for symbols a preset does
    // not need would delay every packet it already decodes.
    uint8_t sf11_fine_syms_{SF11_FINE_SYMS};
    uint8_t sf11_fine_sym_{0};  // current FINE symbol being demodded
    uint8_t sf11_fine_da_{0};   // current da index within the symbol
    // chosen winner + payload decode
    int sf11_win_da_{0};
    uint32_t sf11_win_off_{0};
    uint16_t sf11_pay_need_{0};  // payload symbols to collect
    uint16_t sf11_pay_have_{0};

    void execute_sf11(const buffer_c8_t& buffer);
    // Staged demod: start a job, then run ONE bounded step per buffer (see SF11_STEPS).
    void sf11_demod_start(uint32_t win_start, bool is_up);
    bool sf11_demod_step();                     // true = result ready in last_peak_*
    void sf11_dechirp();                        // xconj-ref window -> sf11_fft()
    bool sf11_start_job();                      // pick next window per state; false = not ready
    bool sf11_win_stale(uint32_t start) const;  // window already overwritten (fell behind)
    void sf11_consume();                        // act on a finished demod
    void sf11_recompute_ref(float cfo_ramp);    // rebuild ref11 = exp(-j(base+cfo*n))
    void sf11_dechirp_chunk(uint32_t part, uint32_t parts);
    uint8_t sf11_decode_bins(const int16_t* bins, uint8_t nbins, uint32_t off, uint8_t* out, uint8_t out_cap);  // bins->frame bytes (self-contained)
    bool sf11_try_decode();                                                                                     // decode all (da,off) from bins -> winner
    void sf11_reset();

    // --- Core DSP ------------------------------------------------------------
    // Fold demod of the symbol at ring index `start`: dechirp (x ref_chirp_),
    // FFT(sps), sum the os aliases -> 128 bins, argmax. Returns the chip value.
    int32_t dechirp_at(uint32_t start);        // sets last_peak_mag_/bin_/frac_
    int32_t dechirp_down(uint32_t start);      // DOWN-ref dechirp (SFD): sets down_peak_*
    float sharpness_at(uint32_t start);        // peak/total from the ring; sets sharp_peak_bin_
    float sharpness_buf(const complex8_t* p);  // peak/total from a linear buffer; sets sharp_peak_bin_
    uint16_t gray_decode(uint16_t value) const;

    // --- Decode chain ---------------------------------------------------------
    void process_one_symbol();
    uint8_t decode_header();  // reduced-rate header block -> packet length
    void process_sym_block();
    void feed_nibble(uint8_t nibble);  // nibble -> byte assembly + dewhiten -> payload_buf
    uint8_t lora_whiten_step();
    void reset_rx();

    void configure(const LoRaConfigureMessage& message);
    void send_packet(const uint8_t* data, size_t len);
    // Tell the application whether a packet is currently being demodulated, so it can
    // listen before talking (only sent on a change - one message per packet edge).
    void set_rx_busy(bool busy);
    bool rx_busy_{false};

    /* NB: Threads must be the last members. */
    BasebandThread baseband_thread{baseband_fs, this, baseband::Direction::Receive};
    RSSIThread rssi_thread{};
};

#endif /* __PROC_LORA_H__ */
