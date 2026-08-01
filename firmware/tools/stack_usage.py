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

"""Report per-function stack frame sizes for an ELF or object file.

The M0 process stack is 4 kB total (__process_stack_size__ in LPC43xx_M0.ld),
shared by the whole UI thread including external apps. A guru that says
"Stack Overflow" is a hard fault where get_free_stack_space() < 16, i.e. the
4 kB really was consumed - it is not a heap problem.

Watch out for anything holding a File: _FS_TINY is 0 and _MAX_SS is 512 in
ffconf.h, so every FIL carries a 512 byte sector cache and a File local costs
~560 bytes of stack. copy_file() holds two of them plus a 512 byte block
buffer, ~1.8 kB in one frame.

Thumb-1 cannot encode `sub sp, #imm` above 508 bytes, so large frames appear as
a negative constant loaded from the literal pool:

    ldr r4, [pc, #728]   ; (2e0 <...>)
    add sp, r4           ; r4 = 0xfffffcfc = -772

Naive greps for `sub sp` miss exactly the frames that matter. This resolves the
literal.

Usage:
    stack_usage.py build/firmware/application/application.elf          # top 40
    stack_usage.py <file> --min 512                                    # everything >= 512
    stack_usage.py <file> --grep copy_file                             # filter by name
    stack_usage.py <file> --chain main run_external_app copy_file      # sum a call chain
"""

import argparse
import os
import re
import subprocess
import sys

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
M0_STACK_BYTES = 4096

REG_ORDER = ['r0', 'r1', 'r2', 'r3', 'r4', 'r5', 'r6', 'r7',
             'r8', 'r9', 'sl', 'fp', 'ip', 'sp', 'lr', 'pc']


def find_toolchain(explicit):
    for prefix in (explicit, os.environ.get("ARM_TOOLCHAIN"),
                   os.path.join(REPO, "armbin", "bin", "arm-none-eabi-"),
                   "arm-none-eabi-"):
        if not prefix:
            continue
        try:
            subprocess.run([prefix + "objdump", "--version"],
                           capture_output=True, check=True)
            return prefix
        except (OSError, subprocess.CalledProcessError):
            continue
    sys.exit("error: no arm-none-eabi toolchain found (set $ARM_TOOLCHAIN)")


def push_bytes(reglist):
    n = 0
    for part in reglist.split(','):
        part = part.strip()
        if '-' in part:
            a, b = part.split('-')
            if a in REG_ORDER and b in REG_ORDER:
                n += REG_ORDER.index(b) - REG_ORDER.index(a) + 1
            else:
                n += 2
        else:
            n += 1
    return n * 4


def frame_sizes(tc, path):
    """function name (mangled) -> stack bytes reserved in its prologue."""
    out = subprocess.run([tc + "objdump", "-d", path],
                         capture_output=True, text=True).stdout

    bodies, cur = {}, None
    for line in out.splitlines():
        m = re.match(r'^[0-9a-f]+ <(.+)>:', line)
        if m:
            cur = m.group(1)
            bodies[cur] = []
        elif cur is not None:
            bodies[cur].append(line)

    sizes = {}
    for func, body in bodies.items():
        # Literal pool words, so `add sp, rN` can be resolved.
        literals = {}
        for line in body:
            m = re.match(r'\s*([0-9a-f]+):\s+([0-9a-f]{8})\s+\.word\s+0x([0-9a-f]+)', line)
            if m:
                literals[int(m.group(1), 16)] = int(m.group(3), 16)

        total, regval = 0, {}
        for line in body:
            m = re.search(r'ldr\s+(\w+), \[pc, #\d+\].*;\s*\(([0-9a-f]+)', line)
            if m:
                regval[m.group(1)] = literals.get(int(m.group(2), 16))

            m = re.search(r'\bsub(?:\.w)?\s+sp, (?:sp, )?#(\d+)', line)
            if m:
                total += int(m.group(1))

            # Thumb-1 large frame: add sp, rN where rN holds a negative literal.
            # Bound it: the same register may instead hold a data address, and
            # external app addresses (0xADxxxxxx) also look "negative" here.
            m = re.search(r'\badd\s+sp, (r\d+|sl|fp|ip)\b', line)
            if m:
                v = regval.get(m.group(1))
                if v and v > 0x80000000:
                    adjust = 0x100000000 - v
                    if adjust <= 2 * M0_STACK_BYTES:
                        total += adjust

            m = re.search(r'\bpush(?:\.w)?\s+\{(.+)\}', line)
            if m:
                total += push_bytes(m.group(1))

        sizes[func] = total
    return sizes


def demangle(tc, names):
    if not names:
        return []
    out = subprocess.run([tc + "c++filt"], input="\n".join(names),
                         capture_output=True, text=True).stdout
    return out.splitlines()


def main():
    p = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("path", help="ELF or .obj to analyse")
    p.add_argument("--min", type=int, default=0, help="only show frames >= this many bytes")
    p.add_argument("--top", type=int, default=40, help="max rows to print (0 = all)")
    p.add_argument("--grep", help="only functions whose demangled name matches this regex")
    p.add_argument("--chain", nargs="+",
                   help="sum the frames of these functions (substring match) as a call chain")
    p.add_argument("--toolchain", help="arm-none-eabi- prefix")
    args = p.parse_args()

    if not os.path.exists(args.path):
        sys.exit("error: no such file: " + args.path)

    tc = find_toolchain(args.toolchain)
    sizes = frame_sizes(tc, args.path)
    names = list(sizes)
    pretty = dict(zip(names, demangle(tc, names)))

    if args.chain:
        print("\ncall chain, M0 process stack is %d bytes\n" % M0_STACK_BYTES)
        total = 0
        for want in args.chain:
            hit = None
            for n in names:
                if want in pretty[n] or want in n:
                    if hit is None or sizes[n] > sizes[hit]:
                        hit = n
            if hit is None:
                print("  %6s  %s   (not found)" % ("?", want))
                continue
            total += sizes[hit]
            print("  %6d  %s" % (sizes[hit], pretty[hit][:96]))
        pct = 100.0 * total / M0_STACK_BYTES
        print("  " + "-" * 60)
        print("  %6d  total   (%.0f%% of the 4 kB stack, %d bytes free)"
              % (total, pct, M0_STACK_BYTES - total))
        if total > M0_STACK_BYTES:
            print("\n  OVERFLOWS - this chain cannot fit.")
        elif pct > 75:
            print("\n  Tight. Anything the callees add on top may fault.")
        return

    rows = [(v, pretty[k]) for k, v in sizes.items() if v >= args.min]
    if args.grep:
        rx = re.compile(args.grep)
        rows = [r for r in rows if rx.search(r[1])]
    rows.sort(key=lambda r: -r[0])
    if args.top:
        rows = rows[:args.top]

    print("\n%6s  %s" % ("bytes", "function"))
    for v, n in rows:
        flag = "  <-- over half the stack" if v > M0_STACK_BYTES // 2 else ""
        print("%6d  %s%s" % (v, n[:100], flag))


if __name__ == "__main__":
    main()
