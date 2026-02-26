# Opera Cake

Opera Cake is an antenna-switching add-on board for HackRF One.
The PortaPack app lets you control which antenna port is connected to the HackRF
and can automatically switch ports based on the current receiver frequency.

---

## Hardware Overview

Opera Cake uses a **PCA9557** 8-bit I/O expander at I²C address **0x18**.

| Port | Direction | Description |
|------|-----------|-------------|
| A0   | Input      | HackRF antenna connector |
| A1–A4 | Output   | External antenna ports on side A |
| B0   | Input      | HackRF second port connector |
| B1–B4 | Output   | External antenna ports on side B |

In normal 2×4 operation A0 connects to one of A1–A4, and B0 mirrors it to the
matching Bx port.  A through-switch (U1CTRL) allows 1×8 chaining, but that mode
is not exposed in this app.

---

## App Modes

### Manual

Select any combination of A-side and B-side ports independently.
Press **Apply** to send the selection to the board immediately.

| Control | Description |
|---------|-------------|
| A0 | Choose which port A1–A4 A0 connects to |
| B0 | Choose which port B1–B4 B0 connects to |

### Frequency

Map four frequency bands to ports A1–A4.  A4 also acts as the **catch-all
fallback** for any frequency that does not match A1–A3.  B0 always mirrors A0
(Ax↔Bx).

| Field | Description |
|-------|-------------|
| A1 From / To | Frequency range (MHz) that maps to A1/B1 |
| A2 From / To | Frequency range (MHz) that maps to A2/B2 |
| A3 From / To | Frequency range (MHz) that maps to A3/B3 |
| A4 From / To | Informational; A4 is also the fallback for unmatched frequencies |

Press **Apply** to evaluate the current receiver frequency once and switch.

#### Monitor

When **Monitor: On** is selected (Frequency mode only), the app re-evaluates
the current receiver frequency **once per second** and switches ports
automatically.  This is useful when scanning or tuning across bands.

> **Note:** Monitor only activates when both Mode = Frequency **and** an Opera
> Cake board is detected.  If no board is found at boot, Monitor does nothing
> and will not freeze the device.

---

## Buttons

| Button | Description |
|--------|-------------|
| Apply | Apply the current mode settings to the board immediately |
| Re-scan board | Probe the I²C bus again to detect the board |

---

## Boot Restore

Settings are persisted to the SD card (`opera_cake.ini`).  At every PortaPack
boot the firmware reads these settings and applies them to the board
**automatically**, before the UI starts.

- **Manual mode at boot:** the saved A0/B0 port selection is restored.
- **Frequency mode at boot:** A4/B4 (the fallback port) is selected because no
  receiver frequency is active yet.

If no Opera Cake board is detected (I²C probe timeout), the restore is skipped
silently.

---

## Settings File

Stored on the SD card at `/SETTINGS/opera_cake.ini`:

```
mode=0          # 0 = Manual, 1 = Frequency
port_a=0        # Manual A0 destination: 0=A1 … 3=A4
port_b=0        # Manual B0 destination: 0=B1 … 3=B4
min_a1=1        # A1 band lower bound (MHz)
max_a1=30       # A1 band upper bound (MHz)
min_a2=30
max_a2=300
min_a3=300
max_a3=1000
min_a4=1000
max_a4=6000
```

---

## Supported Boards

| Board | I²C Address | Notes |
|-------|-------------|-------|
| Opera Cake (address pins = 0) | 0x18 | Only one board supported |

Multiple Opera Cake boards (different address-pin settings) are not yet
supported.

---

## PCA9557 Output Byte Reference

```
Bit 7: /OE     — Output Enable, active-low  (0 = switches active)
Bit 6: U2CTRL1 — Port-A mux select bit 1
Bit 5: U2CTRL0 — Port-A mux select bit 0
Bit 4: U3CTRL1 — Port-B mux select bit 1
Bit 3: U3CTRL0 — Port-B mux select bit 0
Bit 2: U1CTRL  — Through-switch (1×8 mode, unused)
Bit 1: LEDEN2  — LED enable 2
Bit 0: LEDEN   — LED enable 1
```

Port-A bit patterns (U2CTRL1:U2CTRL0):

| Port | Bits 6:5 | Hex contribution |
|------|----------|-----------------|
| A1   | 00       | 0x00 |
| A2   | 01       | 0x20 |
| A3   | 10       | 0x40 |
| A4   | 11       | 0x60 |

Port-B bit patterns (U3CTRL1:U3CTRL0):

| Port | Bits 4:3 | Hex contribution |
|------|----------|-----------------|
| B1   | 00       | 0x00 |
| B2   | 01       | 0x08 |
| B3   | 10       | 0x10 |
| B4   | 11       | 0x18 |

---

## Known Limitations

- Time-based automatic switching is **not supported**: the required timer GPIO
  pins conflict with PortaPack hardware.
- Only the first Opera Cake board (I²C address 0x18) is supported.
- In Frequency mode at boot the board defaults to A4/B4 until the receiver is
  tuned and the app is opened (or Monitor is enabled from the app).
