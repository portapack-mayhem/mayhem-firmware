# H4M boot failure after custom Meteor 1 MiB flash — investigation

## Symptoms

- Official `flashing/firmware/firmware_hackrf.bin` (1048576 bytes) boots.
- Custom `share-out/artifacts/portapack-mayhem-firmware.bin` (1048576 bytes, SPI gate OK) does **not** boot (blank screen / no Mayhem UI).
- **Boot splash hangs** on the first Mayhem loading icon (memory / USB step) and the unit never reaches the main menu or HackRF mode from the UI.

### Boot splash hang (immediate recovery)

1. **Unplug USB** from the HackRF (USB during boot can stall `usb_serial` init on the first splash icon).
2. **DFU recovery** (hardware DFU button on the HackRF — you do not need the PortaPack menu):
   - Run `flashing\mayhem_flasher.bat` → device **1** → action **2** (DFU then Mayhem) or **3** (DFU only first).
   - Flash **official** `flashing\firmware\firmware_hackrf.bin` (1048576 bytes), **not** `share-out\artifacts\` until a clean rebuild passes `validate_meteor_artifacts.sh`.
3. **RESET** the HackRF (short press). Confirm the normal Mayhem menu appears.
4. Only then flash a new custom image from a **clean** build (see below).

**Before reflashing custom firmware**, verify the `.bin` on the PC:

```bash
python scripts/compare_spi_bins.py
python3 -c "import sys; d=open('share-out/artifacts/portapack-mayhem-firmware.bin','rb').read(); sys.exit(0 if d.find(b'PMLR')<0 else 1)"
```

Exit code **0** means no `PMLR` chunk in the SPI pack (required for 1 MiB H4M Meteor profile). If `PMLR` is present at a high offset (e.g. `0xaf184`), the pack was built from **stale** `baseband.img` / `PMLR.img` — run `MAYHEM_CLEAN_BUILD=1 ./scripts/wsl-build-share-out.sh` (or at least ensure `spi-prep-1mb.sh` runs before every `make firmware`).

## Root causes (ranked)

### 1. Wrong CMake flash pair: `FLASH_MB_SIZE=1` (critical)

Upstream Mayhem **1 MiB release** images use:

- `FLASH_MB_SIZE=2` — SPI **memory map** size (`memory_map.hpp` → `spifi_uncached` / chunk region)
- `FLASH_MB_LIMIT_SIZE=1` — **packed** `portapack-mayhem-firmware.bin` size (1048576 bytes)

Our scripts previously forced **both to 1**. That shrinks the cached SPI region and the baseband chunk arena (`spi_image.hpp` `images.size = spifi_size - _textend`). Official recovery bins match **SIZE=2, LIMIT=1** (see `flashing/firmware/firmware_hackrf.bin` layout: baseband chunks start ~0xB4000 after a ~700 KiB application).

**Fix:** build with `-DFLASH_MB_SIZE=2 -DFLASH_MB_LIMIT_SIZE=1` (`scripts/stable-meteor-docker-1mb.sh`).

Do **not** confuse with a **2 MiB image** (`FLASH_MB_LIMIT_SIZE=2` → 2097152-byte `.bin`), which bricks 1 MiB W25Q80 hardware.

### 2. M0 AHB RAM overflow from Meteor static buffers (critical)

Meteor LRPT adds ~35 KiB of **static** deinterleave buffers in `meteor_lrpt_deint_service.cpp` plus ~4.5 KiB `MsumrDemux` BSS on a **64 KiB** M0 RAM part. That leaves too little room for stacks/heap/ChibiOS → corruption during `portapack::init()` (often `is_portapack_present()` / I2C) → **HackRF-only mode** (TX LED blink, no Mayhem UI).

**Symptom match:** `hackrf_spiflash` reports success; device stays in HackRF mode until DFU recovery with official 1 MiB.

**Fix:** heap-allocate deinterleave buffers on first `deint_service_configure()`, heap-allocate `MsumrDemux` in `meteor_lrpt_g4_init()`, restore `__process_stack_size__ = 0x1000` in `LPC43xx_M0.ld`.

### 3. Flash / image mismatch (user workflow)

| Mistake | Result |
|--------|--------|
| Flash **2097152**-byte build on 1 MiB SPI | Blank screen; recover with DFU + official 1 MiB |
| Interrupted Flash Utility / wrong SD path | Corrupt SPI; checksum OK in file but device bad |
| Mix `.ppma` from another build | Meteor broken; core Mayhem may still boot |

Flash Utility stops when the **file** ends (`proc_flash_utility.cpp`), but erase is always **full chip**. Use a complete 1048576-byte image from one `share-out/artifacts/` tree.

### 4. H4M-specific (usually OK in this tree)

- **AGM CPLD** `0x25610`: no JTAG CPLD update at boot (`portapack.cpp`).
- **Backlight**: AGM must use CAT4004 (fixed in `portapack.cpp` `backlight()`).
- **PMLR** is in external `.ppma`, not inside `baseband.img`; absence of `PMLR` tag in SPI is normal.

### 5. Not the boot blocker

- Version string `unknown` vs `v2.4.0` in firmware info — cosmetic.
- Omitted PWTH/PSGD/PSON baseband modes — only affect those apps, not boot.
- SPI checksum in `.bin` — both official and custom validate.

## Verification after rebuild

```bash
./scripts/stable-meteor-docker-1mb.sh
./scripts/spi-gate-check.sh
python scripts/compare_spi_bins.py   # HRF1 chunk should be ~0xED000+ on Meteor build
```

On device: DFU + official 1 MiB once if needed, then Flash Utility with new `portapack-mayhem_OCI.ppfw.tar` only from this build.

## SD inserted → boot hang (firmware OK without SD)

Symptom: official or custom Mayhem boots with **no** SD card; with SD inserted, stuck on boot loading icons.

Fix on device (no reflash): boot without SD → **SETTINGS** → **P.Memory Mgmt** → **Load mem defaults** → reboot → **SETTINGS** → **SD Card** → uncheck **enable high speed IO** → **Save** → reboot with SD.

Firmware hardening (this tree): `portapack::init()` mounts SD at 25 MHz before optional high-speed; UI re-applies high speed only if already mounted.

Do not enable high-speed SDIO until you have run the SD Card speed test on **your** card and confirmed stability.

## References

- `CMakeLists.txt` — default `FLASH_MB_SIZE=2`, `FLASH_MB_LIMIT_SIZE=1`
- `firmware/common/spi_image.hpp` — chunk region sizing
- `scripts/H4M_METEOR_BUILD.txt` — flash workflow
