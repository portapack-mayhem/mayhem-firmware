# List of all the LPC11Uxx platform files.
PLATFORMSRC = ${CHIBIOS}/os/hal/platforms/LPC11Uxx/hal_lld.c \
              ${CHIBIOS}/os/hal/platforms/LPC11Uxx/gpt_lld.c \
              ${CHIBIOS}/os/hal/platforms/LPC11Uxx/pal_lld.c \
              ${CHIBIOS}/os/hal/platforms/LPC11Uxx/spi_lld.c \
              ${CHIBIOS}/os/hal/platforms/LPC11Uxx/serial_lld.c \
              ${CHIBIOS}/os/hal/platforms/LPC11Uxx/ext_lld.c \
              ${CHIBIOS}/os/hal/platforms/LPC11Uxx/ext_lld_isr.c 

# Required include directories
PLATFORMINC = ${CHIBIOS}/os/hal/platforms/LPC11Uxx
