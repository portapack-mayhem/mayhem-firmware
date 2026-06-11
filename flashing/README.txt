Run mayhem_flasher.bat from anywhere (the script switches to the flashing folder automatically).

You need the bundled tools and DFU/firmware binaries next to this script:

  flashing\utils\     dfu-util-static.exe, hackrf_spiflash.exe, ...
  flashing\firmware\  hackrf_usb.dfu (HackRF One), hackrf_hpro_usb.dfu (HackRF Pro),
                      firmware_hackrf.bin, firmware_portarf.bin, firmware_hpro.bin

Those folders are NOT in the git repo. Get them by either:

  A) Download the latest Mayhem release package from https://release.hackrf.app/ (or the
     GitHub release assets) and extract so the above paths exist under flashing\, or

  B) After a local build, copy from your build tree, for example:
       build\hackrf\firmware\hackrf_usb\hackrf_usb.dfu  ->  flashing\firmware\hackrf_usb.dfu
     and use a release zip for utils\ and the Mayhem .bin files if you do not have them.

Read https://github.com/portapack-mayhem/mayhem-firmware/wiki/Update-firmware for more detail.

PowerShell: use .\flashing\mayhem_flasher.bat  (leading .\ ) — do not run .flashing\...

SD card (Flash Utility): put packages under FIRMWARE\ at the root of the FAT32 card — not on the card root.
  Example:  X:\FIRMWARE\portapack-mayhem_OCI.ppfw.tar
  Meteor LRPT needs matching firmware (PMLR in SPI) plus apps from the same build; prefer the single .ppfw.tar.

Stable reproducible build (Docker): see scripts\stable-meteor-docker.ps1 or scripts/stable-meteor-docker.sh
  One-liner after docker build:  docker run --rm -v "%CD%:/havoc" portapack-dev make -j4
  If compiles fail with "Cannot allocate memory", use -j2 or increase Docker Desktop memory.

Recovery from a bad flash: DFU mode + mayhem_flasher.bat / dfu-util, then flash a known-good official .bin before retrying experimental builds.

H4M + Meteor (HackRF One + PortaPack H4M, not Pro): see scripts\H4M_METEOR_BUILD.txt and scripts\flash-h4m-share-out.ps1 for share-out artifacts.
