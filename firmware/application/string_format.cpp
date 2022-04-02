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

#include "string_format.hpp"

static char* to_string_dec_uint_internal(
	char* p,
	uint32_t n
) {
	*p = 0;
	auto q = p;

	do {
		const uint32_t d = n % 10;
		const char c = d + 48;
		*(--q) = c;
		n /= 10;
	} while( n != 0 );

	return q;
}

static char* to_string_dec_uint_pad_internal(
	char* const term,
	const uint32_t n,
	const int32_t l,
	const char fill
) {
	auto q = to_string_dec_uint_internal(term, n);

	if( fill ) {
		while( (term - q) < l ) {
			*(--q) = fill;
		}
	}

	return q;
}

std::string to_string_bin(
	const uint32_t n,
	const uint8_t l)
{
	char p[33];
	for (uint8_t c = 0; c < l; c++) {
		if (n & (1 << (l - 1 - c)))
			p[c] = '1';
		else
			p[c] = '0';
	}
	p[l] = 0;
	return p;
}

std::string to_string_dec_uint(
	const uint32_t n,
	const int32_t l,
	const char fill
) {
	char p[16];
	auto term = p + sizeof(p) - 1;
	auto q = to_string_dec_uint_pad_internal(term, n, l, fill);

	// Right justify.
	while( (term - q) < l ) {
		*(--q) = ' ';
	}

	return q;
}

std::string to_string_dec_int(
	const int32_t n,
	const int32_t l,
	const char fill
) {
	const size_t negative = (n < 0) ? 1 : 0;
	uint32_t n_abs = negative ? -n : n;

	char p[16];
	auto term = p + sizeof(p) - 1;
	auto q = to_string_dec_uint_pad_internal(term, n_abs, l - negative, fill);

	// Add sign.
	if( negative ) {
		*(--q) = '-';
	}

	// Right justify.
	while( (term - q) < l ) {
		*(--q) = ' ';
	}

	return q;
}

std::string to_string_decimal(float decimal, int8_t precision) {
	double integer_part;
	double fractional_part;

	std::string result;
	
	fractional_part = modf(decimal, &integer_part) * pow(10, precision);

	if (fractional_part < 0) {
		fractional_part = -fractional_part;
	}

	result = to_string_dec_int(integer_part) + "." + to_string_dec_uint(fractional_part, precision, '0');

	return result;
}

std::string to_string_freq(const uint64_t f) {
	auto final_str = to_string_dec_int(f / 1000000,4) + to_string_dec_int(f % 1000000, 6, '0');
	return final_str;
}

std::string to_string_short_freq(const uint64_t f) {
	auto final_str = to_string_dec_int(f / 1000000,4) + "." + to_string_dec_int((f / 100) % 10000, 4, '0');
	return final_str;
}

std::string to_string_time_ms(const uint32_t ms) {
	std::string final_str { "" };
	
	if (ms < 1000) {
		final_str = to_string_dec_uint(ms) + "ms";
	} else {
		auto seconds = ms / 1000;
		
		if (seconds >= 60)
			final_str = to_string_dec_uint(seconds / 60) + "m";
		
		return final_str + to_string_dec_uint(seconds % 60) + "s";
	}
	
	return final_str;
}

static void to_string_hex_internal(char* p, const uint64_t n, const int32_t l) {
	const uint32_t d = n & 0xf;
	p[l] = (d > 9) ? (d + 55) : (d + 48);
	if( l > 0 ) {
		to_string_hex_internal(p, n >> 4, l - 1);
	}
}

std::string to_string_hex(const uint64_t n, int32_t l) {
	char p[32];
	
	l = std::min(l, 31L);
	to_string_hex_internal(p, n, l - 1);
	p[l] = 0;
	return p;
}

std::string to_string_hex_array(uint8_t * const array, const int32_t l) {
	std::string str_return = "";
	uint8_t bytes;
	
	for (bytes = 0; bytes < l; bytes++)
		str_return += to_string_hex(array[bytes], 2);
	
	return str_return;
}

std::string to_string_datetime(const rtc::RTC& value, const TimeFormat format) {
	std::string string { "" };
	
	if (format == YMDHMS) {
		string += to_string_dec_uint(value.year(), 4) + "-" +
					to_string_dec_uint(value.month(), 2, '0') + "-" +
					to_string_dec_uint(value.day(), 2, '0') + " ";
	}
	
	string += to_string_dec_uint(value.hour(), 2, '0') + ":" +
				to_string_dec_uint(value.minute(), 2, '0');
	
	if ((format == YMDHMS) || (format == HMS))
		string += ":" + to_string_dec_uint(value.second(), 2, '0');
	
	return string;
}

std::string to_string_timestamp(const rtc::RTC& value) {
	return to_string_dec_uint(value.year(), 4, '0') +
		to_string_dec_uint(value.month(), 2, '0') +
		to_string_dec_uint(value.day(), 2, '0') +
		to_string_dec_uint(value.hour(), 2, '0') +
		to_string_dec_uint(value.minute(), 2, '0') +
		to_string_dec_uint(value.second(), 2, '0');
}

std::string to_string_FAT_timestamp(const FATTimestamp& timestamp) {
	return to_string_dec_uint((timestamp.FAT_date >> 9) + 1980) + "-" +
		to_string_dec_uint((timestamp.FAT_date >> 5) & 0xF, 2, '0') + "-" +
		to_string_dec_uint((timestamp.FAT_date & 0x1F), 2, '0') + " " +
		to_string_dec_uint((timestamp.FAT_time >> 11), 2, '0') + ":" +
		to_string_dec_uint((timestamp.FAT_time >> 5) & 0x3F, 2, '0');
}

std::string unit_auto_scale(double n, const uint32_t base_nano, uint32_t precision) {
	const uint32_t powers_of_ten[5] = { 1, 10, 100, 1000, 10000 };
	std::string string { "" };
	uint32_t prefix_index = base_nano;
	double integer_part;
	double fractional_part;
	
	precision = std::min((uint32_t)4, precision);
	
	while (n > 1000) {
		n /= 1000.0;
		prefix_index++;
	}
	
	fractional_part = modf(n, &integer_part) * powers_of_ten[precision];
	if (fractional_part < 0)
		fractional_part = -fractional_part;
	
	string = to_string_dec_int(integer_part);
	if (precision)
		string += '.' + to_string_dec_uint(fractional_part, precision);
	
	if (prefix_index != 3)
		string += unit_prefix[prefix_index];
	
	return string;
}

double get_decimals(double num, int16_t mult, bool round) {
	num -= int(num);				//keep decimals only
	num *= mult;					//Shift decimals into integers
	if (!round) return num;				
	int16_t intnum = int(num);		//Round it up if necessary
	num -= intnum;					//Get decimal part
	if (num > .5) intnum++;			//Round up
	return intnum;
}
