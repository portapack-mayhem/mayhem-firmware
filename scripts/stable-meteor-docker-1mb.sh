#!/usr/bin/env bash
# Stable Mayhem + Meteor for HackRF One / H4M with 1 MiB SPI (W25Q80-class).
# Match upstream Mayhem 1 MiB releases: SIZE=2 (SPI memory map) + LIMIT=1 (packed .bin).
set -euo pipefail
export MAYHEM_FLASH_MB_SIZE=2
export MAYHEM_FLASH_MB_LIMIT=1
ROOT="$(cd "$(dirname "$0")/.." && pwd -P)"
exec "$ROOT/scripts/stable-meteor-docker.sh"
