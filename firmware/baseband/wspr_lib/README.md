# wspr_lib

WSPR protocol encoding and decoding for PortaPack Mayhem. Protocol references:

- **"The WSPR Coding Process"**, Andy Talbot (G4JNT), 2009. HTML translation:
  <https://swharden.com/software/FSKview/wspr> (original PDF:
  `wspr_coding_process.pdf` at the same site). Source of the packing formulas,
  the convolutional polynomials, the bit-reversal interleave description and
  the sync vector table.
- **ganymedeham.blogspot.com**, "Arduino WSPR symbol generator" (2015-04):
  <http://ganymedeham.blogspot.com/2015/04/arduino-wspr-symbol-generator.html>
  — an independent copy of the sync vector, used to cross-check the G4JNT
  table bit for bit (they agree on all 162 entries; the swharden rendering has
  one typo, `0.0` for `0,0`, in the last row).
- **Hackaday.io "Careless WSPR"**, log "Encoding WSPRs":
  <https://hackaday.io/project/166875-careless-wspr/log/167301-encoding-wsprs>
  — confirms the polynomial values `0xF2D05351` / `0xE4613C47`, K=32, r=1/2.
- **Wikipedia, "WSPR (amateur radio software)"** — timing, bit allocation
  (28 + 15 + 5 + 2 = 50), decode threshold, start-of-transmission rule
  (1 s into even UTC minutes).

## Files

- `wspr_constants.h/.c` — protocol constants and the 162-bit sync vector.
- `wspr_encode.h/.c` — reference encoder (pack → convolutional → interleave →
  sync merge → 162 tone indices). also used by the receiver to re-encode decoded bits for agreement checks.
- `firmware/test/baseband/wspr_lib_test.cpp` — doctest suite pinning every
  constant to its published value (sync vector, permutation examples,
  hand-computed packing values, rejection rules).

## Validation

- Sync vector: two independent published copies compared bit by bit (162
  entries, 63 ones).
- End-to-end: `wspr_encode_message("K1ABC", "FN20", 37)` synthesized into a
  12 kHz WAV (phase-continuous 4-FSK at 1500 Hz) decodes as `K1ABC FN20 37`
  with the canonical wsprd decoder, compiled and run as a black-box oracle;
  this is a synthetic protocol compatibility test, not an RF sensitivity benchmark.
- One protocol detail the documentation leaves ambiguous was pinned against
  the same oracle: the convolutional shift registers start at zero — the
  polynomial values are tap masks, not initial register states (the G4JNT
  wording "Register 0: 0xF2D05351" reads both ways; the preloaded variant
  does not decode).

## License

All files carry the standard GPL-2.0-or-later PortaPack header.
