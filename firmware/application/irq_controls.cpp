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
#include "debounce.hpp"
#include "encoder.hpp"
#include "event_m0.hpp"
#include "hal.h"
#include "touch.hpp"
#include "touch_adc.hpp"
#include "utility.hpp"

#include <cstdint>
#include <array>

#include "portapack_io.hpp"
#include "hackrf_hal.hpp"

using namespace hackrf::one;
using namespace portapack;

static Thread* thread_controls_event = NULL;

// Index with the Switch enum.
static std::array<Debounce, 6> switch_debounce;
static std::array<Debounce, 2> encoder_debounce;

static_assert(std::size(switch_debounce) == toUType(Switch::Dfu) + 1);

static Encoder encoder;
static volatile uint32_t encoder_position{0};
static volatile uint32_t touch_phase{0};

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
static std::array<IO::TouchPinsConfig, 3> touch_pins_configs{
    /* State machine will pause here until touch is detected. */
    IO::TouchPinsConfig::SensePressure,
    IO::TouchPinsConfig::SenseX,
    IO::TouchPinsConfig::SenseY,
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

    switch (current_phase) {
        case IO::TouchPinsConfig::SensePressure: {
            const auto z1 = samples.xp - samples.xn;
            const auto z2 = samples.yp - samples.yn;
            const auto touch_raw = (z1 > touch::touch_threshold) || (z2 > touch::touch_threshold);
            touch_debounce = (touch_debounce << 1) | (touch_raw ? 1U : 0U);
            touch_detected = ((touch_debounce & touch_debounce_mask) == touch_debounce_mask);
            if (!touch_detected && !touch_cycle) {
                temp_frame.pressure = {};
                return false;
            } else {
                temp_frame.pressure += samples;
            }
        } break;

        case IO::TouchPinsConfig::SenseX:
            temp_frame.x += samples;
            break;

        case IO::TouchPinsConfig::SenseY:
            temp_frame.y += samples;
            break;

        default:
            break;
    }

    touch_phase++;
    if (touch_phase >= touch_pins_configs.size()) {
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
static uint8_t injected_switch = 0;
static uint8_t injected_encoder = 0;

/* The raw data is not packed in a way that makes looping over it easy.
 * One option would be an accessor helper (RawSwitch). Another option
 * is to swizzle the bits into a friendlier order. */
// /* Type to access the bits in the raw switch data. */
// struct RawSwitch {
//     const uint8_t raw_{0};

// uint8_t right() const { return (raw_ >> 0) & 1; }
// uint8_t left() const { return (raw_ >> 1) & 1; }
// uint8_t down() const { return (raw_ >> 2) & 1; }
// uint8_t up() const { return (raw_ >> 3) & 1; }
// uint8_t select() const { return (raw_ >> 4) & 1; }
// uint8_t rot_a() const { return (raw_ >> 5) & 1; }
// uint8_t rot_b() const { return (raw_ >> 6) & 1; }
// uint8_t dfu() const { return (raw_ >> 7) & 1; }};

uint8_t swizzled_switches() {
    uint8_t raw = io.io_update(touch_pins_configs[touch_phase]);

    return (raw & 0x1F) |         // Keep the bottom 5 bits the same.
           ((raw >> 2) & 0x20) |  // Shift the DFU bit down to bit 6.
           ((raw << 1) & 0xC0);   // Shift the encoder bits up to be 7 & 8.
}

static bool switches_update(const uint8_t raw) {
    // TODO: Only fire event on press, not release?
    bool switch_changed = false;

    for (size_t i = 0; i < switch_debounce.size(); ++i) {
        uint8_t bit = (raw >> i) & 0x01;
        switch_changed |= switch_debounce[i].feed(bit);
    }

    return switch_changed;
}

static bool encoder_update(const uint8_t raw) {
    bool encoder_changed = false;

    encoder_changed |= encoder_debounce[0].feed((raw >> 6) & 0x01);
    encoder_changed |= encoder_debounce[1].feed((raw >> 7) & 0x01);

    return encoder_changed;
}

static bool encoder_read() {
    const auto delta = encoder.update(
        encoder_debounce[0].state() | (injected_encoder == 1),
        encoder_debounce[1].state() | (injected_encoder == 2));

    if (injected_encoder > 0)
        injected_encoder = 0;

    if (delta != 0) {
        encoder_position += delta;
        return true;
    } else {
        return false;
    }
}

void timer0_callback(GPTDriver* const) {
    eventmask_t event_mask = 0;
    if (touch_update()) event_mask |= EVT_MASK_TOUCH;

    switches_raw = swizzled_switches();
    if (switches_update(switches_raw) || (injected_switch > 0))
        event_mask |= EVT_MASK_SWITCHES;

    if (encoder_update(switches_raw) || encoder_read())
        event_mask |= EVT_MASK_ENCODER;

    /* Signal event loop */
    if (event_mask) {
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
static GPTConfig timer0_config{
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

    // Enable repeat for directional switches only
    for (auto i = Switch::Right; i <= Switch::Up; incr(i))
        switch_debounce[toUType(i)].enable_repeat();
}

// Note: Called by event handler or apps, not in ISR, so some presses might be missed during high CPU utilization
SwitchesState get_switches_state() {
    SwitchesState result;

    // TODO: Ignore multiple keys at the same time?
    for (size_t i = 0; i < result.size(); i++)
        result[i] = switch_debounce[i].state();

    if (injected_switch > 0 && injected_switch <= 6) {
        result[injected_switch - 1] = 1;
        injected_switch = 0xff;
    } else if (injected_switch == 0xff) {
        injected_switch = 0x00;
    }

    return result;
}

/* Gets the long press enabled state for all the switches. */
SwitchesState get_switches_long_press_config() {
    SwitchesState result;

    for (size_t i = 0; i < result.size(); i++)
        result[i] = switch_debounce[i].get_long_press_enabled();

    return result;
}

/* Configures which switches support long press.
 * NB: those switches will not support Repeat function. */
void set_switches_long_press_config(SwitchesState switch_config) {
    for (size_t i = 0; i < switch_config.size(); i++)
        switch_debounce[i].set_long_press_enabled(switch_config[i]);
}

bool switch_is_long_pressed(Switch s) {
    return switch_debounce[toUType(s)].long_press_occurred();
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

void inject_switch(uint8_t button) {
    if (button <= 6)
        injected_switch = button;
    else if (button > 6)
        injected_encoder = button - 6;
}

}  // namespace debug
}  // namespace control
