#!/usr/bin/env python3

# Copyright (C) 2017 Furrtek
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

from __future__ import print_function
import sys
import struct
from PIL import Image

outfile = open('../../sdcard/ADSB/world_map.bin', 'wb')

# Allow for bigger images
Image.MAX_IMAGE_PIXELS = None
im = Image.open("../../sdcard/ADSB/world_map.jpg")
pix = im.load()

outfile.write(struct.pack('H', im.size[0]))
outfile.write(struct.pack('H', im.size[1]))

print("Generating: \t" + outfile.name + "\n from\t\t" + im.filename + "\n please wait...");

for y in range (0, im.size[1]):
	line = b''
	for x in range (0, im.size[0]):
		# RRRRRGGGGGGBBBBB
		pixel_lcd = (pix[x, y][0] >> 3) << 11
		pixel_lcd |= (pix[x, y][1] >> 2) << 5
		pixel_lcd |= (pix[x, y][2] >> 3)
		#         RRRGGGBB to
		# RRR00GGG000BB000
		# pixel_lcd = (pix[x, y][0] >> 5) << 5
		# pixel_lcd |= (pix[x, y][1] >> 5) << 2
		# pixel_lcd |= (pix[x, y][2] >> 6)
		line += struct.pack('H', pixel_lcd)
	outfile.write(line)
	print(str(y) + '/' + str(im.size[1]) + '\r', end="")

print("Ready.");
