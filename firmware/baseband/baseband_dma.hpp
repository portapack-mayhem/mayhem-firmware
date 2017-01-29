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

#ifndef __BASEBAND_DMA_H__
#define __BASEBAND_DMA_H__

#include <cstddef>
#include <array>

#include "complex.hpp"
#include "baseband.hpp"

namespace baseband {
namespace dma {

void init();
void configure(
	baseband::sample_t* const buffer_base,
	const baseband::Direction direction
);

void enable(const baseband::Direction direction);
bool is_enabled();

void disable();

baseband::buffer_t wait_for_buffer();

} /* namespace dma */
} /* namespace baseband */

#endif/*__BASEBAND_DMA_H__*/
