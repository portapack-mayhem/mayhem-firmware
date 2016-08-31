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

#include "rf_path.hpp"

#include <array>
#include <initializer_list>

#include "hackrf_gpio.hpp"
using namespace hackrf::one;

#include "utility.hpp"

namespace rf {
namespace path {

namespace {

using GPIOs = std::array<GPIO, 13>;

/* TODO: ARM GCC 4.8 2014q3 doesn't like this array inside struct Config.
 * No idea why.
 */
constexpr GPIOs gpios {
	gpio_tx,
	gpio_rx,
	gpio_mix_bypass,
	gpio_not_mix_bypass,
	gpio_tx_mix_bp,
	gpio_rx_mix_bp,
	gpio_hp,
	gpio_lp,
	gpio_amp_bypass,
	gpio_tx_amp,
	gpio_not_tx_amp_pwr,
	gpio_rx_amp,
	gpio_not_rx_amp_pwr,
};

struct Config {
	using base_type = uint16_t;

	union {
		struct {
			bool tx				: 1;
			bool rx				: 1;
			bool mix_bypass		: 1;
			bool not_mix_bypass	: 1;
			bool tx_mix_bp		: 1;
			bool rx_mix_bp		: 1;
			bool hp				: 1;
			bool lp 			: 1;
			bool amp_bypass		: 1;
			bool tx_amp			: 1;
			bool not_tx_amp		: 1;
			bool rx_amp			: 1;
			bool not_rx_amp		: 1;
		};
		base_type w;
	};

	constexpr Config(
		const Direction direction,
		const Band band,
		const bool amplify
	) :
		tx(direction == Direction::Transmit),
		rx(direction == Direction::Receive),
		mix_bypass(band == Band::Mid),
		not_mix_bypass(band != Band::Mid),
		tx_mix_bp((direction == Direction::Transmit) && (band == Band::Mid)),
		rx_mix_bp((direction == Direction::Receive) && (band == Band::Mid)),
		hp(band == Band::High),
		lp(band == Band::Low),
		amp_bypass(!amplify),
		tx_amp((direction == Direction::Transmit) && amplify),
		not_tx_amp(!tx_amp),
		rx_amp((direction == Direction::Receive) && amplify),
		not_rx_amp(!rx_amp)
	{
	}

	constexpr Config(
	) : Config(Direction::Receive, Band::Mid, false)
	{
	}

	constexpr Config(
		const base_type w
	) : w(w)
	{
	}

	constexpr Config operator^(const Config& r) const {
		return w ^ r.w;
	}

	constexpr Config operator&(const Config& r) const {
		return w & r.w;
	}

	constexpr bool operator[](const size_t n) const {
		return (w >> n) & 1;
	}

	static void gpio_init() {
		for(auto gpio : gpios) {
			gpio.output();
		}
	}

	void apply() const {
		/* NOTE: Assumes order in gpios[] and Config bitfield match. */
		for(size_t n=0; n<gpios.size(); n++) {
			gpios[n].write((*this)[n]);
		}
	}
};

using ConfigAmp = std::array<Config, 2>;
using ConfigDirection = std::array<ConfigAmp, 2>;
using ConfigBand = std::array<ConfigDirection, 3>;

constexpr ConfigAmp config_amp(
	const Direction direction,
	const Band band
) {
	return { {
		{ .direction = direction, .band = band, .amplify = false },
		{ .direction = direction, .band = band, .amplify = true  },
	} };
}

constexpr ConfigDirection config_rx_tx(
	const Band band
) {
	return {
		config_amp(Direction::Receive, band),
		config_amp(Direction::Transmit, band),
	};
}

constexpr ConfigBand config_band() {
	return {
		config_rx_tx(Band::Low),
		config_rx_tx(Band::Mid),
		config_rx_tx(Band::High),
	};
}

constexpr ConfigBand config_table = config_band();

static_assert(sizeof(config_table) == sizeof(Config::base_type) * 3 * 2 * 2, "rf path config table unexpected size");

constexpr Config get_config(
	const Direction direction,
	const Band band,
	const bool amplify
) {
	return config_table[toUType(band)][toUType(direction)][amplify ? 1 : 0];
}

} /* namespace */

void Path::init() {
	update();
	Config::gpio_init();
}

void Path::set_direction(const Direction new_direction) {
	direction = new_direction;
	update();
}

void Path::set_band(const Band new_band) {
	band = new_band;
	update();
}

void Path::set_rf_amp(const bool new_rf_amp) {
	rf_amp = new_rf_amp;
	update();
}

void Path::update() {
	/* 0 ^ 0 => 0 & 0 = 0 ^ 0 = 0 (no change)
	 * 0 ^ 1 => 1 & 0 = 0 ^ 0 = 0 (ignore change to 1)
	 * 1 ^ 0 => 1 & 1 = 1 ^ 1 = 0 (allow change to 0)
	 * 1 ^ 1 => 0 & 1 = 0 ^ 1 = 1 (no change)
	 */
	//const Config changed = _config ^ config_next;
	//const Config turned_off = _config & changed;

	/* In transition, ignore the bits that are turning on. So this transition phase
	 * only turns off signals. It doesn't turn on signals.
	 */
	//const Config transition_config = _config ^ turned_off;
	//update_signals(transition_config);

	/* Move to the final state by turning on required signals. */
	const auto config = get_config(direction, band, rf_amp);
	config.apply();
}

} /* path */
} /* rf */
