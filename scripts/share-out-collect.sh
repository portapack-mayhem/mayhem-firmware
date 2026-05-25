#!/usr/bin/env bash
# Copy Mayhem + Meteor artifacts and hashes into share-out/artifacts (run after a green build).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd -P)"
OUT="$ROOT/share-out/artifacts"
LOGDIR="$ROOT/share-out/logs"
mkdir -p "$OUT" "$LOGDIR"

BIN="$ROOT/build/firmware/portapack-mayhem-firmware.bin"
PPFW="$ROOT/build/firmware/portapack-mayhem_OCI.ppfw.tar"
CAPTURE_SRC="$ROOT/build/firmware/application/meteor_capture.ppma"
DEC_SRC="$ROOT/build/firmware/standalone/meteor_lrpt_decode/meteor_lrpt_decode_app.ppmp"
VIEW_SRC="$ROOT/build/firmware/standalone/meteor_lrpt_view/meteor_lrpt_view_app.ppmp"

CAPTURE_OUT="$OUT/meteor-capture.ppma"
DECODE_OUT="$OUT/meteor-decode.ppmp"
VIEW_OUT="$OUT/meteor-view.ppmp"

for f in "$BIN" "$PPFW" "$CAPTURE_SRC" "$DEC_SRC" "$VIEW_SRC"; do
  if [[ ! -f "$f" ]]; then
    echo "share-out-collect: missing $f" >&2
    exit 1
  fi
done

python3 "$ROOT/scripts/check_spi_no_pmlr_chunk.py" "$BIN"

cp -f "$BIN" "$PPFW" "$OUT/"
cp -f "$CAPTURE_SRC" "$CAPTURE_OUT"
cp -f "$DEC_SRC" "$DECODE_OUT"
cp -f "$VIEW_SRC" "$VIEW_OUT"

cap_sz=$(wc -c <"$CAPTURE_OUT")
if [[ "$cap_sz" -lt 1024 ]]; then
  echo "share-out-collect: ERROR: meteor-capture.ppma is only $cap_sz bytes (rebuild application + export; need .external_app_meteor_lrpt_capture)" >&2
  exit 1
fi

bin_sz=$(wc -c <"$OUT/portapack-mayhem-firmware.bin")
if [[ "$bin_sz" -ne 1048576 ]]; then
  echo "share-out-collect: ERROR: portapack-mayhem-firmware.bin is $bin_sz bytes (expected 1048576 for 1 MiB H4M)" >&2
  exit 1
fi

CACHE="$ROOT/build/CMakeCache.txt"
PROFILE="$OUT/build-profile.txt"
if [[ -f "$CACHE" ]]; then
  clog="$LOGDIR/cmake-flash-$(date -u +%Y%m%dT%H%M%SZ).log"
  {
    echo "share-out-collect FLASH_* from CMakeCache.txt ($(date -u +%Y-%m-%dT%H:%M:%SZ))"
    grep -E 'FLASH_MB_(SIZE|LIMIT_SIZE):|MAYHEM_SPI_1MB:' "$CACHE" || true
  } >>"$clog"
  grep -E '^(FLASH_MB_SIZE|FLASH_MB_LIMIT_SIZE|MAYHEM_SPI_1MB):' "$CACHE" >"$PROFILE" || true
  echo "share-out-collect: logged FLASH_MB lines to $clog"
  echo "share-out-collect: wrote $PROFILE"
fi

(
  cd "$OUT"
  sha256sum portapack-mayhem-firmware.bin portapack-mayhem_OCI.ppfw.tar \
    meteor-capture.ppma meteor-decode.ppmp meteor-view.ppmp > SHA256SUMS.txt
)

echo "share-out-collect: wrote $OUT"
echo "  Meteor SD apps: meteor-capture.ppma meteor-decode.ppmp meteor-view.ppmp"
ls -la "$OUT"/meteor-capture.ppma "$OUT"/meteor-decode.ppmp "$OUT"/meteor-view.ppmp
