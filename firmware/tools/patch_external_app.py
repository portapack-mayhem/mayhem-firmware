#!/usr/bin/env python3

#
# Copyright (C) 2023 Bernd Herzog
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
PortaPack External app address patcher

Usage: <command> <input bin> <output bin> <0xeee91000> <0x10088000>
       Where paths refer to the .bin files for each component project.
"""

if len(sys.argv) != 5:
	print(usage_message)
	sys.exit(-1)

def read_image(path):
	f = open(path, 'rb')
	data = f.read()
	f.close()
	return data

def write_image(data, path):
	f = open(path, 'wb')
	f.write(data)
	f.close()

external_application_image = read_image(sys.argv[1])
search_address = int(sys.argv[3], 16)
replace_address = int(sys.argv[4], 16)

data_length = len(sys.argv)

if (len(external_application_image) % 4) != 0:
	print("file size not divideable by 4")
	sys.exit(-1)

patched_external_application_image = bytearray()

for x in range(int(len(external_application_image)/4)):
    snippet = external_application_image[x*4:4*(x+1)]
    val = int.from_bytes(snippet, byteorder='little')

    if val > search_address and (val - search_address) < (40*1024):
        relative_address = val - search_address
        new_address = replace_address + relative_address

        new_snippet = new_address.to_bytes(4, byteorder='little')
        patched_external_application_image += new_snippet
    else:
        patched_external_application_image += snippet

#print(patched_external_application_image)
write_image(patched_external_application_image, sys.argv[2])


