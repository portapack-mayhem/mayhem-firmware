# Mayhem Meteor LRPT SOFT recording format

This document specifies the **raw int8 soft symbol stream** written by the **Meteor LRPT** external app when **SOFT** record is enabled (second `RecordView` row). It is the on-disk format `tools/meteor_lrpt/deint_sector_histogram.py` and `m2x_interleaved_decode.py` assume unless noted otherwise.

## Transport

- **Path:** User-selected under `/LRPT` (same folder pattern as CADU REC).
- **Extension:** `.C8` (see `FileConvertWriter` / `RecordView::FileType::RawS8` in firmware).
- **Endianness:** Little-endian host order (native `int8_t` bytes, no multibyte fields in the payload).

## Block size and alignment (two profiles)

| Profile | `CaptureConfig` / `RecordView` `write_size` | When used |
|---------|-------------------------------------------|------------|
| **legacy_soft_16k** | **16384** bytes per `f_write` | Default: legacy QPSK path or M2-x non-interleaved (`soft_bytes_needed`). |
| **m2x_interleaved_soft_8k** | **8192** bytes per `f_write` | **M2-x + Interleaved** with M0 IPC + SD rings (`soft_bytes_target()` in [`proc_meteor_lrpt_rx.cpp`](../../firmware/baseband/proc_meteor_lrpt_rx.cpp)). |

- While recording is active, blocks are **contiguous** in file order: no padding between blocks.
- **Partial final block:** If the user stops recording mid-block, the last write may be **shorter** than the active profile’s block size. Host tools should handle a trailing short block or ignore it for FEC alignment tests.
- Validate alignment with: `python tools/meteor_lrpt/compare_cadu.py --validate-soft-file path.C8 --soft-block-bytes 8192` (or `16384`).

## Sample layout inside each block

- Data are **interleaved int8 I/Q soft symbols** in the same order produced at the M4 tap used for Viterbi input:
  - Byte `2*k`   = **I** (in-phase) soft for symbol `k` (signed, typical correlator / AGC range).
  - Byte `2*k+1` = **Q** (quadrature) soft for symbol `k`.
- **16384-byte profile:** **8192 symbol pairs** per block (legacy / M2-x non-interleaved accumulation).
- **8192-byte profile:** **4096 symbol pairs** per block (one interleaved `soft_in` IPC chunk; still I/Q interleaved int8).

## RF / DSP context (for SatDump alignment)

- **IQ baseband rate into M4:** `3072000` Hz (`baseband_fs` in `proc_meteor_lrpt_rx`).
- **Symbol path:** Decimator stack → **192000** Hz complex (`fs_sym_path`) → DC block → **RRC** (`sym_shaping_*`) → OQPSK-style half-symbol mapping into `int8_t` soft pairs accumulated in `soft_buf_` until `soft_bytes_target()` bytes (16384 or 8192), then one decode / record write cycle.
- **Symbol rate:** UI-selectable **72k** or **80k** symbols/s (`sym_rate_hz_`); default **72000**. Host pipelines must use the **same** assumption as the capture metadata (see companion `.TXT` metadata file next to the `.C8` from `write_metadata_file`).

## Single-stream rule (critical)

The M4 baseband holds **at most one** `StreamInput` for capture (`cadu_stream_` **or** `soft_stream_`, never both). See [`proc_meteor_lrpt_rx::capture_config`](../../firmware/baseband/proc_meteor_lrpt_rx.cpp): `write_size` **16384** or **8192** selects SOFT; otherwise CADU. **Do not** enable CADU REC and SOFT REC simultaneously.

## M2-x interleaved parity note

SatDump **interleaved** M2-x uses **two** phased soft streams (`DintSampleReader` + `rotate_soft` 90° on a duplicate). Mayhem SOFT export is **one** stream (the primary tap). For **bit-identical** interleaved CADU parity vs SatDump on a `.C8` alone, derive the second stream off-line with the same `rotate_soft(..., PHASE_90, false)` semantics as SatDump, or run SatDump on IQ `.C16` directly.

**On-device `m2x && interleaved`:** when [`SharedMemory::MeteorLrptIpc`](../../firmware/common/portapack_shared_memory.hpp) is active (M0 opened `/LRPT/DEINT_*.BIN` and `deint_service` is running), M4 posts **8192** raw int8 bytes per block to `soft_in`; M0 runs dual `MeteorDeinterleaverReader` + sector-batched SD deinterleave, then M4 runs dual `MeteorViterbi1_2` + winner + BPSK deframer + RS (see `proc_meteor_lrpt_rx.cpp` / `meteor_lrpt_deint_service.cpp`). If IPC is not ready, the firmware falls back to a **legacy byte Viterbi** probe on the same 8192-byte block (UI `interleaved_mode_flags` bit0).

## References

- [`SATDUMP_VENDOR.md`](../../firmware/baseband/meteor_lrpt/SATDUMP_VENDOR.md) — pinned SatDump commit for `module_meteor_lrpt_decoder.cpp` / `deint.*`.
- [`tools/meteor_lrpt/README.md`](README.md) — tooling and gate table.
