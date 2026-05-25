#!/usr/bin/env bash
# After a build: verify Meteor-related artifacts exist and ppfw.tar has expected top-level dirs.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd -P)"
cd "$ROOT"
mkdir -p share-out/logs
LOG="share-out/logs/validate-$(date -u +%Y%m%dT%H%M%SZ).log"
exec > >(tee -a "$LOG") 2>&1
echo "validate_meteor_artifacts log: $LOG"

# Build tree may live only on WSL ext4 (~/mayhem-firmware-1mb); share-out is synced to Windows.
BUILD_ROOT="${VALIDATE_BUILD_ROOT:-}"
if [[ -z "$BUILD_ROOT" ]]; then
  if [[ -f "${HOME}/mayhem-firmware-1mb/build/CMakeCache.txt" ]] \
     && grep -qE '^FLASH_MB_SIZE:' "${HOME}/mayhem-firmware-1mb/build/CMakeCache.txt"; then
    BUILD_ROOT="${HOME}/mayhem-firmware-1mb/build"
    echo "NOTE: using WSL build tree $BUILD_ROOT"
  elif [[ -f "$ROOT/build/CMakeCache.txt" ]] \
     && grep -qE '^FLASH_MB_SIZE:' "$ROOT/build/CMakeCache.txt"; then
    BUILD_ROOT="$ROOT/build"
  elif [[ -f "${HOME}/mayhem-firmware-1mb/build/CMakeCache.txt" ]]; then
    BUILD_ROOT="${HOME}/mayhem-firmware-1mb/build"
    echo "NOTE: WSL build tree present but incomplete CMakeCache; flash checks use build-profile.txt"
  fi
fi

SHARE_OUT="$ROOT/share-out/artifacts"
SHARE_CAPTURE="$SHARE_OUT/meteor-capture.ppma"
SHARE_DECODE="$SHARE_OUT/meteor-decode.ppmp"
SHARE_VIEW="$SHARE_OUT/meteor-view.ppmp"
PROFILE="$SHARE_OUT/build-profile.txt"

expect_cache_val() {
  local key="$1" want="$2"
  local line val
  line="$(grep -E "^${key}:" "$CACHE" | head -1 || true)"
  if [[ -z "$line" ]]; then
    echo "ERROR: ${key} not found in $CACHE"
    exit 1
  fi
  val="${line##*=}"
  if [[ "$val" != "$want" ]]; then
    echo "ERROR: ${key}=${val} (expected ${want} for 1 MiB H4M Meteor stable build)"
    exit 1
  fi
}

CACHE=""
if [[ -n "$BUILD_ROOT" && -f "$BUILD_ROOT/CMakeCache.txt" ]] \
   && grep -qE '^FLASH_MB_SIZE:' "$BUILD_ROOT/CMakeCache.txt"; then
  CACHE="$BUILD_ROOT/CMakeCache.txt"
  expect_cache_val FLASH_MB_SIZE 2
  expect_cache_val FLASH_MB_LIMIT_SIZE 1
  echo "OK CMakeCache: FLASH_MB_SIZE=2 FLASH_MB_LIMIT_SIZE=1 ($CACHE)"
elif [[ -f "$PROFILE" ]]; then
  CACHE="$PROFILE"
  expect_cache_val FLASH_MB_SIZE 2
  expect_cache_val FLASH_MB_LIMIT_SIZE 1
  echo "OK build-profile (share-out): FLASH_MB_SIZE=2 FLASH_MB_LIMIT_SIZE=1"
  if [[ -n "$BUILD_ROOT" && -f "$BUILD_ROOT/CMakeCache.txt" ]]; then
    echo "NOTE: ignored incomplete $BUILD_ROOT/CMakeCache.txt (use WSL build or re-run share-out-collect)"
  fi
else
  echo "ERROR: no usable CMakeCache (need FLASH_MB_SIZE in build/) and no share-out/artifacts/build-profile.txt"
  echo "  Run: ./scripts/wsl-build-share-out.sh  (builds on WSL ext4 and syncs share-out)"
  exit 1
fi

# Prefer share-out artifacts (what you flash on Windows); fall back to build/ tree.
BIN="$SHARE_OUT/portapack-mayhem-firmware.bin"
PPFW="$SHARE_OUT/portapack-mayhem_OCI.ppfw.tar"
PPMA="$SHARE_CAPTURE"
DEC="$SHARE_DECODE"
VIEW="$SHARE_VIEW"
if [[ ! -f "$BIN" && -n "$BUILD_ROOT" ]]; then
  BIN="$BUILD_ROOT/firmware/portapack-mayhem-firmware.bin"
  PPFW="$BUILD_ROOT/firmware/portapack-mayhem_OCI.ppfw.tar"
  PPMA="$BUILD_ROOT/firmware/application/meteor_capture.ppma"
  DEC="$BUILD_ROOT/firmware/standalone/meteor_lrpt_decode/meteor_lrpt_decode_app.ppmp"
  VIEW="$BUILD_ROOT/firmware/standalone/meteor_lrpt_view/meteor_lrpt_view_app.ppmp"
  echo "NOTE: validating from build tree (share-out artifacts missing)"
