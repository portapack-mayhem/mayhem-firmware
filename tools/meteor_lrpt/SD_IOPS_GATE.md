# SD / FatFs gate for live M2-x interleaved deinterleave

## Goal

Decide whether **random 512-byte sector read–modify–write** on the deinterleaver ring can keep up with **real-time** soft-symbol production (~72 ksym/s class), using measured card + FatFs behavior on PortaPack hardware.

## Method (manual benchmark)

1. Format a representative SD card (FAT32 or exFAT as you use in the field).
2. Create a scratch file of size **2,654,208** bytes (full ring) on `/LRPT` (e.g. `DEINT1.BIN`).
3. In a small M0 loop or shell tool (not committed here), for **5 seconds**:
   - Pick random `sector_index` in `0 .. 5183`
   - `f_lseek` / `File::seek` to `sector_index * 512`
   - `read` 512 bytes, flip one byte, `write` 512 bytes, optional `f_sync` every N iterations
4. Count completed RMW cycles → **IOPS** (ops/s).

## Compare to histogram

Run:

`python tools/meteor_lrpt/deint_sector_histogram.py --batches 200 --summary-only`

Use **distinct write sectors per batch** (typical **576** for `len=8192` at `_offset=0`, `_cur_branch=0`) plus **distinct read sectors** printed by the same tool (sequential `read_byte` pass after the write loop in `meteor_deinterleave`). For a **naive** FatFs RMW upper bound per deinterleave batch, add write + read distinct counts (the script prints `write+read_distinct_sectors_upper_bound`; real RMW may be lower if read/write hit the same 512 B sector).

Multiply by **batches per second** implied by your decoder loop (SatDump uses **8192** output samples per `read_samples` per deinterleaver per outer iteration). **Dual SD ring files** (`DEINT_A` + `DEINT_B`): budget **~2×** single-ring sector traffic, plus extra seek penalty between files (measure).

If `required_RMW_per_sec` exceeds **sustained measured IOPS** with margin, treat **live on-device interleaved decode** as **not viable** and keep **host / SatDump** path (see `m2x_interleaved_decode.py`).

`MeteorDeintFileRing::create` preallocates the full **2_654_208**-byte file (see `firmware/application/meteor_lrpt_sector_file_ring.cpp`) so `seek`/`read` never see an undersized sparse file.

## FatFs notes (Mayhem)

- Sector size **512** (`firmware/common/ffconf.h` `_MIN_SS` / `_MAX_SS`).
- `_USE_FASTSEEK` helps long `seek` on large files.
- `File::write` requires full sector writes after merge for deterministic FatFs behavior (`firmware/application/file.cpp`).
