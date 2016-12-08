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

enable_language(C CXX ASM)

project(baseband_shared)

# Compiler options here.
set(USE_OPT "-O3 -g -falign-functions=16 -fno-math-errno --specs=nano.specs")

# C specific options here (added to USE_OPT).
set(USE_COPT "-std=gnu99")

# C++ specific options here (added to USE_OPT).
set(USE_CPPOPT "-std=c++11 -fno-rtti -fno-exceptions")

# Enable this if you want the linker to remove unused code and data
set(USE_LINK_GC yes)

# Linker extra options here.
set(USE_LDOPT)

# Enable this if you want link time optimizations (LTO)
set(USE_LTO no)

# If enabled, this option allows to compile the application in THUMB mode.
set(USE_THUMB yes)

# Enable this if you want to see the full log while compiling.
set(USE_VERBOSE_COMPILE no)

#
# Build global options
##############################################################################

##############################################################################
# Architecture or project specific options
#

# Enables the use of FPU on Cortex-M4 (no, softfp, hard).
set(USE_FPU hard)

#
# Architecture or project specific options
##############################################################################

##############################################################################
# Project, sources and paths
#

# Imported source files and paths
include(${CHIBIOS_PORTAPACK}/boards/GSG_HACKRF_ONE/board.cmake)
include(${CHIBIOS_PORTAPACK}/os/hal/platforms/LPC43xx_M4/platform.cmake)
include(${CHIBIOS}/os/hal/hal.cmake)
include(${CHIBIOS_PORTAPACK}/os/ports/GCC/ARMCMx/LPC43xx_M4/port.cmake)
include(${CHIBIOS}/os/kernel/kernel.cmake)

include(${CHIBIOS}/test/test.cmake)

# Define linker script file here
set(LDSCRIPT ${PORTLD}/LPC43xx_M4.ld)

# C sources that can be compiled in ARM or THUMB mode depending on the global
# setting.
set(CSRC
	${PORTSRC}
	${KERNSRC}
	${TESTSRC}
	${HALSRC}
	${PLATFORMSRC}
	${BOARDSRC}
)

# C++ sources that can be compiled in ARM or THUMB mode depending on the global
# setting.
set(CPPSRC
	baseband.cpp
	${COMMON}/message_queue.cpp
	${COMMON}/event.cpp
	event_m4.cpp
	${COMMON}/thread_wait.cpp
	${COMMON}/gpdma.cpp
	baseband_dma.cpp
	${COMMON}/baseband_sgpio.cpp
	${COMMON}/portapack_shared_memory.cpp
	baseband_thread.cpp
	baseband_processor.cpp
	baseband_stats_collector.cpp
	dsp_decimate.cpp
	dsp_demodulate.cpp
	matched_filter.cpp
	spectrum_collector.cpp
	stream_input.cpp
	dsp_squelch.cpp
	clock_recovery.cpp
	packet_builder.cpp
	${COMMON}/dsp_fft.cpp
	${COMMON}/dsp_fir_taps.cpp
	${COMMON}/dsp_iir.cpp
	fxpt_atan2.cpp
	rssi.cpp
	rssi_dma.cpp
	rssi_thread.cpp
	audio_compressor.cpp
	audio_output.cpp
	audio_dma.cpp
	audio_stats_collector.cpp
	${COMMON}/utility.cpp
	${COMMON}/chibios_cpp.cpp
	${COMMON}/debug.cpp
	${COMMON}/gcc.cpp
)

# C sources to be compiled in ARM mode regardless of the global setting.
# NOTE: Mixing ARM and THUMB mode enables the -mthumb-interwork compiler
#       option that results in lower performance and larger code size.
set(ACSRC)

# C++ sources to be compiled in ARM mode regardless of the global setting.
# NOTE: Mixing ARM and THUMB mode enables the -mthumb-interwork compiler
#       option that results in lower performance and larger code size.
set(ACPPSRC)

