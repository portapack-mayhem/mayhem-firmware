# copyleft 2024 zxkmm AKA zix aka sommermorgentraum

import sys
import re


def hex_cpp_arr_to_bmp(input_file, output_file):
    with open(input_file, 'r') as f:
        content = f.read()

    # in the mayhem code some of them not const
    array_name_const = re.search(r'const unsigned char (\w+)\[\]', content)
    array_name_no_const = re.search(r'unsigned char (\w+)\[]', content)
    if array_name_const or array_name_no_const:

        if array_name_const:
            array_name = array_name_const.group(1)
        else:
            array_name = array_name_no_const.group(1)

        print(f"Array name: {array_name}")

    else:
        print("the file provided seems doesn't contains needed arr.")
        return

    hex_data = re.findall(r'0x[0-9A-Fa-f]{2}', content)

    if not hex_data:
        print("Error: No hex data found.")
        return

    binary_data = bytes([int(x, 16) for x in hex_data])

    with open(output_file, 'wb') as f:
        f.write(binary_data)

    print(f"Done. Output file: {output_file}")
    print(f"Array name: {array_name}")
    print(f"Data length: {len(binary_data)} bytes")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("usage: python hex_cpp_arr_to_bmp.py INPUTFILE OUTPUTFILE")
        sys.exit(1)

    input_file = sys.argv[1]
    output_file = sys.argv[2]

    hex_cpp_arr_to_bmp(input_file, output_file)