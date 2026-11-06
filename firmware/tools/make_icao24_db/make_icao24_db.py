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

"""Create icao24.db from the OpenSky and Mictronics aircraft databases."""

import csv
import os
import shutil
import urllib.request
import zipfile
from dataclasses import dataclass
from typing import Dict, Set, Tuple


OPENSKY_CSV_URL = (
    "https://opensky-network.org/datasets/metadata/aircraftDatabase.csv"
)
MICRONICS_ZIP_URL = (
    "https://github.com/Mictronics/aircraft-database/"
    "raw/refs/heads/main/icao24plus.zip"
)


@dataclass
class AircraftRecord:
    """Represents an aircraft record with all relevant fields."""

    icao24: str
    registration: str
    manufacturer: str
    model: str
    actype: str
    owner: str
    operator: str

    def data_equals(self, other: "AircraftRecord") -> bool:
        """Check whether all fields other than ICAO24 are equal."""
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
    """Back up the existing OpenSky CSV file, if present."""
    if os.path.exists("aircraftDatabase.csv"):
        shutil.copy2("aircraftDatabase.csv", "aircraftDatabase_previous.csv")
        print("Backed up previous CSV file")
        return True
    return False


def download_file(url: str, filename: str) -> None:
    """Download a file from a URL."""
    print(f"Downloading {filename} from {url}...")
    urllib.request.urlretrieve(url, filename)
    print(f"Download completed: {filename}")


def parse_opensky_csv(filename: str) -> Dict[str, AircraftRecord]:
    """Parse the OpenSky CSV file."""
    records: Dict[str, AircraftRecord] = {}

    if not os.path.exists(filename):
        return records

    try:
        with open(filename, "rt", encoding="utf-8", newline="") as csv_file:
            # Keep the source's existing ordering behavior.
            sorted_lines = sorted(csv_file.readlines()[1:])
            for row in csv.reader(
                sorted_lines,
                quotechar='"',
                delimiter=",",
                quoting=csv.QUOTE_ALL,
                skipinitialspace=True,
            ):
                if len(row) != 27 or len(row[0]) != 6 or not row[1]:
                    continue

                icao24_code = row[0][:6].strip().upper()
                if len(icao24_code) != 6:
                    continue

                # Use the ICAO aircraft type when available; otherwise use
                # the aircraft type field (for example, BALL for a balloon).
                actype = row[8][:3] if len(row[8]) == 3 else row[5][:4]

                records[icao24_code] = AircraftRecord(
                    icao24=icao24_code,
                    registration=row[1][:8],
                    manufacturer=row[3][:32],
                    model=row[4][:32],
                    actype=actype,
                    owner=row[13][:32],
                    operator=row[9][:32],
                )
    except Exception as exc:
        print(f"Warning: Could not parse CSV file {filename}: {exc}")
        return {}

    return records


def extract_micronics_tsv(zip_filename: str) -> str:
    """Extract the TSV data file from the Mictronics ZIP archive."""
    extracted_filename = "icao24plus.tsv"

    with zipfile.ZipFile(zip_filename, "r") as archive:
        members = [name for name in archive.namelist() if not name.endswith("/")]
        if not members:
            raise ValueError(f"No files found in {zip_filename}")

        # Prefer a member named icao24plus, otherwise use the first file.
        member = next(
            (
                name
                for name in members
                if os.path.basename(name).lower().startswith("icao24plus")
            ),
            members[0],
        )

        with archive.open(member, "r") as source:
            with open(extracted_filename, "wb") as destination:
                shutil.copyfileobj(source, destination)

    print(f"Extracted {member} to {extracted_filename}")
    return extracted_filename


def parse_micronics_tsv(filename: str) -> Dict[str, AircraftRecord]:
    """Parse the Mictronics tab-separated file.

    Its columns are ICAO24, registration, aircraft type, and an ignored
    fourth column. A header row, if present, is skipped.
    """
    records: Dict[str, AircraftRecord] = {}

    with open(filename, "rt", encoding="utf-8-sig", errors="replace", newline="") as tsv_file:
        for row in csv.reader(tsv_file, delimiter="\t"):
            if len(row) < 3:
                continue

            icao24_code = row[0].strip().upper()
            if icao24_code.lower() == "icao24":
                continue
            if len(icao24_code) != 6:
                continue

            registration = row[1].strip()[:8]
            actype = row[2].strip()[:4]

            records[icao24_code] = AircraftRecord(
                icao24=icao24_code,
                registration=registration,
                manufacturer="",
                model="",
                actype=actype,
                owner="",
                operator="",
            )

    return records


def merge_records(
    opensky_records: Dict[str, AircraftRecord],
    micronics_records: Dict[str, AircraftRecord],
) -> Dict[str, AircraftRecord]:
    """Merge both sources, using Mictronics registration/type when provided.

    OpenSky records retain their manufacturer, model, owner, and operator
    fields. Mictronics records with no OpenSky match are added with those
    fields empty.
    """
    merged = dict(opensky_records)

    for icao24_code, micronics_record in micronics_records.items():
        opensky_record = merged.get(icao24_code)

        if opensky_record is None:
            merged[icao24_code] = micronics_record
            continue

        if micronics_record.registration:
            opensky_record.registration = micronics_record.registration
        if micronics_record.actype:
            opensky_record.actype = micronics_record.actype

    return merged


