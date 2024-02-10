/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2023 Bernd Herzog
 * Copyright (C) 2023 Mark Thompson
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

#ifndef __DEBUG_H__
#define __DEBUG_H__

#include "hackrf_gpio.hpp"
#include <string>

void __debug_log(const std::string& msg);
#define __LOG2(l, msg) __debug_log(std::string{#l} + ":" + msg)
#define __LOG1(l, msg) __LOG2(l, msg)
#define DEBUG_LOG(msg) __LOG1(__LINE__, msg)

extern void draw_guru_meditation(uint8_t, const char*);
extern void draw_guru_meditation(uint8_t, const char*, struct extctx*, uint32_t);

extern uint32_t __process_stack_base__;
extern uint32_t __process_stack_end__;
#define CRT0_STACKS_FILL_PATTERN 0x55555555

inline uint32_t get_free_stack_space() {
    uint32_t* p;
    for (p = &__process_stack_base__; *p == CRT0_STACKS_FILL_PATTERN && p < &__process_stack_end__; p++)
        ;
    auto stack_space_left = p - &__process_stack_base__;

    return stack_space_left;
}

bool stack_dump();
bool memory_dump(uint32_t* addr_start, uint32_t num_words, bool stack_flag);

#endif /*__DEBUG_H__*/
