#!/usr/bin/env python3

# Copyright (C) 2026 Dmytro Onyshko
# Copyright (C) 2026 Khanfar
# Copyright (C) 2026 gullradriel, Nilorea Studio Inc.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.
#

# Makes ADSB/world_map_2048.bin, the light 2048 x 2048 world map the FT8 app draws its
# stations on. world_map.bin is 32768 px wide, so a whole-world view would read about
# 1 MB from the SD card per screen line. Same format: width and height as little
# endian uint16, then RGB565 pixels, little endian, row by row.
#
# Usage: generate_world_map_2048.bin.py [input] [output]
#   input   world_map.jpg (needs Pillow) or world_map.bin (needs nothing, but samples
#           one pixel in 16 instead of averaging), default ../../sdcard/ADSB/world_map.jpg
#   output  default ../../sdcard/ADSB/world_map_2048.bin

import struct
import sys
from array import array

SIZE = 2048

input_path = sys.argv[1] if len(sys.argv) > 1 else "../../sdcard/ADSB/world_map.jpg"
output_path = sys.argv[2] if len(sys.argv) > 2 else "../../sdcard/ADSB/world_map_2048.bin"

rows = []
if input_path.lower().endswith(".bin"):
	with open(input_path, "rb") as f:
		width, height = struct.unpack("<HH", f.read(4))
		step_x = width // SIZE
		step_y = height // SIZE
		for y in range(SIZE):
			f.seek(4 + (y * step_y) * width * 2)
			line = array("H")
			line.frombytes(f.read(width * 2))
			rows.append(line[::step_x][:SIZE])
else:
	from PIL import Image
	Image.MAX_IMAGE_PIXELS = None
	im = Image.open(input_path)
	# Let the JPEG decoder scale down while decoding, rather than holding the full
	# 32768 px image in memory.
	im.draft("RGB", (SIZE * 2, SIZE * 2))
	im = im.convert("RGB").resize((SIZE, SIZE), Image.LANCZOS)
	data = im.tobytes()
	for y in range(SIZE):
		line = array("H", bytes(SIZE * 2))
		base = y * SIZE * 3
		for x in range(SIZE):
			r, g, b = data[base + x * 3:base + x * 3 + 3]
			line[x] = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)
		rows.append(line)

with open(output_path, "wb") as out:
	out.write(struct.pack("<HH", SIZE, SIZE))
	for line in rows:
		if sys.byteorder != "little":
			line.byteswap()
		out.write(line.tobytes())

print("Wrote " + output_path)
