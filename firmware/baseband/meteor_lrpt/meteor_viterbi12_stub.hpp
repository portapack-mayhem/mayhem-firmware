/*
 * Copyright (C) 2026
 *
 * SatDump `viterbi::Viterbi1_2` (`viterbi_1_2.cpp` / `.h`, pinned raw URLs in SATDUMP_VENDOR.md) implements
 * BER-based re-lock, IQ swap, and puncturing shifts for **M2-x interleaved** dual soft streams. It sits
 * **after** phased soft alignment / `DeinterleaverReader::read_samples` in
 * `plugins/meteor_support/meteor/module_meteor_lrpt_decoder.cpp`.
 *
 * Mayhem already vendors `CCDecoder` as `MeteorCcDecoder` (`meteor_cc_decoder.*`). A full `Viterbi1_2`
 * port still needs upstream encoder/decoder glue, dual-stream scheduling, and (for on-device parity)
 * a ~2.65 MiB deinterleaver backing store reachable from M4 (today: `MeteorDeintFileRing` on M0 + IPC,
 * or host-only SatDump — see `m2x_interleaved_decode.py`).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef METEOR_LRPT_METEOR_VITERBI12_STUB_HPP
#define METEOR_LRPT_METEOR_VITERBI12_STUB_HPP

namespace meteor_lrpt {

/* Placeholder type so future wiring has a single include anchor. */
struct MeteorViterbi12Future;

}  // namespace meteor_lrpt

#endif
