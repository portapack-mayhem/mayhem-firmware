# portapack-HAVOC

HAVOC is a firmware for the PortaPack H1, a portability add-on for the [HackRF One software-defined radio](http://greatscottgadgets.com/hackrf/).

As its name implies, HAVOC is a firmware aimed towards serious fun, mainly with French systems (for now).
Don't use it, we never did.

Fork features:
* RDS (Radio Data System) basic group forming and transmit (PSK)
* LCR (Language de Commande Routier) basic message forming and transmit
* Xylos transmitter, urban lighting control (NFM CCIR tones)
* Fully configurable AFSK transmit
* Fully configurable Jammer
* "Play Dead" in case of emergency
* Dynamic baseband code loading (see [wiki](https://github.com/furrtek/portapack-havoc/wiki))

Todo:
* Rolling-code jam and replay trick
* Make SIGFOX interceptor work with all modules
* AFSK receiver
* Moltonel (tone detector/scanner)
* Whistle (tone generator)
* Wireless microphone transmit
* CTCSS talkie transmit
* Play wave files from µSD
* EPAR transmit (old Xylos)
* Detect/decode/play/replay OOK from PT2262 and friends (doorbells, cheap remotes... )

Visit the [wiki](https://github.com/furrtek/portapack-havoc/wiki) for more details.

Hardware is available at [ShareBrained Technology](http://sharebrained.com/portapack).

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

Jared Boone <jared@sharebrained.com>

ShareBrained Technology, Inc.

<http://www.sharebrained.com/>

The latest version of this repository can be found at
https://github.com/sharebrained/portapack-hackrf/
