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

#include "chibios_cpp.hpp"

#include <cstdint>

#include <ch.h>

// "Out of Memory" on its own says nothing about why, and the panic screen is all
// there is afterwards. The number that decides it is the size that could not be
// served: a four-figure request means the budget is too tight, a two-figure one
// means the heap is fragmented, and they call for opposite fixes.
//
// Only the store happens here. Formatting it cost one of the external applications
// 900 bytes and broke their 32 kB ceiling, so the panic screen prints it instead.
size_t g_oom_size = 0;

void* operator new(size_t size) {
    void* p = chHeapAlloc(0x0, size);
    if (p == nullptr) {
        g_oom_size = size;
        chDbgPanic("Out of Memory");
    }
    return p;
}

void* operator new[](size_t size) {
    void* p = chHeapAlloc(0x0, size);
    if (p == nullptr) {
        g_oom_size = size;
        chDbgPanic("Out of Memory");
    }
    return p;
}

void operator delete(void* p) noexcept {
    chHeapFree(p);
}

void operator delete[](void* p) noexcept {
    chHeapFree(p);
}

void operator delete(void* ptr, std::size_t) noexcept {
    ::operator delete(ptr);
}

void operator delete[](void* ptr, std::size_t) noexcept {
    ::operator delete(ptr);
}

extern uint8_t __heap_base__[];
extern uint8_t __heap_end__[];

namespace chibios {

size_t heap_size() {
    return __heap_end__ - __heap_base__;
}

size_t heap_used() {
    const auto core_free = chCoreStatus();
    size_t heap_free = 0;
    chHeapStatus(NULL, &heap_free);
    return heap_size() - (core_free + heap_free);
}

} /* namespace chibios */
