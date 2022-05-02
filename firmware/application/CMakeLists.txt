#
# Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
# Copyright (C) 2016 Furrtek
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

project(application)

# Compiler options here.
set(USE_OPT "-Os -g --specs=nano.specs")

# C specific options here (added to USE_OPT).
set(USE_COPT "-std=gnu99")

# C++ specific options here (added to USE_OPT).
set(USE_CPPOPT "-std=c++17 -fno-rtti -fno-exceptions -Weffc++ -Wuninitialized")

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
set(USE_FPU no)

#
# Architecture or project specific options
##############################################################################

##############################################################################
# Project, sources and paths
#

set(CPLD_20150901_SVF_PATH ${HARDWARE_PATH}/portapack_h1/cpld/20150901/output_files/portapack_h1_cpld.svf)
set(CPLD_20150901_DATA_CPP ${CMAKE_CURRENT_BINARY_DIR}/portapack_cpld_20150901_data.cpp)

set(CPLD_20170522_SVF_PATH ${HARDWARE_PATH}/portapack_h1/cpld/20170522/output_files/portapack_h1_cpld.svf)
set(CPLD_20170522_DATA_CPP ${CMAKE_CURRENT_BINARY_DIR}/portapack_cpld_20170522_data.cpp)

set(HACKRF_CPLD_DATA_HPP ${CMAKE_CURRENT_BINARY_DIR}/hackrf_cpld_data.hpp)
set(HACKRF_CPLD_DATA_CPP ${CMAKE_CURRENT_BINARY_DIR}/hackrf_cpld_data.cpp)

# Imported source files and paths
include(${CHIBIOS_PORTAPACK}/boards/PORTAPACK_APPLICATION/board.cmake)
include(${CHIBIOS_PORTAPACK}/os/hal/platforms/LPC43xx_M0/platform.cmake)
include(${CHIBIOS}/os/hal/hal.cmake)
include(${CHIBIOS_PORTAPACK}/os/ports/GCC/ARMCMx/LPC43xx_M0/port.cmake)
include(${CHIBIOS}/os/kernel/kernel.cmake)
include(${CHIBIOS_PORTAPACK}/os/various/fatfs_bindings/fatfs.cmake)
include(${CHIBIOS}/test/test.cmake)

# Define linker script file here
set(LDSCRIPT ${PORTLD}/LPC43xx_M0.ld)

# C sources that can be compiled in ARM or THUMB mode depending on the global
# setting.
set(CSRC
	${PORTSRC}
	${KERNSRC}
	${TESTSRC}
	${HALSRC}
	${PLATFORMSRC}
	${BOARDSRC}
	${FATFSSRC}
)

