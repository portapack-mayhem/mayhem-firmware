/*
 * Copyright (C) 2026
 *
 * This file is part of PortaPack / Mayhem.
 *
 * FEC pipeline wiring is aligned with SatDump
 * `plugins/meteor_support/meteor/module_meteor_lrpt_decoder.cpp` (see
 * `firmware/baseband/meteor_lrpt/SATDUMP_VENDOR.md` for pinned commit and raw URLs).
 */
#include "proc_meteor_lrpt_rx.hpp"

#include "hal.h"
#include "audio_dma.hpp"
#include "portapack_shared_memory.hpp"
#include "event_m4.hpp"
#include "dsp_types.hpp"

#include "meteor_lrpt/ccsds_derandomize.hpp"
#include "meteor_lrpt/meteor_jpeg_scan.hpp"
#include "meteor_lrpt/rs223_decode.hpp"
#include "meteor_lrpt/meteor_polyphase_interp.hpp"
#include "meteor_lrpt/meteor_soft_correlate.hpp"
#include "meteor_lrpt/meteor_symbol_timing.hpp"

#include "lpc43xx_cpp.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

using namespace lpc43xx;

namespace {
constexpr meteor_lrpt::MeteorLrptSymbolTimingGains kSymTimingGains{};
}

/* CCSDS ASM 0x1ACFFC1D (big-endian marker); file prefix matches SatDump CADU export */
static constexpr uint8_t kCaduAsm[] = {0x1A, 0xCF, 0xFC, 0x1D};

/**
 * Push one post-RS 1020 B CADU record into the M4→M0 ring (LRPT live path).
 *
 * SPSC discipline: M4 is the **only** writer of `ring_push` and slot payloads; M0 is the **only**
 * writer of `ring_pop` (`g4_ring_pop` in `meteor_lrpt_g4_service.cpp`). When the ring is full,
 * we **drop the incoming** CADU (count + sticky `G4_DROP_RING_OVERFLOW`) instead of advancing
 * `ring_pop` from M4, which would race with the consumer.
 */
static void meteor_lrpt_g4_ring_push_post_rs(const uint8_t* cadu1020) {
    auto& g4 = shared_memory.meteor_lrpt_g4_ipc;
    if (!cadu1020 || !g4.enabled || !g4.live_ring_enable)
        return;
    __DMB();
    const uint32_t w = g4.ring_push;
    const uint32_t r = g4.ring_pop;
    constexpr uint32_t kSlots = (uint32_t)SharedMemory::MeteorLrptG4Ipc::kLiveRingSlots;
    if (w - r >= kSlots) {
        g4.drop_bits |= (1u << 8); /* G4_DROP_RING_OVERFLOW */
        g4.live_ring_overflows++;
        __DMB();
        return;
    }
    const uint32_t slot = w % kSlots;
    std::memcpy((void*)g4.live_ring_slots[slot], cadu1020, 1020u);
    g4.ring_push = w + 1u;
    __DMB();
}

int8_t MeteorLrptRx::clamp8(int v) {
    if (v > 127) return 127;
    if (v < -128) return -128;
    return (int8_t)v;
}

int16_t MeteorLrptRx::clamp16(int32_t v) {
    if (v > 32767) return 32767;
    if (v < -32768) return -32768;
    return (int16_t)v;
}

