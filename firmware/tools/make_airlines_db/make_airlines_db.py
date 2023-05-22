#!/usr/bin/env python3
# Copyright (C) 2021 ArjanOnwezen
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
import unicodedata
icao_codes=bytearray()
airlines_countries=bytearray()
row_count=0
database=open("airlines.db", "wb")
with open('airlinecodes.txt', 'rt', encoding="utf-8") as csv_file:
    sorted_lines=sorted(csv_file.readlines()[1:]) 
    for row in csv.reader(sorted_lines, quotechar='"', delimiter=',', quoting=csv.QUOTE_ALL, skipinitialspace=True):
            icao_code=row[0]
            # Normalize some unicode characters
            airline=unicodedata.normalize('NFKD', row[1][:32]).encode('ascii', 'ignore')
            country=unicodedata.normalize('NFKD', row[3][:32]).encode('ascii', 'ignore')
            if len(icao_code) == 3 :
                airline_padding=bytearray()
                country_padding=bytearray()
                icao_codes=icao_codes+bytearray(icao_code+'\0', encoding='ascii')
                airline_padding=bytearray('\0' * (32 - len(airline)), encoding='ascii')
                country_padding=bytearray('\0' * (32 - len(country)), encoding='ascii')
                airlines_countries=airlines_countries+bytearray(airline+airline_padding+country+country_padding)
                row_count+=1
database.write(icao_codes+airlines_countries)
print("Total of", row_count, "ICAO codes stored in database")

