/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __PROC_AUDIOTX_H__
#define __PROC_AUDIOTX_H__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "tone_gen.hpp"
#include "stream_output.hpp"
#include "audio_output.hpp"
#include "audio_dma.hpp"

#define AUDIO_OUTPUT_BUFFER_SIZE 32

class AudioTXProcessor : public BasebandProcessor {
   public:
    void execute(const buffer_c8_t& buffer) override;

    void on_message(const Message* const msg) override;

   private:
    static constexpr size_t baseband_fs = 1536000;

    std::unique_ptr<StreamOutput> stream{};

    ToneGen tone_gen{};

    uint32_t resample_inc{}, resample_acc{};
    uint32_t fm_delta{0};
    uint32_t phase{0}, sphase{0};
    uint32_t audio_sample{};
    int32_t sample{0}, delta{};
    int8_t re{0}, im{0};
    uint8_t bytes_per_sample{1};
    uint32_t sampling_rate{48000};

    int16_t audio_data[AUDIO_OUTPUT_BUFFER_SIZE];
    AudioOutput audio_output{};

    size_t progress_interval_samples = 0, progress_samples = 0;

    bool configured{false};
    uint32_t samples_read{0};
    bool tone_key_enabled{false};

    void sample_rate_config(const SampleRateConfigMessage& message);
    void audio_config(const AudioTXConfigMessage& message);
    void replay_config(const ReplayConfigMessage& message);

    TXProgressMessage txprogress_message{};
    RequestSignalMessage sig_message{RequestSignalMessage::Signal::FillRequest};

    /* NB: Threads should be the last members in the class definition. */
    BasebandThread baseband_thread{baseband_fs, this, baseband::Direction::Transmit};
};

#endif
