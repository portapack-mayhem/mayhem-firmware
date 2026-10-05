#!/usr/bin/env python3
"""List every baseband image, the build tiers it is external in, and which
apps start it. For testing only, not part of the build.

Sources:
  * firmware/baseband/CMakeLists.txt: DeclareTargets(<chunk tag> <name>) lines and
    the "if(FLASH_TIER ...) set(add_to_firmware FALSE)" switches that divide the
    images into sections.
  * firmware/common/spi_image.hpp: image_tag_<x>{'P','A','D','R'} <-> chunk tag.
  * every .cpp/.hpp under firmware/ (except baseband/, chibios and the tag
    definition itself): uses of image_tag_<x> (run_image, variables, ...) and
    calls of run_prepared_image. Commented-out code is ignored.

An owner is an external app (firmware/application/external/<dir>, with its
tier.txt value) or an internal file (anywhere else).

Image availability per build tier (0 = <=1MB, 1 = <=2MB, 2 = >2MB):
  internal    in the firmware in every build
  ext 0       external in build 0 only        (baseband section "TIER 0")
  ext 0-1     external in builds 0 and 1      (baseband section "TIER 0, 1")
  ext all     external in every build         (baseband section "TIER 2")

Warnings are printed when an owner that is internal in some build uses an image
that is external in that build (the image would be missing), and for chunk tags missing from
spi_image.hpp. Images without any image_tag_ user are marked in the list; they
are normally started by an external app with run_prepared_image.

Usage: tools/list_baseband_images.py [-v]
"""
import argparse
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
FW = ROOT / "firmware"
EXT = FW / "application" / "external"
BUILDS = (0, 1, 2)


def strip_comments(text):
    text = re.sub(r'/\*.*?\*/', lambda m: "\n" * m.group(0).count("\n"), text, flags=re.S)
    return re.sub(r'//[^\n]*', '', text)


def parse_images():
    """Return list of dicts: name, chunk, section (0 = firmware, 1..3 = external sections)."""
    text = re.sub(r'(?m)^\s*#.*$', '', (FW / "baseband" / "CMakeLists.txt").read_text())
    images, section = [], 0
    for m in re.finditer(r'if\(\s*FLASH_TIER\s+(EQUAL|GREATER_EQUAL)\s+(\d)\s*\)\s*set\(\s*add_to_firmware\s+FALSE\s*\)|DeclareTargets\(\s*(\w+)\s+(\w+)\s*\)', text):
        if m.group(3):
            images.append({"chunk": m.group(3), "name": m.group(4), "section": section})
        else:
            section += 1
    return images


def parse_tags():
    text = (FW / "common" / "spi_image.hpp").read_text()
    tags = {}
    for m in re.finditer(r"image_tag_(\w+)\s*\{\s*'(.)'\s*,\s*'(.)'\s*,\s*'(.)'\s*,\s*'(.)'\s*\}", text):
        tags[m.group(1)] = "".join(m.group(2, 3, 4, 5))
    return tags


def read_tier(d):
    f = EXT / d / "tier.txt"
    return int(f.read_text().strip()) if f.exists() else 0


def owner_of(path):
    rel = path.relative_to(FW)
    try:
        d = path.relative_to(EXT).parts[0]
        return f"ext:{d}", d
    except ValueError:
        return f"int:{rel}", None


def internal_builds(ext_dir):
    """Builds in which the owner's code is part of the firmware."""
    if ext_dir is None:
        return set(BUILDS)
    t = read_tier(ext_dir)
    return set() if t <= 0 else {b for b in BUILDS if b >= t}


