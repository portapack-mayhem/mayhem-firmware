#!/usr/bin/env python3

# Copyright (C) 2022 ArjanOnwezen
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
# Create mids.db, used for AIS receiver application, using converted Excel file on page:
# https://www.itu.int/en/ITU-R/terrestrial/fmd/Pages/mid.aspx
# as a source.
# MID stands for Marine Identification Digits and can be used to determine conutry of
# vessel or coastal station.
# -------------------------------------------------------------------------------------
import csv
import re
import unicodedata
mid_codes=bytearray()
countries=bytearray()
row_count=0

database=open("mids.db", "wb")

with open('MaritimeIdentificationDigits.csv', 'rt') as csv_file:
        sorted_lines=sorted(csv_file.readlines())

        for row in csv.reader(sorted_lines, quotechar='"', delimiter=',', quoting=csv.QUOTE_ALL, skipinitialspace=True):
                mid_code=row[0]
                # Normalize some unicode characters
#unicodedata.normalize('NFKD', row[3][:32]).encode('ascii', 'ignore') 
                country=unicodedata.normalize('NFKD', "".join(re.split("\(|\)|\[|\]", row[1].split('-')[-1].replace(" of Great Britain and Northern Ireland" , ""))[::2])).replace("Democratic People's Republic of Korea", "North-Korea").strip().encode('ascii', 'ignore')[:32]
                if len(mid_code) == 3 :
                        country_padding=bytearray()
                        print(mid_code,' - ', country)
                        mid_codes=mid_codes+bytearray(mid_code+'\0', encoding='ascii')
                        country_padding=bytearray('\0' * (32 - len(country)), encoding='ascii')
                        countries=countries+country+country_padding
                        row_count+=1

database.write(mid_codes+countries)
print("Total of", row_count, "MID codes stored in database")

