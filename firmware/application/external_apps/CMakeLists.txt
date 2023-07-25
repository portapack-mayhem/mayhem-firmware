#
# Copyright (C) 2023 Kyle Reed
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

# External Applications - PortaPack App (PPA)
#
# External application (*.ppa) files are applications which can be loaded
# at runtime from the SD card. Special compilation and linking is performed
# at compile time so that the PPA can be loaded by the bootloader app.

enable_language(CXX)

include(CheckCXXCompilerFlag)

project(ppa_framework)

# Common framework code, libs, etc.

#####################################################################
# External Apps
#

macro(DeclarePPA name)
    project(ppa_${name})
    add_library(${PROJECT_NAME} STATIC ${PPA_CPPSRC})

    # Give the output a nice name like "name.ppa"
	set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
	set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME ${name})
	set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".ppa")

    # Add macro definitions
    add_definitions()

    # Linker Script -- probably need to hand-roll one for apps.
    # firmware/chibios-portapack/os/ports/GCC/ARMCMx/LPC43xx_M0/ld/LPC43xx_M0.ld

    # From baseband macro for reference
    # include(${RULESPATH}/rules.cmake)
	# add_executable(${PROJECT_NAME}.elf $<TARGET_OBJECTS:baseband_shared> ${MODE_CPPSRC} ${HALSRC} ${PLATFORMSRC})
	# set_target_properties(${PROJECT_NAME}.elf PROPERTIES LINK_DEPENDS ${LDSCRIPT})
	# add_definitions(${DEFS})
	# target_compile_definitions(${PROJECT_NAME}.elf PUBLIC "-DBASEBAND_${name}")
	# include_directories(. ${INCDIR} ${MODE_INCDIR})
	# link_directories(${LLIBDIR})
	# target_link_libraries(${PROJECT_NAME}.elf ${LIBS})
	# target_link_libraries(${PROJECT_NAME}.elf -Wl,-Map=${PROJECT_NAME}.map)

	# add_custom_command(
	# 	OUTPUT ${PROJECT_NAME}.bin ${PROJECT_NAME}.img
	# 	COMMAND ${CMAKE_OBJCOPY} -O binary ${PROJECT_NAME}.elf ${PROJECT_NAME}.bin
	# 	COMMAND ${LZ4} -f -9 ${PROJECT_NAME}.bin ${PROJECT_NAME}.lz4
	# 	COMMAND ${MAKE_IMAGE_CHUNK} ${PROJECT_NAME}.lz4 ${chunk_tag} ${PROJECT_NAME}.img
	# 	DEPENDS ${PROJECT_NAME}.elf ${MAKE_IMAGE_CHUNK}
	# 	VERBATIM
	# )

	# set(BASEBAND_IMAGES ${BASEBAND_IMAGES} ${PROJECT_NAME}.img)
endmacro()

#####################################################################
# Apps
#

### Sample App
set(PPA_CPPSRC
    sample_app/app.cpp
)
DeclarePPA(sample_app)

### Add additional apps ...