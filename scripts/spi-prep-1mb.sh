#!/usr/bin/env bash
# Before `make firmware` on H4M 1 MiB profile: drop stale baseband chunks from an older SPI pack.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd -P)"
CACHE="${ROOT}/build/CMakeCache.txt"
if [[ ! -f "$CACHE" ]]; then
	echo "spi-prep-1mb: no build/CMakeCache.txt (skip)" >&2
	exit 0
fi
if ! grep -q 'MAYHEM_SPI_1MB:INTERNAL=TRUE' "$CACHE"; then
	echo "spi-prep-1mb: MAYHEM_SPI_1MB not TRUE — configure with -DFLASH_MB_LIMIT_SIZE=1" >&2
	exit 1
fi
BB="${ROOT}/build/firmware/baseband"
FW_BIN="${ROOT}/build/firmware/portapack-mayhem-firmware.bin"
rm -f "${BB}/baseband.img" "${FW_BIN}" 2>/dev/null || true
for tag in PMLR PWTH PSGD PTST PSON PFSR PPVW PWFX PNOA PSCD PTNE; do
	rm -f "${BB}/${tag}.img" 2>/dev/null || true
done
rm -f "${BB}/baseband_meteor_lrpt_rx.bin" "${BB}/PMLR.img" 2>/dev/null || true
echo "spi-prep-1mb: cleared stale baseband.img, packed firmware .bin, and 1 MiB-excluded mode chunks"
