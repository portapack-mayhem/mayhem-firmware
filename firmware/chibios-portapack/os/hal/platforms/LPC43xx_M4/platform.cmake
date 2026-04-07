# List of all the LPC43xx M4 platform files.
set(PLATFORMSRC
	${CHIBIOS_PORTAPACK}/os/hal/platforms/LPC43xx_M4/hal_lld.c
	${CHIBIOS_PORTAPACK}/os/hal/platforms/LPC43xx/gpt_lld.c
	${CHIBIOS_PORTAPACK}/os/hal/platforms/LPC43xx/i2c_lld.c
	${CHIBIOS_PORTAPACK}/os/hal/platforms/LPC43xx/pal_lld.c
	${CHIBIOS_PORTAPACK}/os/hal/platforms/LPC43xx/rtc_lld.c
	${CHIBIOS_PORTAPACK}/os/hal/platforms/LPC43xx/sdc_lld.c
	${CHIBIOS_PORTAPACK}/os/hal/platforms/LPC43xx/serial_lld.c
	${CHIBIOS_PORTAPACK}/os/hal/platforms/LPC43xx/spi_lld.c
	${CHIBIOS_PORTAPACK}/os/hal/platforms/LPC43xx/lpc43xx.c
)

# Required include directories
set(PLATFORMINC
	${CHIBIOS_PORTAPACK}/os/hal/platforms/LPC43xx_M4
	${CHIBIOS_PORTAPACK}/os/hal/platforms/LPC43xx
)
