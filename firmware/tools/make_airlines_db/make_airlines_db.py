#!/usr/bin/env python3
# Copyright (C) 2021 ArjanOnwezen
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
# Create airline.db, used for ADS-B receiver application, using
# https://raw.githubusercontent.com/kx1t/planefence-airlinecodes/main/airlinecodes.txt
# as a source.
# -------------------------------------------------------------------------------------
import csv
import os
import shutil
import unicodedata
import urllib.request
from dataclasses import dataclass
from typing import Dict, Set, Tuple


@dataclass
class AirlineRecord:
    """Represents an airline record with all relevant fields"""

    icao_code: str
    airline: str
    country: str

    def __hash__(self):
        return hash(self.icao_code)

    def __eq__(self, other):
        if not isinstance(other, AirlineRecord):
            return False
        return self.icao_code == other.icao_code

    def data_equals(self, other):
        """Check if all data fields are equal (excluding ICAO code which is the key)"""
        if not isinstance(other, AirlineRecord):
            return False
        return self.airline == other.airline and self.country == other.country


def backup_previous_file() -> bool:
    """Backup the existing airlinecodes.txt file if it exists. Returns True if backup was made."""
    if os.path.exists("airlinecodes.txt"):
        shutil.copy2("airlinecodes.txt", "airlinecodes_previous.txt")
        print("Backed up previous airline codes file")
        return True
    return False


def download_airline_codes() -> None:
    """Download the airline codes file from GitHub"""
    url = "https://raw.githubusercontent.com/kx1t/planefence-airlinecodes/main/airlinecodes.txt"
    print(f"Downloading airline codes database from {url}...")
    try:
        urllib.request.urlretrieve(url, "airlinecodes.txt")
        print("Download completed successfully.")
    except Exception as e:
        print(f"Error downloading airline codes file: {e}")
        raise


def parse_airline_file(filename: str) -> Dict[str, AirlineRecord]:
    """Parse airline codes file and return a dictionary of records"""
    records = {}

    if not os.path.exists(filename):
        return records

    try:
        with open(filename, "rt", encoding="utf-8") as csv_file:
            sorted_lines = sorted(csv_file.readlines()[1:])
            for row in csv.reader(
                sorted_lines,
                quotechar='"',
                delimiter=",",
                quoting=csv.QUOTE_ALL,
                skipinitialspace=True,
            ):
                if len(row) >= 4:  # Ensure we have enough columns
                    icao_code = row[0]
                    # Normalize some unicode characters
                    airline = (
                        unicodedata.normalize("NFKD", row[1][:32])
                        .encode("ascii", "ignore")
                        .decode("ascii")
                    )
                    country = (
                        unicodedata.normalize("NFKD", row[3][:32])
                        .encode("ascii", "ignore")
                        .decode("ascii")
                    )

                    if len(icao_code) == 3:
                        records[icao_code] = AirlineRecord(
                            icao_code=icao_code, airline=airline, country=country
                        )
    except Exception as e:
        print(f"Warning: Could not parse airline file {filename}: {e}")
        return {}

    return records


def compare_records(
    old_records: Dict[str, AirlineRecord], new_records: Dict[str, AirlineRecord]
) -> Tuple[Set[str], Set[str], Set[str]]:
    """Compare old and new records, return sets of new, deleted, and changed ICAO codes"""
    old_keys = set(old_records.keys())
    new_keys = set(new_records.keys())

    new_icao_codes = new_keys - old_keys
    deleted_icao_codes = old_keys - new_keys

    # Check for changes in existing records
    changed_icao_codes = set()
    for icao in old_keys & new_keys:
        if not old_records[icao].data_equals(new_records[icao]):
            changed_icao_codes.add(icao)

    return new_icao_codes, deleted_icao_codes, changed_icao_codes


