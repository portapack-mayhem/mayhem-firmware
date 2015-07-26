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

all: $(TARGET).elf

$(TARGET).lst: $(TARGET).elf
	$(OBJDUMP) -S $(TARGET).elf >$(TARGET).lst

$(TARGET).elf: $(TARGET).o startup_ARMCM4.S
	$(LD) $(LDOPT) $(LIB) -o $(TARGET).elf $(TARGET).o startup_ARMCM4.S

$(TARGET).o: $(TARGET).c
	$(CC) $(COPT) $(INCLUDE) -c -o $(TARGET).o $(TARGET).c

clean:
	rm -f $(TARGET).o $(TARGET).elf $(TARGET).bin $(TARGET).lst $(TARGET).map
