# List of all the SPC563Mxx platform files.
PLATFORMSRC = ${CHIBIOS}/os/hal/platforms/SPC563Mxx/hal_lld.c \
              ${CHIBIOS}/os/hal/platforms/SPC5xx/DSPI_v1/spi_lld.c \
              ${CHIBIOS}/os/hal/platforms/SPC5xx/EDMA_v1/spc5_edma.c \
              ${CHIBIOS}/os/hal/platforms/SPC5xx/EQADC_v1/adc_lld.c \
              ${CHIBIOS}/os/hal/platforms/SPC5xx/SIU_v1/pal_lld.c \
              ${CHIBIOS}/os/hal/platforms/SPC5xx/ESCI_v1/serial_lld.c

# Required include directories
PLATFORMINC = ${CHIBIOS}/os/hal/platforms/SPC563Mxx \
              ${CHIBIOS}/os/hal/platforms/SPC5xx/DSPI_v1 \
              ${CHIBIOS}/os/hal/platforms/SPC5xx/EDMA_v1 \
              ${CHIBIOS}/os/hal/platforms/SPC5xx/EQADC_v1 \
              ${CHIBIOS}/os/hal/platforms/SPC5xx/ESCI_v1 \
              ${CHIBIOS}/os/hal/platforms/SPC5xx/SIU_v1
