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

#include <dmr/voice.h>
#include <string.h>

static size_t payload_bit(size_t index) {
    return index < 108u ? index : index + 48u;
}

int dmr_voice_burst_extract(
    const dmr_burst_t* burst,
    uint8_t frames[DMR_VOICE_FRAME_COUNT * DMR_VOICE_FRAME_BYTES]) {
    if (!burst || !frames)
        return 0;

    memset(frames, 0, DMR_VOICE_FRAME_COUNT * DMR_VOICE_FRAME_BYTES);

    for (size_t i = 0; i < DMR_VOICE_FRAME_COUNT * DMR_VOICE_FRAME_BITS; ++i) {
        const uint8_t bit = dmr_burst_get_bit(burst->data, payload_bit(i));

        if (bit == UINT8_MAX)
            return 0;

        frames[(i / DMR_VOICE_FRAME_BITS) * DMR_VOICE_FRAME_BYTES +
               (i % DMR_VOICE_FRAME_BITS) / 8u] |=
            (uint8_t)(bit << (7u - (i % 8u)));
    }
    return 1;
}
