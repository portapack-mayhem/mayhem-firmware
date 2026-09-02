#!/usr/bin/env python3
"""Scriptable HackRF capture/replay via ctypes over the libhackrf bundled with SDR++.

No brew / build needed. Reuses /Applications/SDR++.app/Contents/Frameworks/libhackrf.0.dylib
(and the libusb-1.0.0.dylib next to it). The HackRF/PortaPack must be in *HackRF mode*
(stock HackRF, USB 1d50:6089) — not PortaPack Mayhem.

Usage:
  python3 hackrf_io.py info
  python3 hackrf_io.py rx  -f 868900000 -s 2000000 -n 4 -o captures/cap.cs8   # record N seconds, int8 IQ
  python3 hackrf_io.py tx  -f 868900000 -s 2000000 -i captures/our.cs8        # transmit int8 IQ file

IQ file format: interleaved signed 8-bit I,Q (cs8) — same as `hackrf_transfer`.
"""
import ctypes as C
import os
import sys
import time
import argparse

FRAMEWORKS = "/Applications/SDR++.app/Contents/Frameworks"
LIBHACKRF = os.path.join(FRAMEWORKS, "libhackrf.0.dylib")
LIBUSB = os.path.join(FRAMEWORKS, "libusb-1.0.0.dylib")


def _load():
    # Preload libusb by full path so libhackrf's @rpath/libusb-1.0.0.dylib resolves.
    C.CDLL(LIBUSB, mode=C.RTLD_GLOBAL)
    return C.CDLL(LIBHACKRF)


class hackrf_transfer(C.Structure):
    _fields_ = [
        ("device", C.c_void_p),
        ("buffer", C.POINTER(C.c_uint8)),
        ("buffer_length", C.c_int),
        ("valid_length", C.c_int),
        ("rx_ctx", C.c_void_p),
        ("tx_ctx", C.c_void_p),
    ]


CALLBACK = C.CFUNCTYPE(C.c_int, C.POINTER(hackrf_transfer))


class HackRF:
    def __init__(self):
        self.lib = _load()
        self.dev = C.c_void_p()
        self._chk(self.lib.hackrf_init(), "hackrf_init")

    def _chk(self, rc, what):
        if rc != 0:
            raise RuntimeError(f"{what} failed: rc={rc}")

    def open(self):
        self._chk(self.lib.hackrf_open(C.byref(self.dev)), "hackrf_open")

    def close(self):
        if self.dev:
            self.lib.hackrf_close(self.dev)
            self.dev = C.c_void_p()
        self.lib.hackrf_exit()

    def setup(self, freq_hz, fs_hz, lna=32, vga=40, amp=0, tx_vga=40, bb_bw=None):
        self.lib.hackrf_set_sample_rate.argtypes = [C.c_void_p, C.c_double]
        self.lib.hackrf_set_freq.argtypes = [C.c_void_p, C.c_uint64]
        self._chk(self.lib.hackrf_set_sample_rate(self.dev, C.c_double(fs_hz)), "set_sample_rate")
        self._chk(self.lib.hackrf_set_freq(self.dev, C.c_uint64(int(freq_hz))), "set_freq")
        self.lib.hackrf_set_amp_enable(self.dev, C.c_uint8(amp))
        self.lib.hackrf_set_lna_gain(self.dev, C.c_uint32(lna))   # 0..40 step 8
        self.lib.hackrf_set_vga_gain(self.dev, C.c_uint32(vga))   # 0..62 step 2
        self.lib.hackrf_set_txvga_gain(self.dev, C.c_uint32(tx_vga))  # 0..47
        if bb_bw:
            self.lib.hackrf_set_baseband_filter_bandwidth(self.dev, C.c_uint32(int(bb_bw)))

    # ---- RX ----
    def record(self, path, seconds, fs_hz):
        target = int(seconds * fs_hz * 2)  # 2 bytes per IQ sample (I8,Q8)
        f = open(path, "wb")
        state = {"written": 0, "done": False}

        def _rx(transfer_p):
            t = transfer_p.contents
            n = t.valid_length
            buf = C.cast(t.buffer, C.POINTER(C.c_uint8 * n)).contents
            f.write(bytes(buf))
            state["written"] += n
            if state["written"] >= target:
                state["done"] = True
                return -1
            return 0

        cb = CALLBACK(_rx)
        self._chk(self.lib.hackrf_start_rx(self.dev, cb, None), "start_rx")
        t0 = time.time()
        while not state["done"] and time.time() - t0 < seconds + 5:
            time.sleep(0.05)
        self.lib.hackrf_stop_rx(self.dev)
        f.close()
        return state["written"]

    # ---- TX ----
    def transmit(self, path, fs_hz):
        data = open(path, "rb").read()
        state = {"pos": 0, "done": False}

        def _tx(transfer_p):
            t = transfer_p.contents
            cap = t.buffer_length
            remaining = len(data) - state["pos"]
            n = min(cap, remaining)
            if n > 0:
                src = (C.c_uint8 * n).from_buffer_copy(data[state["pos"]:state["pos"] + n])
                C.memmove(t.buffer, src, n)
                state["pos"] += n
            # zero-pad the rest of the buffer
            if n < cap:
                C.memset(C.cast(t.buffer, C.c_void_p).value + n, 0, cap - n)
            t.valid_length = cap
            if state["pos"] >= len(data):
                state["done"] = True
            return 0

        cb = CALLBACK(_tx)
        self._chk(self.lib.hackrf_start_tx(self.dev, cb, None), "start_tx")
        while not state["done"]:
            time.sleep(0.02)
        time.sleep(0.3)  # flush
        self.lib.hackrf_stop_tx(self.dev)
        return state["pos"]


