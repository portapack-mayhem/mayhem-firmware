#!/usr/bin/env python

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

# From http://mmi-comm.tripod.com/dcs.html

import sys
import struct

cb = [0,0,0,0,0,0,0,0,0]
for c in range (0, 0x200):		# DCS code is 9 bits
	for bit in range(0, 9):
		cb[bit] = (c >> bit) & 1
	
	p1 = cb[0] ^ cb[1] ^ cb[2] ^ cb[3] ^ cb[4] ^ cb[7];
	p2 = cb[1] ^ cb[2] ^ cb[3] ^ cb[4] ^ cb[5] ^ cb[8] ^ 1;
	p3 = cb[0] ^ cb[1] ^ cb[5] ^ cb[6] ^ cb[7];
	p4 = cb[1] ^ cb[2] ^ cb[6] ^ cb[7] ^ cb[8] ^ 1;
	p5 = cb[0] ^ cb[1] ^ cb[4] ^ cb[8] ^ 1;
	p6 = cb[0] ^ cb[3] ^ cb[4] ^ cb[5] ^ cb[7] ^ 1;
	p7 = cb[0] ^ cb[2] ^ cb[3] ^ cb[5] ^ cb[6] ^ cb[7] ^ cb[8];
	p8 = cb[1] ^ cb[3] ^ cb[4] ^ cb[6] ^ cb[7] ^ cb[8];
	p9 = cb[2] ^ cb[4] ^ cb[5] ^ cb[7] ^ cb[8];
	p10 = cb[3] ^ cb[5] ^ cb[6] ^ cb[8] ^ 1;
	p11 = cb[0] ^ cb[1] ^ cb[2] ^ cb[3] ^ cb[6] ^ 1;
	
	print '	0b{0:011b}'.format((p11<<10) | (p10<<9) | (p9<<8) |(p8<<7) | (p7<<6) | (p6<<5) | (p5<<4) | (p4<<3) | (p3<<2) | (p2<<1) | (p1)) + ', 	// ' + str(c);
