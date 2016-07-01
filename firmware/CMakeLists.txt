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

project(portapack-h1-firmware)

set(COMMON ${PROJECT_SOURCE_DIR}/common)
set(CHIBIOS ${PROJECT_SOURCE_DIR}/chibios)
set(CHIBIOS_PORTAPACK ${PROJECT_SOURCE_DIR}/chibios-portapack)

set(HACKRF_FIRMWARE_FILENAME hackrf_one_usb_ram.dfu)
set(HACKRF_FIRMWARE_IMAGE ${PROJECT_SOURCE_DIR}/${HACKRF_FIRMWARE_FILENAME})
set(STRIP_DFU ${PROJECT_SOURCE_DIR}/tools/strip_dfu.py)
set(MAKE_SPI_IMAGE ${PROJECT_SOURCE_DIR}/tools/make_spi_image.py)
set(MAKE_IMAGE_CHUNK ${PROJECT_SOURCE_DIR}/tools/make_image_chunk.py)

add_subdirectory(application)
add_subdirectory(baseband)
add_subdirectory(bootstrap)

add_custom_target(
	${PROJECT_NAME}.bin
	COMMAND ${MAKE_SPI_IMAGE} ${bootstrap_BINARY_DIR}/bootstrap.bin ${HACKRF_FIRMWARE_IMAGE} ${baseband_BINARY_DIR}/baseband.img ${application_BINARY_DIR}/application.bin ${PROJECT_NAME}.bin
	DEPENDS bootstrap.bin baseband.img application.bin
)

add_custom_target(
	program
	COMMAND dfu-util --device 1fc9:000c --download ${HACKRF_FIRMWARE_IMAGE} --reset
	COMMAND sleep 1s
	COMMAND hackrf_spiflash -w ${PROJECT_NAME}.bin
	DEPENDS ${PROJECT_NAME}.bin
)

# TODO: Bad hack to fix location of LICENSE file for tar.
add_custom_target(
	release
	COMMAND cp ${LICENSE_PATH} LICENSE
	COMMAND cp ${HACKRF_FIRMWARE_IMAGE} ${HACKRF_FIRMWARE_FILENAME}
	COMMAND tar -c -j -f ${PROJECT_NAME}-${GIT_REVISION}.tar.bz2 ${PROJECT_NAME}.bin ${HACKRF_FIRMWARE_FILENAME} LICENSE
	COMMAND zip -9 -q ${PROJECT_NAME}-${GIT_REVISION}.zip ${PROJECT_NAME}.bin ${HACKRF_FIRMWARE_FILENAME} LICENSE
	COMMAND rm -f LICENSE ${HACKRF_FIRMWARE_FILENAME}
	DEPENDS ${PROJECT_NAME}.bin
)
