# WSPR RX

WSPR RX is a receive-only app for amateur-radio Weak Signal Propagation
Reporter (WSPR) transmissions. It searches for packets in the selected band's
WSPR channel and lists the stations it decodes.

![WSPR RX receiving live 20 m signals on PortaPack](images/wspr_rx.png)

[WSPR protocol on Wikipedia](https://en.wikipedia.org/wiki/WSPR_(amateur_radio_software)#The_WSPR_protocol)

## Getting started

- Adjust **RF Amp**, **LNA** and **VGA** for your antenna and signal conditions.
- Leave the receiver running through several complete transmissions. Each
  lasts about 110.6 seconds, and a station may skip several slots.
- **Optional:** Press **RX RESTART** just before an even UTC minute (for
  example, just before 12:04 UTC) if you want to start a fresh capture ahead
  of the next slot. WSPR transmissions typically begin about one second
  into that minute. During normal reception the app finds transmission timing
  automatically.

## Station table

Each row represents one callsign. Scroll the table to see more stations.

| Field | Meaning |
| --- | --- |
| **CALL** | Decoded callsign. The character to its left shows the confirmation state below. |
| **N** | Number of distinct accepted reports for this callsign, up to 999. Reprocessing the same transmission leaves the count unchanged. |
| **GRID** | Maidenhead locator reported by the station. |
| **PWR** | Transmit power declared in the latest accepted WSPR message, in dBm. |
| **SNR** | Approximate received signal-to-noise ratio estimated by PortaPack for the latest accepted report, in dB. Negative values are normal for WSPR. |

### Callsign markers

| Marker | Meaning |
| --- | --- |
| `?` | Tentative report from one transmission. The early decoder recovered the same payload in a second pass using neighboring tone powers. Both passes use the same transmission and count as one report. |
| `*` | Unconfirmed report from the ordinary multi-slot decode path. The station table is waiting for an independent matching report. |
| Blank | Distinct transmissions reported the same callsign, locator and power on the same dial. The row stays confirmed during silence. |

A later accepted report that changes the locator, declared power or receiver
dial starts confirmation again. The row keeps its report count. A change in the
estimated audio offset alone preserves confirmation.

### Checking a possible false decode

Weak signals, noise or interference can produce a plausible-looking WSPR
message with an incorrect callsign, locator or power. Either marker can appear
next to a false decode. For `?`, both passes can agree on the same wrong
payload. A `*` reflects multi-slot decoding, and the station table is still
waiting for an independent matching report.

If a row looks suspicious, leave the app on the same dial and look for another
report with the same callsign, locator and power. You can also compare those
fields with a second receiver or WSPR spots from the same band and time.
A matching repeat raises confidence. Stations may transmit only occasionally.

### Receiver status

| Display | Meaning |
| --- | --- |
| `FILLING` | Collecting the first full receive buffer, typically after opening the app or restarting capture. |
| `RECEIVING` | The buffer is ready and capture is active. The decoder is idle between passes. |
| `DECODING` | A scan or decode pass is running. A decoded station appears separately in the table when a report is accepted. |
| `NO STATUS 301s` | The interface has gone more than 300 seconds without a status message from the baseband. The number shows the age of the last status. |

The counter following the receiver state (for example, `r123/351`) shows
the current row position in the circular receive buffer. It wraps as capture
continues.

## Buttons

- **RX RESTART** restarts capture and decoding without clearing the station list. It discards reception data accumulated so far. The button reads **WAIT** until the receiver acknowledges the restart.
- **CLEAR LIST** clears the station list and its confirmation state.

## How it works

The receiver continuously collects spectral data in a ring buffer and searches
for the WSPR synchronisation pattern. Selected candidates are copied for
processing while capture continues. The decoder combines soft bits across
compatible transmissions. It can also report a packet early when the
decoded payload agrees on a second pass using the neighboring tone powers.
That pass uses the same transmission, so it counts as one observation.
The receiver uses sample timing and searches roughly 200 Hz around the
1.5 kHz USB audio offset.

## Reception notes

Weak stations may be missed even when a dedicated receiver decodes them.
Sensitivity has not been formally benchmarked against other WSPR receivers.
