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

#include <cstdint>

#include "ch.h"
#include "hal.h"

struct PinConfig {
	const uint32_t mode;
	const uint32_t pd;
	const uint32_t pu;
	const uint32_t fast;
	const uint32_t input;
	const uint32_t ifilt;

	constexpr operator uint16_t() const {
		return
			  (((~ifilt) & 1) << 7)
			|  ((input   & 1) << 6)
			|  ((fast    & 1) << 5)
			| (((~pu)    & 1) << 4)
			|  ((pd      & 1) << 3)
			|  ((mode    & 7) << 0);
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
		return { .mode = 0, .pd = 0, .pu = 1, .fast = 0, .input = 0, .ifilt = 1 };
	}

	static constexpr PinConfig floating(
		const uint32_t mode
	) {
		return {
			.mode = mode,
			.pd = 0,
			.pu = 0,
			.fast = 0,
			.input = 0,
			.ifilt = 1
		};
	}

	static constexpr PinConfig floating_input(
		const uint32_t mode
	) {
		return {
			.mode = mode,
			.pd = 0,
			.pu = 0,
			.fast = 0,
			.input = 1,
			.ifilt = 1
		};
	}

	static constexpr PinConfig floating_input_with_pull(
		const uint32_t pull_direction,
		const uint32_t mode
	) {
		return {
			.mode = mode,
			.pd = (pull_direction == 0) ? 1U : 0U,
			.pu = (pull_direction == 1) ? 1U : 0U,
			.fast = 0,
			.input = 1,
			.ifilt = 1
		};
	}

	static constexpr PinConfig gpio_led(const uint32_t mode) {
		return { .mode = mode, .pd = 0, .pu = 0, .fast = 0, .input = 0, .ifilt = 1 };
	}

	static constexpr PinConfig gpio_inout_with_pull(
		const uint32_t mode,
		const uint32_t pull_direction
	) {
		return {
			.mode = mode,
			.pd = (pull_direction == 0) ? 1U : 0U,
			.pu = (pull_direction == 1) ? 1U : 0U,
			.fast = 0,
			.input = 1,
			.ifilt = 1
		};
	}

	static constexpr PinConfig gpio_inout_with_pullup(const uint32_t mode) {
		return gpio_inout_with_pull(mode, 1);
	}

	static constexpr PinConfig gpio_inout_with_pulldown(const uint32_t mode) {
		return gpio_inout_with_pull(mode, 0);
	}

	static constexpr PinConfig gpio_out_with_pull(
		const uint32_t mode,
		const uint32_t pull_direction
	) {
		return {
			.mode = mode,
			.pd = (pull_direction == 0) ? 1U : 0U,
			.pu = (pull_direction == 1) ? 1U : 0U,
			.fast = 0,
			.input = 0,
			.ifilt = 1
		};
	}

	static constexpr PinConfig gpio_out_with_pulldown(const uint32_t mode) {
		return gpio_out_with_pull(mode, 0);
	}

	static constexpr PinConfig gpio_out_with_pullup(const uint32_t mode) {
		return gpio_out_with_pull(mode, 1);
	}

	static constexpr PinConfig sgpio_in_fast(const uint32_t mode) {
		return { .mode = mode, .pd = 0, .pu = 0, .fast = 0, .input = 1, .ifilt = 0 };
	}

	static constexpr PinConfig sgpio_out_fast_with_pull(
		const uint32_t mode,
		const uint32_t pull_direction
	) {
		return {
			.mode = mode,
			.pd = (pull_direction == 0) ? 1U : 0U,
			.pu = (pull_direction == 1) ? 1U : 0U,
			.fast = 1,
			.input = 0,
			.ifilt = 1
		};
	}

	static constexpr PinConfig sgpio_out_fast_with_pullup(const uint32_t mode) {
		return sgpio_out_fast_with_pull(mode, 1);
	}

	static constexpr PinConfig sgpio_inout_fast(const uint32_t mode) {
		return { .mode = mode, .pd = 0, .pu = 0, .fast = 1, .input = 1, .ifilt = 0 };
	}

	static constexpr PinConfig i2c(const uint32_t mode) {
		return { .mode = mode, .pd = 0, .pu = 0, .fast = 0, .input = 1, .ifilt = 1 };
	}

	static constexpr PinConfig spifi_sck(const uint32_t mode ) {
		return { .mode = mode, .pd = 0, .pu = 0, .fast = 1, .input = 1, .ifilt = 0 };
	}

	static constexpr PinConfig spifi_inout(const uint32_t mode) {
		return { .mode = mode, .pd = 0, .pu = 0, .fast = 1, .input = 1, .ifilt = 0 };
	}

	static constexpr PinConfig spifi_cs(const uint32_t mode) {
		return { .mode = mode, .pd = 0, .pu = 0, .fast = 1, .input = 0, .ifilt = 0 };
	}
};

struct Pin {
	// Pin() = delete;
	// Pin(const Pin&) = delete;
	// Pin(Pin&&) = delete;

	constexpr Pin(
		const uint8_t port,
		const uint8_t pad,
		const PinConfig initial_config
	) : _pin_port { port },
		_pin_pad { pad },
		_initial_config { initial_config }
	{
	}
/*
	constexpr Pin(
		const Pin& pin
	) : _pin_port { pin._pin_port },
		_pin_pad { pin._pin_pad },
		_initial_config { pin._initial_config }
	{
	}
*/
	void init() const {
		LPC_SCU->SFSP[_pin_port][_pin_pad] = _initial_config;
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
	uint16_t _initial_config;
};

struct GPIO {
	// GPIO() = delete;
	// GPIO(const GPIO& gpio) = delete;
	// GPIO(GPIO&&) = delete;

	constexpr GPIO(
		const Pin& pin,
		const ioportid_t gpio_port,
		const iopadid_t gpio_pad,
		const uint16_t gpio_mode
	) : _pin { pin },
		_gpio_port { gpio_port },
		_gpio_pad { gpio_pad },
		_gpio_mode { gpio_mode }
	{
	}
/*
	constexpr GPIO(
		const GPIO& gpio
	) : _pin { gpio._pin },
		_gpio_port { gpio._gpio_port },
		_gpio_pad { gpio._gpio_pad },
		_gpio_mode { gpio._gpio_mode }
	{
	}
*/
	constexpr ioportid_t port() const {
		return _gpio_port;
	}

	constexpr iopadid_t pad() const {
		return _gpio_pad;
	}

	constexpr Pin pin() const {
		return _pin;
	}

	void configure() const {
		_pin.mode(_gpio_mode);
	}

	uint_fast16_t mode() const {
		return _gpio_mode;
	}

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

	bool operator!=(const GPIO& other) const {
		return (port() != other.port()) || (pad() != other.pad());
	}

	const Pin _pin;
	const ioportid_t _gpio_port;
	const iopadid_t _gpio_pad;
	const uint16_t _gpio_mode;
};

#endif/*__GPIO_H__*/
