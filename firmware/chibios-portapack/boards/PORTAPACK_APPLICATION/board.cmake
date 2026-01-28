
# 1. Define the HackRF common directory path
set(HACKRF_COMMON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../hackrf/firmware/common/)
set(HACKRF_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../hackrf/)

# 2. Add the bridge and the required HackRF drivers to BOARDSRC
if(BOARD STREQUAL "PRALINE")

	# Need to tell the assembler to use Unified Syntax correctly choose the 16-bit or 32-bit version of the instructions.
	add_compile_options("-masm-syntax-unified")

	SET(PATH_PRALINE_FPGA_BIN ${HACKRF_PATH}/firmware/fpga/build/praline_fpga.bin)
	set(FPGA_OBJ ${CMAKE_CURRENT_BINARY_DIR}/fpga.o)

	# Commented out to reduce firmware size by nearly 200kB.
	#add_custom_command(
	#	OUTPUT  ${FPGA_OBJ}
	#	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	#	COMMAND ${CMAKE_COMMAND} -E copy ${PATH_PRALINE_FPGA_BIN} "fpga.bin"
	#	COMMAND ${CMAKE_OBJCOPY} 
	#		-I binary 
	#		-O elf32-littlearm 
	#		-B armv7e-m 
	#		--rename-section .data=.rodata,alloc,load,readonly,data,contents #,.rom_only 
	#		fpga.bin ${FPGA_OBJ}
	#	DEPENDS ${PATH_PRALINE_FPGA_BIN}
	#)

	# This is an external object so we don't need to look for a .c file
	#set_source_files_properties(${FPGA_OBJ} PROPERTIES EXTERNAL_OBJECT TRUE GENERATED TRUE)

	set(BOARDSRC
		${CHIBIOS_PORTAPACK}/boards/PORTAPACK_APPLICATION/fpga_bridge.c
		${CHIBIOS_PORTAPACK}/boards/PORTAPACK_APPLICATION/board.cpp
		#${HACKRF_COMMON_DIR}/adc.c
		#${HACKRF_COMMON_DIR}/clkin.c
		#${HACKRF_COMMON_DIR}/firmware_info.c
		${HACKRF_COMMON_DIR}/fpga.c
		${HACKRF_COMMON_DIR}/fpga_image.c
		${HACKRF_COMMON_DIR}/hackrf_core.c
		${HACKRF_COMMON_DIR}/i2c_bus.c
		${HACKRF_COMMON_DIR}/i2c_lpc.c
		${HACKRF_COMMON_DIR}/ice40_spi.c
		#${HACKRF_COMMON_DIR}/gpdma.c
		${HACKRF_COMMON_DIR}/gpio_lpc.c
		${HACKRF_COMMON_DIR}/lz4_blk.c
		${HACKRF_COMMON_DIR}/mixer.c
		${HACKRF_COMMON_DIR}/max2831.c
		${HACKRF_COMMON_DIR}/max5864.c
		${HACKRF_COMMON_DIR}/platform_detect.c
		${HACKRF_COMMON_DIR}/radio.c
		${HACKRF_COMMON_DIR}/rf_path.c
		${HACKRF_COMMON_DIR}/rffc5071_spi.c
		#${HACKRF_COMMON_DIR}/rffc5071.c
		${HACKRF_COMMON_DIR}/selftest.c
		${HACKRF_COMMON_DIR}/si5351c.c
		${HACKRF_COMMON_DIR}/spi_ssp.c
		${HACKRF_COMMON_DIR}/spi_bus.c
		${HACKRF_COMMON_DIR}/sgpio.c
		${HACKRF_COMMON_DIR}/tuning.c
		#${HACKRF_COMMON_DIR}/usb.c
		#${HACKRF_COMMON_DIR}/usb_queue.c
		#${HACKRF_COMMON_DIR}/usb_request.c
		#${HACKRF_COMMON_DIR}/usb_standard_request.c
		${HACKRF_COMMON_DIR}/w25q80bv.c
		${HACKRF_COMMON_DIR}/w25q80bv_target.c
		#${HACKRF_PATH}/firmware/hackrf_usb/usb_device.c
		#${HACKRF_PATH}/firmware/hackrf_usb/usb_endpoint.c
		${HACKRF_PATH}/firmware/libopencm3/lib/cm3/nvic.c
		${HACKRF_PATH}/firmware/libopencm3/lib/cm3/sync.c
		${HACKRF_PATH}/firmware/libopencm3/lib/lpc43xx/scu.c
		${HACKRF_PATH}/firmware/libopencm3/lib/lpc43xx/timer.c
		${HACKRF_PATH}/firmware/libopencm3/lib/lpc43xx/i2c.c
	)

	# Append the generated fpga object, either compressed with LZ4, or otherwise.
	# Finally, add it to your BOARDSRC list
	#list(APPEND BOARDSRC ${FPGA_OBJ})
else()
	set(BOARDSRC
		${CHIBIOS_PORTAPACK}/boards/PORTAPACK_APPLICATION/fpga_bridge.c
		${CHIBIOS_PORTAPACK}/boards/PORTAPACK_APPLICATION/board.cpp
	)
endif()

# 3. Ensure the include path is set so the compiler finds the headers
if(BOARD STREQUAL "PRALINE")
	set(BOARDINC
		${CHIBIOS_PORTAPACK}/boards/PORTAPACK_APPLICATION
		${HACKRF_PATH}/firmware
		${HACKRF_PATH}/firmware/common
		${HACKRF_PATH}/firmware/hackrf_usb
		${HACKRF_PATH}/firmware/libopencm3/include
	)
else()
	set(BOARDINC
		${CHIBIOS_PORTAPACK}/boards/PORTAPACK_APPLICATION
	)
endif()
