/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
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

#include "proc_ook.hpp"
#include "portapack_shared_memory.hpp"
#include "sine_table_int8.hpp"
#include "event_m4.hpp"

#include <cstdint>

inline void OOKProcessor::write_sample(const buffer_c8_t& buffer, uint8_t bit_value, size_t i) {
	int8_t re, im;

	if (bit_value) {
		phase = (phase + 200);			// What ?
		sphase = phase + (64 << 18);

		re = (sine_table_i8[(sphase & 0x03FC0000) >> 18]);
		im = (sine_table_i8[(phase & 0x03FC0000) >> 18]);
	} else {
		re = 0;
		im = 0;
	}

	buffer.p[i] = {re, im};
}

inline void OOKProcessor::duval_algo(const buffer_c8_t& buffer) {
	size_t buf_ptr = 0;
	const unsigned int w = de_bruijn_length;

	// Duval's algorithm for generating de Bruijn sequence
	while (idx) {
		if (w % idx == 0) {
			for (; k < idx; k++) {
				size_t available_size = buffer.count - buf_ptr;
				size_t len = (samples_per_bit > available_size) ? available_size : samples_per_bit;

				for (; bit_ptr < len; bit_ptr++) {
					write_sample(buffer, v[k], buf_ptr);
					buf_ptr++;
				}

				if (buf_ptr == buffer.count) {
					txprogress_message.done = false;
					txprogress_message.progress = scan_progress++;
					shared_memory.application_queue.push(txprogress_message);
					return;
				}

				bit_ptr = 0;
			}

			k = 0;
		}

		for (unsigned int j = 0; j < w - idx; j++)
			v[idx + j] = v[j];

		for (idx = w; idx > 0 && v[idx - 1]; idx--) ;

		if (idx)
			v[idx - 1] = 1;
	}

	// clear the buffer in case we have any bytes left
	if (buf_ptr < buffer.count) {
		for (size_t i = buf_ptr; i < buffer.count; i++) {
			buffer.p[i] = {0, 0};
		}
	}

	if (!scan_done) {
		txprogress_message.done = true;
		shared_memory.application_queue.push(txprogress_message);
	}

	scan_done = 1;
}

void OOKProcessor::execute(const buffer_c8_t& buffer) {
	// This is called at 2.28M/2048 = 1113Hz

	if (!configured) return;

	if (de_bruijn_length) {
		duval_algo(buffer);
		return;
	}

	for (size_t i = 0; i < buffer.count; i++) {

		// Synthesis at 2.28M/10 = 228kHz
		if (!s) {
			s = 10 - 1;
			if (sample_count >= samples_per_bit) {
				if (configured) {
					if (bit_pos >= length) {
						// End of data
						if (pause_counter == 0) {
							pause_counter = pause;
							cur_bit = 0;
						} else if (pause_counter == 1) {
							if (repeat_counter < repeat) {
								// Repeat
								cur_bit = shared_memory.bb_data.data[0] & 0x80;
								txprogress_message.progress = repeat_counter + 1;
								txprogress_message.done = false;
								shared_memory.application_queue.push(txprogress_message);
								bit_pos = 1;
								repeat_counter++;
							} else {
								// Stop
								cur_bit = 0;
								txprogress_message.done = true;
								shared_memory.application_queue.push(txprogress_message);
								configured = false;
							}
							pause_counter = 0;
						} else {
							pause_counter--;
						}
					} else {
						cur_bit = (shared_memory.bb_data.data[bit_pos >> 3] << (bit_pos & 7)) & 0x80;
						bit_pos++;
					}
				}

				sample_count = 0;
			} else {
				sample_count++;
			}
		} else {
			s--;
		}

		write_sample(buffer, cur_bit, i);
	}
}

void OOKProcessor::on_message(const Message* const p) {
	const auto message = *reinterpret_cast<const OOKConfigureMessage*>(p);

	if (message.id == Message::ID::OOKConfigure) {
		configured = false;

		repeat = message.repeat - 1;
		length = message.stream_length;
		pause = message.pause_symbols + 1;
		de_bruijn_length = message.de_bruijn_length;
		samples_per_bit = message.samples_per_bit;

		if (!length && !samples_per_bit) {
			// shutdown
			return;
		}

		if (de_bruijn_length) {
			if (de_bruijn_length > sizeof(v)) {
				return;
			}

			if (samples_per_bit > 2048) {
				// can't handle more than dma::transfer_samples
				return;
			}

			k = 0;
			bit_ptr = 0;
			idx = 1;
			scan_done = false;
			scan_progress = 0;

			memset(v, 0, sizeof(v));
		} else {
			samples_per_bit /= 10;
		}

		pause_counter = 0;
		s = 0;
		sample_count = samples_per_bit;
		repeat_counter = 0;
		bit_pos = 0;
		cur_bit = 0;
		txprogress_message.progress = 0;
		txprogress_message.done = false;
		configured = true;
	}
}

int main() {
	EventDispatcher event_dispatcher { std::make_unique<OOKProcessor>() };
	event_dispatcher.run();
	return 0;
}
