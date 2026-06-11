/*
 * Copyright (C) 2026
 *
 * Meteor LRPT capture-only M4 processor (PMLS).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "proc_meteor_lrpt_capture.hpp"

#include "meteor_lrpt_m0_configure_ipc.hpp"
#include "debug.hpp"
#include "hal.h"
#include "ch.h"
#include "audio_dma.hpp"
#include "portapack_shared_memory.hpp"
#include "event_m4.hpp"
#include "dsp_types.hpp"

#include "meteor_lrpt/meteor_polyphase_interp.hpp"
#include "meteor_lrpt/meteor_symbol_timing.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace {
constexpr meteor_lrpt::MeteorLrptSymbolTimingGains kSymTimingGains{};
std::array<int8_t, MeteorLrptCapture::soft_buf_cap> g_capture_soft_buf{};
}  // namespace

MeteorLrptCapture::MeteorLrptCapture()
    : soft_buf_{g_capture_soft_buf.data()} {
    baseband_thread.start();
    rssi_thread.start();
}

int8_t MeteorLrptCapture::clamp8(int v) {
    if (v > 127)
        return 127;
    if (v < -128)
        return -128;
    return static_cast<int8_t>(v);
}

int16_t MeteorLrptCapture::clamp16(int32_t v) {
    if (v > 32767)
        return 32767;
    if (v < -32768)
        return -32768;
    return static_cast<int16_t>(v);
}

complex16_t MeteorLrptCapture::dc_and_rrc(complex16_t s) {
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

void MeteorLrptCapture::configure(const MeteorLrptRxConfigureMessage& message) {
    flags_ = static_cast<uint8_t>(message.flags & ~0x02u);
    dc_i_ = dc_q_ = 0.0f;
    rrc_hist_.fill({});
    rrc_w_ = 0;
    sym_phase_f_ = sym_pll_i_ = sym_rate_fine_ = sym_last_ted_ = 0.f;
    sym_ir_hist_[0] = sym_ir_hist_[1] = sym_ir_hist_[2] = 0;
    for (float& v : sym_ci_hist_)
        v = 0.f;
    for (float& v : sym_cq_hist_)
        v = 0.f;
    soft_fill_ = 0;

    uint32_t sk = message.symbol_rate_k;
    if (sk < 50 || sk > 120)
        sk = 72;
    sym_rate_hz_ = sk * 1000U;

    decim_0.set<dsp::decimate::FIRC8xR16x24FS4Decim8>().configure(taps_200k_decim_0.taps);
    decim_1.set<dsp::decimate::FIRC16xR16x16Decim2>().configure(taps_200k_decim_1.taps);

    configured = true;

    shared_memory.meteor_lrpt_rx_m0_command.ack = 1;
    __DMB();
    shared_memory.baseband_message = nullptr;
    __DMB();
}

void MeteorLrptCapture::capture_config(const CaptureConfigMessage& message) {
    if (!message.config) {
        soft_stream_.reset();
        return;
    }

    constexpr size_t kSoft16384 = 16384;
    constexpr size_t kSoft8192 = 8192;
    if (message.config->write_size == kSoft16384 || message.config->write_size == kSoft8192) {
        soft_stream_ = std::make_unique<StreamInput>(message.config);
    } else {
        soft_stream_.reset();
    }
}

size_t MeteorLrptCapture::soft_bytes_target() const {
    const bool m2x = (flags_ & 1u) != 0;
    if (m2x)
        return 8192;
    return soft_bytes_needed;
}

void MeteorLrptCapture::flush_soft_block() {
    if (!soft_stream_ || !soft_buf_)
        return;
    (void)soft_stream_->write(soft_buf_, soft_bytes_target());
    soft_blocks_out_++;
    soft_fill_ = 0;
}

void MeteorLrptCapture::execute(const buffer_c8_t& buffer) {
    meteor_lrpt_rx_poll_m0_configure(this);
    if (!configured)
        return;

    ++execute_count_;

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
            const float ted = (float)sym_ir_hist_[1] * ((float)sym_ir_hist_[0] - (float)sym_ir_hist_[2]) *
                              kSymTimingGains.ted_scale;
            sym_last_ted_ = ted;
            sym_pll_i_ += kSymTimingGains.pll_i_gain * ted;
            float adj = kSymTimingGains.rate_prop_gain * ted + sym_pll_i_;
            if (adj > kSymTimingGains.rate_clamp)
                adj = kSymTimingGains.rate_clamp;
            if (adj < -kSymTimingGains.rate_clamp)
                adj = -kSymTimingGains.rate_clamp;
            sym_rate_fine_ = adj;
            if (soft_fill_ + 2 <= soft_buf_cap) {
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
        flush_soft_block();

    if ((execute_count_ & 0x1ff) == 0) {
        status_.soft_sym_count = (uint32_t)soft_fill_;
        status_.demod_lock = soft_fill_ > 8000 ? 1 : 0;
        status_.sym_timing_err = (int16_t)std::max(-32767, std::min(32767, (int)(sym_last_ted_ * 20000.f)));
        status_.sym_timing_lock = (uint8_t)((sym_rate_fine_ > -kSymTimingGains.rate_clamp * 0.3f &&
                                             sym_rate_fine_ < kSymTimingGains.rate_clamp * 0.3f && soft_fill_ > 4000)
                                                ? 1
                                                : 0);
        status_.cadu_frames = soft_blocks_out_;
        shared_memory.application_queue.push(status_);
    }
}

void MeteorLrptCapture::on_message(const Message* const message) {
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
        case Message::ID::SampleRateConfig:
            break;
        default:
            break;
    }
}

int main() {
    audio::dma::init_audio_out();
    meteor_lrpt_rx_reset_m0_configure_state();
    auto processor = std::make_unique<MeteorLrptCapture>();
    if (!processor) {
        write_m4_panic_msg("Meteor cap OOM", nullptr);
        while (true) {
        }
    }
    EventDispatcher event_dispatcher{std::move(processor)};
    event_dispatcher.run();
    return 0;
}
