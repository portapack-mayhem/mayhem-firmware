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

#pragma once

/* Period parameters */
#define N 624
#define M 397
#define MATRIX_A 0x9908b0dfUL /* constant vector a */
#define UMASK 0x80000000UL    /* most significant w-r bits */
#define LMASK 0x7fffffffUL    /* least significant r bits */
#define MIXBITS(u, v) (((u)&UMASK) | ((v)&LMASK))
#define TWIST(u, v) ((MIXBITS(u, v) >> 1) ^ ((v)&1UL ? MATRIX_A : 0UL))

/* initializes state[N] with a seed */
extern void init_genrand(unsigned long s);
extern long genrand_int31(void);
