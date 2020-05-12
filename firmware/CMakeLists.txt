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
set(MAKE_SPI_IMAGE ${PROJECT_SOURCE_DIR}/tools/make_spi_image.py)
set(MAKE_IMAGE_CHUNK ${PROJECT_SOURCE_DIR}/tools/make_image_chunk.py)

set(FIRMWARE_NAME portapack-h1_h2-mayhem)
set(FIRMWARE_FILENAME ${FIRMWARE_NAME}.bin)

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
	firmware ALL
	DEPENDS ${FIRMWARE_FILENAME} ${HACKRF_FIRMWARE_DFU_FILENAME}
)

add_custom_target(
	program
	COMMAND dfu-util --device 1fc9:000c --download ${HACKRF_FIRMWARE_DFU_IMAGE}
	COMMAND sleep 3s
	COMMAND hackrf_spiflash -w ${FIRMWARE_FILENAME}
	DEPENDS ${FIRMWARE_FILENAME}
)

# TODO: Bad hack to fix location of LICENSE file for tar.
add_custom_command(
	OUTPUT ${FIRMWARE_NAME}-${VERSION}.tar.bz2 ${FIRMWARE_NAME}-${VERSION}.zip
	COMMAND cp ${LICENSE_PATH} LICENSE
	COMMAND cp ${HACKRF_FIRMWARE_DFU_IMAGE} ${HACKRF_FIRMWARE_DFU_FILENAME}
	COMMAND tar -c -j -f ${FIRMWARE_NAME}-${VERSION}.tar.bz2 ${FIRMWARE_FILENAME} ${HACKRF_FIRMWARE_DFU_FILENAME} LICENSE
	COMMAND zip -9 -q ${FIRMWARE_NAME}-${VERSION}.zip ${FIRMWARE_FILENAME} ${HACKRF_FIRMWARE_DFU_FILENAME} LICENSE
	COMMAND rm -f LICENSE ${HACKRF_FIRMWARE_DFU_FILENAME}
	DEPENDS ${FIRMWARE_FILENAME} ${LICENSE_PATH} ${HACKRF_FIRMWARE_DFU_FILENAME}
	VERBATIM
)

add_custom_command(
	OUTPUT MD5SUMS SHA256SUMS
	COMMAND md5sum --binary ${FIRMWARE_NAME}-${VERSION}.tar.bz2 ${FIRMWARE_NAME}-${VERSION}.zip >MD5SUMS
	COMMAND sha256sum --binary ${FIRMWARE_NAME}-${VERSION}.tar.bz2 ${FIRMWARE_NAME}-${VERSION}.zip >SHA256SUMS
	DEPENDS ${FIRMWARE_NAME}-${VERSION}.tar.bz2 ${FIRMWARE_NAME}-${VERSION}.zip
)

add_custom_target(
	release
	DEPENDS MD5SUMS SHA256SUMS
)