complex16_t MeteorLrptRx::dc_and_rrc(complex16_t s) {
    constexpr float dc_alpha = 0.002f;
    dc_i_ += dc_alpha * ((float)s.real() - dc_i_);
    dc_q_ += dc_alpha * ((float)s.imag() - dc_q_);
    const float xrf = (float)s.real() - dc_i_;
    const float xqf = (float)s.imag() - dc_q_;
    const complex16_t z{clamp16((int32_t)xrf), clamp16((int32_t)xqf)};

    const size_t newest = rrc_w_ % meteor_lrpt::sym_shaping_ntaps;
    rrc_hist_[newest] = z;
    rrc_w_++;

    int64_t acc_r = 0;
    int64_t acc_i = 0;
    for (size_t k = 0; k < meteor_lrpt::sym_shaping_ntaps; k++) {
        const size_t idx = (newest + meteor_lrpt::sym_shaping_ntaps - k) % meteor_lrpt::sym_shaping_ntaps;
        const int64_t tr = rrc_hist_[idx].real();
        const int64_t ti = rrc_hist_[idx].imag();
        const int64_t tap = meteor_lrpt::sym_shaping_taps_q13[k];
        acc_r += tap * tr;
        acc_i += tap * ti;
    }
    acc_r >>= 13;
    acc_i >>= 13;
    return {clamp16((int32_t)acc_r), clamp16((int32_t)acc_i)};
}

void MeteorLrptRx::configure(const MeteorLrptRxConfigureMessage& message) {
    flags_ = message.flags;
    interleaved_post_.reset();
    m4_ipc_post_seq_ = 0;
    if (!(message.flags & (1u << 1))) {
        shared_memory.meteor_lrpt_ipc.magic = 0;
        shared_memory.meteor_lrpt_ipc.state = 0;
        shared_memory.meteor_lrpt_ipc.m4_exec_at_post = 0;
        shared_memory.meteor_lrpt_ipc.seq = 0;
    }
    nrzm_.reset();
    nrzm_bits_.reset();
    bpsk_deframer_.reset();
    dc_i_ = dc_q_ = 0.0f;
    rrc_hist_.fill({});
    rrc_w_ = 0;
    sym_phase_f_ = sym_pll_i_ = sym_rate_fine_ = sym_last_ted_ = 0.f;
    sym_ir_hist_[0] = sym_ir_hist_[1] = sym_ir_hist_[2] = 0;
    for (float& v : sym_ci_hist_)
        v = 0.f;
    for (float& v : sym_cq_hist_)
        v = 0.f;

    uint32_t sk = message.symbol_rate_k;
    if (sk < 50 || sk > 120)
        sk = 72;
    sym_rate_hz_ = sk * 1000U;

    decim_0.set<dsp::decimate::FIRC8xR16x24FS4Decim8>().configure(taps_200k_decim_0.taps);
    decim_1.set<dsp::decimate::FIRC16xR16x16Decim2>().configure(taps_200k_decim_1.taps);

    configured = true;
}

void MeteorLrptRx::capture_config(const CaptureConfigMessage& message) {
    if (!message.config) {
        cadu_stream_.reset();
        soft_stream_.reset();
        return;
    }
    /* Route to one M4 sink; drop the other so we never hold two StreamInput configs. */
    if (message.config->write_size == soft_bytes_needed ||
        message.config->write_size == interleaved_soft_bytes) {
        cadu_stream_.reset();
        soft_stream_ = std::make_unique<StreamInput>(message.config);
    } else {
        soft_stream_.reset();
        cadu_stream_ = std::make_unique<StreamInput>(message.config);
    }
}

void MeteorLrptRx::update_params() {
}

void MeteorLrptRx::soft_circular_shift(const int8_t* src, const size_t len, const size_t shift, int8_t* dst) {
    if (!len)
        return;
    const size_t s = shift % len;
    for (size_t i = 0; i < len; i++)
        dst[i] = src[(i + s) % len];
}

bool MeteorLrptRx::post_fec_frame_checks(std::array<uint8_t, 1024>& frame) {
    meteor_derand_ccsds(&frame[4], 1020);

    if (frame[9] == 0xff) {
        for (size_t i = 0; i < frame.size(); i++)
            frame[i] ^= 0xff;
    }

    return memcmp(&frame[4], kCaduAsm, sizeof(kCaduAsm)) == 0;
}

