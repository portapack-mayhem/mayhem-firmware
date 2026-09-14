#!/usr/bin/env python3
"""Read the PortaPack screen as text, using the firmware's own fonts.

Every glyph on screen comes from fixed_8x16_glyph_data (menus, labels) or
fixed_5x8_glyph_data (the chat console), so matching cells against those
tables returns the exact characters - no image guessing. Both fonts are
LSB-first: bit 0 is the leftmost pixel.

  ocr.py shot.png            8x16 grid (default)
  ocr.py shot.png 5x8        5x8 grid, for console text
  ocr.py shot.png both       both, 8x16 first
"""
import sys, re
import os
from PIL import Image
from collections import Counter

BASE = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                    "..", "..", "firmware", "application", "ui") + os.sep
W, H = 240, 320
FONTS = {"8x16": (BASE + "ui_font_fixed_8x16.cpp", "fixed_8x16_glyph_data", 8, 16),
         "5x8":  (BASE + "ui_font_fixed_5x8.cpp",  "fixed_5x8_glyph_data",  5, 8)}

def load_font(name):
    path, sym, fw, fh = FONTS[name]
    body = open(path, encoding="latin-1").read().split(sym + "[] = {", 1)[1]
    body = re.sub(r"//[^\n]*", "", body.split("\n};", 1)[0])
    vals = [int(v, 16) for v in re.findall(r"0x([0-9a-fA-F]{2})", body)]
    g = {}
    for i in range(len(vals) // fh):
        g.setdefault(tuple(vals[i * fh:(i + 1) * fh]), chr(0x20 + i))
    return g, fw, fh

def cell(px, x0, y0, fw, fh):
    """Cell bits with the cell's own dominant colour as background."""
    cols = Counter(px[x0 + dx, y0 + dy] for dy in range(fh) for dx in range(fw))
    bg = cols.most_common(1)[0][0]
    rows = []
    for dy in range(fh):
        b = 0
        for dx in range(fw):
            if px[x0 + dx, y0 + dy] != bg:
                b |= 1 << dx
        rows.append(b)
    return tuple(rows)

def read_row(px, glyphs, fw, fh, y, ox):
    line, hits = "", 0
    for x in range(ox, W - fw + 1, fw):
        bits = cell(px, x, y, fw, fh)
        ch = glyphs.get(bits)
        if ch is not None and any(bits):
            hits += 1
        line += ch if ch is not None else (" " if not any(bits) else "~")
    return line, hits

def read(path, name="8x16", y0=0, y1=H, ox=None, oy=0):
    px = Image.open(path).convert("RGB").load()
    glyphs, fw, fh = load_font(name)
    out = []
    for y in range(oy, min(y1, H) - fh + 1, fh):
        if y < y0:
            continue
        # Per-row x offset. Chat lines carrying a delivery marker are indented by the
        # width of that stripe, which is not a multiple of the glyph width, so a single
        # grid for the whole screen reads them as noise.
        cands = [ox] if ox is not None else range(fw)
        line = max((read_row(px, glyphs, fw, fh, y, o) for o in cands),
                   key=lambda r: r[1])[0]
        if line.strip():
            out.append(f"{y:3d} | {line.rstrip()}")
    return out

if __name__ == "__main__":
    path = sys.argv[1]
    which = sys.argv[2] if len(sys.argv) > 2 else "8x16"
    a = int(sys.argv[3]) if len(sys.argv) > 3 else 0
    b = int(sys.argv[4]) if len(sys.argv) > 4 else H
    oy = int(sys.argv[5]) if len(sys.argv) > 5 else 0
    for nm in (["8x16", "5x8"] if which == "both" else [which]):
        if which == "both":
            print(f"--- {nm} ---")
        print("\n".join(read(path, nm, a, b, None, oy)))
