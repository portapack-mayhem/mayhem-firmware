#!/usr/bin/env python3
"""Bring the HackRF back out of standalone mode into the PortaPack application.

The `hackrf` console command hands the radio to the host and takes the PortaPack's
own serial port away with it, so this is the way back that does not involve the
power switch.
"""
import ctypes as C, os, sys

LIB = os.environ.get("HACKRF_LIB",
                     "/Applications/SDR++.app/Contents/Frameworks/libhackrf.0.dylib")

def main():
    lib = C.CDLL(LIB)
    if lib.hackrf_init() != 0:
        print("hackrf_init failed"); return 1
    dev = C.c_void_p()
    if lib.hackrf_open(C.byref(dev)) != 0:
        print("no HackRF found (already back in the app?)"); return 1
    rc = lib.hackrf_reset(dev)
    print("hackrf_reset ->", rc)
    lib.hackrf_close(dev); lib.hackrf_exit()
    return 0 if rc == 0 else 1

if __name__ == "__main__":
    sys.exit(main())
