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
#include <array>
#include <string>
#include <string_view>

// #include "file.hpp"

// BARF! rtc::RTC is leaking everywhere.
// #include "lpc43xx_cpp.hpp"
// using namespace lpc43xx;

enum TimeFormat
{
    YMDHMS = 0,
    HMS = 1,
    HM = 2
};

const char unit_prefix[7]{'n', 'u', 'm', 0, 'k', 'M', 'G'};

using StringFormatBuffer = std::array<char, 24>;

/* Integer conversion without memory allocations. */
char *to_string_dec_int(int64_t n, StringFormatBuffer &buffer, size_t &length);
char *to_string_dec_uint(uint64_t n, StringFormatBuffer &buffer, size_t &length);

std::string to_string_dec_int(int64_t n);
std::string to_string_dec_uint(uint64_t n);

std::string to_string_bin(const uint32_t n, const uint8_t l = 0);
std::string to_string_dec_uint(const uint32_t n, const int32_t l, const char fill = ' ');
std::string to_string_dec_int(const int32_t n, const int32_t l, const char fill = 0);
std::string to_string_decimal(float decimal, int8_t precision);
std::string to_string_decimal_padding(float decimal, int8_t precision, const int32_t l);

std::string to_string_hex(uint64_t n, int32_t length);
std::string to_string_hex_array(uint8_t *array, int32_t length);

/* Helper to select length based on type size. */
template <typename T>
std::string to_string_hex(T n)
{
    return to_string_hex(n, sizeof(T) * 2); // Two digits/byte.
}

std::string to_string_freq(const uint64_t f);
std::string to_string_short_freq(const uint64_t f);
std::string to_string_rounded_freq(const uint64_t f, int8_t precision);
std::string to_string_time_ms(const uint32_t ms);

// TODO: wire standalone api: std::string to_string_datetime(const rtc::RTC &value, const TimeFormat format = YMDHMS);
// TODO: wire standalone api: std::string to_string_timestamp(const rtc::RTC &value);
// TODO: wire standalone api: std::string to_string_FAT_timestamp(const FATTimestamp &timestamp);

// Gets a human readable file size string.
std::string to_string_file_size(uint32_t file_size);

// Converts Mac Address to string.
std::string to_string_mac_address(const uint8_t *macAddress, uint8_t length, bool noColon);
std::string to_string_formatted_mac_address(const char *macAddress);
void generateRandomMacAddress(char *macAddress);

// TODO: wire standalone api: uint64_t readUntil(File &file, char *result, std::size_t maxBufferSize, char delimiter);

/* Scales 'n' to be a value less than 1000. 'base_unit' is the index of the unit from
 * 'unit_prefix' that 'n' is in initially. 3 is the index of the '1s' unit. */
std::string unit_auto_scale(double n, const uint32_t base_unit, uint32_t precision);
double get_decimals(double num, int16_t mult, bool round = false);

std::string trim(std::string_view str);  // Remove whitespace at ends.
std::string trimr(std::string_view str); // Remove trailing spaces
std::string truncate(std::string_view, size_t length);

/* Gets the int value for a character given the radix.
 * e.g. '5' => 5, 'D' => 13. Out of bounds => 0. */
uint8_t char_to_uint(char c, uint8_t radix = 10);

/* Gets the int value for a character given the radix.
 * e.g. 5 => '5', 13 => 'D'. Out of bounds => '\0'. */
char uint_to_char(uint8_t val, uint8_t radix = 10);

#endif /*__STRING_FORMAT_H__*/
