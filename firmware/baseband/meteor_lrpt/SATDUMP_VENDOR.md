# SatDump / third-party vendoring — Meteor LRPT (Mayhem)

This directory contains **ported or adapted** code for parity with [SatDump](https://github.com/SatDump/SatDump) Meteor LRPT decoding, plus other third-party kernels. Mayhem ships as **GPL v2**; combined works must respect each upstream licence.

## Pinned upstream revision

| Field | Value |
|--------|--------|
| **Remote** | `https://github.com/SatDump/SatDump` |
| **Git commit** | `f7c2c04f8bca6fbba223c9337a4499f022d2412b` |
| **Pinned for** | Meteor LRPT parity work (update this file + `tools/meteor_lrpt/README.md` when rebasing). |

**Raw prefix:** `https://raw.githubusercontent.com/SatDump/SatDump/f7c2c04f8bca6fbba223c9337a4499f022d2412b/`

## Upstream licence (summary)

- **SatDump** `src-core` / `plugins`: predominantly **MIT** — see upstream `LICENCE` / `LICENSE` in the SatDump tree at the pinned commit.
- **GNU Radio `volk`** (`volk_k7_r2_generic_fixed.h`, Viterbi butterfly): **GPLv3+** (GNU Radio COPYING).
- **Phil Karn libfec** (RS byte engine patterns): **LGPL** (see `rs223_decode.cpp` header).

When adding new ports, extend the table below and run:

`python tools/meteor_lrpt/verify_satdump_vendor_urls.py`

## Mayhem file → upstream reference

| Mayhem path | Upstream source (relative to SatDump repo root) | Notes |
|-------------|--------------------------------------------------|--------|
| `meteor_soft_correlate.cpp` | `src-core/common/codings/correlator.cpp`, `rotation.cpp` | QPSK sync + `rotate_soft` semantics; MIT. |
| `meteor_soft_correlate.hpp` | `plugins/meteor_support/meteor/module_meteor_lrpt_decoder.cpp` | `0xfca2b63db00d9794` / `0xfc4ef4fd0cc2df89` sync constants. |
| `ccsds_derandomize.cpp`, `ccsds_derandomize.hpp` | `src-core/common/codings/randomization.cpp` | `ccsds_pn[]` table; MIT. |
| `meteor_nrzm.cpp`, `meteor_nrzm.hpp` | `src-core/common/codings/differential/nrzm.cpp` | `diff::NRZMDiff::decode` / `decode_bits`; MIT. |
| `meteor_bpsk_ccsds_deframer.cpp`, `meteor_bpsk_ccsds_deframer.hpp` | `src-core/common/codings/deframing/bpsk_ccsds_deframer.cpp`, `bpsk_ccsds_deframer.h` | CCSDS ASM bit deframer; MIT. |
| `meteor_cc_decoder.cpp`, `meteor_cc_decoder.hpp` | `src-core/common/codings/viterbi/cc_decoder.cpp` / SatDump `CCDecoder` usage; **GNU Radio** `gr-fec/lib/cc_decoder_impl.cc` patterns | Streaming K=7 R=1/2; combined GPLv3 (GR) + MIT (SatDump glue). |
| `volk_k7_r2_generic_fixed.h` | GNU Radio **volk** `volk_8u_x4_conv_k7_r2_8u` generic implementation | GPLv3+; see file header. |
| `rs223_decode.cpp`, `rs223_decode.hpp` | **Phil Karn libfec** `decode_rs`; SatDump `reedsolomon::ReedSolomon::decode_interlaved(..., false, 4, …)` in `src-core/common/codings/reedsolomon/reedsolomon.cpp` + `plugins/meteor_support/meteor/module_meteor_lrpt_decoder.cpp` | RS(255,223) + depth-4 interleave layout; LGPL + MIT API parity. |
| `proc_meteor_lrpt_rx.cpp` (FEC wiring) | `plugins/meteor_support/meteor/module_meteor_lrpt_decoder.cpp` | Pipeline order (legacy vs M2-x); Mayhem integration is original. |

## Mayhem-only (no SatDump verbatim port)

| Mayhem path | Notes |
|-------------|--------|
| `meteor_jpeg_scan.cpp`, `meteor_jpeg_scan.hpp` | Heuristic UI preview; not a SatDump file copy. |
| `../common/dsp_fir_taps_meteor_lrpt.hpp` | Mayhem FIR taps (stand-in shaping); not from SatDump. |

## Not ported (reference only for future work)

| SatDump upstream | Purpose |
|------------------|---------|
| `src-core/common/codings/viterbi/viterbi_1_2.cpp` / `.h` | BER-based lock, IQ swap, puncturing shifts (M2-x). |
| `plugins/meteor_support/meteor/deint.cpp`, `deint.h` | M2-x interleaved phased streams (~2.65 MiB ring). |
| `plugins/meteor_support/meteor/deframer.cpp` | Older/plugin deframer paths (M2-x wiring references `BPSK_CCSDS_Deframer` in `src-core`). |

## Raw URLs (pin `f7c2c04f8bca6fbba223c9337a4499f022d2412b`)

Used by `verify_satdump_vendor_urls.py`:

- https://raw.githubusercontent.com/SatDump/SatDump/f7c2c04f8bca6fbba223c9337a4499f022d2412b/src-core/common/codings/correlator.cpp
- https://raw.githubusercontent.com/SatDump/SatDump/f7c2c04f8bca6fbba223c9337a4499f022d2412b/src-core/common/codings/rotation.cpp
- https://raw.githubusercontent.com/SatDump/SatDump/f7c2c04f8bca6fbba223c9337a4499f022d2412b/src-core/common/codings/randomization.cpp
- https://raw.githubusercontent.com/SatDump/SatDump/f7c2c04f8bca6fbba223c9337a4499f022d2412b/src-core/common/codings/differential/nrzm.cpp
- https://raw.githubusercontent.com/SatDump/SatDump/f7c2c04f8bca6fbba223c9337a4499f022d2412b/src-core/common/codings/deframing/bpsk_ccsds_deframer.cpp
- https://raw.githubusercontent.com/SatDump/SatDump/f7c2c04f8bca6fbba223c9337a4499f022d2412b/src-core/common/codings/deframing/bpsk_ccsds_deframer.h
- https://raw.githubusercontent.com/SatDump/SatDump/f7c2c04f8bca6fbba223c9337a4499f022d2412b/src-core/common/codings/viterbi/cc_decoder.cpp
- https://raw.githubusercontent.com/SatDump/SatDump/f7c2c04f8bca6fbba223c9337a4499f022d2412b/src-core/common/codings/reedsolomon/reedsolomon.cpp
- https://raw.githubusercontent.com/SatDump/SatDump/f7c2c04f8bca6fbba223c9337a4499f022d2412b/plugins/meteor_support/meteor/module_meteor_lrpt_decoder.cpp
