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

#include "phy/sync.h"

#include <math.h>
#include <stdint.h>
#include <string.h>

/* ETSI TS 102 361-1, table 9.2. Bits are transmitted most-significant first. */
#define DMR_BS_VOICE_SYNC UINT64_C(0x755FD7DF75F7)
#define DMR_MS_VOICE_SYNC UINT64_C(0x7F7D5DD57DFD)
#define DMR_BS_DATA_SYNC UINT64_C(0xDFF57D75DF5D)
#define DMR_MS_DATA_SYNC UINT64_C(0xD5D7F77FD757)

bool dmr_sync_word(dmr_sync_kind_t kind, uint64_t* word) {
    if (word == NULL)
        return false;
    if (kind == DMR_SYNC_MS_VOICE)
        *word = DMR_MS_VOICE_SYNC;
    else if (kind == DMR_SYNC_BS_VOICE)
        *word = DMR_BS_VOICE_SYNC;
    else if (kind == DMR_SYNC_MS_DATA)
        *word = DMR_MS_DATA_SYNC;
    else if (kind == DMR_SYNC_BS_DATA)
        *word = DMR_BS_DATA_SYNC;
    else
        return false;
    return true;
}

/* Constant on-air templates; avoid expanding 48-bit words per sample. */

static const float templates[DMR_SYNC_KIND_COUNT][DMR_SYNC_SYMBOL_COUNT] = {
    {3.0f, -3.0f, 3.0f, 3.0f, 3.0f, 3.0f, -3.0f, -3.0f, -3.0f, 3.0f, 3.0f, -3.0f, -3.0f, 3.0f, -3.0f, -3.0f, 3.0f, -3.0f, 3.0f, 3.0f, -3.0f, -3.0f, 3.0f, -3.0f},
    {3.0f, -3.0f, -3.0f, -3.0f, 3.0f, -3.0f, -3.0f, 3.0f, 3.0f, 3.0f, -3.0f, 3.0f, -3.0f, 3.0f, 3.0f, 3.0f, 3.0f, -3.0f, -3.0f, 3.0f, -3.0f, -3.0f, -3.0f, 3.0f},
    {-3.0f, 3.0f, -3.0f, -3.0f, -3.0f, -3.0f, 3.0f, 3.0f, 3.0f, -3.0f, -3.0f, 3.0f, 3.0f, -3.0f, 3.0f, 3.0f, -3.0f, 3.0f, -3.0f, -3.0f, 3.0f, 3.0f, -3.0f, 3.0f},
    {-3.0f, 3.0f, 3.0f, 3.0f, -3.0f, 3.0f, 3.0f, -3.0f, -3.0f, -3.0f, 3.0f, -3.0f, 3.0f, -3.0f, -3.0f, -3.0f, -3.0f, 3.0f, 3.0f, -3.0f, 3.0f, 3.0f, 3.0f, -3.0f},
};

void dmr_sync_expected_levels(dmr_sync_kind_t kind,
                              float levels[DMR_SYNC_SYMBOL_COUNT]) {
    if ((unsigned)kind < DMR_SYNC_KIND_COUNT)
        memcpy(levels, templates[kind], sizeof(templates[0]));
    else
        for (size_t i = 0; i < DMR_SYNC_SYMBOL_COUNT; ++i)
            levels[i] = 1.0f;
}

/* A conservative cheap gate. All templates contain only +/-3, so their
 * centered energy is 216 - 24*mean^2. Share the observation statistics
 * across the four hypotheses; calculate neither RMS nor square roots
 * until a candidate passes. Full correlation remains authoritative. */

uint32_t dmr_sync_candidates(const float observed[DMR_SYNC_SYMBOL_COUNT],
                             float threshold) {
    static const float means[DMR_SYNC_KIND_COUNT] = {0.0f, 0.0f, 0.0f, 0.0f};
    float mean = 0.0f;

    for (size_t i = 0; i < DMR_SYNC_SYMBOL_COUNT; ++i) {
        if (!isfinite(observed[i])) return 0;
        mean += observed[i];
    }

    mean /= DMR_SYNC_SYMBOL_COUNT;

    float centered[DMR_SYNC_SYMBOL_COUNT];
    float energy = 0.0f;

    for (size_t i = 0; i < DMR_SYNC_SYMBOL_COUNT; ++i) {
        centered[i] = observed[i] - mean;
        energy += centered[i] * centered[i];
    }

    if (!(energy > 0.0f)) return 0;

    // Admit a small rounding margin; exact threshold is checked by caller.

    const float gate = fmaxf(0.0f, threshold - 0.00001f);
    uint32_t mask = 0;

    for (unsigned kind = 0; kind < DMR_SYNC_KIND_COUNT; ++kind) {
        float covariance = 0.0f;

        for (size_t i = 0; i < DMR_SYNC_SYMBOL_COUNT; ++i)
            covariance += centered[i] * (templates[kind][i] - means[kind]);

        const float expected_energy = 216.0f - 24.0f * means[kind] * means[kind];

        if (covariance > 0.0f &&
            covariance * covariance >= gate * gate * expected_energy * energy)
            mask |= 1u << kind;
    }
    return mask;
}

bool dmr_sync_correlate(const float observed[DMR_SYNC_SYMBOL_COUNT],
                        dmr_sync_kind_t kind,
                        dmr_sync_fit_t* fit) {
    float expected[DMR_SYNC_SYMBOL_COUNT];
    float mean_expected = 0.0f;
    float mean_observed = 0.0f;

    dmr_sync_expected_levels(kind, expected);

    for (size_t i = 0; i < DMR_SYNC_SYMBOL_COUNT; ++i) {
        if (!isfinite(observed[i]))
            return false;

        mean_expected += expected[i];
        mean_observed += observed[i];
    }

    mean_expected /= DMR_SYNC_SYMBOL_COUNT;
    mean_observed /= DMR_SYNC_SYMBOL_COUNT;

    float covariance = 0.0f;
    float expected_energy = 0.0f;
    float observed_energy = 0.0f;

    for (size_t i = 0; i < DMR_SYNC_SYMBOL_COUNT; ++i) {
        const float x = expected[i] - mean_expected;
        const float y = observed[i] - mean_observed;

        covariance += x * y;
        expected_energy += x * x;
        observed_energy += y * y;
    }

    if (!(expected_energy > 0.0f) || !(observed_energy > 0.0f))
        return false;

    fit->gain_hz_per_level = covariance / expected_energy;
    fit->offset_hz = mean_observed - fit->gain_hz_per_level * mean_expected;
    fit->score = covariance / sqrtf(expected_energy * observed_energy);

    float squared_error = 0.0f;

    for (size_t i = 0; i < DMR_SYNC_SYMBOL_COUNT; ++i) {
        const float predicted = fit->offset_hz +
                                fit->gain_hz_per_level * expected[i];
        const float error = observed[i] - predicted;

        squared_error += error * error;
    }

    fit->residual_rms_hz = sqrtf(squared_error / DMR_SYNC_SYMBOL_COUNT);

    return fit->gain_hz_per_level > 0.0f;
}
