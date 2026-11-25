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

project(application_test)

enable_language(C CXX ASM)

include(${CHIBIOS_PORTAPACK}/boards/PORTAPACK_APPLICATION/board.cmake)
include(${CHIBIOS_PORTAPACK}/os/hal/platforms/LPC43xx_M0/platform.cmake)
include(${CHIBIOS}/os/hal/hal.cmake)
include(${CHIBIOS_PORTAPACK}/os/ports/GCC/ARMCMx/LPC43xx_M0/port.cmake)
include(${CHIBIOS}/os/kernel/kernel.cmake)
include(${CHIBIOS_PORTAPACK}/os/various/fatfs_bindings/fatfs.cmake)
include(${CHIBIOS}/test/test.cmake)

set(CMAKE_CXX_COMPILER g++)

add_executable(application_test EXCLUDE_FROM_ALL
	${PROJECT_SOURCE_DIR}/main.cpp
	${PROJECT_SOURCE_DIR}/test_basics.cpp
	${PROJECT_SOURCE_DIR}/test_circular_buffer.cpp
	${PROJECT_SOURCE_DIR}/test_convert.cpp
	${PROJECT_SOURCE_DIR}/test_file_reader.cpp
	${PROJECT_SOURCE_DIR}/test_file_wrapper.cpp
	${PROJECT_SOURCE_DIR}/test_freqman_db.cpp
	${PROJECT_SOURCE_DIR}/test_mock_file.cpp
	${PROJECT_SOURCE_DIR}/test_optional.cpp
	${PROJECT_SOURCE_DIR}/test_string_format.cpp
	${PROJECT_SOURCE_DIR}/test_utility.cpp

	# DAB/DAB+ protocol and FEC tests
	${PROJECT_SOURCE_DIR}/dab/test_dab_aac.cpp
	${PROJECT_SOURCE_DIR}/dab/test_dab_blocks.cpp
	${PROJECT_SOURCE_DIR}/dab/test_dab_cache.cpp
	${PROJECT_SOURCE_DIR}/dab/test_dab_charset.cpp
	${PROJECT_SOURCE_DIR}/dab/test_dab_crc.cpp
	${PROJECT_SOURCE_DIR}/dab/test_dab_deinterleave.cpp
	${PROJECT_SOURCE_DIR}/dab/test_dab_dls.cpp
	${PROJECT_SOURCE_DIR}/dab/test_dab_ensemble.cpp
	${PROJECT_SOURCE_DIR}/dab/test_dab_fft.cpp
	${PROJECT_SOURCE_DIR}/dab/test_dab_fic.cpp
	${PROJECT_SOURCE_DIR}/dab/test_dab_fig.cpp
	${PROJECT_SOURCE_DIR}/dab/test_dab_freq_interleave.cpp
	${PROJECT_SOURCE_DIR}/dab/test_dab_mot.cpp
	${PROJECT_SOURCE_DIR}/dab/test_dab_msc.cpp
	${PROJECT_SOURCE_DIR}/dab/test_dab_ofdm.cpp
	${PROJECT_SOURCE_DIR}/dab/test_dab_prbs.cpp
	${PROJECT_SOURCE_DIR}/dab/test_dab_protection.cpp
	${PROJECT_SOURCE_DIR}/dab/test_dab_reed_solomon.cpp
	${PROJECT_SOURCE_DIR}/dab/test_dab_scanner.cpp
	${PROJECT_SOURCE_DIR}/dab/test_dab_superframe.cpp
	${PROJECT_SOURCE_DIR}/dab/test_dab_viterbi.cpp
	${PROJECT_SOURCE_DIR}/dab/test_dab_xpad.cpp

	# DAB/DAB+ implementation under test
	${PROJECT_SOURCE_DIR}/../../common/dab/dab_aac.cpp
	${PROJECT_SOURCE_DIR}/../../common/dab/dab_blocks.cpp
	${PROJECT_SOURCE_DIR}/../../common/dab/dab_cache.cpp
	${PROJECT_SOURCE_DIR}/../../common/dab/dab_crc.cpp
	${PROJECT_SOURCE_DIR}/../../common/dab/dab_deinterleave.cpp
	${PROJECT_SOURCE_DIR}/../../common/dab/dab_dls.cpp
	${PROJECT_SOURCE_DIR}/../../common/dab/dab_ensemble.cpp
	${PROJECT_SOURCE_DIR}/../../common/dab/dab_fft.cpp
	${PROJECT_SOURCE_DIR}/../../common/dab/dab_fic.cpp
	${PROJECT_SOURCE_DIR}/../../common/dab/dab_fig.cpp
	${PROJECT_SOURCE_DIR}/../../common/dab/dab_charset.cpp
	${PROJECT_SOURCE_DIR}/../../common/dab/dab_freq_interleave.cpp
	${PROJECT_SOURCE_DIR}/../../common/dab/dab_mot.cpp
	${PROJECT_SOURCE_DIR}/../../common/dab/dab_mp2.cpp
	${PROJECT_SOURCE_DIR}/../../common/dab/dab_msc.cpp
	${PROJECT_SOURCE_DIR}/../../common/dab/dab_ofdm.cpp
	${PROJECT_SOURCE_DIR}/../../common/dab/dab_prbs.cpp
	${PROJECT_SOURCE_DIR}/../../common/dab/dab_protection.cpp
	${PROJECT_SOURCE_DIR}/../../common/dab/dab_reed_solomon.cpp
	${PROJECT_SOURCE_DIR}/../../common/dab/dab_scanner.cpp
	${PROJECT_SOURCE_DIR}/../../common/dab/dab_superframe.cpp
	${PROJECT_SOURCE_DIR}/../../common/dab/dab_viterbi.cpp
	${PROJECT_SOURCE_DIR}/../../common/dab/dab_xpad.cpp
	${PROJECT_SOURCE_DIR}/../../common/dab/vendor/kjmp2.c

	${PROJECT_SOURCE_DIR}/../../application/file_reader.cpp
	${PROJECT_SOURCE_DIR}/../../application/freqman_db.cpp
	${PROJECT_SOURCE_DIR}/../../common/utility.cpp
	
	# Dependencies
	${PROJECT_SOURCE_DIR}/../../application/file.cpp
	${PROJECT_SOURCE_DIR}/../../application/string_format.cpp
	${PROJECT_SOURCE_DIR}/../../application/tone_key.cpp
	${PROJECT_SOURCE_DIR}/linker_stubs.cpp
)

target_include_directories(application_test PRIVATE
	${DOCTESTINC}
	${PROJECT_SOURCE_DIR}/../../application
	${COMMON}
	${PORTINC}
	${KERNINC}
	${TESTINC}
	${HALINC}
	${PLATFORMINC}
	${BOARDINC}
	${CHIBIOS}/os/various
	${FATFSINC}
	${BASEBAND}
)

target_compile_options(application_test PRIVATE
	-std=c++17
	-DLPC43XX
	-DLPC43XX_M0
	-D__NEWLIB__
	-DHACKRF_ONE
	-DTOOLCHAIN_GCC
	-DTOOLCHAIN_GCC_ARM
	-D_RANDOM_TCC=0
	-DVERSION_STRING=\"${VERSION}\"
	${USE_CPPOPT}
	${USE_OPT}
	${CPPWARN}
)

add_test(NAME application_test
    COMMAND application_test
)
