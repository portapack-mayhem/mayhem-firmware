#!/usr/bin/env python3
"""Capture PortaPack screen via Mayhem `screenframeshort` and save a PNG.
Each char = 1 pixel: value-32 -> bits [R2<<4|G2<<2|B2], 2 bits per channel."""
import sys, time, serial
import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from ports import portapack_port

PORT = portapack_port()
W, H = 240, 320
out = sys.argv[1] if len(sys.argv) > 1 else "captures/screen.png"

p = serial.Serial(PORT, 115200, timeout=0.5)
time.sleep(0.3)
p.reset_input_buffer()
p.write(b"screenframeshort\r\n"); p.flush()
buf = b""
t0 = time.time()
while time.time() - t0 < 8:
    d = p.read(8192)
    if d:
        buf += d
        if b"ok\r\n" in buf[-8:]:
            break
p.close()

# split into lines; rows are exactly W chars (after the echo line)
lines = buf.split(b"\n")
rows = []
for ln in lines:
    s = ln.rstrip(b"\r")
    if len(s) == W:
        rows.append(s)
print(f"got {len(buf)} bytes, {len(rows)} full rows (need {H})")
rows = rows[:H]

img = np.zeros((len(rows), W, 3), dtype=np.uint8)
for y, s in enumerate(rows):
    for x in range(W):
        v = s[x] - 32
        if v < 0 or v > 63:
            v = 0
        img[y, x] = [((v >> 4) & 3) * 85, ((v >> 2) & 3) * 85, (v & 3) * 85]

plt.figure(figsize=(4, 5.3))
plt.imshow(img); plt.axis("off"); plt.tight_layout(pad=0)
plt.savefig(out, dpi=120, bbox_inches="tight")
print(f"saved {out}")