def scan_users(tags):
    by_sym = {}
    prepared = []
    skip = ("baseband", "chibios", "chibios-contrib", "hackrf", "tools", "test")
    for p in sorted(FW.rglob("*")):
        if p.suffix not in (".cpp", ".hpp", ".h", ".c") or p.is_dir():
            continue
        parts = p.relative_to(FW).parts
        if parts[0] in skip or p == FW / "common" / "spi_image.hpp":
            continue
        text = strip_comments(p.read_text(errors="ignore"))
        owner, ext_dir = owner_of(p)
        for m in re.finditer(r'\bimage_tag_(\w+)\b', text):
            if m.group(1) in tags:
                line = text.count("\n", 0, m.start()) + 1
                by_sym.setdefault(m.group(1), []).append((owner, ext_dir, f"{p.relative_to(FW)}:{line}"))
        for m in re.finditer(r'\brun_prepared_image\s*\(', text):
            if p.name not in ("baseband_api.cpp", "baseband_api.hpp"):
                line = text.count("\n", 0, m.start()) + 1
                prepared.append((owner, ext_dir, f"{p.relative_to(FW)}:{line}"))
    return by_sym, prepared


def section_label(images, i):
    s = images[i]["section"]
    return {0: "internal", 1: "ext 0", 2: "ext 0-1", 3: "ext all"}.get(s, f"section {s}")


def external_in(section):
    """Builds in which an image of this section is NOT in the firmware."""
    return {0: set(), 1: {0}, 2: {0, 1}, 3: {0, 1, 2}}[section]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("-v", "--verbose", action="store_true", help="show every call site")
    args = ap.parse_args()

    images = parse_images()
    tags = parse_tags()
    by_sym, prepared = scan_users(tags)
    chunk_to_sym = {c: s for s, c in tags.items()}
    warnings = []

    print(f"{len(images)} baseband images\n")
    for i, img in enumerate(images):
        sym = chunk_to_sym.get(img["chunk"])
        users = by_sym.get(sym, []) if sym else []
        ext_b = external_in(img["section"])
        print(f"{img['name']:<20} {img['chunk']}  {section_label(images, i):<9} image_tag_{sym}")
        if not sym:
            warnings.append(f"{img['name']}: chunk tag {img['chunk']} has no image_tag_ constant in spi_image.hpp")
        owners = {}
        for owner, ext_dir, loc in users:
            owners.setdefault((owner, ext_dir), []).append(loc)
        for (owner, ext_dir), locs in sorted(owners.items()):
            if ext_dir is None:
                note = "internal, all builds"
            else:
                t = read_tier(ext_dir)
                note = f"external app, tier.txt={t}" + ("" if t <= 0 else f" (internal in builds >= {t})")
            print(f"    {owner:<40} {note}")
            if args.verbose:
                for loc in locs:
                    print(f"        {loc}")
            missing = sorted(internal_builds(ext_dir) & ext_b)
            if missing:
                warnings.append(f"{img['name']} ({section_label(images, i)}) is external in build(s) {missing} "
                                f"but {owner} is internal there")
        if not users:
            print("    (no image_tag_ users, probably bundled with an external app and started by run_prepared_image)")

    print("\nrun_prepared_image callers (use the image bundled with the external app, no tag):")
    for owner, ext_dir, loc in prepared:
        t = f", tier.txt={read_tier(ext_dir)}" if ext_dir else ""
        print(f"    {loc}{t}")
        if ext_dir is None:
            warnings.append(f"{loc}: internal code uses run_prepared_image")
        elif internal_builds(ext_dir):
            warnings.append(f"{loc}: run_prepared_image only works while the app is external, but tier.txt={read_tier(ext_dir)} "
                            f"makes it internal in builds {sorted(internal_builds(ext_dir))}; use run_image(image_tag_...)")

    unknown = sorted(set(by_sym) - set(chunk_to_sym.get(img["chunk"]) for img in images))
    if unknown:
        print("\nimage tags used in code but with no baseband image in CMake:", ", ".join(unknown))

    print(f"\n{len(warnings)} warning(s)")
    for w in warnings:
        print("WARNING:", w)
    return 0


if __name__ == "__main__":
    sys.exit(main())
