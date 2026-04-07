# List of all the STM32F37x platform files.
PLATFORMSRC = ${CHIBIOS}/os/hal/platforms/STM32F37x/stm32_dma.c \
              ${CHIBIOS}/os/hal/platforms/STM32F37x/hal_lld.c \
              ${CHIBIOS}/os/hal/platforms/STM32F37x/adc_lld.c \
              ${CHIBIOS}/os/hal/platforms/STM32F37x/ext_lld_isr.c \
              ${CHIBIOS}/os/hal/platforms/STM32/can_lld.c \
              ${CHIBIOS}/os/hal/platforms/STM32/ext_lld.c \
              ${CHIBIOS}/os/hal/platforms/STM32/GPIOv2/pal_lld.c \
              ${CHIBIOS}/os/hal/platforms/STM32/I2Cv2/i2c_lld.c \
              ${CHIBIOS}/os/hal/platforms/STM32/RTCv2/rtc_lld.c \
              ${CHIBIOS}/os/hal/platforms/STM32/SPIv2/spi_lld.c \
              ${CHIBIOS}/os/hal/platforms/STM32/TIMv1/gpt_lld.c \
              ${CHIBIOS}/os/hal/platforms/STM32/TIMv1/icu_lld.c \
              ${CHIBIOS}/os/hal/platforms/STM32/TIMv1/pwm_lld.c \
              ${CHIBIOS}/os/hal/platforms/STM32/USARTv2/serial_lld.c \
              ${CHIBIOS}/os/hal/platforms/STM32/USARTv2/uart_lld.c \
              ${CHIBIOS}/os/hal/platforms/STM32/USBv1/usb_lld.c

# Required include directories
PLATFORMINC = ${CHIBIOS}/os/hal/platforms/STM32F37x \
              ${CHIBIOS}/os/hal/platforms/STM32 \
              ${CHIBIOS}/os/hal/platforms/STM32/GPIOv2 \
              ${CHIBIOS}/os/hal/platforms/STM32/I2Cv2 \
              ${CHIBIOS}/os/hal/platforms/STM32/RTCv2 \
              ${CHIBIOS}/os/hal/platforms/STM32/SPIv2 \
              ${CHIBIOS}/os/hal/platforms/STM32/TIMv1 \
              ${CHIBIOS}/os/hal/platforms/STM32/USARTv2 \
              ${CHIBIOS}/os/hal/platforms/STM32/USBv1
