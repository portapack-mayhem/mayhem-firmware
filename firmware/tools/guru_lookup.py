#!/usr/bin/env python3

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

"""Translate a guru meditation pc/lr into a symbol, source line and disassembly.

Plain `arm-none-eabi-gdb -ex "x/3i 0xADDR" application.elf` only works for
addresses in the main M0 firmware. External apps are linked at a placeholder
0xADxxxxxx address but *run* from ~0x1008xxxx, so the guru shows a runtime
address that does not exist in the ELF. This script does the translation:

    link_addr = section_vma + (runtime_addr - memory_location)

where memory_location is the first word of the app's .ppma header.

Usage:
    guru_lookup.py 0x10085CF9 0x0FF36284
    guru_lookup.py --app waterfall_designer 0x10085CF9
    guru_lookup.py --baseband adsbrx 0x10081234
"""

import argparse
import os
import re
import struct
import subprocess
import sys

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))


def find_toolchain(explicit):
    """Locate the arm-none-eabi- prefix: --toolchain, $ARM_TOOLCHAIN, ./armbin/bin, PATH."""
    candidates = []
    if explicit:
        candidates.append(explicit)
    if os.environ.get("ARM_TOOLCHAIN"):
        candidates.append(os.environ["ARM_TOOLCHAIN"])
    candidates.append(os.path.join(REPO, "armbin", "bin", "arm-none-eabi-"))
    candidates.append("arm-none-eabi-")

    for prefix in candidates:
        try:
            subprocess.run([prefix + "addr2line", "--version"],
                           capture_output=True, check=True)
            return prefix
        except (OSError, subprocess.CalledProcessError):
            continue
    sys.exit("error: no arm-none-eabi toolchain found (try --toolchain /path/to/arm-none-eabi-)")


def run(*cmd):
    return subprocess.run(cmd, capture_output=True, text=True).stdout


def section_vmas(tc, elf):
    """section name -> (vma, size) for every .external_app_* section."""
    out = run(tc + "objdump", "-h", elf)
    vmas = {}
    for m in re.finditer(r'\.external_app_(\S+)\s+([0-9a-f]{8})\s+([0-9a-f]{8})', out):
        vmas[m.group(1)] = (int(m.group(3), 16), int(m.group(2), 16))
    return vmas


def app_load_addresses(build_dir):
    """app name -> memory_location, read from the first word of each .ppma."""
    app_dir = os.path.join(build_dir, "firmware", "application")
    loads = {}
    if not os.path.isdir(app_dir):
        return loads
    for name in os.listdir(app_dir):
        if not name.endswith(".ppma"):
            continue
        path = os.path.join(app_dir, name)
        try:
            with open(path, "rb") as f:
                (mem,) = struct.unpack("<I", f.read(4))
            loads[name[:-5]] = mem
        except (OSError, struct.error):
            pass
    return loads


def describe(tc, elf, addr, label=""):
    """Print addr2line (with inline frames) plus a short disassembly window."""
    a = "0x%08x" % addr
    src = run(tc + "addr2line", "-f", "-C", "-i", "-e", elf, a).strip()
    print("    %s%s" % (label, a))
    for i, line in enumerate(src.splitlines()):
        print("      %s %s" % ("in" if i % 2 == 0 else "  at", line))

    dis = run(tc + "objdump", "-d", "--start-address=0x%x" % (addr - 8),
              "--stop-address=0x%x" % (addr + 8), elf)
    body = [l for l in dis.splitlines() if re.match(r'\s*[0-9a-f]+:\t', l)]
    if body:
        print("      --")
        for l in body:
            here = re.match(r'\s*0*%x:' % addr, l)
            print("      %s%s" % (" >> " if here else "    ", l.strip()))


def main():
    p = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("addresses", nargs="+", help="pc / lr values from the guru screen")
    p.add_argument("--build", default=os.path.join(REPO, "build"), help="build directory")
    p.add_argument("--app", help="external app name, if auto-detection is ambiguous")
    p.add_argument("--baseband", help="resolve against firmware/baseband/baseband_<name>.elf")
    p.add_argument("--toolchain", help="arm-none-eabi- prefix")
    args = p.parse_args()

    tc = find_toolchain(args.toolchain)
    app_elf = os.path.join(args.build, "firmware", "application", "application.elf")

    if args.baseband:
        elf = os.path.join(args.build, "firmware", "baseband",
                           "baseband_%s.elf" % args.baseband)
        if not os.path.exists(elf):
            sys.exit("error: no such baseband image: " + elf)
        for a in args.addresses:
            addr = int(a, 16) & ~1
            print("\n%s  (M4 baseband: %s)" % (a, args.baseband))
            describe(tc, elf, addr)
        return

    if not os.path.exists(app_elf):
        sys.exit("error: %s not found (pass --build)" % app_elf)

    vmas = section_vmas(tc, app_elf)
    loads = app_load_addresses(args.build)

    for a in args.addresses:
        addr = int(a, 16) & ~1  # drop the Thumb bit
        print("\n=== %s ===" % a)

        # Main M0 firmware is linked at 0 (SPIFI shadow), so it resolves directly.
        if addr < 0x00100000:
            print("  main M0 firmware")
            describe(tc, app_elf, addr)
            continue

        # Otherwise it is an external app running from local SRAM. Every app has
        # its own memory_location, so several can plausibly contain the address;
        # list each candidate and let the caller pick the app they actually ran.
        names = [args.app] if args.app else sorted(loads)
        hits = 0
        for name in names:
            if name not in loads or name not in vmas:
                continue
            off = addr - loads[name]
            vma, size = vmas[name]
            if not (0 <= off < size):
                continue
            hits += 1
            print("  candidate: %s   load=0x%08X  offset=0x%X" % (name, loads[name], off))
            describe(tc, app_elf, vma + off, "link ")

        if hits == 0:
            print("  no external app contains this address.")
            print("  If the guru header said M4, re-run with --baseband <image>.")
            print("  A wild pc with a sane lr usually means an indirect call through")
            print("  a bad pointer - look up the lr instead, that is the caller.")
        elif hits > 1 and not args.app:
            print("  (%d candidates - narrow it with --app <name>)" % hits)


if __name__ == "__main__":
    main()
