/*
 * Copyright (C) 2026 PortaPack Mayhem contributors
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

#include "dab_charset.hpp"

namespace dab {

namespace {

/* EBU Latin (TS 101 756 annex C) as Unicode. 0x20-0x7A are ASCII; the rest
 * differ. 0 marks an unassigned code. */
constexpr uint16_t kEbuLow[0x20] = {
    0x0000, 0x0118, 0x012E, 0x0172, 0x0102, 0x0116, 0x010E, 0x0218,
    0x021A, 0x010A, 0x0000, 0x0000, 0x0120, 0x0139, 0x017B, 0x0143,
    0x0105, 0x0119, 0x012F, 0x0173, 0x0103, 0x0117, 0x010F, 0x0219,
    0x021B, 0x010B, 0x0147, 0x011A, 0x0121, 0x013A, 0x017C, 0x0000};

constexpr uint8_t kEbuHighFirst = 0x7B;
constexpr uint16_t kEbuHigh[0x100 - kEbuHighFirst] = {
    0x00AB, 0x016F, 0x00BB, 0x013D, 0x0126,
    0x00E1, 0x00E0, 0x00E9, 0x00E8, 0x00ED, 0x00EC, 0x00F3, 0x00F2,
    0x00FA, 0x00F9, 0x00D1, 0x00C7, 0x015E, 0x00DF, 0x00A1, 0x0178,
    0x00E2, 0x00E4, 0x00EA, 0x00EB, 0x00EE, 0x00EF, 0x00F4, 0x00F6,
    0x00FB, 0x00FC, 0x00F1, 0x00E7, 0x015F, 0x011F, 0x0131, 0x00FF,
    0x0136, 0x0145, 0x00A9, 0x0122, 0x011E, 0x011B, 0x0148, 0x0151,
    0x0150, 0x20AC, 0x00A3, 0x0024, 0x0100, 0x0112, 0x012A, 0x016A,
    0x0137, 0x0146, 0x013B, 0x0123, 0x013C, 0x0130, 0x0144, 0x0171,
    0x0170, 0x00BF, 0x013E, 0x00B0, 0x0101, 0x0113, 0x012B, 0x016B,
    0x00C1, 0x00C0, 0x00C9, 0x00C8, 0x00CD, 0x00CC, 0x00D3, 0x00D2,
    0x00DA, 0x00D9, 0x0158, 0x010C, 0x0160, 0x017D, 0x00D0, 0x013F,
    0x00C2, 0x00C4, 0x00CA, 0x00CB, 0x00CE, 0x00CF, 0x00D4, 0x00D6,
    0x00DB, 0x00DC, 0x0159, 0x010D, 0x0161, 0x017E, 0x0111, 0x0140,
    0x00C3, 0x00C5, 0x00C6, 0x0152, 0x0177, 0x00DD, 0x00D5, 0x00D8,
    0x00DE, 0x014A, 0x0154, 0x0106, 0x015A, 0x0179, 0x0164, 0x00F0,
    0x00E3, 0x00E5, 0x00E6, 0x0153, 0x0175, 0x00FD, 0x00F5, 0x00F8,
    0x00FE, 0x014B, 0x0155, 0x0107, 0x015B, 0x017A, 0x0165, 0x0127};

/* Latin Extended-A, U+0100-U+017F, folded to the base letter. */
constexpr char kLatinExtA[] =
    "AaAaAaCcCcCcCcDd"
    "DdEeEeEeEeEeGgGg"
    "GgGgHhHhIiIiIiIi"
    "IiIiJjKkkLlLlLlL"
    "lLlNnNnNnnNnOoOo"
    "OoOoRrRrRrSsSsSs"
    "SsTtTtTtUuUuUuUu"
    "UuUuWwYyYZzZzZzs";
static_assert(sizeof(kLatinExtA) == 0x80 + 1, "one letter per code point");

/* 0 means "drop". */
char display_char(uint32_t cp) {
    if (cp < 0x20 || cp == 0x7F)
        return 0;
    if (cp < 0x7F || (cp >= 0xA0 && cp <= 0xFF))
        return static_cast<char>(cp);
    if (cp >= 0x100 && cp < 0x180)
        return kLatinExtA[cp - 0x100];
    switch (cp) {
        case 0x0218:
            return 'S';
        case 0x0219:
            return 's';
        case 0x021A:
            return 'T';
        case 0x021B:
            return 't';
        case 0x2018:
        case 0x2019:
            return '\'';
        case 0x201C:
        case 0x201D:
            return '"';
        case 0x2013:
        case 0x2014:
            return '-';
        case 0x20AC:
            return 'E';
        default:
            return '?';
    }
}

uint32_t ebu_latin(uint8_t b) {
    if (b < 0x20)
        return kEbuLow[b];
    if (b < kEbuHighFirst)
        return b;
    return kEbuHigh[b - kEbuHighFirst];
}

}  // namespace

size_t to_display(const uint8_t* in, size_t length, uint8_t charset, char* out, size_t capacity) {
    size_t n = 0;
    size_t i = 0;
    while (i < length && n < capacity) {
        uint32_t cp;
        if (charset == kCharsetUtf8) {
            const uint8_t b = in[i++];
            size_t extra = 0;
            if (b < 0x80) {
                cp = b;
            } else if ((b & 0xE0) == 0xC0) {
                cp = b & 0x1F;
                extra = 1;
            } else if ((b & 0xF0) == 0xE0) {
                cp = b & 0x0F;
                extra = 2;
            } else if ((b & 0xF8) == 0xF0) {
                cp = b & 0x07;
                extra = 3;
            } else {
                cp = '?';
            }
            for (; extra > 0; --extra) {
                if (i >= length || (in[i] & 0xC0) != 0x80) {
                    cp = '?';
                    break;
                }
                cp = (cp << 6) | (in[i++] & 0x3F);
            }
        } else if (charset == kCharsetUcs2) {
            if (i + 1 >= length)
                break;
            cp = (static_cast<uint32_t>(in[i]) << 8) | in[i + 1];
            i += 2;
        } else {
            cp = ebu_latin(in[i++]);
        }
        const char c = display_char(cp);
        if (c != 0)
            out[n++] = c;
    }
    return n;
}

} /* namespace dab */
