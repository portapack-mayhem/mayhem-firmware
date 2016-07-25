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
set(CMAKE_EXE_LINKER_FLAGS "${LDFLAGS}")
