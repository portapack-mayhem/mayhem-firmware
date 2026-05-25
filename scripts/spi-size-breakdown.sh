#!/usr/bin/env bash
# Print application.bin + baseband.img sizes (SPI content before padding) after a green build.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd -P)"
APP="$ROOT/build/firmware/application/application.bin"
BB="$ROOT/build/firmware/baseband/baseband.img"
FW="$ROOT/build/firmware/portapack-mayhem-firmware.bin"
for f in "$APP" "$BB" "$FW"; do
	if [[ ! -f "$f" ]]; then
		echo "spi-size-breakdown: missing $f (build first)" >&2
		exit 1
	fi
done
app_sz=$(wc -c <"$APP")
bb_sz=$(wc -c <"$BB")
fw_sz=$(wc -c <"$FW")
sum=$((app_sz + bb_sz))
echo "application.bin:  $app_sz bytes"
echo "baseband.img:     $bb_sz bytes"
echo "SPI content sum:  $sum bytes (+ 4 byte checksum in .bin)"
echo "portapack-mayhem-firmware.bin: $fw_sz bytes (padded to FLASH_MB_LIMIT)"
if [[ -f "$ROOT/build/firmware/baseband/baseband_meteor_lrpt_rx.bin" ]]; then
	echo "PMLR chunk:       $(wc -c <"$ROOT/build/firmware/baseband/baseband_meteor_lrpt_rx.bin") bytes"
fi
