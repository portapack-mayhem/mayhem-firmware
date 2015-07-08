#
#       !!!! Do NOT edit this makefile with an editor which replace tabs by spaces !!!!
#
##############################################################################################
#
# On command line:
#
# make all = Create project
#
# make clean = Clean project files.
#
# To rebuild project do "make clean" and "make all".
#

##############################################################################################
# Start of default section
#

TRGT = 
CC   = $(TRGT)gcc
AS   = $(TRGT)gcc -x assembler-with-cpp

# List all default C defines here, like -D_DEBUG=1
DDEFS = -DSIMULATOR -DSHELL_USE_IPRINTF=FALSE

# List all default ASM defines here, like -D_DEBUG=1
DADEFS =

# List all default directories to look for include files here
DINCDIR =

# List the default directory to look for the libraries here
DLIBDIR =

# List all default libraries here
DLIBS =

#
# End of default section
##############################################################################################

##############################################################################################
# Start of user section
#

# Define project name here
PROJECT = ch

# Define linker script file here
LDSCRIPT =

# List all user C define here, like -D_DEBUG=1
UDEFS =

# Define ASM defines here
UADEFS =

# Imported source files
CHIBIOS = ../..
include $(CHIBIOS)/boards/simulator/board.mk
include ${CHIBIOS}/os/hal/hal.mk
include ${CHIBIOS}/os/hal/platforms/Posix/platform.mk
include ${CHIBIOS}/os/ports/GCC/SIMIA32/port.mk
include ${CHIBIOS}/os/kernel/kernel.mk
include ${CHIBIOS}/test/test.mk

# List C source files here
SRC  = ${PORTSRC} \
       ${KERNSRC} \
       ${TESTSRC} \
       ${HALSRC} \
       ${PLATFORMSRC} \
       $(BOARDSRC) \
       ${CHIBIOS}/os/various/shell.c \
       ${CHIBIOS}/os/various/memstreams.c \
       ${CHIBIOS}/os/various/chprintf.c \
       main.c

# List ASM source files here
ASRC =

# List all user directories here
UINCDIR = $(PORTINC) $(KERNINC) $(TESTINC) \
          $(HALINC) $(PLATFORMINC) $(BOARDINC) \
          ${CHIBIOS}/os/various

# List the user directory to look for the libraries here
ULIBDIR =

# List all user libraries here
ULIBS =

# Define optimisation level here
OPT = -ggdb -O2 -fomit-frame-pointer

#
# End of user defines
##############################################################################################

INCDIR  = $(patsubst %,-I%,$(DINCDIR) $(UINCDIR))
LIBDIR  = $(patsubst %,-L%,$(DLIBDIR) $(ULIBDIR))
DEFS    = $(DDEFS) $(UDEFS)
ADEFS   = $(DADEFS) $(UADEFS)
OBJS    = $(ASRC:.s=.o) $(SRC:.c=.o)
LIBS    = $(DLIBS) $(ULIBS)

ASFLAGS = -Wa,-amhls=$(<:.s=.lst) $(ADEFS)
CPFLAGS = $(OPT) -Wall -Wextra -Wstrict-prototypes -fverbose-asm $(DEFS) 

ifeq ($(HOST_OSX),yes)
  ifeq ($(OSX_SDK),)
    OSX_SDK = /Developer/SDKs/MacOSX10.7.sdk
  endif
  ifeq ($(OSX_ARCH),)
    OSX_ARCH = -mmacosx-version-min=10.3 -arch i386
  endif

  CPFLAGS += -isysroot $(OSX_SDK) $(OSX_ARCH)
  LDFLAGS = -Wl -Map=$(PROJECT).map,-syslibroot,$(OSX_SDK),$(LIBDIR)
  LIBS += $(OSX_ARCH)
else
  # Linux, or other
  CPFLAGS += -m32 -Wa,-alms=$(<:.c=.lst)
  LDFLAGS = -m32 -Wl,-Map=$(PROJECT).map,--cref,--no-warn-mismatch $(LIBDIR)
endif

# Generate dependency information
CPFLAGS += -MD -MP -MF .dep/$(@F).d

#
# makefile rules
#

all: $(OBJS) $(PROJECT)

%.o : %.c
	$(CC) -c $(CPFLAGS) -I . $(INCDIR) $< -o $@

%.o : %.s
	$(AS) -c $(ASFLAGS) $< -o $@

$(PROJECT): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) $(LIBS) -o $@

gcov:
	-mkdir gcov
	$(COV) -u $(subst /,\,$(SRC))
	-mv *.gcov ./gcov

clean:                                      
	-rm -f $(OBJS)
	-rm -f $(PROJECT)
	-rm -f $(PROJECT).map
	-rm -f $(SRC:.c=.c.bak)
	-rm -f $(SRC:.c=.lst)
	-rm -f $(ASRC:.s=.s.bak)
	-rm -f $(ASRC:.s=.lst)
	-rm -fR .dep

#
# Include the dependency files, should be the last of the makefile
#
-include $(shell mkdir .dep 2>/dev/null) $(wildcard .dep/*)

# *** EOF ***
