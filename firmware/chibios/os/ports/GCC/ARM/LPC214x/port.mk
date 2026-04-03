# List of the ChibiOS/RT ARM7 LPC214x port files.
PORTSRC = ${CHIBIOS}/os/ports/GCC/ARM/chcore.c

PORTASM = ${CHIBIOS}/os/ports/GCC/ARM/crt0.s \
          ${CHIBIOS}/os/ports/GCC/ARM/chcoreasm.s \
          ${CHIBIOS}/os/ports/GCC/ARM/LPC214x/vectors.s

PORTINC = ${CHIBIOS}/os/ports/GCC/ARM \
          ${CHIBIOS}/os/ports/GCC/ARM/LPC214x

PORTLD  = ${CHIBIOS}/os/ports/GCC/ARM/LPC214x/ld
