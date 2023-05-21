# Copyright (C) 2023 Bernd Herzog
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

enable_language(C CXX ASM)

project(baseband_test)

include(${CHIBIOS_PORTAPACK}/boards/PORTAPACK_BASEBAND/board.cmake)
include(${CHIBIOS_PORTAPACK}/os/hal/platforms/LPC43xx_M4/platform.cmake)
include(${CHIBIOS}/os/hal/hal.cmake)
include(${CHIBIOS_PORTAPACK}/os/ports/GCC/ARMCMx/LPC43xx_M4/port.cmake)
include(${CHIBIOS}/os/kernel/kernel.cmake)

include(${CHIBIOS}/test/test.cmake)

set(TEST_FILENAME testrunner)
set(HOST_TOOLCHAIN g++)
set(DDEFS -DLPC43XX -DLPC43XX_M4 -D__NEWLIB__ -DHACKRF_ONE -DTOOLCHAIN_GCC -DTOOLCHAIN_GCC_ARM -D_RANDOM_TCC=0 -D'VERSION_STRING=\"${VERSION}\"')

set(CPP_FILES
	${PROJECT_SOURCE_DIR}/main.cpp
	${PROJECT_SOURCE_DIR}/dsp_fft_test.cpp
	${COMMON}/dsp_fft.cpp
)

set(CPP_INCLUDES
	${PROJECT_SOURCE_DIR}/include
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

add_custom_target(
	baseband_test ALL
	COMMAND echo ${CPP_INCLUDES} | sed "s/[^[:space:]]*/-I \\\\0/g" > include_files
	COMMAND ${HOST_TOOLCHAIN} ${DDEFS} -o ${TEST_FILENAME} -g ${CPP_FILES} `cat include_files`
)

add_test(NAME /home/j39f3fs/dev/portapack-mayhem
    COMMAND ${TEST_FILENAME}
)
