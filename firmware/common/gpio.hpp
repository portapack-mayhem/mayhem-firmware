/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __GPIO_H__
#define __GPIO_H__

#include <array>
#include <cstddef>
#include <cstdint>

#include "ch.h"
#include "hal.h"
#include "pal_lld.h"

struct PinConfig {
    const uint32_t mode;
    const uint32_t pd;
    const uint32_t pu;
    const uint32_t fast;
    const uint32_t input;
    const uint32_t ifilt;

    constexpr operator uint16_t() const {
        return (((~ifilt) & 1) << 7) | ((input & 1) << 6) | ((fast & 1) << 5) | (((~pu) & 1) << 4) | ((pd & 1) << 3) | ((mode & 7) << 0);
    }
    /*
        constexpr operator uint32_t() {
                return scu::sfs::mode::value(mode)
                        << scu::sfs::epd::value(pd)
                        << scu::sfs::epun::value(~pu)
                        << scu::sfs::ehs::value(fast)
                        << scu::sfs::ezi::value(input)
                        << scu::sfs::zif::value(~ifilt)
                        ;
        }
*/
    static constexpr PinConfig reset() {
        return {.mode = 0, .pd = 0, .pu = 1, .fast = 0, .input = 0, .ifilt = 1};
    }

    static constexpr PinConfig floating(
        const uint32_t mode) {
        return {
            .mode = mode,
            .pd = 0,
            .pu = 0,
            .fast = 0,
            .input = 0,
            .ifilt = 1};
    }

    static constexpr PinConfig floating_input(
        const uint32_t mode) {
        return {
            .mode = mode,
            .pd = 0,
            .pu = 0,
            .fast = 0,
            .input = 1,
            .ifilt = 1};
    }

    static constexpr PinConfig floating_input_with_pull(
        const uint32_t pull_direction,
        const uint32_t mode) {
        return {
            .mode = mode,
            .pd = (pull_direction == 0) ? 1U : 0U,
            .pu = (pull_direction == 1) ? 1U : 0U,
            .fast = 0,
            .input = 1,
            .ifilt = 1};
    }

    static constexpr PinConfig gpio_led(const uint32_t mode) {
        return {.mode = mode, .pd = 0, .pu = 0, .fast = 0, .input = 0, .ifilt = 1};
    }

    static constexpr PinConfig gpio_inout_with_pull(
        const uint32_t mode,
        const uint32_t pull_direction) {
        return {
            .mode = mode,
            .pd = (pull_direction == 0) ? 1U : 0U,
            .pu = (pull_direction == 1) ? 1U : 0U,
            .fast = 0,
            .input = 1,
            .ifilt = 1};
    }

    static constexpr PinConfig gpio_inout_with_pullup(const uint32_t mode) {
        return gpio_inout_with_pull(mode, 1);
    }

    static constexpr PinConfig gpio_inout_with_pulldown(const uint32_t mode) {
        return gpio_inout_with_pull(mode, 0);
    }

    static constexpr PinConfig gpio_out_with_pull(
        const uint32_t mode,
        const uint32_t pull_direction) {
        return {
            .mode = mode,
            .pd = (pull_direction == 0) ? 1U : 0U,
            .pu = (pull_direction == 1) ? 1U : 0U,
            .fast = 0,
            .input = 0,
            .ifilt = 1};
    }

    static constexpr PinConfig gpio_out_with_pulldown(const uint32_t mode) {
        return gpio_out_with_pull(mode, 0);
    }

    static constexpr PinConfig gpio_out_with_pullup(const uint32_t mode) {
        return gpio_out_with_pull(mode, 1);
    }

    static constexpr PinConfig sgpio_in_fast(const uint32_t mode) {
        return {.mode = mode, .pd = 0, .pu = 0, .fast = 0, .input = 1, .ifilt = 0};
    }

    static constexpr PinConfig sgpio_out_fast_with_pull(
        const uint32_t mode,
        const uint32_t pull_direction) {
        return {
            .mode = mode,
            .pd = (pull_direction == 0) ? 1U : 0U,
            .pu = (pull_direction == 1) ? 1U : 0U,
            .fast = 1,
            .input = 0,
            .ifilt = 1};
    }

