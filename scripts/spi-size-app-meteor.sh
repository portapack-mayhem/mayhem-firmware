#!/usr/bin/env bash
# Print largest Meteor/G4-related symbols in application.elf (run inside Docker or WSL with arm-none-eabi-size).
set -euo pipefail
ROOT="${1:-$(cd "$(dirname "$0")/.." && pwd -P)}"
ELF="$ROOT/build/firmware/application/application.elf"
SZ="${ARM_SIZE:-/opt/build/armbin/bin/arm-none-eabi-size}"
if [[ ! -f "$ELF" ]]; then
	echo "missing $ELF" >&2
	exit 1
fi
"$SZ" -t "$ELF" | grep -E 'meteor|tjpgd|msumr|g4|deint|jpeg' | sort -n
