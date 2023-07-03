/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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
#include "message.hpp"
#include "rf_path.hpp"
#include "max283x.hpp"
#include "volume.hpp"

// TODO: consider a base class for ReceiverModel & TransmitterModel.
// There are multiple values that are actually shared by both.
class ReceiverModel {
   public:
    enum class Mode : uint8_t{
        AMAudio = 0,
        NarrowbandFMAudio = 1,
        WidebandFMAudio = 2,
        SpectrumAnalysis = 3,
        Capture = 4
    };

    constexpr uint32_t default_baseband_bandwidth = max283x::filter::bandwidth_minimum;
    constexpr uint32_t default_sampling_rate = 3'072'000;
    constexpr rf::Frequency default_frequency_step = 25'000;
    constexpr uint8_t default_gain = 32;
    constexpr bool default_amp = false;
    constexpr Mode default_modulation = Mode::NarrowbandFMAudio;
    constexpr uint8_t default_squelch = 80;

    /* The frequency to receive (no offset). */
    rf::Frequency target_frequency() const;
    void set_target_frequency(rf::Frequency f);

    rf::Frequency frequency_step() const;
    void set_frequency_step(rf::Frequency f);

    void set_antenna_bias();

    bool rf_amp() const;
    void set_rf_amp(bool enabled);

    int32_t lna() const;
    void set_lna(int32_t v_db);

    uint32_t baseband_bandwidth() const;
    void set_baseband_bandwidth(uint32_t v);

    int32_t vga() const;
    void set_vga(int32_t v_db);

    uint32_t sampling_rate() const;
    void set_sampling_rate(uint32_t v);

    Mode modulation() const;
    void set_modulation(Mode v);

    volume_t headphone_volume() const;
    void set_headphone_volume(volume_t v);

    /* Volume range 0-99, normalized for audio HW. */
    uint8_t normalized_headphone_volume() const;
    void set_normalized_headphone_volume(uint8_t v);

    uint8_t squelch_level() const;
    void set_squelch_level(uint8_t v);

    void enable();
    void disable();

    size_t am_configuration() const;
    void set_am_configuration(const size_t n);

    size_t nbfm_configuration() const;
    void set_nbfm_configuration(const size_t n);

    size_t wfm_configuration() const;
    void set_wfm_configuration(const size_t n);

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

   private:
    uint32_t baseband_bandwidth_ = default_baseband_bandwidth;
    uint32_t sampling_rate_ = default_sampling_rate;
    rf::Frequency frequency_step_ = default_frequency_step;
    uint8_t lna_gain_db_ = default_gain;
    uint8_t vga_gain_db_ = default_gain;
    bool rf_amp_ = default_amp;
    Mode mode_ = default_modulation;
    uint8_t am_config_index = 0;
    uint8_t nbfm_config_index = 0;
    uint8_t wfm_config_index = 0;
    uint8_t squelch_level_ = default_squelch;
    bool enabled_ = false;

    int32_t tuning_offset();

    void update_tuning_frequency();
    void update_antenna_bias();
    void update_rf_amp();
    void update_lna();
    void update_baseband_bandwidth();
    void update_vga();
    void update_sampling_rate();
    void update_headphone_volume();

    void update_modulation();
    void update_am_configuration();
    void update_nbfm_configuration();
    void update_wfm_configuration();
};

#endif /*__RECEIVER_MODEL_H__*/
