# Meteor LRPT golden vectors and SatDump parity

## Practical reception guide (operator)

Independent **Meteor M LRPT** reception walk-through (SatDump UI, bias tee, sample rates, 72k vs 80k, Primary 137.9 MHz vs Backup 137.1 MHz, DC blocking, pass prediction):

- [METEOR-M (M2-3 / M2-4) satellite reception guide — a-centauri](https://a-centauri.com/articoli/meteor-satellite-reception)

Use that article as the **human-facing reference** for antenna, SatDump pipeline selection (`METEOR M2-x LRPT 72k`, optional `80k` test mode), and workflow. This `tools/meteor_lrpt/` folder tracks **Mayhem ↔ SatDump parity** (binary CADU / soft streams), not general LRPT theory.

## Gate status (see `.cursor/plans/meteor_lrpt_mayhem_6575c48e.plan.md`)

| Gate | Criterion | Mayhem status (high level) |
|------|-----------|----------------------------|
| **G1** | External app loads M4 (`PMLR`), RX usable at 137.9 MHz | **Meteor LRPT** external app + baseband image; Capture mode; default 137.9 MHz. |
| **G2** | Soft symbols or decimated IQ correlate vs SatDump on same golden `.C16` | **Partial:** M4 DC block + shaping FIR + soft buffer; `corr_soft.py` for host lag search. **Todo:** optional M4→SD soft dump or host-side capture hook for automated G2; Gardner / symbol PLL when CPU allows. |
| **G3** | CADU stream matches SatDump (frame-aligned compare; RS repair counts) | **Partial:** Legacy path matches SatDump (correlator, rotate_soft, Viterbi, NRZ-M **bytes**, RS). **M2-x non-interleaved:** on-device `BPSK_CCSDS_Deframer` + NRZ-M **bits** + same Viterbi block as legacy (no SatDump `Viterbi1_2` BER re-lock yet). **M2-x interleaved:** `DeinterleaverReader` **~2.65 MiB** — not on M4; checkbox uses byte-ASM fallback (SatDump-inexact). **Tools:** `compare_cadu.py`, `golden_cadu_workflow.py`. Status **B** = RS-repair stress 0..1000 (not RF BER). |
| **G4** | BMP/visual vs SatDump | **Deferred until G3:** MSU-MR APID demux, JPEG (`tjpgd` on M0), RGB BMP — plan forbids heavy product work before G3. |

## Pinned SatDump reference

- **Repository:** https://github.com/SatDump/SatDump  
- **Pin commit (SatDump `HEAD` at doc time):** `f7c2c04f8bca6fbba223c9337a4499f022d2412b` — update when rebasing parity work.
- **Vendoring manifest (per-file upstream map + raw URLs):** [firmware/baseband/meteor_lrpt/SATDUMP_VENDOR.md](../../firmware/baseband/meteor_lrpt/SATDUMP_VENDOR.md)
- **Primary sources for parity:**
  - `plugins/meteor_support/meteor/module_meteor_lrpt_decoder.cpp` — CADU FEC (legacy + M2-x).
  - `plugins/meteor_support/meteor/deint.*`, `deframer.*`
  - `plugins/meteor_support/meteor/module_meteor_instruments.cpp` — CADU to instruments.
  - `plugins/meteor_support/meteor/instruments/msumr/` — MSU-MR / JPEG path.

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

**Golden CADU check:** after filling paths, run:

`python tools/meteor_lrpt/golden_cadu_workflow.py path/mayhem.cadu path/satdump.cadu --frame 1020`

If SatDump exports **1024-byte** records (ASM + 1020 body per chunk), add e.g. `--normalize-mayhem b`.

## Licence note

SatDump-derived code in `firmware/baseband/meteor_lrpt/` must remain compatible with SatDump’s licence and this project’s **GPL v2**. Prefer documented ports with file-level attribution headers and entries in **`SATDUMP_VENDOR.md`** (pinned commit + MIT/GPL/LGPL breakdown). After changing the pin or URL list, run `python tools/meteor_lrpt/verify_satdump_vendor_urls.py` (network) to confirm raw sources still resolve.

## Tooling

- `compare_cadu.py` — byte-aligned CADU comparison; options `--frame`, `--skip-a` / `--skip-b`, `--normalize-mayhem {none,a,b,both}`, `--max-frames`.
- `golden_cadu_workflow.py` — thin wrapper that invokes `compare_cadu.py` with the same arguments (for docs/CI).
- `verify_satdump_vendor_urls.py` — HEAD-checks every `raw.githubusercontent.com` URL embedded in `SATDUMP_VENDOR.md` (use when rebasing the SatDump pin).
- `corr_soft.py` — exploratory lag correlation on raw int8 soft dumps (G2 aid).
- **Mayhem SOFT `.C8` REC:** In **Meteor LRPT** app, use the second **SOFT** row (16 384-byte writes). Each block is one Viterbi input buffer (`int8` I/Q pairs). Do **not** run SOFT and CADU recorders at the same time (single M4 `CaptureConfig` path).
- `test_nrzm_standalone.c` — host-only smoke compile: `gcc -Wall -Wextra -O2 test_nrzm_standalone.c -o test_nrzm && ./test_nrzm`

## Building firmware (ARM)

Full `portapack-h1` / baseband images need **gcc-arm-none-eabi** (see repo `dockerfile-nogit` / `./dockerize.sh`). **Docker Desktop must be running** on Windows, or use WSL/Linux with the toolchain and CMake installed. This environment was validated with: `python -m py_compile` on the Python tools and the NRZ-M host smoke test above.

## Mayhem firmware notes

- External app **Meteor LRPT** (RX menu) loads baseband tag `PMLR` / `image_tag_meteor_lrpt_rx`, keeps `ReceiverModel::Mode::Capture`, and runs the M4 chain at **3.072 Msps** IQ into the decimator stack documented in `proc_meteor_lrpt_rx.*`. **G2 soft path:** export a contiguous int8 soft buffer from the same tap as `soft_buf_` (e.g. SD raw sidecar or future `RecordView` mode) and run `corr_soft.py` against SatDump-exported soft for lag/scale sanity.
- **FEC parity:** legacy path includes depth-4 **RS(255,223)** after ASM (same interleave / basis as SatDump `decode_interlaved(..., false, 4, …)`). **M2-x non-interleaved:** `BPSK_CCSDS_Deframer` (SatDump port) + optional NRZ-M on bits; **M2-x interleaved** still needs phased deinterleave + dual-stream Viterbi — firmware falls back to the legacy byte-ASM probe. SatDump `Viterbi1_2` (BER-based re-lock / IQ swap) is **not** ported yet.
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
