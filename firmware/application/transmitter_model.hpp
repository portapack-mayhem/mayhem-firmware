/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
 * Copyright (C) 2023 Kyle Reed
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

#ifndef __TRANSMITTER_MODEL_H__
#define __TRANSMITTER_MODEL_H__

#include <cstdint>
#include <cstddef>

#include "app_settings.hpp"
#include "max2837.hpp"
#include "message.hpp"
#include "receiver_model.hpp"
#include "rf_path.hpp"
#include "signal.hpp"

class TransmitterModel {
   public:
    static constexpr uint32_t default_baseband_bandwidth = max283x::filter::bandwidth_minimum;
    static constexpr uint32_t default_sampling_rate = 3'072'000;
    static constexpr uint32_t default_channel_bandwidth = 1;
    /* 35 should give approx 1m transmission range. */
    static constexpr uint8_t default_tx_gain = 35;
    static constexpr uint8_t default_gain = 0;
    static constexpr bool default_amp = false;

    /* The frequency to transmit on. */
    rf::Frequency target_frequency() const;
    void set_target_frequency(rf::Frequency f);

    uint32_t baseband_bandwidth() const;
    void set_baseband_bandwidth(uint32_t v);

    // TODO: Doesn't actually affect radio.
    uint32_t channel_bandwidth() const;
    void set_channel_bandwidth(uint32_t v);

    uint32_t sampling_rate() const;
    void set_sampling_rate(uint32_t v);

    uint8_t tx_gain() const;
    void set_tx_gain(uint8_t v_db);

    // TODO: does this make sense on TX?
    uint8_t lna() const;
    void set_lna(uint8_t v_db);

    // TODO: does this make sense on TX?
    uint8_t vga() const;
    void set_vga(uint8_t v_db);

    bool rf_amp() const;
    void set_rf_amp(bool enabled);

    void set_antenna_bias();

    void enable();
    void disable();

    void initialize();

    void configure_from_app_settings(const app_settings::AppSettings& settings);

   private:
    uint32_t baseband_bandwidth_ = default_baseband_bandwidth;
    uint32_t sampling_rate_ = default_sampling_rate;
    uint32_t channel_bandwidth_ = default_channel_bandwidth;
    uint8_t tx_gain_db_ = default_tx_gain;
    uint8_t lna_gain_db_ = default_gain;
    uint8_t vga_gain_db_ = default_gain;
    bool rf_amp_ = default_amp;
    bool enabled_ = false;
    SignalToken signal_token_tick_second{};

    void update_tuning_frequency();
    void update_baseband_bandwidth();
    void update_sampling_rate();
    void update_tx_gain();
    void update_lna();
    void update_vga();
    void update_rf_amp();
    void update_antenna_bias();
    void on_tick_second();
};

#endif /*__TRANSMITTER_MODEL_H__*/
