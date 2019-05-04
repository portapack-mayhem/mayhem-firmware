# Copyright 2016 Jared Boone <jared@sharebrained.com>
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

project(firmware)

set(BASEBAND ${PROJECT_SOURCE_DIR}/baseband)
set(COMMON ${PROJECT_SOURCE_DIR}/common)
set(CHIBIOS ${PROJECT_SOURCE_DIR}/chibios)
set(CHIBIOS_PORTAPACK ${PROJECT_SOURCE_DIR}/chibios-portapack)

set(EXTRACT_CPLD_DATA ${PROJECT_SOURCE_DIR}/tools/extract_cpld_data.py)
set(STRIP_DFU ${PROJECT_SOURCE_DIR}/tools/strip_dfu.py)
set(MAKE_SPI_IMAGE ${PROJECT_SOURCE_DIR}/tools/make_spi_image.py)
set(MAKE_IMAGE_CHUNK ${PROJECT_SOURCE_DIR}/tools/make_image_chunk.py)

set(FIRMWARE_NAME portapack-h1-havoc)
set(FIRMWARE_FILENAME ${FIRMWARE_NAME}.bin)

include(ExternalProject)
find_program(MAKE_EXE NAMES gmake nmake make)
ExternalProject_Add(hackrf
	GIT_REPOSITORY https://github.com/jboone/hackrf.git
	# SOURCE_SUBDIR firmware
	# SOURCE_SUBDIR isn't available in CMake 3.5 (Ubuntu 16.04 LTS), so the following is a work-around:
	CONFIGURE_COMMAND ${CMAKE_COMMAND} "-GUnix Makefiles" ../hackrf/firmware
	BUILD_COMMAND ${MAKE_EXE} hackrf_usb.dfu
	INSTALL_COMMAND ""
)
ExternalProject_Get_Property(hackrf SOURCE_DIR)
ExternalProject_Get_Property(hackrf BINARY_DIR)
set(hackrf_SOURCE_DIR ${SOURCE_DIR})
set(hackrf_BINARY_DIR ${BINARY_DIR})

set(HACKRF_FIRMWARE_DFU_FILENAME hackrf_usb.dfu)
set(HACKRF_FIRMWARE_BIN_FILENAME hackrf_usb.bin)

set(HACKRF_FIRMWARE_DFU_IMAGE ${hackrf_BINARY_DIR}/hackrf_usb/${HACKRF_FIRMWARE_DFU_FILENAME})
set(HACKRF_FIRMWARE_BIN_IMAGE ${hackrf_BINARY_DIR}/hackrf_usb/${HACKRF_FIRMWARE_BIN_FILENAME})
set(HACKRF_CPLD_TOOL ${hackrf_SOURCE_DIR}/firmware/tools/cpld_bitstream.py)
set(HACKRF_CPLD_XSVF_FILENAME default.xsvf)
set(HACKRF_CPLD_XSVF_PATH ${hackrf_SOURCE_DIR}/firmware/cpld/sgpio_if/${HACKRF_CPLD_XSVF_FILENAME})

add_subdirectory(application)
add_subdirectory(baseband)

# NOTE: Dependencies break if the .bin files aren't included in DEPENDS. WTF, CMake?
add_custom_command(
	OUTPUT ${FIRMWARE_FILENAME}
	COMMAND ${MAKE_SPI_IMAGE} ${application_BINARY_DIR}/application.bin ${baseband_BINARY_DIR}/baseband.img ${FIRMWARE_FILENAME}
	DEPENDS baseband application ${MAKE_SPI_IMAGE}
		 ${baseband_BINARY_DIR}/baseband.img ${application_BINARY_DIR}/application.bin
	VERBATIM
)

add_custom_target(
	firmware
	DEPENDS ${FIRMWARE_FILENAME}
)

add_custom_target(
	program
	COMMAND dfu-util --device 1fc9:000c --download ${HACKRF_FIRMWARE_DFU_IMAGE}
	COMMAND sleep 3s
	COMMAND hackrf_spiflash -w ${FIRMWARE_FILENAME}
	DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${FIRMWARE_FILENAME}
)

# TODO: Bad hack to fix location of LICENSE file for tar.
add_custom_command(
	OUTPUT ${FIRMWARE_NAME}-${GIT_REVISION}.tar.bz2 ${FIRMWARE_NAME}-${GIT_REVISION}.zip
	COMMAND cp ${LICENSE_PATH} LICENSE
	COMMAND cp ${HACKRF_FIRMWARE_DFU_IMAGE} ${HACKRF_FIRMWARE_DFU_FILENAME}
	COMMAND tar -c -j -f ${FIRMWARE_NAME}-${GIT_REVISION}.tar.bz2 ${FIRMWARE_FILENAME} ${HACKRF_FIRMWARE_DFU_FILENAME} LICENSE
	COMMAND zip -9 -q ${FIRMWARE_NAME}-${GIT_REVISION}.zip ${FIRMWARE_FILENAME} ${HACKRF_FIRMWARE_DFU_FILENAME} LICENSE
	COMMAND rm -f LICENSE ${HACKRF_FIRMWARE_DFU_FILENAME}
	DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${FIRMWARE_FILENAME} ${LICENSE_PATH} ${HACKRF_FIRMWARE_DFU_IMAGE}
	VERBATIM
)

add_custom_target(
	release
	DEPENDS ${FIRMWARE_NAME}-${GIT_REVISION}.tar.bz2 ${FIRMWARE_NAME}-${GIT_REVISION}.zip
)
