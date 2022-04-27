# List of the ChibiOS/RT ARM7 AT91SAM7 port files.
PORTSRC = ${CHIBIOS}/os/ports/GCC/ARM/chcore.c

PORTASM = ${CHIBIOS}/os/ports/GCC/ARM/crt0.s \
          ${CHIBIOS}/os/ports/GCC/ARM/chcoreasm.s \
          ${CHIBIOS}/os/ports/GCC/ARM/AT91SAM7/vectors.s

PORTINC = ${CHIBIOS}/os/ports/GCC/ARM \
          ${CHIBIOS}/os/ports/GCC/ARM/AT91SAM7

PORTLD  = ${CHIBIOS}/os/ports/GCC/ARM/AT91SAM7/ld
