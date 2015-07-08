# List of all the AT91SAM7 platform files.
PLATFORMSRC = ${CHIBIOS}/os/hal/platforms/AT91SAM7/hal_lld.c \
              ${CHIBIOS}/os/hal/platforms/AT91SAM7/pal_lld.c \
              ${CHIBIOS}/os/hal/platforms/AT91SAM7/i2c_lld.c \
              ${CHIBIOS}/os/hal/platforms/AT91SAM7/adc_lld.c \
              ${CHIBIOS}/os/hal/platforms/AT91SAM7/ext_lld.c \
              ${CHIBIOS}/os/hal/platforms/AT91SAM7/serial_lld.c \
              ${CHIBIOS}/os/hal/platforms/AT91SAM7/spi_lld.c \
              ${CHIBIOS}/os/hal/platforms/AT91SAM7/mac_lld.c \
              ${CHIBIOS}/os/hal/platforms/AT91SAM7/pwm_lld.c \
              ${CHIBIOS}/os/hal/platforms/AT91SAM7/gpt_lld.c \
              ${CHIBIOS}/os/hal/platforms/AT91SAM7/at91sam7_mii.c \
              ${CHIBIOS}/os/hal/platforms/AT91SAM7/at91lib/aic.c

# Required include directories
PLATFORMINC = ${CHIBIOS}/os/hal/platforms/AT91SAM7
