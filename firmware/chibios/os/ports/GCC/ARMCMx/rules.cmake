# ARM Cortex-Mx common makefile scripts and rules.

##############################################################################
# Processing options coming from the upper Makefile.
#

# Compiler options
set(OPT ${USE_OPT})
set(COPT ${USE_COPT})
set(CPPOPT ${USE_CPPOPT})

# Garbage collection
if(USE_LINK_GC STREQUAL "yes")
  set(OPT "${OPT} -ffunction-sections -fdata-sections -fno-common")
  set(LDOPT ",--gc-sections")
else()
  set(LDOPT)
endif()

# Linker extra options
if(DEFINED USE_LDOPT)
  set(LDOPT "${LDOPT},${USE_LDOPT}")
endif()

# Link time optimizations
if(USE_LTO STREQUAL "yes")
  set(OPT "${OPT} -flto")
endif()

# FPU-related options
if(NOT DEFINED USE_FPU)
  set(USE_FPU no)
endif()
if(NOT USE_FPU STREQUAL "no")
  set(OPT "${OPT} -mfloat-abi=${USE_FPU} -mfpu=fpv4-sp-d16 -fsingle-precision-constant")
  set(DDEFS ${DDEFS} -DCORTEX_USE_FPU=TRUE)
  set(DADEFS ${DADEFS} -DCORTEX_USE_FPU=TRUE)
else()
  set(DDEFS ${DDEFS} -DCORTEX_USE_FPU=FALSE)
  set(DADEFS ${DADEFS} -DCORTEX_USE_FPU=FALSE)
endif()

# Source files groups and paths
if(USE_THUMB STREQUAL "yes")
  set(TCSRC ${TCSRC} ${CSRC})
  set(TCPPSRC ${TCPPSRC} ${CPPSRC})
else()
  set(ACSRC ${ACSRC} ${CSRC})
  set(ACPPSRC ${ACPPSRC} ${CPPSRC})
endif()
set(ASRC ${ACSRC} ${ACPPSRC})
set(TSRC ${TCSRC} ${TCPPSRC})

# Paths
set(IINCDIR ${INCDIR} ${DINCDIR} ${UINCDIR})
set(LLIBDIR ${DLIBDIR} ${ULIBDIR})

# Macros
set(DEFS ${DDEFS} ${UDEFS})
set(ADEFS ${DADEFS} ${UADEFS})

# Libs
set(LIBS ${DLIBS} ${ULIBS})

# Various settings
set(MCFLAGS -mcpu=${MCU})
set(ODFLAGS "-x --syms")
set(ASFLAGS "${MCFLAGS} ${ADEFS}")
set(ASXFLAGS "${MCFLAGS} ${ADEFS}")
set(CFLAGS "${MCFLAGS} ${OPT} ${COPT} ${CWARN}")
set(CPPFLAGS "${MCFLAGS} ${OPT} ${CPPOPT} ${CPPWARN}")
set(LDFLAGS "-nostartfiles -Wl,--library-path=${RULESPATH},--script=${LDSCRIPT}${LDOPT}")

# Thumb interwork enabled only if needed because it kills performance.
if(DEFINED TSRC)
  set(CFLAGS   "${CFLAGS}")
  set(CPPFLAGS "${CPPFLAGS}")
  set(ASFLAGS  "${ASFLAGS}")
  set(DEFS ${DEFS} -DTHUMB_PRESENT)
  set(ADEFS ${ADEFS} -DTHUMB_PRESENT)
  if(DEFINED ASRC)
    # Mixed ARM and THUMB mode.
    set(CFLAGS   "${CFLAGS} -mthumb-interwork")
    set(CPPFLAGS "${CPPFLAGS} -mthumb-interwork")
    set(ASFLAGS  "${ASFLAGS} -mthumb-interwork")
    set(LDFLAGS  "${LDFLAGS} -mthumb-interwork")
  else()
    # Pure THUMB mode, THUMB C code cannot be called by ARM asm code directly.
    set(CFLAGS   "${CFLAGS} -mno-thumb-interwork")
    set(CPPFLAGS "${CPPFLAGS} -mno-thumb-interwork")
    set(ASFLAGS  "${ASFLAGS} -mno-thumb-interwork -mthumb")
    set(LDFLAGS  "${LDFLAGS} -mno-thumb-interwork -mthumb")
    set(DEFS ${DEFS} -DTHUMB_NO_INTERWORKING)
    set(ADEFS ${ADEFS} -DTHUMB_NO_INTERWORKING)
  endif()
