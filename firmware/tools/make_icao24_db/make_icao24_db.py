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
# Create icao24.db, used for ADS-B receiver application, using
# https://opensky-network.org/datasets/metadata/aircraftDatabase.csv 
# as a source.
# -------------------------------------------------------------------------------------
import csv
import unicodedata
icao24_codes=bytearray()
data=bytearray()
row_count=0

database=open("icao24.db", "wb")

with open('aircraftDatabase.csv', 'rt') as csv_file:
    sorted_lines=sorted(csv_file.readlines()[1:]) 
    for row in csv.reader(sorted_lines, quotechar='"', delimiter=',', quoting=csv.QUOTE_ALL, skipinitialspace=True):
            # only store in case enough info is available
            if len(row) == 27 and len(row[0]) == 6 and len(row[1]) > 0:
                icao24_code=row[0][:6].upper()
                registration=row[1][:8].encode('ascii', 'ignore')
                manufacturer=row[3][:32].encode('ascii', 'ignore') 
                model=row[4][:32].encode('ascii', 'ignore')
                # in case icao aircraft type isn't, use ac type like BALL for balloon
                if len(row[8]) == 3:  
                    actype=row[8][:3].encode('ascii', 'ignore') 
                else:
                    actype=row[5][:4].encode('ascii', 'ignore')                
                owner=row[13][:32].encode('ascii', 'ignore')
                operator=row[9][:32].encode('ascii', 'ignore')
                #padding
                icao24_codes=icao24_codes+bytearray(icao24_code+'\0', encoding='ascii')
                registration_padding=bytearray('\0' * (9 - len(registration)), encoding='ascii')    
                manufacturer_padding=bytearray('\0' * (33 - len(manufacturer)), encoding='ascii')
                model_padding=bytearray('\0' * (33 - len(model)), encoding='ascii')
                actype_padding=bytearray('\0' * (5 - len(actype)), encoding='ascii')
                owner_padding=bytearray('\0' * (33 - len(owner)), encoding='ascii')
                operator_padding=bytearray('\0' * (33 - len(operator)), encoding='ascii')
                data=data+bytearray(registration+registration_padding+manufacturer+manufacturer_padding+model+model_padding+actype+actype_padding+owner+owner_padding+operator+operator_padding)
                row_count+=1

database.write(icao24_codes+data)
print("Total of", row_count, "ICAO codes stored in database")