    static constexpr PinConfig sgpio_out_fast_with_pullup(const uint32_t mode) {
        return sgpio_out_fast_with_pull(mode, 1);
    }

    static constexpr PinConfig sgpio_inout_fast(const uint32_t mode) {
        return {.mode = mode, .pd = 0, .pu = 0, .fast = 1, .input = 1, .ifilt = 0};
    }

    static constexpr PinConfig i2c(const uint32_t mode) {
        return {.mode = mode, .pd = 0, .pu = 0, .fast = 0, .input = 1, .ifilt = 1};
    }

    static constexpr PinConfig spifi_sck(const uint32_t mode) {
        return {.mode = mode, .pd = 0, .pu = 0, .fast = 1, .input = 1, .ifilt = 0};
    }

    static constexpr PinConfig spifi_inout(const uint32_t mode) {
        return {.mode = mode, .pd = 0, .pu = 0, .fast = 1, .input = 1, .ifilt = 0};
    }

    static constexpr PinConfig spifi_cs(const uint32_t mode) {
        return {.mode = mode, .pd = 0, .pu = 0, .fast = 1, .input = 1, .ifilt = 0};
    }
};

struct Pin {
    // Pin() = delete;
    // Pin(const Pin&) = delete;
    // Pin(Pin&&) = delete;

    constexpr Pin(
        const uint8_t port,
        const uint8_t pad)
        : _pin_port{port},
          _pin_pad{pad} {
    }

    void mode(const uint_fast16_t mode) const {
        LPC_SCU->SFSP[_pin_port][_pin_pad] =
            (LPC_SCU->SFSP[_pin_port][_pin_pad] & 0xfffffff8) | mode;
    }

    void configure(const PinConfig config) const {
        LPC_SCU->SFSP[_pin_port][_pin_pad] = config;
    }

    uint8_t _pin_port;
    uint8_t _pin_pad;
};

// Defines the logical polarity of the GPIO pin
enum class Polarity {
    ActiveHigh,  // High physical state means the function is ON (default)
    ActiveLow    // Low physical state means the function is ON (inverted)
};

struct GPIO {
    // GPIO() = delete;
    // GPIO(const GPIO& gpio) = delete;
    // GPIO(GPIO&&) = delete;

    constexpr GPIO(
        const Pin& pin,
        const ioportid_t gpio_port,
        const iopadid_t gpio_pad,
        const uint16_t gpio_mode,
        const Polarity polarity = Polarity::ActiveHigh)  // Default parameter ensures backward compatibility
        : _pin{pin},
          _gpio_port{gpio_port},
          _gpio_pad{gpio_pad},
          _gpio_mode{gpio_mode},
          _polarity{polarity} {
    }

    constexpr ioportid_t port() const {
        return _gpio_port;
    }

    constexpr iopadid_t pad() const {
        return _gpio_pad;
    }

    constexpr Pin pin() const {
        return _pin;
    }

    // Returns the configured polarity of the pin
    constexpr Polarity polarity() const {
        return _polarity;
    }

    void configure() const {
        _pin.mode(_gpio_mode);
    }

    uint_fast16_t mode() const {
        return _gpio_mode;
    }

    // Physical Level Operations

    void set() const {
        palSetPad(_gpio_port, _gpio_pad);
    }

    void clear() const {
        palClearPad(_gpio_port, _gpio_pad);
    }

    void toggle() const {
        palTogglePad(_gpio_port, _gpio_pad);
    }

    void output() const {
        palSetPadMode(_gpio_port, _gpio_pad, PAL_MODE_OUTPUT_PUSHPULL);
    }

    void input() const {
        palSetPadMode(_gpio_port, _gpio_pad, PAL_MODE_INPUT);
    }

    void write(const bool value) const {
        palWritePad(_gpio_port, _gpio_pad, value);
    }

    bool read() const {
        return palReadPad(_gpio_port, _gpio_pad);
    }

    // Turns the feature ON based on its polarity
    void setActive() const {
        if (_polarity == Polarity::ActiveHigh) {
            set();
        } else {
            clear();
        }
    }

    // Turns the feature OFF based on its polarity
    void setInactive() const {
        if (_polarity == Polarity::ActiveHigh) {
            clear();
        } else {
            set();
        }
    }

