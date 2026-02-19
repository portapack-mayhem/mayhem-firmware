# HackRF Pro (PRALINE) Touchscreen Investigation

## Problem

The touchscreen does not work when the PortaPack is connected to a **HackRF Pro (PRALINE)**.
The screen itself works (display is fine), but touch input is unresponsive.
The same PortaPack works correctly on a **HackRF One**.

---

## How Touch Works

The resistive touchscreen system has two separate signal paths:

### 1. Drive Path (MCU → CPLD → Touch Panel)

The firmware writes touch configuration (XP/XN/YP/YN drive states) via the 8-bit data bus
to the PortaPack CPLD, which latches them into `tp_q` and drives `TP_U/D/L/R` output pins.

**Key files:**
- `firmware/common/portapack_io.cpp` — `IO::io_update()` writes drive config to CPLD
- `firmware/application/irq_controls.cpp` — timer ISR cycles through `SensePressure / SenseX / SenseY`
- `hardware/portapack_h1/cpld/20170522/top.vhd` — CPLD VHDL captures `tp_q` and drives `TP_U/D/L/R`
- `hardware/portapack_h4m/CPLD/AG256SL100/top.vhd` — H4M CPLD (same logic)

### 2. Sense Path (Touch Panel Voltages → LPC43xx ADC0)

The resulting analog voltage divider on the touch panel wires is read directly by ADC0
on the LPC43xx microcontroller. The channel assignments are **hardcoded** with no
platform-specific variants:

| ADC0 Channel | LPC43xx Pin | Touch Signal |
|:---:|:---:|:---:|
| CH0 | P4_3 | YP |
| CH2 | P4_4 | YN |
| CH5 | P4_5 | XP |
| CH6 | P4_6 | XN |

**Key file:** `firmware/common/portapack_adc.hpp`

```cpp
constexpr size_t adc0_touch_yp_input = 0;  // P4_3 → ADC0_CH0
constexpr size_t adc0_touch_yn_input = 2;  // P4_4 → ADC0_CH2
constexpr size_t adc0_touch_xp_input = 5;  // P4_5 → ADC0_CH5
constexpr size_t adc0_touch_xn_input = 6;  // P4_6 → ADC0_CH6
```

There is **no `#ifdef PRALINE`** — the same ADC channels are assumed for all hardware variants.

---

## Root Causes Found

### Issue 1 — P4_3 Left in SGPIO9 Function on PRALINE ⚠️ HIGH

In the common pin setup in `board.cpp`, P4_3 is configured as **SGPIO9**:

```cpp
// firmware/chibios-portapack/boards/PORTAPACK_APPLICATION/board.cpp ~L337
{ 4, 3, { .mode=7, .epd=0, .epun=1, .ehs=0, .ezi=1, .zif=1 } }  // SGPIO9 / HOST_CAPTURE
```

For PRALINE, SGPIO9 is remapped to P9_3 in `__early_init()`:

```cpp
// board.cpp ~L965
LPC_SCU->SFSP[9][3] = 0xF6;  /* SGPIO9 = P9_3 function 6 (HOST_CAPTURE) */
```

**However, P4_3 is never reconfigured after this.** It remains configured as SGPIO9 function
(mode=7). If the SGPIO peripheral drives P4_3 with any digital activity, it will corrupt
ADC0_CH0 (touch YP), which is used for **pressure detection**. This likely causes the
touch system to never register a valid press.

---

### Issue 2 — ADC Analog Input Pins Not Configured with Analog Mode ⚠️ MEDIUM

For correct ADC analog input operation on LPC43xx, the SCU pin must have `ZIF=1`
(input buffer disable / high-impedance for analog). In `pins_setup_og` / `pins_setup_r9`,
the P4_5 and P4_6 pins are configured as digital GPIO for RF control (`zif=0`):

```cpp
// board.cpp ~L314-315
{ 4, 5, { .mode=0, .epd=0, .epun=1, .ehs=0, .ezi=0, .zif=0 } },  // RXENABLE (HackRF One)
{ 4, 6, { .mode=0, .epd=0, .epun=1, .ehs=0, .ezi=0, .zif=0 } },  // XCVR_EN (HackRF One)
```

On **PRALINE**, MAX2837 is replaced by MAX2831 controlled via different pins,
so P4_5/P4_6 are no longer needed for RF control — but they are never explicitly
overridden to analog input mode for the PortaPack touch function.

---

### Issue 3 — touch_adc.cpp Has No Platform Variants ⚠️ FOR REVIEW

`firmware/application/hw/touch_adc.cpp` always initialises ADC0 on the same four
channels. If HackRF Pro routes the PortaPack expansion connector touch signals
to **different LPC43xx pins**, the ADC channel assignments in `portapack_adc.hpp`
would need `#ifdef PRALINE` variants.

**Requires hardware schematic confirmation** — see `hardware/portapack_h4m/To_HackRF_Connector_Schematic.pdf`.

---

## Summary Table

