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
 * @file    STM32/sdc_lld.c
 * @brief   STM32 SDC subsystem low level driver source.
 *
 * @addtogroup SDC
 * @{
 */

/*
 TODO: Try preerase blocks before writing (ACMD23).
 */

#include <string.h>

#include "ch.h"
#include "hal.h"

#if HAL_USE_SDC || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

#define DMA_CHANNEL                                                         \
  STM32_DMA_GETCHANNEL(STM32_SDC_SDIO_DMA_STREAM,                           \
                       STM32_SDC_SDIO_DMA_CHN)

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/** @brief SDCD1 driver identifier.*/
SDCDriver SDCD1;

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

#if STM32_SDC_SDIO_UNALIGNED_SUPPORT
/**
 * @brief   Buffer for temporary storage during unaligned transfers.
 */
static union {
  uint32_t  alignment;
  uint8_t   buf[MMCSD_BLOCK_SIZE];
} u;
#endif /* STM32_SDC_SDIO_UNALIGNED_SUPPORT */

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/**
 * @brief   Prepares card to handle read transaction.
 *
 * @param[in] sdcp      pointer to the @p SDCDriver object
 * @param[in] startblk  first block to read
 * @param[in] n         number of blocks to read
 * @param[in] resp      pointer to the response buffer
 *
 * @return              The operation status.
 * @retval CH_SUCCESS   operation succeeded.
 * @retval CH_FAILED    operation failed.
 *
 * @notapi
 */
static bool_t sdc_lld_prepare_read(SDCDriver *sdcp, uint32_t startblk,
                                   uint32_t n, uint32_t *resp) {

  /* Driver handles data in 512 bytes blocks (just like HC cards). But if we
     have not HC card than we must convert address from blocks to bytes.*/
  if (!(sdcp->cardmode & SDC_MODE_HIGH_CAPACITY))
    startblk *= MMCSD_BLOCK_SIZE;

  if (n > 1) {
    /* Send read multiple blocks command to card.*/
    if (sdc_lld_send_cmd_short_crc(sdcp, MMCSD_CMD_READ_MULTIPLE_BLOCK,
                                   startblk, resp) || MMCSD_R1_ERROR(resp[0]))
      return CH_FAILED;
  }
  else{
    /* Send read single block command.*/
    if (sdc_lld_send_cmd_short_crc(sdcp, MMCSD_CMD_READ_SINGLE_BLOCK,
                                   startblk, resp) || MMCSD_R1_ERROR(resp[0]))
      return CH_FAILED;
  }

  return CH_SUCCESS;
}

/**
 * @brief   Prepares card to handle write transaction.
 *
 * @param[in] sdcp      pointer to the @p SDCDriver object
 * @param[in] startblk  first block to read
 * @param[in] n         number of blocks to write
 * @param[in] resp      pointer to the response buffer
 *
 * @return              The operation status.
 * @retval CH_SUCCESS   operation succeeded.
 * @retval CH_FAILED    operation failed.
 *
 * @notapi
 */
static bool_t sdc_lld_prepare_write(SDCDriver *sdcp, uint32_t startblk,
                                    uint32_t n, uint32_t *resp) {

  /* Driver handles data in 512 bytes blocks (just like HC cards). But if we
     have not HC card than we must convert address from blocks to bytes.*/
  if (!(sdcp->cardmode & SDC_MODE_HIGH_CAPACITY))
    startblk *= MMCSD_BLOCK_SIZE;

  if (n > 1) {
    /* Write multiple blocks command.*/
    if (sdc_lld_send_cmd_short_crc(sdcp, MMCSD_CMD_WRITE_MULTIPLE_BLOCK,
                                   startblk, resp) || MMCSD_R1_ERROR(resp[0]))
      return CH_FAILED;
  }
  else{
    /* Write single block command.*/
    if (sdc_lld_send_cmd_short_crc(sdcp, MMCSD_CMD_WRITE_BLOCK,
                                   startblk, resp) || MMCSD_R1_ERROR(resp[0]))
      return CH_FAILED;
  }

  return CH_SUCCESS;
}

/**
 * @brief   Wait end of data transaction and performs finalizations.
 *
 * @param[in] sdcp      pointer to the @p SDCDriver object
 * @param[in] n         number of blocks in transaction
 * @param[in] resp      pointer to the response buffer
 *
 * @return              The operation status.
 * @retval CH_SUCCESS   operation succeeded.
 * @retval CH_FAILED    operation failed.
 */
static bool_t sdc_lld_wait_transaction_end(SDCDriver *sdcp, uint32_t n,
                                           uint32_t *resp) {

  /* Note the mask is checked before going to sleep because the interrupt
     may have occurred before reaching the critical zone.*/
  chSysLock();
  if (SDIO->MASK != 0) {
    chDbgAssert(sdcp->thread == NULL,
                "sdc_lld_start_data_transaction(), #1", "not NULL");
    sdcp->thread = chThdSelf();
    chSchGoSleepS(THD_STATE_SUSPENDED);
    chDbgAssert(sdcp->thread == NULL,
                "sdc_lld_start_data_transaction(), #2", "not NULL");
  }
  if ((SDIO->STA & SDIO_STA_DATAEND) == 0) {
    chSysUnlock();
    return CH_FAILED;
  }

#if (defined(STM32F4XX) || defined(STM32F2XX))
  /* Wait until DMA channel enabled to be sure that all data transferred.*/
  while (sdcp->dma->stream->CR & STM32_DMA_CR_EN)
    ;

  /* DMA event flags must be manually cleared.*/
  dmaStreamClearInterrupt(sdcp->dma);

  SDIO->ICR = STM32_SDIO_ICR_ALL_FLAGS;
  SDIO->DCTRL = 0;
  chSysUnlock();

  /* Wait until interrupt flags to be cleared.*/
  /*while (((DMA2->LISR) >> (sdcp->dma->ishift)) & STM32_DMA_ISR_TCIF)
    dmaStreamClearInterrupt(sdcp->dma);*/
#else
  /* Waits for transfer completion at DMA level, the the stream is
     disabled and cleared.*/
  dmaWaitCompletion(sdcp->dma);

  SDIO->ICR = STM32_SDIO_ICR_ALL_FLAGS;
  SDIO->DCTRL = 0;
  chSysUnlock();
#endif

  /* Finalize transaction.*/
  if (n > 1)
    return sdc_lld_send_cmd_short_crc(sdcp, MMCSD_CMD_STOP_TRANSMISSION, 0, resp);

  return CH_SUCCESS;
}

/**
 * @brief   Gets SDC errors.
 *
 * @param[in] sdcp      pointer to the @p SDCDriver object
 * @param[in] sta       value of the STA register
 *
 * @notapi
 */
static void sdc_lld_collect_errors(SDCDriver *sdcp, uint32_t sta) {
  uint32_t errors = SDC_NO_ERROR;

  if (sta & SDIO_STA_CCRCFAIL)
    errors |= SDC_CMD_CRC_ERROR;
  if (sta & SDIO_STA_DCRCFAIL)
    errors |= SDC_DATA_CRC_ERROR;
  if (sta & SDIO_STA_CTIMEOUT)
    errors |= SDC_COMMAND_TIMEOUT;
  if (sta & SDIO_STA_DTIMEOUT)
    errors |= SDC_DATA_TIMEOUT;
  if (sta & SDIO_STA_TXUNDERR)
    errors |= SDC_TX_UNDERRUN;
  if (sta & SDIO_STA_RXOVERR)
    errors |= SDC_RX_OVERRUN;
  if (sta & SDIO_STA_STBITERR)
    errors |= SDC_STARTBIT_ERROR;

  sdcp->errors |= errors;
}

/**
 * @brief   Performs clean transaction stopping in case of errors.
 *
 * @param[in] sdcp      pointer to the @p SDCDriver object
 * @param[in] n         number of blocks in transaction
 * @param[in] resp      pointer to the response buffer
 *
 * @notapi
 */
static void sdc_lld_error_cleanup(SDCDriver *sdcp,
                                  uint32_t n,
                                  uint32_t *resp) {
  uint32_t sta = SDIO->STA;

  dmaStreamClearInterrupt(sdcp->dma);
  dmaStreamDisable(sdcp->dma);
  SDIO->ICR   = STM32_SDIO_ICR_ALL_FLAGS;
  SDIO->MASK  = 0;
  SDIO->DCTRL = 0;
  sdc_lld_collect_errors(sdcp, sta);
  if (n > 1)
    sdc_lld_send_cmd_short_crc(sdcp, MMCSD_CMD_STOP_TRANSMISSION, 0, resp);
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

#if !defined(STM32_SDIO_HANDLER)
#error "STM32_SDIO_HANDLER not defined"
#endif
/**
 * @brief   SDIO IRQ handler.
 * @details It just wakes transaction thread. All error  handling performs in
 *          that thread.
 *
 * @isr
 */
CH_IRQ_HANDLER(STM32_SDIO_HANDLER) {

  CH_IRQ_PROLOGUE();

  chSysLockFromIsr()

  /* Disables the source but the status flags are not reset because the
     read/write functions needs to check them.*/
  SDIO->MASK = 0;

  if (SDCD1.thread != NULL) {
    chSchReadyI(SDCD1.thread);
    SDCD1.thread = NULL;
  }

  chSysUnlockFromIsr();

  CH_IRQ_EPILOGUE();
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level SDC driver initialization.
 *
 * @notapi
 */
void sdc_lld_init(void) {

  sdcObjectInit(&SDCD1);
  SDCD1.thread = NULL;
  SDCD1.dma    = STM32_DMA_STREAM(STM32_SDC_SDIO_DMA_STREAM);
#if CH_DBG_ENABLE_ASSERTS
  SDCD1.sdio   = SDIO;
#endif
}

/**
 * @brief   Configures and activates the SDC peripheral.
 *
 * @param[in] sdcp      pointer to the @p SDCDriver object
 *
 * @notapi
 */
void sdc_lld_start(SDCDriver *sdcp) {

  sdcp->dmamode = STM32_DMA_CR_CHSEL(DMA_CHANNEL) |
                  STM32_DMA_CR_PL(STM32_SDC_SDIO_DMA_PRIORITY) |
                  STM32_DMA_CR_PSIZE_WORD |
                  STM32_DMA_CR_MSIZE_WORD |
                  STM32_DMA_CR_MINC;

#if (defined(STM32F4XX) || defined(STM32F2XX))
  sdcp->dmamode |= STM32_DMA_CR_PFCTRL |
                   STM32_DMA_CR_PBURST_INCR4 |
                   STM32_DMA_CR_MBURST_INCR4;
#endif

  if (sdcp->state == BLK_STOP) {
    /* Note, the DMA must be enabled before the IRQs.*/
    bool_t b;
    b = dmaStreamAllocate(sdcp->dma, STM32_SDC_SDIO_IRQ_PRIORITY, NULL, NULL);
    chDbgAssert(!b, "sdc_lld_start(), #1", "stream already allocated");
    dmaStreamSetPeripheral(sdcp->dma, &SDIO->FIFO);
#if (defined(STM32F4XX) || defined(STM32F2XX))
    dmaStreamSetFIFO(sdcp->dma, STM32_DMA_FCR_DMDIS | STM32_DMA_FCR_FTH_FULL);
#endif
    nvicEnableVector(STM32_SDIO_NUMBER,
                     CORTEX_PRIORITY_MASK(STM32_SDC_SDIO_IRQ_PRIORITY));
    rccEnableSDIO(FALSE);
  }

  /* Configuration, card clock is initially stopped.*/
  SDIO->POWER  = 0;
  SDIO->CLKCR  = 0;
  SDIO->DCTRL  = 0;
  SDIO->DTIMER = 0;
}

/**
 * @brief   Deactivates the SDC peripheral.
 *
 * @param[in] sdcp      pointer to the @p SDCDriver object
 *
 * @notapi
 */
void sdc_lld_stop(SDCDriver *sdcp) {

  if (sdcp->state != BLK_STOP) {

    /* SDIO deactivation.*/
    SDIO->POWER  = 0;
    SDIO->CLKCR  = 0;
    SDIO->DCTRL  = 0;
    SDIO->DTIMER = 0;

    /* Clock deactivation.*/
    nvicDisableVector(STM32_SDIO_NUMBER);
    dmaStreamRelease(sdcp->dma);
    rccDisableSDIO(FALSE);
  }
}

/**
 * @brief   Starts the SDIO clock and sets it to init mode (400kHz or less).
 *
 * @param[in] sdcp      pointer to the @p SDCDriver object
 *
 * @notapi
 */
void sdc_lld_start_clk(SDCDriver *sdcp) {

  (void)sdcp;

  /* Initial clock setting: 400kHz, 1bit mode.*/
  SDIO->CLKCR  = STM32_SDIO_DIV_LS;
  SDIO->POWER |= SDIO_POWER_PWRCTRL_0 | SDIO_POWER_PWRCTRL_1;
  SDIO->CLKCR |= SDIO_CLKCR_CLKEN;

  /* Clock activation delay.*/
  chThdSleepMilliseconds(STM32_SDC_CLOCK_ACTIVATION_DELAY);
}

/**
 * @brief   Sets the SDIO clock to data mode (25MHz or less).
 *
 * @param[in] sdcp      pointer to the @p SDCDriver object
 *
 * @notapi
 */
void sdc_lld_set_data_clk(SDCDriver *sdcp) {

  (void)sdcp;

  SDIO->CLKCR = (SDIO->CLKCR & 0xFFFFFF00) | STM32_SDIO_DIV_HS;
}

/**
 * @brief   Stops the SDIO clock.
 *
 * @param[in] sdcp      pointer to the @p SDCDriver object
 *
 * @notapi
 */
void sdc_lld_stop_clk(SDCDriver *sdcp) {

  (void)sdcp;

  SDIO->CLKCR = 0;
  SDIO->POWER = 0;
}

/**
 * @brief   Switches the bus to 4 bits mode.
 *
 * @param[in] sdcp      pointer to the @p SDCDriver object
 * @param[in] mode      bus mode
 *
 * @notapi
 */
void sdc_lld_set_bus_mode(SDCDriver *sdcp, sdcbusmode_t mode) {
  uint32_t clk = SDIO->CLKCR & ~SDIO_CLKCR_WIDBUS;

  (void)sdcp;

  switch (mode) {
  case SDC_MODE_1BIT:
    SDIO->CLKCR = clk;
    break;
  case SDC_MODE_4BIT:
    SDIO->CLKCR = clk | SDIO_CLKCR_WIDBUS_0;
    break;
  case SDC_MODE_8BIT:
    SDIO->CLKCR = clk | SDIO_CLKCR_WIDBUS_1;
    break;
  }
}

/**
 * @brief   Sends an SDIO command with no response expected.
 *
 * @param[in] sdcp      pointer to the @p SDCDriver object
 * @param[in] cmd       card command
 * @param[in] arg       command argument
 *
 * @notapi
 */
void sdc_lld_send_cmd_none(SDCDriver *sdcp, uint8_t cmd, uint32_t arg) {

  (void)sdcp;

  SDIO->ARG = arg;
  SDIO->CMD = (uint32_t)cmd | SDIO_CMD_CPSMEN;
  while ((SDIO->STA & SDIO_STA_CMDSENT) == 0)
    ;
  SDIO->ICR = SDIO_ICR_CMDSENTC;
}

/**
 * @brief   Sends an SDIO command with a short response expected.
 * @note    The CRC is not verified.
 *
 * @param[in] sdcp      pointer to the @p SDCDriver object
 * @param[in] cmd       card command
 * @param[in] arg       command argument
 * @param[out] resp     pointer to the response buffer (one word)
 *
 * @return              The operation status.
 * @retval CH_SUCCESS   operation succeeded.
 * @retval CH_FAILED    operation failed.
 *
 * @notapi
 */
bool_t sdc_lld_send_cmd_short(SDCDriver *sdcp, uint8_t cmd, uint32_t arg,
                              uint32_t *resp) {
  uint32_t sta;

  (void)sdcp;

  SDIO->ARG = arg;
  SDIO->CMD = (uint32_t)cmd | SDIO_CMD_WAITRESP_0 | SDIO_CMD_CPSMEN;
  while (((sta = SDIO->STA) & (SDIO_STA_CMDREND | SDIO_STA_CTIMEOUT |
                               SDIO_STA_CCRCFAIL)) == 0)
    ;
  SDIO->ICR = sta & (SDIO_STA_CMDREND | SDIO_STA_CTIMEOUT | SDIO_STA_CCRCFAIL);
  if ((sta & (SDIO_STA_CTIMEOUT)) != 0) {
    sdc_lld_collect_errors(sdcp, sta);
    return CH_FAILED;
  }
  *resp = SDIO->RESP1;
  return CH_SUCCESS;
}

/**
 * @brief   Sends an SDIO command with a short response expected and CRC.
 *
 * @param[in] sdcp      pointer to the @p SDCDriver object
 * @param[in] cmd       card command
 * @param[in] arg       command argument
 * @param[out] resp     pointer to the response buffer (one word)
 *
 * @return              The operation status.
 * @retval CH_SUCCESS   operation succeeded.
 * @retval CH_FAILED    operation failed.
 *
 * @notapi
 */
bool_t sdc_lld_send_cmd_short_crc(SDCDriver *sdcp, uint8_t cmd, uint32_t arg,
                                  uint32_t *resp) {
  uint32_t sta;

  (void)sdcp;

  SDIO->ARG = arg;
  SDIO->CMD = (uint32_t)cmd | SDIO_CMD_WAITRESP_0 | SDIO_CMD_CPSMEN;
  while (((sta = SDIO->STA) & (SDIO_STA_CMDREND | SDIO_STA_CTIMEOUT |
                               SDIO_STA_CCRCFAIL)) == 0)
    ;
  SDIO->ICR = sta & (SDIO_STA_CMDREND | SDIO_STA_CTIMEOUT | SDIO_STA_CCRCFAIL);
  if ((sta & (SDIO_STA_CTIMEOUT | SDIO_STA_CCRCFAIL)) != 0) {
    sdc_lld_collect_errors(sdcp, sta);
    return CH_FAILED;
  }
  *resp = SDIO->RESP1;
  return CH_SUCCESS;
}

/**
 * @brief   Sends an SDIO command with a long response expected and CRC.
 *
 * @param[in] sdcp      pointer to the @p SDCDriver object
 * @param[in] cmd       card command
 * @param[in] arg       command argument
 * @param[out] resp     pointer to the response buffer (four words)
 *
 * @return              The operation status.
 * @retval CH_SUCCESS   operation succeeded.
 * @retval CH_FAILED    operation failed.
 *
 * @notapi
 */
bool_t sdc_lld_send_cmd_long_crc(SDCDriver *sdcp, uint8_t cmd, uint32_t arg,
                                 uint32_t *resp) {
  uint32_t sta;

  (void)sdcp;

  SDIO->ARG = arg;
  SDIO->CMD = (uint32_t)cmd | SDIO_CMD_WAITRESP_0 | SDIO_CMD_WAITRESP_1 |
                              SDIO_CMD_CPSMEN;
  while (((sta = SDIO->STA) & (SDIO_STA_CMDREND | SDIO_STA_CTIMEOUT |
                               SDIO_STA_CCRCFAIL)) == 0)
    ;
  SDIO->ICR = sta & (SDIO_STA_CMDREND | SDIO_STA_CTIMEOUT | SDIO_STA_CCRCFAIL);
  if ((sta & (STM32_SDIO_STA_ERROR_MASK)) != 0) {
    sdc_lld_collect_errors(sdcp, sta);
    return CH_FAILED;
  }
  /* Save bytes in reverse order because MSB in response comes first.*/
  *resp++ = SDIO->RESP4;
  *resp++ = SDIO->RESP3;
  *resp++ = SDIO->RESP2;
  *resp   = SDIO->RESP1;
  return CH_SUCCESS;
}

/**
 * @brief   Reads one or more blocks.
 *
 * @param[in] sdcp      pointer to the @p SDCDriver object
 * @param[in] startblk  first block to read
 * @param[out] buf      pointer to the read buffer
 * @param[in] n         number of blocks to read
 *
 * @return              The operation status.
 * @retval CH_SUCCESS   operation succeeded.
 * @retval CH_FAILED    operation failed.
 *
 * @notapi
 */
bool_t sdc_lld_read_aligned(SDCDriver *sdcp, uint32_t startblk,
                            uint8_t *buf, uint32_t n) {
  uint32_t resp[1];

  chDbgCheck((n < (0x1000000 / MMCSD_BLOCK_SIZE)), "max transaction size");

  SDIO->DTIMER = STM32_SDC_READ_TIMEOUT;

  /* Checks for errors and waits for the card to be ready for reading.*/
  if (_sdc_wait_for_transfer_state(sdcp))
    return CH_FAILED;

  /* Prepares the DMA channel for writing.*/
  dmaStreamSetMemory0(sdcp->dma, buf);
  dmaStreamSetTransactionSize(sdcp->dma,
                              (n * MMCSD_BLOCK_SIZE) / sizeof (uint32_t));
  dmaStreamSetMode(sdcp->dma, sdcp->dmamode | STM32_DMA_CR_DIR_P2M);
  dmaStreamEnable(sdcp->dma);

  /* Setting up data transfer.*/
  SDIO->ICR   = STM32_SDIO_ICR_ALL_FLAGS;
  SDIO->MASK  = SDIO_MASK_DCRCFAILIE |
                SDIO_MASK_DTIMEOUTIE |
                SDIO_MASK_STBITERRIE |
                SDIO_MASK_RXOVERRIE |
                SDIO_MASK_DATAENDIE;
  SDIO->DLEN  = n * MMCSD_BLOCK_SIZE;

  /* Transaction starts just after DTEN bit setting.*/
  SDIO->DCTRL = SDIO_DCTRL_DTDIR |
                SDIO_DCTRL_DBLOCKSIZE_3 |
                SDIO_DCTRL_DBLOCKSIZE_0 |
                SDIO_DCTRL_DMAEN |
                SDIO_DCTRL_DTEN;

  /* Talk to card what we want from it.*/
  if (sdc_lld_prepare_read(sdcp, startblk, n, resp) == TRUE)
    goto error;
  if (sdc_lld_wait_transaction_end(sdcp, n, resp) == TRUE)
    goto error;

  return CH_SUCCESS;

error:
  sdc_lld_error_cleanup(sdcp, n, resp);
  return CH_FAILED;
}

/**
 * @brief   Writes one or more blocks.
 *
 * @param[in] sdcp      pointer to the @p SDCDriver object
 * @param[in] startblk  first block to write
 * @param[out] buf      pointer to the write buffer
 * @param[in] n         number of blocks to write
 *
 * @return              The operation status.
 * @retval CH_SUCCESS   operation succeeded.
 * @retval CH_FAILED    operation failed.
 *
 * @notapi
 */
bool_t sdc_lld_write_aligned(SDCDriver *sdcp, uint32_t startblk,
                             const uint8_t *buf, uint32_t n) {
  uint32_t resp[1];

  chDbgCheck((n < (0x1000000 / MMCSD_BLOCK_SIZE)), "max transaction size");

  SDIO->DTIMER = STM32_SDC_WRITE_TIMEOUT;

  /* Checks for errors and waits for the card to be ready for writing.*/
  if (_sdc_wait_for_transfer_state(sdcp))
    return CH_FAILED;

  /* Prepares the DMA channel for writing.*/
  dmaStreamSetMemory0(sdcp->dma, buf);
  dmaStreamSetTransactionSize(sdcp->dma,
                              (n * MMCSD_BLOCK_SIZE) / sizeof (uint32_t));
  dmaStreamSetMode(sdcp->dma, sdcp->dmamode | STM32_DMA_CR_DIR_M2P);
  dmaStreamEnable(sdcp->dma);

  /* Setting up data transfer.*/
  SDIO->ICR   = STM32_SDIO_ICR_ALL_FLAGS;
  SDIO->MASK  = SDIO_MASK_DCRCFAILIE |
                SDIO_MASK_DTIMEOUTIE |
                SDIO_MASK_STBITERRIE |
                SDIO_MASK_TXUNDERRIE |
                SDIO_MASK_DATAENDIE;
  SDIO->DLEN  = n * MMCSD_BLOCK_SIZE;

  /* Talk to card what we want from it.*/
  if (sdc_lld_prepare_write(sdcp, startblk, n, resp) == TRUE)
    goto error;

  /* Transaction starts just after DTEN bit setting.*/
  SDIO->DCTRL = SDIO_DCTRL_DBLOCKSIZE_3 |
                SDIO_DCTRL_DBLOCKSIZE_0 |
                SDIO_DCTRL_DMAEN |
                SDIO_DCTRL_DTEN;
  if (sdc_lld_wait_transaction_end(sdcp, n, resp) == TRUE)
    goto error;

  return CH_SUCCESS;

error:
  sdc_lld_error_cleanup(sdcp, n, resp);
  return CH_FAILED;
}

/**
 * @brief   Reads one or more blocks.
 *
 * @param[in] sdcp      pointer to the @p SDCDriver object
 * @param[in] startblk  first block to read
 * @param[out] buf      pointer to the read buffer
 * @param[in] n         number of blocks to read
 *
 * @return              The operation status.
 * @retval CH_SUCCESS   operation succeeded.
 * @retval CH_FAILED    operation failed.
 *
 * @notapi
 */
bool_t sdc_lld_read(SDCDriver *sdcp, uint32_t startblk,
                    uint8_t *buf, uint32_t n) {

#if STM32_SDC_SDIO_UNALIGNED_SUPPORT
  if (((unsigned)buf & 3) != 0) {
    uint32_t i;
    for (i = 0; i < n; i++) {
      if (sdc_lld_read_aligned(sdcp, startblk, u.buf, 1))
        return CH_FAILED;
      memcpy(buf, u.buf, MMCSD_BLOCK_SIZE);
      buf += MMCSD_BLOCK_SIZE;
      startblk++;
    }
    return CH_SUCCESS;
  }
#endif /* STM32_SDC_SDIO_UNALIGNED_SUPPORT */
  return sdc_lld_read_aligned(sdcp, startblk, buf, n);
}

/**
 * @brief   Writes one or more blocks.
 *
 * @param[in] sdcp      pointer to the @p SDCDriver object
 * @param[in] startblk  first block to write
 * @param[out] buf      pointer to the write buffer
 * @param[in] n         number of blocks to write
 *
 * @return              The operation status.
 * @retval CH_SUCCESS  operation succeeded.
 * @retval CH_FAILED    operation failed.
 *
 * @notapi
 */
bool_t sdc_lld_write(SDCDriver *sdcp, uint32_t startblk,
                     const uint8_t *buf, uint32_t n) {

#if STM32_SDC_SDIO_UNALIGNED_SUPPORT
  if (((unsigned)buf & 3) != 0) {
    uint32_t i;
    for (i = 0; i < n; i++) {
      memcpy(u.buf, buf, MMCSD_BLOCK_SIZE);
      buf += MMCSD_BLOCK_SIZE;
      if (sdc_lld_write_aligned(sdcp, startblk, u.buf, 1))
        return CH_FAILED;
      startblk++;
    }
    return CH_SUCCESS;
  }
#endif /* STM32_SDC_SDIO_UNALIGNED_SUPPORT */
  return sdc_lld_write_aligned(sdcp, startblk, buf, n);
}

/**
 * @brief   Waits for card idle condition.
 *
 * @param[in] sdcp      pointer to the @p SDCDriver object
 *
 * @return              The operation status.
 * @retval CH_SUCCESS   the operation succeeded.
 * @retval CH_FAILED    the operation failed.
 *
 * @api
 */
bool_t sdc_lld_sync(SDCDriver *sdcp) {

  /* TODO: Implement.*/
  (void)sdcp;
  return CH_SUCCESS;
}

#endif /* HAL_USE_SDC */

/** @} */