# C++ sources that can be compiled in ARM or THUMB mode depending on the global
# setting.
set(CPPSRC
	main.cpp
	${COMMON}/acars_packet.cpp
	${COMMON}/adsb.cpp
	${COMMON}/adsb_frame.cpp
	${COMMON}/ais_baseband.cpp
	${COMMON}/ais_packet.cpp
	${COMMON}/ak4951.cpp
	${COMMON}/backlight.cpp
	${COMMON}/baseband_cpld.cpp
	${COMMON}/bch_code.cpp
	${COMMON}/buffer.cpp
	${COMMON}/buffer_exchange.cpp
	${COMMON}/chibios_cpp.cpp
	${COMMON}/cpld_max5.cpp
	${COMMON}/cpld_update.cpp
	${COMMON}/cpld_xilinx.cpp
	${COMMON}/debug.cpp
	${COMMON}/ert_packet.cpp
	${COMMON}/event.cpp
	${COMMON}/gcc.cpp
	${COMMON}/hackrf_hal.cpp
	${COMMON}/i2c_pp.cpp
	${COMMON}/jtag.cpp
	${COMMON}/jtag_tap.cpp
	${COMMON}/lcd_ili9341.cpp
	${COMMON}/lfsr_random.cpp
	${COMMON}/manchester.cpp
	${COMMON}/message_queue.cpp
	${COMMON}/morse.cpp
	${COMMON}/png_writer.cpp
	${COMMON}/pocsag.cpp
	${COMMON}/pocsag_packet.cpp
	${COMMON}/aprs_packet.cpp
	${COMMON}/portapack_io.cpp
	${COMMON}/portapack_persistent_memory.cpp
	${COMMON}/portapack_shared_memory.cpp
	${COMMON}/sonde_packet.cpp
	# ${COMMON}/test_packet.cpp
	${COMMON}/tpms_packet.cpp
	${COMMON}/ui.cpp
	${COMMON}/ui_focus.cpp
	${COMMON}/ui_painter.cpp
	${COMMON}/ui_text.cpp
	${COMMON}/ui_widget.cpp
	${COMMON}/utility.cpp
	${COMMON}/wm8731.cpp
	app_settings.cpp
	audio.cpp
	baseband_api.cpp
	capture_thread.cpp
	clock_manager.cpp
	core_control.cpp
	database.cpp
	de_bruijn.cpp
	#emu_cc1101.cpp
	rfm69.cpp
	event_m0.cpp
	file.cpp
	freqman.cpp
	io_file.cpp
	io_wave.cpp
	irq_controls.cpp
	irq_lcd_frame.cpp
	irq_rtc.cpp
	log_file.cpp
	portapack.cpp
	qrcodegen.cpp
	radio.cpp
	receiver_model.cpp
	recent_entries.cpp
	replay_thread.cpp
	rf_path.cpp
	rtc_time.cpp
	sd_card.cpp
	serializer.cpp
	spectrum_color_lut.cpp
	string_format.cpp
	temperature_logger.cpp
	touch.cpp
	tone_key.cpp
	transmitter_model.cpp
	tuning.cpp
	hw/debounce.cpp
	hw/encoder.cpp
	hw/max2837.cpp
	hw/max5864.cpp
	hw/rffc507x.cpp
	hw/rffc507x_spi.cpp
	hw/si5351.cpp
	hw/spi_pp.cpp
	hw/touch_adc.cpp
	ui_baseband_stats_view.cpp
	ui_navigation.cpp
	ui_playdead.cpp
	ui_record_view.cpp
	ui_sd_card_status_view.cpp
	ui/ui_alphanum.cpp
	ui/ui_audio.cpp
	ui/ui_channel.cpp
	ui/ui_font_fixed_8x16.cpp
	ui/ui_geomap.cpp
        ui/ui_qrcode.cpp
	ui/ui_menu.cpp
	ui/ui_btngrid.cpp
	ui/ui_receiver.cpp
	ui/ui_rssi.cpp
	ui/ui_tv.cpp
	ui/ui_spectrum.cpp
	ui/ui_tabview.cpp
	ui/ui_textentry.cpp
	ui/ui_transmitter.cpp
	apps/ui_about_simple.cpp
	apps/ui_adsb_rx.cpp
	apps/ui_adsb_tx.cpp
	apps/ui_afsk_rx.cpp
	apps/ui_aprs_rx.cpp
	apps/ui_btle_rx.cpp
	apps/ui_nrf_rx.cpp
	apps/ui_aprs_tx.cpp
	apps/ui_bht_tx.cpp
	apps/ui_coasterp.cpp
	apps/ui_debug.cpp
	apps/ui_encoders.cpp
	apps/ui_fileman.cpp
	apps/ui_freqman.cpp
	apps/ui_jammer.cpp
	apps/ui_keyfob.cpp
	apps/ui_lcr.cpp
	apps/lge_app.cpp
	apps/ui_looking_glass_app.cpp
	apps/ui_mictx.cpp
	apps/ui_modemsetup.cpp
	apps/ui_morse.cpp
	# apps/ui_nuoptix.cpp
	apps/ui_pocsag_tx.cpp
	apps/ui_rds.cpp
	apps/ui_remote.cpp
	apps/ui_scanner.cpp
	apps/ui_search.cpp
	apps/ui_sd_wipe.cpp
	apps/ui_settings.cpp
	apps/ui_siggen.cpp
	apps/ui_sonde.cpp
	apps/ui_sstvtx.cpp
	# apps/ui_test.cpp
	apps/ui_tone_search.cpp
	apps/ui_touch_calibration.cpp
	apps/ui_touchtunes.cpp
	apps/ui_view_wav.cpp
	apps/ui_whipcalc.cpp
	apps/acars_app.cpp
	apps/ais_app.cpp
	apps/analog_audio_app.cpp
	apps/analog_tv_app.cpp
	apps/capture_app.cpp
	apps/ert_app.cpp
	apps/lge_app.cpp
	apps/pocsag_app.cpp
	apps/replay_app.cpp
	apps/gps_sim_app.cpp
	apps/soundboard_app.cpp
	apps/tpms_app.cpp
	protocols/aprs.cpp
	protocols/ax25.cpp
	protocols/bht.cpp
	protocols/dcs.cpp
	protocols/encoders.cpp
	protocols/lcr.cpp
	protocols/modems.cpp
	protocols/rds.cpp
	# ui_handwrite.cpp
	# ui_loadmodule.cpp
	# ui_numbers.cpp
	# ui_replay_view.cpp
	# ui_script.cpp
	ui_sd_card_debug.cpp
	${CPLD_20150901_DATA_CPP}
	${CPLD_20170522_DATA_CPP}
	${HACKRF_CPLD_DATA_CPP}
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

set(INCDIR ${CMAKE_CURRENT_BINARY_DIR} ${COMMON} ${PORTINC} ${KERNINC} ${TESTINC}
	${HALINC} ${PLATFORMINC} ${BOARDINC}
	${FATFSINC}
	${CHIBIOS}/os/various
	ui
	hw
	apps
	protocols
	bitmaps
)

#
# Project, sources and paths
##############################################################################

##############################################################################
# Compiler settings
#

# TODO: Entertain using MCU=cortex-m0.small-multiply for LPC43xx M0 core.
# However, on GCC-ARM-Embedded 4.9 2015q2, it seems to produce non-functional
# binaries.
set(MCU cortex-m0)

# ARM-specific options here
set(AOPT)

# THUMB-specific options here
set(TOPT "-mthumb -DTHUMB")

# Define C warning options here
set(CWARN "-Wall -Wextra -Wstrict-prototypes")

# Define C++ warning options here
set(CPPWARN "-Wall -Wextra -Wno-psabi")

#
# Compiler settings
##############################################################################

##############################################################################
# Start of default section
#

# List all default C defines here, like -D_DEBUG=1
# TODO: Switch -DCRT0_INIT_DATA depending on load from RAM or SPIFI?
# NOTE: _RANDOM_TCC to kill a GCC 4.9.3 error with std::max argument types
set(DDEFS "-DLPC43XX -DLPC43XX_M0 -D__NEWLIB__ -DHACKRF_ONE -DTOOLCHAIN_GCC -DTOOLCHAIN_GCC_ARM -D_RANDOM_TCC=0 -D'VERSION_STRING=\"${VERSION}\"'")

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

##############################################################################

add_custom_command(
	OUTPUT ${CPLD_20150901_DATA_CPP}
	COMMAND ${EXTRACT_CPLD_DATA} ${CPLD_20150901_SVF_PATH} rev_20150901 >${CPLD_20150901_DATA_CPP}
	DEPENDS ${EXTRACT_CPLD_DATA} ${CPLD_20150901_SVF_PATH}
)

add_custom_command(
	OUTPUT ${CPLD_20170522_DATA_CPP}
	COMMAND ${EXTRACT_CPLD_DATA} ${CPLD_20170522_SVF_PATH} rev_20170522 >${CPLD_20170522_DATA_CPP}
	DEPENDS ${EXTRACT_CPLD_DATA} ${CPLD_20170522_SVF_PATH}
)

add_custom_command(
	OUTPUT ${HACKRF_CPLD_DATA_CPP}
	COMMAND ${HACKRF_CPLD_TOOL} --xsvf ${HACKRF_CPLD_XSVF_PATH} --portapack-data ${HACKRF_CPLD_DATA_CPP}
	DEPENDS ${HACKRF_CPLD_TOOL} ${HACKRF_CPLD_XSVF_PATH}
)

add_executable(${PROJECT_NAME}.elf ${CSRC} ${CPPSRC} ${ASMSRC})
set_target_properties(${PROJECT_NAME}.elf PROPERTIES LINK_DEPENDS ${LDSCRIPT})
add_definitions(${DEFS})
include_directories(. ${INCDIR})
link_directories(${LLIBDIR})
target_link_libraries(${PROJECT_NAME}.elf ${LIBS})
target_link_libraries(${PROJECT_NAME}.elf -Wl,-Map=${PROJECT_NAME}.map)

add_custom_command(
	OUTPUT ${PROJECT_NAME}.bin
	COMMAND ${CMAKE_OBJCOPY} -O binary ${PROJECT_NAME}.elf ${PROJECT_NAME}.bin
	DEPENDS ${PROJECT_NAME}.elf
)

add_custom_target(
	${PROJECT_NAME}
	DEPENDS ${PROJECT_NAME}.bin
)
