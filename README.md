![HAVOC banner](doc/banner.png)

HAVOC is an **unofficial** fork of the PortaPack H1 firmware, a portability add-on for the [HackRF One software-defined radio](http://greatscottgadgets.com/hackrf/).

Hardware is available at [ShareBrained Technology](http://sharebrained.com/portapack).

It is build on top of [ShareBrained's firmware](https://github.com/sharebrained/portapack-hackrf/), meaning most of the original functionality remains the same.

# Documentation & finding help

![Helpful note](doc/helpful.png)

Please RTFM before asking for help:
* [Havoc wiki](https://github.com/furrtek/portapack-havoc/wiki)
* [PortaPack wiki](https://github.com/sharebrained/portapack-hackrf/wiki)
* [Some questions and answers](https://github.com/furrtek/portapack-havoc/issues)
* [Facebook group](https://www.facebook.com/groups/177623356165819/) if that's your thing
* And probably a bunch of posts on a variety of forums...

If you want to submit a bug report, use this page: https://github.com/furrtek/portapack-havoc/issues. Check if it hasn't been already posted, there's a search function. Also check the progress list below. Tickets which aren't related to the firmware itself, or the original HackRF and Portapack H1 will be closed.

# Summary

As its name implies, HAVOC's functions can be fun, mean or even useful sometimes. You probably shouldn't use them. No ! Bad ! Put it down.

**In most countries, radio transmissions are tightly regulated. Transmitting outside of free/public bands without a licence or authorization, even at very low power, is certainly forbidden where you live. Always bear that in mind. You're the ONLY ONE responsible for what you do with this software.**

# Fork features

* IQ file replay
* Microphone FM transmit with CTCSS
* CTCSS decoder
* Frequency manager (save & load from SD card, with categories and notes)
* File manager
* "Soundboard" wave file player (put 8-bit mono files in SD card /wav directory)
* ADS-B receiver with map view
* ADS-B transmitter (aircraft spoof)
* SSTV transmitter
* Fully configurable jammer
* POCSAG transmitter
* POCSAG receiver/decoder
* Morse transmitter (FM tone and CW)
* OOK transmitter for common remote encoders (PT2262, doorbells, remote outlets, some garage doors, ...)
* RDS (Radio Data System) PSN, RadioText and Time groups transmitter
* Meteorological radiosonde receiver (M10, M2K2, ...)
* AFSK receiver
* AFSK transmitter (Bell202, ...)
* Nuoptix DTMF sync transmitter (quite specific but can be useful in some theme parks :) )
* TouchTunes jukebox universal remote (by Notpike)
* LCR (Language de Commande Routier) message generator
* Street lighting control transmitter (CCIR tones)
* "Play Dead" in case of emergency
* Fully configurable RF signal generator
* RSSI audio output as pitch (for direction finding)

# Progress

Feature | Progress | Notes
------- | -------- | -----
POCSAG RX   | 95% | Needs support for numeric messages
Morse TX    | 95% | Needs fox hunt scheduler and live keying mode
Mic. TX     | 95% | Carrier leak bug, need to find guard tones for various brands of wireless mics
ADS-B RX    | 90% | Needs angle and speed decoding
Close-Call™ | 85% | Needs adjustments and optimization for wider frequency range
ADS-B TX    | 85% | Works but baseband module needs cleaning
SSTV TX     | 80% | Needs better bitmap file handling, support for other modes (ROBOT ?) and callsign FSK ID
Radiosondes | 75% | Needs support for other models
Wave visualizer | 70% | Needs cleaning and handling of other sample formats, high priority
AFSK RX     | 70% | Needs work regarding flexibility
Sigfox RX   | 40% | Tuning basics done, needs decoding code and testing
Generic TXs | 30% | Raw AX.25, AFSK, FSK, CCIR, DTMF... Tonesets are ready
CC1101 TRX  | 10% | And other sub-GHz transceiver chips like SI4032...
SSTV RX     | 0%  | 
Scanner     | 0%  | Easy, could be used with POCSAG RX to catch jumping channels
SSB TX      | 0%  | Requested but math is hard :(
OOK RX      | 0%  | See if rtl_433's author is fine with using protocol defs
Analog TV TX| 0%  | Enough CPU ? B&W and no sound ?
LoJack RX   | 0%  | Basically AFSK RX
DMR info RX | 0%  | Retrieve DMR channel info. **No voice** because of vocoder complexity and possible legal issue
Tetra info? | 0%  | Same

# Screenshots

![HAVOC screenshots](doc/screenshots.png)

# Thanks

* Sig and cLx for research on AFSK LCR, Xylos, and for lending remote-controlled outlets
* Pyr3x, Rainer Matla and DC1RDB for the donations :)
* Keld Norman and Giorgio Campiotti for ideas and suggestions
* In general, people who help making it better instead of asking already answered questions

# License

Except where specified in subdirectories of this project, all work is offered under the following license:

Copyright (C) 2013-2019 Jared Boone, ShareBrained Technology, Inc.

Copyright (C) 2015-2016 Furrtek

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
02110-1301, USA.

# Contact

## Original firmware and hardware

Jared Boone <jared@sharebrained.com>

ShareBrained Technology, Inc.

<http://www.sharebrained.com/>

The latest version of this repository can be found at
https://github.com/sharebrained/portapack-hackrf/

## HAVOC specific things

Warning: won't reply to questions about flashing and compiling. See links above.

Furrtek <furrtek@gmail.com>

<http://www.furrtek.org>
