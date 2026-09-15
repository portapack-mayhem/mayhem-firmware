# ft8_lib

Imported from [kgoba/ft8_lib](https://github.com/kgoba/ft8_lib) by Kārlis Goba, YL3JG,
under the MIT licence in `LICENSE`. Only the decoder is taken; the encoder, the kissfft
copy by Mark Borgerding and the WSJT-X-derived `ft4_ft8_public` directory are not part of
this import, so no licence beyond MIT applies here.

The FT8 protocol itself was designed by Joe Taylor K1JT and Steve Franke K9AN and is
described in *The FT4 and FT8 Communication Protocols*, QEX July/August 2020:
https://physics.princeton.edu/pulsar/k1jt/FT4_FT8_QEX.pdf

## Imported files

    constants.c/.h   Costas array, Gray map, LDPC generator and parity matrices
    crc.c/.h         14-bit CRC over the source-encoded message
    decode.c/.h      Candidate search over the waterfall, soft-decision extraction
    ldpc.c/.h        Belief-propagation decoder for LDPC(174,91)
    message.c/.h     Payload packing and unpacking
    text.c/.h        Callsign and locator string handling
    debug.h          Logging stubs

Imported at upstream commit `9fec6ca39886edbf96f4f5e71edc76da5074e871` (2025-08-24,
"non-standard callsigns; special CQ; field type annotation"). Ten of the twelve files
above are byte-identical to that revision; the two that are not are listed below.

## Local changes

`decode.c` and `ldpc.c` move four large arrays from the stack into file statics
(`log174`, `plain174`, `tov`, `toc`, 5.3 KB together). The decoder runs on a ChibiOS
thread whose working area is 4 KB, so the upstream layout overflows it. Only one thread
ever calls the decoder, so the statics are safe here, but the library is no longer
reentrant.

`decode.c` also guards `ftx_normalize_logl` against a zero variance, which otherwise
divides by zero and feeds infinities to the LDPC decoder when every likelihood comes out
identical. That guard is a fix rather than a port change and belongs upstream.

No other file differs from upstream. `.clang-format` disables reformatting in this
directory to keep it that way, so the next import stays a file copy.

Every file in this directory comes from upstream. The PortaPack glue that drives the
decoder lives one level up in `firmware/baseband/ft8_portapack.c/.h` and is covered by the
project's GPL licence, not by the MIT licence above.
