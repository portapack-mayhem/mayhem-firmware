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

usage_message = """
PortaPack SPI flash image generator

Usage: <command> <bootstrap_path> <hackrf_path> <baseband_path> <application_path> <output_path>
       Where paths refer to the .bin files for each component project.
"""

def read_image(path):
	f = open(path, 'rb')
	data = f.read()
	f.close()
	return data

def read_image_from_dfu(path):
	data = read_image(path)
	# Strip DFU header from file to get binary image.
	return data[16:]

def write_image(data, path):
	f = open(path, 'wb')
	f.write(data)
	f.close()

if len(sys.argv) != 6:
	print(usage_message)
	sys.exit(-1)

bootstrap_image = read_image(sys.argv[1])
hackrf_image = read_image_from_dfu(sys.argv[2])
baseband_image = read_image(sys.argv[3])
application_image = read_image(sys.argv[4])
output_path = sys.argv[5]

spi_size = 1048576

images = (
	{
		'name': 'bootstrap',
		'data': bootstrap_image,
		'size': 0x10000,
	},
	{
		'name': 'hackrf',
		'data': hackrf_image,
		'size': 0x10000,
	},
	{
		'name': 'baseband',
		'data': baseband_image,
		'size': 0x20000,
	},
	{
		'name': 'application',
		'data': application_image,
		'size': len(application_image),
	}
)

spi_image = bytearray()
spi_image_default_byte = bytearray((255,))

for image in images:
	if len(image['data']) > image['size']:
		raise RuntimeError('data for image "%(name)s" is longer than 0x%(size)x bytes' % image)
	pad_size = image['size'] - len(image['data'])
	padded_data = image['data'] + (spi_image_default_byte * pad_size)
	spi_image += padded_data

if len(spi_image) > spi_size:
	raise RuntimeError('SPI flash image size of %d exceeds device size of %d bytes' % (len(spi_image), spi_size))

write_image(spi_image, output_path)
