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

"""Estimate core flash savings from ONE build, without rebuilding per idea.

application.elf is linked with --emit-relocs, so every call (bl), every
address taken (literal pool words, vtables, std::function managers, string
literals) is still recorded as a relocation. From those this builds the
reference graph of everything in core flash (.text), rooted at the vector
table, the constructors, .data initialisers and every external app (ext apps
call into core, so core code they use is live).

Modes:

  top [-n N] [-f REGEX]
      Rank symbols by *retained* size: the bytes that would disappear from
      core flash if nothing referenced that symbol any more (its dominator
      subtree). `self` is the symbol alone. A big retained/self ratio means
      the symbol is the only door to a lot of other code.

  whatif REGEX [REGEX ...]
      Pretend every symbol whose demangled name matches any REGEX is gone
      (no caller references it any more) and report how many bytes become
      unreachable, and which. Use it to answer "what do we get if we stop
      using std::to_string" before writing the patch. It is an upper bound:
      the code you replace it with is not counted.

  why REGEX
      Show who keeps the matching symbols alive (direct referrers).

  diff OLD.elf NEW.elf
      Per-symbol size delta between two real builds, to confirm a claim.

The model is approximate: indirect calls through pointers that are computed
at run time (not loaded from a relocated word) are invisible, padding between
functions is not attributed, and LTO may inline differently once the code
changes. For a final number, build once and use `diff`.

Usage:
    size_whatif.py [--elf build/firmware/application/application.elf] top
    size_whatif.py whatif 'std::__cxx11::to_string' '__to_xstring'
    size_whatif.py why '_svfprintf_r'
    size_whatif.py diff old.elf new.elf
"""

import argparse
import bisect
import os
import re
import struct
import subprocess
import sys

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
DEFAULT_ELF = os.path.join(REPO, "build", "firmware", "application", "application.elf")

SHT_SYMTAB = 2
SHT_REL = 9
SHF_ALLOC = 0x2
STT_OBJECT = 1
STT_FUNC = 2

R_ARM_ABS32 = 2
R_ARM_REL32 = 3
R_ARM_THM_CALL = 10
R_ARM_THM_JUMP24 = 30
R_ARM_TARGET1 = 38

# Core flash sections that can hold referenced code/data, and sections whose
# references keep core code alive (roots).
CORE_SECTIONS = (".text",)
ROOT_SECTIONS = ("startup", "constructors", ".data", ".ARM.exidx")


