# List of all the ChibiOS/RT kernel files, there is no need to remove the files
# from this list, you can disable parts of the kernel by editing chconf.h.
KERNSRC = ${CHIBIOS}/os/kernel/src/chsys.c \
          ${CHIBIOS}/os/kernel/src/chdebug.c \
          ${CHIBIOS}/os/kernel/src/chlists.c \
          ${CHIBIOS}/os/kernel/src/chvt.c \
          ${CHIBIOS}/os/kernel/src/chschd.c \
          ${CHIBIOS}/os/kernel/src/chthreads.c \
          ${CHIBIOS}/os/kernel/src/chdynamic.c \
          ${CHIBIOS}/os/kernel/src/chregistry.c \
          ${CHIBIOS}/os/kernel/src/chsem.c \
          ${CHIBIOS}/os/kernel/src/chmtx.c \
          ${CHIBIOS}/os/kernel/src/chcond.c \
          ${CHIBIOS}/os/kernel/src/chevents.c \
          ${CHIBIOS}/os/kernel/src/chmsg.c \
          ${CHIBIOS}/os/kernel/src/chmboxes.c \
          ${CHIBIOS}/os/kernel/src/chqueues.c \
          ${CHIBIOS}/os/kernel/src/chmemcore.c \
          ${CHIBIOS}/os/kernel/src/chheap.c \
          ${CHIBIOS}/os/kernel/src/chmempools.c

# Required include directories
KERNINC = ${CHIBIOS}/os/kernel/include
