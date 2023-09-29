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

import os
import sys
import struct
import subprocess

usage_message = """
PortaPack external app image creator

Usage: <command> <project source dir> <binary dir> <cmake objcopy path> <list of external image prefixes>
"""

if len(sys.argv) < 4:
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

project_source_dir = sys.argv[1]   #/portapack-mayhem/firmware/application
binary_dir = sys.argv[2]           #/portapack-mayhem/build/firmware/application
cmake_objcopy = sys.argv[3]

for external_image_prefix in sys.argv[4:]:

	# COMMAND ${CMAKE_OBJCOPY} -v -O binary ${PROJECT_NAME}.elf ${PROJECT_NAME}_ext_pacman.bin --only-section=.external_app_pacman
	himg = "{}/external_app_{}.himg".format(binary_dir, external_image_prefix)
	subprocess.run([cmake_objcopy, "-v", "-O", "binary", "{}/application.elf".format(binary_dir), himg, "--only-section=.external_app_{}".format(external_image_prefix)]) 

	external_application_image = read_image(himg)
	replace_address = int.from_bytes(external_application_image[0:4], byteorder='little')
	search_address = int.from_bytes(external_application_image[4:8], byteorder='little') & 0xFFFF0000

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

	#set version
	patched_external_application_image[8:12] = 0x12341234.to_bytes(4, byteorder='little')

	#m4 image @ 0x44
	chunk_tag = patched_external_application_image[68:72].decode("utf-8") 
	m4_image = read_image("{}/../baseband/{}.bin".format(binary_dir, chunk_tag))
	patched_external_application_image[72:76] = len(external_application_image).to_bytes(4, byteorder='little')
	patched_external_application_image += m4_image

	# .ppma portapack mayhem application
	write_image(patched_external_application_image, "{}/{}.ppma".format(binary_dir, external_image_prefix))
