#!/usr/bin/env python3
"""Send one command to the PortaPack console and print what comes back."""
import sys, time, serial
from ports import portapack_port
PORT = portapack_port()

def run(cmd, wait=2.0, port=PORT):
    with serial.Serial(port, 115200, timeout=0.3) as s:
        s.reset_input_buffer()
        s.write((cmd + "\r\n").encode()); s.flush()
        t0 = time.time(); buf = b""
        while time.time() - t0 < wait:
            chunk = s.read(65536)
            if chunk:
                buf += chunk; t0 = min(t0, time.time())
            else:
                time.sleep(0.05)
        return buf.decode("utf-8", "replace")

if __name__ == "__main__":
    cmd = sys.argv[1]
    wait = float(sys.argv[2]) if len(sys.argv) > 2 else 2.0
    sys.stdout.write(run(cmd, wait))
