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

#ifndef DMR_PHY_DISCRIMINATOR_H
#define DMR_PHY_DISCRIMINATOR_H

#include <complex.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct {
    float complex previous;
    bool has_previous;
} dmr_discriminator_t;

void dmr_discriminator_reset(dmr_discriminator_t* discriminator);

/*
 * Produces instantaneous frequency in hertz. The first valid IQ sample after
 * reset establishes phase history and produces no output. A non-finite or
 * zero sample breaks phase continuity and is otherwise ignored.
 */
size_t dmr_discriminator_process(dmr_discriminator_t* discriminator,
                                 const float complex* samples,
                                 size_t count,
                                 float* frequency_hz);

#endif
