# List of all the board related files.
set(BOARDSRC
	${CHIBIOS_PORTAPACK}/boards/PORTAPACK_APPLICATION/board.cpp
)

# Add FPGA bridge for PRALINE (HackRF Pro)
if(BOARD STREQUAL "PRALINE")
	list(APPEND BOARDSRC
		${CHIBIOS_PORTAPACK}/boards/PORTAPACK_APPLICATION/fpga_bridge.c
		${CHIBIOS_PORTAPACK}/../../hackrf/firmware/common/lz4_blk.c
	)
endif()

# Required include directories
set(BOARDINC
	${CHIBIOS_PORTAPACK}/boards/PORTAPACK_APPLICATION
)

# Add hackrf common include for PRALINE (for lz4_blk.h)
if(BOARD STREQUAL "PRALINE")
	list(APPEND BOARDINC ${CHIBIOS_PORTAPACK}/../../hackrf/firmware/common)
endif()
