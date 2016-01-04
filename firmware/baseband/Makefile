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

##############################################################################
# Build global options
# NOTE: Can be overridden externally.
#

# Compiler options here.
ifeq ($(USE_OPT),)
  USE_OPT = -mthumb \
            -O3 -ggdb3 \
            -ffunction-sections \
            -fdata-sections \
            -fno-builtin \
            -falign-functions=16 \
            -fno-math-errno \
            --specs=nano.specs
            #-fomit-frame-pointer
endif

# C specific options here (added to USE_OPT).
ifeq ($(USE_COPT),)
  USE_COPT = -std=gnu99
endif

# C++ specific options here (added to USE_OPT).
ifeq ($(USE_CPPOPT),)
  USE_CPPOPT = -std=c++11 -fno-rtti -fno-exceptions
endif

# Enable this if you want the linker to remove unused code and data
ifeq ($(USE_LINK_GC),)
  USE_LINK_GC = yes
endif

# Linker extra options here.
ifeq ($(USE_LDOPT),)
  USE_LDOPT =
endif

# Enable this if you want link time optimizations (LTO)
ifeq ($(USE_LTO),)
  USE_LTO = no
endif

# If enabled, this option allows to compile the application in THUMB mode.
ifeq ($(USE_THUMB),)
  USE_THUMB = yes
endif

# Enable this if you want to see the full log while compiling.
ifeq ($(USE_VERBOSE_COMPILE),)
  USE_VERBOSE_COMPILE = no
endif

#
# Build global options
##############################################################################

##############################################################################
# Architecture or project specific options
#

# Enables the use of FPU on Cortex-M4 (no, softfp, hard).
ifeq ($(USE_FPU),)
  USE_FPU = hard
endif

#
# Architecture or project specific options
##############################################################################

##############################################################################
# Project, sources and paths
#

# Define project name here
PROJECT = baseband

# Imported source files and paths
CHIBIOS = ../chibios
CHIBIOS_PORTAPACK = ../chibios-portapack
include $(CHIBIOS_PORTAPACK)/boards/GSG_HACKRF_ONE/board.mk
include $(CHIBIOS_PORTAPACK)/os/hal/platforms/LPC43xx_M4/platform.mk
include $(CHIBIOS)/os/hal/hal.mk
include $(CHIBIOS_PORTAPACK)/os/ports/GCC/ARMCMx/LPC43xx_M4/port.mk
include $(CHIBIOS)/os/kernel/kernel.mk

include $(CHIBIOS)/test/test.mk

# Define linker script file here
LDSCRIPT= $(PORTLD)/LPC43xx_M4.ld

# C sources that can be compiled in ARM or THUMB mode depending on the global
# setting.
CSRC = $(PORTSRC) \
       $(KERNSRC) \
       $(TESTSRC) \
       $(HALSRC) \
       $(PLATFORMSRC) \
       $(BOARDSRC)


# C++ sources that can be compiled in ARM or THUMB mode depending on the global
# setting.
CPPSRC = main.cpp \
         message_queue.cpp \
         event_m4.cpp \
         gpdma.cpp \
         baseband_dma.cpp \
         portapack_shared_memory.cpp \
         baseband_thread.cpp \
         baseband_processor.cpp \
         dsp_decimate.cpp \
         dsp_demodulate.cpp \
         matched_filter.cpp \
         proc_am_audio.cpp \
         proc_nfm_audio.cpp \
         spectrum_collector.cpp \
         proc_wfm_audio.cpp \
         proc_ais.cpp \
         proc_wideband_spectrum.cpp \
         proc_tpms.cpp \
         proc_ert.cpp \
         dsp_squelch.cpp \
         clock_recovery.cpp \
         packet_builder.cpp \
         dsp_fft.cpp \
         dsp_fir_taps.cpp \
         dsp_iir.cpp \
         fxpt_atan2.cpp \
         rssi.cpp \
         rssi_dma.cpp \
         rssi_thread.cpp \
         audio.cpp \
         audio_dma.cpp \
         audio_stats_collector.cpp \
         touch_dma.cpp \
         ../common/utility.cpp \
         ../common/chibios_cpp.cpp \
         ../common/debug.cpp \
         ../common/gcc.cpp

