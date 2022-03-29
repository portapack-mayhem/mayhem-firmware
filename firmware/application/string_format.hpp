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

#include "file.hpp"

// BARF! rtc::RTC is leaking everywhere.
#include "lpc43xx_cpp.hpp"
using namespace lpc43xx;

enum TimeFormat {
	YMDHMS = 0,
	HMS = 1,
	HM = 2
};

const char unit_prefix[7] { 'n', 'u', 'm', 0, 'k', 'M', 'G' };

// TODO: Allow l=0 to not fill/justify? Already using this way in ui_spectrum.hpp...
std::string to_string_bin(const uint32_t n, const uint8_t l = 0);
std::string to_string_dec_uint(const uint32_t n, const int32_t l = 0, const char fill = ' ');
std::string to_string_dec_int(const int32_t n, const int32_t l = 0, const char fill = 0);
std::string to_string_decimal(float decimal, int8_t precision);

std::string to_string_hex(const uint64_t n, const int32_t l = 0);
std::string to_string_hex_array(uint8_t * const array, const int32_t l = 0);

std::string to_string_freq(const uint64_t f);
std::string to_string_short_freq(const uint64_t f);
std::string to_string_time_ms(const uint32_t ms);

std::string to_string_datetime(const rtc::RTC& value, const TimeFormat format = YMDHMS);
std::string to_string_timestamp(const rtc::RTC& value);
std::string to_string_FAT_timestamp(const FATTimestamp& timestamp);

std::string unit_auto_scale(double n, const uint32_t base_nano, uint32_t precision);
double get_decimals(double num, int16_t mult,  bool round = false); //euquiq added
#endif/*__STRING_FORMAT_H__*/
