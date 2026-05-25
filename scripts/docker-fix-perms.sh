#!/usr/bin/env bash
# Docker runs as root; fix ownership so WSL user can rm build/ and rsync artifacts.
# Uses a tiny Alpine container (no sudo / no password prompt).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd -P)"
cd "$ROOT"
uid="$(id -u)"
gid="$(id -g)"

chown_tree() {
	local d="$1"
	[[ -e "$d" ]] || return 0
	if [[ -O "$d" ]] 2>/dev/null; then
		return 0
	fi
	if chown -R "${uid}:${gid}" "$d" 2>/dev/null; then
		echo "docker-fix-perms: chown ${d}"
		return 0
	fi
	# Root-owned after docker run — fix via container (never sudo; avoids password hang).
	if command -v docker >/dev/null; then
		docker run --rm -v "${ROOT}:/havoc" alpine:3.20 \
			chown -R "${uid}:${gid}" "/havoc/${d}" 2>/dev/null && {
			echo "docker-fix-perms: alpine chown ${d}"
			return 0
		}
	fi
	echo "docker-fix-perms: ERROR cannot chown ${d}" >&2
	return 1
}

chown_tree build || true
chown_tree share-out || true
exit 0
