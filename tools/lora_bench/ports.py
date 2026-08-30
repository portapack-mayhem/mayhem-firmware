#!/usr/bin/env python3
"""Find the serial ports of the PortaPack and of a Meshtastic node.

The device names differ by operating system and by which USB socket was used, so
nothing here hardcodes one. Order of preference:

  1. the environment: PORTAPACK_PORT / MESHTASTIC_PORT
  2. whatever is plugged in, recognised by name

The PortaPack announces itself with "Transceiver" in its device name, which is what
separates it from any other USB serial device on the same machine.
"""
import glob
import os
import sys

# Where USB serial devices appear. macOS uses the "cu" (call-out) names; Linux uses
# ttyACM for CDC devices, which is what both boards present.
PATTERNS = ['/dev/cu.usbmodem*', '/dev/cu.usbserial*',
            '/dev/ttyACM*', '/dev/ttyUSB*']


def _candidates():
    out = []
    for p in PATTERNS:
        out.extend(sorted(glob.glob(p)))
    return out


def portapack_port(required=True):
    """The PortaPack's console port."""
    env = os.environ.get('PORTAPACK_PORT')
    if env:
        return env
    for p in _candidates():
        if 'Transceiver' in p:
            return p
    # On Linux the name carries no hint, so a single candidate is taken as it.
    cand = _candidates()
    if len(cand) == 1:
        return cand[0]
    if required:
        sys.exit("PortaPack not found. Plug it in, or set PORTAPACK_PORT=/dev/...\n"
                 "Serial ports seen: " + (', '.join(cand) or 'none'))
    return None


def meshtastic_port(required=True):
    """A stock Meshtastic node (Heltec, T-Beam, ...) for the on-air tests."""
    env = os.environ.get('MESHTASTIC_PORT')
    if env:
        return env
    others = [p for p in _candidates() if 'Transceiver' not in p]
    if len(others) == 1:
        return others[0]
    if required:
        sys.exit("Meshtastic node not found. Plug it in, or set MESHTASTIC_PORT=/dev/...\n"
                 "Serial ports seen: " + (', '.join(_candidates()) or 'none'))
    return None


if __name__ == '__main__':
    print("PortaPack:  ", portapack_port(required=False) or "not found")
    print("Meshtastic: ", meshtastic_port(required=False) or "not found")
