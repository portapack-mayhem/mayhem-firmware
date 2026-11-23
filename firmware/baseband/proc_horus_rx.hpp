/*
 * Copyright (C) 2026 OE1LBS
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

#ifndef __PROC_HORUS_RX_H__
#define __PROC_HORUS_RX_H__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "rssi_thread.hpp"
#include "message.hpp"
#include "dsp_decimate.hpp"
#include "dsp_demodulate.hpp"
#include "audio_output.hpp"

#include "horus/horus_demod.hpp"
#include "horus/horus_frame.hpp"

class HorusRxProcessor : public BasebandProcessor {
   public:
    HorusRxProcessor();

    void execute(const buffer_c8_t& buffer) override;
    void on_message(const Message* const message) override;

   private:
    static constexpr size_t baseband_fs = 3072000;

    // 3.072 MHz -> 384 kHz -> 48 kHz -> 24 kHz (the FT8/AFSK chain, +-5.5 kHz channel),
    // then 24 -> 12 kHz by pair averaging for the demodulator.
    static constexpr size_t channel_fs = 24000;
    static constexpr uint32_t status_interval = horus::kFs / 2;  // two status updates a second

    std::array<complex16_t, 512> dst{};
    const buffer_c16_t dst_buffer{dst.data(), dst.size()};

    std::array<float, 32> audio{};
    const buffer_f32_t audio_buffer{audio.data(), audio.size()};

    dsp::decimate::FIRC8xR16x24FS4Decim8 decim_0{};
    dsp::decimate::FIRC16xR16x32Decim8 decim_1{};
    dsp::decimate::FIRAndDecimateComplex channel_filter{};
    dsp::demodulate::SSB demod_ssb{};
    AudioOutput audio_output{};

    horus::Demod4FSK demod{};

    /* The I/Q orientation of the channel depends on the radio front end, and USB audio
     * sounds the same either way, so both tone orders are decoded and whichever one
     * passes the CRC wins. A mirrored spectrum maps symbol s to 3 - s. */
    horus::Framer framer{};
    horus::Framer framer_mirrored{};
    bool mirrored{false};  // last packet came from the mirrored decoder

    bool configured{false};
    bool pair_phase{false};
    horus::cf32 pair_acc{0.0f, 0.0f};
    uint32_t status_countdown{status_interval};

    void process_channel(const buffer_c16_t& channel);
    void send_packet(const horus::Packet& p);
    void send_status();

    /* Last, so everything above is constructed before the threads start calling execute(). */
    BasebandThread baseband_thread{baseband_fs, this, baseband::Direction::Receive};
    RSSIThread rssi_thread{};
};

#endif /*__PROC_HORUS_RX_H__*/
