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

#include "audio_dma.hpp"
#include "portapack_shared_memory.hpp"
#include "event_m4.hpp"
#include "dsp_types.hpp"

#include "meteor_lrpt/ccsds_derandomize.hpp"
#include "meteor_lrpt/meteor_jpeg_scan.hpp"
#include "meteor_lrpt/rs223_decode.hpp"
#include "meteor_lrpt/meteor_soft_correlate.hpp"

#include <algorithm>
#include <cstring>

/* CCSDS ASM 0x1ACFFC1D (big-endian marker); file prefix matches SatDump CADU export */
static constexpr uint8_t kCaduAsm[] = {0x1A, 0xCF, 0xFC, 0x1D};

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
    nrzm_.reset();
    nrzm_bits_.reset();
    bpsk_deframer_.reset();
    dc_i_ = dc_q_ = 0.0f;
    rrc_hist_.fill({});
    rrc_w_ = 0;

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
    if (message.config->write_size == soft_bytes_needed) {
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

void MeteorLrptRx::try_decode_block() {
    status_.ber_x1000 = 0;

    if (soft_stream_) {
        (void)soft_stream_->write(soft_buf_.data(), soft_bytes_needed);
    }

    std::array<uint8_t, 1024> frame{};
    bool asm_ok = false;
    uint8_t win_shift = 0;
    meteor_lrpt::CorrQpskResult corr_out{};

    const bool m2x = (flags_ & 1u) != 0;
    const bool interleaved = (flags_ & (1u << 1)) != 0;
    const bool diff = (flags_ & (1u << 2)) != 0;
    const bool leg_corr_fallback = (flags_ & (1u << 3)) != 0;

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
        /* Interleaved M2-x needs phased deinterleave + dual Viterbi (not on-device); byte-ASM path for basic captures. */
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

    preview_.line_y = (uint16_t)(status_.cadu_frames & 0xffff);
    preview_.pixel_count = (uint16_t)meteor_scan_jpeg_preview_gray(frame.data(), frame.size(), preview_.gray, sizeof(preview_.gray));
    if (preview_.pixel_count > 0)
        shared_memory.application_queue.push(preview_);

    soft_fill_ = 0;
}

void MeteorLrptRx::execute(const buffer_c8_t& buffer) {
    if (!configured)
        return;

    const auto d0 = decim_0.execute(buffer, dst_buffer);
    const auto d1 = decim_1.execute(d0, dst_buffer);

    feed_channel_stats(d1);

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

        phase_acc_ += sym_rate_hz_;
        if (phase_acc_ >= fs_sym_path) {
            phase_acc_ -= fs_sym_path;
            if (soft_fill_ + 2 <= soft_buf_.size()) {
                soft_buf_[soft_fill_++] = clamp8((int)(ir / 1024));
                soft_buf_[soft_fill_++] = clamp8((int)(qsym / 1024));
            }
        }
    }

    if (soft_fill_ >= soft_bytes_needed)
        try_decode_block();

    if ((++execute_count_ & 0x3f) == 0) {
        status_.soft_sym_count = (uint32_t)soft_fill_;
        status_.demod_lock = soft_fill_ > 8000 ? 1 : 0;
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
