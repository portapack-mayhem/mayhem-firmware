#!/usr/bin/env python3
"""Regenerate external_tier{0,1,2}.ld from the per-app "tier.txt" markers.

Each folder in firmware/application/external/ is an external app. An optional
"tier.txt" in the folder holds a single integer (default 0 when missing):
the highest flash tier in which the app is still built as an external app.
The value -1 marks a disabled app: it is left out of every .ld file and is not
checked against ui_navigation.cpp or external.cmake.
An app with tier N is listed in external_tier0.ld .. external_tierN.ld, so
tier 0 always contains every external app.

The app's section name (app_<name>) is read from the
".external_app.app_<name>.application_information" attribute in its sources
(it can differ from the folder name, e.g. foxhunt -> foxhunt_rx).

Only the three .ld files are modified. Region layout matches the existing files:
32k regions, contiguous, 0x10000 step starting at 0xADB10000. The MEMORY
comment header is preserved. App order follows the existing MEMORY order (tier0 first); new
apps are appended alphabetically.

It also verifies firmware/application/ui_navigation.cpp: an app marked tier T
(< 2) must be #included from "external/<dir>/" inside a "#if FLASH_TIER >= T+1"
block (it is internal only from that tier up), and an app marked 2 (external in
every tier) must not be included at all. Finally every app must be enabled in external.cmake: its main.cpp in EXTCPPSRC
and its section name in EXTAPPLIST (commented-out entries don't count). Any mismatch is reported and the script
exits with status 1 (the .ld files are still written unless --check is given).

Usage: tools/update_external_tier_ld.py [--check]
"""
import argparse
import re
import sys
from pathlib import Path

EXT_DIR = Path(__file__).resolve().parent.parent / "firmware" / "application" / "external"
TIERS = (0, 1, 2)
BASE = 0xADB10000
STEP = 0x10000
LEN_K = 32

SECTION_RE = re.compile(r'section\(\s*"\.external_app\.app_(\w+)\.application_information"')
LD_REGION_RE = re.compile(r'ram_external_app_(\w+)\s*\(rwx\)')
LD_SECTION_RE = re.compile(r'\.external_app_(\w+)\s*:[^{]*\{([^}]*)\}\s*>\s*ram_external_app_(\w+)')


def ld_path(tier):
    return EXT_DIR / f"external_tier{tier}.ld"


def read_tier(app_dir):
    f = app_dir / "tier.txt"
    if not f.exists():
        return 0
    text = re.sub(r'(#|//).*', '', f.read_text()).strip()
    if not re.fullmatch(r'-?\d+', text) or int(text) not in (-1, *TIERS):
        sys.exit(f"{f}: expected a single number in {TIERS} or -1, got {text!r}")
    return int(text)


def find_section_name(app_dir):
    names = set()
    for f in sorted(app_dir.rglob("*")):
        if f.suffix in (".cpp", ".hpp", ".h", ".c"):
            names.update(SECTION_RE.findall(f.read_text(errors="ignore")))
    if len(names) != 1:
        sys.exit(f"{app_dir.name}: expected exactly one application_information section, found {sorted(names)}")
    return names.pop()


def scan_apps():
    apps = {}
    for d in sorted(p for p in EXT_DIR.iterdir() if p.is_dir()):
        if not (d / "main.cpp").exists():
            continue
        name = find_section_name(d)
        if name in apps:
            sys.exit(f"duplicate section name {name}: {apps[name]['dir']} and {d.name}")
        apps[name] = {"dir": d.name, "tier": read_tier(d)}
    return apps


def parse_existing(apps):
    """Return (order, uses_path_filter) from the current .ld files."""
    order, wildcard = [], set()
    for tier in TIERS:
        p = ld_path(tier)
        if not p.exists():
            continue
        text = p.read_text()
        for name in LD_REGION_RE.findall(text):
            if name not in order:
                order.append(name)
        for name, body, _ in LD_SECTION_RE.findall(text):
            if f"*/external/" not in body:
                wildcard.add(name)
    order = [n for n in order if n in apps and apps[n]["tier"] >= 0]
    order += sorted(n for n in apps if n not in order and apps[n]["tier"] >= 0)
    return order, wildcard


def header_of(tier):
    text = ld_path(tier).read_text()
    m = re.search(r'^MEMORY\s*\{\n(.*?)(?=^\s*ram_external_app_|^\})', text, re.S | re.M)
    if not m:
        sys.exit(f"{ld_path(tier)}: cannot find MEMORY header")
    return text[:m.start()] + "MEMORY\n{\n" + m.group(1)


