# lora_bench

The bench the Meshtastic app was built on. Two kinds of thing live here: tests that
run on your computer and need no hardware, and scripts that drive a PortaPack and a
Meshtastic node over USB to test on the air.

## Tests, no hardware needed

```
make            build and run all of them
make clean
```

They compile the firmware's own sources, not copies, so a test that passes here is
testing the code that ships. Any C++17 compiler will do.

| test | what it holds the code to |
|---|---|
| `test_mesh_regions` | 6429 combinations of region, preset and slot against an independently written implementation of Meshtastic's frequency formula, plus points measured on hardware |
| `test_lora_framing` | 192 end-to-end frame round-trips across every spreading factor, bandwidth and coding rate, and one frame recorded off the air from a stock node |
| `test_lora_ref` | the chunked reference-chirp generator against one computed whole |
| `test_lora_rssi` | the dechirp-peak to dBm mapping |
| `test_mesh_crypto` | AES-CTR channel encryption and the passphrase-to-key derivation |
| `test_mesh_router` | dedup, relay mutation, channel hash filtering, packet id seeding, the traceroute request |
| `test_mesh_telemetry` | telemetry encode and decode per variant |
| `test_mesh_partial` | decoding a packet whose tail was cut off mid-field |

A round-trip test proves only that the two ends agree with each other. That is why
`test_lora_framing` also carries a real frame from a stock node: a wrong coding rate
sailed through the round-trip for weeks because both ends read the same wrong table,
and only an outside frame could disagree.

## The glyph table

```
python3 gen_font.py --list       scripts, sizes, which language needs what
python3 gen_font.py cyrillic     writes mesh_font.fnt
```

Copy the result to `/APPS/mesh_font.fnt` on the SD card. Needs Pillow and matplotlib
(for a monospace TTF to trace glyphs from). The generator walks every alphabet it
claims to cover and refuses to write a file that falls short.

## Scripts that need hardware

These talk to a PortaPack over its USB console, and to a stock Meshtastic node
through the `meshtastic` command-line tool (`pip install meshtastic`).

Ports are found automatically: the PortaPack is the one whose device name contains
"Transceiver". Override either with an environment variable when that guess is wrong:

```
export PORTAPACK_PORT=/dev/ttyACM0
export MESHTASTIC_PORT=/dev/ttyACM1
python3 ports.py                 print what was found
```

| script | what it does |
|---|---|
| `mc.py "<cmd>"` | run one PortaPack console command and print the reply |
| `shot.py out.png` | capture the screen (240x320) to a PNG |
| `ocr.py shot.png` | read the screen back as text, matching against the firmware's own fonts, so the result is the exact characters rather than a guess |
| `sweep_one.py <preset>` | drive both radios through one preset in both directions and report pass or fail |
| `preset_set.py <preset>` | switch the app's modem preset through the UI |
| `hlisten.py [seconds]` | print every packet the Meshtastic node receives |
| `watch_rx.py` | the same, filtered to text |
| `capture.sh` | record raw IQ from the HackRF for offline analysis |
| `flash.py`, `hreset.py`, `diag.py` | flash a build, reset the radio, read device state |

`ocr.py` is worth knowing about: reading the screen as text rather than as an image
is what makes a scripted on-air test possible at all, because it turns "did the chat
show the message" into something a script can answer.

## Offline analysis

The demodulator was debugged by dumping raw symbol bins from the device and decoding
them on a computer, where you can print anything and take as long as you like. These
carry that work:

| file | what it is |
|---|---|
| `fw_rx_host.cpp`, `fw_rx_host_sf11.cpp` | the firmware's receive DSP compiled for the host, verbatim, so a capture can be decoded the way the device would |
| `fw_tx_host.cpp` | the same for the transmitter |
| `fw_sf11_stream_host.cpp` | the streaming receive path, for timing experiments |
| `lora_decode.py`, `lora_encode.py` | an independent implementation, deliberately written from the specification rather than from our code, so it can disagree with us |
| `lora_crc.py`, `lora_gold.py`, `lora_inspect.py` | frame checksum, a known-good reference decoder, and a bin-by-bin dump |
| `sf12_off.py` | decodes an SF12 capture end to end; the algorithm the device does not yet have the time budget to run |
| `selftest_demod.py`, `sync_test.py` | synthetic signals through the demodulator |
| `heltec_rx_monitor/` | an Arduino sketch that turns a Heltec into a bare LoRa receiver, for when the question is "did anything at all leave the antenna" |

Captures are not in the repository; `capture.sh` makes them.