def compare_records(
    old_records: Dict[str, AircraftRecord],
    new_records: Dict[str, AircraftRecord],
) -> Tuple[Set[str], Set[str], Set[str]]:
    """Return the sets of new, deleted, and changed ICAO24 codes."""
    old_keys = set(old_records)
    new_keys = set(new_records)

    new_icao_codes = new_keys - old_keys
    deleted_icao_codes = old_keys - new_keys
    changed_icao_codes = {
        icao24_code
        for icao24_code in old_keys & new_keys
        if not old_records[icao24_code].data_equals(new_records[icao24_code])
    }

    return new_icao_codes, deleted_icao_codes, changed_icao_codes


def write_database(records: Dict[str, AircraftRecord]) -> int:
    """Write records to icao24.db in the existing binary format."""
    icao24_codes = bytearray()
    data = bytearray()

    for record in sorted(records.values(), key=lambda item: item.icao24):
        registration = record.registration.encode("ascii", "ignore")[:8]
        manufacturer = record.manufacturer.encode("ascii", "ignore")[:32]
        model = record.model.encode("ascii", "ignore")[:32]
        actype = record.actype.encode("ascii", "ignore")[:4]
        owner = record.owner.encode("ascii", "ignore")[:32]
        operator = record.operator.encode("ascii", "ignore")[:32]

        icao24_codes.extend(record.icao24.encode("ascii") + b"\0")

        fields = (
            (registration, 9),
            (manufacturer, 33),
            (model, 33),
            (actype, 5),
            (owner, 33),
            (operator, 33),
        )
        for value, field_size in fields:
            data.extend(value)
            data.extend(b"\0" * (field_size - len(value)))

    with open("icao24.db", "wb") as database:
        database.write(icao24_codes + data)

    return len(records)


def print_change_summary(
    old_records: Dict[str, AircraftRecord],
    new_records: Dict[str, AircraftRecord],
) -> None:
    """Print a summary of changes between two record sets."""
    new_codes, deleted_codes, changed_codes = compare_records(
        old_records, new_records
    )

    print("\n" + "=" * 50)
    print("DATABASE CHANGE SUMMARY")
    print("=" * 50)
    print(f"New records:     {len(new_codes):>8}")
    print(f"Deleted records: {len(deleted_codes):>8}")
    print(f"Changed records: {len(changed_codes):>8}")
    print(f"Total records:   {len(new_records):>8}")
    print("=" * 50)

    for label, codes in (
        ("New", new_codes),
        ("Deleted", deleted_codes),
        ("Changed", changed_codes),
    ):
        if not codes:
            continue

        sorted_codes = sorted(codes)
        if len(sorted_codes) <= 10:
            print(f"{label} ICAO24 codes: {', '.join(sorted_codes)}")
        else:
            sample = ", ".join(sorted_codes[:5])
            print(f"Sample {label.lower()} ICAO24 codes: {sample} "
                  f"(and {len(sorted_codes) - 5} more)")


def create_database() -> None:
    """Download both sources, merge them, and create the database."""
    has_previous = backup_previous_csv()

    download_file(OPENSKY_CSV_URL, "aircraftDatabase.csv")
    download_file(MICRONICS_ZIP_URL, "icao24plus.zip")

    old_records: Dict[str, AircraftRecord] = {}
    if has_previous:
        print("Parsing previous OpenSky CSV file...")
        old_records = parse_opensky_csv("aircraftDatabase_previous.csv")
        print(f"Found {len(old_records)} records in previous CSV")

    print("Parsing new OpenSky CSV file...")
    opensky_records = parse_opensky_csv("aircraftDatabase.csv")
    print(f"Found {len(opensky_records)} OpenSky records")

    tsv_filename = extract_micronics_tsv("icao24plus.zip")
    print("Parsing Mictronics TSV file...")
    micronics_records = parse_micronics_tsv(tsv_filename)
    print(f"Found {len(micronics_records)} Mictronics records")

    merged_records = merge_records(opensky_records, micronics_records)
    print(f"Found {len(merged_records)} records after merging")

    if old_records:
        print_change_summary(old_records, merged_records)
    else:
        print("\nNo previous CSV found - this appears to be the first run")

    print("\nCreating ICAO24 database...")
    row_count = write_database(merged_records)
    print(f"Total of {row_count} ICAO codes stored in database")


def cleanup_previous_files() -> None:
    """Remove temporary/previous input files if they exist."""
    for filename in (
        "aircraftDatabase_previous.csv",
        "icao24plus.zip",
        "icao24plus.tsv",
    ):
        if os.path.exists(filename):
            os.remove(filename)


if __name__ == "__main__":
    try:
        create_database()
        cleanup_previous_files()
    except Exception as exc:
        print(f"Error creating ICAO24 database: {exc}")
    else:
        print("ICAO24 database created successfully.")
