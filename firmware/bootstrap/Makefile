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

PATH_COMMON=../common
PATH_APP_M0=../application
TARGET_M0=$(PATH_APP_M0)/build/application
APP_M0_SRC=$(PATH_APP_M0)/*.cpp $(PATH_APP_M0)/*.hpp $(PATH_COMMON)/*.cpp $(PATH_COMMON)/*.hpp

PATH_APP_M4=../baseband
TARGET_M4=$(PATH_APP_M4)/build/baseband
APP_M4_SRC=$(PATH_APP_M4)/*.cpp $(PATH_APP_M4)/*.hpp $(PATH_COMMON)/*.cpp $(PATH_COMMON)/*.hpp

TARGET=bootstrap

CC=arm-none-eabi-gcc
LD=arm-none-eabi-gcc
CP=arm-none-eabi-objcopy
OBJDUMP=arm-none-eabi-objdump

CHIBIOS_PORTAPACK=../chibios-portapack
CHIBIOS=../chibios
INCLUDE=-I $(CHIBIOS)/os/ports/common/ARMCMx/CMSIS/include \
        -I $(CHIBIOS)/os/ports/common/ARMCMx \
        -I $(CHIBIOS_PORTAPACK)/os/hal/platforms/LPC43xx \
        -I $(CHIBIOS_PORTAPACK)/os/hal/platforms/LPC43xx_M4
MCPU=cortex-m4
CPUFLAGS=-mcpu=$(MCPU) -mthumb -mno-thumb-interwork -DTHUMB -DTHUMB_PRESENT -DTHUMB_NO_INTERWORKING

COPT=-std=gnu99 \
	 -Wall -Wextra -Wstrict-prototypes \
	 $(CPUFLAGS) \
	 -DLPC43XX -DLPC43XX_M4 \
	 -Os \
	 -ffunction-sections \
	 -fdata-sections \
	 -fno-builtin --specs=nano.specs

LDOPT=-nostartfiles \
	  $(CPUFLAGS) \
	  -D__START=main \
	  -Wl,-Map=$(TARGET).map,--cref,--no-warn-mismatch,--library-path=.,--script=m4.ld,--gc-sections

all: image.bin

list: $(TARGET).lst $(TARGET_M4).lst $(TARGET_M0).lst

program: image.bin
	dfu-util --device 1fc9 --download hackrf_one_usb_ram.dfu --reset
	sleep 1s
	hackrf_spiflash -w image.bin

image.bin: $(TARGET).elf $(TARGET_M4).elf $(TARGET_M0).elf
	$(CP) -O binary --pad-to 0x10000 $(TARGET).elf $(TARGET).bin
	$(CP) -O binary --pad-to 0x10000 $(TARGET_M4).elf m4.bin
	$(CP) -O binary $(TARGET_M0).elf m0.bin
	cat $(TARGET).bin m4.bin m0.bin >image.bin

$(TARGET).lst: $(TARGET).elf
	$(OBJDUMP) -S $(TARGET).elf >$(TARGET).lst

$(TARGET).elf: $(TARGET).o startup_ARMCM4.S
	$(LD) $(LDOPT) $(LIB) -o $(TARGET).elf $(TARGET).o startup_ARMCM4.S

$(TARGET).o: $(TARGET).c
	$(CC) $(COPT) $(INCLUDE) -c -o $(TARGET).o $(TARGET).c

$(TARGET_M4).lst: $(TARGET_M4).elf
	$(OBJDUMP) -S $(TARGET_M4).elf >$(TARGET_M4).lst

$(TARGET_M4).elf: $(PATH_APP_M4)/Makefile $(APP_M4_SRC)
	$(MAKE) -C $(PATH_APP_M4) -f Makefile

$(TARGET_M0).lst: $(TARGET_M0).elf
	$(OBJDUMP) -S $(TARGET_M0).elf >$(TARGET_M0).lst

$(TARGET_M0).elf: $(PATH_APP_M0)/Makefile $(APP_M0_SRC)
	$(MAKE) -C $(PATH_APP_M0) -f Makefile

clean:
	rm -f image.bin m0.bin m4.bin
	rm -f $(TARGET).o $(TARGET).elf $(TARGET).bin $(TARGET).lst $(TARGET).map
	$(MAKE) -C $(PATH_APP_M4) -f Makefile clean
	rm -f $(TARGET_M4).lst
	$(MAKE) -C $(PATH_APP_M0) -f Makefile clean
	rm -f $(TARGET_M0).lst
