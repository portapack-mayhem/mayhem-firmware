# List of the ChibiOS/RT SPC560Pxx port files.
PORTSRC = ${CHIBIOS}/os/ports/GCC/PPC/chcore.c

PORTASM = ${CHIBIOS}/os/ports/GCC/PPC/SPC560Pxx/bam.s \
          ${CHIBIOS}/os/ports/GCC/PPC/SPC560Pxx/core.s \
          ${CHIBIOS}/os/ports/GCC/PPC/SPC560Pxx/vectors.s \
          ${CHIBIOS}/os/ports/GCC/PPC/ivor.s \
          ${CHIBIOS}/os/ports/GCC/PPC/crt0.s

PORTINC = ${CHIBIOS}/os/ports/GCC/PPC \
          ${CHIBIOS}/os/ports/GCC/PPC/SPC560Pxx

PORTLD  = ${CHIBIOS}/os/ports/GCC/PPC/SPC560Pxx/ld
