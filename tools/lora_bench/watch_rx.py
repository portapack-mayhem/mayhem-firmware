#!/usr/bin/env python3
"""Watch the Heltec RX-monitor and announce every PortaPack frame it decodes.

Usage:  python3 tools/lora_bench/watch_rx.py
Then open the Meshtastic app on the PortaPack (it broadcasts NodeInfo on launch
and every ~minute). Each decoded frame prints here. crc=OK = full success.
Ctrl-C to stop.
"""
import sys, time
import serial
from ports import meshtastic_port

# Through the shared helper rather than a glob of its own: that one only knew the
# macOS device names, so this script was the one piece of the bench that did not run
# on Linux.
port = meshtastic_port()
print(f"Listening on {port} @115200, open the Meshtastic app on the PortaPack now.")
print("Waiting for frames (Ctrl-C to stop)...\n")

s = serial.Serial(port, 115200, timeout=0.3)
ok = 0
last_hb = time.time()
buf = ""
while True:
    try:
        buf += s.read(4096).decode('utf-8', 'replace')
        while '\n' in buf:
            line, buf = buf.split('\n', 1)
            line = line.strip()
            if '>>> RX' in line:
                crc = 'crc=OK' in line
                if crc:
                    ok += 1
                tag = "✅ DECODED (CRC OK)" if crc else "⚠️  decoded, CRC fail"
                print(f"{tag}  [{ok} good so far]")
                print(f"    {line}\n")
            elif 'maxRSSI' in line:
                # quiet heartbeat: only note if a strong spike (TX nearby)
                try:
                    r = float(line.split('maxRSSI=')[1])
                    if r > -75:
                        print(f"    …RF energy seen (maxRSSI={r:.0f} dBm) — a frame is arriving")
                except Exception:
                    pass
        if time.time() - last_hb > 10:
            last_hb = time.time()
            print("    (still listening…)")
    except KeyboardInterrupt:
        print(f"\nStopped. Total CRC-OK frames from PortaPack: {ok}")
        break
