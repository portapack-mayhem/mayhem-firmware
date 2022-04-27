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
 * @file    LPC8xx/spi_lld.c
 * @brief   LPC8xx low level SPI driver code.
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

#if LPC8xx_SPI_USE_SPI0 || defined(__DOXYGEN__)
/** @brief SPI1 driver identifier.*/
SPIDriver SPID1;
#endif

#if LPC8xx_SPI_USE_SPI1 || defined(__DOXYGEN__)
/** @brief SPI2 driver identifier.*/
SPIDriver SPID2;
#endif

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

static void spi_load_txdata(SPIDriver *spip) {

  LPC_SPI_TypeDef *spi = spip->spi;

  if (--spip->txcnt == 0) {
    spi->TXCTRL |= SPI_TXCTRL_EOT;
  }

  if (spip->txptr != NULL) {
    if ((spi->TXCTRL & SPI_TXCTRL_FLEN_MASK) > SPI_TXCTRL_FLEN(8)) {
      const uint16_t *p = spip->txptr;
      spi->TXDAT = *p++;
      spip->txptr = p;
    }
    else {
      const uint8_t *p = spip->txptr;
      spi->TXDAT = *p++;
      spip->txptr = p;
    }
  }
  else
    spi->TXDAT = 0xffff;
}

/**
 * @brief   Common IRQ handler.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 */
static void spi_serve_interrupt(SPIDriver *spip) {

  LPC_SPI_TypeDef *spi = spip->spi;

  if (spi->INTSTAT & (SPI_STAT_RXOV | SPI_STAT_TXUR)) {
    /* The overflow condition should never happen becausepriority is given
       to receive but a hook macro is provided anyway...*/
    LPC8xx_SPI_ERROR_HOOK(spip);
    spi->STAT = (SPI_STAT_RXOV | SPI_STAT_TXUR);
  }

  if (spi->INTSTAT & SPI_STAT_TXRDY) {
    spi_load_txdata( spip );
  }

  if (spip->txcnt == 0) {
    spi->INTENCLR = (SPI_STAT_TXRDY | SPI_STAT_TXUR);
  }

  if (spi->INTSTAT & SPI_STAT_RXRDY) {
    if (spip->rxptr != NULL) {
      if ((spi->TXCTRL & SPI_TXCTRL_FLEN_MASK) > SPI_TXCTRL_FLEN(8)) {
        uint16_t *p = spip->rxptr;
        *p++ = spi->RXDAT;
        spip->rxptr = p;
      }
      else {
        uint8_t *p = spip->rxptr;
        *p++ = spi->RXDAT;
        spip->rxptr = p;
      }
    }
    else
      (void)spi->RXDAT;
      
    if (--spip->rxcnt == 0) {
      chDbgAssert(spip->txcnt == 0,
                  "spi_serve_interrupt(), #1", "counter out of synch");
      /* Stops the IRQ sources.*/
      spi->INTENCLR = (SPI_STAT_RXRDY | SPI_STAT_TXRDY |
                        SPI_STAT_RXOV | SPI_STAT_TXUR);

      /* Portable SPI ISR code defined in the high level driver, note, it is
         a macro.*/
      _spi_isr_code(spip);
      return;
    }
  }

}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

