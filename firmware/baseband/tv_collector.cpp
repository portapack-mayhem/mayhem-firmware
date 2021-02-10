/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2020 Shao
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

#include "tv_collector.hpp"

#include "dsp_fft.hpp"

#include "utility.hpp"
#include "event_m4.hpp"
#include "portapack_shared_memory.hpp"

#include "event_m4.hpp"

#include <algorithm>

void TvCollector::on_message(const Message* const message) {
	switch(message->id) {
	case Message::ID::UpdateSpectrum:
		update();
		break;

	case Message::ID::SpectrumStreamingConfig:
		set_state(*reinterpret_cast<const SpectrumStreamingConfigMessage*>(message));
		break;

	default:
		break;
	}
}

void TvCollector::set_state(const SpectrumStreamingConfigMessage& message) {
	if( message.mode == SpectrumStreamingConfigMessage::Mode::Running ) {
		start();
	} else {
		stop();
	}
}

void TvCollector::start() {
	streaming = true;
	ChannelSpectrumConfigMessage message { &fifo };
	shared_memory.application_queue.push(message);
}

void TvCollector::stop() {
	streaming = false;
	fifo.reset_in();
}

void TvCollector::set_decimation_factor(
	const size_t decimation_factor
) {
	channel_spectrum_decimator.set_factor(decimation_factor);
}

/* TODO: Refactor to register task with idle thread?
 * It's sad that the idle thread has to call all the way back here just to
 * perform the deferred task on the buffer of data we prepared.
 */

void TvCollector::feed(
	const buffer_c16_t& channel
) {
	// Called from baseband processing thread.
	channel_spectrum_decimator.feed(channel,[this](const buffer_c16_t& data) {this->post_message(data);});
}

void TvCollector::post_message(const buffer_c16_t& data) {
	// Called from baseband processing thread.
        float re, im;
	float mag;
        float max;
	if( streaming && !channel_spectrum_request_update ) {
                for(size_t i=0; i<256; i++) 
		{
			const auto s = data.p[i];
                        re = (float)(data.p[i].real());
			im = (float)(data.p[i].imag());
			mag = __builtin_sqrtf((re * re) + (im * im)) ;
			channel_spectrum[i] = {mag, mag};
		}
                channel_spectrum_sampling_rate = data.sampling_rate;
		channel_spectrum_request_update = true;
		EventDispatcher::events_flag(EVT_MASK_SPECTRUM);
	}
}


void TvCollector::update() {
	// Called from idle thread (after EVT_MASK_SPECTRUM is flagged)
	if( streaming && channel_spectrum_request_update ) {
		ChannelSpectrum spectrum;
		spectrum.sampling_rate = channel_spectrum_sampling_rate;
		for(size_t i=0; i<spectrum.db.size(); i++) {
                        const auto corrected_sample = channel_spectrum[i].real();
			const unsigned int v = corrected_sample + 127.0f;
			spectrum.db[i] = std::max(0U, std::min(255U, v));
		}
		fifo.in(spectrum);
	}

	channel_spectrum_request_update = false;
}
