#!/usr/bin/env python

# Copyright (C) 2016 Furrtek
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

import sys
import struct
from PIL import Image

usage_message = """
1BPP PortaPack bitmap generator

Usage: <command> <name> <filename>
"""

if len(sys.argv) < 3:
	print(usage_message)
	sys.exit(-1)

c = 0
data = 0

im = Image.open(sys.argv[2])
rgb_im = im.convert('RGBA')

if rgb_im.size[0] % 8 or rgb_im.size[1] % 8:
	print('Bitmap size isn\'t a multiple of 8')
	sys.exit(-1)

name = sys.argv[1]

print "static constexpr uint8_t bitmap_" + name + "_data[] = {"
print '	',		# Tab

for i in range(rgb_im.size[0]):
	for j in range(rgb_im.size[1]):
		r, g, b, a = rgb_im.getpixel((j, i))
		
		data >>= 1
		
		if r > 127 and g > 127 and b > 127 and a > 127:
			data += 128
		
		if j % 8 == 7:
			print "0x%0.2X," % data,
			data = 0

	print ''
	if i < rgb_im.size[0] - 1:
		print '	',		# Tab

print "};"
print ''
print "static constexpr Bitmap bitmap_"  + name + " {"
print "	{ " + str(rgb_im.size[0]) + ", " + str(rgb_im.size[1]) + " }, bitmap_" + name+ "_data"
print "};"
