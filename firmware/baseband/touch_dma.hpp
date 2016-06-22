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

#ifndef __TOUCH_DMA_H__
#define __TOUCH_DMA_H__

#include "buffer.hpp"

#include <cstddef>

namespace touch {
namespace dma {

using sample_t = uint32_t;
using buffer_t = buffer_t<sample_t>;

void init();

void allocate();
void free();

void enable();
bool is_enabled();

void disable();

buffer_t wait_for_buffer();

} /* namespace dma */
} /* namespace touch */

#endif/*__TOUCH_DMA_H__*/
