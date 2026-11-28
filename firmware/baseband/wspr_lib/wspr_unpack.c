/*
 * Copyright (C) 2026 Ivan
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

#include "wspr_unpack.h"

#include <string.h>

// Same character table as the packer: '0'-'9' = 0..9, 'A'-'Z' = 10..35,
// space = 36.
static char value_char(int v) {
    if (v <= 9) return (char)('0' + v);
    if (v <= 35) return (char)('A' + v - 10);
    return ' ';
}

static int bit_at(const uint8_t* bits, int index) {
    return (bits[index >> 3] >> (7 - (index & 7))) & 1;
}

bool wspr_unpack_message(const uint8_t* bits, wspr_message_t* message) {
    if (bits == NULL || message == NULL) return false;

    uint32_t n = 0;
    uint32_t m = 0;
    for (int i = 0; i < 28; i++) n = (n << 1) | (uint32_t)bit_at(bits, i);
    for (int i = 28; i < 50; i++) m = (m << 1) | (uint32_t)bit_at(bits, i);

    // The power field's top bit marks a standard message; types 2 and 3
    // (compound callsigns, 6-digit locators) are not handled.
    if ((m & 0x40u) == 0) return false;
    const int dbm = (int)(m & 0x3Fu);
    if (dbm > 60) return false;

    // Locator, inverting M1 = (179 - 10*L1 - D1)*180 + 10*L2 + D2.
    const uint32_t m1 = m >> 7;
    if (m1 >= 180u * 180u) return false;
    const int l2v = (int)((m1 % 180u) / 10u);
    const int d2 = (int)(m1 % 10u);
    const int q = 179 - (int)(m1 / 180u);
    const int l1v = q / 10;
    const int d1 = q % 10;
    if (l1v > 17 || l2v > 17 || d1 > 9 || d2 > 9) return false;

    // Callsign, inverting the documented chain. The last three characters are
    // base 27 (letters or space), the third is a digit, the first two come
    // from mixed bases.
    int c6 = (int)(n % 27u) + 10;
    n /= 27u;
    int c5 = (int)(n % 27u) + 10;
    n /= 27u;
    int c4 = (int)(n % 27u) + 10;
    n /= 27u;
    int c3 = (int)(n % 10u);
    n /= 10u;
    int c2 = (int)(n % 36u);
    n /= 36u;
    int c1 = (int)n;
    if (c1 > 36) return false;

    char call[7];
    call[0] = value_char(c1);
    call[1] = value_char(c2);
    call[2] = value_char(c3);
    call[3] = value_char(c4);
    call[4] = value_char(c5);
    call[5] = value_char(c6);
    call[6] = '\0';

    // Structural validation on the unpacked call: a digit in third position,
    // letters or spaces after, no space in the middle of the last three.
    if (call[2] < '0' || call[2] > '9') return false;
    for (int i = 3; i < 6; i++) {
        if (call[i] == ' ') {
            for (int j = i + 1; j < 6; j++) {
                if (call[j] != ' ') return false;
            }
            break;
        }
        if (call[i] < 'A' || call[i] > 'Z') return false;
    }
    // Strip the padding: one leading space and trailing spaces.
    const char* start = call;
    if (*start == ' ') start++;
    int end = 5;
    while (end >= 0 && call[end] == ' ') end--;
    const int length = end - (int)(start - call) + 1;
    if (length < 3) return false;

    memcpy(message->callsign, start, (size_t)length);
    message->callsign[length] = '\0';
    message->grid[0] = (char)('A' + l1v);
    message->grid[1] = (char)('A' + l2v);
    message->grid[2] = (char)('0' + d1);
    message->grid[3] = (char)('0' + d2);
    message->grid[4] = '\0';
    message->dbm = (int8_t)dbm;
    return true;
}
