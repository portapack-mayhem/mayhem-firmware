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

#include <algorithm>
#include <array>
#include <complex>
#include <cstdint>
#include <cstddef>
#include <initializer_list>
#include <memory>
#include <string_view>
#include <type_traits>

using namespace std::literals;

/* This takes a pointer to the end of a buffer
 * and fills it backwards towards the front.
 * The return value 'q' is a pointer to the start.
 * TODO: use std::array for all this. */
template <typename Int>
static char *to_string_dec_uint_internal(
    char *p,
    Int n)
{
    *p = 0;
    auto q = p;

    do
    {
        *(--q) = n % 10 + '0';
        n /= 10;
    } while (n != 0);

    return q;
}

static char *to_string_dec_uint_pad_internal(
    char *const term,
    const uint32_t n,
    const int32_t l,
    const char fill)
{
    auto q = to_string_dec_uint_internal(term, n);

    // Fill with padding if needed.
    // TODO: use std::array instead. There's no
    // bounds checks on any of this!
    if (fill)
    {
        while ((term - q) < l)
        {
            *(--q) = fill;
        }
    }

    return q;
}

static char *to_string_dec_uint_internal(uint64_t n, StringFormatBuffer &buffer, size_t &length)
{
    auto end = &buffer.back();
    auto start = to_string_dec_uint_internal(end, n);
    length = end - start;
    return start;
}

char *to_string_dec_uint(uint64_t n, StringFormatBuffer &buffer, size_t &length)
{
    return to_string_dec_uint_internal(n, buffer, length);
}

char *to_string_dec_int(int64_t n, StringFormatBuffer &buffer, size_t &length)
{
    bool negative = n < 0;
    auto start = to_string_dec_uint(negative ? -n : n, buffer, length);

    if (negative)
    {
        *(--start) = '-';
        ++length;
    }

    return start;
}

std::string to_string_dec_int(int64_t n)
{
    StringFormatBuffer b{};
    size_t len{};
    char *str = to_string_dec_int(n, b, len);
    return std::string(str, len);
}

std::string to_string_dec_uint(uint64_t n)
{
    StringFormatBuffer b{};
    size_t len{};
    char *str = to_string_dec_uint(n, b, len);
    return std::string(str, len);
}

