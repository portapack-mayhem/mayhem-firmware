#!/usr/bin/env python

#
# Copyright (C) 2016 Furrtek
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
import md5

usage_message = """
Baseband processors modules package file generator

Usage: <command> <module_name> <module_name>...
"""

def read_image(path):
	f = open(path, 'rb')
	data = f.read()
	f.close()
	return data

def write_file(data, path):
	f = open(path, 'wb')
	f.write(data)
	f.close()

if len(sys.argv) == 1:
	print(usage_message)
	sys.exit(-1)

data = bytearray()
h_data = bytearray()
name = bytearray()
info = bytearray()
description = bytearray()
data_default_byte = bytearray((0,))
m = md5.new()

sys.argv = sys.argv[1:]

# Format for module file:
# Magic (4), Version (2), Length (4), Name (16), MD5 (16), Description (214)
# Unpadded module binary...

for args in sys.argv:
	data = read_image(args + '/build/' + args + '.bin')
	info = 'PPM '
	info += struct.pack('H', 1)
	info += struct.pack('I', len(data))
	name = read_image(args + '/name')
	if len(name) > 16:
		name = name[0:15]
	pad_size = 16 - len(name)
	name += (data_default_byte * pad_size)
	info += name
	m.update(data)
	digest = m.digest()
	pad_size = 16 - len(digest)
	digest += (data_default_byte * pad_size)
	info += digest
	description = read_image(args + '/description')
	if len(description) > 214:
		description = description[0:213]
	pad_size = 214 - len(description)
	description += (data_default_byte * pad_size)
	info += description
	data = info + data
	write_file(data, args + '.bin')
	md5sum = ''
	for byte in digest:
		md5sum += '0x' + format(byte, '02x') + ','
	h_data += 'const char md5_' + args.replace('-','_') + '[16] = {' + md5sum + '};\n'

write_file(h_data, 'common/modules.h')
