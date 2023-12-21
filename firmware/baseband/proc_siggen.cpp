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

#include "proc_siggen.hpp"
#include "portapack_shared_memory.hpp"
#include "sine_table_int8.hpp"
#include "event_m4.hpp"

#include <cstdint>

void SigGenProcessor::execute(const buffer_c8_t& buffer) {
    if (!configured) return;

    for (size_t i = 0; i < buffer.count; i++) {
        if (!sample_count && auto_off) {
            configured = false;
            txprogress_message.done = true;
            shared_memory.application_queue.push(txprogress_message);
        } else
            sample_count--;

        if (tone_shape == 0) {
            // CW
            re = 127;  // max. signed 8 bits value .   (-128 ...+127), max. amplitude ,  static phasor at 0º
            im = 0;
        } else {
            if (tone_shape == 1) {
                // Sine
                sample = (sine_table_i8[(tone_phase & 0xFF000000) >> 24]);
            } else if (tone_shape == 2) {
                // Triangle
                int8_t a = (tone_phase & 0xFF000000) >> 24;
                sample = (a & 0x80) ? ((a << 1) ^ 0xFF) - 0x80 : (a << 1) + 0x80;
            } else if (tone_shape == 3) {
                // Saw up
                sample = ((tone_phase & 0xFF000000) >> 24);
            } else if (tone_shape == 4) {
                // Saw down
                sample = ((tone_phase & 0xFF000000) >> 24) ^ 0xFF;
            } else if (tone_shape == 5) {
                // Square
                sample = (((tone_phase & 0xFF000000) >> 24) & 0x80) ? 127 : -128;
            } else if (tone_shape == 6) {
                // Noise generator, pseudo random noise generator, 16 bits linear-feedback shift register (LFSR) algorithm, variant Fibonacci.
                // https://en.wikipedia.org/wiki/Linear-feedback_shift_register
                // 16 bits LFSR .taps: 16, 15, 13, 4 ;feedback polynomial: x^16 + x^15 + x^13 + x^4 + 1
                // Periode 65535= 2^n-1, quite continuous .
                if (counter == 0) {  // we slow down the shift register, because the pseudo random noise clock freq was too high for modulator.
                    bit_16 = ((lfsr_16 >> 0) ^ (lfsr_16 >> 1) ^ (lfsr_16 >> 3) ^ (lfsr_16 >> 4) ^ ((lfsr_16 >> 12) & 1));
                    lfsr_16 = (lfsr_16 >> 1) | (bit_16 << 15);
                    sample = (lfsr_16 & 0x00FF);  // main pseudo random noise generator.
                }
                if (counter == 5) {                                  // after many empiric test, that combination mix of >>4 and >>5, gives a reasonable trade off white noise / good rf power level .
                    sample = ((lfsr_16 & 0b0000111111110000) >> 4);  // just changing the spectrum shape .
                }
                if (counter == 10) {
                    sample = ((lfsr_16 & 0b0001111111100000) >> 5);  // just changing the spectrum shape .
                }
                counter++;
                if (counter == 15) {
                    counter = 0;
                }
            } else if (tone_shape == 7) {
                // Digital BPSK consecutive 0,1,0,...continuous cycle, 1 bit/symbol, at rate of 2 symbols / Freq Tone Periode... without any Pulse shape at the moment .
                re = (((tone_phase & 0xFF000000) >> 24) & 0x80) ? 127 : -128;  // Sending 2 bits by Periode T of the GUI tone, alternative static phasor to 0, -180º , 0º
                im = 0;
            } else if (tone_shape == 8) {
                // Digital QPSK  consecutive 00, 01, 10, 11,00, ...continuous cycle ,2 bits/symbol, at rate of 4 symbols / Freq Tone Periode. not random., without any Pulse shape at the moment .

                switch (((tone_phase & 0xFF000000) >> 24)) {
                    case 0 ... 63:  // equivalent to 1/4 of total 360º degrees.
                        /* "00" */
                        re = (sine_table_i8[32]);       // we are sending symbol-phasor 45º during  1/4 of the total periode
                        im = (sine_table_i8[32 + 64]);  // 32 index  = rounded (45º/360º * 255 total sin table steps) = 31,875
                        break;

                    case 64 ... 127:
                        /* "01" */
                        re = (sine_table_i8[96]);       // symbol-phasor 135º
                        im = (sine_table_i8[96 + 64]);  // 96 index   = 32 + 256/4
                        break;
                        break;

                    case 128 ... 191:
                        /* "10" */
                        re = (sine_table_i8[159]);       // symbol-phasor 225º
                        im = (sine_table_i8[159 + 64]);  // 159 rounded index = 96 + 256/4 = 159.3
                        break;

                    case 192 ... 255:
                        /* "11" */
                        re = (sine_table_i8[223]);                  // symbol-phasor 315º ; 223 rounded index = (315/360) * 255 =223.125
                        im = (sine_table_i8[((223 + 64) & 0xFF)]);  // Max index 255, circular periodic conversion.
                        break;

                    default:
                        break;
                }
            }

            if (tone_shape != 6) {         //(all except Pseudo Random White Noise). We are in (1):periodic signals or (2):BPSK/QPSK , in both cases ,we  need Tone updated acum sum phases to modulate in FM / or control phasor phase (BPSK & QPSK.)
                tone_phase += tone_delta;  // In periodic signals(Sine/triangle/square) we are using to FM mod. in BPSK-QSPK we are using to calculate each 1/4 of the periode.
            }

            if (tone_shape < 7) {  // All Option shape signals  except BPSK(7) & QPSK(8) we are modulating in FM. (Those two has phase shift modulation XPSK , not FM  )
                // Do FM modulation
                delta = sample * fm_delta;

                phase += delta;
                sphase = phase + (64 << 24);

                re = (sine_table_i8[(sphase & 0xFF000000) >> 24]);  // sin LUT is not dealing with decimals , output range [-128 ,...127]
                im = (sine_table_i8[(phase & 0xFF000000) >> 24]);
            }
        }

        buffer.p[i] = {re, im};
    }
};

void SigGenProcessor::on_message(const Message* const msg) {
    const auto message = *reinterpret_cast<const SigGenConfigMessage*>(msg);

    switch (msg->id) {
        case Message::ID::SigGenConfig:
            if (!message.bw) {
                configured = false;
                return;
            }

            if (message.duration) {
                sample_count = message.duration;
                auto_off = true;
            } else
                auto_off = false;

            fm_delta = message.bw * (0xFFFFFFULL / 1536000);
            tone_shape = message.shape;

            // lfsr = seed_value ;  		// Finally not used , init lfsr 8 bits.
            lfsr_16 = seed_value_16;  // init lfsr 16 bits.

            configured = true;
            break;

        case Message::ID::SigGenTone:
            tone_delta = reinterpret_cast<const SigGenToneMessage*>(msg)->tone_delta;
            break;

        default:
            break;
    }
}

int main() {
    EventDispatcher event_dispatcher{std::make_unique<SigGenProcessor>()};
    event_dispatcher.run();
    return 0;
}
