# Copyright (C) 2023 Bernd Herzog, Kyle Reed
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

project(baseband_test)

enable_language(C CXX ASM)

include(${CHIBIOS_PORTAPACK}/boards/PORTAPACK_BASEBAND/board.cmake)
include(${CHIBIOS_PORTAPACK}/os/hal/platforms/LPC43xx_M4/platform.cmake)
include(${CHIBIOS}/os/hal/hal.cmake)
include(${CHIBIOS_PORTAPACK}/os/ports/GCC/ARMCMx/LPC43xx_M4/port.cmake)
include(${CHIBIOS}/os/kernel/kernel.cmake)
include(${CHIBIOS}/test/test.cmake)

set(CMAKE_CXX_COMPILER g++)

add_executable(baseband_test EXCLUDE_FROM_ALL
	${PROJECT_SOURCE_DIR}/main.cpp
	${PROJECT_SOURCE_DIR}/dsp_fft_test.cpp
	${COMMON}/dsp_fft.cpp
)

target_include_directories(baseband_test PRIVATE
	${DOCTESTINC}
	${COMMON}
	${PORTINC}
	${KERNINC}
	${TESTINC}
	${HALINC}
	${PLATFORMINC}
	${BOARDINC}
	${CHIBIOS}/os/various
	${BASEBAND}
)

target_compile_options(baseband_test PRIVATE
	-DLPC43XX
	-DLPC43XX_M4
	-D__NEWLIB__
	-DHACKRF_ONE
	-DTOOLCHAIN_GCC
	-DTOOLCHAIN_GCC_ARM
	-D_RANDOM_TCC=0
	-DVERSION_STRING=\"${VERSION}\"
)

add_test(NAME baseband_test
    COMMAND baseband_test
)
