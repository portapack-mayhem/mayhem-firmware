#!/usr/bin/env python3

#
# Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
#
# This file is part of PortaPack.
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

usage_message = """
PortaPack image chunk writer

Usage: <command> <input_binary> <four-characer tag> <output_tagged_binary>
"""

def read_image(path):
	f = open(path, 'rb')
	data = f.read()
	f.close()
	return data

def write_image(data, path):
	f = open(path, 'wb')
	f.write(data)
	f.close()

if len(sys.argv) == 4:
	input_image = read_image(sys.argv[1])
	input_image = bytearray(input_image)
	tag = tuple(map(ord, sys.argv[2]))
	output_path = sys.argv[3]

	if input_image[4] & 8 == 8:
		input_image = input_image[15:]
	else:
		input_image = input_image[7:]

	if (len(input_image) & 3) != 0:
		for i in range(4 - (len(input_image) & 3)):
			input_image.append(0)

	output_image = bytearray()
	output_image += struct.pack('<4BI', tag[0], tag[1], tag[2], tag[3], len(input_image) - 4)
	output_image += input_image
	write_image(output_image, output_path)

elif len(sys.argv) == 2:
	null_image = bytearray()
	tag = (0, 0, 0, 0)
	output_path = sys.argv[1]
	null_image += struct.pack('<4BI', tag[0], tag[1], tag[2], tag[3], 0)
	write_image(null_image, output_path)

else:
	print(usage_message)
	sys.exit(-1)

if len(tag) != 4:
	print(usage_message)
	sys.exit(-2)
