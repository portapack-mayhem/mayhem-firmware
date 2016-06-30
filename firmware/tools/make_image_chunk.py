#!/usr/bin/env python

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

if len(sys.argv) != 4:
	print(usage_message)
	sys.exit(-1)

input_image = read_image(sys.argv[1])
tag = sys.argv[2].encode()
output_path = sys.argv[3]

if len(tag) != 4:
	print(usage_message)
	sys.exit(-2)

input_image_max_length = 32768
if len(input_image) > input_image_max_length:
	raise RuntimeError('image size of %d exceeds device size of %d bytes' % (len(input_image), input_image_max_length))
if (len(input_image) & 3) != 0:
	raise RuntimeError('image size of %d is not multiple of four' % (len(input_image,)))

output_image = bytearray()
output_image += struct.pack('<4sI', tag, len(input_image))
output_image += input_image

write_image(output_image, output_path)
