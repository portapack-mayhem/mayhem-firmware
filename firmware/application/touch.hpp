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

#ifndef __TOUCH_H__
#define __TOUCH_H__

#include <cstdint>
#include <cstddef>

#include <algorithm>
#include <array>
#include <functional>

#include "debounce.hpp"
#include "ui.hpp"

namespace touch {

using sample_t = uint16_t;

constexpr sample_t sample_max = 1023;

constexpr sample_t touch_threshold = sample_max / 5;

struct Samples {
	sample_t xp;
	sample_t xn;
	sample_t yp;
	sample_t yn;

	constexpr Samples(
	) : Samples { 0 }
	{
	}

	constexpr Samples(
		uint32_t v
	) : xp { static_cast<sample_t>(v) },
		xn { static_cast<sample_t>(v) },
		yp { static_cast<sample_t>(v) },
		yn { static_cast<sample_t>(v) }
	{
	}

	constexpr Samples(
		uint32_t xp,
		uint32_t xn,
		uint32_t yp,
		uint32_t yn
	) : xp { static_cast<sample_t>(xp) },
		xn { static_cast<sample_t>(xn) },
		yp { static_cast<sample_t>(yp) },
		yn { static_cast<sample_t>(yn) }
	{
	}

	Samples& operator +=(const Samples& r) {
		xp += r.xp;
		xn += r.xn;
		yp += r.yp;
		yn += r.yn;
		return *this;
	}

	Samples operator/(const unsigned int r) const {
		return {
			static_cast<sample_t>(xp / r),
			static_cast<sample_t>(xn / r),
			static_cast<sample_t>(yp / r),
			static_cast<sample_t>(yn / r)
		};
	}

	Samples operator>>(const size_t n) const {
		return {
			static_cast<sample_t>(xp >> n),
			static_cast<sample_t>(xn >> n),
			static_cast<sample_t>(yp >> n),
			static_cast<sample_t>(yn >> n)
		};
	}
};

struct Frame {
	Samples pressure { };
	Samples x { };
	Samples y { };
	bool touch { false };
};

struct Metrics {
	const float x;
	const float y;
	const float r;
};

Metrics calculate_metrics(const Frame& frame);

struct DigitizerPoint {
	int32_t x;
	int32_t y;
};

struct Calibration {
	/* Touch screen calibration matrix, based on article by Carlos E. Vidales:
	 * http://www.embedded.com/design/system-integration/4023968/How-To-Calibrate-Touch-Screens
	 */

	constexpr Calibration(
		const std::array<DigitizerPoint, 3>& s,
		const std::array<ui::Point, 3>& d
	) : k { (s[0].x - s[2].x) * (s[1].y - s[2].y) - (s[1].x - s[2].x) * (s[0].y - s[2].y) },
		a { (d[0].x() - d[2].x()) * (s[1].y - s[2].y) - (d[1].x() - d[2].x()) * (s[0].y - s[2].y) },
		b { (s[0].x - s[2].x) * (d[1].x() - d[2].x()) - (d[0].x() - d[2].x()) * (s[1].x - s[2].x) },
		c { s[0].y * (s[2].x * d[1].x() - s[1].x * d[2].x()) + s[1].y * (s[0].x * d[2].x() - s[2].x * d[0].x()) + s[2].y * (s[1].x * d[0].x() - s[0].x * d[1].x()) },
		d { (d[0].y() - d[2].y()) * (s[1].y - s[2].y) - (d[1].y() - d[2].y()) * (s[0].y - s[2].y) },
		e { (s[0].x - s[2].x) * (d[1].y() - d[2].y()) - (d[0].y() - d[2].y()) * (s[1].x - s[2].x) },
		f { s[0].y * (s[2].x * d[1].y() - s[1].x * d[2].y()) + s[1].y * (s[0].x * d[2].y() - s[2].x * d[0].y()) + s[2].y * (s[1].x * d[0].y() - s[0].x * d[1].y()) }
	{
	}

	constexpr Calibration() :
		Calibration(
			/* Values derived from one PortaPack H1 unit. */
			{ { { 256, 731 }, { 880, 432 }, { 568, 146 } } },
			{ { {  32,  48 }, { 208, 168 }, { 120, 288 } } }
		)
	{
	}

	ui::Point translate(const DigitizerPoint& p) const;

private:
	int32_t k;
	int32_t a;
	int32_t b;
	int32_t c;
	int32_t d;
	int32_t e;
	int32_t f;
};

template<size_t N>
class Filter {
public:
	constexpr Filter() = default;

	void reset() {
		history.fill(0);
		history_history = 0;
		accumulator = 0;
		n = 0;
	}

	void feed(const sample_t value) {
		accumulator = accumulator + value - history[n];
		history[n] = value;
		n = (n + 1) % history.size();

		history_history = (history_history << 1) | 1U;
	}

	int32_t value() const {
		return accumulator / N;
	}

	bool stable(const uint32_t bound) const {
		if( history_valid() ) {
			const auto minmax = std::minmax_element(history.cbegin(), history.cend());
			const auto min = *minmax.first;
			const auto max = *minmax.second;
			const uint32_t delta = max - min;
			return (delta < bound);
		} else {
			return false;
		}
	}

private:
	static constexpr uint32_t history_history_mask { (1U << N) - 1 };

	std::array<sample_t, N> history { };
	uint32_t history_history { 0 };
	int32_t accumulator { 0 };
	size_t n { 0 };

	bool history_valid() const {
		return (history_history & history_history_mask) == history_history_mask;
	}
};

class Manager {
public:
	std::function<void(ui::TouchEvent)> on_event { };

	void feed(const Frame& frame);

private:
	enum State {
		NoTouch,
		TouchDetected,
	};

	static constexpr float r_touch_threshold = 640;
	static constexpr size_t touch_count_threshold { 3 };
	static constexpr uint32_t touch_stable_bound { 8 };

	// Ensure filter length is equal or less than touch_count_threshold,
	// or coordinates from the last touch will be in the initial averages.
	Filter<touch_count_threshold> filter_x { };
	Filter<touch_count_threshold> filter_y { };

	//Debounce touch_debounce;

	State state { State::NoTouch };

	bool point_stable() const {
		return filter_x.stable(touch_stable_bound)
			&& filter_y.stable(touch_stable_bound);
	}

	ui::Point filtered_point() const;

	void touch_started() {
		fire_event(ui::TouchEvent::Type::Start);
	}

	void touch_moved() {
		fire_event(ui::TouchEvent::Type::Move);
	}

	void touch_ended() {
		fire_event(ui::TouchEvent::Type::End);
	}

	void fire_event(ui::TouchEvent::Type type) {
		if( on_event ) {
			on_event({ filtered_point(), type });
		}
	}
};

} /* namespace touch */

#endif/*__TOUCH_H__*/
