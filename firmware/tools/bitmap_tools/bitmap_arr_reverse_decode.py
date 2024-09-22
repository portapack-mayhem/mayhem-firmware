#!/usr/bin/env python3

#
# Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
# Copyleft (ɔ) 2024 zxkmm with the GPL license
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

from PIL import Image
import numpy as np

bitmap_data = [
    0x30, 0x00,
    0x30, 0x00,
    0x30, 0x00,
    0x30, 0x00,
    0x30, 0x00,
    0x30, 0x00,
    0xFC, 0x00,
    0xCE, 0x01,
    0x86, 0x01,
    0xFE, 0x01,
    0x86, 0x31,
    0x86, 0x49,
    0xCE, 0x87,
    0xFC, 0x84,
    0xFC, 0x4B,
    0x78, 0x30,
]

bitmap_size = (16, 16)

image_data = np.zeros((bitmap_size[1], bitmap_size[0]), dtype=np.uint8)

for y in range(bitmap_size[1]):
    for x in range(bitmap_size[0]):
        byte_index = (y * bitmap_size[0] + x) // 8
        bit_index = x % 8
        # bit_index = 7 - (x % 8)
        pixel_value = (bitmap_data[byte_index] >> bit_index) & 1
        image_data[y, x] = pixel_value * 255

image = Image.fromarray(image_data, 'L')
image.save('decoded.png')
