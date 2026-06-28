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
#include "platform.hpp"
#include "utility.hpp"
#include "gpio.hpp"
using namespace gpio_control;

namespace rf {
namespace path {

void Path::init() {
    /* Set safe initial default states */
    direction = Direction::Receive;
    rf_amp_en = false;
    ant_bias_en = false;

#ifdef PRALINE
    band = Band::Low;
#else
    band = Band::Mid;
#endif

    update();
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
    rf_amp_en = new_rf_amp;
    update();
}

void Path::set_ant_bias(const bool new_ant_bias) {
    ant_bias_en = new_ant_bias;
    update();
}

bool Path::get_ant_bias() const {
    return ant_bias_en;
}

void Path::update() {
    /* 0 ^ 0 => 0 & 0 = 0 ^ 0 = 0 (no change)
     * 0 ^ 1 => 1 & 0 = 0 ^ 0 = 0 (ignore change to 1)
     * 1 ^ 0 => 1 & 1 = 1 ^ 1 = 0 (allow change to 0)
     * 1 ^ 1 => 0 & 1 = 0 ^ 1 = 1 (no change) */
    const bool is_tx = (direction == Direction::Transmit);

#ifdef PRALINE
    // PRALINE specific RF path control directly applied to pins.
    // Active-low pin inversion is handled internally inside setState().

    tx_enable.setState(is_tx);

    // On the PRALINE board, the mixer is used ONLY on the Low band.
    // Since setState() internally handles the active-low (MIX_ENABLE_N) hardware inversion,
    // we simply pass 'true' to enable the mixer on Low band, and 'false' for Mid/High bands.

    mix_bypass.setState(band == Band::Low);

    lpf.setState(band == Band::Low);
    rf_amp_enable.setState(rf_amp_en);
    ant_bias.setState(ant_bias_en);

#else

    const bool is_rx = (direction == Direction::Receive);

    // HackRF One (OG & R9) RF path control
    const bool mix_bypass_en = (band == Band::Mid);
    const bool amplify = rf_amp_en;

    // Primary TX/RX routing switches
    if (!hackrf_r9) {
        og_tx.setState(is_tx);
    }

    if (hackrf_r9) {
        r9_rx.setState(is_rx);  // Single pin handles directional switching on R9
    } else {
        og_rx.setState(is_rx);
    }

    // RF path switch configuration matrix
    rx_mix_bypass.setState(mix_bypass_en);
    tx_mix_bp.setState(is_tx && mix_bypass_en);
    rx_mix_bp.setState(is_rx && mix_bypass_en);

    hpf.setState(band == Band::High);
    lpf.setState(band == Band::Low);

    amp_bypass.setState(!amplify);
    tx_amp.setState(is_tx && amplify);
    rx_amp.setState(is_rx && amplify);

    tx_mix_bypass.setState(mix_bypass_en);
    tx_amp_pwr.setState(is_tx && amplify);
    rx_amp_pwr.setState(is_rx && amplify);

    ant_bias.setState(ant_bias_en);
#endif
}

}  // namespace path
}  // namespace rf