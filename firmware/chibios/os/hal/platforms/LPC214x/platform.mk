# List of all the LPC214x platform files.
PLATFORMSRC = ${CHIBIOS}/os/hal/platforms/LPC214x/hal_lld.c \
              ${CHIBIOS}/os/hal/platforms/LPC214x/pal_lld.c \
              ${CHIBIOS}/os/hal/platforms/LPC214x/serial_lld.c \
              ${CHIBIOS}/os/hal/platforms/LPC214x/spi_lld.c \
              ${CHIBIOS}/os/hal/platforms/LPC214x/vic.c

# Required include directories
PLATFORMINC = ${CHIBIOS}/os/hal/platforms/LPC214x
