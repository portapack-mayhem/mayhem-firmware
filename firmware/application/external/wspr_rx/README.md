# WSPR RX

WSPR RX receives amateur-radio WSPR transmissions and displays the callsign,
Maidenhead locator and declared transmit power. It is receive-only.

![WSPR RX station table](images/wspr_rx.png)

[Watch a reception demonstration](https://youtu.be/VbYk4-VxHH8).

## Getting started

1. Install `wspr_rx.ppma` in the SD card's `APPS` folder, using the matching Mayhem firmware and app set.
2. Open **Receive → WSPR**.
3. Select a band preset or set the WSPR dial frequency manually.
4. Adjust **RF Amp**, **LNA** and **VGA** for your antenna and signal conditions.
5. Leave the receiver running through several complete transmissions.

WSPR transmissions last about 110.6 seconds and normally begin about one second
into an even UTC minute. The app searches for their timing automatically. If
you start halfway through a transmission, that packet may be missed; reception
continues for subsequent transmissions. Stations do not necessarily transmit
every slot, so results can take several minutes.

## Station table

Scroll the table to see more stations.

| Field | Meaning |
| --- | --- |
| **CALL** | Decoded callsign. |
| **N** | Number of counted observations of that callsign. Re-decoding the same transmission does not increase it. |
| **GRID** | Station's Maidenhead locator. |
| **FREQ** | Audio offset from the dial frequency, in Hz. |
| **SNR** | Approximate signal-to-noise estimate, in dB. |
| **A%** | Agreement between the decoded message and received symbol decisions, not a probability that the callsign is correct. |
| **Age** | Time since the latest counted observation. |

A grey **? CALL** is a decoded message awaiting an independent matching repeat.
The callsign turns green when the same callsign, locator and power are decoded
again from a different transmission. It stays confirmed during silence.
Changing locator or power requires confirmation again; frequency changes alone
do not.

The **... frequency age** indicator shows a recent search candidate that has
not produced an accepted message. It is not a decoded callsign.

## Buttons

- **RX RESTART** restarts capture and decoding without clearing the station list. It discards reception data accumulated so far.
- **CLEAR LIST** clears the station list and its confirmation state.

When checking reception against a known transmitter, you can press **RX
RESTART** a few seconds before an even UTC minute to start a fresh capture
before its transmission. This is a convenient way to run a controlled check,
not a required synchronisation step or a proven sensitivity improvement.
Repeated restarts interrupt accumulation; normally, leave the app running.

## How it works

The receiver continuously collects spectral data in a ring buffer and searches
for the WSPR synchronisation pattern. Selected candidates are copied for
processing while capture continues. Soft symbol data from compatible
transmission slots is combined before error-correcting decoding and message
checks. The current decoder requires evidence from at least two slots before
reporting a message.

## Reception notes

Weak stations may be missed even when a dedicated receiver decodes them.
Sensitivity has not been formally benchmarked against other WSPR receivers.

Occasional false decodes are possible. Independent matching repeats reduce
this risk but do not eliminate it. A green callsign or high A% is not a
guarantee; check doubtful results against another receiver or WSPR reports.
