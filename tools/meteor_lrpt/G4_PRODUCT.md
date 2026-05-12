# Meteor LRPT G4 — MSU-MR product (Mayhem M0)

This document is the **operator + parity scope** for the on-device **G4** path: CCSDS Space Packet scan / light reassembly, **baseline JPEG** decode via vendored **TJpgDec (tjpgd)**, and **24 bpp BMP** output under `/LRPT/`.

## SatDump reference (pinned)

Use the same SatDump commit as [`firmware/baseband/meteor_lrpt/SATDUMP_VENDOR.md`](../../firmware/baseband/meteor_lrpt/SATDUMP_VENDOR.md) (`f7c2c04…`). Primary upstream stages for instruments:

- `plugins/meteor_support/meteor/module_meteor_instruments.cpp` — CADU → instruments pipeline.
- `plugins/meteor_support/meteor/instruments/msumr/*` — MSU-MR LRPT reader / JPEG assembly (e.g. `lrpt_msumr_reader.cpp`, `module_meteor_msumr_lrpt.cpp`).

Mayhem G4 is an **MVP subset**: per-APID reassembly in `msumr_demux` (APIDs **64–69**, one buffer each; `kReassemblyCapBytes` in-tree), fixed **10-byte** secondary header when the CCSDS secondary flag is set (common case; refine vs SatDump when extending).

## Memory / RAM budget (M0 + Bank2)

- **Bank2 shared RAM** (`portapack::memory::map::shared_memory`, **14 KiB**): holds `SharedMemory` including `MeteorLrptG4Ipc` (live CADU ring, UTF-8 paths, **RGB888 LCD preview** thumbnail, counters). `sizeof(SharedMemory) <= 14 KiB` is enforced by `static_assert` in [`firmware/common/portapack_shared_memory.cpp`](../../firmware/common/portapack_shared_memory.cpp).
- **M0 AHB SRAM** (HackRF One: **64 KiB** typical; see [`firmware/chibios-portapack/os/ports/GCC/ARMCMx/LPC43xx_M0/ld/LPC43xx_M0.ld`](../../firmware/chibios-portapack/os/ports/GCC/ARMCMx/LPC43xx_M0/ld/LPC43xx_M0.ld) / `LD_RAM_SIZE`): `MsumrDemux` static reassembly is **`kReassemblyStreams * kReassemblyCapBytes`** (see [`msumr_demux.hpp`](../../firmware/application/meteor_lrpt_g4/msumr_demux.hpp)) plus TJpgDec workspace and stacks — if the M0 link fails after raising caps, trim `kReassemblyCapBytes` or M0-only buffers before touching shared RAM.

## Live CADU ring (M4 → M0, optional)

When **MSU-MR** and **Live** are enabled, M4 pushes each **post-RS 1020 B** CADU into a tiny fixed ring (`kLiveRingSlots` in [`portapack_shared_memory.hpp`](../../firmware/common/portapack_shared_memory.hpp)). This keeps **RAM fixed** (no second full-rate REC buffer in the live path).

**Overflow policy (SPSC-safe):** if the ring is full (`ring_push - ring_pop == kLiveRingSlots`), M4 **does not** advance `ring_pop` (only M0 may). The new CADU is **dropped**, `live_ring_overflows` increments, and `G4_DROP_RING_OVERFLOW` is OR’d into `drop_bits`. The LRPT status line shows this as `ro` (ring overflow count).

