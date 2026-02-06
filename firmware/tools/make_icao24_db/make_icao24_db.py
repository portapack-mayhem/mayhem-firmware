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
# Create icao24.db, used for ADS-B receiver application, using
# https://opensky-network.org/datasets/metadata/aircraftDatabase.csv
# as a source.
# -------------------------------------------------------------------------------------
import csv
import os
import shutil
import urllib.request
from dataclasses import dataclass
from typing import Dict, Set, Tuple


@dataclass
class AircraftRecord:
    """Represents an aircraft record with all relevant fields"""

    icao24: str
    registration: str
    manufacturer: str
    model: str
    actype: str
    owner: str
    operator: str

    def __hash__(self):
        return hash(self.icao24)

    def __eq__(self, other):
        if not isinstance(other, AircraftRecord):
            return False
        return self.icao24 == other.icao24

    def data_equals(self, other):
        """Check if all data fields are equal (excluding ICAO24 which is the key)"""
        if not isinstance(other, AircraftRecord):
            return False
        return (
            self.registration == other.registration
            and self.manufacturer == other.manufacturer
            and self.model == other.model
            and self.actype == other.actype
            and self.owner == other.owner
            and self.operator == other.operator
        )


def backup_previous_csv() -> bool:
    """Backup the existing CSV file if it exists. Returns True if backup was made."""
    if os.path.exists("aircraftDatabase.csv"):
        shutil.copy2("aircraftDatabase.csv", "aircraftDatabase_previous.csv")
        print("Backed up previous CSV file")
        return True
    return False


def download_aircraft_database() -> None:
    """Download the aircraft database file from OpenSky Network"""
    url = "https://opensky-network.org/datasets/metadata/aircraftDatabase.csv"
    print(f"Downloading aircraft database from {url}...")
    try:
        urllib.request.urlretrieve(url, "aircraftDatabase.csv")
        print("Download completed successfully.")
    except Exception as e:
        print(f"Error downloading aircraft database file: {e}")
        raise


def parse_csv_file(filename: str) -> Dict[str, AircraftRecord]:
    """Parse CSV file and return a dictionary of records"""
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
                # only store in case enough info is available
                if len(row) == 27 and len(row[0]) == 6 and len(row[1]) > 0:
                    icao24_code = row[0][:6].upper()
                    registration = row[1][:8]
                    manufacturer = row[3][:32]
                    model = row[4][:32]
                    # in case icao aircraft type isn't, use ac type like BALL for balloon
                    if len(row[8]) == 3:
                        actype = row[8][:3]
                    else:
                        actype = row[5][:4]
                    owner = row[13][:32]
                    operator = row[9][:32]

                    records[icao24_code] = AircraftRecord(
                        icao24=icao24_code,
                        registration=registration,
                        manufacturer=manufacturer,
                        model=model,
                        actype=actype,
                        owner=owner,
                        operator=operator,
                    )
    except Exception as e:
        print(f"Warning: Could not parse CSV file {filename}: {e}")
        return {}

    return records


