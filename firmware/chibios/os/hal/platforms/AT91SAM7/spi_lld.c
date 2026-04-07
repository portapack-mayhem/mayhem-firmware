/*
    ChibiOS/RT - Copyright (C) 2006-2013 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

/**
 * @file    AT91SAM7/spi_lld.c
 * @brief   AT91SAM7 low level SPI driver code.
 *
 * @addtogroup SPI
 * @{
 */

#include "ch.h"
#include "hal.h"

#if HAL_USE_SPI || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

#if AT91SAM7_SPI_USE_SPI0 || defined(__DOXYGEN__)
/** @brief SPI1 driver identifier.*/
SPIDriver SPID1;
#endif

#if AT91SAM7_SPI_USE_SPI1 || defined(__DOXYGEN__)
/** @brief SPI2 driver identifier.*/
SPIDriver SPID2;
#endif

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/**
 * @brief   Idle line value.
 * @details This thing's DMA apparently does not allow to *not* increment
 *          the memory pointer so a buffer filled with ones is required
 *          somewhere.
 * @note    This buffer size also limits the maximum transfer size, 512B,
 *          for @p spiReceive() and @p spiIgnore(). @p spiSend() and
 *          @p spiExchange are not affected.
 */
static const uint16_t idle_buf[] = {
  0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
  0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
  0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
  0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
  0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
  0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
  0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
  0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
  0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
  0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
  0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
  0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
  0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
  0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
  0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
  0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
  0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
  0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
  0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
  0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
  0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
  0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
  0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
  0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
  0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
  0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
  0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
  0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
  0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
  0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
  0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
  0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF
};

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/**
 * @brief   Initializes a SPI device.
 */
static void spi_init(AT91PS_SPI spi) {

  /* Software reset must be written twice (errata for revision B parts).*/
  spi->SPI_CR   = AT91C_SPI_SWRST;
  spi->SPI_CR   = AT91C_SPI_SWRST;
  spi->SPI_RCR  = 0;
  spi->SPI_RNCR = 0;
  spi->SPI_TCR  = 0;
  spi->SPI_TNCR = 0;
  spi->SPI_PTCR = AT91C_PDC_RXTDIS | AT91C_PDC_TXTDIS;
  spi->SPI_MR   = AT91C_SPI_MSTR | AT91C_SPI_MODFDIS;
}

#if defined(__GNUC__)
__attribute__((noinline))
#endif
/**
 * @brief   Shared interrupt handling code.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 */
