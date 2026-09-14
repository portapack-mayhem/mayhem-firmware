#!/usr/bin/env python3
"""Put the PortaPack's Meshtastic app on a named preset, and prove it did.

Reads the preset off the screen first and steps from there. Counting clicks from
an assumed starting point is how this went wrong three times: the field is an
OptionsField, so touching it only takes focus and the encoder does the moving.
"""
import subprocess, sys, time, serial
import os
from ports import portapack_port

PORT = portapack_port()
HERE = os.path.dirname(os.path.abspath(__file__))
# MODEM_PRESETS order in mesh_regions.hpp.
PRESETS = ["LONG_FAST", "LONG_SLOW", "VERY_LONG_SLOW", "MEDIUM_SLOW", "MEDIUM_FAST",
           "SHORT_SLOW", "SHORT_FAST", "SHORT_TURBO", "LONG_MODERATE"]

def send(cmd, wait=2.2):
    with serial.Serial(PORT, 115200, timeout=0.2) as s:
        s.write((cmd + "\r\n").encode()); s.flush(); time.sleep(wait); s.read(16384)

def screen(tag="ps"):
    subprocess.run([sys.executable, f"{HERE}/shot.py", f"/tmp/{tag}.png"],
                   capture_output=True)
    out = subprocess.run([sys.executable, f"{HERE}/ocr.py", f"/tmp/{tag}.png", "8x16"],
                         capture_output=True, text=True).stdout
    return out

def current_preset():
    for line in screen().splitlines():
        if "Preset:" in line:
            return line.split("Preset:")[1].strip()
    return None

def main(target):
    if target not in PRESETS:
        print(f"unknown preset {target}"); return 1
    now = current_preset()
    if now is None:
        print("not on the Radio page - open Setup > Radio first"); return 1
    print(f"on {now}, want {target}")
    if now == target:
        print("already there"); return 0
    # Focus the field, then step. Read back after every click rather than trusting
    # the count: a dropped click is silent, and so is a click that moved something else.
    send("touch 110 52")
    for _ in range(len(PRESETS) + 1):
        now = current_preset()
        if now == target:
            print(f"set to {now}"); return 0
        want_i, now_i = PRESETS.index(target), PRESETS.index(now)
        send("button 8" if (want_i - now_i) % len(PRESETS) <= len(PRESETS) // 2 else "button 7", 1.4)
    print(f"stuck at {current_preset()}"); return 1

if __name__ == "__main__":
    sys.exit(main(sys.argv[1]))