bool MeteorLrptRx::process_soft_to_frame(const int8_t* soft, std::array<uint8_t, 1024>& frame) {
    nrzm_.reset();
    viterbi_.decode_soft(soft, frame.data(), frame.size());

    if (flags_ & (1u << 2))
        nrzm_.decode(frame.data(), (int)frame.size());

    return post_fec_frame_checks(frame);
}

bool MeteorLrptRx::process_m2x_noninterleaved(const int8_t* soft, bool diff, std::array<uint8_t, 1024>& frame) {
    std::array<uint8_t, 1024> vb{};
    viterbi_.decode_soft(soft, vb.data(), vb.size());

    size_t bi = 0;
    for (size_t i = 0; i < vb.size(); i++) {
        const uint8_t b = vb[i];
        for (int k = 7; k >= 0; k--)
            m2x_bits_[bi++] = (uint8_t)((b >> (unsigned)k) & 1u);
    }

    if (diff)
        nrzm_bits_.decode_bits(m2x_bits_.data(), 8192);

    const int n = bpsk_deframer_.work(m2x_bits_.data(), 8192, deframer_batch_out_.data());
    const int max_frames = (int)(deframer_batch_out_.size() / 1024);
    for (int fi = 0; fi < n && fi < max_frames; fi++) {
        std::memcpy(frame.data(), deframer_batch_out_.data() + (size_t)fi * 1024, 1024);
        if (post_fec_frame_checks(frame))
            return true;
    }
    return false;
}

size_t MeteorLrptRx::soft_bytes_target() const {
    const bool m2x = (flags_ & 1u) != 0;
    const bool interleaved = (flags_ & (1u << 1)) != 0;
    if (m2x && interleaved)
        return interleaved_soft_bytes;
    return soft_bytes_needed;
}

bool MeteorLrptRx::interleaved_ipc_ready() const {
    const auto& ipc = shared_memory.meteor_lrpt_ipc;
    return ipc.magic == SharedMemory::MeteorLrptIpc::kMagic && ipc.soft_in && ipc.deint_a && ipc.deint_b;
}

