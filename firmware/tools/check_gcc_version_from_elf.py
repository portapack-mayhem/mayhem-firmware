#
# Created by zxkmm on ArchHaseeHome - 2024/04/22
#

import subprocess
import sys

if_succeed = False


def get_gcc_version(elf_file):
    global if_succeed

    output = subprocess.check_output(['readelf', '-p', '.comment', elf_file])
    output = output.decode('utf-8')
    lines = output.split('\n')

    for line in lines:
        if 'GCC:' in line:
            version_info = line.split('GCC:')[1].strip()
            if_succeed = True
            return version_info

    if not if_succeed:  # didn't use try except here cuz don't need to break compile if this is bad result anyway
        return None


if __name__ == "__main__":
    if len(sys.argv) != 2:
        current_dir = subprocess.check_output(['pwd'])
        print(f"current dir: {current_dir}")
        print("usage python check_gcc_version_from_elf.py xxx.elf")
        sys.exit(1)

    elf_file = sys.argv[1]
    version_info = get_gcc_version(elf_file)

if version_info is not None:
    print(f"real gcc version readed from elf: {version_info}")
else:
    print("something went wrong when checking gcc version don't worry tho, it's not deadly issue. skip.")
