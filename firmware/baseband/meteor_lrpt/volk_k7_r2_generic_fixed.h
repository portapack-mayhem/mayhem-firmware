/* -*- c++ -*- */
/*
 * Copyright 2014 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * GNU Radio is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * GNU Radio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Radio; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

/*!
 * \page volk_8u_x4_conv_k7_r2_8u
 *
 * \b Overview
 *
 * Performs convolutional decoding for a K=7, rate 1/2 convolutional
 * code. The polynomials user defined.
 *
 * <b>Dispatcher Prototype</b>
 * \code
 * void volk_8u_x4_conv_k7_r2_8u(unsigned char* Y, unsigned char* X, unsigned char* syms,
 * unsigned char* dec, unsigned int framebits, unsigned int excess, unsigned char*
 * Branchtab) \endcode
 *
 * \b Inputs
 * \li X: <FIXME>
 * \li syms: <FIXME>
 * \li dec: <FIXME>
 * \li framebits: size of the frame to decode in bits.
 * \li excess: <FIXME>
 * \li Branchtab: <FIXME>
 *
 * \b Outputs
 * \li Y: The decoded output bits.
 *
 * \b Example
 * \code
 * int N = 10000;
 *
 * volk_8u_x4_conv_k7_r2_8u();
 *
 * volk_free(x);
 * \endcode
 */

// The default version in Volk is broken right now, so I am including this fix
// SatDump will default to unless volk_config returned a working implemenation
//
// Mayhem: bundled for `MeteorCcDecoder`; see `SATDUMP_VENDOR.md` (GNU Radio VOLK, GPLv3+).

#ifndef INCLUDED_volk_8u_x4_conv_k7_r2_8u_H
#define INCLUDED_volk_8u_x4_conv_k7_r2_8u_H

#include <cstring>

namespace volk_fixed
{
    typedef union
    {
        unsigned char /*DECISIONTYPE*/ t[64 /*NUMSTATES*/ / 8 /*DECISIONTYPE_BITSIZE*/];
        unsigned int w[64 /*NUMSTATES*/ / 32];
        unsigned short s[64 /*NUMSTATES*/ / 16];
        unsigned char c[64 /*NUMSTATES*/ / 8];
#ifdef _MSC_VER
    } decision_t;
#else
    } decision_t __attribute__((aligned(16)));
#endif

    static inline void renormalize(unsigned char *X)
    {
        int NUMSTATES = 64;
        int i;

        unsigned char min = X[0];

        for (i = 0; i < NUMSTATES; i++)
            if (min > X[i])
                min = X[i];
        for (i = 0; i < NUMSTATES; i++)
            X[i] -= min;
    }

    // helper BFLY for GENERIC version
    static inline void BFLY(int i,
                            int s,
                            unsigned char *syms,
                            unsigned char *Y,
                            unsigned char *X,
                            decision_t *d,
                            unsigned char *Branchtab)
    {
        int j;
        unsigned int decision0, decision1;
        unsigned char metric, m0, m1, m2, m3;
        unsigned short metricsum;

        int NUMSTATES = 64;
        int RATE = 2;
        int METRICSHIFT = 1;
        int PRECISIONSHIFT = 2;

        metricsum = 1;
        for (j = 0; j < RATE; j++)
            metricsum += (Branchtab[i + j * NUMSTATES / 2] ^ syms[s * RATE + j]);
        metric = (metricsum >> METRICSHIFT) >> PRECISIONSHIFT;

        unsigned char max = ((RATE * ((256 - 1) >> METRICSHIFT)) >> PRECISIONSHIFT);

        m0 = X[i] + metric;
        m1 = X[i + NUMSTATES / 2] + (max - metric);
        m2 = X[i] + (max - metric);
        m3 = X[i + NUMSTATES / 2] + metric;

        decision0 = (signed int)(m0 - m1) >= 0;
        decision1 = (signed int)(m2 - m3) >= 0;

        Y[2 * i] = decision0 ? m1 : m0;
        Y[2 * i + 1] = decision1 ? m3 : m2;

        d->w[i / (sizeof(unsigned int) * 8 / 2) +
             s * (sizeof(decision_t) / sizeof(unsigned int))] |=
            (decision0 | decision1 << 1) << ((2 * i) & (sizeof(unsigned int) * 8 - 1));
    }

    static inline void volk_8u_x4_conv_k7_r2_8u_generic(unsigned char *Y,
                                                        unsigned char *X,
                                                        unsigned char *syms,
                                                        unsigned char *dec,
                                                        unsigned int framebits,
                                                        unsigned int excess,
                                                        unsigned char *Branchtab)
    {
        int nbits = framebits + excess;
        int NUMSTATES = 64;

        int s, i;
        for (s = 0; s < nbits; s++)
        {
            void *tmp;
            for (i = 0; i < NUMSTATES / 2; i++)
            {
                BFLY(i, s, syms, Y, X, (decision_t *)dec, Branchtab);
            }

            renormalize(Y);

            ///     Swap pointers to old and new metrics
            tmp = (void *)X;
            X = Y;
            Y = (unsigned char *)tmp;
        }
    }

}  // namespace volk_fixed

#endif /*INCLUDED_volk_8u_x4_conv_k7_r2_8u_H*/