    // Sets the logical state of the feature
    void setState(const bool active) const {
        if (active) {
            setActive();
        } else {
            setInactive();
        }
    }

    // Returns true if the feature is logically active/enabled
    bool isEnabled() const {
        const bool physical_state = read();
        return (_polarity == Polarity::ActiveHigh) ? physical_state : !physical_state;
    }

    bool operator!=(const GPIO& other) const {
        return (port() != other.port()) || (pad() != other.pad());
    }

    const Pin _pin;
    const ioportid_t _gpio_port;
    const iopadid_t _gpio_pad;
    const uint16_t _gpio_mode;
    const Polarity _polarity;
};

constexpr uint32_t boot_bit(const GPIO& pin, bool state) {
    return (state ? 1UL : 0UL) << pin.pad();
}

inline void setup_pin(const scu_setup_t& pin_setup) {
    LPC_SCU->SFSP[pin_setup.port][pin_setup.pin] = pin_setup.config;
}

template <size_t N>
inline void setup_gpios(const std::array<gpio_setup_t, N>& pins_setup) {
    for (size_t i = 0; i < N; i++) {
        LPC_GPIO->PIN[i] |= pins_setup[i].data;
        LPC_GPIO->DIR[i] |= pins_setup[i].dir;
    }
}

template <size_t N>
inline void setup_pins(const std::array<scu_setup_t, N>& pins_setup) {
    for (const auto& pin_setup : pins_setup) {
        setup_pin(pin_setup);
    }
}

