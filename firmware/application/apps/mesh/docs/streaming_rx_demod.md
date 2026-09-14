# Design note: streaming RX demodulator (fix for RX ring truncation)

Status: proposed, not yet implemented. Owner: Alexey Verhogladov.

## Problem (corrected after reading the code)

`firmware/baseband/proc_lora.cpp` has **two** RX paths, and only one of them truncates:

1. **SF8-11 / BW250 - streaming (`execute_sf11`, the default LongFast path).** It runs a
   HUNT -> RESOLVE -> FINE -> PAYLOAD state machine over a ~5.5-symbol *rolling window*
   (`SF11_WIN`), demodulating one payload symbol at a time (`sf11_read_` advances) and
   accumulating decoded bytes into `payload_buf` until `decoded_len_ >= payload_len_target_`
   (the header-declared length, <= `MAX_PAYLOAD` = 255). **Packet length is bounded by the
   255-byte payload buffer, not by any IQ ring.** So a ~120-symbol NodeInfo carrying a PKC
   public key decodes in full on LongFast (and the other BW250 presets). The only failure
   mode is falling behind real time (`sf11_win_stale` -> reset), which is per-symbol and
   independent of packet length.

2. **SF7 / BW500 - store-and-decode (`ring_`, the ShortTurbo path).** It stores a whole
   packet's decimated IQ into `RING = 108 * 256` (54 KiB) and decodes after the packet
   ends. A packet longer than ~108 symbols overruns the ring and the tail is lost
   (`proc_lora.cpp` emits the partial payload; that is why peer *names* survive but the
   tail does not).

So the earlier note ("NodeInfo overruns the ring") is **only true for the SF7
ShortTurbo path**. It was written before the streaming path was generalised to SF8-11.

### What this means for the features we cared about

- **PKC RX (peer keys) should already work on LongFast** and the other BW250 presets,
  because those stream. This must be confirmed on hardware (send a NodeInfo with a public
  key to the PortaPack on LongFast, check the node detail shows `PKC: yes`), but the code
  path supports it.
- **Long chat text** likewise streams fine on LongFast.
- Both are still **truncated on ShortTurbo (SF7/BW500)**, which uses the ring.

### Narrowed scope

The refactor is therefore *not* a from-scratch rewrite and is *not* needed to unblock PKC
on the mainstream presets. It is: **extend the existing streaming model to SF7/BW500 so
ShortTurbo also streams** instead of using the ring. That is the hard real-time case (fast
symbols; an earlier OS=2 streaming attempt missed ~50% of buffers), and it is exactly where
mmmaly's tight OS=2 `lora_lite` is the useful reference. Low priority: verify PKC on
LongFast first; ShortTurbo users are rare.

## Reference: mmmaly/LoraReceiverStandalone (PR #9)

An independent PortaPack/Mayhem LoRa RX app (`lora_lite.h` + `proc_lorarx.cpp`) solves
the same M4 memory constraint with a fundamentally different buffering model. It is
RX only, SF7-10, MeshCore-tested, and not yet run on hardware, so it is a *reference
for the memory architecture*, not a drop-in.

Key idea: the demodulator is a **streaming state machine**. It consumes IQ one symbol
at a time and discards it, keeping only:

- the current symbol window plus a few preamble symbols (for CFO/SFO estimation), and
- the decoded output nibbles (`uint8_t nibbles_[NIB_CAP]`).

Packet length is bounded by the nibble buffer (cheap to grow), not by a raw-IQ ring.
Its M4 loop is trivial:

```
execute(2048 c8) -> decim /4 -> /2 -> 256 complex16
  -> normalize to float -> g_demod.feed(block, 256)   // per block, streaming
```

`feed()` advances internal state and calls `packet_callback()` when a packet
completes; the callback copies metadata + bytes to shared memory and pushes a
message to M0. The whole demod is ~63 KiB `.bss`, no heap.

### Side-by-side

