/*
 * Copyright (C) 2026 justin080
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

#ifndef __PROC_SEEWETTER_H__
#define __PROC_SEEWETTER_H__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "rssi_thread.hpp"
#include "message.hpp"
#include "dsp_decimate.hpp"
#include "audio_output.hpp"
#include "seewetter_dsp.hpp"
#include "seewetter_protocol.hpp"

#include <atomic>

/* DWD maritime weather RTTY (F1B, 50 Bd, 450 Hz shift) receiver.
 * 3.072 MHz -> 384 kHz -> 48 kHz -> 24 kHz complex, then seewetter::Demod. */
class SeewetterProcessor : public BasebandProcessor {
   public:
    SeewetterProcessor();

    void execute(const buffer_c8_t& buffer) override;
    void on_message(const Message* const message) override;

   private:
    static constexpr size_t baseband_fs = 3072000;
    static_assert(seewetter::Demod::SPEC_BINS == seewetter::SPECTRUM_BINS, "spectrum size mismatch");

    std::array<complex16_t, 512> dst{};
    const buffer_c16_t dst_buffer{
        dst.data(),
        dst.size()};

    std::array<float, 32> audio{};

    dsp::decimate::FIRC8xR16x24FS4Decim8 decim_0{};
    dsp::decimate::FIRC16xR16x32Decim8 decim_1{};
    dsp::decimate::FIRAndDecimateComplex channel_filter{};

    AudioOutput audio_output{};

    seewetter::Demod demod{};

    /* Config arrives on the event thread, it is applied by the baseband thread. */
    struct Config {
        uint16_t baud_x100;
        uint16_t shift;
        uint8_t polarity;
        uint8_t afc;
        uint8_t squelch;
    };
    Config pending_config{5000, 450, 0, 1, 2};
    std::atomic<bool> config_pending{false};
    bool configured{false};

    RTTYDataMessage text_message{};
    RTTYDataMessage status_message{};

    void send_text();
    void send_status();

    /* NB: Threads should be the last members in the class definition. */
    BasebandThread baseband_thread{baseband_fs, this, baseband::Direction::Receive};
    RSSIThread rssi_thread{};
};

#endif /*__PROC_SEEWETTER_H__*/
