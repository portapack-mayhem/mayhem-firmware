#!/usr/bin/env python3

#
# Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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
from external_app_info import maximum_application_size
from external_app_info import external_apps_address_start
from external_app_info import external_apps_address_end

usage_message = """
PortaPack SPI flash image generator

Usage: <command> <application_path> <baseband_path> <output_path>
       Where paths refer to the .bin files for each component project.
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

application_image = read_image(sys.argv[1])
baseband_image = read_image(sys.argv[2])
output_path = sys.argv[3]

spi_size = 1048576

images = (
	{
		'name': 'application',
		'data': application_image,
		'size': len(application_image),
	},
	{
		'name': 'baseband',
		'data': baseband_image,
		'size': len(baseband_image),
	},
)

spi_image = bytearray()
spi_image_default_byte = bytearray((255,))

for image in images:
	if len(image['data']) > image['size']:
		raise RuntimeError('data for image "%(name)s" is longer than 0x%(size)x bytes 0x%(sz)x' % {'name':image['name'], 'size':image['size'], 'sz':len(image['data'])})
	pad_size = image['size'] - len(image['data'])
	padded_data = image['data'] + (spi_image_default_byte * pad_size)
	spi_image += padded_data

if len(spi_image) > spi_size - 4:
	raise RuntimeError('SPI flash image size of %d exceeds device size of %d bytes' % (len(spi_image) + 4, spi_size))

pad_size = spi_size - 4 - len(spi_image)
for i in range(pad_size):
	spi_image += spi_image_default_byte

# quick "add up the words" checksum, and check for possible references to code in external apps
checksum = 0
for i in range(0, len(spi_image), 4):
	snippet = spi_image[i:i+4]
	val = int.from_bytes(snippet, byteorder='little')
	checksum += val
	if (val >= external_apps_address_start) and (val < external_apps_address_end) and ((val & 0xFFFF) < maximum_application_size) and ((val & 0x3)==0):
		print ("WARNING: External code address", hex(val), "at offset", hex(i), "in", sys.argv[3])

final_checksum = 0
checksum = (final_checksum - checksum) & 0xFFFFFFFF

spi_image += checksum.to_bytes(4, 'little')

write_image(spi_image, output_path)

percent_remaining = round(1000 * pad_size / spi_size) / 10;
print ("Space remaining in flash ROM:", pad_size, "bytes (", percent_remaining, "%)")