# C sources to be compiled in THUMB mode regardless of the global setting.
# NOTE: Mixing ARM and THUMB mode enables the -mthumb-interwork compiler
#       option that results in lower performance and larger code size.
set(TCSRC)

# C sources to be compiled in THUMB mode regardless of the global setting.
# NOTE: Mixing ARM and THUMB mode enables the -mthumb-interwork compiler
#       option that results in lower performance and larger code size.
set(TCPPSRC)

# List ASM source files here
set(ASMSRC ${PORTASM})

set(INCDIR ${COMMON} ${PORTINC} ${KERNINC} ${TESTINC}
	${HALINC} ${PLATFORMINC} ${BOARDINC}
	${CHIBIOS}/os/various
)

#
# Project, sources and paths
##############################################################################

##############################################################################
# Compiler settings
#

set(MCU cortex-m4)

# ARM-specific options here
set(AOPT)

# THUMB-specific options here
set(TOPT "-mthumb -DTHUMB")

# Define C warning options here
set(CWARN "-Wall -Wextra -Wstrict-prototypes")

# Define C++ warning options here
set(CPPWARN "-Wall -Wextra")

#
# Compiler settings
##############################################################################

##############################################################################
# Start of default section
#

# List all default C defines here, like -D_DEBUG=1
# TODO: Switch -DCRT0_INIT_DATA depending on load from RAM or SPIFI?
# NOTE: _RANDOM_TCC to kill a GCC 4.9.3 error with std::max argument types
set(DDEFS -DLPC43XX -DLPC43XX_M4 -D__NEWLIB__ -DHACKRF_ONE -DTOOLCHAIN_GCC -DTOOLCHAIN_GCC_ARM -D_RANDOM_TCC=0 -DGIT_REVISION=\"${GIT_REVISION}\")

# List all default ASM defines here, like -D_DEBUG=1
set(DADEFS)

# List all default directories to look for include files here
set(DINCDIR)

# List the default directory to look for the libraries here
set(DLIBDIR)

# List all default libraries here
set(DLIBS)

#
# End of default section
##############################################################################

##############################################################################
# Start of user section
#

# List all user C define here, like -D_DEBUG=1
set(UDEFS)

# Define ASM defines here
set(UADEFS)

# List all user directories here
set(UINCDIR)

# List the user directory to look for the libraries here
set(ULIBDIR)

# List all user libraries here
set(ULIBS)

#
# End of user defines
##############################################################################

set(RULESPATH ${CHIBIOS}/os/ports/GCC/ARMCMx)
include(${RULESPATH}/rules.cmake)

#######################################################################

add_library(${PROJECT_NAME} OBJECT ${CSRC} ${CPPSRC} ${ASMSRC})
include_directories(. ${INCDIR})

#######################################################################

set(BASEBAND_IMAGES)

macro(DeclareTargets chunk_tag name)
	project("baseband_${name}")

	include(${RULESPATH}/rules.cmake)
	add_executable(${PROJECT_NAME}.elf $<TARGET_OBJECTS:baseband_shared> ${MODE_CPPSRC})
	set_target_properties(${PROJECT_NAME}.elf PROPERTIES LINK_DEPENDS ${LDSCRIPT})
	add_definitions(${DEFS})
	include_directories(. ${INCDIR})
	link_directories(${LLIBDIR})
	target_link_libraries(${PROJECT_NAME}.elf ${LIBS})

	add_custom_command(
		OUTPUT ${PROJECT_NAME}.bin ${PROJECT_NAME}.img
		COMMAND ${CMAKE_OBJCOPY} -O binary ${PROJECT_NAME}.elf ${PROJECT_NAME}.bin
		COMMAND ${MAKE_IMAGE_CHUNK} ${PROJECT_NAME}.bin ${chunk_tag} ${PROJECT_NAME}.img
		DEPENDS ${PROJECT_NAME}.elf ${MAKE_IMAGE_CHUNK}
		VERBATIM
	)

	set(BASEBAND_IMAGES ${BASEBAND_IMAGES} ${PROJECT_NAME}.img)
endmacro()

### AIS

set(MODE_CPPSRC
	proc_ais.cpp
)
DeclareTargets(PAIS ais)

### AM Audio

set(MODE_CPPSRC
	proc_am_audio.cpp
)
DeclareTargets(PAMA am_audio)

### Capture

set(MODE_CPPSRC
	proc_capture.cpp
)
DeclareTargets(PCAP capture)

### ERT

set(MODE_CPPSRC
	proc_ert.cpp
)
DeclareTargets(PERT ert)

### POCSAG RX

set(MODE_CPPSRC
	proc_pocsag.cpp
)
DeclareTargets(PPOC pocsag)

### NFM Audio

set(MODE_CPPSRC
	proc_nfm_audio.cpp
)
DeclareTargets(PNFM nfm_audio)

### TPMS

set(MODE_CPPSRC
	proc_tpms.cpp
)
DeclareTargets(PTPM tpms)

### WFM Audio

set(MODE_CPPSRC
	proc_wfm_audio.cpp
)
DeclareTargets(PWFM wfm_audio)

### Wideband Spectrum

set(MODE_CPPSRC
	proc_wideband_spectrum.cpp
)
DeclareTargets(PSPE wideband_spectrum)

### Close Call

set(MODE_CPPSRC
	proc_closecall.cpp
)
DeclareTargets(PCLC closecall)

### OOK

set(MODE_CPPSRC
	proc_ook.cpp
)
DeclareTargets(POOK ook)

### Jammer

set(MODE_CPPSRC
	proc_jammer.cpp
)
DeclareTargets(PJAM jammer)

### Audio transmit

set(MODE_CPPSRC
	proc_audiotx.cpp
)
DeclareTargets(PATX audio_tx)

### AFSK

set(MODE_CPPSRC
	proc_afsk.cpp
)
DeclareTargets(PAFS afsk)

### Epar

#set(MODE_CPPSRC
#	proc_epar.cpp
#)
#DeclareTargets(PEPR epar)

### Xylos

set(MODE_CPPSRC
	proc_xylos.cpp
)
DeclareTargets(PXYL xylos)

### DTMF TX

set(MODE_CPPSRC
	proc_dtmf_tx.cpp
)
DeclareTargets(PDTX dtmf_tx)

### RDS

set(MODE_CPPSRC
	proc_rds.cpp
)
DeclareTargets(PRDS rds)

### ADS-B TX

set(MODE_CPPSRC
	proc_adsbtx.cpp
)
DeclareTargets(PADS ads)

### HackRF "factory" firmware

add_custom_command(
	OUTPUT hackrf.bin hackrf.img
	COMMAND ${STRIP_DFU} ${HACKRF_FIRMWARE_IMAGE} hackrf.bin
	COMMAND ${MAKE_IMAGE_CHUNK} hackrf.bin HRF1 hackrf.img
	DEPENDS ${HACKRF_FIRMWARE_IMAGE} ${STRIP_DFU} ${MAKE_IMAGE_CHUNK}
	VERBATIM
)

set(BASEBAND_IMAGES ${BASEBAND_IMAGES} hackrf.img)

### Terminator image

add_custom_command(
	OUTPUT terminator.img
	COMMAND ${MAKE_IMAGE_CHUNK} terminator.img
	DEPENDS ${MAKE_IMAGE_CHUNK}
	VERBATIM
)

set(BASEBAND_IMAGES ${BASEBAND_IMAGES} terminator.img)

#######################################################################

project(baseband)

add_custom_command(
	OUTPUT ${PROJECT_NAME}.img
	COMMAND cat ${BASEBAND_IMAGES} > ${PROJECT_NAME}.img
	DEPENDS ${BASEBAND_IMAGES}
	VERBATIM
)

add_custom_target(
	${PROJECT_NAME}
	DEPENDS ${PROJECT_NAME}.img
)
