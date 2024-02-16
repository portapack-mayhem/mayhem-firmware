/*
 * Copyright (C) 2023 Great Scott Gadgets
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

#ifndef __MAX283X_H__
#define __MAX283X_H__

#include <array>

#include "rf_path.hpp"

namespace max283x {

/*************************************************************************/

namespace lo {

constexpr std::array<rf::FrequencyRange, 4> band{{
    {2170000000, 2400000000},
    {2400000000, 2500000000},
    {2500000000, 2600000000},
    {2600000000, 2740000000},
}};

} /* namespace lo */

/*************************************************************************/

namespace lna {

constexpr range_t<int8_t> gain_db_range{0, 40};
constexpr int8_t gain_db_step = 8;

} /* namespace lna */

/*************************************************************************/

namespace vga {

constexpr range_t<int8_t> gain_db_range{0, 62};
constexpr int8_t gain_db_step = 2;

} /* namespace vga */

/*************************************************************************/

namespace tx {

constexpr range_t<int8_t> gain_db_range{0, 47};
constexpr int8_t gain_db_step = 1;
}  // namespace tx

/*************************************************************************/

namespace filter {

constexpr std::array<uint32_t, 16> bandwidths{
    /* Assumption: these values are in ascending order */
    1750000,
    2500000, /* Some documentation says 2.25MHz */
    3500000,
    5000000,
    5500000,
    6000000,
    7000000,
    8000000,
    9000000,
    10000000,
    12000000,
    14000000,
    15000000,
    20000000,
    24000000,
    28000000,
};

constexpr auto bandwidth_minimum = bandwidths[0];
constexpr auto bandwidth_maximum = bandwidths[bandwidths.size() - 1];

} /* namespace filter */

/*************************************************************************/

enum Mode {  // MAX283x Operating modes.
    Shutdown,
    Standby,
    Receive,
    Transmit,
    Rx_Calibration,  // just add the sequential enum of those two CAL operating modes .
    Tx_Calibration,
};

using reg_t = uint16_t;
using address_t = uint8_t;

class MAX283x {
   public:
    virtual ~MAX283x() = default;

    virtual void init();
    virtual void set_mode(const Mode mode);

    virtual void set_tx_vga_gain(const int_fast8_t db);
    virtual void set_lna_gain(const int_fast8_t db);
    virtual void set_vga_gain(const int_fast8_t db);
    virtual void set_lpf_rf_bandwidth_rx(const uint32_t bandwidth_minimum);
    virtual void set_lpf_rf_bandwidth_tx(const uint32_t bandwidth_minimum);

    virtual bool set_frequency(const rf::Frequency lo_frequency);

    virtual void set_rx_lo_iq_calibration(const size_t v);
    virtual void set_tx_LO_iq_phase_calibration(const size_t v);

    virtual void set_rx_buff_vcm(const size_t v);

    virtual int8_t temp_sense();

    virtual reg_t read(const address_t reg_num);
    virtual void write(const address_t reg_num, const reg_t value);
};

}  // namespace max283x

#endif /*__MAX283X_H__*/