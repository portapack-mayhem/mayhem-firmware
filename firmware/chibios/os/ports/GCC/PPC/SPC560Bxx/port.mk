# List of the ChibiOS/RT SPC560Bxx port files.
PORTSRC = ${CHIBIOS}/os/ports/GCC/PPC/chcore.c

PORTASM = ${CHIBIOS}/os/ports/GCC/PPC/SPC560Bxx/bam.s \
          ${CHIBIOS}/os/ports/GCC/PPC/SPC560Bxx/core.s \
          ${CHIBIOS}/os/ports/GCC/PPC/SPC560Bxx/vectors.s \
          ${CHIBIOS}/os/ports/GCC/PPC/ivor.s \
          ${CHIBIOS}/os/ports/GCC/PPC/crt0.s

PORTINC = ${CHIBIOS}/os/ports/GCC/PPC \
          ${CHIBIOS}/os/ports/GCC/PPC/SPC560Bxx

PORTLD  = ${CHIBIOS}/os/ports/GCC/PPC/SPC560Bxx/ld
