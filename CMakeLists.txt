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

cmake_minimum_required(VERSION 3.16)
cmake_policy(SET CMP0005 NEW)

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_LIST_DIR}/firmware/toolchain-arm-cortex-m.cmake)

set(CMAKE_COLOR_MAKEFILE ON)

add_compile_options(-fdiagnostics-color=always)

option(USE_CCACHE "Enable ccache, please keep an eye on this because sometimes it's not quite stable and generated unusable binary" OFF)

project(portapack-h1)

set(EXPECTED_GCC_VERSION "9.2.1")

set(VERSION "$ENV{VERSION_STRING}")
if ("${VERSION}" STREQUAL "")
	execute_process(
		COMMAND git log -n 1 --format=%h
		WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
		RESULT_VARIABLE GIT_VERSION_FOUND
		ERROR_QUIET
		OUTPUT_VARIABLE GIT_VERSION
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	if (GIT_VERSION_FOUND)
		set(VERSION "unknown")
	else (GIT_VERSION_FOUND)
		set(VERSION "${GIT_VERSION}")
	endif (GIT_VERSION_FOUND)

	set(VERSION_NOHASH "dev")
else()
	set(VERSION_NOHASH "${VERSION}")
endif()

execute_process(
	COMMAND bash "-c" "echo ${VERSION} | md5sum | head -c 8"
	OUTPUT_VARIABLE VERSION_MD5
)
set(VERSION_MD5 "0x${VERSION_MD5}")

message("Building version: ${VERSION} MD5: ${VERSION_MD5}")

set(LICENSE_PATH ${CMAKE_CURRENT_LIST_DIR}/LICENSE)
set(HARDWARE_PATH ${CMAKE_CURRENT_LIST_DIR}/hardware)

add_subdirectory(hackrf/firmware)

set(HACKRF_FIRMWARE_DFU_FILENAME hackrf_usb.dfu)
set(HACKRF_FIRMWARE_BIN_FILENAME hackrf_usb_ram.bin)
set(HACKRF_FIRMWARE_FILENAME hackrf_usb.bin)
set(HACKRF_CPLD_XSVF_FILENAME default.xsvf)

set(HACKRF_PATH ${CMAKE_CURRENT_LIST_DIR}/hackrf)
set(HACKRF_CPLD_TOOL ${HACKRF_PATH}/firmware/tools/cpld_bitstream.py)
set(HACKRF_CPLD_XSVF_PATH ${HACKRF_PATH}/firmware/cpld/sgpio_if/${HACKRF_CPLD_XSVF_FILENAME})

set(HACKRF_FIRMWARE_DFU_IMAGE ${hackrf_usb_BINARY_DIR}/${HACKRF_FIRMWARE_DFU_FILENAME})
set(HACKRF_FIRMWARE_BIN_IMAGE ${hackrf_usb_BINARY_DIR}/${HACKRF_FIRMWARE_BIN_FILENAME})

# this seems causing some issues (ref. in discord discussions), but it sometimes do helps, so you have your choice to use. it's default disabled,  unless you use such command:
# cmake -DUSE_CCACHE=ON ..
# `make` or `make -j` or `make -j10` etc
# the default behavior `cmake ..` isn't changed (aka don't use ccache)
message(STATUS "-------v ccache info--------")
if(USE_CCACHE)
	find_program(CCACHE_FOUND ccache)
	if(CCACHE_FOUND)
		set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_FOUND})
		set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_FOUND})
		message(STATUS "Using ccache, please keep an eye on this because sometimes it's not quite stable and generated unusable binary.\n-- use `cmake -DUSE_CCACHE=OFF ..` (pure cmake) or `cmake -G Ninja -DUSE_CCACHE=OFF .. ` (cmake with Ninja) to turn off ccache")
	else()
		message(WARNING "ccache is enabled but not found on the system.")
	endif()
else()
	message(STATUS "Not using ccache, use `cmake -DUSE_CCACHE=ON ..` (pure cmake) or `cmake -G Ninja -DUSE_CCACHE=ON ..` (cmake with Ninja) to turn on ccache")
endif()
message(STATUS "-------^ ccache info--------")

enable_testing()
add_subdirectory(firmware)
