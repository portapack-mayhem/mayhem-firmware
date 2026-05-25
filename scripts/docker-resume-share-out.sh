#!/usr/bin/env bash
# Resume an interrupted Docker build without deleting build/ (same tree as last attempt).
# Logs always go to share-out/logs/. Default -j2; set MAYHEM_MAKE_JOBS to override.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd -P)"
cd "$ROOT"
chmod +x scripts/docker-fix-perms.sh 2>/dev/null || true
mkdir -p share-out/logs share-out/artifacts
LOG="share-out/logs/docker-make-resume-$(date -u +%Y%m%dT%H%M%SZ).log"
JOBS="${MAYHEM_MAKE_JOBS:-2}"
DOCKER_UID="$(id -u)"
DOCKER_GID="$(id -g)"
echo "Logging to $LOG (make -j${JOBS}, docker -u ${DOCKER_UID}:${DOCKER_GID})"
if [[ ! -d build ]]; then
	echo "ERROR: no build/ directory — run scripts/stable-meteor-docker.sh first." >&2
	exit 1
fi
if [[ -f build/CMakeCache.txt ]] && grep -qE 'c:/Users|C:/Users' build/CMakeCache.txt 2>/dev/null; then
	echo "ERROR: build/ was configured with Windows-native CMake paths; Docker will fail." >&2
	echo "Fix: ./scripts/docker-fix-perms.sh && rm -rf build && ./scripts/stable-meteor-docker-1mb.sh" >&2
	exit 1
fi
./scripts/docker-fix-perms.sh || true
FLASH_MB_SIZE="${MAYHEM_FLASH_MB_SIZE:-2}"
FLASH_LIMIT="${MAYHEM_FLASH_MB_LIMIT:-1}"
set +e
docker run --rm \
	-u "${DOCKER_UID}:${DOCKER_GID}" \
	-v "${ROOT}:/havoc" \
	portapack-dev bash -lc \
	"cd /havoc/build && cmake .. -DFLASH_MB_SIZE=${FLASH_MB_SIZE} -DFLASH_MB_LIMIT_SIZE=${FLASH_LIMIT} && make -j${JOBS}" \
	2>&1 | stdbuf -oL tee "$LOG"
rc="${PIPESTATUS[0]}"
set -e
./scripts/docker-fix-perms.sh || true
if [[ "$rc" -eq 0 ]]; then
	"$ROOT/scripts/share-out-collect.sh"
	./scripts/validate_meteor_artifacts.sh 2>/dev/null || true
	echo "Resume build OK; artifacts in share-out/artifacts/"
else
	echo "Build failed — see $LOG" >&2
	exit "$rc"
fi