else()
  # Pure ARM mode
  set(CFLAGS   "${CFLAGS} -mno-thumb-interwork")
  set(CPPFLAGS "${CPPFLAGS} -mno-thumb-interwork")
  set(ASFLAGS  "${ASFLAGS} -mno-thumb-interwork")
  set(LDFLAGS  "${LDFLAGS} -mno-thumb-interwork")
endif()

set(CMAKE_C_FLAGS "${CFLAGS} ${TOPT}")
set(CMAKE_CXX_FLAGS "${CPPFLAGS} ${TOPT}")
set(CMAKE_AS_FLAGS "${ASFLAGS} ${TOPT}")

if(BOARD STREQUAL "PRALINE")
    # Praline (LPC4330) - Expanded Memory
    # AHB SRAM: 32KB + 32KB = 64KB
    # Left M0_RAM_SIZE as 64k and USB_RAM_SIZE as 32k for
    # backwards consistenct with hackrf one setup. 
    # Recommend revisit by devs. 
    # Local SRAM: 128KB (Bank 1) + 72KB (Bank 2)
    # Bank 1 can also be allocated as 96KB for M4, and 32KB for M0
    set(M0_RAM_SIZE           "64k")         # AHB SRAM Bank 0 for M0 (stacks, data, bss)
    set(M0_LOCAL_HEAP_SIZE    "0")           # No Local SRAM, could be 32k heap for additional 16-bit IQ
    set(M0_LOCAL_HEAP_ORIGIN  "0x10020000")  # 0x10018000 if allocating 32k heap for M0 after M4's 96KB 
    set(M4_RAM_SIZE           "128k")        # Local SRAM Bank 1 Fully allocated to M4 (HackRF One is 96k)
    set(M4_FLASH_SIZE         "72k")         # Local SRAM Bank 2
    set(USB_RAM_SIZE          "32k")         # AHB SRAM shared with M0
    set(FLASH_SIZE            "4M")
else() 
    # HackRF One (LPC4320) - Standard Memory
    # AHB SRAM: 32KB + 16KB = 48KB (split between M0 and USB)
    # Left M0_RAM_SIZE as 64k and USB_RAM_SIZE as 32k to keep 
    # original hackrf one settings intact. 
    # Recommend revisit by devs.
    # Local SRAM: 96KB (Bank 1) + 40KB (Bank 2)
    set(M0_RAM_SIZE           "64k")   # Standard AHB SRAM for M0
    set(M0_LOCAL_HEAP_SIZE    "0k")    # No local heap available
    set(M0_LOCAL_HEAP_ORIGIN  "0")     # Not used
    set(M4_RAM_SIZE           "96k")   # Standard Local SRAM Bank 1
    set(M4_FLASH_SIZE         "32752") # Standard Local SRAM Bank 2 (~32KB)
    set(USB_RAM_SIZE          "32k")   # Standard 32k/32k split for AHB Bank 1
    set(FLASH_SIZE            "1M")    # Standard SPI Flash limit
endif()

# Apply all symbols to the linker flags in one command
# Build linker flags
set(CMAKE_EXE_LINKER_FLAGS "${LDFLAGS} \
    -Wl,--defsym=LD_RAM_SIZE=${M0_RAM_SIZE} \
    -Wl,--defsym=LD_M0_LOCAL_HEAP_SIZE=${M0_LOCAL_HEAP_SIZE} \
    -Wl,--defsym=LD_M0_LOCAL_HEAP_ORIGIN=${M0_LOCAL_HEAP_ORIGIN} \
    -Wl,--defsym=LD_M4_RAM_LEN=${M4_RAM_SIZE} \
    -Wl,--defsym=LD_M4_FLASH_LEN=${M4_FLASH_SIZE} \
    -Wl,--defsym=LD_USB_RAM_LEN=${USB_RAM_SIZE} \
    -Wl,--defsym=LD_FLASH_SIZE=${FLASH_SIZE}"
)

