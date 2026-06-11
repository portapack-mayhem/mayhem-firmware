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

## Mode parity matrix (Mayhem vs SatDump `module_meteor_lrpt_decoder.cpp`)

| Mode | Soft bytes / block to Viterbi tap | Deinterleave | Viterbi / bits | NRZ-M | Deframer `work` bits | RS |
|------|-------------------------------------|----------------|-----------------|--------|------------------------|-----|
| Legacy !M2-x | 16384 int8 I/Q | N/A (correlator + byte rotate) | `MeteorCcDecoder` 8192 **frame_bits** → 1024 B | **bytes** optional | N/A (ASM on bytes) | depth-4 |
| M2-x non-interleaved | 16384 | N/A | Same `MeteorCcDecoder` → unpack **8192** bits | **bits** optional on 8192 | `BPSK_CCSDS` **8192** bits | depth-4 |
| M2-x interleaved (IPC) | 8192 × 2 streams (A + phase90 B) after M0 | Dual `DeinterleaverReader` + rings | Dual `MeteorViterbi12` → 512 B each → winner | **bits** on **4096** | `BPSK_CCSDS` **4096** bits | depth-4 |

`MeteorBpskCcsdsDeframer::reset()` is only invoked from RX **configure** (mode change); the deframer **persists** across soft blocks within a session.

## Mayhem-only (no SatDump verbatim port)

| Mayhem path | Notes |
|-------------|--------|
| `meteor_jpeg_scan.cpp`, `meteor_jpeg_scan.hpp` | Heuristic UI preview; not a SatDump file copy. |
| `../common/dsp_fir_taps_meteor_lrpt.hpp` | Mayhem FIR taps (stand-in shaping); not from SatDump. |
| `external_ring.hpp`, `external_ring_ram.cpp`, `external_ring_sector_batch.hpp` | Pluggable ~2.65 MiB ring + 512 B sector batch helpers (FatFs path on M0: `firmware/application/meteor_lrpt_sector_file_ring.*`). |
| `meteor_deinterleaver.hpp`, `meteor_deinterleaver.cpp` | `DeinterleaverReader::deinterleave` semantics on `IMeteorDeintRing` (SatDump `deint.cpp` write loop). |
| `meteor_deinterleaver_reader.hpp`, `meteor_deinterleaver_reader.cpp` | SatDump `DeinterleaverReader::read_samples` flow (autocorr, `from_prev`, `rotate_soft` / `soft_to_hard`, deinterleave dispatch). |
| `meteor_lrpt_deinterleave_dispatch.hpp`, `meteor_lrpt_deinterleave_dispatch.cpp` | M4: RAM `meteor_deinterleave`; M0: strong override in `firmware/application/meteor_lrpt_deinterleave_dispatch_m0.cpp` → `meteor_deinterleave_file_ring` (512 B RMW). |
| `meteor_m2x_interleaved.hpp` | Dual-stream `rotate_soft` PHASE_90 copy helper (SatDump `DintSampleReader`). |
| `meteor_viterbi12.hpp`, `meteor_viterbi12.cpp` | SatDump `viterbi::Viterbi1_2` (`viterbi_1_2.{h,cpp}`): BER probe, IQ swap, phase, puncturing shift; `ber_soft_` sized `kTestBitsLength+1` for `+shift` indexing parity with upstream. |
| `meteor_m2x_interleaved_pipeline.hpp`, `meteor_m2x_interleaved_pipeline.cpp` | Post-deint dual Viterbi + winner + NRZ-M bits + `MeteorBpskCcsdsDeframer` (4096-bit `work` per block vs SatDump module). |
| `meteor_symbol_timing.hpp` | Gardner loop default gains for `proc_meteor_lrpt_rx.cpp`. |
| `../../firmware/common/meteor_lrpt_ring_iface.hpp` | Shared `IMeteorDeintRing` + `kDeinterleaverRingBytes` / sector size for M4 baseband and M0 file ring. |
| `firmware/application/meteor_lrpt_sector_file_ring.hpp` | One-sector write-back cache over `File` for ring storage (M0). |
| `../../tools/meteor_lrpt/deint_sector_histogram.py` | Host sector-touch stats vs `deinterleave` batch size. |
| `../../tools/meteor_lrpt/ring_mmap_parity.py` | mmap vs RAM byte parity smoke for full ring size (host). |
| `../../tools/meteor_lrpt/run_reader_long_golden_docker.sh`, `run_reader_long_golden_docker.ps1` | Dockerized compile+run for host golden tools (`test_reader_long_golden_host.cpp`, `test_m2x_pipeline_host.cpp`, `test_m2x_interleaved_ram_pipeline_host.cpp`, `compare_cadu.py` soft alignment smoke). |
| `../../tools/meteor_lrpt/m2x_interleaved_decode.py` | Host SatDump workflow helper for interleaved CADU. |
| `../../tools/meteor_lrpt/test_external_ring_host.cpp` | Optional host compile of RAM ring + `meteor_deinterleave`. |
| `../../tools/meteor_lrpt/test_reader_long_golden_host.cpp` | Host FNV digest after one long-path `read_samples(8192)` (regression guard). |
| `../../tools/meteor_lrpt/test_m2x_pipeline_host.cpp` | Host smoke: link `M2xInterleavedPostDeintPipeline` on zeroed soft (expect no CADU). |
| `../../tools/meteor_lrpt/test_m2x_interleaved_ram_pipeline_host.cpp` | Host: dual RAM-ring `read_samples` + post-deint pipeline + RS (Docker CI). |
| `../../tools/meteor_lrpt/compare_cadu.py` | CADU compare; optional `--validate-soft-file` + `--soft-block-bytes` for `.C8` alignment. |
| `firmware/application/meteor_lrpt_g4_service.cpp` | M0 G4 worker: `/LRPT/g4_cadu.bin` tail, demux + JPEG + BMP (reference: `lrpt_msumr_reader.cpp`); Mayhem integration is original. |
| `firmware/application/meteor_lrpt_g4/msumr_demux.{hpp,cpp}` | CCSDS packet scan, JPEG span extract, 64 KiB reassembly (reference: `lrpt_msumr_reader.cpp`, `module_meteor_msumr_lrpt.cpp`); not a verbatim port. |
| `firmware/application/meteor_lrpt_g4/jpeg_decode.{hpp,cpp}` | Baseline JPEG → `BMPFile` via ChaN **TJpgDec** (RGB888). |
| `firmware/application/meteor_lrpt_g4/third_party/tjpgd/*` | Vendored **TJpgDec** R0.03 (Bodmer mirror); see `LICENSE.txt` in that folder. |
| `firmware/application/meteor_lrpt_msumr.{hpp,cpp}` | CCSDS Space Packet primary header helpers + optional **PEC** strip (CRC-16-CCITT vs SatDump `crcCheckCCITT`); wire big-endian. |

