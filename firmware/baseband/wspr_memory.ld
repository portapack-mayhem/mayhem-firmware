/* WSPR boot allocations on ARM: processor 3072, baseband DMA 16384,
 * FIR 256+128, RSSI samples 1600 and descriptors 64, six 8-byte heap headers.
 * Allow alignment and at least 1 KiB beyond the 21552-byte boot requirement.
 * A .bss-only size check misses the stacks reserved before .data.
 */
ASSERT(__heap_end__ - __heap_base__ >= 22580,
       "WSPR M4 heap cannot hold processor, FIR, DMA and 1 KiB reserve")
