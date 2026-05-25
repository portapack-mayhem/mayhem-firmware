#!/usr/bin/env bash
set -euo pipefail
DST="${HOME}/mayhem-firmware-1mb"
WIN=/mnt/c/Users/MeMyself/mayhem-firmware
rsync -a "${WIN}/firmware/" "${DST}/firmware/"
rsync -a "${WIN}/firmware/tools/" "${DST}/firmware/tools/"
docker run --rm \
  -v "${DST}:/havoc" \
  portapack-dev bash -lc 'cd /havoc/build && make -j1'
"${DST}/scripts/share-out-collect.sh"
rsync -a --no-group --no-owner "${DST}/share-out/" "${WIN}/share-out/"
cat "${DST}/share-out/artifacts/SHA256SUMS.txt"
