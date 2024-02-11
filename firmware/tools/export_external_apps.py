#!/usr/bin/env python3

#
# Copyright (C) 2023 Bernd Herzog
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

import os
import sys
import struct
import subprocess
from external_app_info import maximum_application_size
from external_app_info import external_apps_address_start
from external_app_info import external_apps_address_end

usage_message = """
PortaPack external app image creator
This script is used in the build process and should never be run manually.
See firmware/application/CMakeLists.txt > COMMAND ${EXPORT_EXTERNAL_APP_IMAGES}

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

def patch_image(path, image_data, search_address, replace_address):
	if (len(image_data) % 4) != 0:
		print("file size not divideable by 4")
		sys.exit(-1)

	external_application_image = bytearray()

	for x in range(int(len(image_data)/4)):
		snippet = image_data[x*4:4*(x+1)]
		val = int.from_bytes(snippet, byteorder='little')

		# in firmware/application/external/external.ld the origin is set to something like search_address=0xADB00000
		# if the value is above the search_address and inside a 32kb window (maximum size of an app) we replace it
		# with replace_address=(0x1008000 + m4 size) where the app will actually be located. The reason we do this instead just
		# using the right address in external.ld is gcc does not permit to use the same memory range multiple times.
		if val > search_address and (val - search_address) < maximum_application_size:
			relative_address = val - search_address
			new_address = replace_address + relative_address

			new_snippet = new_address.to_bytes(4, byteorder='little')
			external_application_image += new_snippet
		else:
			external_application_image += snippet
			if (val >= external_apps_address_start) and (val < external_apps_address_end) and ((val & 0xFFFF) < maximum_application_size) and ((val & 0x3)==0):
				print ("WARNING: External code address", hex(val), "at offset", hex(x*4), "in", path)

	return external_application_image

project_source_dir = sys.argv[1]   #/portapack-mayhem/firmware/application
binary_dir = sys.argv[2]           #/portapack-mayhem/build/firmware/application
cmake_objcopy = sys.argv[3]

memory_location_header_position = 0
externalAppEntry_header_position = 4
m4_app_tag_header_position = 72
m4_app_offset_header_position = 76

for external_image_prefix in sys.argv[4:]:

	# COMMAND ${CMAKE_OBJCOPY} -v -O binary ${PROJECT_NAME}.elf ${PROJECT_NAME}_ext_pacman.bin --only-section=.external_app_pacman
	himg = "{}/external_app_{}.himg".format(binary_dir, external_image_prefix)
	subprocess.run([cmake_objcopy, "-v", "-O", "binary", "{}/application.elf".format(binary_dir), himg, "--only-section=.external_app_{}".format(external_image_prefix)])

	external_application_image = read_image(himg)

	#m4 image @ 0x44
	chunk_data = external_application_image[m4_app_tag_header_position:m4_app_tag_header_position+4]

	# skip m4 if not set
	if (chunk_data[0] == 0 and chunk_data[1] == 0 and chunk_data[2] == 0 and chunk_data[3] == 0):
		replace_address = 0x10080000
		search_address = int.from_bytes(external_application_image[externalAppEntry_header_position:externalAppEntry_header_position+4], byteorder='little') & 0xFFFF0000
		external_application_image = patch_image(himg, external_application_image, search_address, replace_address)
		external_application_image[memory_location_header_position:memory_location_header_position+4] = replace_address.to_bytes(4, byteorder='little')

		checksum = 0
		for i in range(0, len(external_application_image), 4):
			checksum += external_application_image[i] + (external_application_image[i + 1] << 8) + (external_application_image[i + 2] << 16) + (external_application_image[i + 3] << 24)

		final_checksum = 0
		checksum = (final_checksum - checksum) & 0xFFFFFFFF
		external_application_image += checksum.to_bytes(4, 'little')

		write_image(external_application_image, "{}/{}.ppma".format(binary_dir, external_image_prefix))
		continue

	chunk_tag = chunk_data.decode("utf-8")
	m4_image = read_image("{}/../baseband/{}.bin".format(binary_dir, chunk_tag))
	app_image_len = len(external_application_image)
	external_application_image += m4_image

	if (len(m4_image) % 4) != 0:
		print("m4 file size not divideable by 4")
		sys.exit(-1)

	replace_address = 0x10080000 + len(m4_image)
	search_address = int.from_bytes(external_application_image[externalAppEntry_header_position:externalAppEntry_header_position+4], byteorder='little') & 0xFFFF0000
	external_application_image = patch_image(himg, external_application_image, search_address, replace_address)

	external_application_image[memory_location_header_position:memory_location_header_position+4] = replace_address.to_bytes(4, byteorder='little')
	external_application_image[m4_app_offset_header_position:m4_app_offset_header_position+4] = app_image_len.to_bytes(4, byteorder='little')

	if (len(external_application_image) > maximum_application_size) != 0:
		print("application {} can not exceed 32kb: {} bytes used".format(external_image_prefix, len(external_application_image)))
		sys.exit(-1)

	checksum = 0
	for i in range(0, len(external_application_image), 4):
		checksum += external_application_image[i] + (external_application_image[i + 1] << 8) + (external_application_image[i + 2] << 16) + (external_application_image[i + 3] << 24)

	final_checksum = 0
	checksum = (final_checksum - checksum) & 0xFFFFFFFF
	external_application_image += checksum.to_bytes(4, 'little')

	# write .ppma (portapack mayhem application)
	write_image(external_application_image, "{}/{}.ppma".format(binary_dir, external_image_prefix))
