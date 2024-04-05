/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __RECEIVER_MODEL_H__
#define __RECEIVER_MODEL_H__

#include <cstdint>
#include <cstddef>

#include "app_settings.hpp"
#include "max283x.hpp"
#include "message.hpp"
#include "rf_path.hpp"
#include "volume.hpp"

// TODO: consider a base class for ReceiverModel & TransmitterModel.
// There are multiple values that are actually shared by both.
class ReceiverModel {
   public:
    enum class Mode : uint8_t {
        AMAudio = 0,
        NarrowbandFMAudio = 1,
        WidebandFMAudio = 2,
        SpectrumAnalysis = 3,
        Capture = 4
    };

    struct settings_t {
        uint32_t baseband_bandwidth = max283x::filter::bandwidth_minimum;
        uint32_t sampling_rate = 3'072'000;
        rf::Frequency frequency_step = 25'000;
        rf::Frequency frequency_app_override = 0;
        uint8_t lna_gain_db = 32;
        uint8_t vga_gain_db = 32;
        bool rf_amp = false;
        Mode mode = Mode::NarrowbandFMAudio;
        uint8_t am_config_index = 0;
        uint8_t nbfm_config_index = 0;
        uint8_t wfm_config_index = 0;
        uint8_t squelch_level = 80;
    };

    /* The frequency to receive (no offset). */
    rf::Frequency target_frequency() const;
    void set_target_frequency(rf::Frequency f);

    uint32_t baseband_bandwidth() const;
    void set_baseband_bandwidth(uint32_t v);

    uint32_t sampling_rate() const;
    void set_sampling_rate(uint32_t v);

    rf::Frequency frequency_step() const;
    void set_frequency_step(rf::Frequency f);

    uint8_t lna() const;
    void set_lna(uint8_t v_db);

    uint8_t vga() const;
    void set_vga(uint8_t v_db);

    bool rf_amp() const;
    void set_rf_amp(bool enabled);

    Mode modulation() const;
    void set_modulation(Mode v);

    uint8_t am_configuration() const;
    void set_am_configuration(uint8_t n);

    uint8_t nbfm_configuration() const;
    void set_nbfm_configuration(uint8_t n);

    uint8_t wfm_configuration() const;
    void set_wfm_configuration(uint8_t n);

    uint8_t squelch_level() const;
    void set_squelch_level(uint8_t v);

    void set_antenna_bias();

    volume_t headphone_volume() const;
    void set_headphone_volume(volume_t v);

    /* Volume range 0-99, normalized for audio HW. */
    uint8_t normalized_headphone_volume() const;
    void set_normalized_headphone_volume(uint8_t v);

    void enable();
    void disable();

    /* Resets some members back to default. */
    void initialize();

    void set_configuration_without_update(
        Mode new_mode,
        rf::Frequency new_frequency_step,
        size_t new_am_config_index,
        size_t new_nbfm_config_index,
        size_t new_wfm_config_index,
        uint8_t new_squelch_level);

    void configure_from_app_settings(const app_settings::AppSettings& settings);

    /* Get access to the underlying settings to allow
     * values to be set directly without calling update. */
    settings_t& settings() { return settings_; }

   private:
    settings_t settings_{};
    bool enabled_ = false;

    int32_t tuning_offset();

    void update_tuning_frequency();
    void update_baseband_bandwidth();
    void update_sampling_rate();
    void update_lna();
    void update_vga();
    void update_rf_amp();

    void update_modulation();
    void update_am_configuration();
    void update_nbfm_configuration();
    void update_wfm_configuration();

    void update_antenna_bias();
    void update_headphone_volume();
};

#endif /*__RECEIVER_MODEL_H__*/
