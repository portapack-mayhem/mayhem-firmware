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

#ifndef DMR_VOICE_H
#define DMR_VOICE_H

#include <dmr/burst.h>

#include <stddef.h>
#include <stdint.h>

enum {
    DMR_VOICE_FRAME_COUNT = 3,
    DMR_VOICE_FRAME_BITS = 72,
    DMR_VOICE_FRAME_BYTES = 9
};

/* A normal voice burst carries three 72-bit AMBE+2 channel frames.  Their
 * 216 bits occupy the two 108-bit payload regions around the 48-bit centre
 * field.  Frames are packed MSB-first in on-air order. */
int dmr_voice_burst_extract(
    const dmr_burst_t* burst,
    uint8_t frames[DMR_VOICE_FRAME_COUNT * DMR_VOICE_FRAME_BYTES]);

#endif
