#!/usr/bin/env python3

#
# copyleft 2026 zxkmm co author with AI
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

"""Check that every external app symbol landed in its own app's memory region.

The rules in external.ld match input *section names*, so a glob like
`*(*ui*external_app*level*)` also matches

    .text._ZN2ui12external_app18waterfall_designer...12on_add_levelEv
                                                       ^^^^^ contains "level"

Section assignment is first-match-wins, so a symbol whose mangled name merely
contains another app's name gets linked into that app's region. Only one
external app is resident at a time, so calling it jumps into unmapped memory
and hard faults.

export_external_apps.py already warns about *data* words that point at another
app, but a `bl` is PC-relative: the target never appears as a literal, so that
check cannot see it. This one works on symbol addresses instead and does.

Usage:
    check_external_symbol_placement.py [build/firmware/application/application.elf]
"""

import os
import re
import subprocess
import sys

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
DEFAULT_ELF = os.path.join(REPO, "build", "firmware", "application", "application.elf")
DEFAULT_LD = os.path.join(REPO, "firmware", "application", "external", "external.ld")


def find_toolchain():
    for prefix in (os.environ.get("ARM_TOOLCHAIN"),
                   os.path.join(REPO, "armbin", "bin", "arm-none-eabi-"),
                   "arm-none-eabi-"):
        if not prefix:
            continue
        try:
            subprocess.run([prefix + "nm", "--version"], capture_output=True, check=True)
            return prefix
        except (OSError, subprocess.CalledProcessError):
            continue
    sys.exit("error: no arm-none-eabi toolchain found (set $ARM_TOOLCHAIN)")


def parse_regions(ld_path):
    with open(ld_path) as f:
        ld = f.read()
    regions = {}
    for m in re.finditer(r'ram_external_app_(\w+)\s+\(rwx\)\s*:\s*org\s*=\s*'
                         r'(0x[0-9A-Fa-f]+),\s*len\s*=\s*(\d+)k', ld):
        regions[m.group(1)] = (int(m.group(2), 16), int(m.group(3)) * 1024)
    return regions


def main():
    elf = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_ELF
    if not os.path.exists(elf):
        print("skipping symbol placement check, no ELF at %s" % elf)
        return 0

    tc = find_toolchain()
    regions = parse_regions(DEFAULT_LD)
    if not regions:
        print("skipping symbol placement check, could not parse external.ld")
        return 0

    def owner(addr):
        for name, (base, size) in regions.items():
            if base <= addr < base + size:
                return name
        return None

    nm = subprocess.run([tc + "nm", "-C", "--defined-only", elf],
                        capture_output=True, text=True).stdout

    misplaced = []
    total = 0
    for line in nm.splitlines():
        parts = line.split(" ", 2)
        if len(parts) < 3 or not re.fullmatch(r'[0-9a-f]{8}', parts[0]):
            continue
        addr, sym = int(parts[0], 16), parts[2]
        if "_veneer" in sym:
            continue
        m = re.search(r'external_app::(\w+)::', sym)
        if not m:
            continue
        ns, host = m.group(1), owner(addr)
        total += 1
        if host is None:
            continue  # inlined into main firmware, harmless
        # Namespace and region name need not be identical: ert/ert_app,
        # keeloqtx/ui_keeloqtx, secplustx/ui_secplustx. Substring either way is fine.
        if ns not in host and host not in ns:
            misplaced.append((ns, host, sym))

    print("\nchecking placement of %d external app symbols across %d regions"
          % (total, len(regions)))

    if not misplaced:
        print("all external app symbols are in their own app's region")
        return 0

    print("\nERROR: %d symbol(s) linked into the wrong app's region." % len(misplaced))
    print("These will hard fault when called - the owning app is not resident.\n")
    for ns, host, sym in misplaced:
        print("  %s  ->  landed in '%s' region" % (ns, host))
        print("      %s" % sym)
    print("\nFix: make the rule in external.ld specific to the app's own sources, e.g.")
    print("     */external/<app>/*(*ui*external_app*<app>*);")
    return 1


if __name__ == "__main__":
    sys.exit(main())
