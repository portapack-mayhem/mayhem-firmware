/*
 * Copyright (C) 2026
 *
 * Meteor LRPT capture-only M4: QPSK demod + Gardner timing → soft int8 I/Q stream.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef __PROC_METEOR_LRPT_CAPTURE_HPP__
#define __PROC_METEOR_LRPT_CAPTURE_HPP__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "rssi_thread.hpp"
#include "dsp_decimate.hpp"
#include "dsp_fir_taps.hpp"
#include "dsp_fir_taps_meteor_lrpt.hpp"
#include "message.hpp"
#include "stream_input.hpp"
#include "meteor_lrpt/meteor_symbol_timing.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <variant>

class NoopDecimMeteorCap {
   public:
    static constexpr int decimation_factor = 1;
    template <typename Buffer>
    Buffer execute(const Buffer& src, const Buffer&) {
        return {src.p, src.count, src.sampling_rate};
    }
};

class MultiDecimatorMeteorCap {
   public:
    template <typename Source, typename Destination>
    Destination execute(const Source& src, const Destination& dst) {
        return std::visit(
            [&src, &dst](auto&& arg) -> Destination {
                return arg.execute(src, dst);
            },
            decimator_);
    }

    template <typename Decimator>
    Decimator& set() {
        decimator_ = Decimator{};
        return std::get<Decimator>(decimator_);
    }

   private:
    std::variant<
        dsp::decimate::FIRC8xR16x24FS4Decim8,
        dsp::decimate::FIRC8xR16x24FS4Decim4>
        decimator_{};
};

class MultiDecimatorMeteor16Cap {
   public:
    template <typename Source, typename Destination>
    Destination execute(const Source& src, const Destination& dst) {
        return std::visit(
            [&src, &dst](auto&& arg) -> Destination {
                return arg.execute(src, dst);
            },
            decimator_);
    }

    template <typename Decimator>
    Decimator& set() {
        decimator_ = Decimator{};
        return std::get<Decimator>(decimator_);
    }

   private:
    std::variant<
        dsp::decimate::FIRC16xR16x16Decim2,
        dsp::decimate::FIRC16xR16x32Decim8,
        NoopDecimMeteorCap>
        decimator_{};
};

class MeteorLrptCapture : public BasebandProcessor {
   public:
    static constexpr size_t soft_buf_cap = 16384;
    static constexpr size_t soft_bytes_needed = 16384;

    MeteorLrptCapture();
    void execute(const buffer_c8_t& buffer) override;
    void on_message(const Message* const message) override;

   private:
    void configure(const MeteorLrptRxConfigureMessage& message);
    void capture_config(const CaptureConfigMessage& message);
    size_t soft_bytes_target() const;
    void flush_soft_block();
    static int8_t clamp8(int v);
    static int16_t clamp16(int32_t v);
    complex16_t dc_and_rrc(complex16_t s);

    static constexpr size_t baseband_fs = 3072000;
    static constexpr uint32_t fs_sym_path = 192000;

    bool configured{false};
    uint8_t flags_{0};
    uint32_t sym_rate_hz_{72000};

    std::array<complex16_t, 512> dst{};
    const buffer_c16_t dst_buffer{dst.data(), dst.size()};

    MultiDecimatorMeteorCap decim_0{};
    MultiDecimatorMeteor16Cap decim_1{};

    float dc_i_{0.0f};
    float dc_q_{0.0f};
    std::array<complex16_t, meteor_lrpt::sym_shaping_ntaps> rrc_hist_{};
    uint32_t rrc_w_{0};

    float agc_gain_{256.0f};
    float sym_phase_f_{0};
    float sym_pll_i_{0};
    float sym_rate_fine_{0};
    float sym_last_ted_{0};
    int32_t sym_ir_hist_[3]{};
    int32_t prev_q_{0};
    float sym_ci_hist_[4]{};
    float sym_cq_hist_[4]{};

    int8_t* soft_buf_{nullptr};
    size_t soft_fill_{0};

    uint32_t execute_count_{0};
    uint32_t soft_blocks_out_{0};

    std::unique_ptr<StreamInput> soft_stream_{};

    MeteorLrptRxStatusDataMessage status_{};

    BasebandThread baseband_thread{baseband_fs, this, baseband::Direction::Receive, false, NORMALPRIO - 4};
    RSSIThread rssi_thread{false};
};

#endif /* __PROC_METEOR_LRPT_CAPTURE_HPP__ */
