/*
 * Copyright (C) 2023 Bernd Herzog
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

#include "random.hpp"

static unsigned long state[N]; /* the array for the state vector  */
static int rnd_left = 1;
static int rnd_init = 0;
static unsigned long* rnd_next;

/* initializes state[N] with a seed */
void init_genrand(unsigned long s) {
    int j;
    state[0] = s & 0xffffffffUL;

    for (j = 1; j < N; j++) {
        state[j] = (1812433253UL * (state[j - 1] ^ (state[j - 1] >> 30)) + j);
        /* See Knuth TAOCP Vol2. 3rd Ed. P.106 for multiplier. */
        /* In the previous versions, MSBs of the seed affect   */
        /* only MSBs of the array state[].                     */
        /* 2002/01/09 modified by Makoto Matsumoto             */
        state[j] &= 0xffffffffUL; /* for >32 bit machines      */
    }

    rnd_left = 1;
    rnd_init = 1;
}

static void next_state(void) {
    unsigned long* p = state;
    int j;

    /* if init_genrand() has not been called, */
    /* a default initial seed is used         */
    if (rnd_init == 0)
        init_genrand(5489UL);

    rnd_left = N;
    rnd_next = state;

    for (j = N - M + 1; --j; p++)
        *p = p[M] ^ TWIST(p[0], p[1]);

    for (j = M; --j; p++)
        *p = p[M - N] ^ TWIST(p[0], p[1]);

    *p = p[M - N] ^ TWIST(p[0], state[0]);
}

long genrand_int31(void) {
    unsigned long y;

    if (--rnd_left == 0)
        next_state();
    y = *rnd_next++;

    /* Tempering */
    y ^= (y >> 11);
    y ^= (y << 7) & 0x9d2c5680UL;
    y ^= (y << 15) & 0xefc60000UL;
    y ^= (y >> 18);

    return (long)(y >> 1);
}