def write_database(records: Dict[str, AirlineRecord]) -> int:
    """Write records to database file using original format"""
    icao_codes = bytearray()
    airlines_countries = bytearray()
    row_count = 0

    # Sort by ICAO code for consistency
    sorted_records = sorted(records.values(), key=lambda r: r.icao_code)

    for record in sorted_records:
        # Normalize and encode data (same as original)
        airline = unicodedata.normalize("NFKD", record.airline[:32]).encode(
            "ascii", "ignore"
        )
        country = unicodedata.normalize("NFKD", record.country[:32]).encode(
            "ascii", "ignore"
        )

        # Add ICAO code with null terminator (original format)
        icao_codes = icao_codes + bytearray(record.icao_code + "\0", encoding="ascii")

        # Add padded data fields (original format)
        airline_padding = bytearray("\0" * (32 - len(airline)), encoding="ascii")
        country_padding = bytearray("\0" * (32 - len(country)), encoding="ascii")

        airlines_countries = airlines_countries + bytearray(
            airline + airline_padding + country + country_padding
        )
        row_count += 1

    with open("airlines.db", "wb") as database:
        database.write(icao_codes + airlines_countries)

    return row_count


def create_database() -> None:
    """Create the airline database from the downloaded file"""
    # Backup existing file before downloading new one
    has_previous = backup_previous_file()

    # Download new file
    download_airline_codes()

    # Parse both files for comparison
    old_records = {}
    if has_previous:
        print("Parsing previous airline codes file...")
        old_records = parse_airline_file("airlinecodes_previous.txt")
        print(f"Found {len(old_records)} records in previous file")

    print("Parsing new airline codes file...")
    new_records = parse_airline_file("airlinecodes.txt")
    print(f"Found {len(new_records)} records in new file")

    # Compare records if we have a previous version
    if old_records:
        new_icao_codes, deleted_icao_codes, changed_icao_codes = compare_records(
            old_records, new_records
        )

        # Print change statistics
        print("\n" + "=" * 50)
        print("AIRLINE CODES CHANGE SUMMARY")
        print("=" * 50)
        print(f"New records:     {len(new_icao_codes):>8}")
        print(f"Deleted records: {len(deleted_icao_codes):>8}")
        print(f"Changed records: {len(changed_icao_codes):>8}")
        print(f"Total records:   {len(new_records):>8}")
        print("=" * 50)

        # Show examples of changes (limited to avoid spam)
        if new_icao_codes and len(new_icao_codes) <= 15:
            print(f"\nNew ICAO codes: {', '.join(sorted(new_icao_codes))}")
        elif new_icao_codes:
            sample_new = sorted(list(new_icao_codes))[:10]
            print(
                f"\nSample new ICAO codes: {', '.join(sample_new)} (and {len(new_icao_codes)-10} more)"
            )

        if deleted_icao_codes and len(deleted_icao_codes) <= 15:
            print(f"Deleted ICAO codes: {', '.join(sorted(deleted_icao_codes))}")
        elif deleted_icao_codes:
            sample_deleted = sorted(list(deleted_icao_codes))[:10]
            print(
                f"Sample deleted ICAO codes: {', '.join(sample_deleted)} (and {len(deleted_icao_codes)-10} more)"
            )

        if changed_icao_codes and len(changed_icao_codes) <= 15:
            print(f"Changed ICAO codes: {', '.join(sorted(changed_icao_codes))}")
        elif changed_icao_codes:
            sample_changed = sorted(list(changed_icao_codes))[:10]
            print(
                f"Sample changed ICAO codes: {', '.join(sample_changed)} (and {len(changed_icao_codes)-10} more)"
            )

        # Show some specific examples of changes
        if changed_icao_codes:
            print(f"\nExample changes:")
            for icao in sorted(list(changed_icao_codes))[:3]:
                old_rec = old_records[icao]
                new_rec = new_records[icao]
                if old_rec.airline != new_rec.airline:
                    print(
                        f"  {icao}: Airline '{old_rec.airline}' → '{new_rec.airline}'"
                    )
                if old_rec.country != new_rec.country:
                    print(
                        f"  {icao}: Country '{old_rec.country}' → '{new_rec.country}'"
                    )
    else:
        print(
            "\nNo previous airline codes file found - this appears to be the first run"
        )

    # Create database from new records
    print("\nCreating airline database...")
    row_count = write_database(new_records)
    print("Total of", row_count, "ICAO codes stored in database")


def cleanup_temp() -> None:
    """Cleanup temporary files created during the process"""
    if os.path.exists("airlinecodes_previous.txt"):
        os.remove("airlinecodes_previous.txt")


if __name__ == "__main__":
    try:
        create_database()
        cleanup_temp()
    except Exception as e:
        print(f"Error creating airline database: {e}")
    else:
        print("Airline database created successfully.")
