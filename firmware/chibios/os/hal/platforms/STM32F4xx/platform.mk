# List of all the STM32F2xx/STM32F4xx platform files.
PLATFORMSRC = ${CHIBIOS}/os/hal/platforms/STM32F4xx/stm32_dma.c \
              ${CHIBIOS}/os/hal/platforms/STM32F4xx/hal_lld.c \
              ${CHIBIOS}/os/hal/platforms/STM32F4xx/adc_lld.c \
              ${CHIBIOS}/os/hal/platforms/STM32F4xx/ext_lld_isr.c \
              ${CHIBIOS}/os/hal/platforms/STM32/can_lld.c \
              ${CHIBIOS}/os/hal/platforms/STM32/ext_lld.c \
              ${CHIBIOS}/os/hal/platforms/STM32/mac_lld.c \
              ${CHIBIOS}/os/hal/platforms/STM32/sdc_lld.c \
              ${CHIBIOS}/os/hal/platforms/STM32/GPIOv2/pal_lld.c \
              ${CHIBIOS}/os/hal/platforms/STM32/I2Cv1/i2c_lld.c \
              ${CHIBIOS}/os/hal/platforms/STM32/OTGv1/usb_lld.c \
              ${CHIBIOS}/os/hal/platforms/STM32/RTCv2/rtc_lld.c \
              ${CHIBIOS}/os/hal/platforms/STM32/SPIv1/spi_lld.c \
              ${CHIBIOS}/os/hal/platforms/STM32/TIMv1/gpt_lld.c \
              ${CHIBIOS}/os/hal/platforms/STM32/TIMv1/icu_lld.c \
              ${CHIBIOS}/os/hal/platforms/STM32/TIMv1/pwm_lld.c \
              ${CHIBIOS}/os/hal/platforms/STM32/USARTv1/serial_lld.c \
              ${CHIBIOS}/os/hal/platforms/STM32/USARTv1/uart_lld.c

# Required include directories
PLATFORMINC = ${CHIBIOS}/os/hal/platforms/STM32F4xx \
              ${CHIBIOS}/os/hal/platforms/STM32 \
              ${CHIBIOS}/os/hal/platforms/STM32/GPIOv2 \
              ${CHIBIOS}/os/hal/platforms/STM32/I2Cv1 \
              ${CHIBIOS}/os/hal/platforms/STM32/OTGv1 \
              ${CHIBIOS}/os/hal/platforms/STM32/RTCv2 \
              ${CHIBIOS}/os/hal/platforms/STM32/SPIv1 \
              ${CHIBIOS}/os/hal/platforms/STM32/TIMv1 \
              ${CHIBIOS}/os/hal/platforms/STM32/USARTv1
