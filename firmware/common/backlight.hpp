/*
 * Copyright (C) 2017 Jared Boone, ShareBrained Technology, Inc.
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

#pragma once

#include <cstdint>

namespace portapack {

class Backlight {
public:
	using value_t = int_fast8_t;

	virtual ~Backlight() = default;

	virtual value_t levels() const = 0;

	virtual void set_level(const value_t value) = 0;
	virtual value_t level() const = 0;

	virtual void increase() = 0;
	virtual void decrease() = 0;

	virtual void on() = 0;
	virtual void off() = 0;

	virtual bool is_on() const = 0;
};

class BacklightBase : public Backlight {
public:
	void increase() override {
		set_level(level() + 1);
	}

	void decrease() override {
		set_level(level() - 1);
	}
};

class BacklightOnOff : public BacklightBase {
public:
	value_t levels() const override {
		return 1;
	}

	void set_level(const value_t) override {
	}

	value_t level() const override {
		return levels() - 1;
	}

	void on() override;
	void off() override;

	bool is_on() const override {
		return on_;
	}

private:
	static constexpr value_t maximum_level = 1;

	bool on_ { false };
};

class BacklightCAT4004 : public BacklightBase {
public:
	value_t levels() const override {
		return maximum_level + 1;
	}

	void set_level(const value_t value) override;

	value_t level() const override {
		return level_;
	}

	void on() override;
	void off() override;

	bool is_on() const override {
		return on_;
	}

private:
	static constexpr value_t initial_brightness = 25;
	static constexpr value_t maximum_level = 31;

	static constexpr uint32_t ticks_setup  = 204e6 * 10e-6;
	static constexpr uint32_t ms_pwrdwn    = 5;
	static constexpr uint32_t ticks_lo     = 204e6 * 1e-6;
	static constexpr uint32_t ticks_hi     = 204e6 * 1e-6;

	value_t level_ { initial_brightness };
	bool on_ { false };

	void pulses(value_t target);
	void pulse();
};

} /* namespace portapack */
