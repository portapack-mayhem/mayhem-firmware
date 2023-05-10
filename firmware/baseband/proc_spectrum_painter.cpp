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
#include "debug.hpp"

#include <cstdint>

complex32_t *current_line_data = nullptr;
complex32_t * volatile next_line_data = nullptr;
uint32_t current_line_index = 0;
uint32_t current_line_width = 0;
constexpr std::complex<int8_t> wave[4] = {{127, 0}, {0, 127}, {-127, 0}, {0, -127}};


/* Period parameters */
#define N 624
#define M 397
#define MATRIX_A 0x9908b0dfUL		/* constant vector a */
#define UMASK 0x80000000UL		/* most significant w-r bits */
#define LMASK 0x7fffffffUL		/* least significant r bits */
#define MIXBITS(u,v) ( ((u) & UMASK) | ((v) & LMASK) )
#define TWIST(u,v) ((MIXBITS(u,v) >> 1) ^ ((v)&1UL ? MATRIX_A : 0UL))

static unsigned long state[N];		/* the array for the state vector  */
static int left2 = 1;
static int initf = 0;
static unsigned long *next2;

/* initializes state[N] with a seed */
void    init_genrand(unsigned long s)
{
    int     j;

    state[0] = s & 0xffffffffUL;
    for (j = 1; j < N; j++) {
	state[j] = (1812433253UL * (state[j - 1] ^ (state[j - 1] >> 30)) + j);
	/* See Knuth TAOCP Vol2. 3rd Ed. P.106 for multiplier. */
	/* In the previous versions, MSBs of the seed affect   */
	/* only MSBs of the array state[].                        */
	/* 2002/01/09 modified by Makoto Matsumoto             */
	state[j] &= 0xffffffffUL;		/* for >32 bit machines */
    }
    left2 = 1;
    initf = 1;
}
static void next_state(void)
{
    unsigned long *p = state;
    int     j;

    /* if init_genrand() has not been called, */
    /* a default initial seed is used         */
    if (initf == 0)
	init_genrand(5489UL);

    left2 = N;
    next2 = state;

    for (j = N - M + 1; --j; p++)
	*p = p[M] ^ TWIST(p[0], p[1]);

    for (j = M; --j; p++)
	*p = p[M - N] ^ TWIST(p[0], p[1]);

    *p = p[M - N] ^ TWIST(p[0], state[0]);
}
long    genrand_int31(void)
{
    unsigned long y;

    if (--left2 == 0)
	next_state();
    y = *next2++;

    /* Tempering */
    y ^= (y >> 11);
    y ^= (y << 7) & 0x9d2c5680UL;
    y ^= (y << 15) & 0xefc60000UL;
    y ^= (y >> 18);

    return (long) (y >> 1);
}

extern "C" void update_performance_counters();

std::vector<uint8_t> fifo_data[1 << SpectrumPainterBufferConfigureResponseMessage::fifo_k] { };
SpectrumPainterFIFO fifo { fifo_data, SpectrumPainterBufferConfigureResponseMessage::fifo_k };

int max_val = 127;

// This is called at 3072000/2048 = 1500Hz
void SpectrumPainterProcessor::execute(const buffer_c8_t& buffer) {

	if (!configured) return;

	for (uint32_t i = 0; i < buffer.count; i++) {
	 	if (current_line_data == nullptr)
			if (((current_line_index++ >> 21) & 0x01) == 0x01) {
				buffer.p[i] = {0,0}; // wave[(2048 - i) % 4];

			}
			else {
				buffer.p[i] = wave[i % 4];

			}

	 	else {
			// if (((current_line_index++ >> 21) & 0x01) == 0x01) {
				int rampUp = current_line_index + 1;
				if (rampUp > 100) {
					rampUp = 100;
				}
				int rampDown = (current_line_index % current_line_width)-current_line_width;
				if (rampDown > 100) {
					rampDown = 100;
				}
				int ramp = rampUp * rampDown / 100;

				auto data = current_line_data[current_line_index++ % current_line_width];
				if (data.real() > 127)
					max_val = data.real();
				if (data.real() < -127)
					max_val = - data.real();

				if (data.imag() > 127 )
					max_val = data.imag();
				if (data.imag() < -128)
					max_val = -data.imag();

	 			buffer.p[i] = {(int8_t) (data.real() * 120 / max_val * ramp / 100), (int8_t) (data.imag() * 120 / max_val * ramp / 100)};
			// }
			// else
			// 	buffer.p[i] = {0,0}; //wave[i % 4];
		}
	}

	if (next_line_data != nullptr) {
		if (current_line_data != nullptr)
			delete current_line_data;
		current_line_data = next_line_data;
		next_line_data = nullptr;
	}

	
  	update_performance_counters();

}

WORKING_AREA(thread_wa, 4096);

void SpectrumPainterProcessor::run() {
int ui = 0;init_genrand(22267);
	while (true) {
		if (fifo.is_empty() == false && next_line_data == nullptr) {
			std::vector<uint8_t> data;
			fifo.out(data);

			auto size = data.size() *2;
			auto qu = size/4;

			complex32_t *v = new complex32_t[size];
			complex32_t *tmp = new complex32_t[size];

			for (std::size_t i = 0; i < data.size(); i++) {
				if (i < qu) {
					v[i] = {0, 0};
				}
				else if (i < qu*3) {
					// auto phase_offset = std::exp(complex32_t({0, phase}));

					auto bin_power = data[i-qu]; // 0 to 255
					auto bin_phase = genrand_int31() >> 23; // 0 to 255

					// rotate by random angle
					auto phase_cos = (sine_table_i8[((int)(bin_phase + 0x40)) & 0xFF]); // -127 to 127
            		auto phase_sin = (sine_table_i8[((int)(bin_phase)) & 0xFF]);

					// do: fftshift
					auto fftshift_index = 0;
					if (i < qu*2)
						fftshift_index = i + qu;
					else
						fftshift_index = i - qu;

					v[fftshift_index] = {(int16_t)phase_cos * bin_power / 255, (int16_t)phase_sin * bin_power / 255}; // -127 to 127
				}
				else {
					v[i] = {0, 0};
				}
				

				// if (i < (ui % 100))
				// 	v[i] = {10, 255 - (genrand_int31() >> 23)};
				// else
				// 	v[i] = {0, 0};
			}

			ifft<complex32_t>(v, size, tmp);

			delete tmp;
			next_line_data = v;
			ui++;
		}
		else {
			chThdSleepMilliseconds(1);
		}
	}
}

void SpectrumPainterProcessor::on_message(const Message* const msg) {

	switch(msg->id) {
		case Message::ID::SpectrumPainterBufferRequestConfigure:
		{
			const auto message = *reinterpret_cast<const SpectrumPainterBufferConfigureRequestMessage*>(msg);
			current_line_width = message.width;

			SpectrumPainterBufferConfigureResponseMessage response { &fifo };
			shared_memory.application_queue.push(response);

			if (configured == false) {
				thread = chThdCreateStatic(thread_wa, sizeof(thread_wa),
					NORMALPRIO, SpectrumPainterProcessor::fn,
					this
				);
		
				configured = true;
			}
			break;
		}

		default:
			break;
	}

}

int main() {
	EventDispatcher event_dispatcher { std::make_unique<SpectrumPainterProcessor>() };
	event_dispatcher.run();
	return 0;
}
