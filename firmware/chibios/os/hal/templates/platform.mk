# List of all the template platform files.
PLATFORMSRC = ${CHIBIOS}/os/hal/templates/hal_lld.c \
              ${CHIBIOS}/os/hal/templates/adc_lld.c \
              ${CHIBIOS}/os/hal/templates/can_lld.c \
              ${CHIBIOS}/os/hal/templates/ext_lld.c \
              ${CHIBIOS}/os/hal/templates/gpt_lld.c \
 			  ${CHIBIOS}/os/hal/templates/i2c_lld.c \
              ${CHIBIOS}/os/hal/templates/icu_lld.c \
              ${CHIBIOS}/os/hal/templates/mac_lld.c \
              ${CHIBIOS}/os/hal/templates/pal_lld.c \
              ${CHIBIOS}/os/hal/templates/pwm_lld.c \
              ${CHIBIOS}/os/hal/templates/sdc_lld.c \
              ${CHIBIOS}/os/hal/templates/serial_lld.c \
              ${CHIBIOS}/os/hal/templates/spi_lld.c \
              ${CHIBIOS}/os/hal/templates/uart_lld.c \
              ${CHIBIOS}/os/hal/templates/usb_lld.c

# Required include directories
PLATFORMINC = ${CHIBIOS}/os/hal/templates
