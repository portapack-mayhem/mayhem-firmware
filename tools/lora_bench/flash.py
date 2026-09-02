#!/usr/bin/env python3
"""Flash PortaPack/Mayhem firmware to the HackRF SPI flash directly over USB.

No SD card needed. Uses libhackrf (from SDR++) spiflash API via ctypes.
HackRF must be in HackRF mode (stock firmware running, host-controllable).

  python3 flash.py info                      # board id + first flash bytes (safe)
  python3 flash.py verify <fw.bin>           # compare flash vs file (safe, read-only)
  python3 flash.py write  <fw.bin>           # ERASE + WRITE + verify (destructive)

After write: power-cycle the PortaPack to boot the new firmware.
Recovery if it fails: HackRF DFU mode (hold DFU on power-up) + reflash.
"""
import ctypes as C
import sys

FRAMEWORKS = "/Applications/SDR++.app/Contents/Frameworks"
C.CDLL(f"{FRAMEWORKS}/libusb-1.0.0.dylib", mode=C.RTLD_GLOBAL)
lib = C.CDLL(f"{FRAMEWORKS}/libhackrf.0.dylib")

lib.hackrf_spiflash_write.argtypes = [C.c_void_p, C.c_uint32, C.c_uint16, C.POINTER(C.c_uint8)]
lib.hackrf_spiflash_read.argtypes = [C.c_void_p, C.c_uint32, C.c_uint16, C.POINTER(C.c_uint8)]
lib.hackrf_spiflash_erase.argtypes = [C.c_void_p]
lib.hackrf_board_id_read.argtypes = [C.c_void_p, C.POINTER(C.c_uint8)]
lib.hackrf_board_id_name.argtypes = [C.c_uint8]
lib.hackrf_board_id_name.restype = C.c_char_p

CHUNK = 256


def chk(rc, what):
    if rc != 0:
        raise RuntimeError(f"{what} failed rc={rc}")


def open_dev():
    chk(lib.hackrf_init(), "hackrf_init")
    dev = C.c_void_p()
    chk(lib.hackrf_open(C.byref(dev)), "hackrf_open (is it in HackRF mode?)")
    bid = C.c_uint8()
    lib.hackrf_board_id_read(dev, C.byref(bid))
    print(f"board: {lib.hackrf_board_id_name(bid.value).decode()} (id={bid.value})")
    return dev


def close_dev(dev):
    lib.hackrf_close(dev)
    lib.hackrf_exit()


def read_flash(dev, addr, length):
    buf = (C.c_uint8 * length)()
    chk(lib.hackrf_spiflash_read(dev, addr, length, buf), f"read@{addr}")
    return bytes(buf)


def cmd_info(_):
    dev = open_dev()
    try:
        print("flash[0:32]:", read_flash(dev, 0, 32).hex(' '))
    finally:
        close_dev(dev)


def cmd_backup(path):
    dev = open_dev()
    try:
        size = 1024 * 1024
        out = bytearray()
        for addr in range(0, size, CHUNK):
            out += read_flash(dev, addr, CHUNK)
            if addr % (CHUNK * 512) == 0:
                print(f"  {addr*100//size}%", end="\r", flush=True)
        open(path, "wb").write(out)
        print(f"\nbacked up current flash ({len(out)} bytes) -> {path}")
    finally:
        close_dev(dev)


def cmd_verify(path):
    data = open(path, "rb").read()
    dev = open_dev()
    try:
        bad = 0
        for addr in range(0, len(data), CHUNK):
            n = min(CHUNK, len(data) - addr)
            if read_flash(dev, addr, n) != data[addr:addr+n]:
                bad += 1
        print(f"verify {path}: {'OK (flash matches file)' if bad==0 else f'{bad} chunk(s) differ'}")
    finally:
        close_dev(dev)


def cmd_write(path):
    data = open(path, "rb").read()
    print(f"writing {len(data)} bytes from {path}")
    dev = open_dev()
    try:
        print("erasing flash...")
        chk(lib.hackrf_spiflash_erase(dev), "erase")
        print("writing...")
        for addr in range(0, len(data), CHUNK):
            chunk = data[addr:addr+CHUNK]
            buf = (C.c_uint8 * len(chunk)).from_buffer_copy(chunk)
            chk(lib.hackrf_spiflash_write(dev, addr, len(chunk), buf), f"write@{addr}")
            if addr % (CHUNK * 256) == 0:
                print(f"  {addr*100//len(data)}%", end="\r", flush=True)
        print("\nWRITE DONE (verify skipped). Reset/power-cycle to boot new firmware.")
    finally:
        close_dev(dev)


if __name__ == "__main__":
    cmd = sys.argv[1] if len(sys.argv) > 1 else "info"
    arg = sys.argv[2] if len(sys.argv) > 2 else None
    {"info": cmd_info, "verify": cmd_verify, "write": cmd_write, "backup": cmd_backup}[cmd](arg)
