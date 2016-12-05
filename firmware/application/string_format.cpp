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
		if (n & (1 << (l - c)))
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

static void to_string_hex_internal(char* p, const uint64_t n, const int32_t l) {
	const uint32_t d = n & 0xf;
	p[l] = (d > 9) ? (d + 55) : (d + 48);
	if( l > 0 ) {
		to_string_hex_internal(p, n >> 4, l - 1);
	}
}

std::string to_string_hex(const uint64_t n, const int32_t l) {
	char p[32];
	to_string_hex_internal(p, n, l - 1);
	p[l] = 0;
	return p;
}

std::string to_string_datetime(const rtc::RTC& value) {
	return to_string_dec_uint(value.year(), 4, '0') + "/" +
		to_string_dec_uint(value.month(), 2, '0') + "/" +
		to_string_dec_uint(value.day(), 2, '0') + " " +
		to_string_dec_uint(value.hour(), 2, '0') + ":" +
		to_string_dec_uint(value.minute(), 2, '0') + ":" +
		to_string_dec_uint(value.second(), 2, '0');
}

std::string to_string_time(const rtc::RTC& value) {
	return to_string_dec_uint(value.hour(), 2, '0') + ":" +
		to_string_dec_uint(value.minute(), 2, '0');
}

std::string to_string_timestamp(const rtc::RTC& value) {
	return to_string_dec_uint(value.year(), 4, '0') +
		to_string_dec_uint(value.month(), 2, '0') +
		to_string_dec_uint(value.day(), 2, '0') +
		to_string_dec_uint(value.hour(), 2, '0') +
		to_string_dec_uint(value.minute(), 2, '0') +
		to_string_dec_uint(value.second(), 2, '0');
}