def cmd_info(_):
    h = HackRF()
    try:
        h.open()
        print("HackRF opened OK (device present, HackRF mode).")
    finally:
        h.close()


def cmd_rx(a):
    h = HackRF()
    try:
        h.open()
        h.setup(a.freq, a.rate, lna=a.lna, vga=a.vga, amp=a.amp, bb_bw=a.rate)
        print(f"Recording {a.seconds}s @ {a.freq/1e6:.4f} MHz, fs={a.rate/1e6:.3f} Msps -> {a.out}")
        n = h.record(a.out, a.seconds, a.rate)
        print(f"Wrote {n} bytes ({n/2} IQ samples)")
    finally:
        h.close()


def cmd_tx(a):
    h = HackRF()
    try:
        h.open()
        h.setup(a.freq, a.rate, tx_vga=a.txvga, amp=a.amp, bb_bw=a.rate)
        print(f"Transmitting {a.inp} @ {a.freq/1e6:.4f} MHz, fs={a.rate/1e6:.3f} Msps")
        n = h.transmit(a.inp, a.rate)
        print(f"Sent {n} bytes")
    finally:
        h.close()


def main():
    p = argparse.ArgumentParser()
    sub = p.add_subparsers(dest="cmd", required=True)
    pi = sub.add_parser("info"); pi.set_defaults(fn=cmd_info)
    pr = sub.add_parser("rx")
    pr.add_argument("-f", "--freq", type=float, required=True)
    pr.add_argument("-s", "--rate", type=float, default=2e6)
    pr.add_argument("-n", "--seconds", type=float, default=4)
    pr.add_argument("-o", "--out", required=True)
    pr.add_argument("--lna", type=int, default=32)
    pr.add_argument("--vga", type=int, default=40)
    pr.add_argument("--amp", type=int, default=0)
    pr.set_defaults(fn=cmd_rx)
    pt = sub.add_parser("tx")
    pt.add_argument("-f", "--freq", type=float, required=True)
    pt.add_argument("-s", "--rate", type=float, default=2e6)
    pt.add_argument("-i", "--inp", required=True)
    pt.add_argument("--txvga", type=int, default=40)
    pt.add_argument("--amp", type=int, default=0)
    pt.set_defaults(fn=cmd_tx)
    a = p.parse_args()
    a.fn(a)


if __name__ == "__main__":
    main()
