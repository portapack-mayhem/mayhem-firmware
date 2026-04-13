/*
 * Copyright (C) 2024 Bernd Herzog
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

#ifndef __STANDALONE_HEAP_THREAD_HPP__
#define __STANDALONE_HEAP_THREAD_HPP__

#include <cstddef>
#include <cstdint>

/* Uses standalone_application_api_t::thread_create_heap and ::thread_wait.
 * Requires _api to be set (after initialize()). Queues each heap thread for a
 * static reaper that calls thread_wait so ChibiOS returns the working area. */
void standalone_create_heap_thread(int32_t (*fn)(void*), void* arg, size_t stack_size, int priority);

#endif /* __STANDALONE_HEAP_THREAD_HPP__ */
