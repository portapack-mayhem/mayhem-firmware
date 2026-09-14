# Mesh — Meshtastic on PortaPack

*Draft of the wiki article for this app. Kept in the tree so it is reviewed with the
code; the published copy lives on the project wiki.*

> **Screenshot placeholder** — main screen (Chat tab), to be added before publishing.

## What it does

Joins a [Meshtastic](https://meshtastic.org) LoRa mesh as a full node: text chat,
encrypted channels, a node list, telemetry, positions, a map, and mesh routing.

The PortaPack is not a LoRa radio and has no chip that understands the protocol. Every
part of it is done in software here — the modulation and demodulation on the M4, the
protocol on the M0 — so this app talks to stock Meshtastic nodes on the air rather than
to other PortaPacks only.

Verified against a stock Heltec V4 running Meshtastic firmware: seven of the nine modem
presets work in both directions, an eighth transmits.

## What you need

- **An antenna for your band.** 868/915 MHz for most regions, 433 MHz for EU_433. A
  telescopic whip works; Settings → Radio shows the quarter-wave length to extend it to.
- **An SD card with the matching build on it.** This is not optional and is the most
  common reason "half the menu is missing": most baseband images and applications live on
  the card, not in the firmware. Copy the release's `FIRMWARE`, `APPS` **and**
  `BASEBAND` folders, all from the same build.
- **Another Meshtastic node** to talk to, unless you are only listening.

## First run

1. **Settings → Radio → Region.** Nothing works until this matches where you are: it
   decides the frequency band. The app cannot know it for you.
2. **Settings → Radio → Preset.** Must match the other nodes. `LONG_FAST` is the
   Meshtastic default and the right first choice.
3. **Settings → Profile → Name.** What other nodes will call you.
4. That is enough to receive. To be seen, tap **Info** on the Data tab, which announces
   you immediately instead of waiting for the timer.

## The four tabs

### Chat

Messages, newest at the bottom. Incoming lines carry a colour stripe on the left
identifying the sender; your own are set against the right edge with a delivery mark.

| control | what it does |
|---|---|
| `>>` | write a message |
| `Ch:` | which channel or private thread you are in |
| `>All` | who the message goes to — everyone, or one node |
| `All` | filter: everything / messages only / one node |
| `X` | clear the on-screen chat |

Scroll with the encoder, or land on the chat with the D-pad and page with Up/Down.

**Delivery marks.** Four states, and the distinction between the last two matters:

| mark | meaning |
|---|---|
| yellow | in flight, or waiting for confirmation |
| green | confirmed — a neighbour acknowledged it, or flooded it back to us |
| red | a **direct** message nobody acknowledged |
| yellow **?** | sent, and nothing came back |

A broadcast has no single recipient, so nobody owes it an answer: "no echo" is not the
same as "did not arrive", and it is not drawn as failure. While a message is being
retried, the attempt count is shown beside it.

### Nodes

Everyone heard, with signal, hop count and age. Up to 10 nodes, a hard limit set by
the memory left after the app loads, and shown in the footer as `Nodes: n/10`. Tap a node
for its detail page: eight pages of what it has told you, and four buttons under them.

| button | what it does |
|---|---|
| **Message** | open a private conversation with that node |
| **Exchange info** | send it our details and ask for its own, including its public key |
| **Share QR** | its name, id and public key as a QR code |
| **Map** | show it on the map, or read `No pos` if it has never sent one |
| **Trace** | ask which path reaches it; the answer arrives in the chat as `* route` |
| **Stats** | ask for the router's own counters |
| **Metrics** | ask for battery, voltage and airtime |

The **Route** page keeps the answers to the last six traceroutes, newest first, with a
star against the node whose card you are on. Tracing the same node again replaces its
line rather than adding one, so the page shows the current path to each node rather
than a scroll of repeats.

`Trace` and `Metrics` work on a node that has never sent a position or a battery
reading, which is the point of them. A reply takes a few seconds and lands on the page
the button opened.

`Stats` and `Metrics` are answered by another PortaPack. A stock Meshtastic node may
answer neither: measured against a Heltec V4 on current firmware, the request arrives
and is acknowledged, and no telemetry comes back. The request itself is byte for byte
what the official client sends, and the same node's own request to us is answered, so
this is the far end's choice rather than a fault at either end. Nothing is lost by
pressing it; it simply may stay quiet.

A page with nothing on it says `not reported` rather than showing an empty screen.

### Data

Your own position, battery, channel utilisation and uptime, plus the nodes that have sent
coordinates and a map with links between them.

| button | what it does |
|---|---|
| **Map** | full-screen map |
| **Telem** | what you broadcast about yourself |
| **Info** | announce yourself now |
| **Pos** | send your position now |

### Answering other people

Requests that arrive from a phone app or another node are answered without you doing
anything, unless **Listen only** is set:

| they ask for | we send |
|---|---|
| exchange user info | our NodeInfo, public key included |
| position | our position, if one is set |
| device metrics | battery, voltage, uptime, airtime |
| local stats | the router's counters |
| traceroute | the path their packet took to reach us |

The user info reply is worth knowing about: it carries our public key, and without that
key nobody can send us an encrypted direct message. It is answered even when beacons
are switched off, because a reply to a direct question is not a beacon.

### Setup

Profile, Radio, Privacy, System and Chat pages.

Worth knowing on **Radio**:

- **Freq** — leave on `Auto (Region)` unless you know why not. A manual frequency
  overrides the region and preset, and a node on a different frequency is simply not there.
- **CR** — leave on `preset`. The receiver reads the coding rate from each packet's own
  header, so this only affects what you transmit.
- **TX pwr** — `Region` steps the gain down by as many dB as your region's limit sits
  below the most permissive one; `Max` asks for everything; `Custom` is a number. This
  informs rather than enforces: the gain scale is not calibrated in dBm, and what leaves
  the antenna depends on band, hardware and antenna. **Knowing your local limits is your
  responsibility.** The firmware's own global TX cap still applies on top.
- **Whip 1/4 wave** — how far to extend a telescopic antenna for the frequency in use.

## Presets

Each is a trade of range against speed. Both ends must use the same one.

| preset | spreading | bandwidth | symbol | note |
|---|---|---|---|---|
| SHORT_TURBO | SF7 | 500 kHz | 0.26 ms | fastest, shortest |
| SHORT_FAST | SF7 | 250 kHz | 0.5 ms | |
| SHORT_SLOW | SF8 | 250 kHz | 1.0 ms | |
| MEDIUM_FAST | SF9 | 250 kHz | 2.0 ms | |
| MEDIUM_SLOW | SF10 | 250 kHz | 4.1 ms | |
| LONG_FAST | SF11 | 250 kHz | 8.2 ms | **Meshtastic default** |
| LONG_MODERATE | SF11 | 125 kHz | 16.4 ms | |
| LONG_SLOW | SF12 | 125 kHz | 32.8 ms | transmit only, see limitations |

`VERY_LONG_SLOW` is not offered: current Meshtastic firmware no longer implements it — a
node told to use it comes back reporting LongFast's parameters.

## Channels and encryption

The **primary** channel uses Meshtastic's well-known key, so it interoperates with stock
nodes out of the box. "Encryption" there separates networks; it does not keep secrets.

**Custom channels** take a name and a passphrase. Two PortaPacks with the same passphrase
will talk. **With a stock node they will not**, unless you enter that node's raw 32-hex
key instead of a passphrase: the way this app turns a passphrase into a key is its own and
does not match Meshtastic's.

Private messages between nodes that have exchanged public keys are encrypted to those
keys. A node's public key only travels in its NodeInfo, which stock nodes send every few
hours, so the app keeps the keys it has learned on the card.

## Other alphabets

The firmware font carries ASCII and Latin-1, and the flash is full, so anything else
comes off the card. Put a glyph table at `/APPS/mesh_font.fnt` and the chat can read
it; without one, those characters show as dots and the chat says so rather than
pretending. `tools/lora_bench/gen_font.py` writes the file:

```
python3 gen_font.py --list                     # scripts, sizes, and what needs what
python3 gen_font.py cyrillic                   # ru, uk, be, bg, sr, mk, kk, tt, ba
python3 gen_font.py greek latin-ext            # combined
```

`--list` names the languages each one covers, and the generator refuses to write a file
whose ranges do not cover what it claims - the first draft of that table promised Kazakh,
Ukrainian, Romanian and Vietnamese it could not actually write, and only the test caught
it. A language is not always one block: Ukrainian needs a letter that sits well outside
the Cyrillic range, Kazakh sixteen of them, and Vietnamese draws on four separate blocks.

The table is loaded only if the file is there, and costs 16 bytes of memory per
glyph — 1.5 kB for Cyrillic. On a device with no such file it costs nothing.

What can be in it is a property of the writing system rather than a preference. An
alphabet whose letters keep one shape and run left to right needs only its pictures,
and any of those fits. Arabic chooses one of four forms per letter from its
neighbours and runs right to left; the Indic scripts fuse consonants and reorder
vowels; Han needs thousands of glyphs and more than eight pixels of width. Those need
text shaping, which is a different thing from a glyph table and does not fit here.

Hebrew is left out for the same reason in a milder form: its glyphs would fit, but the
chat draws a line left to right, so the words would come out in reverse reading order.
A sentence shown backwards is worse than one shown as dots with a note saying so.

Typing is a separate matter: the on-screen keyboard is the firmware's own and offers
ASCII. Text in another alphabet can be received, stored, shown and passed on, but not
composed on the device.

## Files on the SD card

| path | what |
|---|---|
| `/LOGS/mesh_c<N>.txt` | chat history, one file per channel |
| `/LOGS/mesh_d<nodeid>.txt` | one per private thread |
| `/LOGS/mesh_dms.txt` | known nodes, names and public keys |
| `/LOGS/mesh_rx.csv` | packet log, when enabled in Settings → Chat |

The packet log is one row per received frame, from the plaintext header — so packets on
channels you cannot read are logged as faithfully as your own:

```
time,from,to,id,hops,hoplimit,rssi,snr,len,chan,colour
2026-01-31 09:12:04,A1B2C3D4,FFFFFFFF,7714E80E,0,7,-1,3.8,24,08,2
```

`colour` is the colour the chat paints that node's lines with, so a row can be matched to
a stripe on screen.

"Clear history" deletes the conversation files for channels and for nodes currently in the
node list. It deliberately leaves the key file and the packet log alone, and it cannot
reach threads with nodes long since forgotten — delete those from the card by hand.

## Keeping quiet

Three separate controls, and they do different things:

| control | where | effect |
|---|---|---|
| `RX` / `TX` lock | tap the letters in the chat's status row | a locked direction is absolute: `RX` sends nothing at all, `TX` acts on nothing received. Shown inverted, white on dark, so it does not read as the usual activity blink |
| Role `Client Mute` | Settings, Profile, Role | heard packets are not repeated for others |
| No beacons or replies | Settings, Privacy | this node stops announcing itself and stops answering requests |

One interaction is worth knowing before it costs an evening: a node's public key
travels in its NodeInfo and nowhere else, so silencing beacons stops peers ever learning
it, and encrypted private messages then fail in both directions - refused, with a reason
that names the key rather than the setting that caused it. The app says so when the
setting is turned on with encryption enabled.

The distinction matters if a peer keeps showing your messages as delivered while you
believe you are silent. Meshtastic treats hearing its own packet flooded back as an
acknowledgement, so a node that still relays will confirm delivery for everyone around
it whatever else it has been told to keep to itself. Relaying is the role's business.

## Store and forward

The app can keep the last few text messages it hears and hand them back later, which is
what a node does for neighbours that were out of range at the time.

Ask for them the way Meshtastic itself does: **send a direct message containing just
`SF`**. That is the documented Android gesture for requesting history from a store and
forward server, and it works here from any node - including a phone.

Two differences from a stock server, both deliberate:

- a real server answers with `STORE_FORWARD_APP` packets that the phone app renders as a
  history view; this one answers with ordinary text lines prefixed `[SF]`, which show up
  as messages;
- a real server refuses history requests on the public channel and expects a private one.
  This app answers anywhere, which suits a handheld better.

What it keeps: broadcasts, and messages addressed to somebody else that happened to pass
within earshot. **Not** messages addressed to us - we are their destination, not a
waypoint, and nobody could ever be handed them back. What it hands over on request:
broadcasts from the channel the question arrived on, and messages addressed to whoever
asked. Never somebody else's private message.

Size and lifetime are in Settings > Advanced. Both cost memory - see below.

## Memory

Worth knowing, because it is the one thing in this app that can stop it dead.

The app takes about 16.7 kB of the M0's core memory when it starts, and the on-screen
keyboard wants **4848 bytes in one contiguous piece** when you first open it in a session.
Between those, roughly two kilobytes are left. Core memory here is handed out and never
returned: a freed block goes on a free list and is reused only for a request of a similar
size, so opening a screen for the first time in a session takes a fresh bite.

The failure looks like a red `Out of Memory` panic, and the screen now reports both
numbers - what was asked for and what was free - so the cause is visible rather than
guessed at.

Two settings trade comfort for that margin, and both default to the safe side:

| setting | where | default | cost |
|---|---|---|---|
| chat scrollback | Settings > Chat > Lines | 20 lines | ~40 bytes a line |
| store-and-forward buffer | Settings > Advanced > buffer | 8 messages | ~50 bytes a message plus its text |

Raise them if your traffic needs it and you do not mind running closer to the edge. The
node list is fixed at ten for the same reason.

## Known limitations

- **LONG_SLOW (SF12) receives nothing.** It transmits fine and stock nodes decode it. The
  receiver is off for that preset: at 4096 samples per symbol one demodulation takes a
  whole symbol time, and the sync stage needs three of them, by which point the sample
  window has moved past the header. The algorithm itself is proven offline against
  recorded air; what is missing is a cheaper way to resolve the half-symbol ambiguity.
- **10 nodes.** Beyond that the oldest is evicted.
- **Custom-channel passphrases do not match stock Meshtastic** — see above.
- **Memory is tight** - see the section above. Wandering through screens not yet opened
  in this session can exhaust the margin; the panic screen reports the size that could
  not be served and what was free.
- **Uncalibrated signal readings.** RSSI is monotone and comparable between nodes but not
  in real dBm, and it saturates when the other node is on the same desk.
- **Half duplex.** Transmitting unloads the receiver, so the app holds off transmitting
  while a packet is arriving.

## Troubleshooting

**Half the applications are missing from the menu.** The card does not have the matching
`APPS` and `BASEBAND` folders. Copy all three folders from the same build.

**Nothing is received.** Region and preset must match the other node — both, exactly. Then
check `Freq` is on `Auto (Region)`: a leftover manual frequency overrides everything.

**Received, but the chat stays empty.** A `* hash XX!=YY` line means the packet arrived
intact but belongs to a channel the app is not on. Usually the primary channel's name
differs: a configured Meshtastic node keeps an explicit name, and both the frequency slot
and the channel hash follow that name rather than the preset.

**A message shows the yellow question mark.** It was sent and nothing came back. On a
broadcast with no neighbours in range, that is the expected outcome.