void MeteorLrptRx::finish_interleaved_from_m0() {
    const bool m2x = (flags_ & 1u) != 0;
    const bool interleaved = (flags_ & (1u << 1)) != 0;
    if (!m2x || !interleaved)
        return;

    auto& ipc = shared_memory.meteor_lrpt_ipc;
    if (ipc.magic != SharedMemory::MeteorLrptIpc::kMagic || ipc.state != 2u)
        return;
    if (ipc.seq != m4_ipc_post_seq_) {
        ipc.state = 0u;
        ipc.m4_exec_at_post = 0u;
        __DMB();
        status_.cadu_asm_rejects++;
        return;
    }

    const bool diff = (flags_ & (1u << 2)) != 0;
    std::array<uint8_t, 1024> frame{};
    int8_t* da = const_cast<int8_t*>(ipc.deint_a);
    int8_t* db = const_cast<int8_t*>(ipc.deint_b);
    bool asm_ok = interleaved_post_.process(da, db, diff, frame);
    if (asm_ok)
        asm_ok = post_fec_frame_checks(frame);

    status_.m2x_vit_winner = interleaved_post_.last_viterbi_winner();
    status_.m2x_vit_state_a = (uint8_t)interleaved_post_.viterbi_state_a();
    status_.m2x_vit_state_b = (uint8_t)interleaved_post_.viterbi_state_b();
    {
        const float bwin = interleaved_post_.last_viterbi_winner() ? interleaved_post_.viterbi_ber_b()
                                                                   : interleaved_post_.viterbi_ber_a();
        const int cs = (int)(bwin * 200.f);
        status_.corr_score = (uint8_t)std::max(0, std::min(cs, 63));
        status_.corr_lock = (interleaved_post_.viterbi_state_a() == 1 || interleaved_post_.viterbi_state_b() == 1) ? 1 : 0;
        status_.soft_align_skip = (uint16_t)std::min((size_t)(int)(bwin * 1000.f), (size_t)65535);
    }

    if (!asm_ok) {
        ipc.state = 0;
        ipc.m4_exec_at_post = 0;
        __DMB();
        status_.cadu_asm_rejects++;
        status_.fec_lock = 0;
        status_.viterbi_sync = 0;
        status_.soft_rotate_shift = 0;
        status_.rs_err0 = status_.rs_err1 = status_.rs_err2 = status_.rs_err3 = -1;
        status_.deframer_sync = 0;
        return;
    }

    status_.cadu_asm_accepts++;
    status_.soft_rotate_shift = 0;
    status_.fec_lock = 1;
    status_.viterbi_sync = 1;
    status_.deframer_sync = (interleaved_post_.bpsk_deframer_state() == MeteorBpskCcsdsDeframer::kStateSynced) ? 1 : 0;

    std::array<uint8_t, 1020> pre_rs{};
    std::memcpy(pre_rs.data(), &frame[4], pre_rs.size());
    int16_t rs_e[4]{-1, -1, -1, -1};
    const bool rs_ok = meteor_lrpt_rs_decode_interleaved_depth4(&frame[4], rs_e);
    status_.rs_err0 = rs_e[0];
    status_.rs_err1 = rs_e[1];
    status_.rs_err2 = rs_e[2];
    status_.rs_err3 = rs_e[3];
    if (!rs_ok) {
        std::memcpy(&frame[4], pre_rs.data(), pre_rs.size());
        ipc.state = 0;
        ipc.m4_exec_at_post = 0;
        __DMB();
        return;
    }

    {
        const unsigned sum = (unsigned)std::max(0, (int)rs_e[0]) + (unsigned)std::max(0, (int)rs_e[1]) +
                               (unsigned)std::max(0, (int)rs_e[2]) + (unsigned)std::max(0, (int)rs_e[3]);
        status_.ber_x1000 = (uint16_t)std::min(1000u, sum * 5u);
    }

    status_.cadu_frames++;

    if (cadu_stream_) {
        (void)cadu_stream_->write(&frame[4], 1020);
    }
    meteor_lrpt_g4_ring_push_post_rs(&frame[4]);

    preview_.line_y = (uint16_t)(status_.cadu_frames & 0xffff);
    preview_.pixel_count = (uint16_t)meteor_scan_jpeg_preview_gray(frame.data(), frame.size(), preview_.gray, sizeof(preview_.gray));
    if (preview_.pixel_count > 0)
        shared_memory.application_queue.push(preview_);

    ipc.state = 0;
    ipc.m4_exec_at_post = 0;
    __DMB();
}

