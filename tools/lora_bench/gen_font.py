#!/usr/bin/env python3
"""Build the optional glyph table the Meshtastic chat reads from the SD card.

The PortaPack firmware font covers ASCII and Latin-1, and the SPI flash is full, so
no further alphabet can be baked in. The app reads this file instead - as a set of
codepoint ranges, not one built-in script, so which alphabet a device shows is the
choice of whoever holds the card rather than of whoever wrote the firmware.

What can and cannot be here is a property of the writing system, not a preference:
an alphabet whose letters keep one shape and run left to right needs only its
pictures, and any of them fits. Arabic picks one of four forms per letter from its
neighbours and runs right to left; the Indic scripts fuse consonants and reorder
vowels; Han needs thousands of glyphs and more than eight pixels of width. Those
need text shaping, which is a different piece of software from a glyph table.

Glyph format (must match ILI9341::draw_bitmap and the firmware's own 8x16 face):
  8x16, 16 bytes per glyph, byte[y] = row y, bit x (LSB = leftmost), 1 = ink.
  Caps sit on rows 5-13 so the table lines up with the built-in font.

File layout, little-endian, deploy to /APPS/mesh_font.fnt:
  "MGF1" | w=8 | h=16 | ranges | flags=0 | ranges*{u16 first, u8 count} | glyphs

Hebrew and the other right-to-left alphabets are deliberately absent: the chat draws
a line left to right, so their glyphs would come out in reverse reading order. Showing
a sentence backwards is worse than admitting there is no glyph for it.

  usage:  gen_font.py cyrillic            8x16, for the standard chat font
          gen_font.py cyrillic --small    5x8, for the compact one
          gen_font.py cyrillic cyrillic-ext     # + Kazakh, Tatar, Ukrainian g'
          gen_font.py --list
"""
import struct, os, sys
from PIL import Image, ImageFont, ImageDraw
import matplotlib

TTF = os.path.join(os.path.dirname(matplotlib.__file__),
                   'mpl-data', 'fonts', 'ttf', 'DejaVuSansMono.ttf')
# The card table has to match the height of the face it sits beside, so there are two
# of them: 8x16 for the standard chat font and the keyboard, 5x8 for the compact chat.
# A file carries one or the other; the app loads whichever it finds for the face in use.
FACES = {
    #        w  h  ttf size  dx  dy  threshold  output
    'big':   (8, 16, 13, -1, 1, 110, 'mesh_font.fnt'),
    'small': (5, 8, 9, -1, -2, 110, 'mesh_font5.fnt'),
}
W, H = 8, 16
SIZE, DX, DY, THR = 13, -1, 1, 110  # tuned so caps sit rows 5-13

MAX_RANGES = 16   # MeshConsole::MAX_RANGES
MAX_GLYPHS = 256  # MeshtasticChatView::MAX_EXT_GLYPHS - 4 kB of a very small heap

# (first codepoint, count) per script, and what it is for. Sized to be useful, not
# exhaustive: every glyph costs sixteen bytes of a heap the whole application shares.
# The ranges are checked against real alphabets by test_coverage() below - the first
# draft claimed Kazakh, Ukrainian, Romanian and Vietnamese it could not actually
# write, and only a test could tell.
SCRIPTS = {
    # The basic block, then the letters the other Cyrillic languages add - by the
    # letter rather than by the block. Taking U+0490..U+04E9 whole would be 90 glyphs
    # for the twenty-six that anyone here writes with; these eight short runs are 26.
    'cyrillic':     [(0x0400, 96),                  # ru, be, bg, sr, mk
                     (0x0490, 4),                   # G' G  - uk, kk
                     (0x0496, 6),                   # ZH DH Q - tt, ba, kk
                     (0x04A0, 4),                   # QQ NG - ba, kk
                     (0x04AA, 2),                   # SS    - ba
                     (0x04AE, 4),                   # U  UU - kk, tt
                     (0x04BA, 2),                   # H     - kk, tt, ba
                     (0x04D8, 2),                   # AE    - kk, tt, ba
                     (0x04E8, 2)],                  # OE    - kk, tt, ba
    'greek':        [(0x0386, 73)],                 # el
    'latin-ext':    [(0x0100, 128), (0x0218, 4)],   # pl, cs, tr, lt, lv, ro, hu
    'vietnamese':   [(0x1EA0, 90), (0x0102, 2), (0x0110, 2),   # vi
                     (0x0128, 2), (0x0168, 2), (0x01A0, 2), (0x01AF, 2)],
}

