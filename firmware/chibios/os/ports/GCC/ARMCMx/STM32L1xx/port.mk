# List of the ChibiOS/RT Cortex-M3 STM32L1xx port files.
PORTSRC = $(CHIBIOS)/os/ports/GCC/ARMCMx/crt0.c \
          $(CHIBIOS)/os/ports/GCC/ARMCMx/STM32L1xx/vectors.c \
          ${CHIBIOS}/os/ports/GCC/ARMCMx/chcore.c \
          ${CHIBIOS}/os/ports/GCC/ARMCMx/chcore_v7m.c \
          ${CHIBIOS}/os/ports/common/ARMCMx/nvic.c

PORTASM =

PORTINC = ${CHIBIOS}/os/ports/common/ARMCMx/CMSIS/include \
          ${CHIBIOS}/os/ports/common/ARMCMx \
          ${CHIBIOS}/os/ports/GCC/ARMCMx \
          ${CHIBIOS}/os/ports/GCC/ARMCMx/STM32L1xx

PORTLD  = ${CHIBIOS}/os/ports/GCC/ARMCMx/STM32L1xx/ld
