# List of the ChibiOS/RT Cortex-M0 LPC11xx port files.
PORTSRC = $(CHIBIOS)/os/ports/GCC/ARMCMx/crt0.c \
          $(CHIBIOS)/os/ports/GCC/ARMCMx/LPC11xx/vectors.c \
          ${CHIBIOS}/os/ports/GCC/ARMCMx/chcore.c \
          ${CHIBIOS}/os/ports/GCC/ARMCMx/chcore_v6m.c \
          ${CHIBIOS}/os/ports/common/ARMCMx/nvic.c

PORTASM =

PORTINC = ${CHIBIOS}/os/ports/common/ARMCMx/CMSIS/include \
          ${CHIBIOS}/os/ports/common/ARMCMx \
          ${CHIBIOS}/os/ports/GCC/ARMCMx \
          ${CHIBIOS}/os/ports/GCC/ARMCMx/LPC11xx

PORTLD  = ${CHIBIOS}/os/ports/GCC/ARMCMx/LPC11xx/ld
