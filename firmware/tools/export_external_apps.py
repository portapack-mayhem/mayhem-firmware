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

import sys
import subprocess
from external_app_info import maximum_application_size
from external_app_info import external_apps_address_start
from external_app_info import external_apps_address_end
from elf_info import external_app_section_prefix
from elf_info import read_relocations
from elf_info import read_section_addresses

usage_message = """
PortaPack external app image creator
This script is used in the build process and should never be run manually.
See firmware/application/CMakeLists.txt > COMMAND ${EXPORT_EXTERNAL_APP_IMAGES}

Usage: <command> <binary dir> <cmake objcopy path> <cmake readelf path> <m4 code region size> <list of external image prefixes>
"""

if len(sys.argv) < 6:
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

def patch_image(path, image_data, section_address, replace_address, reloc_addresses):
	if (len(image_data) % 4) != 0:
		#sys.exit(-1)
		print("\n External App image file:", path, ", size not divideable by 4 :", len(image_data))
		j=0
		while (len(image_data) % 4) != 0:
			image_data += b'\x00' ; j+=1
		print("file size:", len(image_data)," after padded:",j, "bytes")

	external_application_image = bytearray(image_data)
	# The whole link region belongs to this app, not just the bytes it occupies:
	# a one-past-the-end pointer to an object at the end of the section is valid
	# and still has to be relocated. Regions are 64 KiB apart in "external.ld"
	# and maximum_application_size is 32 KiB, so this window cannot reach into
	# the next app's region.
	section_end = section_address + maximum_application_size

	# The app is linked at section_address (0xADxxxxxx, picked in
	# "external.ld" so the range is unused) but actually runs at
	# replace_address inside m4_code, so every pointer into the app's own
	# section is corrected here. The link cannot use the real address because
	# gcc does not permit the same memory range for several apps.
	for address in reloc_addresses:
		offset = address - section_address
		if offset < 0 or offset + 4 > len(image_data):
			continue
		val = int.from_bytes(image_data[offset:offset+4], byteorder='little')

		if section_address <= val < section_end:
			new_address = replace_address + (val - section_address)
			external_application_image[offset:offset+4] = new_address.to_bytes(4, byteorder='little')
		elif external_apps_address_start <= val < external_apps_address_end:
			# A pointer into a different app's section. That app is not
			# loaded, so the address is not valid at run time.
			print("WARNING: External code address", hex(val), "at offset", hex(offset), "in", path)

	return external_application_image

binary_dir = sys.argv[1]           #/portapack-mayhem/build/firmware/application
cmake_objcopy = sys.argv[2]
cmake_readelf = sys.argv[3]

# Size of portapack::memory::map::m4_code, from M4_CODE_SIZE in "rules.cmake".
# An app's M0 code and the baseband image it carries both live there, so their
# total is what has to fit. 40 KiB on the LPC4320, 64 KiB on the LPC4330.
m4_code_size = int(sys.argv[4], 0)


def check_fits(prefix, app_len, m4_len, m4_tag):
	total = app_len + m4_len
	if total <= m4_code_size:
		return
	print("application {} does not fit in the {} byte m4_code region: "
	      "{} bytes of app code + {} bytes of baseband image {} = {} bytes, "
	      "{} too many".format(prefix, m4_code_size, app_len, m4_len,
	                           m4_tag if m4_tag else "(none)", total,
	                           total - m4_code_size))
	sys.exit(-1)

memory_location_header_position = 0
m4_app_tag_header_position = 76
m4_app_offset_header_position = 80

application_elf = "{}/application.elf".format(binary_dir)
section_addresses = read_section_addresses(cmake_readelf, application_elf)
relocations = read_relocations(cmake_readelf, application_elf)
abs32_offsets = {
	section: [r.offset for r in entries if r.type == "R_ARM_ABS32"]
	for section, entries in relocations.items()
	if section.startswith(external_app_section_prefix)
}

if not abs32_offsets:
	print("no .rel.external_app_* sections in {} - the application must be linked "
	      "with --emit-relocs for the app images to be relocated".format(application_elf))
	sys.exit(-1)

for external_image_prefix in sys.argv[5:]:
	section_name = external_app_section_prefix + external_image_prefix
	if section_name not in section_addresses:
		print("no {} section in {}".format(section_name, application_elf))
		sys.exit(-1)
	section_address = section_addresses[section_name]
	reloc_addresses = abs32_offsets.get(section_name, [])

	# COMMAND ${CMAKE_OBJCOPY} -v -O binary ${PROJECT_NAME}.elf ${PROJECT_NAME}_ext_pacman.bin --only-section=.external_app_pacman
	himg = "{}/external_app_{}.himg".format(binary_dir, external_image_prefix)
	print("Creating external application image for {}".format(external_image_prefix))
	subprocess.run([cmake_objcopy, "-v", "-O", "binary", "{}/application.elf".format(binary_dir), himg, "--only-section=.external_app_{}".format(external_image_prefix)])

	external_application_image = read_image(himg)

	#m4 image @ 0x44
	chunk_data = external_application_image[m4_app_tag_header_position:m4_app_tag_header_position+4]

	# skip m4 if not set
	if (chunk_data[0] == 0 and chunk_data[1] == 0 and chunk_data[2] == 0 and chunk_data[3] == 0):
		replace_address = 0x10080000
		external_application_image = patch_image(himg, external_application_image, section_address, replace_address, reloc_addresses)
		external_application_image[memory_location_header_position:memory_location_header_position+4] = replace_address.to_bytes(4, byteorder='little')

		check_fits(external_image_prefix, len(external_application_image), 0, None)

		checksum = 0
		for i in range(0, len(external_application_image), 4):
			checksum += external_application_image[i] + (external_application_image[i + 1] << 8) + (external_application_image[i + 2] << 16) + (external_application_image[i + 3] << 24)

		final_checksum = 0
		checksum = (final_checksum - checksum) & 0xFFFFFFFF
		external_application_image += checksum.to_bytes(4, 'little')

		write_image(external_application_image, "{}/{}.ppma".format(binary_dir, external_image_prefix))
		continue

	print(chunk_data)
	chunk_tag = chunk_data.decode("utf-8")
	print(chunk_tag)
	print("{}/../baseband/{}.bin".format(binary_dir, chunk_tag))
	m4_image = read_image("{}/../baseband/{}.bin".format(binary_dir, chunk_tag))
	app_image_len = len(external_application_image)
	external_application_image += m4_image

	if (len(m4_image) % 4) != 0:
		print("m4 file size not divideable by 4")
		sys.exit(-1)

	replace_address = 0x10080000 + len(m4_image)
	external_application_image = patch_image(himg, external_application_image, section_address, replace_address, reloc_addresses)

	external_application_image[memory_location_header_position:memory_location_header_position+4] = replace_address.to_bytes(4, byteorder='little')
	external_application_image[m4_app_offset_header_position:m4_app_offset_header_position+4] = app_image_len.to_bytes(4, byteorder='little')

	check_fits(external_image_prefix, app_image_len, len(m4_image), chunk_tag)

	checksum = 0
	for i in range(0, len(external_application_image), 4):
		checksum += external_application_image[i] + (external_application_image[i + 1] << 8) + (external_application_image[i + 2] << 16) + (external_application_image[i + 3] << 24)

	final_checksum = 0
	checksum = (final_checksum - checksum) & 0xFFFFFFFF
	external_application_image += checksum.to_bytes(4, 'little')

	# write .ppma (portapack mayhem application)
	write_image(external_application_image, "{}/{}.ppma".format(binary_dir, external_image_prefix))
