/*
 * Copyright (C) 2026 Belousov Oleg aka R1CBU
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

/* Internal AAPCS entry points for the Cortex-M4 assembly core. */

#ifndef VOCODER_ASM_H
#define VOCODER_ASM_H

#include <stdint.h>

int32_t asm_decode_wav(
    int16_t* output,
    int32_t sample_count,
    int16_t* bit_input,
    int32_t configuration_index,
    uint32_t flags,
    int16_t second_half,
    void* state);

#endif
