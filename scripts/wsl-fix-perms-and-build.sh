#!/usr/bin/env bash
set -euo pipefail
DST="${HOME}/mayhem-firmware-1mb"
WIN=/mnt/c/Users/MeMyself/mayhem-firmware
rsync -a "${WIN}/" "${DST}/" --exclude build --exclude build-win
cd "${DST}"
sed -i 's/\r$//' scripts/*.sh
chmod +x scripts/docker-fix-perms.sh scripts/wsl-build-share-out.sh
./scripts/docker-fix-perms.sh
exec ./scripts/wsl-build-share-out.sh
