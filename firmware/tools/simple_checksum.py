#!/usr/bin/env python3

#
# Copyright (C) 2024 Mark Thompson
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
PortaPack ROM image checksum checker

Usage: <command> <input-file>
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

if len(sys.argv) != 2:
	print(usage_message)
	sys.exit(-1)

image = read_image(sys.argv[1])
image = bytearray(image)

# simple "add up the words" checksum:
checksum = 0
for i in range(0, len(image), 4):
	checksum += (image[i] + (image[i + 1] << 8) + (image[i + 2] << 16) + (image[i + 3] << 24))

checksum &= 0xFFFFFFFF
print ("Simple checksum =", checksum)