static void spi_lld_serve_interrupt(SPIDriver *spip) {
  uint32_t sr = spip->spi->SPI_SR;

  if ((sr & AT91C_SPI_ENDRX) != 0) {
    (void)spip->spi->SPI_RDR;                   /* Clears eventual overflow.*/
    spip->spi->SPI_PTCR = AT91C_PDC_RXTDIS |
                              AT91C_PDC_TXTDIS; /* PDC disabled.            */
    spip->spi->SPI_IDR  = AT91C_SPI_ENDRX;      /* Interrupt disabled.      */
    spip->spi->SPI_CR   = AT91C_SPI_SPIDIS;     /* SPI disabled.            */
    /* Portable SPI ISR code defined in the high level driver, note, it is
       a macro.*/
    _spi_isr_code(spip);
  }
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

#if AT91SAM7_SPI_USE_SPI0 || defined(__DOXYGEN__)
/**
 * @brief   SPI0 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(SPI0IrqHandler) {

  CH_IRQ_PROLOGUE();
  spi_lld_serve_interrupt(&SPID1);
  AT91C_BASE_AIC->AIC_EOICR = 0;
  CH_IRQ_EPILOGUE();
}
#endif

#if AT91SAM7_SPI_USE_SPI1 || defined(__DOXYGEN__)
/**
 * @brief   SPI1 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(SPI1IrqHandler) {

  CH_IRQ_PROLOGUE();
  spi_lld_serve_interrupt(&SPID2);
  AT91C_BASE_AIC->AIC_EOICR = 0;
  CH_IRQ_EPILOGUE();
}
#endif

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level SPI driver initialization.
 *
 * @notapi
 */
void spi_lld_init(void) {

#if AT91SAM7_SPI_USE_SPI0
  spiObjectInit(&SPID1);
  SPID1.spi = AT91C_BASE_SPI0;
  spi_init(AT91C_BASE_SPI0);
  AT91C_BASE_PIOA->PIO_PDR   = SPI0_MISO | SPI0_MOSI | SPI0_SCK;
  AT91C_BASE_PIOA->PIO_ASR   = SPI0_MISO | SPI0_MOSI | SPI0_SCK;
  AT91C_BASE_PIOA->PIO_PPUDR = SPI0_MISO | SPI0_MOSI | SPI0_SCK;
  AIC_ConfigureIT(AT91C_ID_SPI0,
                  AT91C_AIC_SRCTYPE_HIGH_LEVEL | AT91SAM7_SPI0_PRIORITY,
                  SPI0IrqHandler);
#endif

#if AT91SAM7_SPI_USE_SPI1
  spiObjectInit(&SPID2);
  SPID2.spi = AT91C_BASE_SPI1;
  spi_init(AT91C_BASE_SPI1);
  AT91C_BASE_PIOA->PIO_PDR   = SPI1_MISO | SPI1_MOSI | SPI1_SCK;
  AT91C_BASE_PIOA->PIO_BSR   = SPI1_MISO | SPI1_MOSI | SPI1_SCK;
  AT91C_BASE_PIOA->PIO_PPUDR = SPI1_MISO | SPI1_MOSI | SPI1_SCK;
  AIC_ConfigureIT(AT91C_ID_SPI1,
                  AT91C_AIC_SRCTYPE_HIGH_LEVEL | AT91SAM7_SPI1_PRIORITY,
                  SPI1IrqHandler);
#endif
}

/**
 * @brief   Configures and activates the SPI peripheral.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 *
 * @notapi
 */
void spi_lld_start(SPIDriver *spip) {

  if (spip->state == SPI_STOP) {
#if AT91SAM7_SPI_USE_SPI0
    if (&SPID1 == spip) {
      /* Clock activation.*/
      AT91C_BASE_PMC->PMC_PCER = (1 << AT91C_ID_SPI0);
      /* Enables associated interrupt vector.*/
      AIC_EnableIT(AT91C_ID_SPI0);
      // Setup the chip select pin
      // No bug here but control CS0 automatically anyway
      if (spip->config->ssport == AT91C_BASE_PIOA && spip->config->sspad == SPI0_CS0) {
    	  AT91C_BASE_PIOA->PIO_PDR = 1 << SPI0_CS0;
    	  AT91C_BASE_PIOA->PIO_OER = 1 << SPI0_CS0;
    	  AT91C_BASE_PIOA->PIO_ASR = 1 << SPI0_CS0;
      } else if (spip->config->ssport) {
    	  uint32_t		mask;

    	  mask = 1 << spip->config->sspad;
    	  spip->config->ssport->PIO_SODR = mask;
    	  spip->config->ssport->PIO_OER = mask;
    	  spip->config->ssport->PIO_PER = mask;
    	  spip->config->ssport->PIO_MDDR = mask;
    	  spip->config->ssport->PIO_PPUDR = mask;
      }
    }
#endif
#if AT91SAM7_SPI_USE_SPI1
    if (&SPID2 == spip) {
      /* Clock activation.*/
      AT91C_BASE_PMC->PMC_PCER = (1 << AT91C_ID_SPI1);
      /* Enables associated interrupt vector.*/
      AIC_EnableIT(AT91C_ID_SPI1);
      // Setup the chip select pin
      // A bug in SPI1 requires that the CS be automatically controlled if it matches the CS0 pin.
      //	If it is not automatically controlled SPI1 will turn off when data is written.
      //	If you are not using CS0 make absolutely sure you don't use it as an output pin and
      //	clear it otherwise you will get the bug in anyway.
      if (spip->config->ssport == AT91C_BASE_PIOA && spip->config->sspad == SPI1_CS0) {
    	  AT91C_BASE_PIOA->PIO_PDR = 1 << SPI1_CS0;
    	  AT91C_BASE_PIOA->PIO_OER = 1 << SPI1_CS0;
    	  AT91C_BASE_PIOA->PIO_BSR = 1 << SPI1_CS0;
      } else if (spip->config->ssport) {
    	  uint32_t		mask;

    	  mask = 1 << spip->config->sspad;
    	  spip->config->ssport->PIO_SODR = mask;
    	  spip->config->ssport->PIO_OER = mask;
    	  spip->config->ssport->PIO_PER = mask;
    	  spip->config->ssport->PIO_MDDR = mask;
    	  spip->config->ssport->PIO_PPUDR = mask;
      }
    }
#endif
  }

  /* Configuration.*/
  spip->spi->SPI_CSR[0] = spip->config->csr;

}

/**
 * @brief   Deactivates the SPI peripheral.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 *
 * @notapi
 */
void spi_lld_stop(SPIDriver *spip) {

  if (spip->state != SPI_STOP) {
#if AT91SAM7_SPI_USE_SPI0
    if (&SPID1 == spip) {
      AT91C_BASE_PMC->PMC_PCDR = (1 << AT91C_ID_SPI0);
      AIC_DisableIT(AT91C_ID_SPI0);
    }
#endif
#if AT91SAM7_SPI_USE_SPI1
    if (&SPID1 == spip) {
      AT91C_BASE_PMC->PMC_PCDR = (1 << AT91C_ID_SPI1);
      AIC_DisableIT(AT91C_ID_SPI0);
    }
#endif
  }
}

/**
 * @brief   Asserts the slave select signal and prepares for transfers.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 *
 * @notapi
 */
void spi_lld_select(SPIDriver *spip) {

  // Not needed for CS0 but it doesn't hurt either
  palClearPad(spip->config->ssport, spip->config->sspad);
}

/**
 * @brief   Deasserts the slave select signal.
 * @details The previously selected peripheral is unselected.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 *
 * @notapi
 */
void spi_lld_unselect(SPIDriver *spip) {

  // Not needed for CS0 but it doesn't hurt either
  palSetPad(spip->config->ssport, spip->config->sspad);
}

/**
 * @brief   Ignores data on the SPI bus.
 * @details This function transmits a series of idle words on the SPI bus and
 *          ignores the received data. This function can be invoked even
 *          when a slave select signal has not been yet asserted.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 * @param[in] n         number of words to be ignored
 *
 * @notapi
 */
void spi_lld_ignore(SPIDriver *spip, size_t n) {

  spip->spi->SPI_TCR  = n;
  spip->spi->SPI_RCR  = n;
  spip->spi->SPI_TPR  = (AT91_REG)idle_buf;
  spip->spi->SPI_RPR  = (AT91_REG)idle_buf;
  spip->spi->SPI_IER  = AT91C_SPI_ENDRX;
  spip->spi->SPI_CR   = AT91C_SPI_SPIEN;
  spip->spi->SPI_PTCR = AT91C_PDC_RXTEN | AT91C_PDC_TXTEN;
}

/**
 * @brief   Exchanges data on the SPI bus.
 * @details This function performs a simultaneous transmit/receive operation.
 * @note    The buffers are organized as uint8_t arrays.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 * @param[in] n         number of words to be exchanged
 * @param[in] txbuf     the pointer to the transmit buffer
 * @param[out] rxbuf    the pointer to the receive buffer
 *
 * @notapi
 */
void spi_lld_exchange(SPIDriver *spip, size_t n,
                      const void *txbuf, void *rxbuf) {

  spip->spi->SPI_TCR  = n;
  spip->spi->SPI_RCR  = n;
  spip->spi->SPI_TPR  = (AT91_REG)txbuf;
  spip->spi->SPI_RPR  = (AT91_REG)rxbuf;
  spip->spi->SPI_IER  = AT91C_SPI_ENDRX;
  spip->spi->SPI_CR   = AT91C_SPI_SPIEN;
  spip->spi->SPI_PTCR = AT91C_PDC_RXTEN | AT91C_PDC_TXTEN;
}

/**
 * @brief   Sends data over the SPI bus.
 * @note    The buffers are organized as uint8_t arrays.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 * @param[in] n         number of words to send
 * @param[in] txbuf     the pointer to the transmit buffer
 *
 * @notapi
 */
void spi_lld_send(SPIDriver *spip, size_t n, const void *txbuf) {

  spip->spi->SPI_TCR  = n;
  spip->spi->SPI_RCR  = n;
  spip->spi->SPI_TPR  = (AT91_REG)txbuf;
  spip->spi->SPI_RPR  = (AT91_REG)idle_buf;
  spip->spi->SPI_IER  = AT91C_SPI_ENDRX;
  spip->spi->SPI_CR   = AT91C_SPI_SPIEN;
  spip->spi->SPI_PTCR = AT91C_PDC_RXTEN | AT91C_PDC_TXTEN;
}

/**
 * @brief   Receives data from the SPI bus.
 * @note    The buffers are organized as uint8_t arrays.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 * @param[in] n         number of words to receive
 * @param[out] rxbuf    the pointer to the receive buffer
 *
 * @notapi
 */
void spi_lld_receive(SPIDriver *spip, size_t n, void *rxbuf) {

  spip->spi->SPI_TCR  = n;
  spip->spi->SPI_RCR  = n;
  spip->spi->SPI_TPR  = (AT91_REG)idle_buf;
  spip->spi->SPI_RPR  = (AT91_REG)rxbuf;
  spip->spi->SPI_IER  = AT91C_SPI_ENDRX;
  spip->spi->SPI_CR   = AT91C_SPI_SPIEN;
  spip->spi->SPI_PTCR = AT91C_PDC_RXTEN | AT91C_PDC_TXTEN;
}

/**
 * @brief   Exchanges one frame using a polled wait.
 * @details This synchronous function exchanges one frame using a polled
 *          synchronization method. This function is useful when exchanging
 *          small amount of data on high speed channels, usually in this
 *          situation is much more efficient just wait for completion using
 *          polling than suspending the thread waiting for an interrupt.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 * @param[in] frame     the data frame to send over the SPI bus
 * @return              The received data frame from the SPI bus.
 */
uint16_t spi_lld_polled_exchange(SPIDriver *spip, uint16_t frame) {

  spip->spi->SPI_CR   = AT91C_SPI_SPIEN;
  spip->spi->SPI_TDR = frame;
  while ((spip->spi->SPI_SR & AT91C_SPI_RDRF) == 0)
    ;
  return spip->spi->SPI_RDR;
}

#endif /* HAL_USE_SPI */

/** @} */
