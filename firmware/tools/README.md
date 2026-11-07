# World map converter

`generate_world_map.bin.py` requires Python 3, NumPy, and Pillow. All repository
Docker build definitions install the two libraries through their distribution
package manager; rebuild existing images to pick up these dependencies.

For a local installation, run from the repository root:

```sh
python3 -m venv .venv-world-map
. .venv-world-map/bin/activate
python3 -m pip install -r firmware/tools/requirements-world-map.txt
python3 firmware/tools/generate_world_map.bin.py \
    --input sdcard/ADSB/world_map.jpg --output /tmp/world_map.bin
```

On Debian/Ubuntu, install `python3-venv` if needed to create the environment.
Alternatively, install `python3-numpy` and `python3-pil` with APT and run the
converter with the system Python. Alpine packages are `py3-numpy` and
`py3-pillow`.

Without arguments, run from `firmware/tools`: the default input and output
are `../../sdcard/ADSB/world_map.jpg` and `../../sdcard/ADSB/world_map.bin`.
Use a separate output filename to preserve an existing map for comparison.
The converter rejects input/output aliases, including existing hard links.
Pillow decodes the full source image into memory; NumPy conversion uses
64-row chunks. The binary stores little-endian 16-bit width and height,
followed by row-major little-endian RGB565 pixels.

Run the converter regression tests from the repository root:

```sh
python3 -m unittest discover -s firmware/tools -p 'test_generate_world_map.py'
```
