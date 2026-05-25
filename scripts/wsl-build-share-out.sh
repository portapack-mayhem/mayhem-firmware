#!/usr/bin/env bash
# One-shot: sync to WSL ext4, Docker build (SIZE=2 LIMIT=1), collect share-out, sync back to Windows tree.
# Default: incremental (keeps build/). Set MAYHEM_CLEAN_BUILD=1 for full clean (~15-25 min).
#
# share-out/artifacts after success:
#   portapack-mayhem-firmware.bin, portapack-mayhem_OCI.ppfw.tar
#   meteor-capture.ppma, meteor-decode.ppmp, meteor-view.ppmp (+ SHA256SUMS.txt)
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd -P)"
WIN="$(cd "${SCRIPT_DIR}/.." && pwd -P)"
DST="${HOME}/mayhem-firmware-1mb"
# -j2 can race external-app .bin generation on 1 MiB builds; use -j1 unless you know the tree is warm.
JOBS="${MAYHEM_MAKE_JOBS:-1}"
DOCKER_UID="$(id -u)"
DOCKER_GID="$(id -g)"

echo "== rsync Windows -> WSL ext4 (${DST}) =="
rsync -a --delete --exclude build --exclude build-win --exclude hackrf "${WIN}/" "${DST}/" || {
	echo "WARNING: rsync had errors; retrying without --delete" >&2
	rsync -a --exclude build --exclude build-win --exclude hackrf "${WIN}/" "${DST}/"
}
cd "${DST}"
sed -i 's/\r$//' scripts/*.sh
chmod +x scripts/docker-fix-perms.sh 2>/dev/null || true
echo "== git submodules (hackrf on ext4; skipped flaky DrvFS copy) =="
git submodule update --init --recursive
mkdir -p share-out/logs share-out/artifacts

# Docker writes build/ as root; fix ownership before rm or incremental make.
./scripts/docker-fix-perms.sh || true

if [[ "${MAYHEM_CLEAN_BUILD:-0}" == "1" ]]; then
	echo "== clean build/ (MAYHEM_CLEAN_BUILD=1) =="
	if [[ -d build ]]; then
		rm -rf build 2>/dev/null || docker run --rm -v "${DST}:/havoc" alpine:3.20 rm -rf /havoc/build
	fi
fi
mkdir -p build

LOG="share-out/logs/docker-make-$(date -u +%Y%m%dT%H%M%SZ).log"
echo "Logging to ${LOG} (make -j${JOBS}, docker -u ${DOCKER_UID}:${DOCKER_GID})"
echo "Targets: firmware (incl. application/*.ppma), standalone_apps, ppfw (portapack-mayhem_OCI.ppfw.tar)"
echo "  (not program-external-apps — that flashes/copies to a USB device, not for Docker)"

MAKE_TARGETS="firmware standalone_apps ppfw"
if [[ "${MAYHEM_RESUME_MAKE:-0}" == "1" && -f build/CMakeCache.txt ]]; then
	echo "MAYHEM_RESUME_MAKE=1: skipping cmake (incremental make only)"
	DOCKER_BUILD="cd /havoc/build && bash ../scripts/spi-prep-1mb.sh && make -j${JOBS} ${MAKE_TARGETS}"
else
	DOCKER_BUILD="cd /havoc/build && cmake .. -DFLASH_MB_SIZE=2 -DFLASH_MB_LIMIT_SIZE=1 && \
	 test -f firmware/flashsize.h || { echo 'ERROR: firmware/flashsize.h missing after cmake' >&2; exit 1; } && \
	 bash ../scripts/spi-prep-1mb.sh && \
	 make -j${JOBS} ${MAKE_TARGETS}"
fi

set +e
docker run --rm \
	-u "${DOCKER_UID}:${DOCKER_GID}" \
	-v "${DST}:/havoc" \
	portapack-dev bash -lc "${DOCKER_BUILD}" \
	2>&1 | stdbuf -oL tee "${LOG}"
rc="${PIPESTATUS[0]}"
set -e
./scripts/docker-fix-perms.sh || true

if [[ "$rc" -ne 0 ]]; then
	echo "Build failed (exit ${rc}) - see ${LOG}" >&2
	if [[ "$rc" -eq 125 ]]; then
		echo "Docker exit 125 usually means the container died (OOM, Docker Desktop restart, or WSL memory limit)." >&2
		echo "  1) Restart Docker Desktop, wait until it is healthy" >&2
		echo "  2) Resume without clean: MAYHEM_RESUME_MAKE=1 ./scripts/wsl-build-share-out.sh" >&2
		echo "  3) Keep MAYHEM_MAKE_JOBS=1; raise WSL RAM in %UserProfile%\\.wslconfig if this repeats" >&2
	fi
	exit "$rc"
fi

echo "== collect share-out artifacts (Meteor: meteor-capture / meteor-decode / meteor-view) =="
./scripts/share-out-collect.sh
./scripts/spi-gate-check.sh
./scripts/validate_meteor_artifacts.sh

echo "== Sync share-out -> Windows tree (${WIN}/share-out/) =="
mkdir -p "${WIN}/share-out"
rsync -a --no-group --no-owner "${DST}/share-out/" "${WIN}/share-out/"
grep -E 'FLASH_MB|MAYHEM_SPI' build/CMakeCache.txt || true
echo "== share-out/artifacts =="
ls -la share-out/artifacts/
wc -c share-out/artifacts/portapack-mayhem-firmware.bin
for m in meteor-capture.ppma meteor-decode.ppmp meteor-view.ppmp; do
	test -f "share-out/artifacts/${m}" || { echo "MISSING share-out/artifacts/${m}" >&2; exit 1; }
	wc -c "share-out/artifacts/${m}"
done
