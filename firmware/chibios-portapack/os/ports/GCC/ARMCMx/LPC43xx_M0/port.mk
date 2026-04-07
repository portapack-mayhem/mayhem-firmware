# List of the ChibiOS/RT Cortex-M0 LPC43xx port files.
PORTSRC = $(CHIBIOS)/os/ports/GCC/ARMCMx/crt0.c \
          $(CHIBIOS_PORTAPACK)/os/ports/GCC/ARMCMx/LPC43xx_M0/vectors.c \
          ${CHIBIOS}/os/ports/GCC/ARMCMx/chcore.c \
          ${CHIBIOS}/os/ports/GCC/ARMCMx/chcore_v6m.c \
          ${CHIBIOS}/os/ports/common/ARMCMx/nvic.c

PORTASM =

PORTINC = ${CHIBIOS}/os/ports/common/ARMCMx/CMSIS/include \
          ${CHIBIOS}/os/ports/common/ARMCMx \
          ${CHIBIOS}/os/ports/GCC/ARMCMx \
          ${CHIBIOS_PORTAPACK}/os/ports/GCC/ARMCMx/LPC43xx_M0

PORTLD  = ${CHIBIOS_PORTAPACK}/os/ports/GCC/ARMCMx/LPC43xx_M0/ld
