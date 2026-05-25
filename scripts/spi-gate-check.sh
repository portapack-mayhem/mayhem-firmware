#!/usr/bin/env bash
# SPI size gate: scan latest docker make log(s) for make_spi_image failure text.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd -P)"
cd "$ROOT"
shopt -s nullglob
logs=(share-out/logs/docker-make*.log share-out/logs/docker-make-resume*.log)
if [[ ${#logs[@]} -eq 0 ]]; then
	echo "spi-gate-check: no logs under share-out/logs/" >&2
	exit 2
fi
latest="$(printf '%s\n' "${logs[@]}" | sort | tail -1)"
echo "spi-gate-check: using $latest"
if grep -E 'RuntimeError|SPI flash image size.*exceed' "$latest"; then
	echo "spi-gate-check: FAIL (SPI overflow or Python error in log)" >&2
	exit 1
fi
if grep -q 'Space remaining in flash ROM:' "$latest"; then
	grep 'Space remaining in flash ROM:' "$latest" | tail -1
else
	echo "spi-gate-check: WARNING: no 'Space remaining' line (build may have failed before make_spi_image)" >&2
fi
BIN="$ROOT/share-out/artifacts/portapack-mayhem-firmware.bin"
if [[ -f "$BIN" ]]; then
	sz=$(wc -c <"$BIN")
	if [[ "$sz" -ne 1048576 ]]; then
		echo "spi-gate-check: FAIL bin size $sz (want 1048576 for 1 MiB H4M)" >&2
		exit 1
	fi
	echo "spi-gate-check: bin size OK ($sz bytes)"
fi
echo "spi-gate-check: PASS"