void MeteorLrptRx::try_decode_block() {
    status_.ber_x1000 = 0;
    status_.interleaved_mode_flags = 0;
    status_.m2x_vit_winner = 0;
    status_.m2x_vit_state_a = 0;
    status_.m2x_vit_state_b = 0;

    const bool m2x = (flags_ & 1u) != 0;
    const bool interleaved = (flags_ & (1u << 1)) != 0;
    const bool diff = (flags_ & (1u << 2)) != 0;
    const bool leg_corr_fallback = (flags_ & (1u << 3)) != 0;

    /* SOFT REC block size matches `soft_bytes_target()` (SatDump SOFT block layout parity). */
    if (soft_stream_) {
        (void)soft_stream_->write(soft_buf_.data(), soft_bytes_target());
    }

    std::array<uint8_t, 1024> frame{};
    bool asm_ok = false;
    uint8_t win_shift = 0;
    meteor_lrpt::CorrQpskResult corr_out{};

    if (m2x && interleaved && interleaved_ipc_ready())
        status_.interleaved_mode_flags |= 2u; /* on-device M0 deint + M4 Viterbi1_2 */
    else if (m2x && interleaved)
        status_.interleaved_mode_flags |= 1u; /* host_decode_recommended / IPC not ready */

    if (!m2x) {
        /* Legacy SatDump: QPSK soft correlator + rotate_soft on full 16k block, then Viterbi. */
        std::memcpy(soft_rot_work_.data(), soft_buf_.data(), soft_bytes_needed);
        meteor_lrpt::correlate_rotate_qpsk_legacy(soft_rot_work_.data(), soft_bytes_needed, diff, &corr_out);
        asm_ok = process_soft_to_frame(soft_rot_work_.data(), frame);
        if (!asm_ok && leg_corr_fallback) {
            for (unsigned sh = 2; sh < 64; sh += 2) {
                soft_circular_shift(soft_buf_.data(), soft_bytes_needed, sh, soft_rot_work_.data());
                meteor_lrpt::correlate_rotate_qpsk_legacy(soft_rot_work_.data(), soft_bytes_needed, diff, &corr_out);
                if (process_soft_to_frame(soft_rot_work_.data(), frame)) {
                    asm_ok = true;
                    win_shift = (uint8_t)sh;
                    break;
                }
            }
        }
    } else if (interleaved) {
        if (interleaved_ipc_ready()) {
            auto& ipc = shared_memory.meteor_lrpt_ipc;
            if (ipc.state != 0u) {
                status_.interleaved_mode_flags |= 8u;
                soft_fill_ = 0;
                return;
            }
            std::memcpy((void*)ipc.soft_in, soft_buf_.data(), interleaved_soft_bytes);
            __DMB();
            ipc.seq++;
            m4_ipc_post_seq_ = ipc.seq;
            ipc.m4_exec_at_post = execute_count_;
            ipc.state = 1u;
            __DMB();
            creg::m4txevent::assert_event();
            soft_fill_ = 0;
            return;
        }
        asm_ok = process_soft_to_frame(soft_buf_.data(), frame);
        win_shift = 0;
    } else {
        /* M2-x non-interleaved: Viterbi bytes → bits → optional NRZ-M bits → CCSDS deframer (SatDump BPSK_CCSDS_Deframer). */
        asm_ok = process_m2x_noninterleaved(soft_buf_.data(), diff, frame);
        win_shift = 0;
    }

    status_.corr_score = (uint8_t)std::min(corr_out.score, 64);
    status_.corr_lock = corr_out.locked ? 1 : 0;
    status_.soft_align_skip = (uint16_t)std::min(corr_out.align_skip, (size_t)65535);

    if (!asm_ok) {
        status_.cadu_asm_rejects++;
        status_.fec_lock = 0;
        status_.viterbi_sync = 0;
        status_.soft_rotate_shift = 0;
        status_.rs_err0 = status_.rs_err1 = status_.rs_err2 = status_.rs_err3 = -1;
        status_.deframer_sync = 0;
        if (m2x && !interleaved)
            status_.deframer_sync = (bpsk_deframer_.get_state() == MeteorBpskCcsdsDeframer::kStateSynced) ? 1 : 0;
        soft_fill_ = 0;
        return;
    }

    status_.cadu_asm_accepts++;
    status_.soft_rotate_shift = win_shift;
    status_.fec_lock = 1;
    status_.viterbi_sync = 1;
    status_.deframer_sync = 0;
    if (m2x && !interleaved)
        status_.deframer_sync = (bpsk_deframer_.get_state() == MeteorBpskCcsdsDeframer::kStateSynced) ? 1 : 0;

    std::array<uint8_t, 1020> pre_rs{};
    std::memcpy(pre_rs.data(), &frame[4], pre_rs.size());
    int16_t rs_e[4]{-1, -1, -1, -1};
    const bool rs_ok = meteor_lrpt_rs_decode_interleaved_depth4(&frame[4], rs_e);
    status_.rs_err0 = rs_e[0];
    status_.rs_err1 = rs_e[1];
    status_.rs_err2 = rs_e[2];
    status_.rs_err3 = rs_e[3];
    if (!rs_ok) {
        std::memcpy(&frame[4], pre_rs.data(), pre_rs.size());
        soft_fill_ = 0;
        return;
    }

    {
        const unsigned sum = (unsigned)std::max(0, (int)rs_e[0]) + (unsigned)std::max(0, (int)rs_e[1]) +
                               (unsigned)std::max(0, (int)rs_e[2]) + (unsigned)std::max(0, (int)rs_e[3]);
        status_.ber_x1000 = (uint16_t)std::min(1000u, sum * 5u);
    }

    status_.cadu_frames++;

    if (cadu_stream_) {
        (void)cadu_stream_->write(&frame[4], 1020);
    }
    meteor_lrpt_g4_ring_push_post_rs(&frame[4]);

    preview_.line_y = (uint16_t)(status_.cadu_frames & 0xffff);
    preview_.pixel_count = (uint16_t)meteor_scan_jpeg_preview_gray(frame.data(), frame.size(), preview_.gray, sizeof(preview_.gray));
    if (preview_.pixel_count > 0)
        shared_memory.application_queue.push(preview_);

    soft_fill_ = 0;
}

