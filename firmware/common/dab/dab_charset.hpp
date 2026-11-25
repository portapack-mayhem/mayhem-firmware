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

#ifndef __DAB_CHARSET_H__
#define __DAB_CHARSET_H__

#include <cstddef>
#include <cstdint>

namespace dab {

/* Character set field of FIG 1 labels and DLS (ETSI TS 101 756 table 1). */
constexpr uint8_t kCharsetEbuLatin = 0x0;
constexpr uint8_t kCharsetUcs2 = 0x6;
constexpr uint8_t kCharsetUtf8 = 0xF;

/* To bytes the UI font draws: ASCII and Latin-1 as is (the font has glyphs for
 * 0xA0-0xFF), other Latin letters folded to the base letter, the rest '?';
 * control codes dropped. Unknown charsets read as EBU Latin. Writes at most
 * `capacity` bytes, unterminated; returns the count. */
size_t to_display(const uint8_t* in, size_t length, uint8_t charset, char* out, size_t capacity);

} /* namespace dab */

#endif /*__DAB_CHARSET_H__*/
