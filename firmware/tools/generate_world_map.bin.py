#!/usr/bin/env python3

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

import argparse
from pathlib import Path
import struct

import numpy as np
from PIL import Image


def convert_map(input_path, output_path, chunk_rows=64):
    """Write the dimensions and row-major RGB565 pixels, all little-endian.

    NumPy temporaries are limited to chunk_rows rows. Pillow still decodes
    the full source image, as in the original converter.
    """
    if chunk_rows <= 0:
        raise ValueError("chunk_rows must be positive")
    if Path(input_path).resolve() == Path(output_path).resolve():
        raise ValueError("Input and output must be different files")

    # The supplied world map deliberately exceeds Pillow's image-size limit.
    Image.MAX_IMAGE_PIXELS = None
    with Image.open(input_path) as im:
        width, height = im.size
        header = struct.pack('<HH', width, height)
        if im.mode not in ('RGB', 'RGBA'):
            raise ValueError("Expected an RGB or RGBA source image")
        im.load()
        print(f"image size[0]={width} size[1]={height} pixels")
        print(f"Generating: {output_path}\n from {input_path}")
        with open(output_path, 'wb') as outfile:
            outfile.write(header)
            for y in range(0, height, chunk_rows):
                end = min(y + chunk_rows, height)
                # Promote before shifting: uint8 arithmetic would lose bits.
                rgb = np.asarray(im.crop((0, y, width, end)), dtype=np.uint16)
                pixels = (rgb[:, :, 0] >> 3) << 11
                pixels |= (rgb[:, :, 1] >> 2) << 5
                pixels |= rgb[:, :, 2] >> 3
                # Explicit byte order also works on big-endian hosts.
                outfile.write(pixels.astype('<u2', copy=False).tobytes(order='C'))
                print(f"{end}/{height}\r", end='', flush=True)
        print("\nReady.")


def main():
    parser = argparse.ArgumentParser(
        description="Convert a world map to RGB565 (requires Pillow and NumPy).")
    parser.add_argument('--input', type=Path,
                        default=Path('../../sdcard/ADSB/world_map.jpg'))
    parser.add_argument('--output', type=Path,
                        default=Path('../../sdcard/ADSB/world_map.bin'),
                        help="Use a different filename to compare with an existing map")
    args = parser.parse_args()
    convert_map(args.input, args.output)


if __name__ == '__main__':
    main()
