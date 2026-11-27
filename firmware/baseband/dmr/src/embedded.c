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

#include <dmr/embedded.h>
#include <string.h>

static unsigned int popcount16(uint16_t x) {
    unsigned int n = 0;

    while (x) {
        n += x & 1u;
        x >>= 1;
    }
    return n;
}

static uint16_t qr_codeword(unsigned int value) {
    unsigned int p = value << 8;
    unsigned int aux = 0x4000u;

    while (p & ~0xffu) {
        while (!(aux & p)) aux >>= 1;
        p ^= (aux >> 8) * 0x139u;
    }

    uint16_t word = (uint16_t)((value << 9) | (p << 1));
    word |= (uint16_t)(popcount16(word) & 1u);

    return word;
}

int dmr_emb_decode(const dmr_burst_t* burst, dmr_emb_t* emb) {
    if (!burst || !emb || burst->has_sync)
        return 0;

    uint16_t received = 0;

    for (size_t i = 0; i < 8u; ++i)
        received = (uint16_t)((received << 1) |
                              dmr_burst_get_bit(burst->data, 108u + i));

    for (size_t i = 0; i < 8u; ++i)
        received = (uint16_t)((received << 1) |
                              dmr_burst_get_bit(burst->data, 148u + i));

    unsigned int best = 17u, value = 0;

    for (unsigned int v = 0; v < 128u; ++v) {
        const unsigned int distance = popcount16(received ^ qr_codeword(v));

        if (distance < best) {
            best = distance;
            value = v;
        }
    }

    if (best > 2u)
        return 0;

    memset(emb, 0, sizeof(*emb));
    emb->color_code = value >> 3;
    emb->privacy = (value >> 2) & 1u;
    emb->lcss = value & 3u;
    emb->corrected_bits = best;

    for (size_t byte = 0; byte < 4u; ++byte)
        for (size_t bit = 0; bit < 8u; ++bit)
            emb->fragment[byte] |= dmr_burst_get_bit(
                                       burst->data, 116u + byte * 8u + bit)
                                   << (7u - bit);
    return 1;
}

static int hamming16114(uint8_t* d) {
    const unsigned int s =
        ((d[0] ^ d[1] ^ d[2] ^ d[3] ^ d[5] ^ d[7] ^ d[8] ^ d[11]) << 0) |
        ((d[1] ^ d[2] ^ d[3] ^ d[4] ^ d[6] ^ d[8] ^ d[9] ^ d[12]) << 1) |
        ((d[2] ^ d[3] ^ d[4] ^ d[5] ^ d[7] ^ d[9] ^ d[10] ^ d[13]) << 2) |
        ((d[0] ^ d[1] ^ d[2] ^ d[4] ^ d[6] ^ d[7] ^ d[10] ^ d[14]) << 3) |
        ((d[0] ^ d[2] ^ d[5] ^ d[6] ^ d[8] ^ d[9] ^ d[10] ^ d[15]) << 4);

    static const uint8_t syndrome[16] =
        {0x19, 0x0b, 0x1f, 0x07, 0x0e, 0x15, 0x1a, 0x0d,
         0x13, 0x16, 0x1c, 0x01, 0x02, 0x04, 0x08, 0x10};

    if (!s) return 0;

    for (unsigned int i = 0; i < 16u; ++i)
        if (s == syndrome[i]) {
            d[i] ^= 1u;
            return 1;
        }

    return -1;
}

void dmr_embedded_lc_decoder_reset(dmr_embedded_lc_decoder_t* d) {
    if (d) memset(d, 0, sizeof(*d));
}

int dmr_embedded_lc_decoder_process(dmr_embedded_lc_decoder_t* d,
                                    const dmr_emb_t* emb,
                                    dmr_embedded_lc_t* lc) {
    if (!d || !emb || !lc) return 0;

    unsigned int part;

    if (emb->lcss == 1u) {
        d->state = 1u;
        d->corrected_bits = emb->corrected_bits;
        part = 0u;
    } else if (emb->lcss == 3u && (d->state == 1u || d->state == 2u)) {
        part = d->state++;
        d->corrected_bits += emb->corrected_bits;
    } else if (emb->lcss == 2u && d->state == 3u) {
        part = 3u;
        d->state = 0u;
        d->corrected_bits += emb->corrected_bits;
    } else {
        d->state = 0u;
        return 0;
    }

    for (size_t i = 0; i < 32u; ++i)
        d->raw[part * 32u + i] =
            (emb->fragment[i / 8u] >> (7u - i % 8u)) & 1u;

    if (part != 3u) return 0;

    uint8_t matrix[128] = {0};
    unsigned int p = 0;

    for (unsigned int i = 0; i < 128u; ++i) {
        matrix[p] = d->raw[i];
        p += 16u;
        if (p > 127u) p -= 127u;
    }

    unsigned int corrected = d->corrected_bits;

    for (unsigned int row = 0; row < 7u; ++row) {
        const int n = hamming16114(matrix + row * 16u);

        if (n < 0) return 0;

        corrected += (unsigned int)n;
    }

    for (unsigned int col = 0; col < 16u; ++col) {
        unsigned int parity = 0;

        for (unsigned int row = 0; row < 8u; ++row)
            parity ^= matrix[row * 16u + col];

        if (parity) return 0;
    }

    uint8_t payload[72];
    unsigned int out = 0;
    const unsigned int lengths[7] = {11, 11, 10, 10, 10, 10, 10};

    for (unsigned int row = 0; row < 7u; ++row)
        for (unsigned int i = 0; i < lengths[row]; ++i)
            payload[out++] = matrix[row * 16u + i];

    unsigned int crc = matrix[42] * 16u + matrix[58] * 8u + matrix[74] * 4u +
                       matrix[90] * 2u + matrix[106];
    unsigned int sum = 0;

    memset(lc, 0, sizeof(*lc));

    for (unsigned int byte = 0; byte < 9u; ++byte) {
        for (unsigned int bit = 0; bit < 8u; ++bit)
            lc->data[byte] |= payload[byte * 8u + bit] << (7u - bit);

        sum += lc->data[byte];
    }

    if (sum % 31u != crc) return 0;

    lc->flco = lc->data[0] & 0x3fu;
    lc->feature_set_id = lc->data[1];
    lc->service_options = lc->data[2];
    lc->target_id = ((uint32_t)lc->data[3] << 16) |
                    ((uint32_t)lc->data[4] << 8) | lc->data[5];
    lc->source_id = ((uint32_t)lc->data[6] << 16) |
                    ((uint32_t)lc->data[7] << 8) | lc->data[8];
    lc->corrected_bits = corrected;

    return 1;
}