## G4 / MSU-MR parity matrix (Mayhem vs SatDump instruments)

| Stage | SatDump (pinned tree) | Mayhem |
|-------|------------------------|--------|
| CADU → space data units | `module_meteor_instruments.cpp` + MSU-MR modules | **Mayhem:** `msumr_lrpt_mpdu_field` matches SatDump `parseMPDU(..., insert=2)` (FHP @ [12:13], MPDU data @ byte **14**, 882 B cap); VCID filter **not** in G4 yet (`module_meteor_msumr_lrpt.cpp` uses VCID 5 upstream). Legacy ASM+4 scan kept for fixtures. |
| MSU-MR APID routing / JPEG assembly | `lrpt_msumr_reader.cpp`, `module_meteor_msumr_lrpt.cpp` | `msumr_demux`: `ccsds_try_parse_space_packet` (optional **PEC** strip when CRC matches trailer), **10 B** secondary when flagged, bounded reassembly (256 B per APID bucket), JPEG `SOI`…`EOI` extract; raw scan fallback. |
| CCSDS PEC (optional) | `src-core/common/ccsds/ccsds.cpp` `crcCheckCCITT` | `ccsds_space_packet_pec_crc16_ccitt`; if CRC equals last two octets of the space packet, user-field length is reduced by 2 (PEC not passed to JPEG assembly). |
| JPEG decode | External / library in SatDump stack | **tjpgd** (`JD_FORMAT=0` RGB888); progressive rejected. |
| Live CADU → M0 (optional) | N/A (host pipeline buffers) | M4 `meteor_lrpt_g4_ring_push_post_rs`: SPSC ring `SharedMemory::MeteorLrptG4Ipc` — M4 updates `ring_push` only; M0 `ring_pop` only; on full ring **incoming** CADU dropped (`live_ring_overflows`, `G4_DROP_RING_OVERFLOW`). |
| Raster product | PNG/BMP pipeline in SatDump UI | `BMPFile` 24 bpp: BMP height grows via `expand_y` as TJpgDec emits MCUs; P6 sidecar preallocates full raster. |

**MCU / quantization:** parity on imagery assumes the same **baseline** JPEG bytes; TJpgDec uses standard IDCT + quant tables from the stream (same as other baseline decoders modulo rounding).

## Not ported (reference only for future work)

| SatDump upstream | Purpose |
|------------------|---------|
| `src-core/common/codings/viterbi/viterbi_1_2.cpp` / `.h` | BER-based lock, IQ swap, puncturing shifts (M2-x); Mayhem `MeteorViterbi12` port. |
| `plugins/meteor_support/meteor/deint.cpp`, `deint.h` | SatDump reference for `DeinterleaverReader` (Mayhem: `meteor_deinterleave` + `MeteorDeinterleaverReader::read_samples`; remaining glue vs SatDump module wiring may differ). |
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
- https://raw.githubusercontent.com/SatDump/SatDump/f7c2c04f8bca6fbba223c9337a4499f022d2412b/plugins/meteor_support/meteor/deint.cpp
- https://raw.githubusercontent.com/SatDump/SatDump/f7c2c04f8bca6fbba223c9337a4499f022d2412b/plugins/meteor_support/meteor/deint.h
- https://raw.githubusercontent.com/SatDump/SatDump/f7c2c04f8bca6fbba223c9337a4499f022d2412b/src-core/common/codings/viterbi/viterbi_1_2.cpp
- https://raw.githubusercontent.com/SatDump/SatDump/f7c2c04f8bca6fbba223c9337a4499f022d2412b/src-core/common/codings/viterbi/viterbi_1_2.h
- https://raw.githubusercontent.com/SatDump/SatDump/f7c2c04f8bca6fbba223c9337a4499f022d2412b/plugins/meteor_support/meteor/module_meteor_instruments.cpp
- https://raw.githubusercontent.com/SatDump/SatDump/f7c2c04f8bca6fbba223c9337a4499f022d2412b/plugins/meteor_support/meteor/instruments/msumr/lrpt_msumr_reader.cpp
- https://raw.githubusercontent.com/SatDump/SatDump/f7c2c04f8bca6fbba223c9337a4499f022d2412b/plugins/meteor_support/meteor/instruments/msumr/module_meteor_msumr_lrpt.cpp
