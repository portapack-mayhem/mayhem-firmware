# Meteor LRPT golden vectors and SatDump parity

## Practical reception guide (operator)

Independent **Meteor M LRPT** reception walk-through (SatDump UI, bias tee, sample rates, 72k vs 80k, Primary 137.9 MHz vs Backup 137.1 MHz, DC blocking, pass prediction):

- [METEOR-M (M2-3 / M2-4) satellite reception guide — a-centauri](https://a-centauri.com/articoli/meteor-satellite-reception)

Use that article as the **human-facing reference** for antenna, SatDump pipeline selection (`METEOR M2-x LRPT 72k`, optional `80k` test mode), and workflow. This `tools/meteor_lrpt/` folder tracks **Mayhem ↔ SatDump parity** (binary CADU / soft streams), not general LRPT theory.

## Gate status (see `.cursor/plans/meteor_lrpt_mayhem_6575c48e.plan.md`)

| Gate | Criterion | Mayhem status (high level) |
|------|-----------|----------------------------|
| **G1** | External app loads M4 (`PMLR`), RX usable at 137.9 MHz | **Meteor LRPT** external app + baseband image; Capture mode; default 137.9 MHz. |
| **G2** | Soft symbols or decimated IQ correlate vs SatDump on same golden `.C16` | **Partial:** M4 DC block + shaping FIR + soft buffer; `corr_soft.py` for host lag search. **Symbol timing:** Gardner-style TED + Catmull–Rom fractional strobes in [`proc_meteor_lrpt_rx.cpp`](../../firmware/baseband/proc_meteor_lrpt_rx.cpp); status `sym_timing_err` / `sym_timing_lock` in [`message.hpp`](../../firmware/common/message.hpp) (diagnostic; correlate with ASM/RS before treating as “RF lock”). Gains live in [`meteor_symbol_timing.hpp`](../../firmware/baseband/meteor_lrpt/meteor_symbol_timing.hpp). |
| **G3** | CADU stream matches SatDump (frame-aligned compare; RS repair counts) | **Partial:** Legacy path matches SatDump (correlator, rotate_soft, Viterbi, NRZ-M **bytes**, RS). **M2-x non-interleaved:** on-device `BPSK_CCSDS_Deframer` + NRZ-M **bits** + same Viterbi block as legacy. **M2-x interleaved:** when M0 IPC + SD rings are up, M4 runs dual `MeteorViterbi12` + winner + deframer + RS on deinterleaved streams; otherwise legacy byte probe. Host: [`SOFT_FORMAT.md`](SOFT_FORMAT.md), `test_m2x_pipeline_host.cpp`, `test_m2x_interleaved_ram_pipeline_host.cpp`, `m2x_interleaved_decode.py`, `SATDUMP_VENDOR.md`. Status **B** = RS-repair stress 0..1000 (not RF BER). **G** / lock = Gardner-style symbol timing diagnostic on M4. |
| **G4** | BMP/visual vs SatDump | **MVP on M0:** MSU-MR CADU scan (`msumr_demux`), baseline JPEG via **TJpgDec** (`firmware/application/meteor_lrpt_g4/third_party/tjpgd/`), 24 bpp BMP under `/LRPT/`. **Operator:** default tail **`/LRPT/g4_cadu.C8`** matches **CADU** `RecordView` raw extension; enable **MSU-MR** in the LRPT app — see [`G4_PRODUCT.md`](G4_PRODUCT.md). LCD **thumbnail** + optional **Img** paging viewer use shared `MeteorLrptG4Ipc` + `File` reads (no large `Message` payloads). **Live M4→M0 ring** optional via **Live** checkbox. |

## Pinned SatDump reference

- **Repository:** https://github.com/SatDump/SatDump  
- **Pin commit (SatDump `HEAD` at doc time):** `f7c2c04f8bca6fbba223c9337a4499f022d2412b` — update when rebasing parity work.
- **Vendoring manifest (per-file upstream map + raw URLs):** [firmware/baseband/meteor_lrpt/SATDUMP_VENDOR.md](../../firmware/baseband/meteor_lrpt/SATDUMP_VENDOR.md)
- **Primary sources for parity:**
  - `plugins/meteor_support/meteor/module_meteor_lrpt_decoder.cpp` — CADU FEC (legacy + M2-x).
  - `plugins/meteor_support/meteor/deint.*`, `deframer.*`
  - `plugins/meteor_support/meteor/module_meteor_instruments.cpp` — CADU to instruments (G4 / MSU-MR context).
  - `plugins/meteor_support/meteor/instruments/msumr/` — MSU-MR / JPEG path (e.g. `lrpt_msumr_reader.cpp`, `module_meteor_msumr_lrpt.cpp`).

Full operator steps and acceptance notes: **[G4_PRODUCT.md](G4_PRODUCT.md)**.
## Mayhem IQ capture (golden input)

1. Build Mayhem with Capture app; set SPEC bandwidth **150k** or **250k**; center **137.9 MHz** (Meteor M2-3 / M2-4 LRPT).
2. Record **`.C16`** + sidecar metadata (frequency, sample rate) from the SD `CAPTURES` folder.
3. In SatDump, ingest the same file as **File** source and run the **Meteor LRPT** pipeline (72k mode; 80k for testing if needed).
4. Export SatDump **CADU** or processed products; compute SHA-256 of both streams for regression notes below.

## Regression table (fill in after captures)

| Artifact | SHA-256 | Notes |
|----------|---------|-------|
| `pass01_mayhem.c16` | (fill) | Mayhem capture |
| `pass01_satdump.cadu` | (fill) | SatDump CADU output |
| `pass01_satdump_rgb.png` | (fill) | Reference image |

**Golden CADU / soft checks:** after filling paths, run:

`python tools/meteor_lrpt/golden_cadu_workflow.py path/mayhem.cadu path/satdump.cadu --frame 1020`

`python tools/meteor_lrpt/golden_cadu_workflow.py --validate-soft-file path/mayhem.C8 --soft-block-bytes 8192` (interleaved SOFT) or `--soft-block-bytes 16384` (legacy).

Dockerized host builds: [`run_reader_long_golden_docker.sh`](run_reader_long_golden_docker.sh) / [`run_reader_long_golden_docker.ps1`](run_reader_long_golden_docker.ps1).

If SatDump exports **1024-byte** records (ASM + 1020 body per chunk), add e.g. `--normalize-mayhem b`.

## Licence note

SatDump-derived code in `firmware/baseband/meteor_lrpt/` must remain compatible with SatDump’s licence and this project’s **GPL v2**. Prefer documented ports with file-level attribution headers and entries in **`SATDUMP_VENDOR.md`** (pinned commit + MIT/GPL/LGPL breakdown). **G4:** vendored **tjpgd** (ChaN / permissive) lives under `firmware/application/meteor_lrpt_g4/third_party/tjpgd/` with `LICENSE.txt`. After changing the pin or URL list, run `python tools/meteor_lrpt/verify_satdump_vendor_urls.py` (network) to confirm raw sources still resolve.

## SOFT stream (Mayhem REC)

Full byte layout, block size, and SatDump alignment notes: **[SOFT_FORMAT.md](SOFT_FORMAT.md)**.

### Interleaved M2-x (host parity)

1. Record IQ `.C16` with Mayhem Capture **or** SOFT `.C8` per `SOFT_FORMAT.md`.
2. Run **SatDump** with `METEOR M2-x LRPT 72k` and `interleaved: true` on the meteor_lrpt_decoder module (see a-centauri / SatDump docs).
3. Export SatDump CADU; compare with `golden_cadu_workflow.py` or `compare_cadu.py`.
4. Optional: `python tools/meteor_lrpt/m2x_interleaved_decode.py --dry-run` prints pointers; set `SATDUMP_CMD` for future subprocess wiring.

Firmware building blocks (ring + `deinterleave` core, PHASE_90 duplicate helper) live under `firmware/baseband/meteor_lrpt/` and `firmware/application/meteor_lrpt_sector_file_ring.*`. Live SD feasibility: **[SD_IOPS_GATE.md](SD_IOPS_GATE.md)**.

## Tooling

- `compare_cadu.py` — byte-aligned CADU comparison; options `--frame`, `--skip-a` / `--skip-b`, `--normalize-mayhem {none,a,b,both}`, `--max-frames`.
- `compare_bmp.py` — compare two **24 bpp** BMP pixel buffers (same WxH); optional `--max-diff` tolerance (G4 / regression).
- `golden_cadu_workflow.py` — thin wrapper that invokes `compare_cadu.py` with the same arguments (for docs/CI).
- `verify_satdump_vendor_urls.py` — HEAD-checks every `raw.githubusercontent.com` URL embedded in `SATDUMP_VENDOR.md` (use when rebasing the SatDump pin).
- `corr_soft.py` — exploratory lag correlation on raw int8 soft dumps (G2 aid).
- `deint_sector_histogram.py` — distinct 512-byte sectors per batch for the **write** loop and the **sequential read** pass in `meteor_deinterleave`; CSV includes `distinct_read_sectors`, `min_write_idx` / `max_write_idx`; see `data/deint_sector_metrics_example.csv`.
- `test_msumr_g4_host.cpp` — host smoke: CCSDS-wrapped CADU + `MsumrDemux` JPEG callback count; default run adds **multi-CADU reassembly** + **parallel APID 64/68** regression (Docker CI with `run_reader_long_golden_docker.*`).
- `test_tjpgd_g4_host.cpp` — host golden: vendored **TJpgDec** decode of `data/g4_tiny_baseline.jpg` (dims + RGB accumulator vs pinned constants).
- **M0:** `meteor_deinterleave_file_ring` in [`firmware/application/meteor_lrpt_sector_file_ring.*`](firmware/application/meteor_lrpt_sector_file_ring.hpp) — FatFs-efficient deinterleave (sector batch writes + sequential sector reads); use when the ring backend is `MeteorDeintFileRing` (not used from M4 baseband yet; see Option A plan).
- `m2x_interleaved_decode.py` — host helper for SatDump-equivalent interleaved M2-x CADU (see script `--help`).
- `SD_IOPS_GATE.md` — how to benchmark SD RMW vs `deint_sector_histogram.py` for live-decode feasibility.
- `test_external_ring_host.cpp` — optional host compile of RAM ring + `meteor_deinterleave` + short-path `read_samples` (see file header `g++` line).
- `test_reader_long_golden_host.cpp` — long-path `read_samples(8192)` regression: compares FNV-1a digest of the full ring; refresh constant with `--dump-golden`. Run under Docker: `tools/meteor_lrpt/run_reader_long_golden_docker.ps1` or `.sh`.
- **Mayhem SOFT `.C8` REC:** In **Meteor LRPT** app, use the second **SOFT** row (16 384-byte writes). Each block is one Viterbi input buffer (`int8` I/Q pairs). Do **not** run SOFT and CADU recorders at the same time (single M4 `CaptureConfig` path).
- `test_nrzm_standalone.c` — host-only smoke compile: `gcc -Wall -Wextra -O2 test_nrzm_standalone.c -o test_nrzm && ./test_nrzm`

## Building firmware (ARM)

Full `portapack-h1` / baseband images need **gcc-arm-none-eabi** (see repo `dockerfile-nogit` / `./dockerize.sh`). **Docker Desktop must be running** on Windows, or use WSL/Linux with the toolchain and CMake installed. This environment was validated with: `python -m py_compile` on the Python tools and the NRZ-M host smoke test above.

## Mayhem firmware notes

- External app **Meteor LRPT** (RX menu) loads baseband tag `PMLR` / `image_tag_meteor_lrpt_rx`, keeps `ReceiverModel::Mode::Capture`, and runs the M4 chain at **3.072 Msps** IQ into the decimator stack documented in `proc_meteor_lrpt_rx.*`. **G2 soft path:** export a contiguous int8 soft buffer from the same tap as `soft_buf_` (e.g. SD raw sidecar or future `RecordView` mode) and run `corr_soft.py` against SatDump-exported soft for lag/scale sanity.
- **FEC parity:** legacy path includes depth-4 **RS(255,223)** after ASM (same interleave / basis as SatDump `decode_interlaved(..., false, 4, …)`). **M2-x non-interleaved:** `BPSK_CCSDS_Deframer` (SatDump port) + optional NRZ-M on bits. **M2-x interleaved:** M0 `MeteorDeintFileRing` + sector-batched `meteor_deinterleave_file_ring` + dual `MeteorDeinterleaverReader`; M4 `MeteorViterbi12` (SatDump `Viterbi1_2` port) + `M2xInterleavedPostDeintPipeline`. Host smoke: compile [`test_m2x_pipeline_host.cpp`](test_m2x_pipeline_host.cpp) (see file header `g++` line).
- **SD / RecordView:** SPEC **150k** or **250k** is recommended for LRPT. Very high SPEC bandwidths can hit **yellow** record thresholds in `RecordView::set_sampling_rate` (CPU + SD card); if recording glitches, reduce bandwidth or use a faster SD card.
- **PRALINE builds:** re-check `receiver_model.cpp` LPF / **PRALINE** bandwidth selection after changing default sample rates or anti-alias paths for this app.
- **Freqman:** copy `sdcard/FREQMAN/METEOR_LRPT.TXT` to the SD card for quick LRPT presets (137.9 MHz primary, 137.1 MHz backup).
- **Licence:** SatDump-derived sources under `firmware/baseband/meteor_lrpt/` carry file-level notes (SatDump MIT, Phil Karn LGPL for RS core, etc.). Mayhem remains **GPL v2**; see root `COPYING`.

## User guide (short)

1. Antenna: RHCP feed for Meteor **M2-3 / M2-4** LRPT near **137.9 MHz**.
2. Open **Receive → Meteor LRPT**; set frequency; optional **REC** writes **1020-byte** CADU records (first four bytes = ASM `1A CF FC 1D` when locked) under `/LRPT`. Compare to SatDump with `compare_cadu.py --frame 1020` or `--frame 1024` depending on export layout.
3. Toggle **80k sym** only when capturing an 80 ksym/s variant; leave off for nominal **72k**.
4. **BMP** (checkbox) saves a thin grayscale scroll of JPEG-preview heuristics (not full MSU-MR RGB product).
5. Compare against SatDump using the pinned commit; run `golden_cadu_workflow.py` or `compare_cadu.py` (see **Golden CADU check** above).

## Regression / iteration (M2-x interleaved work)

1. After changing `meteor_deinterleaver.*`, `external_ring.*`, or SatDump pin: run `python tools/meteor_lrpt/verify_satdump_vendor_urls.py`.
2. Re-run `deint_sector_histogram.py` if `deinterleave` constants change.
3. Re-run `golden_cadu_workflow.py` on stored golden CADU pair; if mismatch, bisect host SatDump vs new ring-backed prototype (`test_external_ring_host.cpp`).
4. After changing `meteor_deinterleaver_reader.*` or `meteor_soft_correlate.*` (rotate/autocorr): run `tools/meteor_lrpt/run_reader_long_golden_docker.ps1` (or `.sh`); if the ring digest changes intentionally, re-run `test_reader_long_golden_host.cpp` with `--dump-golden` and update the `kExpectedRingFnv1a64` constant.