void MeteorLrptRx::execute(const buffer_c8_t& buffer) {
    if (!configured)
        return;

    ++execute_count_;

    if ((flags_ & 1u) && (flags_ & (1u << 1))) {
        auto& ipc = shared_memory.meteor_lrpt_ipc;
        if (ipc.magic == SharedMemory::MeteorLrptIpc::kMagic && ipc.state == 1u && ipc.m4_exec_at_post != 0u) {
            if (execute_count_ - ipc.m4_exec_at_post > 8000u) {
                ipc.dropped++;
                ipc.seq++;
                ipc.state = 0u;
                ipc.m4_exec_at_post = 0u;
                __DMB();
            }
        }
        finish_interleaved_from_m0();
    }

    const auto d0 = decim_0.execute(buffer, dst_buffer);
    const auto d1 = decim_1.execute(d0, dst_buffer);

    feed_channel_stats(d1);

    const float spf = (float)fs_sym_path / (float)sym_rate_hz_;
    for (size_t i = 0; i < d1.count; i++) {
        const complex16_t s = dc_and_rrc(d1.p[i]);
        const int32_t pwr = (int32_t)s.real() * (int32_t)s.real() + (int32_t)s.imag() * (int32_t)s.imag();
        float target = 1.5e6f;
        float g = agc_gain_;
        if (pwr > 32) {
            float est = target / (float)pwr;
            if (est < 0.25f)
                est = 0.25f;
            if (est > 8.0f)
                est = 8.0f;
            g = g * 0.95f + est * 0.05f;
            agc_gain_ = g;
        }

        const int32_t ir = (int32_t)((float)s.real() * g / 256.0f);
        const int32_t qsym = prev_q_;
        prev_q_ = (int32_t)((float)s.imag() * g / 256.0f);

        const float fi = (float)ir / 1024.f;
        const float fq = (float)qsym / 1024.f;

        const float step = (1.f / spf) * (1.f + sym_rate_fine_);
        const float ph_before = sym_phase_f_;
        sym_phase_f_ += step;
        if (sym_phase_f_ >= 1.f) {
            sym_phase_f_ -= 1.f;
            const float lambda = (step > 1e-12f) ? std::clamp((1.f - ph_before) / step, 0.f, 1.f) : 0.f;
            const float p3i = meteor_lrpt::extrap_next_sample(sym_ci_hist_[2], sym_ci_hist_[3], fi);
            const float p3q = meteor_lrpt::extrap_next_sample(sym_cq_hist_[2], sym_cq_hist_[3], fq);
            const float ir_mid_f =
                meteor_lrpt::catmull_rom_segment(sym_ci_hist_[2], sym_ci_hist_[3], fi, p3i, lambda);
            const float q_mid_f =
                meteor_lrpt::catmull_rom_segment(sym_cq_hist_[2], sym_cq_hist_[3], fq, p3q, lambda);
            const int32_t ir_mid = (int32_t)std::lround(ir_mid_f * 1024.f);
            const int32_t q_mid = (int32_t)std::lround(q_mid_f * 1024.f);

            sym_ir_hist_[2] = sym_ir_hist_[1];
            sym_ir_hist_[1] = sym_ir_hist_[0];
            sym_ir_hist_[0] = ir_mid;
            /* Gardner-style TED on strobed cubic I; scaled for int32 path (see tools/meteor_lrpt/README.md). */
            const float ted = (float)sym_ir_hist_[1] * ((float)sym_ir_hist_[0] - (float)sym_ir_hist_[2]) * kSymTimingGains.ted_scale;
            sym_last_ted_ = ted;
            sym_pll_i_ += kSymTimingGains.pll_i_gain * ted;
            float adj = kSymTimingGains.rate_prop_gain * ted + sym_pll_i_;
            if (adj > kSymTimingGains.rate_clamp)
                adj = kSymTimingGains.rate_clamp;
            if (adj < -kSymTimingGains.rate_clamp)
                adj = -kSymTimingGains.rate_clamp;
            sym_rate_fine_ = adj;
            if (soft_fill_ + 2 <= soft_buf_.size()) {
                soft_buf_[soft_fill_++] = clamp8((int)(ir_mid / 1024));
                soft_buf_[soft_fill_++] = clamp8((int)(q_mid / 1024));
            }
        }

        sym_ci_hist_[0] = sym_ci_hist_[1];
        sym_ci_hist_[1] = sym_ci_hist_[2];
        sym_ci_hist_[2] = sym_ci_hist_[3];
        sym_ci_hist_[3] = fi;
        sym_cq_hist_[0] = sym_cq_hist_[1];
        sym_cq_hist_[1] = sym_cq_hist_[2];
        sym_cq_hist_[2] = sym_cq_hist_[3];
        sym_cq_hist_[3] = fq;
    }

    if (soft_fill_ >= soft_bytes_target())
        try_decode_block();

    if ((execute_count_ & 0x3f) == 0) {
        status_.soft_sym_count = (uint32_t)soft_fill_;
        status_.demod_lock = soft_fill_ > 8000 ? 1 : 0;
        status_.sym_timing_err = (int16_t)std::max(-32767, std::min(32767, (int)(sym_last_ted_ * 20000.f)));
        status_.sym_timing_lock = (uint8_t)((sym_rate_fine_ > -kSymTimingGains.rate_clamp * 0.3f &&
                                             sym_rate_fine_ < kSymTimingGains.rate_clamp * 0.3f && soft_fill_ > 4000)
                                                ? 1
                                                : 0);
        status_.ipc_deint_dropped = (uint16_t)std::min<uint32_t>(
            (uint32_t)shared_memory.meteor_lrpt_ipc.dropped, 0xFFFFu);
        status_.ipc_sd_deint_errors = (uint16_t)std::min<uint32_t>(
            (uint32_t)shared_memory.meteor_lrpt_ipc.sd_deint_errors, 0xFFFFu);
        shared_memory.application_queue.push(status_);
    }
}

void MeteorLrptRx::on_message(const Message* const message) {
    switch (message->id) {
        case Message::ID::UpdateSpectrum:
        case Message::ID::SpectrumStreamingConfig:
            break;
        case Message::ID::MeteorLrptRxConfigure:
            configure(*reinterpret_cast<const MeteorLrptRxConfigureMessage*>(message));
            break;
        case Message::ID::CaptureConfig:
            capture_config(*reinterpret_cast<const CaptureConfigMessage*>(message));
            break;
        default:
            break;
    }
}

int main() {
    audio::dma::init_audio_out();
    EventDispatcher event_dispatcher{std::make_unique<MeteorLrptRx>()};
    event_dispatcher.run();
    return 0;
}
