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

#include "phy/discriminator.h"

#include <math.h>

#define DMR_SAMPLE_RATE_HZ 19200.0f
#define DMR_TWO_PI 6.28318530717958647692f

static bool sample_is_valid(float complex sample) {
    const float i = crealf(sample);
    const float q = cimagf(sample);

    return isfinite(i) && isfinite(q) && (i != 0.0f || q != 0.0f);
}

void dmr_discriminator_reset(dmr_discriminator_t* discriminator) {
    discriminator->previous = 0.0f;
    discriminator->has_previous = false;
}

size_t dmr_discriminator_process(dmr_discriminator_t* discriminator,
                                 const float complex* samples,
                                 size_t count,
                                 float* frequency_hz) {
    size_t produced = 0;

    for (size_t i = 0; i < count; ++i) {
        const float complex current = samples[i];

        if (!sample_is_valid(current)) {
            discriminator->has_previous = false;
            continue;
        }

        if (discriminator->has_previous) {
            const float phase = cargf(current * conjf(discriminator->previous));
            frequency_hz[produced++] = phase * DMR_SAMPLE_RATE_HZ / DMR_TWO_PI;
        }

        discriminator->previous = current;
        discriminator->has_previous = true;
    }

    return produced;
}
