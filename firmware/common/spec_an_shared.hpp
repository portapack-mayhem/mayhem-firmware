/*
 * Copyright (C) 2026 zxkmm
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

/* State shared between the Spectrum Analyzer app (M0) and its baseband
 * image (M4).
 *
 * The sweep is driven by the M0, which owns the tuner. Per slice:
 *
 *   M0: retune, then write req_slice/req_settle and bump req_seq.
 *   M4: execute() sees the new seq, throws away req_settle DMA buffers,
 *       copies the samples it needs and immediately posts SpecAnCaptured.
 *   M0: on SpecAnCaptured retunes for the next slice right away, while
 *   M4: FFTs the capture in the event thread and posts SpecAnSlice once the
 *       display points for that slice are in pix[].
 *
 * The request deliberately goes through shared memory rather than a
 * baseband message: baseband::send_message() spins until the M4 event loop
 * picks the message up, and that loop is the one running the FFT. */

#ifndef __SPEC_AN_SHARED_H__
#define __SPEC_AN_SHARED_H__

#include <cstdint>
#include <cstddef>

namespace spec_an {

/* Display points across the graticule. One per landscape pixel column. */
constexpr size_t kPoints = 320;

constexpr size_t kMinFftLog2 = 8;
constexpr size_t kMaxFftLog2 = 11;
constexpr size_t kMaxFft = 1 << kMaxFftLog2;

/* Every DMA transfer the baseband thread hands to execute(). */
constexpr size_t kDmaBufferSamples = 2048;

/* Waterfall history, hosted in M4 RAM (the M0 is short on heap). */
constexpr size_t kWaterfallRows = 80;

/* Which FFT bins each tune contributes, as a run of natural bin indices
 * starting at `first` (negative = below the LO, taken modulo N).
 *
 * Upper: [N/32, 3N/8). Upper sideband only, so the DC/LO-feedthrough spike
 *        and the quadrature image of in-band signals stay out of the result,
 *        and the top edge stays clear of the anti-alias roll-off. For a
 *        plain zero-IF front end (HackRF One).
 * Both:  [-3N/8, 3N/8). For front ends that already keep DC and the image
 *        out of band (PRALINE offsets the analogue LO by a quarter of the
 *        ADC rate and rotates it back in the FPGA). 2.2x the span per tune. */
enum class BinLayout : uint8_t {
    Upper = 0,
    Both,
    Count
};

constexpr int32_t layout_first(BinLayout layout, size_t n) {
    return (layout == BinLayout::Both) ? -static_cast<int32_t>((3 * n) / 8) : static_cast<int32_t>(n / 32);
}

constexpr size_t layout_count(BinLayout layout, size_t n) {
    return (layout == BinLayout::Both) ? (3 * n) / 4 : (3 * n) / 8 - n / 32;
}

constexpr size_t kMaxSliceBins = (3 * kMaxFft) / 4;

enum class Window : uint8_t {
    Rect = 0,
    Hann,
    BlackmanHarris,
    FlatTop,
    Count
};

/* Equivalent noise bandwidth in hundredths of a bin, for RBW readout. */
constexpr uint16_t window_enbw_centibins(Window w) {
    switch (w) {
        case Window::Rect:
            return 100;
        case Window::Hann:
            return 150;
        case Window::BlackmanHarris:
            return 200;
        case Window::FlatTop:
            return 377;
        default:
            return 100;
    }
}

enum class Detector : uint8_t {
    Peak = 0,
    Average,
    Sample,
    NegPeak,
    Count
};

/* Level that marks a display point as "no data yet". */
constexpr int16_t kNoData = INT16_MIN;

struct Shared {
    /* M0 -> M4. req_seq is written last. */
    volatile uint32_t req_seq;
    volatile uint16_t req_slice;
    volatile uint8_t req_sweep;  /* sweep counter; parity picks pix[] buffer */
    volatile uint8_t req_settle; /* whole DMA buffers to discard first */

    /* M4 -> M0, filled in while handling SpecAnConfigMessage. */
    uint8_t* volatile waterfall; /* kWaterfallRows * kPoints palette indices */

    /* Display points in centi-dBFS, double buffered by sweep parity so the
     * M4 can start the next sweep while the M0 still reads this one. */
    int16_t pix[2][kPoints];
};

} /* namespace spec_an */

#endif /*__SPEC_AN_SHARED_H__*/