| # | Issue | File(s) | Severity |
|---|-------|---------|----------|
| 1 | P4_3 remains configured as SGPIO9 after PRALINE remaps SGPIO9 to P9_3 — corrupts ADC0_CH0 (touch YP / pressure detection) | `board.cpp` L337 + L965 | **High** |
| 2 | P4_3–P4_6 not reconfigured as analog inputs (`zif=1`) for PRALINE | `board.cpp` | Medium |
| 3 | `portapack_adc.hpp` has no PRALINE variant — assumes same ADC pins as HackRF One | `portapack_adc.hpp`, `touch_adc.cpp` | Needs HW verification |

---

## Applied Fixes

> **Status**: Fix 1 and Fix 2 have been combined and applied in
> `firmware/chibios-portapack/boards/PORTAPACK_APPLICATION/board.cpp`
> inside the existing `#ifdef PRALINE` `__early_init` block, immediately after
> the SGPIO9 → P9_3 remapping line.

### Fix 1 — Release P4_3 from SGPIO9 for PRALINE (in `board.cpp` `__early_init`)

After the SGPIO9 remapping to P9_3, add:

```cpp
// Release P4_3 for PortaPack ADC0_CH0 (touch YP) — SGPIO9 moved to P9_3
// SCU value 0x90 = EPUN bit[4]=1 | ZIF bit[7]=1 | mode[2:0]=0 (analog input, no pull-down, no drive)
LPC_SCU->SFSP[4][3] = 0x90;
```

### Fix 2 — Configure P4_3–P4_6 as Analog Inputs for PRALINE

```cpp
#ifdef PRALINE
// Configure PortaPack touch ADC pins as analog inputs on PRALINE
// (these pins are not used for RF control on PRALINE)
LPC_SCU->SFSP[4][3] = 0x90;  /* ADC0_CH0 — touch YP: mode=0, EPUN=1, ZIF=1 */
LPC_SCU->SFSP[4][4] = 0x90;  /* ADC0_CH2 — touch YN */
LPC_SCU->SFSP[4][5] = 0x90;  /* ADC0_CH5 — touch XP */
LPC_SCU->SFSP[4][6] = 0x90;  /* ADC0_CH6 — touch XN */
#endif
```

### Fix 3 (If Hardware Routes are Different) — Add PRALINE ADC Channel Variants

If the H4M schematic confirms the touch signals reach *different* LPC43xx ADC pins
through the HackRF Pro expansion connector, update `portapack_adc.hpp`:

```cpp
#ifdef PRALINE
constexpr size_t adc0_touch_yp_input = /* correct channel for Pro */;
constexpr size_t adc0_touch_yn_input = /* correct channel for Pro */;
constexpr size_t adc0_touch_xp_input = /* correct channel for Pro */;
constexpr size_t adc0_touch_xn_input = /* correct channel for Pro */;
#else
constexpr size_t adc0_touch_yp_input = 0;
constexpr size_t adc0_touch_yn_input = 2;
constexpr size_t adc0_touch_xp_input = 5;
constexpr size_t adc0_touch_xn_input = 6;
#endif
```

---

## Investigation Steps

1. **Confirm hardware connectivity** — open `hardware/portapack_h4m/To_HackRF_Connector_Schematic.pdf`
   and verify the H4M PortaPack touch panel lines (TP_U / TP_D / TP_L / TP_R) reach
   LPC43xx P4_3 / P4_4 / P4_5 / P4_6 through the HackRF Pro expansion connector.
   If they route to different pins, update `portapack_adc.hpp`.

2. **Apply Fix 1** as the most likely cause — P4_3 SGPIO9 conflict.

3. **Apply Fix 2** to ensure all four ADC touch pins are in analog input mode on PRALINE.

4. **Test** with a debug build that logs raw ADC values from `touch_adc.cpp` to confirm
   the readings change when the screen is pressed.

---

## Relevant Source Files

| File | Purpose |
|------|---------|
| `firmware/common/portapack_adc.hpp` | ADC0 channel assignments for touch X/Y |
| `firmware/application/hw/touch_adc.cpp` | ADC0 init and sample read |
| `firmware/application/irq_controls.cpp` | Touch scanning state machine (timer ISR) |
| `firmware/common/portapack_io.cpp` | `io_update()` — bus context + touch drive write |
| `firmware/common/portapack_io.hpp` | `TouchPinsConfig` enum (XP/XN/YP/YN bit patterns) |
| `firmware/chibios-portapack/boards/PORTAPACK_APPLICATION/board.cpp` | Pin/SCU setup, PRALINE `__early_init` |
| `hardware/portapack_h1/cpld/20170522/top.vhd` | PortaPack H1 CPLD — drives TP_U/D/L/R |
| `hardware/portapack_h4m/CPLD/AG256SL100/top.vhd` | PortaPack H4M CPLD — same TP drive logic |
| `hardware/portapack_h4m/To_HackRF_Connector_Schematic.pdf` | **Check this for pin routing confirmation** |
