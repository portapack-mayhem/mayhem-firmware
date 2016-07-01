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
PortaPack DFU header stripper

Usage: <command> <dfu_file> <output_file>
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

if len(sys.argv) != 3:
	print(usage_message)
	sys.exit(-1)

dfu_image = read_image_from_dfu(sys.argv[1])
output_path = sys.argv[2]

write_image(dfu_image, output_path)