# Letters a language cannot be written without, outside what the firmware font
# already draws (ASCII and Latin-1). Kept here so a range can never quietly stop
# covering what this file claims it covers.
ALPHABETS = {
    'cyrillic': {
        'ru': 'АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯабвгдеёжзийклмнопрстуфхцчшщъыьэюя',
        'be': 'АБВГДЕЁЖЗІЙКЛМНОПРСТУЎФХЦЧШЫЬЭЮЯабвгдеёжзійклмнопрстуўфхцчшыьэюя',
        'sr': 'АБВГДЂЕЖЗИЈКЛЉМНЊОПРСТЋУФХЦЧЏШабвгдђежзијклљмнњопрстћуфхцчџш',
        'bg': 'АБВГДЕЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЬЮЯабвгдежзийклмнопрстуфхцчшщъьюя',
        'mk': 'АБВГДЃЕЖЗЅИЈКЛЉМНЊОПРСТЌУФХЦЧЏШабвгдѓежзѕијклљмнњопрстќуфхцчџш',
        'ba': 'ӘҒҘҠҢӨҪҮҺәғҙҡңөҫүһ',
        'uk': 'АБВГҐДЕЄЖЗИІЇЙКЛМНОПРСТУФХЦЧШЩЬЮЯабвгґдеєжзиіїйклмнопрстуфхцчшщьюя',
        'kk': 'ӘҒҚҢӨҰҮҺІәғқңөұүһі',
        'tt': 'ӘҖҢӨҮҺәҗңөүһ',
    },
    'greek': {
        'el': 'ΑΒΓΔΕΖΗΘΙΚΛΜΝΞΟΠΡΣΤΥΦΧΨΩαβγδεζηθικλμνξοπρστυφχψωάέήίόύώΐϊϋ',
    },
    'latin-ext': {
        'pl': 'ąćęłńóśźżĄĆĘŁŃÓŚŹŻ',
        'cs': 'áčďéěíňóřšťúůýžÁČĎÉĚÍŇÓŘŠŤÚŮÝŽ',
        'tr': 'çğıİöşüÇĞÖŞÜ',
        'lt': 'ąčęėįšųūžĄČĘĖĮŠŲŪŽ',
        'ro': 'ăâîșțĂÂÎȘȚ',
        'hu': 'áéíóöőúüűÁÉÍÓÖŐÚÜŰ',
    },
    'vietnamese': {
        'vi': 'ăâđêôơưĂÂĐÊÔƠƯáàảãạấầẩẫậắằẳẵặéèẻẽẹếềểễệíìỉĩị'
              'óòỏõọốồổỗộớờởỡợúùủũụứừửữựýỳỷỹỵ',
    },
}


def test_coverage():
    """Every language named above must be writable with the scripts it is listed under."""
    bad = 0
    for names, langs in ALPHABETS.items():
        ranges = [r for n in names.split() for r in SCRIPTS[n]]
        for lang, letters in langs.items():
            miss = sorted({c for c in letters
                           if not (ord(c) < 0x80 or 0xA0 <= ord(c) <= 0xFF or
                                   any(f <= ord(c) < f + n for f, n in ranges))})
            if miss:
                bad += 1
                print(f"  {lang} ({names}): missing " +
                      ' '.join(f"{c} U+{ord(c):04X}" for c in miss), file=sys.stderr)
    return bad


