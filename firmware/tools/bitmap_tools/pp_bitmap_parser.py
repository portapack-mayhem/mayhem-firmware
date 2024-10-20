#!/usr/bin/env python3

## Note: This helper was just a POC for pp_png3hpp.py to test the regex.

import argparse
import numpy as np
import re
from PIL import Image


def parse_bitmaphpp(bitmaphpp_file, icon_name):
    ico_pattern = re.compile(r"static constexpr uint8_t bitmap_(.*)_data\[\] = {\n((?:\s+(?:.*)\n)+)};\nstatic constexpr Bitmap bitmap_.*\{\n\s+\{(.*)\},", re.MULTILINE)
    ico_data = []
    
    # read file to buffer, to find multiline regex
    readfile = open(bitmaphpp_file,'r')
    buff = readfile.read()
    readfile.close()

    if icon_name == 'all':
        for match in ico_pattern.finditer(buff):
            ico_data.append([match.group(1), match.group(2), match.group(3)])
    else:
        for match in ico_pattern.finditer(buff):
            if match.group(1) in icon_name:
                ico_data.append([match.group(1), match.group(2), match.group(3)])

    return (ico_data)

def convert_hpp(icon_name,bitmap_array,iconsize_str):
    iconsize = iconsize_str.split(", ")
    bitmap_size = (int(iconsize[0]),int(iconsize[1]))
    bitmap_data=[]

    image_data = np.zeros((bitmap_size[1], bitmap_size[0]), dtype=np.uint8)

    #print(bitmap_array)
    for value in bitmap_array.split(",\n"):
        if (value):
            #print(int(value, 0))
            bitmap_data.append(int(value, 0))

    print(f"Count {len(bitmap_data)} Size: {bitmap_size[0]}x{bitmap_size[1]} ({bitmap_size[0]*bitmap_size[1]})")

    for y in range(bitmap_size[1]):
        for x in range(bitmap_size[0]):
            byte_index = (y * bitmap_size[0] + x) // 8
            bit_index = x % 8
            # bit_index = 7 - (x % 8)
            pixel_value = (bitmap_data[byte_index] >> bit_index) & 1
            image_data[y, x] = pixel_value * 255

    image = Image.fromarray(image_data, 'L')
    imagea = image.copy()
    imagea.putalpha(image)
    imagea.save(icon_name+".png")


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("hpp", help="Path for bitmap.hpp")
    parser.add_argument("--icon", help="Name of the icon from bitmap.hpp, Use 'All' for all icons in file", default = 'titlebar_image')
    
    args = parser.parse_args()

    if args.icon:
        icon_name = args.icon
    else:
        icon_name = 'titlebar_image'

    print("parse", icon_name)
    icons = parse_bitmaphpp(args.hpp, icon_name)

    for icon in icons:
        convert_hpp(icon[0],icon[1],icon[2]) 


