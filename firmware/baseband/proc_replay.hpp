/*
 * Copyright (C) 2016 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
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

#ifndef __PROC_REPLAY_HPP__
#define __PROC_REPLAY_HPP__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"

#include "spectrum_collector.hpp"

#include "stream_output.hpp"

#include <array>
#include <memory>

class ReplayProcessor : public BasebandProcessor {
   public:
    ReplayProcessor();

    void execute(const buffer_c8_t& buffer) override;
    void on_message(const Message* const message) override;

   private:
    size_t baseband_fs = 3072000;
    static constexpr auto spectrum_rate_hz = 50.0f;

    // Holds the read IQ data chunk from the file to send.
    std::array<complex16_t, 512> iq{};

    int32_t channel_filter_low_f = 0;
    int32_t channel_filter_high_f = 0;
    int32_t channel_filter_transition = 0;

    std::unique_ptr<StreamOutput> stream{};

    SpectrumCollector channel_spectrum{};
    size_t spectrum_interval_samples = 0;
    size_t spectrum_samples = 0;

    bool configured{false};
    uint32_t bytes_read{0};
    OversampleRate oversample_rate = OversampleRate::x8;

    void sample_rate_config(const SampleRateConfigMessage& message);
    void replay_config(const ReplayConfigMessage& message);

    TXProgressMessage txprogress_message{};
    RequestSignalMessage sig_message{RequestSignalMessage::Signal::FillRequest};

    /* NB: Threads should be the last members in the class definition. */
    BasebandThread baseband_thread{
        baseband_fs, this, baseband::Direction::Transmit, /*auto_start*/ false};
};

#endif /*__PROC_REPLAY_HPP__*/
