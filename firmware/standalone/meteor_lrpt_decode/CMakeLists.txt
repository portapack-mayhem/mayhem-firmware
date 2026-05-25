#
# Meteor LRPT offline FEC decoder (.ppmp)
#
enable_language(C CXX ASM)
include(CheckCXXCompilerFlag)
set(CHIBIOS ${CMAKE_CURRENT_SOURCE_DIR}/../../chibios)
set(CHIBIOS_PORTAPACK ${CMAKE_CURRENT_SOURCE_DIR}/../../chibios-portapack)
set(COMMON ${CMAKE_CURRENT_SOURCE_DIR}/../../common)
project(meteor_lrpt_decode_app)

set(USE_OPT "-Os -g --specs=nano.specs --specs=nosys.specs")
set(USE_COPT "-std=gnu99")
check_cxx_compiler_flag("-std=c++20" cpp20_supported)
if(cpp20_supported)
	set(USE_CPPOPT "-std=c++20")
else()
	set(USE_CPPOPT "-std=c++17")
endif()
set(USE_CPPOPT "${USE_CPPOPT} -fno-rtti -fno-exceptions -Weffc++ -Wuninitialized -fno-use-cxa-atexit")
set(USE_LINK_GC yes)
set(USE_LDOPT)
set(USE_LTO no)
set(USE_THUMB yes)
set(USE_VERBOSE_COMPILE no)

set(MCU cortex-m0)
set(LDSCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/external.ld)

set(BB ${CMAKE_CURRENT_SOURCE_DIR}/../../baseband)
include(${CMAKE_CURRENT_SOURCE_DIR}/../meteor_standalone_ui_sources.cmake)
set(CPPSRC
	meteor_decode.cpp
	main.cpp
	${METEOR_STANDALONE_UI_CPPSRC}
	${BB}/meteor_lrpt/meteor_offline_fec.cpp
	${BB}/meteor_lrpt/meteor_cc_decoder.cpp
	${BB}/meteor_lrpt/meteor_viterbi12.cpp
	${BB}/meteor_lrpt/ccsds_derandomize.cpp
	${BB}/meteor_lrpt/meteor_bpsk_ccsds_deframer.cpp
	${BB}/meteor_lrpt/meteor_nrzm.cpp
	${BB}/meteor_lrpt/meteor_soft_correlate.cpp
	${BB}/meteor_lrpt/rs223_decode.cpp
)
set(CSRC)
set(ASMSRC)

set(INCDIR
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/..
	${CMAKE_CURRENT_SOURCE_DIR}/../common/ui
	${COMMON}
	${COMMON}/../application
	${COMMON}/../application/hw
	${BB}
	${BB}/meteor_lrpt
)

set(DDEFS "-DLPC43XX -DLPC43XX_M0 -D__NEWLIB__ -DHACKRF_ONE -DTOOLCHAIN_GCC -DTOOLCHAIN_GCC_ARM -D_RANDOM_TCC=0")
set(RULESPATH ${CHIBIOS}/os/ports/GCC/ARMCMx)
include(${RULESPATH}/rules.cmake)

add_executable(${PROJECT_NAME}.elf ${CSRC} ${CPPSRC} ${ASMSRC})
set_target_properties(${PROJECT_NAME}.elf PROPERTIES LINK_DEPENDS ${LDSCRIPT})
add_definitions(${DEFS})
target_compile_definitions(${PROJECT_NAME}.elf PRIVATE METEOR_STANDALONE_NO_CH=1 METEOR_LRPT_DECODE_PPMP=1)
include_directories(. ${INCDIR})
link_directories(${LLIBDIR})
target_link_libraries(${PROJECT_NAME}.elf -Wl,-Map=${PROJECT_NAME}.map)
target_link_libraries(${PROJECT_NAME}.elf "-Wl,-wrap,_malloc_r")
target_link_libraries(${PROJECT_NAME}.elf "-Wl,--print-memory-usage")

add_custom_command(
	OUTPUT ${PROJECT_NAME}.ppmp
	COMMAND ${CMAKE_OBJCOPY} -v -O binary ${PROJECT_NAME}.elf ${PROJECT_NAME}.ppmp
	DEPENDS ${PROJECT_NAME}.elf
)
add_custom_target(${PROJECT_NAME} DEPENDS ${PROJECT_NAME}.ppmp)
