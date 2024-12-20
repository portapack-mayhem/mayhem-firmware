/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
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

#include "portapack_shared_memory.hpp"
#include "proc_acars.hpp"
#include "dsp_fir_taps.hpp"
#include "audio_dma.hpp"

#include "event_m4.hpp"

#define SYN 0x16
#define SOH 0x01
#define STX 0x02
#define ETX 0x83
#define ETB 0x97
#define DLE 0x7f

ACARSProcessor::ACARSProcessor() {
    audio::dma::init_audio_out();
    decim_0.configure(taps_11k0_decim_0.taps);
    decim_1.configure(taps_11k0_decim_1.taps);
    decode_data = 0;
    decode_count_bit = 0;
    audio_output.configure(false);
    baseband_thread.start();
}

void ACARSProcessor::execute(const buffer_c8_t& buffer) {
    /* 2.4576MHz, 2048 samples */

    const auto decim_0_out = decim_0.execute(buffer, dst_buffer);
    const auto decim_1_out = decim_1.execute(decim_0_out, dst_buffer);
    const auto decimator_out = decim_1_out;

    /* 38.4kHz, 32 samples */
    feed_channel_stats(decimator_out);

    auto audio = demod.execute(decimator_out, audio_buffer);
    audio_output.write(audio);

    for (size_t i = 0; i < decimator_out.count; i++) {
        if (mf.execute_once(decimator_out.p[i])) {
            clock_recovery(mf.get_output());
        }
    }
}

void ACARSProcessor::add_bit(uint8_t bit) {
    decode_data = decode_data << 1 | bit;
    decode_count_bit++;
}

uint16_t ACARSProcessor::update_crc(uint8_t dataByte) {
    (void)dataByte;
    return 0;
}

void ACARSProcessor::sendDebug() {
    // if (curr_state <= 1) return;
    message.state = curr_state;
    shared_memory.application_queue.push(message);
}

void ACARSProcessor::reset() {
    decode_data = 0;
    decode_count_bit = 0;
    curr_state = WSYN;
    message.msg_len = 0;
    memset(message.message, 0, 250);
    message.crc[0] = 0;
    message.crc[1] = 0;
    parity_errors = 0;
}

void ACARSProcessor::consume_symbol(const float raw_symbol) {
    const uint_fast8_t sliced_symbol = (raw_symbol >= 0.0f) ? 1 : 0;

    add_bit(sliced_symbol);
    if (curr_state == WSYN && decode_count_bit == 8) {
        if ((decode_data & 0xff) == SYN) {
            curr_state = SYN2;
            decode_data = 0;
            decode_count_bit = 0;
        } else {
            decode_count_bit -= 1;  // just drop the first bit
        }
        return;
    }
    if (curr_state == SYN2 && decode_count_bit == 8) {
        if ((decode_data & 0xff) == SYN) {
            curr_state = SOH1;
            decode_data = 0;
            decode_count_bit = 0;
            sendDebug();
            return;
        }
        // here i don't have the right packets. so reset
        reset();
    }
    if (curr_state == SOH1 && decode_count_bit == 8) {
        if ((decode_data & 0xff) == SOH) {
            reset();
            curr_state = TXT;
            sendDebug();
            return;
        }
        message.message[0] = (decode_data & 0xff);  // debug
        reset();
        sendDebug();
    }
    if (curr_state == TXT && decode_count_bit == 8) {
        uint8_t ch = (decode_data & 0xff);
        message.message[message.msg_len++] = ch;

        if (!ParityCheck::parity_check(ch)) {
            // parity error
            parity_errors++;
            if (parity_errors > 4) {
                reset();  // too many parity errors, skip packet
                sendDebug();
                return;
            }
        }

        if (ch == ETX || ch == ETB) {
            curr_state = CRC1;
            sendDebug();
            decode_data = 0;
            decode_count_bit = 0;
            return;
        }
        if (message.msg_len > 240) {
            reset();
            sendDebug();
        }
        if (message.msg_len > 20 && ch == DLE) {
            message.msg_len -= 3;
            message.crc[0] = message.message[message.msg_len];
            message.crc[1] = message.message[message.msg_len + 1];
            curr_state = CRC2;
            sendDebug();
            // to hack the path:
            decode_data = message.crc[1];
        } else {
            decode_count_bit = 0;
            decode_data = 0;
            return;
        }
    }
    if (curr_state == CRC1 && decode_count_bit == 8) {
        message.crc[0] = (decode_data & 0xff);
        curr_state = CRC2;
        decode_data = 0;
        decode_count_bit = 0;
        sendDebug();
    }

    if (curr_state == CRC2 && decode_count_bit == 8) {
        message.crc[1] = (decode_data & 0xff);
        // send it to app cpu, and it'll take care of the rest
        payload_handler();
        reset();
        curr_state = END;
        decode_data = 0;
        decode_count_bit = 0;
        sendDebug();
    }
    if (curr_state == END && decode_count_bit == 8) {
        reset();
        sendDebug();
    }
}

void ACARSProcessor::payload_handler() {
    message.state = 255;  // to indicate this is an actual payload, not a debug packet
    shared_memory.application_queue.push(message);
}

int main() {
    EventDispatcher event_dispatcher{std::make_unique<ACARSProcessor>()};
    event_dispatcher.run();
    return 0;
}