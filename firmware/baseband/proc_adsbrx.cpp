/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2017 Furrtek
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

// https://www.icao.int/SAM/Documents/2015-SEMAUTOM/Ses4%20Presentation%20CUBA_ADSB.pdf

#include "proc_adsbrx.hpp"
#include "portapack_shared_memory.hpp"
#include "sine_table_int8.hpp"
#include "event_m4.hpp"
#include "audio_dma.hpp"

#include <cstdint>
#include <cstddef>

using namespace adsb;

void ADSBRXProcessor::execute(const buffer_c8_t& buffer) {
    // This is called at 2M/2048 = 977Hz
    // Each sample is 500ns.
    // One bit is 2 samples == 1us.
    // Bit value is the transition between samples.
    // i.e. lo->hi == 0, hi->lo == 1

    if (!configured) return;

    uint8_t bit = 0;
    uint8_t byte = 0;

    for (size_t i = 0; i < buffer.count; i++) {
        // Compute sample's magnitude.
        int8_t re = buffer.p[i].real();
        int8_t im = buffer.p[i].imag();
        uint16_t mag = (re * re) + (im * im);

        if (decoding) {
            // 1 bit == 2 samples, transition defines bit value.
            if ((sample_count & 1) == 1) {
                if (bit_count >= msg_len) {
                    const ADSBFrameMessage message(frame, amp);
                    shared_memory.application_queue.push(message);
                    decoding = false;
                    bit = (prev_mag > mag) ? 1 : 0;
                } else {
                    bit = (prev_mag > mag) ? 1 : 0;
                }

                byte = bit | (byte << 1);
                bit_count++;

                // Every 8th bit...
                if ((bit_count & 0x7) == 0) {
                    // Store the byte.
                    frame.push_byte(byte);

                    // Perform additional check on the first byte.
                    if (bit_count == 8) {
                        // Abandon all frames that aren't DF17 or DF18 extended squitters.
                        uint8_t df = (byte >> 3);
                        if (df != 17 && df != 18) {
                            decoding = false;
                            bit = (prev_mag > mag) ? 1 : 0;
                            frame.clear();
                        }
                    }
                }
            }

            sample_count++;
        }

        // Continue looking for preamble, even if in a packet.
        // Switch if new preamble is higher magnitude.

        // Shift the preamble.
        for (uint8_t c = 0; c < ADSB_PREAMBLE_LENGTH; c++) {
            shifter[c] = shifter[c + 1];
        }
        shifter[ADSB_PREAMBLE_LENGTH] = mag;

        // First check of relations between the first 12 samples
        // representing a valid preamble. We don't even investigate
        // further if this simple test is not passed.
        // Preamble is 8us - or 16 samples.
        //    0123456789ABCDEF
        //    _-_-____-_-_____
        if (shifter[0] < shifter[1] &&
            shifter[1] > shifter[2] &&
            shifter[2] < shifter[3] &&
            shifter[3] > shifter[4] &&
            shifter[4] < shifter[1] &&
            shifter[5] < shifter[1] &&
            shifter[6] < shifter[1] &&
            shifter[7] < shifter[1] &&
            shifter[8] > shifter[9] &&
            shifter[9] < shifter[10] &&
            shifter[10] > shifter[11]) {
            // The samples between the two spikes must be < than the average
            // of the high spikes level. We don't test bits too near to
            // the high levels as signals can be out of phase so part of the
            // energy can be in the near samples.
            int32_t this_amp = (shifter[1] + shifter[3] + shifter[8] + shifter[10]);
            uint32_t high = this_amp / 9;  // TBD: Why 9?
            if (shifter[5] < high &&
                shifter[6] < high &&
                // Similarly samples in the range 11-13 must be low, as it is the
                // space between the preamble and real data. Again we don't test
                // bits too near to high levels, see above.
                shifter[12] < high &&
                shifter[13] < high &&
                shifter[14] < high) {
                if ((decoding == false) ||                     // New preamble
                    ((decoding == true) && (this_amp > amp)))  // Higher power than existing packet
                {
                    decoding = true;
                    amp = this_amp;
                    sample_count = 0;
                    bit_count = 0;
                    frame.clear();
                }
            }
        }

        // Store mag for next time.
        prev_mag = mag;
    }
}

void ADSBRXProcessor::on_message(const Message* const message) {
    switch (message->id) {
        case Message::ID::ADSBConfigure:
            bit_count = 0;
            sample_count = 0;
            decoding = false;
            configured = true;
            break;

        case Message::ID::AudioBeep:
            on_beep_message(*reinterpret_cast<const AudioBeepMessage*>(message));
            break;

        default:
            break;
    }
}

void ADSBRXProcessor::on_beep_message(const AudioBeepMessage& message) {
    audio::dma::beep_start(message.freq, message.sample_rate, message.duration_ms);
}

#ifndef _WIN32
int main() {
    audio::dma::init_audio_out();
    EventDispatcher event_dispatcher{std::make_unique<ADSBRXProcessor>()};
    event_dispatcher.run();
    return 0;
}
#endif
