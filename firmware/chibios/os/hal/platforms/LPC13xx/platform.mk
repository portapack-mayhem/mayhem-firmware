# List of all the LPC13xx platform files.
PLATFORMSRC = ${CHIBIOS}/os/hal/platforms/LPC13xx/hal_lld.c \
              ${CHIBIOS}/os/hal/platforms/LPC13xx/gpt_lld.c \
              ${CHIBIOS}/os/hal/platforms/LPC13xx/pal_lld.c \
              ${CHIBIOS}/os/hal/platforms/LPC13xx/serial_lld.c \
              ${CHIBIOS}/os/hal/platforms/LPC13xx/spi_lld.c

# Required include directories
PLATFORMINC = ${CHIBIOS}/os/hal/platforms/LPC13xx