For **lossless capture** of a pass (as in typical LRPT workflows: record first, decode after), use **CADU `RecordView`** to SD and the G4 **tail file** path—disk I/O, bounded RAM—aligned with practical LRPT reception guides (e.g. [Meteor satellite reception — a-centauri](https://a-centauri.com/articoli/meteor-satellite-reception)): strong FEC, but avoid overloading USB/SD so samples are not dropped upstream.

## Operator workflow (file + LCD)

### CADU tail file (REC row **CADU**)

1. Capture **CADU** with the **CADU** `RecordView` row. Mayhem writes **raw S8** with extension **`.C8`** and the stem you choose (e.g. `CADU_…`), under folder **`LRPT`** (see [`ui_record_view.cpp`](../../firmware/application/ui_record_view.cpp): `replace_extension(..., u".C8")`). Each logical **1020 B** Mayhem CADU record is consumed sequentially by the G4 worker (`read` of 1020 bytes per step).
2. Point the G4 path (default **`/LRPT/g4_cadu.C8`**) at that capture: copy/rename/symlink so the path matches, or use **Path** in the Meteor LRPT app. The default extension **`.C8`** matches `RecordView`; it is **not** required to rename to `.bin` / `.cadu` unless you prefer a custom name.
3. Open **Meteor LRPT**, enable **MSU-MR** (G4). Status line shows `G4 c… j… b… ro… x… jr…` — `ro` = `live_ring_overflows` (live ring only); `c/j/b` from shared memory (`cadus_processed`, `jpeg_ok_count`, `bmp_write_count`); `x` = `drop_bits` hex; `jr` = last `JRESULT`.
4. BMP products appear as **`/LRPT/msumr_<apid>_<timestamp>.bmp`**. The worker copies the last path into shared **`last_bmp_utf8`** and fills a small **RGB888 thumbnail** (`preview_seq` / `preview_rgb`); the UI draws it below the M4 preview strip when `preview_seq` changes.
5. **Img** + **^** / **v**: optional **paged BMP viewer** (read-only `File` + BMP header) for the last decoded BMP, scaled to 240×44 rows (scroll in image space). Keep **MSU-MR** enabled so status refresh runs.

**Live tail / second REC:** the worker reads sequential 1020-byte frames while the tail file grows. If the file is **truncated** or replaced (shorter than `cadu_off`), the worker resets the file offset and demux state. Avoid opening the same file for read/write from two handles simultaneously if your FatFs build is strict; safest: copy the recording when the pass ends, then enable G4.

### `drop_bits` (diagnostics)

| Bit (hex) | Name | Meaning |
|-----------|------|--------|
| `1<<0` | `G4_DROP_BAD_ASM` | CADU ASM marker mismatch |
| `1<<1` | `G4_DROP_DEMUX_SPARSE` | Reassembly continuation without active segment |
| `1<<2` | `G4_DROP_PACKET_PARSE` | Space packet parse / unsupported fragmented APID (outside 64–69) |
| `1<<3` | `G4_DROP_REASSEMBLY_OVERFLOW` | Per-APID buffer exceeded `kReassemblyCapBytes` |
| `1<<4` / `1<<5` | JPEG prepare / decomp | TJpgDec failures |
| `1<<6` | `G4_DROP_BMP` | BMP write / encode path |
| `1<<7` | Progressive JPEG | Not supported |
| `1<<8` | `G4_DROP_RING_OVERFLOW` | Live ring full (M4 dropped incoming CADU) |
| `1<<9` | `G4_DROP_VCID_MISMATCH` | VCID mismatch in reassembly |
| `1<<10` | `G4_DROP_PACKET_CRC` | PEC / CRC-16 path (reserved) |
| `1<<11` | `G4_DROP_CADU_IO` | CADU tail **seek/read** failure or short read (not BMP) |

## JPEG / decoder policy

- **Baseline JPEG only.** TJpgDec does **not** support progressive DCT (`SOF2` / `0xFFC2`). The decoder rejects progressive inputs (`drop_bits` includes progressive, `last_jresult` set).
- MCU output order follows TJpgDec; `BMPFile` writes pixels in the order produced by `jd_decomp` (MCU rectangles).

## Visual acceptance vs SatDump (host)

For a given pass, export SatDump’s RGB product and Mayhem’s BMP, then compare on a workstation:

- **Byte-identical BMP** after converting SatDump output to uncompressed 24 bpp BMP with identical dimensions, **or**
- **PSNR** on the luminance plane after resize alignment — document your threshold in regression notes (not automated in-repo for v1).

## On-target checklist (quick)

1. SD formatted; `/LRPT` exists; REC **CADU** `.C8` capture or symlink at configured G4 path.
2. **MSU-MR** on: `c` increases from tail or live ring; `j` / `b` increment on good decodes; `x` stays `0` on clean pass (investigate non-zero bits via table above).
3. After decode: new **`msumr_*.bmp`** on SD; LCD **thumbnail** updates (shared preview); optional **Img** viewer scrolls last BMP.
4. Toggle **Live** + high rate: watch `ro` and `G4_DROP_RING_OVERFLOW` if M0 cannot keep up.

## Licences

- **tjpgd (TJpgDec):** ChaN / Bodmer mirror; see `firmware/application/meteor_lrpt_g4/third_party/tjpgd/LICENSE.txt` and per-file headers (`tjpgd.c`).
- **SatDump-derived logic:** parity references are MIT at the pinned commit; Mayhem remains **GPL v2** — see `SATDUMP_VENDOR.md`.
