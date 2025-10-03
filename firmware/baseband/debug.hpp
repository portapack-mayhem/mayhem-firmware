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

#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <ch.h>

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

/* Executes a breakpoint only when a debugger is attached. */
#define HALT_IF_DEBUGGING()                                 \
    do {                                                    \
        if ((*(volatile uint32_t*)0xE000EDF0) & (1 << 0)) { \
            __asm__ __volatile__("bkpt 1");                 \
        }                                                   \
    } while (0)

/* Stops execution until a debugger is attached. */
#define HALT_UNTIL_DEBUGGING()                                \
    while (!((*(volatile uint32_t*)0xE000EDF0) & (1 << 0))) { \
    }                                                         \
    __asm__ __volatile__("bkpt 1")

#endif /*__DEBUG_H__*/