# C sources to be compiled in ARM mode regardless of the global setting.
# NOTE: Mixing ARM and THUMB mode enables the -mthumb-interwork compiler
#       option that results in lower performance and larger code size.
ACSRC =

# C++ sources to be compiled in ARM mode regardless of the global setting.
# NOTE: Mixing ARM and THUMB mode enables the -mthumb-interwork compiler
#       option that results in lower performance and larger code size.
ACPPSRC =

# C sources to be compiled in THUMB mode regardless of the global setting.
# NOTE: Mixing ARM and THUMB mode enables the -mthumb-interwork compiler
#       option that results in lower performance and larger code size.
TCSRC =

# C sources to be compiled in THUMB mode regardless of the global setting.
# NOTE: Mixing ARM and THUMB mode enables the -mthumb-interwork compiler
#       option that results in lower performance and larger code size.
TCPPSRC =

# List ASM source files here
ASMSRC = $(PORTASM)

INCDIR = ../common $(PORTINC) $(KERNINC) $(TESTINC) \
         $(HALINC) $(PLATFORMINC) $(BOARDINC) \
         $(CHIBIOS)/os/various

#
# Project, sources and paths
##############################################################################

##############################################################################
# Compiler settings
#

MCU  = cortex-m4

#TRGT = arm-elf-
TRGT = arm-none-eabi-
CC   = $(TRGT)gcc
CPPC = $(TRGT)g++
# Enable loading with g++ only if you need C++ runtime support.
# NOTE: You can use C++ even without C++ support if you are careful. C++
#       runtime support makes code size explode.
#LD   = $(TRGT)gcc
LD   = $(TRGT)g++
CP   = $(TRGT)objcopy
AS   = $(TRGT)gcc -x assembler-with-cpp
OD   = $(TRGT)objdump
SZ   = $(TRGT)size
HEX  = $(CP) -O ihex
BIN  = $(CP) -O binary

# ARM-specific options here
AOPT =

# THUMB-specific options here
TOPT = -mthumb -DTHUMB

# Define C warning options here
CWARN = -Wall -Wextra -Wstrict-prototypes

# Define C++ warning options here
CPPWARN = -Wall -Wextra

#
# Compiler settings
##############################################################################

##############################################################################
# Start of default section
#

# List all default C defines here, like -D_DEBUG=1
# TODO: Switch -DCRT0_INIT_DATA depending on load from RAM or SPIFI?
# NOTE: _RANDOM_TCC to kill a GCC 4.9.3 error with std::max argument types
DDEFS = -DLPC43XX -DLPC43XX_M4 -D__NEWLIB__ -DHACKRF_ONE \
        -DTOOLCHAIN_GCC -DTOOLCHAIN_GCC_ARM -D_RANDOM_TCC=0 \
        -DGIT_REVISION=\"$(GIT_REVISION)\"

# List all default ASM defines here, like -D_DEBUG=1
DADEFS =

# List all default directories to look for include files here
DINCDIR =

# List the default directory to look for the libraries here
DLIBDIR =

# List all default libraries here
DLIBS =

#
# End of default section
##############################################################################

##############################################################################
# Start of user section
#

# List all user C define here, like -D_DEBUG=1
UDEFS =

# Define ASM defines here
UADEFS =

# List all user directories here
UINCDIR =

# List the user directory to look for the libraries here
ULIBDIR =

# List all user libraries here
ULIBS =

#
# End of user defines
##############################################################################

RULESPATH = $(CHIBIOS)/os/ports/GCC/ARMCMx
include $(RULESPATH)/rules.mk