def render(cp, font, w=None, h=None, dx=None, dy=None, thr=None):
    w = W if w is None else w
    h = H if h is None else h
    dx = DX if dx is None else dx
    dy = DY if dy is None else dy
    thr = THR if thr is None else thr
    img = Image.new('L', (w, h), 0)
    ImageDraw.Draw(img).text((dx, dy), chr(cp), fill=255, font=font)
    px = img.load()
    # Row-major, bit x is the leftmost pixel, exactly as ILI9341::draw_bitmap reads it.
    bits = ''.join('1' if px[x, y] > thr else '0' for y in range(h) for x in range(w))
    out = bytearray((len(bits) + 7) // 8)
    for i, b in enumerate(bits):
        if b == '1':
            out[i // 8] |= 1 << (i % 8)
    return bytes(out)


def preview(glyphs, cps, text, w=None, h=None):
    w = W if w is None else w
    h = H if h is None else h
    idx = {cp: i for i, cp in enumerate(cps)}
    rows = [g for c in text if (g := glyphs[idx[ord(c)]] if ord(c) in idx else None)]
    if not rows:
        return

    def bit(g, x, y):
        i = y * w + x
        return (g[i // 8] >> (i % 8)) & 1

    print(f"=== preview: {text} ===")
    for y in range(h):
        print(''.join('#' if bit(g, x, y) else '.' for g in rows for x in range(w)))


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('-')]
    if '--list' in sys.argv or not args:
        print("scripts:")
        for name, rs in SCRIPTS.items():
            n = sum(c for _, c in rs)
            print(f"  {name:<13} {n:>3} glyphs, {n * 16:>5} B of RAM")
        print("\nlanguages (what each one needs, all of it):")
        for names, langs in ALPHABETS.items():
            print(f"  {' '.join(sorted(langs)):<24} {names}")
        print(f"\nat most {MAX_RANGES} ranges and {MAX_GLYPHS} glyphs in one file")
        return 1 if test_coverage() else 0

    unknown = [a for a in args if a not in SCRIPTS]
    if unknown:
        print(f"unknown: {', '.join(unknown)} (try --list)", file=sys.stderr)
        return 1

    ranges = [r for a in args for r in SCRIPTS[a]]
    total = sum(c for _, c in ranges)
    if len(ranges) > MAX_RANGES:
        print(f"{len(ranges)} ranges, the app reads {MAX_RANGES}", file=sys.stderr)
        return 1
    if total > MAX_GLYPHS:
        print(f"{total} glyphs, the app takes {MAX_GLYPHS}", file=sys.stderr)
        return 1

    if test_coverage():
        print("the ranges do not cover what they claim", file=sys.stderr)
        return 1

    face = 'small' if '--small' in sys.argv else 'big'
    fw, fh, fsize, fdx, fdy, fthr, fname = FACES[face]
    font = ImageFont.truetype(TTF, fsize)
    cps = [first + i for first, count in ranges for i in range(count)]
    glyphs = [render(cp, font, fw, fh, fdx, fdy, fthr) for cp in cps]

    out = os.path.join(os.path.dirname(os.path.abspath(__file__)), fname)
    with open(out, 'wb') as f:
        f.write(b'MGF1' + struct.pack('BBBB', fw, fh, len(ranges), 0))
        for first, count in ranges:
            f.write(struct.pack('<HB', first, count))
        for g in glyphs:
            f.write(g)
    size = os.path.getsize(out)
    print(f"wrote {out}: {', '.join(args)} - "
          f"{len(glyphs)} glyphs in {len(ranges)} range(s), {size} bytes")

    if 'cyrillic' in args:
        preview(glyphs, cps, "Привет" if face == "big" else "Щюжэя", fw, fh)
    if 'greek' in args:
        preview(glyphs, cps, "Γειά", fw, fh)

    # Read it back the way the firmware does, so a broken writer is caught here and
    # not by a chat that silently shows placeholders.
    with open(out, 'rb') as f:
        d = f.read()
    assert d[:4] == b'MGF1' and d[4] == fw and d[5] == fh and d[6] == len(ranges)
    stride = (fw * fh + 7) // 8
    off = 8 + len(ranges) * 3
    seen = 0
    for i, (first, count) in enumerate(ranges):
        rf, rc = struct.unpack_from('<HB', d, 8 + i * 3)
        assert (rf, rc) == (first, count), f"range {i} mismatch"
        seen += rc
    assert seen == len(glyphs) and off + seen * stride == len(d), "size mismatch"
    for i, g in enumerate(glyphs):
        assert d[off + i * stride: off + (i + 1) * stride] == g, f"glyph {i} mismatch"
    print("round-trip OK")
    return 0


if __name__ == '__main__':
    sys.exit(main())
