/*
 * Copyright (C) 2016 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2018 Furrtek
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

#ifndef __PROC_CAPTURE_HPP__
#define __PROC_CAPTURE_HPP__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "rssi_thread.hpp"

#include "dsp_decimate.hpp"
#include "spectrum_collector.hpp"
#include "stream_input.hpp"
#include "message.hpp"

#include <array>
#include <memory>
#include <tuple>
#include <variant>

/* A decimator that just returns the source buffer. */
class NoopDecim {
   public:
    static constexpr int decimation_factor = 1;

    template <typename Buffer>
    Buffer execute(const Buffer& src, const Buffer&) {
        // TODO: should this copy to 'dst'?
        return {src.p, src.count, src.sampling_rate};
    }
};

/* Decimator wrapper that can hold one of a set of decimators and dispatch at runtime. */
template <typename... Args>
class MultiDecimator {
   public:
    /* Dispatches to the underlying type's execute. */
    template <typename Source, typename Destination>
    Destination execute(
        const Source& src,
        const Destination& dst) {
        return std::visit(
            [&src, &dst](auto&& arg) -> Destination {
                return arg.execute(src, dst);
            },
            decimator_);
    }

    size_t decimation_factor() const {
        return std::visit(
            [](auto&& arg) -> size_t {
                return arg.decimation_factor;
            },
            decimator_);
    }

    /* Sets this decimator to a new instance of the specified decimator type.
     * NB: The instance is returned by-ref so 'configure' can easily be called. */
    template <typename Decimator>
    Decimator& set() {
        decimator_ = Decimator{};
        return std::get<Decimator>(decimator_);
    }

   private:
    std::variant<Args...> decimator_{};
};

class CaptureProcessor : public BasebandProcessor {
   public:
    CaptureProcessor();

    void execute(const buffer_c8_t& buffer) override;
    void on_message(const Message* const message) override;

   private:
    void on_signal_message(const RequestSignalMessage& message);
    void on_beep_message(const AudioBeepMessage& message);

    size_t baseband_fs = 3072000;  // aka: sample_rate
    static constexpr auto spectrum_rate_hz = 50.0f;

    std::array<complex16_t, 512> dst{};
    const buffer_c16_t dst_buffer{
        dst.data(),
        dst.size()};

    /* The actual type will be configured depending on the sample rate. */
    MultiDecimator<
        dsp::decimate::FIRC8xR16x24FS4Decim4,
        dsp::decimate::FIRC8xR16x24FS4Decim8>
        decim_0{};
    MultiDecimator<
        dsp::decimate::FIRC16xR16x16Decim2,
        dsp::decimate::FIRC16xR16x32Decim8,
        NoopDecim>
        decim_1{};

    int32_t channel_filter_low_f = 0;
    int32_t channel_filter_high_f = 0;
    int32_t channel_filter_transition = 0;

    std::unique_ptr<StreamInput> stream{};

    SpectrumCollector channel_spectrum{};
    size_t spectrum_interval_samples = 0;
    size_t spectrum_samples = 0;

    /* NB: Threads should be the last members in the class definition. */
    BasebandThread baseband_thread{
        baseband_fs, this, baseband::Direction::Receive, /*auto_start*/ false};
    RSSIThread rssi_thread{};

    void sample_rate_config(const SampleRateConfigMessage& message);
    void capture_config(const CaptureConfigMessage& message);
};

#endif /*__PROC_CAPTURE_HPP__*/
