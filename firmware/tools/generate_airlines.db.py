#!/usr/bin/env python

# Copyright (C) 2017 Furrtek
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

outfile = open("airlines.db", "w")

# Download airlines.txt from http://xdeco.org/?page_id=30
lines = [line.rstrip('\n') for line in open('../../sdcard/ADSB/airlines.txt', 'r')]
n = 0

for line in lines:
	if line:
		nd = line.find('(')
		if (nd == -1):
			nd = None
		else:
			nd -= 1
		nline = line[4:7] + '\0' + line[10:nd] + '\0'
		print nline
		b = bytearray(nline)
		pad_len = 32 - len(b)
		b += "\0" * pad_len
		outfile.write(b)