fi

for f in "$BIN" "$PPFW" "$PPMA" "$DEC" "$VIEW"; do
  if [[ ! -f "$f" ]]; then
    echo "ERROR: missing $f"
    exit 1
  fi
  sz=$(wc -c <"$f")
  echo "OK $(basename "$f") size=$sz bytes ($f)"
  base="$(basename "$f")"
  if [[ "$base" == "meteor_capture.ppma" || "$base" == "meteor-capture.ppma" ]] && [[ "$sz" -lt 1024 ]]; then
    echo "ERROR: meteor capture .ppma only $sz bytes (export used wrong ELF section? rebuild application)"
    exit 1
  fi
  if [[ "$base" == "portapack-mayhem-firmware.bin" && "$sz" -ne 1048576 ]]; then
    echo "ERROR: firmware .bin must be 1048576 bytes for 1 MiB SPI (got $sz)"
    exit 1
  fi
done

hdr="$(dd if="$BIN" bs=1 skip=1024 count=8 2>/dev/null || true)"
if [[ "$hdr" != "HACKRFFW" ]]; then
  msg="HACKRFFW signature missing or unreadable at 0x400"
  if [[ "${VALIDATE_REQUIRE_HACKRFFW:-0}" == "1" ]]; then
    echo "ERROR: $msg (set VALIDATE_REQUIRE_HACKRFFW=0 to warn only)"
    exit 1
  else
    echo "WARNING: $msg"
  fi
else
  echo "OK HACKRFFW @0x400"
fi

echo "== ppfw.tar APPS listing (3 Meteor artifacts) =="
# Normalize paths (strip CR from Windows tar, drop optional ./ prefix).
mapfile -t PPFW_MEMBERS < <(tar tf "$PPFW" | tr -d '\r' | sed 's|^\./||')
ppfw_has_member() {
  local want="$1" m
  for m in "${PPFW_MEMBERS[@]}"; do
    [[ "$m" == "$want" ]] && return 0
  done
  return 1
}
tar tf "$PPFW" | tr -d '\r' | sed 's|^\./||' | grep -E '^APPS/meteor-' || true
for want in APPS/meteor-capture.ppma APPS/meteor-decode.ppmp APPS/meteor-view.ppmp; do
  if ! ppfw_has_member "$want"; then
    echo "ERROR: ppfw tar missing $want"
    exit 1
  fi
  echo "OK ppfw contains $want"
done
if ppfw_has_member 'APPS/meteor_lrpt_rx.ppma'; then
  echo "WARNING: legacy APPS/meteor_lrpt_rx.ppma still in tar (use meteor-capture only)"
fi

for sf in "$SHARE_CAPTURE" "$SHARE_DECODE" "$SHARE_VIEW"; do
  if [[ -f "$sf" ]]; then
    echo "OK share-out $(basename "$sf")"
  else
    echo "WARN: missing $sf (run share-out-collect.sh after build)"
  fi
done

PMLR_BIN="$BUILD_ROOT/firmware/baseband/baseband_meteor_lrpt_rx.bin"
if [[ -n "$BUILD_ROOT" && -f "$PMLR_BIN" ]]; then
  echo "ERROR: PMLR baseband built on 1 MiB profile (must be external PMLS in meteor-capture.ppma only)"
  exit 1
elif [[ -n "$BUILD_ROOT" ]]; then
  echo "OK build tree: no baseband_meteor_lrpt_rx.bin"
fi
if python3 "$ROOT/scripts/check_spi_no_pmlr_chunk.py" "$BIN"; then
  echo "OK SPI firmware.bin: no PMLR chunk (checked $BIN)"
else
  echo "ERROR: PMLR SPI chunk in portapack-mayhem-firmware.bin (stale baseband.img?)"
  echo "  Fix: MAYHEM_CLEAN_BUILD=1 ./scripts/wsl-build-share-out.sh"
  echo "  Or: rm -f build/firmware/baseband/baseband.img build/firmware/portapack-mayhem-firmware.bin && make firmware"
  exit 1
fi

M4_MAX=$((26 * 1024))
M4_PMLS=""
if [[ -n "$BUILD_ROOT" ]]; then
  M4_PMLS="$BUILD_ROOT/firmware/baseband/meteor_lrpt_capture.bin"
