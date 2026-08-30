#!/usr/bin/env python3
"""Regression guard: the firmware LoRa TX encoder (proc_lora_tx.cpp build_frame)
must produce a decodable frame at every payload length.

Compiles fw_tx_host.cpp (which #includes the verbatim build_frame chain),
dumps the chip stream for many lengths via `--chips L`, decodes each with the
golden decoder (lora_gold), and checks header, payload and CRC. Motivated by a
suspected long-message TX freeze — this proves the encoder is not the cause.

  python3 test_tx_lengths.py
"""
import subprocess
import sys
import os

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from lora_gold import decode_header, decode_payload  # noqa: E402

BIN = os.path.join(HERE, "fw_tx_host_bin")


def build():
    src = os.path.join(HERE, "fw_tx_host.cpp")
    subprocess.run(["c++", "-std=c++17", "-w", "-I", HERE, "-o", BIN, src], check=True)


def check(length):
    out = subprocess.run([BIN, "--chips", str(length)],
                         capture_output=True, text=True, check=True).stdout
    lines = out.splitlines()
    chips = [int(x) for x in lines[1].split()[1:]]
    crc_tx = lines[2].split()[1]
    expected = bytes((i * 7 + 3) & 0xFF for i in range(length))

    hdr = decode_header(chips, 0)
    hlen = (hdr[0] << 4) | hdr[1]
    cr = (hdr[2] >> 1) & 7
    crc_flag = hdr[2] & 1
    _, dew = decode_payload(chips[8:], 0, cr if 1 <= cr <= 4 else 1)
    raw = _  # raw (whitened) bytes; CRC is NOT whitened
    crc_rx = raw[length:length + 2].hex()
    crc_expect = crc_tx[2:4] + crc_tx[0:2]  # stream order is lo,hi

    ok = (hlen == length and cr == 1 and crc_flag == 1
          and dew[:length] == expected and crc_rx == crc_expect)
    status = "OK  " if ok else "FAIL"
    print(f"{status} L={length:3d} hdr(len={hlen},cr={cr},crc={crc_flag}) "
          f"payload={'ok' if dew[:length] == expected else 'BAD'} "
          f"crc={'ok' if crc_rx == crc_expect else f'BAD {crc_rx}!={crc_expect}'}")
    return ok


def main():
    build()
    lengths = [1, 2, 5, 10, 16, 25, 30, 43, 50, 59, 62, 80, 100, 150, 200, 237]
    ok = all(check(n) for n in lengths)
    print("\n" + ("ALL PASS" if ok else "FAILURES"))
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
