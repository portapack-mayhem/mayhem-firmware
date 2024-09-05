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

#include "event_m4.hpp"

ACARSProcessor::ACARSProcessor() {
    decim_0.configure(taps_11k0_decim_0.taps);
    decim_1.configure(taps_11k0_decim_1.taps);
    decode_data = 0;
    decode_count_bit = 0;
    baseband_thread.start();
}

void ACARSProcessor::execute(const buffer_c8_t& buffer) {
    /* 2.4576MHz, 2048 samples */

    const auto decim_0_out = decim_0.execute(buffer, dst_buffer);
    const auto decim_1_out = decim_1.execute(decim_0_out, dst_buffer);
    const auto decimator_out = decim_1_out;

    /* 38.4kHz, 32 samples */
    feed_channel_stats(decimator_out);

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
    crc ^= (static_cast<uint16_t>(dataByte) << 8);  // XOR the byte with the CRC's upper byte

    for (int bit = 0; bit < 8; ++bit) {         // Process each bit
        if (crc & 0x8000) {                     // If the uppermost bit is set
            crc = (crc << 1) ^ CRC_POLYNOMIAL;  // Shift left and XOR with the polynomial
        } else {
            crc <<= 1;  // Just shift left if the uppermost bit is not set
        }
    }

    return crc;  // Return the current CRC value
}

void ACARSProcessor::consume_symbol(const float raw_symbol) {
    const uint_fast8_t sliced_symbol = (raw_symbol >= 0.0f) ? 1 : 0;
    // const auto decoded_symbol = acars_decode(sliced_symbol);

    // acars can have max 220 characters.
    //  starts with preampble  0x7E (n number of times)
    // it should followed by message start 0x02, then comes the ascii 7 bit characters (sent with 8 bits!) and the closing 0x03. after it there must be a crc
    // ACARS uses the CRC-16-CCITT (polynomial 0x1021) for error detection. This 16-bit CRC is computed over the message content, excluding the preamble and CRC field itself
    add_bit(sliced_symbol);
    if (curr_state == ACARS_STATE_RESET && decode_count_bit == 8) {
        // this should be the 0x7e
        if ((decode_data & 0xff) == 0x7e) {
            curr_state = ACARS_STATE_PRE;
        } else {
            decode_count_bit -= 1;  // just drop the first bit
        }
        return;
    }
    if (curr_state == ACARS_STATE_PRE && decode_count_bit == 16) {
        if ((decode_data & 0xffff) == 0x7e7e) {
            // multi preamble, drop first
            decode_data = 0x7e;
            decode_count_bit = 8;
            return;
        }
        if ((decode_data & 0xffff) == 0x7e02) {
            // data start found! now comes the real data content until 0x03
            curr_state = ACARS_STATE_MSGSTARTED;
            decode_count_bit = 0;
            decode_data = 0;
            message.msg_len = 0;
            memset(message.message, 0, 250);
            crc = CRC_INITIAL;
            return;
        }
        // here i don't have the right packets. so reset
        decode_count_bit = 0;
        decode_data = 0;
        curr_state = ACARS_STATE_RESET;
    }
    if (curr_state == ACARS_STATE_MSGSTARTED && decode_count_bit == 8) {
        // got a character
        if ((decode_data & 0xff) == 0x03) {
            // message end. should wait for crc, check.
            curr_state = ACARS_STATE_MSGENDED;
            decode_count_bit = 0;
            decode_data = 0;
            return;
        }
        update_crc(decode_data & 0x7f);
        message.message[message.msg_len++] = decode_data & 0x7f;
        decode_data = 0;
        decode_count_bit = 0;

        if (message.msg_len >= 249) {
            message.msg_len = 0;
            memset(message.message, 0, 250);  // todo handle any other way
            crc = CRC_INITIAL;
            curr_state = ACARS_STATE_RESET;
        }
        return;
    }
    if (curr_state == ACARS_STATE_MSGENDED && decode_count_bit == 16) {
        // got the crc data. check it against the calculated one
        if (crc == decode_data || true) {  // todo
            // send message to app core.
            payload_handler();
        }
        curr_state = ACARS_STATE_RESET;
        decode_count_bit = 0;
        decode_data = 0;
        crc = CRC_INITIAL;
        return;
    }
}

void ACARSProcessor::payload_handler() {
    shared_memory.application_queue.push(message);
}

int main() {
    EventDispatcher event_dispatcher{std::make_unique<ACARSProcessor>()};
    event_dispatcher.run();
    return 0;
}
