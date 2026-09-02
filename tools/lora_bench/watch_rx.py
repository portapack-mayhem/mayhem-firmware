#!/usr/bin/env python3
"""Watch the Heltec RX-monitor and announce every PortaPack frame it decodes.

Usage:  ~/.venv/mesh/bin/python tools/lora_bench/watch_rx.py
Then open the Meshtastic app on the PortaPack (it broadcasts NodeInfo on launch
and every ~minute). Each decoded frame prints here. crc=OK = full success.
Ctrl-C to stop.
"""
import glob, sys, time
import serial

ports = [p for p in glob.glob('/dev/cu.usbmodem*') if 'Transceiver' not in p]
if not ports:
    print("Meshtastic node not found. Plug it in, or set MESHTASTIC_PORT=/dev/...")
    sys.exit(1)
port = ports[0]
print(f"Listening on {port} @115200 — open the Meshtastic app on the PortaPack now.")
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
