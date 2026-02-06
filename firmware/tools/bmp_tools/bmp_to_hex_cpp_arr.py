# copyleft 2024 zxkmm AKA zix aka sommermorgentraum

import os
import sys


def bmp_to_hex_cpp_arr(input_file, output_file):

    with open(input_file, 'rb') as f:
        data = f.read()

    name_without_extension = os.path.splitext(os.path.basename(input_file))[0]

    with open(output_file, 'w') as f:

        f.write(f"// converted by bmp_to_hex_cpp_arr.py at the firmware/tools dir\n")
        f.write(f"// fake transparent px should be the exact color rgb(255,0,255), then use true for the last arg "
                f"when drawing bmp with the draw func\n")
        f.write(f"\n")
        f.write(f"const unsigned char {name_without_extension}[] = {{\n")

        # idk about the clang-format rule, but the splash arr is 12 item per line. but other bmp arr is 2 item per line
        for i in range(0, len(data), 12):
            line = data[i:i + 12]
            hex_values = [f"0x{byte:02x}" for byte in line]

            f.write(f"    {', '.join(hex_values)},\n")

        # this is for remove last the extra comma
        f.seek(f.tell() - 2, os.SEEK_SET)
        f.truncate()

        f.write("};\n")



        f.write(f"unsigned int {name_without_extension}_len = {len(data)};\n")

    print(f"done, outputted, pls clang-format yourself before commit (if necessary)：{output_file}")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("usage: python bmp_to_hex_cpp_arr.py INPUTFILE OUTPUTFILE")
        sys.exit(1)

    input_file = sys.argv[1]
    output_file = sys.argv[2]

    bmp_to_hex_cpp_arr(input_file, output_file)