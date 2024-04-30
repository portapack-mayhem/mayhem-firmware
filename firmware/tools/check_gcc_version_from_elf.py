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
