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
    struct settings_t {
        uint32_t baseband_bandwidth = max283x::filter::bandwidth_minimum;
        uint32_t sampling_rate = 3'072'000;
        uint32_t channel_bandwidth = 1;
        /* 35 should give approx 1m transmission range. */
        uint8_t tx_gain_db = 35;
        bool rf_amp = false;
    };

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

    bool rf_amp() const;
    void set_rf_amp(bool enabled);

    void set_antenna_bias();

    void enable();
    void disable();

    void initialize();

    void configure_from_app_settings(const app_settings::AppSettings& settings);

    /* Get access to the underlying settings to allow
     * values to be set directly without calling update. */
    settings_t& settings() { return settings_; }

   private:
    settings_t settings_{};
    bool enabled_ = false;
    SignalToken signal_token_tick_second{};

    void update_tuning_frequency();
    void update_baseband_bandwidth();
    void update_sampling_rate();
    void update_tx_gain();
    void update_rf_amp();
    void update_antenna_bias();
    void on_tick_second();
};

#endif /*__TRANSMITTER_MODEL_H__*/
