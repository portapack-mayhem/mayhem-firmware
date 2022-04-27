# Make mids.db

Licensed under [GNU GPL v3](../../../LICENSE)

Python3 script creates a MID (Marine Identification Digit) database.
MID is part of MMSI and determines (among other things) the conutry. 


USAGE:
 - Copy Excel file from https://www.itu.int/en/ITU-R/terrestrial/fmd/Pages/mid.aspx
 - Convert it to a csv document
 - Run Python 3 script: `./make_mids_db.py` 
 - Copy file to /AIS folder on SDCARD
