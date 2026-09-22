#!/usr/bin/env python3

#
# Copyright (C) 2026 Mayhem firmware contributors
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

# Reading what the linker knows out of an .elf.
#
# The build needs to find the words in an image that are 32 bit addresses. In a
# flat binary a pointer and a pair of Thumb instructions can hold the same
# value: "movs r5, #0" followed by "add r6, sp, #8" reads back as 0xae022500.
# Scanning for the byte pattern therefore reports references that do not exist
# and rewrites instructions that look like a pointer. The linker keeps the
# relocation entries when the application is linked with --emit-relocs, which
# says exactly which words are addresses.

import re
import subprocess

import collections

Relocation = collections.namedtuple(
    "Relocation", ["section", "offset", "type", "symbol_value", "symbol_name"])

external_app_section_prefix = ".external_app_"


def _readelf(readelf, args, elf_path):
    return subprocess.run([readelf] + args + [elf_path],
                          stdout=subprocess.PIPE,
                          universal_newlines=True).stdout


def read_section_addresses(readelf, elf_path):
    """{section name: virtual address} for every section in the .elf."""
    addresses = {}
    for line in _readelf(readelf, ["-S", "-W"], elf_path).splitlines():
        match = re.match(r"\s*\[\s*\d+\]\s+(\.\S+)\s+\S+\s+([0-9a-fA-F]+)", line)
        if match:
            addresses[match.group(1)] = int(match.group(2), 16)
    return addresses


def read_loaded_sections(readelf, elf_path):
    """Names of the sections that a program header actually loads.

    This is what ends up in the flashed image. The DWARF sections are not in
    any segment, and they do legitimately hold external app addresses because
    they describe those sections, so they must not be mistaken for references
    the firmware would follow at run time.
    """
    sections = set()
    in_mapping = False
    for line in _readelf(readelf, ["-l", "-W"], elf_path).splitlines():
        if line.strip().startswith("Section to Segment mapping"):
            in_mapping = True
            continue
        if not in_mapping:
            continue
        fields = line.split()
        if len(fields) < 2 or not fields[0].isdigit():
            continue
        sections.update(fields[1:])
    return sections


def read_relocations(readelf, elf_path):
    """Every relocation in the .elf, grouped by the section it applies to.

    Returns {section name: [Relocation]}. `offset` is the virtual address of
    the word being relocated and `symbol_value` the address of the symbol it
    refers to, both as the linker resolved them.
    """
    relocations = {}
    section = None
    for line in _readelf(readelf, ["-r", "-W"], elf_path).splitlines():
        if line.startswith("Relocation section"):
            match = re.match(r"Relocation section '\.rela?(\.\S+?)'", line)
            section = match.group(1) if match else None
            if section is not None:
                relocations.setdefault(section, [])
            continue
        if section is None:
            continue
        fields = line.split()
        if len(fields) < 4 or not fields[2].startswith("R_ARM"):
            continue
        relocations[section].append(Relocation(
            section=section,
            offset=int(fields[0], 16),
            type=fields[2],
            symbol_value=int(fields[3], 16),
            symbol_name=fields[4] if len(fields) > 4 else ""))
    return relocations
