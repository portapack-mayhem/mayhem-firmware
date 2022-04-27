# List of all the LPC11xx platform files.
PLATFORMSRC = ${CHIBIOS}/os/hal/platforms/LPC11xx/hal_lld.c \
              ${CHIBIOS}/os/hal/platforms/LPC11xx/gpt_lld.c \
              ${CHIBIOS}/os/hal/platforms/LPC11xx/pal_lld.c \
              ${CHIBIOS}/os/hal/platforms/LPC11xx/serial_lld.c \
              ${CHIBIOS}/os/hal/platforms/LPC11xx/spi_lld.c

# Required include directories
PLATFORMINC = ${CHIBIOS}/os/hal/platforms/LPC11xx
