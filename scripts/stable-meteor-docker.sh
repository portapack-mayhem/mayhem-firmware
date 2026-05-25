#!/usr/bin/env bash
# Stable Mayhem + Meteor — canonical Docker build (Linux / Git Bash / WSL).
# Repo root: run from mayhem-firmware directory.

set -euo pipefail
ORIG_ROOT="$(cd "$(dirname "$0")/.." && pwd -P)"
DOCKER_ROOT="${ORIG_ROOT}"

# Docker reading /mnt/c (DrvFS) often fails with "Input/output error" on ChibiOS headers.
# Copy to WSL ext4 first when the repo lives on a Windows mount.
if [[ "${ORIG_ROOT}" == /mnt/* ]]; then
	DOCKER_ROOT="${HOME}/mayhem-firmware-docker-build"
	echo "== Copying repo to WSL ext4 (${DOCKER_ROOT}) =="
	echo "   (avoids DrvFS I/O errors when Docker compiles from /mnt/c)"
	mkdir -p "${DOCKER_ROOT}"
	# hackrf/ often fails on DrvFS/NVIDIA-Workbench mounts; re-fetch on ext4 via git below.
	rsync -a --delete \
		--exclude build --exclude build-win \
		--exclude hackrf \
		"${ORIG_ROOT}/" "${DOCKER_ROOT}/" || {
		echo "WARNING: rsync had errors; retrying without --delete" >&2
		rsync -a \
			--exclude build --exclude build-win \
			--exclude hackrf \
			"${ORIG_ROOT}/" "${DOCKER_ROOT}/"
	}
	sed -i 's/\r$//' "${DOCKER_ROOT}/scripts/"*.sh 2>/dev/null || true
	chmod +x "${DOCKER_ROOT}/scripts/"*.sh "${DOCKER_ROOT}/scripts/docker-fix-perms.sh" 2>/dev/null || true
fi

cd "${DOCKER_ROOT}"

echo "== Submodules =="
git submodule update --init --recursive

echo "== Clean build/ (keep build-win for native Windows experiments) =="
./scripts/docker-fix-perms.sh 2>/dev/null || true
if [[ -d build ]]; then
	rm -rf build 2>/dev/null || docker run --rm -v "${DOCKER_ROOT}:/havoc" alpine:3.20 rm -rf /havoc/build
fi
mkdir -p build

echo "== Docker image =="
docker build -f dockerfile-nogit -t portapack-dev .

mkdir -p share-out/logs share-out/artifacts
LOG="share-out/logs/docker-make-$(date -u +%Y%m%dT%H%M%SZ).log"
echo "== Full build (log: ${LOG}) =="
# Default -j1 avoids OOM on Docker Desktop; raise with care: MAYHEM_MAKE_JOBS=2
JOBS="${MAYHEM_MAKE_JOBS:-1}"
FLASH_MB_SIZE="${MAYHEM_FLASH_MB_SIZE:-2}"
FLASH_LIMIT="${MAYHEM_FLASH_MB_LIMIT:-1}"
DOCKER_UID="$(id -u)"
DOCKER_GID="$(id -g)"
set +e
docker run --rm \
	-u "${DOCKER_UID}:${DOCKER_GID}" \
	-v "${DOCKER_ROOT}:/havoc" \
	portapack-dev bash -lc \
	"cd /havoc/build && cmake .. -DFLASH_MB_SIZE=${FLASH_MB_SIZE} -DFLASH_MB_LIMIT_SIZE=${FLASH_LIMIT} && make -j${JOBS}" \
	2>&1 | tee "$LOG"
rc="${PIPESTATUS[0]}"
set -e
"${DOCKER_ROOT}/scripts/docker-fix-perms.sh" 2>/dev/null || true
if [[ "$rc" -ne 0 ]]; then
	echo "Build failed (exit ${rc}) — see ${LOG}" >&2
	exit "$rc"
fi

echo "== Artifacts in build/ =="
ls -la build/firmware/portapack-mayhem-firmware.bin \
	build/firmware/portapack-mayhem_OCI.ppfw.tar \
	build/firmware/application/meteor_lrpt_rx.ppma 2>/dev/null || true

echo "== share-out/artifacts =="
"${DOCKER_ROOT}/scripts/share-out-collect.sh"
"${DOCKER_ROOT}/scripts/validate_meteor_artifacts.sh" 2>/dev/null || true

if [[ "${DOCKER_ROOT}" != "${ORIG_ROOT}" ]]; then
	echo "== Syncing share-out back to Windows tree =="
	mkdir -p "${ORIG_ROOT}/share-out"
	rsync -a --no-group --no-owner "${DOCKER_ROOT}/share-out/" "${ORIG_ROOT}/share-out/"
fi

echo "Done. SPI gate: grep the log for make_spi_image errors (no RuntimeError size exceed)."
echo "Logs: $LOG | Flash bundle: share-out/artifacts/"
