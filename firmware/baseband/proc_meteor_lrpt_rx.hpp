/*
 * Copyright (C) 2026
 *
 * This file is part of PortaPack / Mayhem.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * SatDump parity references: `firmware/baseband/meteor_lrpt/SATDUMP_VENDOR.md`.
 */
#ifndef __PROC_METEOR_LRPT_RX_HPP__
#define __PROC_METEOR_LRPT_RX_HPP__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "rssi_thread.hpp"
#include "dsp_decimate.hpp"
#include "dsp_fir_taps.hpp"
#include "dsp_fir_taps_meteor_lrpt.hpp"
#include "message.hpp"
#include "stream_input.hpp"

#include "meteor_lrpt/meteor_bpsk_ccsds_deframer.hpp"
#include "meteor_lrpt/meteor_cc_decoder.hpp"
#include "meteor_lrpt/meteor_m2x_interleaved_pipeline.hpp"
#include "meteor_lrpt/meteor_nrzm.hpp"

#include <array>
#include <memory>
#include <cstdint>
#include <variant>

class NoopDecimMeteor {
   public:
    static constexpr int decimation_factor = 1;
    template <typename Buffer>
    Buffer execute(const Buffer& src, const Buffer&) {
        return {src.p, src.count, src.sampling_rate};
    }
};

class MultiDecimatorMeteor {
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

class MultiDecimatorMeteor16 {
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
        NoopDecimMeteor>
        decimator_{};
};

class MeteorLrptRx : public BasebandProcessor {
   public:
    static constexpr size_t soft_buf_cap = 16384;
    static constexpr size_t soft_bytes_needed = 16384;
    static constexpr size_t interleaved_soft_bytes = 8192;

    MeteorLrptRx();
    void execute(const buffer_c8_t& buffer) override;
    void on_message(const Message* const message) override;

   private:
    void update_params();
    void configure(const MeteorLrptRxConfigureMessage& message);
    void capture_config(const CaptureConfigMessage& message);
    void try_decode_block();
    void finish_interleaved_from_m0();
    bool ensure_interleaved_post();
    bool interleaved_ipc_ready() const;
    size_t soft_bytes_target() const;
    bool process_soft_to_frame(const int8_t* soft, std::array<uint8_t, 1024>& frame);
    bool post_fec_frame_checks(std::array<uint8_t, 1024>& frame);
    bool process_m2x_noninterleaved(const int8_t* soft, bool diff, std::array<uint8_t, 1024>& frame);
    static int8_t clamp8(int v);
    static int16_t clamp16(int32_t v);
    complex16_t dc_and_rrc(complex16_t s);
    static void soft_circular_shift(const int8_t* src, size_t len, size_t shift, int8_t* dst);

    static constexpr size_t baseband_fs = 3072000;
    static constexpr uint32_t fs_sym_path = 192000;

    bool configured{false};
    uint8_t flags_{0};
    uint32_t sym_rate_hz_{72000};

    std::array<complex16_t, 512> dst{};
    const buffer_c16_t dst_buffer{dst.data(), dst.size()};

    MultiDecimatorMeteor decim_0{};
    MultiDecimatorMeteor16 decim_1{};

    float dc_i_{0.0f};
    float dc_q_{0.0f};
    std::array<complex16_t, meteor_lrpt::sym_shaping_ntaps> rrc_hist_{};
    uint32_t rrc_w_{0};

    float agc_gain_{256.0f};
    /** Fractional symbol phase 0..1; Gardner adjusts `sym_rate_fine_` (small fractional rate tweak). */
    float sym_phase_f_{0};
    float sym_pll_i_{0};
    float sym_rate_fine_{0};
    float sym_last_ted_{0};
    /** Gardner TED taps (updated only on symbol strobes, cubic-interpolated I). */
    int32_t sym_ir_hist_[3]{};
    int32_t prev_q_{0};
    /** Last four scaled I/Q (÷1024 float) for Catmull–Rom fractional strobes (polyphase timing). */
    float sym_ci_hist_[4]{};
    float sym_cq_hist_[4]{};

    int8_t* soft_buf_{nullptr};
    int8_t* soft_rot_work_{nullptr};
    size_t soft_fill_{0};

    MeteorCcDecoder viterbi_{8192};
    MeteorNrzmByteDecoder nrzm_{};
    MeteorNrzmBitDecoder nrzm_bits_{};
    MeteorBpskCcsdsDeframer bpsk_deframer_{};
    /** Allocated on demand (~27 KiB); keeps M4 heap headroom at app launch. */
    std::unique_ptr<meteor_lrpt::M2xInterleavedPostDeintPipeline> interleaved_post_{};
    std::array<uint8_t, 8192> m2x_bits_{};
    std::array<uint8_t, 2048> deframer_batch_out_{};

    uint32_t execute_count_{0};
    /** `SharedMemory::MeteorLrptIpc::seq` for the active M0 deinterleave block (interleaved IPC handshake). */
    uint32_t m4_ipc_post_seq_{0};

    std::unique_ptr<StreamInput> cadu_stream_{};
    /* G2: int8 I/Q soft pairs; `CaptureConfig` write_size 16384 (legacy) or 8192 (M2-x interleaved). See SOFT_FORMAT.md. */
    std::unique_ptr<StreamInput> soft_stream_{};

    MeteorLrptRxStatusDataMessage status_{};
    MeteorLrptRxPreviewLineMessage preview_{};

    /* Threads start in ctor after soft buffers exist (see proc_meteor_lrpt_rx.cpp). */
    BasebandThread baseband_thread{baseband_fs, this, baseband::Direction::Receive, false, NORMALPRIO - 4};
    RSSIThread rssi_thread{false};
};


#endif /*__PROC_METEOR_LRPT_RX_HPP__*/