def generate(tier, order, apps, wildcard):
    sel = [n for n in order if apps[n]["tier"] >= tier]
    out = [header_of(tier)]
    width = max((len(n) for n in sel), default=0)
    for i, n in enumerate(sel):
        out.append(f"    ram_external_app_{n:<{width}} (rwx) : org = 0x{BASE + i * STEP:08X}, len = {LEN_K}k\n")
    out.append("}\n\nSECTIONS\n{\n")
    blocks = []
    for n in sel:
        pat = f"*(*ui*external_app*{n}*);" if n in wildcard else f"*/external/{apps[n]['dir']}/*(*ui*external_app*{n}*);"
        blocks.append(
            f"    .external_app_{n} : ALIGN(4) SUBALIGN(4)\n    {{\n"
            f"        KEEP(*(.external_app.app_{n}.application_information));\n"
            f"        {pat}\n    }} > ram_external_app_{n}\n")
    out.append("\n".join(blocks))
    out.append("}\n")
    return "".join(out), len(sel)


NAV_FILE = EXT_DIR.parent / "ui_navigation.cpp"
IF_TIER_RE = re.compile(r'#\s*if\s+FLASH_TIER\s*>=\s*(\d+)')
INCLUDE_RE = re.compile(r'#\s*include\s+"external/([^/"]+)/')


def check_navigation(apps):
    """Return a list of error strings for includes that don't match the tier markers."""
    by_dir = {a["dir"]: (n, a["tier"]) for n, a in apps.items() if a["tier"] >= 0}
    stack, guards = [], {}
    for lineno, line in enumerate(NAV_FILE.read_text().splitlines(), 1):
        stripped = line.strip()
        if re.match(r'#\s*if', stripped):
            m = IF_TIER_RE.match(stripped)
            stack.append(int(m.group(1)) if m else 0)
        elif re.match(r'#\s*(else|elif)', stripped) and stack:
            stack[-1] = 0  # the opposite branch is not tier-guarded
        elif re.match(r'#\s*endif', stripped) and stack:
            stack.pop()
        m = INCLUDE_RE.match(stripped)
        if m:
            guards.setdefault(m.group(1), []).append((max(stack, default=0), lineno))
    errors = []
    for d, (name, tier) in sorted(by_dir.items()):
        found = guards.get(d, [])
        if tier >= 2:
            for g, ln in found:
                errors.append(f"{NAV_FILE.name}:{ln}: {d} is marked tier {tier} (external in all tiers) but is included")
            continue
        want = tier + 1
        if not found:
            errors.append(f"{NAV_FILE.name}: {d} is marked tier {tier} but has no #include \"external/{d}/...\" under #if FLASH_TIER >= {want}")
        for g, ln in found:
            if g != want:
                errors.append(f"{NAV_FILE.name}:{ln}: {d} is marked tier {tier}, include must be under '#if FLASH_TIER >= {want}' (found {g or 'no guard'})")
    return errors


def check_external_cmake(apps):
    text = re.sub(r'#[^\n]*', '', (EXT_DIR / "external.cmake").read_text())

    def block(var):
        m = re.search(r'set\(\s*' + var + r'\b(.*?)\)', text, re.S)
        if not m:
            sys.exit(f"external.cmake: cannot find set({var} ...)")
        return m.group(1)

    srcs = set(re.findall(r'external/([^/\s]+)/main\.cpp', block("EXTCPPSRC")))
    listed = set(block("EXTAPPLIST").split())
    errors = []
    for name, a in sorted(apps.items()):
        if a["tier"] < 0:
            continue
        if a["dir"] not in srcs:
            errors.append(f"external.cmake: {a['dir']}/main.cpp is not in EXTCPPSRC (or is commented out)")
        if name not in listed:
            errors.append(f"external.cmake: {name} is not in EXTAPPLIST (or is commented out)")
    return errors


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--check", action="store_true", help="don't write, exit 1 if files are out of date")
    args = ap.parse_args()

    apps = scan_apps()
    order, wildcard = parse_existing(apps)
    end = BASE + len(order) * STEP
    stale = False
    for tier in TIERS:
        text, count = generate(tier, order, apps, wildcard)
        p = ld_path(tier)
        if p.read_text() != text:
            stale = True
            if not args.check:
                p.write_text(text)
        print(f"tier {tier}: {count} apps, last region ends at 0x{BASE + count * STEP - STEP + LEN_K * 1024:08X}")
    print(f"total apps: {len(order)} enabled, {len(apps) - len(order)} disabled (limit external_apps_address_end must be >= 0x{BASE + (len(order) - 1) * STEP + LEN_K * 1024:08X})")
    errors = check_navigation(apps) + check_external_cmake(apps)
    for e in errors:
        print("ERROR:", e, file=sys.stderr)
    if args.check and stale:
        print("out of date")
    if errors or (args.check and stale):
        sys.exit(1)


if __name__ == "__main__":
    main()
