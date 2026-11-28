/* WSPR RX M4 heap guard (T07 update; was 22580 with the pre-T07 numbers).
 *
 * The M4 heap is [__heap_base__, __heap_end__): ChibiOS places __main_stack__
 * (0x400), __process_stack__ (WSPR override 0x400, upstream 0x1000), .data and
 * .bss before __heap_base__, so .bss alone does not size the heap.
 *
 * Boot heap demand (ChibiOS heap: 8-byte header + payload rounded to 8 B;
 * payloads below are already multiples of 8):
 *   WSPRRxProcessor                       3192 + 8 =  3200
 *   baseband DMA 8192 x 2 (complex8_t)   16384 + 8 = 16392
 *   channel FIR samples 32 taps x 2 x 4    256 + 8 =   264
 *   channel FIR taps    32 x 4             128 + 8 =   136
 *   USB FIR samples 64 taps x 2 x 4        512 + 8 =   520   (T07 stage)
 *   USB FIR taps        64 x 4             256 + 8 =   264   (T07 stage)
 *   RSSI samples 4 x 400 x 1              1600 + 8 =  1608
 *   RSSI LLI 4 x 16                         64 + 8 =    72
 *   ------------------------------------------------------------------
 *   boot demand                                     22456
 *
 * Heap depends on the linked image. WSPR release (MinSizeRel, FLASH_MB 2/1,
 * non-PRALINE) measured 23100 B: 644 B above boot demand and 132 B above
 * the link-time minimum below.
 *
 * ACCEPTED MINIMUM RESERVE = 512 B (T07_CONTRACT M7a). This is a project
 * margin for allocator rounding and small future needs, NOT a hardware-proven
 * sufficiency: no device-side measurement shows that 512 B is enough for the
 * real M4 workload. The guard makes a build that would leave less than this
 * reserve fail at link time instead of at boot.
 *
 * Recompute the demand whenever the processor size, a FIR configuration or a
 * boot allocation changes (see .zcode/wspr_refactor/t07/T07_METROLOGY.txt).
 */
ASSERT(__heap_end__ - __heap_base__ >= 22968,
       "WSPR M4 heap below boot demand 22456 + 512 B minimum reserve")
