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

cmake_minimum_required(VERSION 3.5)

project(firmware)

set(BASEBAND ${PROJECT_SOURCE_DIR}/baseband)
set(COMMON ${PROJECT_SOURCE_DIR}/common)
set(CHIBIOS ${PROJECT_SOURCE_DIR}/chibios)
set(CHIBIOS_PORTAPACK ${PROJECT_SOURCE_DIR}/chibios-portapack)

set(EXTRACT_CPLD_DATA ${PROJECT_SOURCE_DIR}/tools/extract_cpld_data.py)
set(EXPORT_EXTERNAL_APP_IMAGES ${PROJECT_SOURCE_DIR}/tools/export_external_apps.py)
set(MAKE_SPI_IMAGE ${PROJECT_SOURCE_DIR}/tools/make_spi_image.py)
set(MAKE_IMAGE_CHUNK ${PROJECT_SOURCE_DIR}/tools/make_image_chunk.py)
set(LZ4 lz4)

set(FIRMWARE_NAME portapack-mayhem-firmware)
set(FIRMWARE_FILENAME ${FIRMWARE_NAME}.bin)
set(PPFW_FILENAME "portapack-mayhem_OCI.ppfw.tar")

# In our current build container cmake need a little help to get the version :)
if(NOT DEFINED ${CMAKE_CXX_COMPILER_VERSION})
	execute_process(COMMAND bash "-c" "arm-none-eabi-g++ -v 2>&1 | grep 'gcc version' | awk '{print $3}'" OUTPUT_VARIABLE CMAKE_CXX_COMPILER_VERSION)
	string(STRIP ${CMAKE_CXX_COMPILER_VERSION} CMAKE_CXX_COMPILER_VERSION)
endif()

set(GCC_VERSION_MISMATCH 0)
if(NOT ${CMAKE_CXX_COMPILER_VERSION} VERSION_EQUAL ${EXPECTED_GCC_VERSION})
	set(GCC_VERSION_MISMATCH 1)
endif()

add_subdirectory(application)
add_subdirectory(baseband)
add_subdirectory(standalone)
add_subdirectory(test)

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
	DEPENDS ${FIRMWARE_FILENAME} ${HACKRF_FIRMWARE_DFU_FILENAME} ${HACKRF_FIRMWARE_FILENAME}
)

if(${GCC_VERSION_MISMATCH})
	set(COMPILER_MISMATCH_MESSAGE "WARNING: Compiler version mismatch, please use the official compiler version ${EXPECTED_GCC_VERSION} when sharing builds! Current compiler version: ${CMAKE_CXX_COMPILER_VERSION}")
	message(${COMPILER_MISMATCH_MESSAGE})
	add_custom_command(
		TARGET firmware POST_BUILD
		COMMAND echo ${COMPILER_MISMATCH_MESSAGE}
		VERBATIM)
endif()

add_custom_target(
	program
	COMMAND ${PROJECT_SOURCE_DIR}/tools/enter_mode.sh hackrf
	COMMAND dfu-util --device 1fc9:000c --download ${HACKRF_FIRMWARE_DFU_IMAGE} || (exit 0) # We need to add it for dfu-utils v.011 , (in v.09 it is not necessary)
	COMMAND sleep 3s
	COMMAND hackrf_spiflash -i -R -w ${FIRMWARE_FILENAME}
	DEPENDS firmware ${FIRMWARE_FILENAME}
)

add_custom_target(
	program-external-apps
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	COMMAND ${PROJECT_SOURCE_DIR}/tools/copy_external_apps.sh
	DEPENDS program standalone_apps
)

add_custom_command(
	OUTPUT ${PPFW_FILENAME}
	
	COMMAND rm -rf firmware_tar
	COMMAND mkdir -p firmware_tar/FIRMWARE
	# Using VERSION_NOHASH to avoid dev builds piling up in the FIRMWARE folder of the sd card of testers in #test-drive
	COMMAND cp ${FIRMWARE_FILENAME} firmware_tar/FIRMWARE/portapack-mayhem_${VERSION_NOHASH}.bin
	COMMAND mkdir -p firmware_tar/APPS
	COMMAND cp application/*.ppma firmware_tar/APPS
	COMMAND cp standalone/*/*.ppmp firmware_tar/APPS
	COMMAND cd firmware_tar && tar -cvaf ../${PPFW_FILENAME} *
	DEPENDS firmware ${FIRMWARE_FILENAME} standalone_apps
	# Dont use VERBATIM here as it prevents usage of globbing (*)
	# There shouldnt be any funny business in the filenames above :)
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
	ppfw ALL
	DEPENDS ${PPFW_FILENAME}
)

add_custom_target(
	oci
	DEPENDS ${PPFW_FILENAME}
)

add_custom_target(
	release
	DEPENDS MD5SUMS SHA256SUMS
)

