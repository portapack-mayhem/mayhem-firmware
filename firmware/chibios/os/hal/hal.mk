# List of all the ChibiOS/RT HAL files, there is no need to remove the files
# from this list, you can disable parts of the HAL by editing halconf.h.
HALSRC = ${CHIBIOS}/os/hal/src/hal.c \
         ${CHIBIOS}/os/hal/src/adc.c \
         ${CHIBIOS}/os/hal/src/can.c \
         ${CHIBIOS}/os/hal/src/ext.c \
         ${CHIBIOS}/os/hal/src/gpt.c \
         ${CHIBIOS}/os/hal/src/i2c.c \
         ${CHIBIOS}/os/hal/src/icu.c \
         ${CHIBIOS}/os/hal/src/mac.c \
         ${CHIBIOS}/os/hal/src/mmc_spi.c \
         ${CHIBIOS}/os/hal/src/mmcsd.c \
         ${CHIBIOS}/os/hal/src/pal.c \
         ${CHIBIOS}/os/hal/src/pwm.c \
         ${CHIBIOS}/os/hal/src/rtc.c \
         ${CHIBIOS}/os/hal/src/sdc.c \
         ${CHIBIOS}/os/hal/src/serial.c \
         ${CHIBIOS}/os/hal/src/serial_usb.c \
         ${CHIBIOS}/os/hal/src/spi.c \
         ${CHIBIOS}/os/hal/src/tm.c \
         ${CHIBIOS}/os/hal/src/uart.c \
         ${CHIBIOS}/os/hal/src/usb.c

# Required include directories
HALINC = ${CHIBIOS}/os/hal/include
