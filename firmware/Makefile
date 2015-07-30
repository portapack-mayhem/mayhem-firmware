#
# Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
#
# This file is part of PortaPack.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.
#

PATH_BOOTSTRAP=bootstrap
PATH_APP_M0=application
PATH_APP_M4=baseband

PAD_BOOTSTRAP=0x10000
PAD_M4=0x10000

TARGET=portapack-h1-firmware

TARGET_BOOTSTRAP=$(PATH_BOOTSTRAP)/bootstrap
TARGET_M0=$(PATH_APP_M0)/build/application
TARGET_M4=$(PATH_APP_M4)/build/baseband

DFU_HACKRF=hackrf_one_usb_ram.dfu
LICENSE=../LICENSE

REVISION=$(shell git log -n 1 --format=%h)

CP=arm-none-eabi-objcopy

all: $(TARGET).bin

release: $(TARGET).bin $(DFU_HACKRF) $(LICENSE)
	# TODO: Bad hack to fix location of LICENSE file for tar.
	cp $(LICENSE) LICENSE
	tar -c -j -f $(TARGET)-$(REVISION).tar.bz2 $(TARGET).bin $(DFU_HACKRF) LICENSE
	zip -9 -q $(TARGET)-$(REVISION).zip $(TARGET).bin $(DFU_HACKRF) LICENSE
	rm -f LICENSE

program: $(TARGET).bin
	dfu-util --device 1fc9 --download hackrf_one_usb_ram.dfu --reset
	sleep 1s
	hackrf_spiflash -w $(TARGET).bin

$(TARGET).bin: $(TARGET_BOOTSTRAP)_pad.bin $(TARGET_M4)_pad.bin $(TARGET_M0).bin
	cat $(TARGET_BOOTSTRAP)_pad.bin $(TARGET_M4)_pad.bin $(TARGET_M0).bin >$(TARGET).bin

$(TARGET_BOOTSTRAP)_pad.bin: $(TARGET_BOOTSTRAP).elf
	$(CP) -O binary --pad-to $(PAD_BOOTSTRAP) $(TARGET_BOOTSTRAP).elf $(TARGET_BOOTSTRAP)_pad.bin

$(TARGET_M4)_pad.bin: $(TARGET_M4).elf
	$(CP) -O binary --pad-to $(PAD_M4) $(TARGET_M4).elf $(TARGET_M4)_pad.bin

$(TARGET_M0).bin: $(TARGET_M0).elf
	$(CP) -O binary $(TARGET_M0).elf $(TARGET_M0).bin

$(TARGET_M4).elf: always_check
	@$(MAKE) -s -C $(PATH_APP_M4)

$(TARGET_M0).elf: always_check
	@$(MAKE) -s -C $(PATH_APP_M0)

$(TARGET_BOOTSTRAP).elf: always_check
	@$(MAKE) -s -C $(PATH_BOOTSTRAP)

clean:
	rm -f $(TARGET).bin
	rm -f $(TARGET_BOOTSTRAP)_pad.bin
	rm -f $(TARGET_M4)_pad.bin
	rm -f $(TARGET_M0).bin
	$(MAKE) -C $(PATH_APP_M4) clean
	$(MAKE) -C $(PATH_APP_M0) clean
	$(MAKE) -C $(PATH_BOOTSTRAP) clean

always_check:
	@true
