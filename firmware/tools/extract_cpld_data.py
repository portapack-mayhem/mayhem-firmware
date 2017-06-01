#!/usr/bin/env python

#
# Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
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

# Very fragile code to extract data from Altera MAX V CPLD SVF

import sys

if len(sys.argv) != 3:
	print('Usage: <command> <Altera MAX V CPLD SVF file path> <revision name>')
	sys.exit(-1)

f = open(sys.argv[1], 'r')
revision_name = sys.argv[2]

calculate_crc = False

# !PROGRAM
# SIR 10 TDI (203);
# RUNTEST 93 TCK;
# SDR 13 TDI (0000);
# SIR 10 TDI (2F4);
# RUNTEST 93 TCK;
# while:
	# SDR 16 TDI (7FFF);
	# RUNTEST 1800 TCK;
# SIR 10 TDI (203);
# RUNTEST 93 TCK;
# SDR 13 TDI (0001);
# SIR 10 TDI (2F4);
# RUNTEST 93 TCK;

phase = None

block_0 = []
block_1 = []
current_block = None

for line in f:
	line = line.strip().upper()

	if line == '!PROGRAM':
		phase = 'block_0'
		current_block = block_0
	elif line == '!VERIFY':
		phase = 'verify'
		current_block = None

	if phase == 'block_0':
		if line == 'SDR 13 TDI (0001);':
			phase = 'block_1'
			current_block = block_1

	if phase == 'block_0' or phase == 'block_1':
		if line.startswith('SDR 16 TDI ('):
			sdr_value = int(line.split('(', 1)[1][:4], 16)
			#print('0x%04x,' % sdr_value)
			current_block.append(sdr_value)

def print_block(block):
	for n in range(0, len(block), 8):
		chunk = block[n:n+8]
		line = ['0x%04x,' % v for v in chunk]
		print('\t%s' % ' '.join(line))

def crc32(blocks):
	import zlib

	crc_bytes = []
	for block in blocks:
		for word in block:
			crc_bytes.append((word >> 0) & 0xff)
			crc_bytes.append((word >> 8) & 0xff)
	return zlib.crc32(bytearray(crc_bytes)) & 0xffffffff

print("""#include "portapack_cpld_data.hpp"

#include <cstdint>
#include <array>

namespace portapack {
namespace cpld {
namespace %s {
""" % revision_name)

print('const std::array<uint16_t, %d> block_0 { {' % len(block_0))
print_block(block_0)

print("""} };
""")

print('const std::array<uint16_t, %d> block_1 { {' % len(block_1))
print_block(block_1)

print("""} };

} /* namespace %s */
} /* namespace cpld */
} /* namespace portapack */
""" % revision_name)

if calculate_crc:
	# Apply post-programming modification to make post-programming CRC correct:
	programmed_block_0 = block_0[:]
	programmed_block_0[0] &= 0xfbff

	crc = crc32((programmed_block_0, block_1))
	print('%08x' % crc)
