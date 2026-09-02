#!/bin/bash
# Capture a golden IQ reference of real Meshtastic traffic while commanding the
# Heltec to transmit a known text on the default-PSK primary channel.
#
# Requires: HackRF in *HackRF mode* + 868 MHz antenna; Heltec on its serial port.
#
# Usage: ./capture.sh [out.cs8] [freq_hz] [rate_hz] [seconds]
set -e
cd "$(dirname "$0")"

OUT="${1:-captures/cap1.cs8}"
FREQ="${2:-868950000}"     # center; RU band 868.7-869.2, 2 Msps covers it
RATE="${3:-2000000}"
SECS="${4:-25}"
HELTEC_PORT="${MESHTASTIC_PORT:-$(python3 "$(dirname "$0")/ports.py" | awk '/Meshtastic/{print $2}')}"
PY=~/.venv/mesh/bin/python
MT=~/.venv/mesh/bin/meshtastic
export DYLD_FALLBACK_LIBRARY_PATH="/Applications/SDR++.app/Contents/Frameworks"

echo ">> Starting recorder: ${SECS}s @ ${FREQ} Hz, ${RATE} sps -> ${OUT}"
# Low gain: Heltec TX is strong & close.
$PY hackrf_io.py rx -f "$FREQ" -s "$RATE" -n "$SECS" -o "$OUT" --lna 8 --vga 12 --amp 0 &
RXPID=$!
sleep 3

# Fire several known texts spaced out so at least one lands cleanly in the capture.
for i in 1 2 3 4 5; do
  echo ">> Heltec sendtext BENCH-00$i"
  $MT --port "$HELTEC_PORT" --sendtext "BENCH-00$i" >/dev/null 2>&1 || echo "   (sendtext $i failed)"
  sleep 4
done

wait $RXPID
echo ">> Capture done: $OUT"
ls -la "$OUT"
