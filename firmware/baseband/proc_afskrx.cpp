/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#include "proc_afskrx.hpp"
#include "sine_table.hpp"
#include "portapack_shared_memory.hpp"

using namespace lpc43xx;

void AFSKRXProcessor::execute(const buffer_c8_t& buffer) {
	if( !configured ) {
		return;
	}
	/* Called every 2048/3072000 second -- 1500Hz. */
	
	const auto decim_0_out = decim_0.execute(buffer, dst_buffer);
	const auto decim_1_out = decim_1.execute(decim_0_out, dst_buffer);
	const auto channel_out = channel_filter.execute(decim_1_out, dst_buffer);

	auto audio = demod.execute(channel_out, work_audio_buffer);

	/*static uint64_t audio_present_history = 0;
	const auto audio_present_now = squelch.execute(audio);
	audio_present_history = (audio_present_history << 1) | (audio_present_now ? 1 : 0);
	const bool audio_present = (audio_present_history != 0);
*/
	//if( !audio_present ) {
		// Zero audio buffer.
		/*for(size_t i=0; i<audio.count; i++) {
			if ((i % 3) > 1)
				audio.p[i] = 4096;
			else
				audio.p[i] = -4096;
		}*/
	//}

	//audio_hpf.execute_in_place(audio);

	for(size_t i=0; i<audio.count; i++) {
		if (spur > 10) {
			if (audio.p[i] > 2000)
				sign = 1;
			if (audio.p[i] < -2000)
				sign = 0;
			spur = 0;
		} else {
			spur++;
		}
		if (sign != prev_sign) {
			if (freq_timer < 15)		// 48
				bit = 0;
			else
				bit++;
			freq_timer = 0;
		}
		prev_sign = sign;
		if (freq_timer < 1000) freq_timer++;	// TODO: Limit in a more intelligent way
	}
	
	if (bit_timer >= 40) {
		bit_timer = 0;
		// Check bit state here !
	} else {
		bit_timer++;
	}
	
    if (sc >= 600) {
		sc = 0;
		//AFSKDataMessage message;
		//memcpy(message.data,aud,128*2);
		//shared_memory.application_queue.push(message);
		audc = 0;
	} else {
		sc++;
	}
	
	if (audc < 4) {
		memcpy(aud+(audc*32),audio.p,32*2);
		audc++;
	}
	
	audio_output.write(audio);
}

void AFSKRXProcessor::data_handler(
	const double data
) {
	/*AFSKDataMessage message;
	message.data = 'T';
	shared_memory.application_queue.push(message);*/
}
