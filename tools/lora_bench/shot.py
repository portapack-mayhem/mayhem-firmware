#!/usr/bin/env python3
"""Capture the PortaPack screen over the console into a PNG.

`screenframe` streams the 240x320 display as one hex line per row (RGB888,
three hex bytes per pixel).
"""
import sys, time, serial
from PIL import Image
from ports import portapack_port
PORT = portapack_port()
W, H = 240, 320

def capture(port=PORT, wait=6.0):
    with serial.Serial(port, 115200, timeout=0.3) as s:
        s.reset_input_buffer()
        s.write(b"screenframe\r\n"); s.flush()
        buf = b""; last = time.time()
        while time.time() - last < 1.5 and time.time() - last < wait:
            chunk = s.read(65536)
            if chunk:
                buf += chunk; last = time.time()
                if b"ch>" in buf[-8:]:
                    break
            else:
                time.sleep(0.05)
        return buf.decode("ascii", "replace")

def to_image(txt):
    img = Image.new("RGB", (W, H))
    px = img.load()
    y = 0
    for line in txt.splitlines():
        line = line.strip()
        if len(line) < W * 6 or any(c not in "0123456789abcdefABCDEF" for c in line[:32]):
            continue
        if y >= H:
            break
        for x in range(W):
            h = line[x * 6:x * 6 + 6]
            px[x, y] = (int(h[0:2], 16), int(h[2:4], 16), int(h[4:6], 16))
        y += 1
    return img, y

if __name__ == "__main__":
    out = sys.argv[1] if len(sys.argv) > 1 else "screen.png"
    txt = capture()
    img, rows = to_image(txt)
    img.save(out)
    print(f"{out}: {rows} rows")
