# List of the ChibiOS/RT SPC564Axx port files.
PORTSRC = ${CHIBIOS}/os/ports/GCC/PPC/chcore.c

PORTASM = ${CHIBIOS}/os/ports/GCC/PPC/SPC564Axx/bam.s \
          ${CHIBIOS}/os/ports/GCC/PPC/SPC564Axx/core.s \
          ${CHIBIOS}/os/ports/GCC/PPC/SPC564Axx/vectors.s \
          ${CHIBIOS}/os/ports/GCC/PPC/ivor.s \
          ${CHIBIOS}/os/ports/GCC/PPC/crt0.s

PORTINC = ${CHIBIOS}/os/ports/GCC/PPC \
          ${CHIBIOS}/os/ports/GCC/PPC/SPC564Axx

PORTLD  = ${CHIBIOS}/os/ports/GCC/PPC/SPC564Axx/ld
