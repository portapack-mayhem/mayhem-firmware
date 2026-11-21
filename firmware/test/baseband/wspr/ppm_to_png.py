#!/usr/bin/env python3
"""Convert a binary P6 PPM to PNG using only the Python standard library
(zlib + struct), so the host preview needs no image toolchain.

Usage: ppm_to_png.py input.ppm output.png
"""
import re
import struct
import sys
import zlib


def read_ppm(path):
    with open(path, "rb") as f:
        data = f.read()
    # Header: P6, whitespace-separated width/height/maxval, single whitespace,
    # then raw RGB.
    m = re.match(rb"P6\s+(\d+)\s+(\d+)\s+(\d+)\s", data)
    if not m:
        raise ValueError("not a binary P6 PPM: %s" % path)
    w, h, maxval = int(m.group(1)), int(m.group(2)), int(m.group(3))
    if maxval != 255:
        raise ValueError("unsupported maxval %d" % maxval)
    pixels = data[m.end():m.end() + w * h * 3]
    if len(pixels) != w * h * 3:
        raise ValueError("truncated PPM payload")
    return w, h, pixels


def write_png(path, w, h, rgb):
    raw = b"".join(
        b"\x00" + rgb[y * w * 3:(y + 1) * w * 3] for y in range(h))

    def chunk(tag, payload):
        c = struct.pack(">I", len(payload)) + tag + payload
        return c + struct.pack(">I", zlib.crc32(tag + payload) & 0xFFFFFFFF)

    ihdr = struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0)
    png = (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", ihdr) +
           chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b""))
    with open(path, "wb") as f:
        f.write(png)


def main():
    if len(sys.argv) != 3:
        print("usage: ppm_to_png.py input.ppm output.png", file=sys.stderr)
        sys.exit(2)
    w, h, rgb = read_ppm(sys.argv[1])
    write_png(sys.argv[2], w, h, rgb)
    print("%s: %dx%d png written" % (sys.argv[2], w, h))


if __name__ == "__main__":
    main()