std::string to_string_bin(
    const uint32_t n,
    const uint8_t l)
{
    char p[33];
    for (uint8_t c = 0; c < l; c++)
    {
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
    const char fill)
{
    char p[16];
    auto term = p + sizeof(p) - 1;
    auto q = to_string_dec_uint_pad_internal(term, n, l, fill);

    // Right justify.
    // (This code is redundant and won't do anything if a fill character was specified)
    while ((term - q) < l)
    {
        *(--q) = ' ';
    }

    return q;
}

std::string to_string_dec_int(
    const int32_t n,
    const int32_t l,
    const char fill)
{
    const size_t negative = (n < 0) ? 1 : 0;
    uint32_t n_abs = negative ? -n : n;

    char p[16];
    auto term = p + sizeof(p) - 1;
    auto q = to_string_dec_uint_pad_internal(term, n_abs, l - negative, fill);

    // Add sign.
    if (negative)
    {
        *(--q) = '-';
    }

    // Right justify.
    // (This code is redundant and won't do anything if a fill character was specified)
    while ((term - q) < l)
    {
        *(--q) = ' ';
    }

    return q;
}

std::string to_string_decimal(float decimal, int8_t precision)
{
    double integer_part;
    double fractional_part;

    std::string result;
    if (precision > 9)
        precision = 9; // we will convert to uin32_t, and that is the max it can hold.

    fractional_part = modf(decimal, &integer_part) * pow(10, precision);

    if (fractional_part < 0)
    {
        fractional_part = -fractional_part;
    }

    result = to_string_dec_int(integer_part) + "." + to_string_dec_uint(fractional_part, precision, '0');

    return result;
}

std::string to_string_decimal_padding(float decimal, int8_t precision, const int32_t l)
{
    double integer_part;
    double fractional_part;

    std::string result;
    if (precision > 9)
        precision = 9; // we will convert to uin32_t, and that is the max it can hold.

    fractional_part = modf(decimal, &integer_part) * pow(10, precision);

    if (fractional_part < 0)
    {
        fractional_part = -fractional_part;
    }

    result = to_string_dec_int(integer_part) + "." + to_string_dec_uint(fractional_part, precision, '0');

    // Add padding with spaces to meet the length requirement
    if (result.length() < (uint32_t)l)
    {
        int padding_length = l - result.length();
        std::string padding(padding_length, ' ');
        result = padding + result;
    }

    return result;
}

// right-justified frequency in Hz, always 10 characters
std::string to_string_freq(const uint64_t f)
{
    std::string final_str{""};

    if (f < 1000000)
        final_str = to_string_dec_int(f, 10, ' ');
    else
        final_str = to_string_dec_int(f / 1000000, 4) + to_string_dec_int(f % 1000000, 6, '0');

    return final_str;
}

// right-justified frequency in MHz, rounded to 4 decimal places, always 9 characters
std::string to_string_short_freq(const uint64_t f)
{
    auto final_str = to_string_dec_int((f + 50) / 1000000, 4) + "." + to_string_dec_int(((f + 50) / 100) % 10000, 4, '0');
    return final_str;
}

// non-justified non-padded frequency in MHz, rounded to specified number of decimal places
std::string to_string_rounded_freq(const uint64_t f, int8_t precision)
{
    std::string final_str{""};
    static constexpr uint32_t pow10[7] = {
        1,
        10,
        100,
        1000,
        10000,
        100000,
        1000000,
    };

    if (precision < 1)
    {
        final_str = to_string_dec_uint(f / 1000000);
    }
    else
    {
        if (precision > 6)
            precision = 6;

        uint32_t divisor = pow10[6 - precision];

        final_str = to_string_dec_uint((f + (divisor / 2)) / 1000000) + "." + to_string_dec_int(((f + (divisor / 2)) / divisor) % pow10[precision], precision, '0');
    }
    return final_str;
}

std::string to_string_time_ms(const uint32_t ms)
{
    std::string final_str{""};

    if (ms < 1000)
    {
        final_str = to_string_dec_uint(ms) + "ms";
    }
    else
    {
        auto seconds = ms / 1000;

        if (seconds >= 60)
            final_str = to_string_dec_uint(seconds / 60) + "m";

        return final_str + to_string_dec_uint(seconds % 60) + "s";
    }

    return final_str;
}

static char *to_string_hex_internal(char *ptr, uint64_t value, uint8_t length)
{
    if (length == 0)
        return ptr;

    *(--ptr) = uint_to_char(value & 0xF, 16);
    return to_string_hex_internal(ptr, value >> 4, length - 1);
}

std::string to_string_hex(uint64_t value, int32_t length)
{
    constexpr uint8_t buffer_length = 33;
    char buffer[buffer_length];

    char *ptr = &buffer[buffer_length - 1];
    *ptr = '\0';

    length = std::min<uint8_t>(buffer_length - 1, length);
    return to_string_hex_internal(ptr, value, length);
}

std::string to_string_hex_array(uint8_t *array, int32_t length)
{
    std::string str_return;
    str_return.reserve(length * 2);

    for (uint8_t i = 0; i < length; i++)
        str_return += to_string_hex(array[i], 2);

    return str_return;
}
// TODO: wire standalone api:
/*
std::string to_string_datetime(const rtc::RTC &value, const TimeFormat format)
{
    std::string string{""};

    if (format == YMDHMS)
    {
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

std::string to_string_timestamp(const rtc::RTC &value)
{
    return to_string_dec_uint(value.year(), 4, '0') +
           to_string_dec_uint(value.month(), 2, '0') +
           to_string_dec_uint(value.day(), 2, '0') +
           to_string_dec_uint(value.hour(), 2, '0') +
           to_string_dec_uint(value.minute(), 2, '0') +
           to_string_dec_uint(value.second(), 2, '0');
}

std::string to_string_FAT_timestamp(const FATTimestamp &timestamp)
{
    return to_string_dec_uint((timestamp.FAT_date >> 9) + 1980) + "-" +
           to_string_dec_uint((timestamp.FAT_date >> 5) & 0xF, 2, '0') + "-" +
           to_string_dec_uint((timestamp.FAT_date & 0x1F), 2, '0') + " " +
           to_string_dec_uint((timestamp.FAT_time >> 11), 2, '0') + ":" +
           to_string_dec_uint((timestamp.FAT_time >> 5) & 0x3F, 2, '0');
}
*/
std::string to_string_file_size(uint32_t file_size)
{
    static const std::string suffix[5] = {"B", "kB", "MB", "GB", "??"};
    size_t suffix_index = 0;

    while (file_size >= 1024)
    {
        file_size /= 1024;
        suffix_index++;
    }

    if (suffix_index > 4)
        suffix_index = 4;

    return to_string_dec_uint(file_size) + suffix[suffix_index];
}

std::string to_string_mac_address(const uint8_t *macAddress, uint8_t length, bool noColon)
{
    std::string string;

    string += to_string_hex(macAddress[0], 2);

    for (int i = 1; i < length; i++)
    {
        string += noColon ? to_string_hex(macAddress[i], 2) : ":" + to_string_hex(macAddress[i], 2);
    }

    return string;
}

std::string to_string_formatted_mac_address(const char *macAddress)
{
    std::string formattedAddress;

    for (int i = 0; i < 12; i += 2)
    {
        if (i > 0)
        {
            formattedAddress += ':';
        }
        formattedAddress += macAddress[i];
        formattedAddress += macAddress[i + 1];
    }

    return formattedAddress;
}

void generateRandomMacAddress(char *macAddress)
{
    const char hexDigits[] = "0123456789ABCDEF";

    // Generate 12 random hexadecimal characters
    for (int i = 0; i < 12; i++)
    {
        int randomIndex = rand() % 16;
        macAddress[i] = hexDigits[randomIndex];
    }
    macAddress[12] = '\0'; // Null-terminate the string
}

// TODO: wire standalone api:
/*
uint64_t readUntil(File &file, char *result, std::size_t maxBufferSize, char delimiter)
{
    std::size_t bytesRead = 0;

    while (true)
    {
        char ch;
        File::Result<File::Size> readResult = file.read(&ch, 1);

        if (readResult.is_ok() && readResult.value() > 0)
        {
            if (ch == delimiter)
            {
                // Found a space character, stop reading
                break;
            }
            else if (bytesRead < maxBufferSize)
            {
                // Append the character to the result if there's space
                result[bytesRead++] = ch;
            }
            else
            {
                // Buffer is full, break to prevent overflow
                break;
            }
        }
        else
        {
            break; // End of file or error
        }
    }

    // Null-terminate the result string
    result[bytesRead] = '\0';

    return bytesRead;
}
*/
std::string unit_auto_scale(double n, const uint32_t base_unit, uint32_t precision)
{
    const uint32_t powers_of_ten[5] = {1, 10, 100, 1000, 10000};
    std::string string{""};
    uint32_t prefix_index = base_unit;
    double integer_part;
    double fractional_part;

    precision = std::min((uint32_t)4, precision);

    while (n > 1000)
    {
        n /= 1000.0;
        prefix_index++;
    }

    fractional_part = modf(n, &integer_part) * powers_of_ten[precision];
    if (fractional_part < 0)
        fractional_part = -fractional_part;

    string = to_string_dec_int(integer_part);
    if (precision)
        string += '.' + to_string_dec_uint(fractional_part, precision, '0');

    if (prefix_index != 3)
        string += unit_prefix[prefix_index];

    return string;
}

double get_decimals(double num, int16_t mult, bool round)
{
    num -= int(num); // keep decimals only
    num *= mult;     // Shift decimals into integers
    if (!round)
        return num;
    int16_t intnum = int(num); // Round it up if necessary
    num -= intnum;             // Get decimal part
    if (num > .5)
        intnum++; // Round up
    return intnum;
}

static const char *whitespace_str = " \t\r\n";

std::string trim(std::string_view str)
{
    auto first = str.find_first_not_of(whitespace_str);
    if (first == std::string::npos)
        return {};

    auto last = str.find_last_not_of(whitespace_str);
    return std::string{str.substr(first, last - first + 1)};
}

std::string trimr(std::string_view str)
{
    size_t last = str.find_last_not_of(whitespace_str);
    return std::string{last != std::string::npos ? str.substr(0, last + 1) : ""};
}

std::string truncate(std::string_view str, size_t length)
{
    return std::string{str.length() <= length ? str : str.substr(0, length)};
}

uint8_t char_to_uint(char c, uint8_t radix)
{
    uint8_t v = 0;

    if (c >= '0' && c <= '9')
        v = c - '0';
    else if (c >= 'A' && c <= 'F')
        v = c - 'A' + 10; // A is dec: 10
    else if (c >= 'a' && c <= 'f')
        v = c - 'a' + 10; // A is dec: 10

    return v < radix ? v : 0;
}

char uint_to_char(uint8_t val, uint8_t radix)
{
    if (val >= radix)
        return 0;

    if (val < 10)
        return '0' + val;
    else
        return 'A' + val - 10; // A is dec: 10
}
