# List of the ChibiOS/RT Cortex-M4 LPC43xx port files.
PORTSRC = $(CHIBIOS)/os/ports/GCC/ARMCMx/crt0.c \
          $(CHIBIOS_PORTAPACK)/os/ports/GCC/ARMCMx/LPC43xx_M4/vectors.c \
          ${CHIBIOS}/os/ports/GCC/ARMCMx/chcore.c \
          ${CHIBIOS}/os/ports/GCC/ARMCMx/chcore_v7m.c \
          ${CHIBIOS}/os/ports/common/ARMCMx/nvic.c

PORTASM =

PORTINC = ${CHIBIOS}/os/ports/common/ARMCMx/CMSIS/include \
          ${CHIBIOS}/os/ports/common/ARMCMx \
          ${CHIBIOS}/os/ports/GCC/ARMCMx \
          ${CHIBIOS_PORTAPACK}/os/ports/GCC/ARMCMx/LPC43xx_M4

PORTLD  = ${CHIBIOS_PORTAPACK}/os/ports/GCC/ARMCMx/LPC43xx_M4/ld