def compare_records(
    old_records: Dict[str, AircraftRecord], new_records: Dict[str, AircraftRecord]
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


def write_database(records: Dict[str, AircraftRecord]) -> int:
    """Write records to database file using original format"""
    # bytearrays to store icao24 codes and data
    icao24_codes = bytearray()
    data = bytearray()
    row_count = 0

    # Sort by ICAO code for consistency
    sorted_records = sorted(records.values(), key=lambda r: r.icao24)

    for record in sorted_records:
        # Encode data fields
        registration = record.registration.encode("ascii", "ignore")
        manufacturer = record.manufacturer.encode("ascii", "ignore")
        model = record.model.encode("ascii", "ignore")
        actype = record.actype.encode("ascii", "ignore")
        owner = record.owner.encode("ascii", "ignore")
        operator = record.operator.encode("ascii", "ignore")

        # Add ICAO code with null terminator (original format)
        icao24_codes.extend(bytearray(record.icao24 + "\0", encoding="ascii"))

        # Add padded data fields (original format)
        registration_padding = bytearray(
            "\0" * (9 - len(registration)), encoding="ascii"
        )
        manufacturer_padding = bytearray(
            "\0" * (33 - len(manufacturer)), encoding="ascii"
        )
        model_padding = bytearray("\0" * (33 - len(model)), encoding="ascii")
        actype_padding = bytearray("\0" * (5 - len(actype)), encoding="ascii")
        owner_padding = bytearray("\0" * (33 - len(owner)), encoding="ascii")
        operator_padding = bytearray("\0" * (33 - len(operator)), encoding="ascii")

        data.extend(
            bytearray(
                registration
                + registration_padding
                + manufacturer
                + manufacturer_padding
                + model
                + model_padding
                + actype
                + actype_padding
                + owner
                + owner_padding
                + operator
                + operator_padding
            )
        )
        row_count += 1

    with open("icao24.db", "wb") as database:
        database.write(icao24_codes + data)

    return row_count


def create_database() -> None:
    """Create the ICAO24 database from the downloaded file"""
    # Backup existing CSV before downloading new one
    has_previous = backup_previous_csv()

    # Download new CSV
    download_aircraft_database()

    # Parse both CSV files for comparison
    old_records = {}
    if has_previous:
        print("Parsing previous CSV file...")
        old_records = parse_csv_file("aircraftDatabase_previous.csv")
        print(f"Found {len(old_records)} records in previous CSV")

    print("Parsing new CSV file...")
    new_records = parse_csv_file("aircraftDatabase.csv")
    print(f"Found {len(new_records)} records in new CSV")

    # Compare records if we have a previous version
    if old_records:
        new_icao_codes, deleted_icao_codes, changed_icao_codes = compare_records(
            old_records, new_records
        )

        # Print change statistics
        print("\n" + "=" * 50)
        print("CSV CHANGE SUMMARY")
        print("=" * 50)
        print(f"New records:     {len(new_icao_codes):>8}")
        print(f"Deleted records: {len(deleted_icao_codes):>8}")
        print(f"Changed records: {len(changed_icao_codes):>8}")
        print(f"Total records:   {len(new_records):>8}")
        print("=" * 50)

        # Show examples of changes (limited to avoid spam)
        if new_icao_codes and len(new_icao_codes) <= 10:
            print(f"\nNew ICAO24 codes: {', '.join(sorted(new_icao_codes))}")
        elif new_icao_codes:
            sample_new = sorted(list(new_icao_codes))[:5]
            print(
                f"\nSample new ICAO24 codes: {', '.join(sample_new)} (and {len(new_icao_codes)-5} more)"
            )

        if deleted_icao_codes and len(deleted_icao_codes) <= 10:
            print(f"Deleted ICAO24 codes: {', '.join(sorted(deleted_icao_codes))}")
        elif deleted_icao_codes:
            sample_deleted = sorted(list(deleted_icao_codes))[:5]
            print(
                f"Sample deleted ICAO24 codes: {', '.join(sample_deleted)} (and {len(deleted_icao_codes)-5} more)"
            )

        if changed_icao_codes and len(changed_icao_codes) <= 10:
            print(f"Changed ICAO24 codes: {', '.join(sorted(changed_icao_codes))}")
        elif changed_icao_codes:
            sample_changed = sorted(list(changed_icao_codes))[:5]
            print(
                f"Sample changed ICAO24 codes: {', '.join(sample_changed)} (and {len(changed_icao_codes)-5} more)"
            )
    else:
        print("\nNo previous CSV found - this appears to be the first run")

    # Create database from new records
    print("\nCreating ICAO24 database...")
    row_count = write_database(new_records)
    print("Total of", row_count, "ICAO codes stored in database")


def cleanup_previous_csv() -> None:
    """Remove the previous CSV file if it exists"""
    if os.path.exists("aircraftDatabase_previous.csv"):
        os.remove("aircraftDatabase_previous.csv")


if __name__ == "__main__":
    try:
        create_database()
        cleanup_previous_csv()
    except Exception as e:
        print(f"Error creating ICAO24 database: {e}")
    else:
        print("ICAO24 database created successfully.")
