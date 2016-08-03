# portapack-HAVOC

HAVOC is a fork of the PortaPack H1 firmware, a portability add-on for the [HackRF One software-defined radio](http://greatscottgadgets.com/hackrf/).

It is build on top of Sharebrained's firmware, meaning that the original functionalities are kept (except when I don't sync for 2 months).

As its name implies, HAVOC is a firmware made to have serious fun, mainly involving French systems (for now). Don't use it, we never did.

**In most countries, radio transmissions are tightly regulated. Transmitting without a licence or authorization, even at very low power, is certainly forbidden where you live. Always bear that in mind. You'll be the only one responsible for what you'll do with this software.**

Fork features:
* (Disabled for now) Close-Call™ style scanner
* PWM audio RSSI output
* Simulate OOK frames from PT2262 encoders and friends (doorbells, remote outlets, ...)
* RDS (Radio Data System) PSN and Radiotext transmit
* Fully configurable AFSK transmit
* LCR (Language de Commande Routier) litteral message forming and transmit
* Xylos transmitter, street lighting control (NFM CCIR tones)
* Virtual keyboard or Unistroke "handwriting" text input
* "Play Dead" in case of emergency
* (Disabled for now) Dynamic baseband code loading from SD card (see [wiki](https://github.com/furrtek/portapack-havoc/wiki))

Todo (highest to lowest priority):
* Fully configurable Jammer (fix)
* Whistle (tone generator)
* Play wave files from µSD
* Wireless microphone transmit
* CTCSS talkie transmit
* Frequency manager
* AFSK receiver
* EPAR transmit (old Xylos)
* POCSAG receiver
* POCSAG transmitter
* Detect/decode OOK
* Make SIGFOX interceptor work with all modules
* Number station
* Spectrum painter
* Rolling-code jam and replay trick

**Visit the [wiki](https://github.com/furrtek/portapack-havoc/wiki) for more details.**

About the PWM RSSI output: Frequency is 800Hz in NFM and 500Hz in WFM. The data path is very messy, the RSSI thread running on the baseband core sends groups of values to the application RSSI widget which computes the average value, which sends it back to the baseband module for audio output (if enabled)...

Hardware is available at [ShareBrained Technology](http://sharebrained.com/portapack).

# Thanks

Sig and cLx for discovery and research on AFSK LCR and Xylos.

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

## HAVOC

Furrtek <furrtek@gmail.com>

<http://www.furrtek.org>

## Original firmware and hardware

Jared Boone <jared@sharebrained.com>

ShareBrained Technology, Inc.

<http://www.sharebrained.com/>

The latest version of this repository can be found at
https://github.com/sharebrained/portapack-hackrf/
