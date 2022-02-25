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
 * @file    STM32/SPIv2/spi_lld.c
 * @brief   STM32 SPI subsystem low level driver source.
 *
 * @addtogroup SPI
 * @{
 */

#include "ch.h"
#include "hal.h"

#if HAL_USE_SPI || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

#define SPI1_RX_DMA_CHANNEL                                                 \
  STM32_DMA_GETCHANNEL(STM32_SPI_SPI1_RX_DMA_STREAM,                        \
                       STM32_SPI1_RX_DMA_CHN)

#define SPI1_TX_DMA_CHANNEL                                                 \
  STM32_DMA_GETCHANNEL(STM32_SPI_SPI1_TX_DMA_STREAM,                        \
                       STM32_SPI1_TX_DMA_CHN)

#define SPI2_RX_DMA_CHANNEL                                                 \
  STM32_DMA_GETCHANNEL(STM32_SPI_SPI2_RX_DMA_STREAM,                        \
                       STM32_SPI2_RX_DMA_CHN)

#define SPI2_TX_DMA_CHANNEL                                                 \
  STM32_DMA_GETCHANNEL(STM32_SPI_SPI2_TX_DMA_STREAM,                        \
                       STM32_SPI2_TX_DMA_CHN)

#define SPI3_RX_DMA_CHANNEL                                                 \
  STM32_DMA_GETCHANNEL(STM32_SPI_SPI3_RX_DMA_STREAM,                        \
                       STM32_SPI3_RX_DMA_CHN)

#define SPI3_TX_DMA_CHANNEL                                                 \
  STM32_DMA_GETCHANNEL(STM32_SPI_SPI3_TX_DMA_STREAM,                        \
                       STM32_SPI3_TX_DMA_CHN)

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/** @brief SPI1 driver identifier.*/
#if STM32_SPI_USE_SPI1 || defined(__DOXYGEN__)
SPIDriver SPID1;
#endif

/** @brief SPI2 driver identifier.*/
#if STM32_SPI_USE_SPI2 || defined(__DOXYGEN__)
SPIDriver SPID2;
#endif

/** @brief SPI3 driver identifier.*/
#if STM32_SPI_USE_SPI3 || defined(__DOXYGEN__)
SPIDriver SPID3;
#endif

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

static uint16_t dummytx;
static uint16_t dummyrx;

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/**
 * @brief   Shared end-of-rx service routine.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 * @param[in] flags     pre-shifted content of the ISR register
 */
static void spi_lld_serve_rx_interrupt(SPIDriver *spip, uint32_t flags) {

  /* DMA errors handling.*/
#if defined(STM32_SPI_DMA_ERROR_HOOK)
  if ((flags & (STM32_DMA_ISR_TEIF | STM32_DMA_ISR_DMEIF)) != 0) {
    STM32_SPI_DMA_ERROR_HOOK(spip);
  }
#else
  (void)flags;
#endif

  /* Stop everything.*/
  dmaStreamDisable(spip->dmatx);
  dmaStreamDisable(spip->dmarx);

  /* Portable SPI ISR code defined in the high level driver, note, it is
     a macro.*/
  _spi_isr_code(spip);
}

/**
 * @brief   Shared end-of-tx service routine.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 * @param[in] flags     pre-shifted content of the ISR register
 */
static void spi_lld_serve_tx_interrupt(SPIDriver *spip, uint32_t flags) {

  /* DMA errors handling.*/
#if defined(STM32_SPI_DMA_ERROR_HOOK)
  (void)spip;
  if ((flags & (STM32_DMA_ISR_TEIF | STM32_DMA_ISR_DMEIF)) != 0) {
    STM32_SPI_DMA_ERROR_HOOK(spip);
  }
#else
  (void)spip;
  (void)flags;
#endif
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level SPI driver initialization.
 *
 * @notapi
 */
void spi_lld_init(void) {

  dummytx = 0xFFFF;

#if STM32_SPI_USE_SPI1
  spiObjectInit(&SPID1);
  SPID1.spi       = SPI1;
  SPID1.dmarx     = STM32_DMA_STREAM(STM32_SPI_SPI1_RX_DMA_STREAM);
  SPID1.dmatx     = STM32_DMA_STREAM(STM32_SPI_SPI1_TX_DMA_STREAM);
  SPID1.rxdmamode = STM32_DMA_CR_CHSEL(SPI1_RX_DMA_CHANNEL) |
                    STM32_DMA_CR_PL(STM32_SPI_SPI1_DMA_PRIORITY) |
                    STM32_DMA_CR_DIR_P2M |
                    STM32_DMA_CR_TCIE |
                    STM32_DMA_CR_DMEIE |
                    STM32_DMA_CR_TEIE;
  SPID1.txdmamode = STM32_DMA_CR_CHSEL(SPI1_TX_DMA_CHANNEL) |
                    STM32_DMA_CR_PL(STM32_SPI_SPI1_DMA_PRIORITY) |
                    STM32_DMA_CR_DIR_M2P |
                    STM32_DMA_CR_DMEIE |
                    STM32_DMA_CR_TEIE;
#endif

#if STM32_SPI_USE_SPI2
  spiObjectInit(&SPID2);
  SPID2.spi       = SPI2;
  SPID2.dmarx     = STM32_DMA_STREAM(STM32_SPI_SPI2_RX_DMA_STREAM);
  SPID2.dmatx     = STM32_DMA_STREAM(STM32_SPI_SPI2_TX_DMA_STREAM);
  SPID2.rxdmamode = STM32_DMA_CR_CHSEL(SPI2_RX_DMA_CHANNEL) |
                    STM32_DMA_CR_PL(STM32_SPI_SPI2_DMA_PRIORITY) |
                    STM32_DMA_CR_DIR_P2M |
                    STM32_DMA_CR_TCIE |
                    STM32_DMA_CR_DMEIE |
                    STM32_DMA_CR_TEIE;
  SPID2.txdmamode = STM32_DMA_CR_CHSEL(SPI2_TX_DMA_CHANNEL) |
                    STM32_DMA_CR_PL(STM32_SPI_SPI2_DMA_PRIORITY) |
                    STM32_DMA_CR_DIR_M2P |
                    STM32_DMA_CR_DMEIE |
                    STM32_DMA_CR_TEIE;
#endif

#if STM32_SPI_USE_SPI3
  spiObjectInit(&SPID3);
  SPID3.spi       = SPI3;
  SPID3.dmarx     = STM32_DMA_STREAM(STM32_SPI_SPI3_RX_DMA_STREAM);
  SPID3.dmatx     = STM32_DMA_STREAM(STM32_SPI_SPI3_TX_DMA_STREAM);
  SPID3.rxdmamode = STM32_DMA_CR_CHSEL(SPI3_RX_DMA_CHANNEL) |
                    STM32_DMA_CR_PL(STM32_SPI_SPI3_DMA_PRIORITY) |
                    STM32_DMA_CR_DIR_P2M |
                    STM32_DMA_CR_TCIE |
                    STM32_DMA_CR_DMEIE |
                    STM32_DMA_CR_TEIE;
  SPID3.txdmamode = STM32_DMA_CR_CHSEL(SPI3_TX_DMA_CHANNEL) |
                    STM32_DMA_CR_PL(STM32_SPI_SPI3_DMA_PRIORITY) |
                    STM32_DMA_CR_DIR_M2P |
                    STM32_DMA_CR_DMEIE |
                    STM32_DMA_CR_TEIE;
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
  uint32_t ds;

  /* If in stopped state then enables the SPI and DMA clocks.*/
  if (spip->state == SPI_STOP) {
#if STM32_SPI_USE_SPI1
    if (&SPID1 == spip) {
      bool_t b;
      b = dmaStreamAllocate(spip->dmarx,
                            STM32_SPI_SPI1_IRQ_PRIORITY,
                            (stm32_dmaisr_t)spi_lld_serve_rx_interrupt,
                            (void *)spip);
      chDbgAssert(!b, "spi_lld_start(), #1", "stream already allocated");
      b = dmaStreamAllocate(spip->dmatx,
                            STM32_SPI_SPI1_IRQ_PRIORITY,
                            (stm32_dmaisr_t)spi_lld_serve_tx_interrupt,
                            (void *)spip);
      chDbgAssert(!b, "spi_lld_start(), #2", "stream already allocated");
      rccEnableSPI1(FALSE);
    }
#endif
#if STM32_SPI_USE_SPI2
    if (&SPID2 == spip) {
      bool_t b;
      b = dmaStreamAllocate(spip->dmarx,
                            STM32_SPI_SPI2_IRQ_PRIORITY,
                            (stm32_dmaisr_t)spi_lld_serve_rx_interrupt,
                            (void *)spip);
      chDbgAssert(!b, "spi_lld_start(), #3", "stream already allocated");
      b = dmaStreamAllocate(spip->dmatx,
                            STM32_SPI_SPI2_IRQ_PRIORITY,
                            (stm32_dmaisr_t)spi_lld_serve_tx_interrupt,
                            (void *)spip);
      chDbgAssert(!b, "spi_lld_start(), #4", "stream already allocated");
      rccEnableSPI2(FALSE);
    }
#endif
#if STM32_SPI_USE_SPI3
    if (&SPID3 == spip) {
      bool_t b;
      b = dmaStreamAllocate(spip->dmarx,
                            STM32_SPI_SPI3_IRQ_PRIORITY,
                            (stm32_dmaisr_t)spi_lld_serve_rx_interrupt,
                            (void *)spip);
      chDbgAssert(!b, "spi_lld_start(), #5", "stream already allocated");
      b = dmaStreamAllocate(spip->dmatx,
                            STM32_SPI_SPI3_IRQ_PRIORITY,
                            (stm32_dmaisr_t)spi_lld_serve_tx_interrupt,
                            (void *)spip);
      chDbgAssert(!b, "spi_lld_start(), #6", "stream already allocated");
      rccEnableSPI3(FALSE);
    }
#endif

    /* DMA setup.*/
    dmaStreamSetPeripheral(spip->dmarx, &spip->spi->DR);
    dmaStreamSetPeripheral(spip->dmatx, &spip->spi->DR);
  }

  /* Configuration-specific DMA setup.*/
  ds = spip->config->cr2 & SPI_CR2_DS;
  if (!ds || (ds <= (SPI_CR2_DS_2 | SPI_CR2_DS_1 | SPI_CR2_DS_0))) {
    /* Frame width is 8 bits or smaller.*/
    spip->rxdmamode = (spip->rxdmamode & ~STM32_DMA_CR_SIZE_MASK) |
                      STM32_DMA_CR_PSIZE_BYTE | STM32_DMA_CR_MSIZE_BYTE;
    spip->txdmamode = (spip->txdmamode & ~STM32_DMA_CR_SIZE_MASK) |
                      STM32_DMA_CR_PSIZE_BYTE | STM32_DMA_CR_MSIZE_BYTE;
  }
  else {
    /* Frame width is larger than 8 bits.*/
    spip->rxdmamode = (spip->rxdmamode & ~STM32_DMA_CR_SIZE_MASK) |
                      STM32_DMA_CR_PSIZE_HWORD | STM32_DMA_CR_MSIZE_HWORD;
    spip->txdmamode = (spip->txdmamode & ~STM32_DMA_CR_SIZE_MASK) |
                      STM32_DMA_CR_PSIZE_HWORD | STM32_DMA_CR_MSIZE_HWORD;
  }
  /* SPI setup and enable.*/
  spip->spi->CR1  = 0;
  spip->spi->CR1  = spip->config->cr1 | SPI_CR1_MSTR | SPI_CR1_SSM |
                    SPI_CR1_SSI;
  spip->spi->CR2  = spip->config->cr2 | SPI_CR2_FRXTH | SPI_CR2_SSOE |
                    SPI_CR2_RXDMAEN | SPI_CR2_TXDMAEN;
  spip->spi->CR1 |= SPI_CR1_SPE;
}

/**
 * @brief   Deactivates the SPI peripheral.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 *
 * @notapi
 */
void spi_lld_stop(SPIDriver *spip) {

  /* If in ready state then disables the SPI clock.*/
  if (spip->state == SPI_READY) {

    /* SPI disable.*/
    spip->spi->CR1 = 0;
    spip->spi->CR2 = 0;
    dmaStreamRelease(spip->dmarx);
    dmaStreamRelease(spip->dmatx);

#if STM32_SPI_USE_SPI1
    if (&SPID1 == spip)
      rccDisableSPI1(FALSE);
#endif
#if STM32_SPI_USE_SPI2
    if (&SPID2 == spip)
      rccDisableSPI2(FALSE);
#endif
#if STM32_SPI_USE_SPI3
    if (&SPID3 == spip)
      rccDisableSPI3(FALSE);
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

  palSetPad(spip->config->ssport, spip->config->sspad);
}

/**
 * @brief   Ignores data on the SPI bus.
 * @details This asynchronous function starts the transmission of a series of
 *          idle words on the SPI bus and ignores the received data.
 * @post    At the end of the operation the configured callback is invoked.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 * @param[in] n         number of words to be ignored
 *
 * @notapi
 */
void spi_lld_ignore(SPIDriver *spip, size_t n) {

  dmaStreamSetMemory0(spip->dmarx, &dummyrx);
  dmaStreamSetTransactionSize(spip->dmarx, n);
  dmaStreamSetMode(spip->dmarx, spip->rxdmamode);

  dmaStreamSetMemory0(spip->dmatx, &dummytx);
  dmaStreamSetTransactionSize(spip->dmatx, n);
  dmaStreamSetMode(spip->dmatx, spip->txdmamode);

  dmaStreamEnable(spip->dmarx);
  dmaStreamEnable(spip->dmatx);
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

  dmaStreamSetMemory0(spip->dmarx, rxbuf);
  dmaStreamSetTransactionSize(spip->dmarx, n);
  dmaStreamSetMode(spip->dmarx, spip->rxdmamode| STM32_DMA_CR_MINC);

  dmaStreamSetMemory0(spip->dmatx, txbuf);
  dmaStreamSetTransactionSize(spip->dmatx, n);
  dmaStreamSetMode(spip->dmatx, spip->txdmamode | STM32_DMA_CR_MINC);

  dmaStreamEnable(spip->dmarx);
  dmaStreamEnable(spip->dmatx);
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

  dmaStreamSetMemory0(spip->dmarx, &dummyrx);
  dmaStreamSetTransactionSize(spip->dmarx, n);
  dmaStreamSetMode(spip->dmarx, spip->rxdmamode);

  dmaStreamSetMemory0(spip->dmatx, txbuf);
  dmaStreamSetTransactionSize(spip->dmatx, n);
  dmaStreamSetMode(spip->dmatx, spip->txdmamode | STM32_DMA_CR_MINC);

  dmaStreamEnable(spip->dmarx);
  dmaStreamEnable(spip->dmatx);
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

  dmaStreamSetMemory0(spip->dmarx, rxbuf);
  dmaStreamSetTransactionSize(spip->dmarx, n);
  dmaStreamSetMode(spip->dmarx, spip->rxdmamode | STM32_DMA_CR_MINC);

  dmaStreamSetMemory0(spip->dmatx, &dummytx);
  dmaStreamSetTransactionSize(spip->dmatx, n);
  dmaStreamSetMode(spip->dmatx, spip->txdmamode);

  dmaStreamEnable(spip->dmarx);
  dmaStreamEnable(spip->dmatx);
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

  /*
   * Data register must be accessed with the appropriate data size.
   * Byte size access (uint8_t *) for transactions that are <= 8-bit.
   * Halfword size access (uint16_t) for transactions that are <= 8-bit.
   */
  if ((spip->config->cr2 & SPI_CR2_DS) <= (SPI_CR2_DS_2 |
                                           SPI_CR2_DS_1 |
                                           SPI_CR2_DS_0)) {
    volatile uint8_t *spidr = (volatile uint8_t *)&spip->spi->DR;
    *spidr = (uint8_t)frame;
    while ((spip->spi->SR & SPI_SR_RXNE) == 0)
      ;
    return (uint16_t)*spidr;
  }
  else {
    spip->spi->DR = frame;
    while ((spip->spi->SR & SPI_SR_RXNE) == 0)
      ;
    return spip->spi->DR;
  }
}

#endif /* HAL_USE_SPI */

/** @} */
