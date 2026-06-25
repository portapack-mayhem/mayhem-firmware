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

#include "gpio.hpp"
using namespace gpio_control;

#include "utility.hpp"

namespace rf {
namespace path {

namespace {

#ifdef PRALINE
/* PRALINE uses a simplified RF path with only 5 control signals.
 * The RF path architecture is completely different from HackRF One.
 */
struct PralineConfig {
    bool tx_en;
    bool mix_bypass_en;  // RF path mixer bypass (GPIO3[2])
    bool lpf_en;
    bool rf_amp_en;
    bool ant_bias_en;

    void apply() const {
        tx_enable.setState(tx_en);
        mix_bypass.setState(mix_bypass_en);
        lpf.setState(lpf_en);
        rf_amp_enable.setState(rf_amp_en);
        ant_bias.setState(ant_bias_en);
    }
};
#else

/* HackRF One uses GPIOs for RF path control, simplified for PortaPack internal use */
struct Config {
    bool tx;
    bool rx;
    bool mix_bypass_en;
    bool tx_mix_bp_en;
    bool rx_mix_bp_en;
    bool hpf_en;
    bool lpf_en;
    bool amp_bypass_en;
    bool tx_amp_en;
    bool rx_amp_en;

    constexpr Config(
        const Direction direction,
        const Band band,
        const bool amplify)
        : tx(direction == Direction::Transmit),
          rx(direction == Direction::Receive),
          mix_bypass_en(band == Band::Mid),
          tx_mix_bp_en((direction == Direction::Transmit) && (band == Band::Mid)),
          rx_mix_bp_en((direction == Direction::Receive) && (band == Band::Mid)),
          hpf_en(band == Band::High),
          lpf_en(band == Band::Low),
          amp_bypass_en(!amplify),
          tx_amp_en((direction == Direction::Transmit) && amplify),
          rx_amp_en((direction == Direction::Receive) && amplify) {
    }

    void apply() const {
        // TX/RX primary switches
        if (!hackrf_r9) {
            og_tx.setState(tx);
        }

        if (hackrf_r9) {
            r9_rx.setState(rx);
        } else {
            og_rx.setState(rx);
        }

        // Apply primary RF path states
        rx_mix_bypass.setState(mix_bypass_en);
        tx_mix_bp.setState(tx_mix_bp_en);
        rx_mix_bp.setState(rx_mix_bp_en);
        hpf.setState(hpf_en);
        lpf.setState(lpf_en);
        amp_bypass.setState(amp_bypass_en);
        tx_amp.setState(tx_amp_en);
        rx_amp.setState(rx_amp_en);

        tx_mix_bypass.setState(mix_bypass_en);
        tx_amp_pwr.setState(tx_amp_en);
        rx_amp_pwr.setState(rx_amp_en);
    }
};

#endif /* PRALINE */

} /* namespace */

void Path::init() {
#ifdef PRALINE
    /* Set safe initial state: RX mode, mixer enabled, LPF on, amp off, no bias */
    PralineConfig config = {
        .tx_en = false,
        .mix_bypass_en = true,
        .lpf_en = true,
        .rf_amp_en = false,
        .ant_bias_en = false};
    config.apply();
#else
    update();
#endif
}

void Path::set_direction(const Direction new_direction) {
    direction = new_direction;
    update();
}

void Path::set_band(const Band new_band) {
    band = new_band;
    _band = new_band;
    update();
}

void Path::set_rf_amp(const bool new_rf_amp) {
    rf_amp = new_rf_amp;
    update();
}

void Path::set_ant_bias(const bool new_ant_bias) {
    ant_bias = new_ant_bias;
    update();
}

bool Path::get_ant_bias() const {
    return ant_bias;
}

void Path::update() {
    /* 0 ^ 0 => 0 & 0 = 0 ^ 0 = 0 (no change)
     * 0 ^ 1 => 1 & 0 = 0 ^ 0 = 0 (ignore change to 1)
     * 1 ^ 0 => 1 & 1 = 1 ^ 1 = 0 (allow change to 0)
     * 1 ^ 1 => 0 & 1 = 0 ^ 1 = 1 (no change) */
#ifdef PRALINE
    /* PRALINE RF path control:
     * - tx_en: 1 for TX, 0 for RX
     * - mix_bypass: 1 to enable RF path mixer bypass (GPIO3[2])
     * - lpf_en: 1 for low band (< 2.4 GHz), 0 for high band
     * - rf_amp_en: 1 to enable RF amplifier
     */

    PralineConfig config;

    config.tx_en = (direction == Direction::Transmit);

    /* BUGFIX: PRALINE enables mixer on Low band! */
    config.mix_bypass_en = (band == Band::Low);

    config.lpf_en = (band == Band::Low);
    config.rf_amp_en = rf_amp;

    config.ant_bias_en = ant_bias;

    config.apply();
#else
    /* HackRF One RF path control - On the fly calculation */
    Config config(direction, band, rf_amp);
    config.apply();
#endif
}

}  // namespace path
}  // namespace rf