| Aspect | mmmaly lora_lite | our proc_lora |
|---|---|---|
| Held in RAM during a packet | current symbol + preamble window + decoded nibbles | raw IQ of the whole packet (54 KiB ring) |
| Packet length bound | `NIB_CAP` (decoded output, cheap) | ring size = 108 symbols (raw IQ, expensive) |
| Long NodeInfo (~120 sym) | fits (raise `MAX_PAYLOAD`) | overruns ring, tail (PKC key) lost |
| Oversampling | OS=2 default | OS per current path (OS=2 was real-time-infeasible in our attempt) |
| FFT | self-contained radix-2, no malloc | our staged FFT |
| Scope | RX only, SF7-10, not on HW | RX+TX, SF7-11 on-air, HW-verified |

## Why we cannot simply adopt it

Two reasons - one legal, one technical.

### Licensing (the hard blocker on copying code)

- His DSP is derived from **gr-lora_sdr, which is GPL-3.0** (his README: "DSP algorithms
  based on gr-lora_sdr (GPL-3.0)"). His FFT is KissFFT (BSD-3-Clause). His repository
  has **no top-level license**, so his own glue code is effectively all-rights-reserved.
- Mayhem is **GPL-2.0-or-later**. That is technically compatible with GPL-3.0 (the
  "or-later" permits upgrading), but pulling GPL-3.0-derived code into a file makes that
  file effectively GPL-3.0, which upstream may not want; and his unlicensed glue code
  cannot be copied at all.

Conclusion: **do not copy his source.** LoRa PHY *algorithms* (dechirp, FFT correlation,
Gray/Hamming/dewhiten, the streaming state machine) are standard techniques and not
copyrightable as ideas; only the literal code is. So we adopt the *architecture* and
implement it clean-room in our own GPL-2.0-or-later `proc_lora.cpp`. His repo stays a
design reference and a source of MeshCore test vectors, nothing more.

### Technical

Our demod is more battle-tested at the hard end (SF11 real-time on-air, sub-bin
RESOLVE, the tau mod-N/2 half-symbol dead-zone fix). His is cleaner but unproven at
SF11/12 and off-hardware. Replacing our buffer layer wholesale risks regressing the
SF11 reliability we fought for - another reason to keep our sync math and only
restructure the buffering.

## Proposed direction (option 1: keep our sync math, stream the buffer)

Refactor `proc_lora.cpp` so the payload stage consumes and discards IQ per symbol
instead of requiring the whole packet resident in the ring:

- Keep our online preamble/SFD sync (already "no global search" for SF11).
- After sync, demod each payload symbol as its samples complete, emit the nibble/byte,
  and drop those samples. Shrink the ring from "whole packet" to "a few symbols".
- Bound packet length by a decoded-byte buffer sized to Meshtastic's max (256 B),
  which is a few hundred bytes of RAM instead of tens of KiB.

Open risks to validate:

- Whether our payload stage needs the whole packet resident for cross-symbol timing/
  CFO refinement. If it does, that refinement must become incremental (running
  estimate) rather than a global pass.
- Real-time throughput at SF7/BW500 (fast symbols). Our earlier OS=2 streaming attempt
  missed ~50% of buffers; his tighter implementation suggests it is achievable, but it
  must be measured on hardware.
- SF11 regression: the streaming path must reproduce the current SF11 on-air decode
  rate before it replaces the ring path.

## Sequencing

1. Ship the current feature set (incl. PKC advertise/TX) and open the PR, with RX ring
   truncation documented as a known limitation (PKC RX of peer keys pending this work).
2. Prototype the streaming payload stage behind a build flag; A/B it against the ring
   path on the same on-air captures (SF7-11) before switching the default.
3. Once truncation is gone, PKC RX (peer key reception) and long text work unblock for
   free; re-test PKC round trip end to end.

## Later: MeshCore

The LoRa PHY layer is protocol-agnostic (the CSS demod recovers raw frames regardless
of Meshtastic vs MeshCore). MeshCore support would be a second frame/crypto parser on
top of the same PHY, selectable as an RX "mode". mmmaly's captures are MeshCore, so
his repo is also a source of real MeshCore test vectors. Track as a separate initiative
after the streaming refactor lands.
