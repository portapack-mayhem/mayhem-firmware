#!/usr/bin/env python3

#
# copyleft 2025 zxkmm AKA zix aka sommermorgentraum
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

import re
import sys
from pathlib import Path

def parse_memory_regions(ld_file_path):
    regions = []
    
    with open(ld_file_path, 'r') as f:
        content = f.read()
        
    memory_section = re.search(r'MEMORY\s*\{(.*?)\}', content, re.DOTALL)
    if not memory_section:
        print("ERROR: Could not find MEMORY section in the linker script.")
        return []
    
    memory_content = memory_section.group(1)
    
    pattern = r'ram_external_app_(\w+)\s*\(rwx\)\s*:\s*org\s*=\s*(0x[A-Fa-f0-9]+),\s*len\s*=\s*(\d+)k'
    matches = re.finditer(pattern, memory_content)
    
    for match in matches:
        app_name = match.group(1)
        address = int(match.group(2), 16)  # string with hex -> int
        length = int(match.group(3)) * 1024  # kb -> bytes
        
        regions.append({
            'app_name': app_name,
            'address': address,
            'length': length
        })
        
    return sorted(regions, key=lambda x: x['address'])

def validate_memory_regions(regions):
    if not regions:
        return False
    
    expected_step = 0x10000  # 64k as step (not sure why)
    expected_base = 0xADB10000 # the start (not sure why this one)
    expected_size = 32 * 1024  # 32k
    issues_found = False
    
    print("\n")
    print(f"checking {len(regions)} external apps address memory regions")
    
    if regions[0]['address'] != expected_base:
        print(f"WARNING: external app first region should start at {hex(expected_base)}, but starts at {hex(regions[0]['address'])}")
        issues_found = True
    
    for i, region in enumerate(regions):
        expected_address = expected_base + (i * expected_step)
        
        # address count
        if region['address'] != expected_address:
            print(f"WARNING: external app region '{region['app_name']}' has incorrect address")
            print(f"want: {hex(expected_address)}, Found: {hex(region['address'])}")
            issues_found = True
        
        # size
        if region['length'] != expected_size:
            print(f"WARNING: external app region '{region['app_name']}' has incorrect size")
            print(f"want: {expected_size//1024}KB, Found: {region['length']//1024}KB")
            issues_found = True
        
        # overlap
        if i < len(regions) - 1:
            next_region = regions[i + 1]
            if region['address'] + region['length'] > next_region['address']:
                print(f"WARNING: external app region '{region['app_name']}' overlapped with '{next_region['app_name']}'")
                issues_found = True
    
    return not issues_found

def main():
    if len(sys.argv) > 1:
        # arg
        input_path = Path(sys.argv[1])
        if input_path.is_dir():
            ld_file_path = input_path / "external.ld"
            if not ld_file_path.exists():
                print(f"some issue causing that we can't see external app's linker script {input_path}, pass.")
                return
        else:
            ld_file_path = input_path
    else:
        # if no arg, this assume this script is in mayhemrepo/firmware/tools
        ld_file_path = Path("..") / "application" / "external" / "external.ld"
        
    try:
        regions = parse_memory_regions(ld_file_path)
        
        if not regions:
            print("some issue causing that we can't see external app's linker script's address list, pass")

            return
        
        if validate_memory_regions(regions):
            print("external app addr seems correct, pass")
        else:
            print("\nWARNING: It seems you are having incorrect external app addresses.")
            
    except Exception as e:
        print(f"err: {e}")

if __name__ == "__main__":
    main()
