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

#ifndef __RSSI_DMA_H__
#define __RSSI_DMA_H__

#include <cstddef>

#include "rssi.hpp"

namespace rf {
namespace rssi {
namespace dma {

void init();

void allocate(size_t buffer_count, size_t items_per_buffer);
void free();

void enable();
bool is_enabled();

void disable();

rf::rssi::buffer_t wait_for_buffer();

} /* namespace dma */
} /* namespace rssi */
} /* namespace rf */

#endif/*__RSSI_DMA_H__*/
