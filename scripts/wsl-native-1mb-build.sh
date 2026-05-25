#!/usr/bin/env bash
# Build 1 MiB Meteor on WSL ext4 (avoids /mnt/c Docker I/O errors). Syncs artifacts back to repo share-out/.
set -euo pipefail
SRC="$(cd "$(dirname "$0")/.." && pwd -P)"
# When invoked from Windows path via /mnt/c, map to a Linux home copy.
if [[ "$SRC" == /mnt/* ]]; then
	WIN_SRC="$SRC"
	SRC="${MAYHEM_WSL_BUILD_ROOT:-$HOME/mayhem-firmware-1mb}"
	mkdir -p "$SRC"
	rsync -a --delete --exclude build-win --exclude build "$WIN_SRC/" "$SRC/"
	rm -rf "$SRC/build"
fi
cd "$SRC"
sed -i 's/\r$//' scripts/*.sh 2>/dev/null || true
chmod +x scripts/*.sh
export MAYHEM_MAKE_JOBS="${MAYHEM_MAKE_JOBS:-1}"
export MAYHEM_FLASH_MB_SIZE=2
export MAYHEM_FLASH_MB_LIMIT=1
if [[ -d build ]] && grep -qE 'c:/Users|C:/Users' build/CMakeCache.txt 2>/dev/null; then
	rm -rf build
fi
if [[ ! -d build ]]; then
	./scripts/stable-meteor-docker-1mb.sh
else
	./scripts/docker-resume-share-out.sh
fi
if [[ "$SRC" != "$(cd "$(dirname "$0")/.." && pwd -P)" ]] && [[ -n "${WIN_SRC:-}" ]]; then
	rsync -a "$SRC/share-out/artifacts/" "$WIN_SRC/share-out/artifacts/"
	rsync -a "$SRC/share-out/logs/" "$WIN_SRC/share-out/logs/"
	rsync -a "$SRC/build/firmware/portapack-mayhem-firmware.bin" "$WIN_SRC/build/firmware/" 2>/dev/null || true
fi