fi
if [[ -n "$M4_PMLS" && -f "$M4_PMLS" ]]; then
  m4_sz=$(wc -c <"$M4_PMLS")
  echo "OK PMLS.bin size=$m4_sz bytes (max $M4_MAX)"
  if [[ "$m4_sz" -gt "$M4_MAX" ]]; then
    echo "ERROR: PMLS baseband exceeds 26 KiB"
    exit 1
  fi
elif [[ -f "$PPMA" ]]; then
  tag=$(python3 -c "import struct; d=open(r'''$PPMA''','rb').read(84); print(bytes(d[76:80]).decode('ascii','replace'))")
  if [[ "$tag" == "PMLS" ]]; then
    echo "OK capture .ppma m4 tag=PMLS (PMLS.bin size check skipped)"
    if [[ -n "$M4_PMLS" ]]; then
      echo "NOTE: missing $M4_PMLS — rebuild baseband on WSL if you need full build-tree checks"
    fi
  else
    echo "ERROR: capture .ppma must bundle PMLS not $tag"
    exit 1
  fi
else
  echo "ERROR: missing meteor capture .ppma and PMLS.bin"
  exit 1
fi

if [[ -f "$PPMA" ]]; then
  m4_off=$(python3 -c "import struct; d=open(r'''$PPMA''','rb').read(84); print(struct.unpack_from('<I', d, 80)[0])")
  M0_METEOR_MAX=$((7168))
  echo "OK meteor-capture.ppma m4_app_offset=$m4_off (max M0 slot $M0_METEOR_MAX)"
  m0_len="$m4_off"
  if [[ "$m0_len" -gt "$M0_METEOR_MAX" ]]; then
    echo "ERROR: capture .ppma M0 section exceeds kMeteorM0PackagedCodeMaxBytes"
    exit 1
  fi
fi

APP_MAP=""
if [[ -n "$BUILD_ROOT" ]]; then
  APP_MAP="$BUILD_ROOT/firmware/application/application.map"
fi
PPMP_MAX=$((96 * 1024))
for ppmp in "$DEC" "$VIEW"; do
  if [[ -f "$ppmp" ]]; then
    psz=$(wc -c <"$ppmp")
    if [[ "$psz" -gt "$PPMP_MAX" ]]; then
      echo "ERROR: $(basename "$ppmp") $psz bytes exceeds loader limit $PPMP_MAX"
      exit 1
    fi
    echo "OK $(basename "$ppmp") size=$psz (max $PPMP_MAX)"
  fi
done
if [[ -n "$BUILD_ROOT" ]]; then
  for elf in \
    "$BUILD_ROOT/firmware/standalone/meteor_lrpt_decode/meteor_lrpt_decode_app.elf" \
    "$BUILD_ROOT/firmware/standalone/meteor_lrpt_view/meteor_lrpt_view_app.elf"; do
    if [[ -f "$elf" ]] && command -v arm-none-eabi-size >/dev/null 2>&1; then
      echo "== $(basename "$elf") =="
      arm-none-eabi-size "$elf" || true
    fi
  done
fi

if [[ -n "$APP_MAP" && -f "$APP_MAP" ]]; then
  if grep -q 'MeteorDeintArenaStorage\|kMeteorDeintArenaBytes' "$APP_MAP" 2>/dev/null; then
    echo "ERROR: 1 MiB OCI elf must not link Meteor deint arena (stub build)"
    exit 1
  fi
  if grep -q 'meteor_lrpt_capture_stub\|MeteorLrptCaptureStub' "$APP_MAP" 2>/dev/null || \
     grep -q 'meteor_lrpt_capture_stub' "$BUILD_ROOT/firmware/application/CMakeFiles/application.elf.dir" 2>/dev/null; then
    echo "OK application.map: no deint arena (1 MiB stub path)"
  else
    echo "OK application.map: no deint arena symbols"
  fi
fi

echo ""
echo "== H4M device gauntlet (manual, flash one share-out set) =="
echo "  A0: Boot → Receive — 3 Meteor apps listed, no Guru"
echo "  A1: Meteor Capture — timing status, record 2–5 min soft .C8"
echo "  A2: /LRPT soft file grows (16384 B blocks)"
echo "  A3: Meteor Decode — soft → /LRPT/dec_cadu.C8"
echo "  A4: Meteor View — CADU → BMP under /LRPT/"
echo "  A5: Re-enter each app 10× — no MsgDblReg / SV#8"
echo "  A6: Capture soak 10+ min — M0 stack stable"
echo "validate_meteor_artifacts: OK"
