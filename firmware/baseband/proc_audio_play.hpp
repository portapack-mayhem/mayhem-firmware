/*
 * Copyright (C) 2026 SecLBL
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

#ifndef __PROC_AUDIO_PLAY_H__
#define __PROC_AUDIO_PLAY_H__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "dsp_iir.hpp"
#include "stream_output.hpp"
#include "message.hpp"

#include <memory>

/* Plays a 48 kHz, 16 bit, mono or stereo sample stream on the audio codec, through a
 * 5 band equalizer. Nothing is transmitted. */
class AudioPlayProcessor : public BasebandProcessor {
   public:
    void execute(const buffer_c8_t& buffer) override;
    void on_message(const Message* const message) override;

   private:
    // The baseband stream is only used as a clock: one buffer of 2048 samples at
    // 3.072 MHz is exactly one 32 sample audio block at 48 kHz, the same cadence the
    // receivers feed the codec with.
    static constexpr size_t baseband_fs = 3072000;
    static constexpr size_t audio_block = 32;
    static constexpr size_t eq_bands = AudioPlayConfigMessage::eq_bands;

    std::unique_ptr<StreamOutput> stream{};

    uint8_t channels{1};
    bool configured{false};

    IIRBiquadFilter eq[2][eq_bands]{};  // [channel][band]
    bool eq_flat{true};                 // all bands at 0 dB: samples pass through untouched
    float preamp{1.0f};                 // brings the peak of the whole EQ curve down to 0 dB

    // A new setting is worked out by the message thread and taken over by execute()
    // between two blocks, so no block is filtered with half of it.
    struct Setting {
        iir_biquad_config_t band[eq_bands];
        float preamp;
        uint8_t channels;
        bool flat;
    };
    Setting wanted{};
    volatile bool wanted_ready{false};

    void prepare(const AudioPlayConfigMessage& message);

    RequestSignalMessage sig_message{RequestSignalMessage::Signal::FillRequest};

    /* NB: Threads should be the last members in the class definition. */
    BasebandThread baseband_thread{baseband_fs, this, baseband::Direction::Transmit};
};

#endif /*__PROC_AUDIO_PLAY_H__*/
