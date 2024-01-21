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
