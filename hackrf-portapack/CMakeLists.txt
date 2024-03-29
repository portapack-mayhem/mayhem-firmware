# Copyright 2012 Jared Boone <jared@sharebrained.com>
#
# This file is part of HackRF.
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

cmake_minimum_required(VERSION 3.5)
set(CMAKE_TOOLCHAIN_FILE ../toolchain-arm-cortex-m.cmake)

project(hackrf_usb C)

include(../hackrf/firmware/hackrf-common.cmake)

set(PATH_HACKRF_USB ../hackrf/firmware/hackrf_usb)

add_custom_command(
	OUTPUT ${PATH_HACKRF_CPLD_DATA_C}
	COMMAND ${PATH_CPLD_BITSTREAM_TOOL} --xsvf ${PATH_HACKRF_CPLD_XSVF} --hackrf-data ${PATH_HACKRF_CPLD_DATA_C}
	DEPENDS ${PATH_CPLD_BITSTREAM_TOOL} ${PATH_HACKRF_CPLD_XSVF}
)

set(SRC_M4
	${PATH_HACKRF_USB}/hackrf_usb.c
	"${PATH_HACKRF_FIRMWARE_COMMON}/tuning.c"
	"${PATH_HACKRF_FIRMWARE_COMMON}/streaming.c"
	"${PATH_HACKRF_FIRMWARE_COMMON}/usb.c"
	"${PATH_HACKRF_FIRMWARE_COMMON}/usb_request.c"
	"${PATH_HACKRF_FIRMWARE_COMMON}/usb_standard_request.c"
	${PATH_HACKRF_USB}/usb_descriptor.c
	${PATH_HACKRF_USB}/usb_device.c
	${PATH_HACKRF_USB}/usb_endpoint.c
	${PATH_HACKRF_USB}/usb_api_cpld.c
	${PATH_HACKRF_USB}/usb_api_m0_state.c
	${PATH_HACKRF_USB}/usb_api_register.c
	${PATH_HACKRF_USB}/usb_api_spiflash.c
	${PATH_HACKRF_USB}/usb_api_transceiver.c
	${PATH_HACKRF_USB}/usb_api_operacake.c
	${PATH_HACKRF_USB}/usb_api_sweep.c
	${PATH_HACKRF_USB}/usb_api_ui.c
	"${PATH_HACKRF_FIRMWARE_COMMON}/usb_queue.c"
	"${PATH_HACKRF_FIRMWARE_COMMON}/fault_handler.c"
	"${PATH_HACKRF_FIRMWARE_COMMON}/cpld_jtag.c"
	"${PATH_HACKRF_FIRMWARE_COMMON}/cpld_xc2c.c"
	"${PATH_HACKRF_CPLD_DATA_C}"
	"${PATH_HACKRF_FIRMWARE_COMMON}/xapp058/lenval.c"
	"${PATH_HACKRF_FIRMWARE_COMMON}/xapp058/micro.c"
	"${PATH_HACKRF_FIRMWARE_COMMON}/xapp058/ports.c"
	"${PATH_HACKRF_FIRMWARE_COMMON}/crc.c"
	"${PATH_HACKRF_FIRMWARE_COMMON}/rom_iap.c"
	"${PATH_HACKRF_FIRMWARE_COMMON}/operacake.c"
	"${PATH_HACKRF_FIRMWARE_COMMON}/operacake_sctimer.c"
)

set(SRC_M0 ${PATH_HACKRF_USB}/sgpio_m0.s)

if(BOARD STREQUAL "HACKRF_ONE")
	SET(SRC_M4
		${SRC_M4}
		usb_api_board_info.c
		portapack.c
		"${PATH_HACKRF_FIRMWARE_COMMON}/ui_portapack.c"
	)
endif()

if(BOARD STREQUAL "RAD1O")
	SET(SRC_M4
		${SRC_M4}
		"${PATH_HACKRF_FIRMWARE_COMMON}/rad1o/display.c"
		"${PATH_HACKRF_FIRMWARE_COMMON}/rad1o/print.c"
		"${PATH_HACKRF_FIRMWARE_COMMON}/rad1o/render.c"
		"${PATH_HACKRF_FIRMWARE_COMMON}/rad1o/decoder.c"
		"${PATH_HACKRF_FIRMWARE_COMMON}/rad1o/smallfonts.c"
		"${PATH_HACKRF_FIRMWARE_COMMON}/rad1o/draw.c"
		"${PATH_HACKRF_FIRMWARE_COMMON}/rad1o/ubuntu18.c"
		"${PATH_HACKRF_FIRMWARE_COMMON}/ui_rad1o.c"
	)
endif()

DeclareTargets()
