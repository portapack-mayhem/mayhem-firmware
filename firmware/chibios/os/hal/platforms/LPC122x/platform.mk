# List of all the LPC122x platform files.
PLATFORMSRC = ${CHIBIOS}/os/hal/platforms/LPC122x/hal_lld.c \
              ${CHIBIOS}/os/hal/platforms/LPC122x/gpt_lld.c \
              ${CHIBIOS}/os/hal/platforms/LPC122x/pal_lld.c \
              ${CHIBIOS}/os/hal/platforms/LPC122x/serial_lld.c \
              ${CHIBIOS}/os/hal/platforms/LPC122x/spi_lld.c

# Required include directories
PLATFORMINC = ${CHIBIOS}/os/hal/platforms/LPC122x
