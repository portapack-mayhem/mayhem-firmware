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

#ifndef __CHIBIOS_CPP_H__
#define __CHIBIOS_CPP_H__

#include <cstddef>

/* Override new/delete to use Chibi/OS heap functions */
/* NOTE: Do not inline these, it doesn't work. ;-) */
void* operator new(size_t size);
void* operator new[](size_t size);
void operator delete(void* p);
void operator delete[](void* p);
void operator delete(void* ptr, std::size_t);
void operator delete[](void* ptr, std::size_t);

namespace chibios {

size_t heap_size();
size_t heap_used();

} /* namespace chibios */

#endif/*__CHIBIOS_CPP_H__*/
