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

#ifndef __LFSR_RANDOM_HPP__
#define __LFSR_RANDOM_HPP__

#include <cstdint>
#include <cstddef>

using lfsr_word_t = uint32_t;

lfsr_word_t lfsr_iterate(lfsr_word_t v);
void lfsr_fill(lfsr_word_t& v, lfsr_word_t* buffer, size_t word_count);
bool lfsr_compare(lfsr_word_t& v, const lfsr_word_t* buffer, size_t word_count);

#endif/*__LFSR_RANDOM_HPP__*/