class Elf:
    def __init__(self, path):
        with open(path, "rb") as f:
            self.raw = f.read()
        raw = self.raw
        if raw[:4] != b"\x7fELF" or raw[4] != 1 or raw[5] != 1:
            sys.exit(f"{path}: not a little-endian ELF32 file")
        (self.entry,) = struct.unpack_from("<I", raw, 0x18)
        shoff, = struct.unpack_from("<I", raw, 0x20)
        shentsize, shnum, shstrndx = struct.unpack_from("<HHH", raw, 0x2E)
        self.sections = []
        for i in range(shnum):
            name, typ, flags, addr, off, size, link, info, _, entsize = struct.unpack_from(
                "<IIIIIIIIII", raw, shoff + i * shentsize)
            self.sections.append(dict(name_off=name, type=typ, flags=flags, addr=addr, off=off,
                                      size=size, link=link, info=info, entsize=entsize))
        strtab = self.sections[shstrndx]
        for s in self.sections:
            s["name"] = self._cstr(strtab["off"] + s["name_off"])

    def _cstr(self, off):
        end = self.raw.index(b"\0", off)
        return self.raw[off:end].decode("utf-8", "replace")

    def section(self, name):
        for s in self.sections:
            if s["name"] == name:
                return s
        return None

    def symbols(self):
        symtab = next(s for s in self.sections if s["type"] == SHT_SYMTAB)
        strtab = self.sections[symtab["link"]]
        out = []
        for i in range(symtab["size"] // 16):
            name, value, size, info, _, shndx = struct.unpack_from("<IIIBBH", self.raw, symtab["off"] + i * 16)
            out.append((self._cstr(strtab["off"] + name), value, size, info & 0xF, shndx))
        return out

    def relocs(self, target_name):
        """Yield (offset, type) for the REL section applying to target_name."""
        target = self.section(target_name)
        if target is None:
            return
        idx = self.sections.index(target)
        for s in self.sections:
            if s["type"] != SHT_REL or s["info"] != idx:
                continue
            for i in range(s["size"] // 8):
                off, info = struct.unpack_from("<II", self.raw, s["off"] + i * 8)
                yield off, info & 0xFF

    def read32(self, sec, addr):
        return struct.unpack_from("<I", self.raw, sec["off"] + addr - sec["addr"])[0]

    def read16(self, sec, addr):
        return struct.unpack_from("<H", self.raw, sec["off"] + addr - sec["addr"])[0]


def thumb_bl_target(elf, sec, addr):
    hw1 = elf.read16(sec, addr)
    hw2 = elf.read16(sec, addr + 2)
    s = (hw1 >> 10) & 1
    imm10 = hw1 & 0x3FF
    j1 = (hw2 >> 13) & 1
    j2 = (hw2 >> 11) & 1
    imm11 = hw2 & 0x7FF
    i1 = 1 - (j1 ^ s)
    i2 = 1 - (j2 ^ s)
    imm = (s << 24) | (i1 << 23) | (i2 << 22) | (imm10 << 12) | (imm11 << 1)
    if s:
        imm -= 1 << 25
    return addr + 4 + imm


def demangle(names):
    try:
        p = subprocess.run(["arm-none-eabi-c++filt"], input="\n".join(names), capture_output=True, text=True)
        out = p.stdout.split("\n")
        if p.returncode == 0 and len(out) >= len(names):
            return out[:len(names)]
    except FileNotFoundError:
        pass
    return names


class Graph:
    def __init__(self, path):
        elf = Elf(path)
        self.elf = elf
        core = [elf.section(n) for n in CORE_SECTIONS if elf.section(n)]
        self.core_ranges = [(s["addr"], s["addr"] + s["size"]) for s in core]
        self.core_bytes = sum(s["size"] for s in core)

        # Nodes: sized FUNC/OBJECT symbols inside core flash, one per address.
        by_addr = {}
        symbols = elf.symbols()
        for name, value, size, typ, _ in symbols:
            if typ not in (STT_FUNC, STT_OBJECT) or size == 0:
                continue
            addr = value & ~1
            if not self.in_core(addr):
                continue
            cur = by_addr.get(addr)
            if cur is None or size > cur[1]:
                by_addr[addr] = [name, size]
        # Overlapping symbols (rare): keep the first, trim to the next start.
        starts = sorted(by_addr)
        self.start, self.size, raw_names = [], [], []
        last_end = 0
        for a in starts:
            name, size = by_addr[a]
            if a < last_end:
                continue
            self.start.append(a)
            self.size.append(size)
            raw_names.append(name)
            last_end = a + size

        # Edges, collected from every relocation; references into unsymbolised
        # bytes (merged string literals, constant pools of anonymous data) get
        # split into anonymous nodes at each referenced address.
        edges_raw = []  # (src_addr or None for root, dst_addr)
        for sec in elf.sections:
            if not (sec["flags"] & SHF_ALLOC) or sec["size"] == 0 or sec["type"] == 8:  # NOBITS
                continue
            is_core = sec["name"] in CORE_SECTIONS
            is_root = sec["name"] in ROOT_SECTIONS or sec["name"].startswith(".external_app")
            if not (is_core or is_root):
                continue
            for off, typ in elf.relocs(sec["name"]):
                if typ in (R_ARM_ABS32, R_ARM_TARGET1):
                    dst = elf.read32(sec, off) & ~1
                elif typ == R_ARM_REL32:
                    dst = (elf.read32(sec, off) + off) & 0xFFFFFFFE
                elif typ in (R_ARM_THM_CALL, R_ARM_THM_JUMP24):
                    dst = thumb_bl_target(elf, sec, off)
                else:
                    continue
                if not self.in_core(dst):
                    continue
                edges_raw.append((off if is_core else None, dst))
        edges_raw.append((None, elf.entry & ~1))

        # External apps reach core through linker-generated long-branch
        # veneers ("__<target>_veneer"), which carry no relocation of their
        # own. Resolve them by name; a veneer lives in an ext app section, so
        # its target is a root.
        addr_of = {name: value & ~1 for name, value, _, typ, _ in symbols if typ in (STT_FUNC, STT_OBJECT)}
        for name, value, _, _, _ in symbols:
            if name.startswith("__") and name.endswith("_veneer"):
                dst = addr_of.get(name[2:-len("_veneer")])
                if dst is not None and self.in_core(dst) and not self.in_core(value & ~1):
                    edges_raw.append((None, dst))

        anon = set()
        for _, dst in edges_raw:
            i = bisect.bisect_right(self.start, dst) - 1
            if i < 0 or dst >= self.start[i] + self.size[i]:
                anon.add(dst)
        if anon:
            raw_names = self._add_anon(sorted(anon), raw_names)
        self.names = demangle(raw_names)

        n = len(self.start)
        self.succ = [set() for _ in range(n + 1)]  # node n = virtual root
        self.root = n
        for src, dst in edges_raw:
            d = self.node_of(dst)
            if d is None:
                continue
            s = self.root if src is None else self.node_of(src)
            if s is None:
                s = self.root  # reference from unattributed padding: treat as live
            if s != d:
                self.succ[s].add(d)
        self.pred = [[] for _ in range(n + 1)]
        for s, ds in enumerate(self.succ):
            for d in ds:
                self.pred[d].append(s)

    def _add_anon(self, anon_addrs, raw_names):
        # Merge symbol starts and anonymous starts; an anonymous node runs to
        # the next start of either kind (or the end of its core section).
        syms = list(zip(self.start, self.size, raw_names))
        merged = syms + [(a, 0, None) for a in anon_addrs]
        merged.sort(key=lambda t: t[0])
        start, size, names = [], [], []
        for i, (a, sz, nm) in enumerate(merged):
            if nm is None:
                nxt = merged[i + 1][0] if i + 1 < len(merged) else self._range_end(a)
                nxt = min(nxt, self._range_end(a))
                sz = nxt - a
                nm = f"<anon data @0x{a:x}>"
                if sz <= 0:
                    continue
            start.append(a)
            size.append(sz)
            names.append(nm)
        self.start, self.size = start, size
        return names

    def _range_end(self, a):
        for lo, hi in self.core_ranges:
            if lo <= a < hi:
                return hi
        return a

    def in_core(self, a):
        for lo, hi in self.core_ranges:
            if lo <= a < hi:
                return True
        return False

    def node_of(self, addr):
        i = bisect.bisect_right(self.start, addr) - 1
        if i >= 0 and addr < self.start[i] + self.size[i]:
            return i
        return None

    def reachable(self, removed=frozenset()):
        seen = bytearray(len(self.succ))
        seen[self.root] = 1
        stack = [self.root]
        while stack:
            v = stack.pop()
            for w in self.succ[v]:
                if not seen[w] and w not in removed:
                    seen[w] = 1
                    stack.append(w)
        return seen

    def dominators(self):
        # Cooper, Harvey, Kennedy: "A Simple, Fast Dominance Algorithm".
        order, seen = [], bytearray(len(self.succ))
        stack = [(self.root, iter(self.succ[self.root]))]
        seen[self.root] = 1
        while stack:
            v, it = stack[-1]
            for w in it:
                if not seen[w]:
                    seen[w] = 1
                    stack.append((w, iter(self.succ[w])))
                    break
            else:
                stack.pop()
                order.append(v)
        rpo = order[::-1]
        index = {v: i for i, v in enumerate(rpo)}
        idom = {self.root: self.root}

        def intersect(a, b):
            while a != b:
                while index[a] > index[b]:
                    a = idom[a]
                while index[b] > index[a]:
                    b = idom[b]
            return a

        changed = True
        while changed:
            changed = False
            for v in rpo[1:]:
                new = None
                for p in self.pred[v]:
                    if p in idom:
                        new = p if new is None else intersect(p, new)
                if idom.get(v) != new:
                    idom[v] = new
                    changed = True
        return idom, rpo


def node_size(g, v):
    return 0 if v == g.root else g.size[v]


def cmd_top(g, args):
    idom, rpo = g.dominators()
    retained = {v: node_size(g, v) for v in rpo}
    for v in reversed(rpo):
        if v != g.root:
            retained[idom[v]] += retained[v]
    live = retained[g.root]
    print(f"core .text {g.core_bytes} B, live symbols {live} B, unattributed {g.core_bytes - sum(g.size)} B")
    pat = re.compile(args.filter) if args.filter else None
    rows = [(retained[v], g.size[v], g.names[v]) for v in rpo if v != g.root and (pat is None or pat.search(g.names[v]))]
    rows.sort(reverse=True)
    print(f"{'retained':>9} {'self':>7}  symbol")
    for r, s, n in rows[:args.n]:
        print(f"{r:9d} {s:7d}  {n[:args.width]}")


def matching(g, patterns):
    pats = [re.compile(p) for p in patterns]
    return {i for i, n in enumerate(g.names) if any(p.search(n) for p in pats)}


def cmd_whatif(g, args):
    removed = matching(g, args.patterns)
    if not removed:
        sys.exit("no symbol matches")
    before = g.reachable()
    after = g.reachable(frozenset(removed))
    gone = [v for v in range(len(g.start)) if before[v] and not after[v]]
    total = sum(g.size[v] for v in gone)
    print(f"{len(removed)} symbols match; {len(gone)} symbols / {total} B of core flash would become unreachable")
    print("(upper bound: replacement code not counted; LTO inlining may shift a few %)")
    for v in sorted(gone, key=lambda v: -g.size[v])[:args.n]:
        tag = "*" if v in removed else " "
        print(f"{g.size[v]:7d} {tag} {g.names[v][:args.width]}")


def cmd_why(g, args):
    hits = matching(g, args.patterns)
    for v in sorted(hits, key=lambda v: -g.size[v])[:args.n]:
        print(f"{g.size[v]:7d}  {g.names[v][:args.width]}")
        for p in sorted(g.pred[v], key=lambda p: -node_size(g, p)):
            who = "<root: vectors/ctors/.data/external app>" if p == g.root else g.names[p]
            print(f"           <- {who[:args.width - 14]}")


def sym_sizes(path):
    out = subprocess.run(["arm-none-eabi-nm", "-S", "-C", path], capture_output=True, text=True, check=True).stdout
    elf = Elf(path)
    text = elf.section(".text")
    lo, hi = text["addr"], text["addr"] + text["size"]
    sizes = {}
    for line in out.splitlines():
        parts = line.split(" ", 3)
        if len(parts) < 4 or parts[2].lower() not in ("t", "r", "w", "d"):
            continue
        if not lo <= int(parts[0], 16) < hi:
            continue
        # LTO renumbers clones and anonymous objects between builds; drop
        # those suffixes so the same function lines up in both builds.
        name = re.sub(r"\s*\[clone [^\]]*\]", "", parts[3])
        name = re.sub(r"\.(constprop|isra|part|lto_priv|cold)\.\d+", "", name)
        name = re.sub(r"(\.obj)?\.\d+$", "", name)
        sizes[name] = sizes.get(name, 0) + int(parts[1], 16)
    return sizes, text["size"]


def cmd_diff(args):
    a, ta = sym_sizes(args.old)
    b, tb = sym_sizes(args.new)
    rows = []
    for k in set(a) | set(b):
        d = b.get(k, 0) - a.get(k, 0)
        if d:
            rows.append((d, a.get(k, 0), b.get(k, 0), k))
    rows.sort()
    print(f".text {ta} -> {tb} ({tb - ta:+d} B)")
    if len(rows) > 2 * args.n:
        rows = rows[:args.n] + [None] + rows[-args.n:]
    for row in rows:
        if row is None:
            print("   ...")
            continue
        d, x, y, k = row
        print(f"{d:+7d} {x:7d} -> {y:<7d} {k[:args.width]}")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--elf", default=DEFAULT_ELF)
    ap.add_argument("-n", type=int, default=40, help="rows to print")
    ap.add_argument("-w", "--width", type=int, default=150)
    sub = ap.add_subparsers(dest="cmd", required=True)
    t = sub.add_parser("top")
    t.add_argument("-f", "--filter", help="only list symbols matching this regex")
    w = sub.add_parser("whatif")
    w.add_argument("patterns", nargs="+")
    y = sub.add_parser("why")
    y.add_argument("patterns", nargs="+")
    d = sub.add_parser("diff")
    d.add_argument("old")
    d.add_argument("new")
    args = ap.parse_args()

    if args.cmd == "diff":
        cmd_diff(args)
        return
    g = Graph(args.elf)
    {"top": cmd_top, "whatif": cmd_whatif, "why": cmd_why}[args.cmd](g, args)


if __name__ == "__main__":
    main()
