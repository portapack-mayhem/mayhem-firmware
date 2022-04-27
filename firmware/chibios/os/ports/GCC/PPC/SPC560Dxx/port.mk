# List of the ChibiOS/RT SPC560Dxx port files.
PORTSRC = ${CHIBIOS}/os/ports/GCC/PPC/chcore.c

PORTASM = ${CHIBIOS}/os/ports/GCC/PPC/SPC560Dxx/bam.s \
          ${CHIBIOS}/os/ports/GCC/PPC/SPC560Dxx/core.s \
          ${CHIBIOS}/os/ports/GCC/PPC/SPC560Dxx/vectors.s \
          ${CHIBIOS}/os/ports/GCC/PPC/ivor.s \
          ${CHIBIOS}/os/ports/GCC/PPC/crt0.s

PORTINC = ${CHIBIOS}/os/ports/GCC/PPC \
          ${CHIBIOS}/os/ports/GCC/PPC/SPC560Dxx

PORTLD  = ${CHIBIOS}/os/ports/GCC/PPC/SPC560Dxx/ld
