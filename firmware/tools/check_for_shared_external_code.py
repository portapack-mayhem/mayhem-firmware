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
from external_app_info import maximum_application_size
from external_app_info import external_apps_address_start
from external_app_info import external_apps_address_end

usage_message = """
PortaPack ROM image checker for possible shared external code addresses

Usage: <command> <input-file>
"""

def read_image(path):
	f = open(path, 'rb')
	data = f.read()
	f.close()
	return data

if len(sys.argv) != 2:
	print(usage_message)
	sys.exit(-1)

image = read_image(sys.argv[1])
image = bytearray(image)

for i in range(0, len(image), 4):
	snippet = image[i:i+4]
	val = int.from_bytes(snippet, byteorder='little')
	offset = val & 0xFFFF
	if (val >= external_apps_address_start) and (val < external_apps_address_end) and ((val & 0xFFFF) < maximum_application_size) and ((val & 0x3)==0):
		print ("External code address", hex(val),"at offset", hex(i),"in", sys.argv[1])
