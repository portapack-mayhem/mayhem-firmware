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

#ifndef __CPLD_UPDATE_H__
#define __CPLD_UPDATE_H__

#include "portapack_cpld_data.hpp"

namespace portapack {
namespace cpld {

enum class CpldUpdateStatus {
    Success = 0,
    Idcode_check_failed = 1,
    Silicon_id_check_failed = 2,
    Program_failed = 3
};

CpldUpdateStatus update_if_necessary(const Config config);
CpldUpdateStatus update_autodetect(const Config config_rev_20150901, const Config config_rev_20170522);

} /* namespace cpld */
} /* namespace portapack */

namespace hackrf {
namespace cpld {

bool load_sram();
void load_sram_no_verify();  // Added to solve issue #637, "ghost" signal at RX, after using any TX App.
bool verify_eeprom();
void init_from_eeprom();

} /* namespace cpld */
} /* namespace hackrf */

#endif /*__CPLD_UPDATE_H__*/