namespace gpio_control {

struct PinMap {
    uint8_t scu_port;
    uint8_t scu_pin;
    uint8_t gpio_port;
    uint8_t gpio_pad;
    uint8_t gpio_mode;
};

#ifdef PRALINE
constexpr PinMap map_p1_ctrl0{2, 10, 0, 14, 0};  // SCU: 2, 10 | GPIO: 0, 14
constexpr PinMap map_p1_ctrl1{6, 8, 5, 16, 4};
constexpr PinMap map_p1_ctrl2{6, 9, 3, 5, 0};

constexpr PinMap map_p2_ctrl0{8, 3, 7, 3, 4};
constexpr PinMap map_p2_ctrl1{8, 4, 7, 4, 4};

constexpr PinMap map_trigger_out{2, 6, 5, 6, 4};
constexpr PinMap map_trigger_in{13, 12, 6, 26, 4};

constexpr PinMap map_clkin_ctrl{1, 20, 0, 15, 0};

constexpr PinMap map_rf5072_mix_en{9, 0, 4, 12, 0};

constexpr PinMap map_aa_en{1, 14, 1, 7, 0};  // Anti-Aliasing filter

constexpr PinMap map_sgpio_4{9, 4, 5, 17, 6};
constexpr PinMap map_sgpio_8{8, 0, 4, 0, 4};
constexpr PinMap map_sgpio_9{9, 3, 4, 15, 6};
constexpr PinMap map_sgpio_10{8, 2, 4, 2, 4};
constexpr PinMap map_en_1v2{8, 7, 4, 7, 0};
constexpr PinMap map_vregmode{4, 9, 5, 13, 4};
constexpr PinMap map_VAA_en{8, 1, 4, 1, 0};
constexpr PinMap map_led_mcu{8, 6, 4, 6, 0};
#else
constexpr PinMap map_sgpio_4{6, 3, 3, 2, 2};
constexpr PinMap map_sgpio_8{9, 6, 4, 11, 6};
constexpr PinMap map_sgpio_9{4, 3, 2, 3, 7};
constexpr PinMap map_sgpio_10{1, 14, 1, 7, 6};
constexpr PinMap map_sgpio_13{4, 8, 5, 12, 4};
constexpr PinMap map_og_1v8_en{6, 10, 3, 6, 0};
constexpr PinMap map_r9_1v8_en{5, 0, 2, 9, 0};
constexpr PinMap map_vregmode{6, 11, 3, 7, 0};
constexpr PinMap map_og_VAA_en{5, 0, 2, 9, 0};
constexpr PinMap map_r9_VAA_en{6, 10, 3, 6, 0};
#endif

constexpr PinMap map_sgpio_0{0, 0, 0, 0, 3};
constexpr PinMap map_sgpio_1{0, 1, 0, 1, 3};
constexpr PinMap map_sgpio_2{1, 15, 0, 2, 2};
constexpr PinMap map_sgpio_3{1, 16, 0, 3, 2};
constexpr PinMap map_sgpio_5{6, 6, 0, 5, 2};
constexpr PinMap map_sgpio_6{2, 2, 5, 2, 0};
constexpr PinMap map_sgpio_7{1, 0, 0, 4, 6};
constexpr PinMap map_sgpio_11{1, 17, 0, 12, 6};
constexpr PinMap map_sgpio_12{1, 18, 0, 13, 0};
constexpr PinMap map_led_usb{4, 1, 2, 1, 0};
constexpr PinMap map_led_rx{4, 2, 2, 2, 0};
constexpr PinMap map_led_tx{6, 12, 2, 8, 0};
#ifdef PRALINE

// P1 Multiplexer control pins
constexpr Pin pin_p1_ctrl0{map_p1_ctrl0.scu_port, map_p1_ctrl0.scu_pin};                                       // SCU: P2_10
constexpr GPIO p1_ctrl0{pin_p1_ctrl0, map_p1_ctrl0.gpio_port, map_p1_ctrl0.gpio_pad, map_p1_ctrl0.gpio_mode};  // GPIO[0]14

constexpr Pin pin_p1_ctrl1{map_p1_ctrl1.scu_port, map_p1_ctrl1.scu_pin};                                       // SCU: P6_8
constexpr GPIO p1_ctrl1{pin_p1_ctrl1, map_p1_ctrl1.gpio_port, map_p1_ctrl1.gpio_pad, map_p1_ctrl1.gpio_mode};  // GPIO[5]16

constexpr Pin pin_p1_ctrl2{map_p1_ctrl2.scu_port, map_p1_ctrl2.scu_pin};                                       // SCU: P6_9
constexpr GPIO p1_ctrl2{pin_p1_ctrl2, map_p1_ctrl2.gpio_port, map_p1_ctrl2.gpio_pad, map_p1_ctrl2.gpio_mode};  // GPIO[3]5

// P2 Multiplexer control pins
constexpr Pin pin_p2_ctrl0{map_p2_ctrl0.scu_port, map_p2_ctrl0.scu_pin};                                       // SCU: PE_3
constexpr GPIO p2_ctrl0{pin_p2_ctrl0, map_p2_ctrl0.gpio_port, map_p2_ctrl0.gpio_pad, map_p2_ctrl0.gpio_mode};  // GPIO[7]3

constexpr Pin pin_p2_ctrl1{map_p2_ctrl1.scu_port, map_p2_ctrl1.scu_pin};                                       // SCU: PE_4
constexpr GPIO p2_ctrl1{pin_p2_ctrl1, map_p2_ctrl1.gpio_port, map_p2_ctrl1.gpio_pad, map_p2_ctrl1.gpio_mode};  // GPIO[7]4

// Trigger pins
constexpr Pin trigger_out_pin{map_trigger_out.scu_port, map_trigger_out.scu_pin};                                             // SCU: P2_6
constexpr GPIO trigger_out{trigger_out_pin, map_trigger_out.gpio_port, map_trigger_out.gpio_pad, map_trigger_out.gpio_mode};  // GPIO[5]6

constexpr Pin trigger_in_pin{map_trigger_in.scu_port, map_trigger_in.scu_pin};                                           // SCU: PD_12
constexpr GPIO trigger_in{trigger_in_pin, map_trigger_in.gpio_port, map_trigger_in.gpio_pad, map_trigger_in.gpio_mode};  // GPIO[6]26

// Clock input control
constexpr Pin pin_clkin_ctrl{map_clkin_ctrl.scu_port, map_clkin_ctrl.scu_pin};                                           // SCU: P1_20
constexpr GPIO clkin_ctrl{pin_clkin_ctrl, map_clkin_ctrl.gpio_port, map_clkin_ctrl.gpio_pad, map_clkin_ctrl.gpio_mode};  // GPIO[0]15

// RF507x
constexpr Pin pin_rf5072_mix_en{map_rf5072_mix_en.scu_port, map_rf5072_mix_en.scu_pin};                                                 // SCU: P9_0
constexpr GPIO rf5072_mix_en{pin_rf5072_mix_en, map_rf5072_mix_en.gpio_port, map_rf5072_mix_en.gpio_pad, map_rf5072_mix_en.gpio_mode};  // GPIO[4]12

// Anti-Aliasing filter
constexpr Pin pin_aa_en{map_aa_en.scu_port, map_aa_en.scu_pin};                                 // SCU: P1_14
constexpr GPIO aa_en{pin_aa_en, map_aa_en.gpio_port, map_aa_en.gpio_pad, map_aa_en.gpio_mode};  // GPIO[1]7

constexpr Pin pin_en_1v2{map_en_1v2.scu_port, map_en_1v2.scu_pin};                                   // SCU: P8_7
constexpr GPIO en_1v2{pin_en_1v2, map_en_1v2.gpio_port, map_en_1v2.gpio_pad, map_en_1v2.gpio_mode};  // GPIO[4]7

constexpr Pin pin_VAA_en{map_VAA_en.scu_port, map_VAA_en.scu_pin};                                                        // SCU: P8_1
constexpr GPIO VAA_en{pin_VAA_en, map_VAA_en.gpio_port, map_VAA_en.gpio_pad, map_VAA_en.gpio_mode, Polarity::ActiveLow};  // GPIO[4]1

constexpr Pin pin_led_usb{map_led_usb.scu_port, map_led_usb.scu_pin};                                                          // SCU: P4_1
constexpr GPIO led_usb{pin_led_usb, map_led_usb.gpio_port, map_led_usb.gpio_pad, map_led_usb.gpio_mode, Polarity::ActiveLow};  // GPIO[2]1

constexpr Pin pin_led_rx{map_led_rx.scu_port, map_led_rx.scu_pin};                                                        // SCU: P4_2
constexpr GPIO led_rx{pin_led_rx, map_led_rx.gpio_port, map_led_rx.gpio_pad, map_led_rx.gpio_mode, Polarity::ActiveLow};  // GPIO[2]2

constexpr Pin pin_led_tx{map_led_tx.scu_port, map_led_tx.scu_pin};                                                        // SCU: P6_12
constexpr GPIO led_tx{pin_led_tx, map_led_tx.gpio_port, map_led_tx.gpio_pad, map_led_tx.gpio_mode, Polarity::ActiveLow};  // GPIO[2]8

constexpr Pin pin_led_mcu{map_led_mcu.scu_port, map_led_mcu.scu_pin};                                                          // SCU: P8_6
constexpr GPIO led_mcu{pin_led_mcu, map_led_mcu.gpio_port, map_led_mcu.gpio_pad, map_led_mcu.gpio_mode, Polarity::ActiveLow};  // GPIO[4]6
#else
constexpr Pin pin_sgpio_13{map_sgpio_13.scu_port, map_sgpio_13.scu_pin};                                       // SCU: P4_8
constexpr GPIO sgpio_13{pin_sgpio_13, map_sgpio_13.gpio_port, map_sgpio_13.gpio_pad, map_sgpio_13.gpio_mode};  // GPIO[5]12

constexpr Pin pin_og_1v8_en{map_og_1v8_en.scu_port, map_og_1v8_en.scu_pin};                                         // SCU: P6_10
constexpr GPIO og_1v8_en{pin_og_1v8_en, map_og_1v8_en.gpio_port, map_og_1v8_en.gpio_pad, map_og_1v8_en.gpio_mode};  // GPIO[3]6

constexpr Pin pin_r9_1v8_en{map_r9_1v8_en.scu_port, map_r9_1v8_en.scu_pin};                                         // SCU: P5_0
constexpr GPIO r9_1v8_en{pin_r9_1v8_en, map_r9_1v8_en.gpio_port, map_r9_1v8_en.gpio_pad, map_r9_1v8_en.gpio_mode};  // GPIO[2]9

constexpr Pin pin_og_VAA_en{map_og_VAA_en.scu_port, map_og_VAA_en.scu_pin};                                                              // SCU: P5_0
constexpr GPIO og_VAA_en{pin_og_VAA_en, map_og_VAA_en.gpio_port, map_og_VAA_en.gpio_pad, map_og_VAA_en.gpio_mode, Polarity::ActiveLow};  // GPIO[2]9

constexpr Pin pin_r9_VAA_en{map_r9_VAA_en.scu_port, map_r9_VAA_en.scu_pin};                                                              // SCU: P6_10
constexpr GPIO r9_VAA_en{pin_r9_VAA_en, map_r9_VAA_en.gpio_port, map_r9_VAA_en.gpio_pad, map_r9_VAA_en.gpio_mode, Polarity::ActiveLow};  // GPIO[3]6

constexpr Pin pin_led_usb{map_led_usb.scu_port, map_led_usb.scu_pin};                                     // SCU: P4_1
constexpr GPIO led_usb{pin_led_usb, map_led_usb.gpio_port, map_led_usb.gpio_pad, map_led_usb.gpio_mode};  // GPIO[2]1

constexpr Pin pin_led_rx{map_led_rx.scu_port, map_led_rx.scu_pin};                                   // SCU: P4_2
constexpr GPIO led_rx{pin_led_rx, map_led_rx.gpio_port, map_led_rx.gpio_pad, map_led_rx.gpio_mode};  // GPIO[2]2

constexpr Pin pin_led_tx{map_led_tx.scu_port, map_led_tx.scu_pin};                                   // SCU: P6_12
constexpr GPIO led_tx{pin_led_tx, map_led_tx.gpio_port, map_led_tx.gpio_pad, map_led_tx.gpio_mode};  // GPIO[2]8

static const motocon_pwm_resources_t motocon_pwm_resources = {
    .base = {.clk = &LPC_CGU->BASE_APB1_CLK, .stat = &LPC_CCU1->BASE_STAT, .stat_mask = (1 << 1)},
    .branch = {.cfg = &LPC_CCU1->CLK_APB1_MOTOCON_PWM_CFG, .stat = &LPC_CCU1->CLK_APB1_MOTOCON_PWM_STAT},
    .reset = {.output_index = 38},
};

static const scu_setup_t pin_setup_vaa_enablex_pwm = {5, 0, scu_config_normal_drive_t{.mode = 1, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}};
static const scu_setup_t pin_setup_vaa_enablex_gpio_og = {map_og_VAA_en.scu_port, map_og_VAA_en.scu_pin, scu_config_normal_drive_t{.mode = map_og_VAA_en.gpio_mode, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}};
static const scu_setup_t pin_setup_vaa_enablex_gpio_r9 = {map_r9_VAA_en.scu_port, map_r9_VAA_en.scu_pin, scu_config_normal_drive_t{.mode = map_r9_VAA_en.gpio_mode, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}};

#endif

// SGPIO
constexpr Pin pin_sgpio_0{map_sgpio_0.scu_port, map_sgpio_0.scu_pin};                                     // SCU: P0_0
constexpr GPIO sgpio_0{pin_sgpio_0, map_sgpio_0.gpio_port, map_sgpio_0.gpio_pad, map_sgpio_0.gpio_mode};  // GPIO[0]0

constexpr Pin pin_sgpio_1{map_sgpio_1.scu_port, map_sgpio_1.scu_pin};                                     // SCU: P0_1
constexpr GPIO sgpio_1{pin_sgpio_1, map_sgpio_1.gpio_port, map_sgpio_1.gpio_pad, map_sgpio_1.gpio_mode};  // GPIO[0]1

constexpr Pin pin_sgpio_2{map_sgpio_2.scu_port, map_sgpio_2.scu_pin};                                     // SCU: P1_15
constexpr GPIO sgpio_2{pin_sgpio_2, map_sgpio_2.gpio_port, map_sgpio_2.gpio_pad, map_sgpio_2.gpio_mode};  // GPIO[0]2

constexpr Pin pin_sgpio_3{map_sgpio_3.scu_port, map_sgpio_3.scu_pin};                                     // SCU: P1_16
constexpr GPIO sgpio_3{pin_sgpio_3, map_sgpio_3.gpio_port, map_sgpio_3.gpio_pad, map_sgpio_3.gpio_mode};  // GPIO[0]3

constexpr Pin pin_sgpio_4{map_sgpio_4.scu_port, map_sgpio_4.scu_pin};                                     // SCU: P6_3, PRALINE: P9_4
constexpr GPIO sgpio_4{pin_sgpio_4, map_sgpio_4.gpio_port, map_sgpio_4.gpio_pad, map_sgpio_4.gpio_mode};  // GPIO[6]3 PRALINE: GPIO[5]17

constexpr Pin pin_sgpio_5{map_sgpio_5.scu_port, map_sgpio_5.scu_pin};                                     // SCU: P6_6
constexpr GPIO sgpio_5{pin_sgpio_5, map_sgpio_5.gpio_port, map_sgpio_5.gpio_pad, map_sgpio_5.gpio_mode};  // GPIO[0]5

constexpr Pin pin_sgpio_6{map_sgpio_6.scu_port, map_sgpio_6.scu_pin};                                     // SCU: P2_2
constexpr GPIO sgpio_6{pin_sgpio_6, map_sgpio_6.gpio_port, map_sgpio_6.gpio_pad, map_sgpio_6.gpio_mode};  // GPIO[5]2

constexpr Pin pin_sgpio_7{map_sgpio_7.scu_port, map_sgpio_7.scu_pin};                                     // SCU: P1_0
constexpr GPIO sgpio_7{pin_sgpio_7, map_sgpio_7.gpio_port, map_sgpio_7.gpio_pad, map_sgpio_7.gpio_mode};  // GPIO[0]4

constexpr Pin pin_sgpio_8{map_sgpio_8.scu_port, map_sgpio_8.scu_pin};                                     // SCU: P9_6, PRALINE: P8_0
constexpr GPIO sgpio_8{pin_sgpio_8, map_sgpio_8.gpio_port, map_sgpio_8.gpio_pad, map_sgpio_8.gpio_mode};  // GPIO[4]11, PRALINE: GPIO[4]0

constexpr Pin pin_sgpio_9{map_sgpio_9.scu_port, map_sgpio_9.scu_pin};                                     // SCU: P4_3, PRALINE: P9_3
constexpr GPIO sgpio_9{pin_sgpio_9, map_sgpio_9.gpio_port, map_sgpio_9.gpio_pad, map_sgpio_9.gpio_mode};  // GPIO[2]3, PRALINE: GPIO[4]12

constexpr Pin pin_sgpio_10{map_sgpio_10.scu_port, map_sgpio_10.scu_pin};                                       // SCU: P1_14, PRALINE: P8_2
constexpr GPIO sgpio_10{pin_sgpio_10, map_sgpio_10.gpio_port, map_sgpio_10.gpio_pad, map_sgpio_10.gpio_mode};  // GPIO[1]7, PRALINE: GPIO[4]2

constexpr Pin pin_sgpio_11{map_sgpio_11.scu_port, map_sgpio_11.scu_pin};                                       // SCU: P1_17
constexpr GPIO sgpio_11{pin_sgpio_11, map_sgpio_11.gpio_port, map_sgpio_11.gpio_pad, map_sgpio_11.gpio_mode};  // GPIO[0]17

constexpr Pin pin_sgpio_12{map_sgpio_12.scu_port, map_sgpio_12.scu_pin};                                       // SCU: P1_18
constexpr GPIO sgpio_12{pin_sgpio_12, map_sgpio_12.gpio_port, map_sgpio_12.gpio_pad, map_sgpio_12.gpio_mode};  // GPIO[0]13

constexpr Pin pin_vregmode{map_vregmode.scu_port, map_vregmode.scu_pin};                                       // SCU: P6_11, PRALINE: P4_9
constexpr GPIO vregmode{pin_vregmode, map_vregmode.gpio_port, map_vregmode.gpio_pad, map_vregmode.gpio_mode};  // GPIO[3]7, PRALINE: GPIO[5]13
}  // namespace gpio_control

namespace power_control {

void vaa_power_on(void);
void vaa_power_off(void);

#ifdef PRALINE
void aux_power_on(void);
void aux_power_off(void);
#endif

void core_power_on(void);
void core_power_off(void);

}  // namespace power_control

#endif /*__GPIO_H__*/
