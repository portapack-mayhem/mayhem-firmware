#!/usr/bin/env python3
# Copyright (C) 2025 Tommaso Ventafridda
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.
#
# -------------------------------------------------------------------------------------
# Create macaddress.db, used for BLE receiver application, using
# https://standards-oui.ieee.org/oui/oui.txt as a source.
# -------------------------------------------------------------------------------------

import argparse
import os
import ssl
import sys
import tempfile
import time
import urllib.request
import unicodedata
import re
from typing import List, Tuple

OUI_URL = "https://standards-oui.ieee.org/oui/oui.txt"

# IEEE limits oui.txt downloads to one per day, so keep a cached copy around
CACHE_FILE = os.path.join(tempfile.gettempdir(), "mayhem_oui.txt")
CACHE_MAX_AGE = 24 * 60 * 60

# IEEE answers the default "Python-urllib" User-Agent with HTTP 418
HEADERS = {
    "User-Agent": "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/126.0 Safari/537.36",
    "Accept": "text/plain,*/*;q=0.9",
    "Accept-Language": "en-US,en;q=0.9",
}


def download_oui_file(force: bool = False) -> str:
    """Download the OUI file from IEEE, reusing a cached copy younger than 24h"""
    if (
        not force
        and os.path.exists(CACHE_FILE)
        and time.time() - os.path.getmtime(CACHE_FILE) < CACHE_MAX_AGE
    ):
        print(f"Using cached OUI database {CACHE_FILE} (use --force to re-download).")
        with open(CACHE_FILE, encoding="utf-8") as f:
            return f.read()

    print(f"Downloading OUI database from {OUI_URL}...")
    req = urllib.request.Request(OUI_URL, headers=HEADERS)
    ctx = ssl.create_default_context()
    with urllib.request.urlopen(req, context=ctx, timeout=60) as response:
        content = response.read().decode("utf-8", errors="replace")
    print("Download completed successfully.")

    # Only cache something that looks like the real file, not a block page
    if "(hex)" in content:
        with open(CACHE_FILE, "w", encoding="utf-8") as f:
            f.write(content)
    return content


def parse_oui_data(content: str) -> List[Tuple[str, str]]:
    """Parse the OUI file content and extract MAC prefixes and vendor names"""
    entries = []
    lines = content.split("\n")

    for _, line in enumerate(lines):
        # Look for lines with (hex) pattern
        if "(hex)" in line:
            # Extract MAC prefix and vendor name
            match = re.match(
                r"^([0-9A-F]{2}-[0-9A-F]{2}-[0-9A-F]{2})\s+\(hex\)\s+(.+)$",
                line.strip(),
            )
            if match:
                # Remove dashes: "28-6F-B9" -> "286FB9"
                mac_prefix = match.group(1).replace(
                    "-", ""
                ) 
                vendor_name = match.group(2).strip()

                # Normalize vendor name and limit to 63 characters
                # (normalize first: one unicode char can expand to several ascii bytes)
                vendor_name = (
                    unicodedata.normalize("NFKD", vendor_name)
                    .encode("ascii", "ignore")
                    .decode("ascii")[:63]
                )

                if mac_prefix and vendor_name:
                    entries.append((mac_prefix, vendor_name))

    print(f"Parsed {len(entries)} MAC address entries.")
    return entries


def create_database(
    entries: List[Tuple[str, str]], output_filename: str = "macaddress.db"
):
    """Create the binary database file."""
    # Sort entries by MAC prefix for binary search
    entries.sort(key=lambda x: x[0])

    mac_prefixes = bytearray()
    vendor_data = bytearray()
    row_count = 0

    for mac_prefix, vendor_name in entries:
        # MAC prefix: 6 hex chars + null terminator = 7 bytes
        mac_prefixes += bytearray(mac_prefix + "\0", encoding="ascii")

        # Vendor name: pad to 64 bytes
        vendor_bytes = vendor_name.encode("ascii", "ignore")
        vendor_padding = bytearray("\0" * (64 - len(vendor_bytes)), encoding="ascii")
        vendor_data += vendor_bytes + vendor_padding

        row_count += 1

    # Write database: index section followed by data section
    with open(output_filename, "wb") as database:
        database.write(mac_prefixes + vendor_data)

    print(f"Created database '{output_filename}' with {row_count} MAC address entries.")
    print(f"Index section: {len(mac_prefixes)} bytes")
    print(f"Data section: {len(vendor_data)} bytes")
    print(f"Total database size: {len(mac_prefixes) + len(vendor_data)} bytes")


def main():
    """Main function to create the MAC address database."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--force",
        action="store_true",
        help="re-download oui.txt even if a cached copy younger than 24h exists",
    )
    args = parser.parse_args()

    try:
        oui_content = download_oui_file(args.force)
        entries = parse_oui_data(oui_content)

        if not entries:
            print("No valid entries found in OUI file!")
            return 1

        create_database(entries)

        print("MAC address database creation completed successfully!")
        return 0

    except Exception as e:
        print(f"Error creating MAC address database: {e}")
        return 1


if __name__ == "__main__":
    sys.exit(main())
