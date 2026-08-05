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

#ifndef __TUNING_H__
#define __TUNING_H__

#include "rf_path.hpp"

namespace tuning {
namespace config {

struct Config {
    /* Empty config to denote an error, in lieu of throwing an exception. */
    constexpr Config()
        : first_lo_frequency(0),
          second_lo_frequency(0),
          rf_path_band(rf::path::Band::Mid),
          mixer_invert(false),
          quarter_shift(0) {
    }

    constexpr Config(
        rf::Frequency first_lo_frequency,
        rf::Frequency second_lo_frequency,
        rf::path::Band rf_path_band,
        bool mixer_invert,
        uint8_t quarter_shift = 0)
        : first_lo_frequency(first_lo_frequency),
          second_lo_frequency(second_lo_frequency),
          rf_path_band(rf_path_band),
          mixer_invert(mixer_invert),
          quarter_shift(quarter_shift) {
    }

    bool is_valid() const {
        return (second_lo_frequency != 0);
    }

    const rf::Frequency first_lo_frequency;
    const rf::Frequency second_lo_frequency;
    const rf::path::Band rf_path_band;
    const bool mixer_invert;

    /* PRALINE only: FPGA RX quarter-rate shift mode, in the encoding the
     * gateware expects in the top two bits of register 0x03 (rx_pstep):
     *   0b00 = none, 0b11 = up, 0b01 = down.
     * Matches fpga_quarter_shift_mode_t in hackrf/firmware/common/fpga.h.
     * Always 0 on HackRF One (no FPGA). */
    const uint8_t quarter_shift;
};

/* afe_rate is the ADC sample rate in Hz (output rate << decimation), needed on
 * PRALINE to work out how far off centre the quarter-rate shift places the
 * analogue passband. Pass 0 (or leave defaulted) to disable the shift.
 * transmit selects the TX tuning table. Both are ignored on HackRF One. */
Config create(
    const rf::Frequency target_frequency,
    const uint32_t afe_rate = 0,
    const bool transmit = false);

} /* namespace config */
} /* namespace tuning */

#endif /*__TUNING_H__*/
