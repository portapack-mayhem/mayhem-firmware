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

#ifndef __RADIO_H__
#define __RADIO_H__

#include "rf_path.hpp"

#include <cstdint>
#include <cstddef>

/* Direct access to the radio. Setting values incorrectly can damage
 * the device. Applications should use ReceiverModel or TransmitterModel
 * instead of calling these functions directly. */
namespace radio {

struct Configuration {
    rf::Frequency tuning_frequency;
    uint32_t baseband_rate;
    uint32_t baseband_filter_bandwidth;
    rf::Direction direction;
    bool rf_amp;
    int8_t lna_gain;
    int8_t vga_gain;
};

void init();

void set_direction(const rf::Direction new_direction);
bool set_tuning_frequency(const rf::Frequency frequency);
void set_rf_amp(const bool rf_amp);
void set_lna_gain(const int_fast8_t db);
void set_vga_gain(const int_fast8_t db);
void set_tx_gain(const int_fast8_t db);
void set_baseband_filter_bandwidth_rx(const uint32_t bandwidth_minimum);
void set_baseband_filter_bandwidth_tx(const uint32_t bandwidth_minimum);
void set_baseband_rate(const uint32_t rate);
void set_antenna_bias(const bool on);
void set_tx_max283x_iq_phase_calibration(const size_t v);
void set_rx_max283x_iq_phase_calibration(const size_t v);

/* Use ReceiverModel or TransmitterModel instead. */
// void enable(Configuration configuration);
// void configure(Configuration configuration);
void disable();

#ifdef PRALINE
void invalidate_spi_config();
void set_rx_buff_vcm(const size_t v);
#endif

namespace debug {

namespace first_if {

uint32_t register_read(const size_t register_number);
void register_write(const size_t register_number, uint32_t value);

#ifdef PRALINE
struct TuningInfo {
    uint32_t requested_freq_mhz;
    uint32_t calculated_vco_mhz;
    uint32_t expected_n;
    uint8_t expected_lodiv;
    uint8_t expected_presc;
    bool was_called;
    uint32_t calc_lo_freq_mhz;
    uint32_t calc_vco_inside_mhz;
    uint8_t calc_lodiv_log2;
    uint8_t calc_presc_log2;
    uint64_t calc_n_q24;
};

TuningInfo get_tuning_info();
#endif

} /* namespace first_if */

namespace second_if {

uint32_t register_read(const size_t register_number);
void register_write(const size_t register_number, uint32_t value);

// TODO: This belongs somewhere else.
int8_t temp_sense();

} /* namespace second_if */

namespace rf_path_info {

rf::path::Band get_current_band();

} /* namespace rf_path_info */

#ifdef PRALINE
namespace fpga {

uint32_t register_read(const size_t register_number);
void register_write(const size_t register_number, uint32_t value);
void init();

} /* namespace fpga */

/* State tracking - GPIO pins are write-only so we cache last known state */
rf::Direction get_cached_direction();
bool get_cached_rf_amp();
int_fast8_t get_cached_lna_gain();
int_fast8_t get_cached_vga_gain();
#endif

namespace sgpio {

uint32_t register_read(const size_t register_number);

} /* namespace sgpio */

} /* namespace debug */

} /* namespace radio */

#endif /*__RADIO_H__*/