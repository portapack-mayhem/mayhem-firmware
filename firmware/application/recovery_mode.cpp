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

#include "recovery_mode.hpp"
#include "core_control.hpp"
#include "hackrf_gpio.hpp"
#include "portapack_hal.hpp"

void recovery_mode_blink_until_dfu();

void recovery_mode_set() {
    portapack::persistent_memory::set_recovery_mode_storage(RECOVERY_MODE_GUARD_VALUE);
}

bool recovery_mode_should_enter() {
    return portapack::persistent_memory::recovery_mode_storage() == RECOVERY_MODE_GUARD_VALUE;
}

void recovery_mode_clear() {
    portapack::persistent_memory::set_recovery_mode_storage(RECOVERY_MODE_NORMAL_VALUE);
}

uint32_t blink_patterns[] = {
    0x00000000,  // 0 Off
    0xFFFFFFFF,  // 1 On
    0xF0F0F0F0,  // 2 blink fast
    0x00FF00FF,  // 3 blink slow
    0xFFF3FFF3   // 4 inverse blink slow
};

void recovery_mode_run() {
    configure_pins_portapack();
    portapack::gpio_dfu.input();

    recovery_mode_blink_until_dfu();

    auto last_dfu_btn = portapack::gpio_dfu.read();
    portapack::persistent_memory::cache::init();
    int8_t cpld_blink_pattern_value = portapack::persistent_memory::config_cpld(); /* 0 .. 4*/
    int32_t counter = 0;

    while (true) {
        auto dfu_btn = portapack::gpio_dfu.read();
        auto dfu_clicked = last_dfu_btn == true && dfu_btn == false;
        last_dfu_btn = dfu_btn;

        if (dfu_clicked) {
            int8_t value = portapack::persistent_memory::config_cpld(); /* 0 .. 4*/
            cpld_blink_pattern_value = value = (value + 1) % 5;

            portapack::persistent_memory::set_config_cpld(value);
            portapack::persistent_memory::cache::persist();
        }

        auto cpld_blink_pattern = blink_patterns[cpld_blink_pattern_value];

        auto led_value = ((cpld_blink_pattern >> ((counter >> 0) & 31)) & 0x1) == 0x1;
        if (led_value) {
            hackrf::one::led_tx.on();
        } else {
            hackrf::one::led_tx.off();
        }

        chThdSleepMilliseconds(100);
        counter++;
    }
}

void recovery_mode_blink_until_dfu() {
    while (true) {
        hackrf::one::led_tx.on();
        hackrf::one::led_rx.on();
        hackrf::one::led_usb.on();
        chThdSleepMilliseconds(10);

        hackrf::one::led_tx.off();
        hackrf::one::led_rx.off();
        hackrf::one::led_usb.off();
        chThdSleepMilliseconds(115);

        auto dfu_btn = portapack::gpio_dfu.read();
        if (dfu_btn)
            break;
    }

    while (true) {
        chThdSleepMilliseconds(10);

        auto dfu_btn = portapack::gpio_dfu.read();
        if (!dfu_btn)
            break;
    }

    chThdSleepMilliseconds(10);
}