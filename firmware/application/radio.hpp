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

#include "rffc507x.hpp"
#include "max2837.hpp"

namespace radio {

void init();

void set_direction(const rf::Direction new_direction);
bool set_tuning_frequency(const rf::Frequency frequency);
void set_rf_amp(const bool rf_amp);
void set_lna_gain(const int_fast8_t db);
void set_vga_gain(const int_fast8_t db);
void set_sampling_frequency(const uint32_t frequency);
void set_baseband_filter_bandwidth(const uint32_t bandwidth_minimum);
void set_baseband_decimation_by(const size_t n);

void streaming_enable();
void streaming_disable();
void disable();

extern rffc507x::RFFC507x first_if;
extern max2837::MAX2837 second_if;

} /* namespace radio */

#endif/*__RADIO_H__*/
