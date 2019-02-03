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

#include "irq_controls.hpp"

#include "ch.h"
#include "hal.h"

#include "event_m0.hpp"

#include "touch.hpp"
#include "touch_adc.hpp"
#include "encoder.hpp"
#include "debounce.hpp"
#include "utility.hpp"

#include <cstdint>
#include <array>

#include "portapack_io.hpp"

#include "hackrf_hal.hpp"
using namespace hackrf::one;

static Thread* thread_controls_event = NULL;

static std::array<Debounce, 7> switch_debounce;

static Encoder encoder;

static volatile uint32_t encoder_position { 0 };

static volatile uint32_t touch_phase { 0 };

/* TODO: Change how touch scanning works. It produces a decent amount of noise
 * when changing potential on the resistive touch pad. Among other things, I
 * see blips of noise when sampling the MAX2837 RSSI signal. And when the
 * radio is off (RSSI signal is not driven?), the signal floats a LOT when the
 * touch panel potentials are changing.
 *
 * Ideally, scan only for pressure until a touch is detected. Then scan X/Y.
 * Noise will only occur when the panel is being touched. Not ideal, but
 * an acceptable improvement.
 */
static std::array<portapack::IO::TouchPinsConfig, 3> touch_pins_configs {
	/* State machine will pause here until touch is detected. */
	portapack::IO::TouchPinsConfig::SensePressure,
	portapack::IO::TouchPinsConfig::SenseX,
	portapack::IO::TouchPinsConfig::SenseY,
};

static touch::Frame temp_frame;
static touch::Frame touch_frame;

static uint32_t touch_debounce = 0;
static uint32_t touch_debounce_mask = (1U << 4) - 1;
static bool touch_detected = false;
static bool touch_cycle = false;

static bool touch_update() {
	const auto samples = touch::adc::get();
	const auto current_phase = touch_pins_configs[touch_phase];

	switch(current_phase) {
	case portapack::IO::TouchPinsConfig::SensePressure:
		{
			const auto z1 = samples.xp - samples.xn;
			const auto z2 = samples.yp - samples.yn;
			const auto touch_raw = (z1 > touch::touch_threshold) || (z2 > touch::touch_threshold);
			touch_debounce = (touch_debounce << 1) | (touch_raw ? 1U : 0U);
			touch_detected = ((touch_debounce & touch_debounce_mask) == touch_debounce_mask);
			if( !touch_detected && !touch_cycle ) {
				temp_frame.pressure = { };
				return false;
			} else {
				temp_frame.pressure += samples;
			}
		}
		break;

	case portapack::IO::TouchPinsConfig::SenseX:
		temp_frame.x += samples;
		break;

	case portapack::IO::TouchPinsConfig::SenseY:
		temp_frame.y += samples;
		break;

	default:
		break;
	}

	touch_phase++;
	if( touch_phase >= touch_pins_configs.size() ) {
		/* New iteration, calculate values and flag touch event */
		touch_phase = 0;
		temp_frame.touch = touch_detected;
		touch_cycle = touch_detected;
		touch_frame = temp_frame;
		temp_frame = {};
		return true;
	} else {
		return false;
	}
}

static uint8_t switches_raw = 0;

static bool switches_update(const uint8_t raw) {
	// TODO: Only fire event on press, not release?
	bool switch_changed = false;
	for(size_t i=0; i<switch_debounce.size(); i++) {
		switch_changed |= switch_debounce[i].feed((raw >> i) & 1);
	}

	return switch_changed;
}

static bool encoder_read() {
	const auto delta = encoder.update(
		switch_debounce[5].state(),
		switch_debounce[6].state()
	);

	if( delta != 0 ) {
		encoder_position += delta;
		return true;;
	} else {
		return false;
	}
}

void timer0_callback(GPTDriver* const) {
	eventmask_t event_mask = 0;
	if( touch_update() ) event_mask |= EVT_MASK_TOUCH;
	switches_raw = portapack::io.io_update(touch_pins_configs[touch_phase]);
	if( switches_update(switches_raw) ) {
		event_mask |= EVT_MASK_SWITCHES;
		if( encoder_read() ) event_mask |= EVT_MASK_ENCODER;
	}

	/* Signal event loop */
	if( event_mask ) {
		chSysLockFromIsr();
		chEvtSignalI(thread_controls_event, event_mask);
		chSysUnlockFromIsr();
	}
}

/* TODO: Refactor some/all of this to appropriate shared headers? */
static constexpr uint32_t timer0_count_f = 1000000;
static constexpr uint32_t timer0_prescaler_ratio = (base_m0_clk_f / timer0_count_f);
static constexpr uint32_t ui_interrupt_rate = 1000;
static constexpr uint32_t timer0_match_count = timer0_count_f / ui_interrupt_rate;

/* GPT driver refers to configuration structure during runtime, so make sure
 * it sticks around.
 */
static GPTConfig timer0_config {
	.callback = timer0_callback,
	.pr = timer0_prescaler_ratio - 1,
};

void controls_init() {
	thread_controls_event = chThdSelf();

	touch::adc::start();

	/* GPT timer 0 is used to scan user interface controls -- touch screen,
	 * navigation switches.
	 */
	gptStart(&GPTD1, &timer0_config);
	gptStartContinuous(&GPTD1, timer0_match_count);
}

SwitchesState get_switches_state() {
	SwitchesState result;
	for(size_t i=0; i<result.size(); i++) {
 		// TODO: Ignore multiple keys at the same time?
 		result[i] = switch_debounce[i].state();
	}

	return result;
}

EncoderPosition get_encoder_position() {
	return encoder_position;
}

touch::Frame get_touch_frame() {
	return touch_frame;
}

namespace control {
namespace debug {

uint8_t switches() {
	return switches_raw;
}

} /* debug */
} /* control */
