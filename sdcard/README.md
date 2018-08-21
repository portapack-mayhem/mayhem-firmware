# How to generate world_map.bin

World_map.bin is required if you want to use the world map view in some apps. It's a huge (~450MB) raw image file in a format that can be easily rendered by the PortaPack.

Since Github doesn't allow uploading such large files, you must generate it yourself from the provided jpg file.

1. Make sure that `world_map.jpg` is in `/sdcard/ADSB`.
1. Go in `/firmware/tools`.
1. Run 'python adsb_map.py'. Give it some time.
1. `world_map.bin` should appear ! Leave it in the ADSB directory.
