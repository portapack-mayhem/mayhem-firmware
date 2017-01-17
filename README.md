# portapack-HAVOC

HAVOC is a fork of the PortaPack H1 firmware, a portability add-on for the [HackRF One software-defined radio](http://greatscottgadgets.com/hackrf/).

Hardware is available at [ShareBrained Technology](http://sharebrained.com/portapack).

It is build on top of [ShareBrained's firmware](https://github.com/sharebrained/portapack-hackrf/), meaning that the original functionalities are kept (except when I don't sync for 2 months).

As its name implies, HAVOC's functionalities can be fun (or mean). You shouldn't use them. We never did.

**In most countries, radio transmissions are tightly regulated. Transmitting outside of free/public bands without a licence or authorization, even at very low power, is certainly forbidden where you live. Always bear that in mind. You're the ONLY ONE responsible for what you do with this software.**

# Fork features

* Frequency manager (save/load from SD card)
* "Soundboard" wave file player from SD card (put 8-bit mono files in /wav directory)
* POCSAG 512/1200/2400 alphanumeric receiver/decoder
* Simulate OOK frames from PT2262 encoders and many others (doorbells, remote outlets, some garage doors, ...)
* Scheduled numbers station (for spies on a budget, alerts, LARP scenarios, fox hunts, ...)
* Nuoptix DTMF sync transmit (quite specific but can be useful in some theme parks :) )
* RDS (Radio Data System) PSN and RadioText groups transmit
* Fully configurable AFSK transmit (Bell202...)
* Virtual keyboard or Unistroke "handwriting" text input
* LCR (Language de Commande Routier) litteral message forming and transmit
* "Xy" and "EP" transmitter, street lighting control (CCIR tones)
* "Play Dead" in case of emergency
* PWM audio RSSI output (for crude direction finding)

# Progress

Feature | Progress | Notes
------- | ------ | -----
Jammer      | 95% | Need to make the different noise modes work
RDS TX      | 90% | Needs testing for Radiotext and audio mux function
POCSAG RX   | 90% | Still misses some messages
Close-Call™ | 70% | Needs adjustments and optimization for wider frequency range
EPAR TX     | 60% | Older Xy, low priority
Sigfox RX   | 40% | Left aside, needs updates and testing
Morse TX    | 30% | Needs UI and testing
Wave visualizer | 30% | High priority
ADS-B TX    | 20% | UI and frame gen stuff done
Whistle     | 20% | Easy
IQ replay   | 10% | High priority
ADS-B RX    | 0%  | Could be fun
Search      | 0%  | Will be a special Close-Call mode
Scanner     | 0%  | Easy
Generic TXs | 0%  | Raw AX.25, AFSK, FSK, CCIR, DTMF...
PMR446 TX   | 0%  | Channel list, mic. in, PTT, CTCSS
OOK RX      | 0%  | Mainly for 433MHz remotes
POCSAG TX   | 0%  | Shouldn't be too tough, a real pager for testing would be nice
AFSK RX     | 0%  | Shouldn't be too tough
Analog TV TX| 0%  | Could be fun
Mic. TX     | 0%  | Need to find guard tones for various brands of wireless mics
Painter     | 0%  | Spectrum painter, low priority

# Screenshots

![HAVOC screenshots](screenshots.png)

**Visit the [wiki](https://github.com/furrtek/portapack-havoc/wiki) for more details.**

## PWM RSSI output

Huge kludge, wrote in a rush for direction finding. Audio frequency is 800Hz in NFM and 500Hz in WFM. The data path is very messy, the RSSI thread running on the baseband core sends groups of values to the application RSSI widget which computes the average value, which sends it back to the baseband module for audio output.

## Getting out of Play Dead mode

Play Dead mode is a persistent fake error screen, which can be useful in case of situations where people become too curious. It's easily enabled, and disabled with a programmable key sequence (see Setup menu).

If no exit key sequence was set up, it defaults to up-down-left-right. Enter the sequence and press select, you should see "Please reset" appear. After reset, you should be able to go back to the main menu. If not, a sequence was probably entered already. Solution hint: remove the backup battery for a few minutes, or ui_navigation.cpp ctrl+F 0x8D1 ;)

This mode can also be used as a login screen, not requiring reset.

# Thanks

* Sig and cLx for research on AFSK LCR, Xylos, and for lending remote-controlled outlets
* Rainer Matla for the donation :)
* Keld Norman for ideas and suggestions

# License

Except where specified in subdirectories of this project, all work is offered under the following license:

Copyright (C) 2013-2015 Jared Boone, ShareBrained Technology, Inc.

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

Furrtek <furrtek@gmail.com>

<http://www.furrtek.org>