#if LPC8xx_SPI_USE_SPI0 || defined(__DOXYGEN__)
/**
 * @brief   SPI0 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(Vector40) {

  CH_IRQ_PROLOGUE();
  spi_serve_interrupt(&SPID1);
  CH_IRQ_EPILOGUE();
}
#endif

#if LPC8xx_SPI_USE_SPI1 || defined(__DOXYGEN__)
/**
 * @brief   SPI1 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(Vector44) {

  CH_IRQ_PROLOGUE();
  spi_serve_interrupt(&SPID2);
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

#if LPC8xx_SPI_USE_SPI0
  spiObjectInit(&SPID1);
  SPID1.spi = LPC_SPI0;  
#endif

#if LPC8xx_SPI_USE_SPI1
  spiObjectInit(&SPID2);
  SPID2.spi = LPC_SPI1;
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
#if LPC8xx_SPI_USE_SPI0
    if (&SPID1 == spip) {
      LPC_SYSCON->SYSAHBCLKCTRL |= (1<<11);
      LPC_SYSCON->PRESETCTRL |= (1<<0);
      spip->spi->DIV = LPC8xx_SPI_SPI0CLKDIV;
      nvicEnableVector(SPI0_IRQn,
                       CORTEX_PRIORITY_MASK(LPC8xx_SPI_SPI0_IRQ_PRIORITY));
    }
#endif
#if LPC8xx_SPI_USE_SPI1
    if (&SPID2 == spip) {
      LPC_SYSCON->SYSAHBCLKCTRL |= (1<<12);
      LPC_SYSCON->PRESETCTRL |= (1<<1);
      spip->spi->DIV = LPC8xx_SPI_SPI1CLKDIV;
      nvicEnableVector(SPI1_IRQn,
                       CORTEX_PRIORITY_MASK(LPC8xx_SPI_SPI1_IRQ_PRIORITY));
    }
#endif
  }

  spip->spi->DLY    = spip->config->dly;
  spip->spi->TXCTRL = spip->config->txctrl;
  spip->spi->STAT   = (SPI_STAT_RXOV | SPI_STAT_TXUR);
  spip->spi->CFG    = spip->config->cfg | SPI_CFG_ENABLE;
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
    spip->spi->CFG = 0;

#if LPC8xx_SPI_USE_SPI0
    if (&SPID1 == spip) {
      nvicDisableVector(SPI0_IRQn);
      LPC_SYSCON->PRESETCTRL &= ~(1<<0);
      LPC_SYSCON->SYSAHBCLKCTRL &= ~(1<<11);
    }
#endif

#if LPC8xx_SPI_USE_SPI1
    if (&SPID2 == spip) {
      nvicDisableVector(SPI1_IRQn);
      LPC_SYSCON->PRESETCTRL &= ~(1<<1);
      LPC_SYSCON->SYSAHBCLKCTRL &= ~(1<<12);
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
  /* Hardware controls SSEL */
  (void)spip;
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
  /* Hardware controls SSEL */
  (void)spip;
}

/**
 * @brief   Exchanges data on the SPI bus.
 * @details This asynchronous function starts a simultaneous transmit/receive
 *          operation.
 * @post    At the end of the operation the configured callback is invoked.
 * @note    The buffers are organized as uint8_t arrays for data sizes below or
 *          equal to 8 bits else it is organized as uint16_t arrays.
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

  spip->rxptr = rxbuf;
  spip->txptr = txbuf;
  spip->rxcnt = spip->txcnt = n;
  spip->spi->TXCTRL &= ~SPI_TXCTRL_EOT;
  spi_load_txdata(spip);

  if (spip->txcnt == 0)
    spip->spi->INTENSET = (SPI_STAT_RXRDY | SPI_STAT_RXOV);
  else
    spip->spi->INTENSET = (SPI_STAT_RXRDY | SPI_STAT_TXRDY |
                            SPI_STAT_RXOV | SPI_STAT_TXUR);
  
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

  spi_lld_exchange( spip, n, NULL, NULL);
}

/**
 * @brief   Sends data over the SPI bus.
 * @details This asynchronous function starts a transmit operation.
 * @post    At the end of the operation the configured callback is invoked.
 * @note    The buffers are organized as uint8_t arrays for data sizes below or
 *          equal to 8 bits else it is organized as uint16_t arrays.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 * @param[in] n         number of words to send
 * @param[in] txbuf     the pointer to the transmit buffer
 *
 * @notapi
 */
void spi_lld_send(SPIDriver *spip, size_t n, const void *txbuf) {

  spi_lld_exchange( spip, n, txbuf, NULL);
}

/**
 * @brief   Receives data from the SPI bus.
 * @details This asynchronous function starts a receive operation.
 * @post    At the end of the operation the configured callback is invoked.
 * @note    The buffers are organized as uint8_t arrays for data sizes below or
 *          equal to 8 bits else it is organized as uint16_t arrays.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 * @param[in] n         number of words to receive
 * @param[out] rxbuf    the pointer to the receive buffer
 *
 * @notapi
 */
void spi_lld_receive(SPIDriver *spip, size_t n, void *rxbuf) {

  spi_lld_exchange( spip, n, NULL, rxbuf);
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

  spip->spi->TXCTRL |= SPI_TXCTRL_EOT;
  spip->spi->TXDAT = frame;
  while ((spip->spi->STAT & SPI_STAT_RXRDY) == 0)
    ;
  return (uint16_t)spip->spi->RXDAT;
}

#endif /* HAL_USE_SPI */

/** @} */
