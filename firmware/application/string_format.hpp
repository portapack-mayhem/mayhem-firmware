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

#ifndef __STRING_FORMAT_H__
#define __STRING_FORMAT_H__

#include <cstdint>
#include <string>

// BARF! rtc::RTC is leaking everywhere.
#include "lpc43xx_cpp.hpp"
using namespace lpc43xx;

// TODO: Allow l=0 to not fill/justify? Already using this way in ui_spectrum.hpp...
std::string to_string_bin(const uint32_t n, const uint8_t l = 0);
std::string to_string_dec_uint(const uint32_t n, const int32_t l = 0, const char fill = 0);
std::string to_string_dec_int(const int32_t n, const int32_t l = 0, const char fill = 0);
std::string to_string_hex(const uint64_t n, const int32_t l = 0);

std::string to_string_datetime(const rtc::RTC& value);
std::string to_string_time(const rtc::RTC& value);
std::string to_string_timestamp(const rtc::RTC& value);

#endif/*__STRING_FORMAT_H__*/
