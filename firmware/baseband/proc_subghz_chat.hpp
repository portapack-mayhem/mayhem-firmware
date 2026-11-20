/*
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

#ifndef __PROC_SUBGHZ_CHAT_H__
#define __PROC_SUBGHZ_CHAT_H__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "rssi_thread.hpp"

#include "dsp_decimate.hpp"
#include "dsp_demodulate.hpp"
#include "cc1101_decoder.hpp"

#include "message.hpp"

// Sub-GHz chat, receive side: the CC1101 packets of the Flipper Zero's chat (GFSK 9.99 kbaud).
// Sending goes through the FSK TX image.
class SubGhzChatProcessor : public BasebandProcessor {
   public:
    SubGhzChatProcessor();

    void execute(const buffer_c8_t& buffer) override;

   private:
    static constexpr size_t baseband_fs = 3072000;
    static constexpr size_t demod_fs = 192000;

    std::array<complex16_t, 512> dst{};
    const buffer_c16_t dst_buffer{dst.data(), dst.size()};
    std::array<complex16_t, 128> channel{};
    const buffer_c16_t channel_buffer{channel.data(), channel.size()};
    std::array<int16_t, 128> fm{};
    const buffer_s16_t fm_buffer{fm.data(), fm.size()};

    // 3072 -> 768 -> 384 kHz with the WFM filters, then the channel (flat to 55 kHz: the
    // signal's +/-30 kHz and a tuning error) down to 192 kHz.
    dsp::decimate::FIRC8xR16x24FS4Decim4 decim_0{};
    dsp::decimate::FIRC16xR16x16Decim2 decim_1{};
    dsp::decimate::FIRAndDecimateComplex channel_filter{};
    dsp::demodulate::FM demod{};

    cc1101::Decoder decoder{on_packet, this};
    SubGhzChatPacketMessage packet_message{};

    static void on_packet(void* context, const uint8_t* payload, size_t length);

    uint32_t noise_{0};
    size_t noise_index_{0};
    void stir_noise(const buffer_c8_t& buffer);

    /* NB: Threads should be the last members in the class definition. */
    // Started by the constructor once the filters are configured.
    BasebandThread baseband_thread{baseband_fs, this, baseband::Direction::Receive, false};
    RSSIThread rssi_thread{};
};

#endif /*__PROC_SUBGHZ_CHAT_H__*/
