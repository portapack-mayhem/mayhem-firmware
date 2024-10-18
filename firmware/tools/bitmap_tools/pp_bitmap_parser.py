#!/usr/bin/env python3

import numpy as np
import re
from PIL import Image



def parse_bitmaphpp():
    ico_pattern = re.compile(r"static constexpr uint8_t bitmap_(.*)_data\[\] = {\n((?:\s+(?:.*)\n)+)};\nstatic constexpr Bitmap bitmap_.*\{\n\s+\{(.*)\},", re.MULTILINE)
    ico_data = []
    bitmaphpp_file = '/home/lupus/work/bitmap.hpp'
    
    # read file to buffer, to find multiline regex
    readfile = open(bitmaphpp_file,'r')
    buff = readfile.read()
    readfile.close()

    for match in ico_pattern.finditer(buff):
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
    image.save(icon_name+".png")


if __name__ == "__main__":

    #ico = 'icon_clk_int'
    #ico = 'icon_dir'
    ico = 'rssipwm'

    icons = parse_bitmaphpp()
    for icon in icons:
        if icon[0] == ico:
            #print(icon[1])
            convert_hpp(icon[0],icon[1],icon[2])
