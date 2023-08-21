/*
 * Copyright (C) 2023 Bernd Herzog
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

#include "proc_spectrum_painter.hpp"
#include "event_m4.hpp"
#include "dsp_fft.hpp"
#include "random.hpp"

#include <cstdint>
#include <memory>
#include <utility>

// TODO move to class members SpectrumPainterProcessor
std::unique_ptr<complex16_t[]> current_line_data;
std::unique_ptr<complex16_t[]> next_line_data;
uint32_t current_line_index = 0;
uint32_t current_line_width = 0;
int32_t current_bw = 0;

std::vector<uint8_t> fifo_data[1 << SpectrumPainterBufferConfigureResponseMessage::fifo_k]{};
SpectrumPainterFIFO fifo{fifo_data, SpectrumPainterBufferConfigureResponseMessage::fifo_k};

int max_val = 127;

// This is called at 3072000/2048 = 1500Hz
void SpectrumPainterProcessor::execute(const buffer_c8_t& buffer) {
    for (uint32_t i = 0; i < buffer.count; i++) {
        if (current_line_data) {
            auto data = current_line_data[(current_line_index++ * current_bw / 3072) % current_line_width];
            buffer.p[i] = {(int8_t)data.real(), (int8_t)data.imag()};
        } else
            buffer.p[i] = {0, 0};
    }

    // Move "next line" into "current line" if set.
    if (next_line_data) {
        current_line_data = std::move(next_line_data);
        next_line_data.reset();
    }
}

WORKING_AREA(thread_wa, 4096);

void SpectrumPainterProcessor::run() {
    int ui = 0;
    init_genrand(22267);

    while (true) {
        if (fifo.is_empty() == false && !next_line_data) {
            std::vector<uint8_t> data;
            fifo.out(data);

            auto picture_width = data.size();

            auto fft_width = picture_width * 2;
            auto qu = fft_width / 4;

            // TODO: can these be statically allocated?
            auto v = std::make_unique<complex16_t[]>(fft_width);
            auto tmp = std::make_unique<complex16_t[]>(fft_width);

            for (uint32_t fft_index = 0; fft_index < fft_width; fft_index++) {
                if (fft_index < qu) {
                } else if (fft_index < qu * 3) {
                    // TODO: Improve index handling
                    auto image_index = fft_index - qu;

                    auto bin_power = data[image_index];  // 0 to 255
                    auto bin_phase = genrand_int31();    // 0 to 255

                    // rotate by random angle
                    auto phase_cos = (sine_table_i8[((int)(bin_phase + 0x40)) & 0xFF]);  // -127 to 127
                    auto phase_sin = (sine_table_i8[((int)(bin_phase)) & 0xFF]);

                    auto real = (int16_t)((int16_t)phase_cos * bin_power / 255);  // -127 to 127
                    auto imag = (int16_t)((int16_t)phase_sin * bin_power / 255);  // -127 to 127

                    auto fftshift_index = 0;
                    if (fft_index < qu * 2)                   // first half (fft_index = qu; fft_index < qu*2)
                        fftshift_index = fft_index + 2 * qu;  // goes to back
                    else                                      // 2nd half (fft_index =  qu*2; fft_index < qu*3)
                        fftshift_index = fft_index - 2 * qu;  // goes to front

                    v[fftshift_index] = {real, imag};
                }
            }

            ifft<complex16_t>(v.get(), fft_width, tmp.get());

            // normalize
            int32_t maximum = 1;
            for (uint32_t i = 0; i < fft_width; i++) {
                if (v[i].real() > maximum)
                    maximum = v[i].real();
                if (v[i].real() < -maximum)
                    maximum = -v[i].real();
                if (v[i].imag() > maximum)
                    maximum = v[i].imag();
                if (v[i].imag() < -maximum)
                    maximum = -v[i].imag();
            }

            if (maximum == 1) {  // a black line
                for (uint32_t i = 0; i < fft_width; i++)
                    v[i] = {0, 0};
            } else {
                for (uint32_t i = 0; i < fft_width; i++) {
                    v[i] = {(int8_t)((int32_t)v[i].real() * 120 / maximum), (int8_t)((int32_t)v[i].imag() * 120 / maximum)};
                }
            }

            next_line_data = std::move(v);
            ui++;
        } else {
            chThdSleepMilliseconds(1);
        }
    }
}

void SpectrumPainterProcessor::on_message(const Message* const msg) {
    switch (msg->id) {
        case Message::ID::SpectrumPainterBufferRequestConfigure: {
            const auto message = *reinterpret_cast<const SpectrumPainterBufferConfigureRequestMessage*>(msg);
            current_line_width = message.width;
            current_bw = message.bw / 500;

            if (message.update == false) {
                SpectrumPainterBufferConfigureResponseMessage response{&fifo};
                shared_memory.application_queue.push(response);

                if (configured == false) {
                    thread = chThdCreateStatic(thread_wa, sizeof(thread_wa),
                                               NORMALPRIO, SpectrumPainterProcessor::fn,
                                               this);

                    configured = true;
                }
            }
            break;
        }

        default:
            break;
    }
}

int main() {
    EventDispatcher event_dispatcher{std::make_unique<SpectrumPainterProcessor>()};
    event_dispatcher.run();
    return 0;
}
