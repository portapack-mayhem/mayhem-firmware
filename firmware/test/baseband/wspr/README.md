Run the WSPR host suites independently of the ARM firmware build:

```sh
cmake -S firmware/test/baseband/wspr -B build_wspr_asan -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build_wspr_asan
ctest --test-dir build_wspr_asan --output-on-failure
```

AddressSanitizer and UndefinedBehaviorSanitizer are enabled by default. The suites
cover the encoder/decoder (including multi-slot accumulation) and the capture glue
at both time oversampling settings, including three successive complete windows.
The existing top-level `baseband_test` target only tests FFT/spectrum code.

On hardware, check `baseband_wspr_rx.map`: the M4 heap is `__heap_end__` minus
`__heap_base__`, after the stacks, `.data`, and `.bss`. WSPR's six boot allocations
are processor 3072, baseband DMA 16384, FIR 256+128, RSSI DMA 1600+64, plus six
8-byte heap headers and alignment. `wspr_memory.ld` requires an additional 1 KiB.
The dedicated decoder stack is 4 KiB; the main event-loop stack is 1 KiB.

When checking an incremental build, the `.ppma` M4 payload must exactly match
`PWSR.bin`. The payload starts at the little-endian offset at header byte 80;
the last four bytes are the checksum. The M0 load address at header byte 0 must
be `0x10080000 + len(PWSR.bin)`. A stale bundled image invalidates a hardware test.
