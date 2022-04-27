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
/*
   Concepts and parts of this file have been contributed by Uladzimir Pylinsky
   aka barthess.
 */

/**
 * @file    STM32/I2Cv2/i2c_lld.c
 * @brief   STM32 I2C subsystem low level driver source.
 *
 * @addtogroup I2C
 * @{
 */

#include "ch.h"
#include "hal.h"

#if HAL_USE_I2C || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

#define DMAMODE_COMMON                                                      \
  (STM32_DMA_CR_PSIZE_BYTE | STM32_DMA_CR_MSIZE_BYTE |                      \
   STM32_DMA_CR_MINC       | STM32_DMA_CR_DMEIE      |                      \
   STM32_DMA_CR_TEIE       | STM32_DMA_CR_TCIE)

#define I2C1_RX_DMA_CHANNEL                                                 \
  STM32_DMA_GETCHANNEL(STM32_I2C_I2C1_RX_DMA_STREAM,                        \
                       STM32_I2C1_RX_DMA_CHN)

#define I2C1_TX_DMA_CHANNEL                                                 \
  STM32_DMA_GETCHANNEL(STM32_I2C_I2C1_TX_DMA_STREAM,                        \
                       STM32_I2C1_TX_DMA_CHN)

#define I2C2_RX_DMA_CHANNEL                                                 \
  STM32_DMA_GETCHANNEL(STM32_I2C_I2C2_RX_DMA_STREAM,                        \
                       STM32_I2C2_RX_DMA_CHN)

#define I2C2_TX_DMA_CHANNEL                                                 \
  STM32_DMA_GETCHANNEL(STM32_I2C_I2C2_TX_DMA_STREAM,                        \
                       STM32_I2C2_TX_DMA_CHN)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

#define I2C_MASTER_TC                                                       \
  ((uint32_t)(I2C_ISR_BUSY | I2C_ISR_TC))

#define I2C_ERROR_MASK                                                      \
  ((uint32_t)(I2C_ISR_BERR | I2C_ISR_ARLO | I2C_ISR_OVR | I2C_ISR_PECERR |  \
              I2C_ISR_TIMEOUT | I2C_ISR_ALERT))

#define I2C_INT_MASK                                                        \
  ((uint32_t)(I2C_ISR_TCR | I2C_ISR_TC | I2C_ISR_STOPF | I2C_ISR_NACKF |    \
              I2C_ISR_ADDR | I2C_ISR_RXNE | I2C_ISR_TXIS))

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/** @brief I2C1 driver identifier.*/
#if STM32_I2C_USE_I2C1 || defined(__DOXYGEN__)
I2CDriver I2CD1;
#endif

/** @brief I2C2 driver identifier.*/
#if STM32_I2C_USE_I2C2 || defined(__DOXYGEN__)
I2CDriver I2CD2;
#endif

/*===========================================================================*/
/* Driver local variables.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/**
 * @brief   Wakes up the waiting thread.
 *
 * @param[in] i2cp      pointer to the @p I2CDriver object
 * @param[in] msg       wakeup message
 *
 * @notapi
 */
#define wakeup_isr(i2cp, msg) {                                             \
  chSysLockFromIsr();                                                       \
  if ((i2cp)->thread != NULL) {                                             \
    Thread *tp = (i2cp)->thread;                                            \
    (i2cp)->thread = NULL;                                                  \
    tp->p_u.rdymsg = (msg);                                                 \
    chSchReadyI(tp);                                                        \
  }                                                                         \
  chSysUnlockFromIsr();                                                     \
}

/**
 * @brief   Aborts an I2C transaction.
 *
 * @param[in] i2cp      pointer to the @p I2CDriver object
 *
 * @notapi
 */
static void i2c_lld_abort_operation(I2CDriver *i2cp) {
  I2C_TypeDef *dp = i2cp->i2c;

  if (dp->CR1 & I2C_CR1_PE) {
    /* Stops the I2C peripheral.*/
    dp->CR1 &= ~I2C_CR1_PE;
    while (dp->CR1 & I2C_CR1_PE)
      dp->CR1 &= ~I2C_CR1_PE;
    dp->CR1 |= I2C_CR1_PE;
  }

  /* Stops the associated DMA streams.*/
  dmaStreamDisable(i2cp->dmatx);
  dmaStreamDisable(i2cp->dmarx);
}

/**
 * @brief   Handling of stalled I2C transactions.
 *
 * @param[in] i2cp      pointer to the @p I2CDriver object
 *
 * @notapi
 */
static void i2c_lld_safety_timeout(void *p) {
  I2CDriver *i2cp = (I2CDriver *)p;

  chSysLockFromIsr();
  if (i2cp->thread) {
    Thread *tp = i2cp->thread;
    i2c_lld_abort_operation(i2cp);
    i2cp->thread = NULL;
    tp->p_u.rdymsg = RDY_TIMEOUT;
    chSchReadyI(tp);
  }
  chSysUnlockFromIsr();
}

/**
 * @brief   I2C shared ISR code.
 *
 * @param[in] i2cp      pointer to the @p I2CDriver object
 * @param[in] isr       content of the ISR register to be decoded
 *
 * @notapi
 */
static void i2c_lld_serve_interrupt(I2CDriver *i2cp, uint32_t isr) {
  I2C_TypeDef *dp = i2cp->i2c;

  if ((isr & I2C_ISR_TC) && (i2cp->state == I2C_ACTIVE_TX)) {
    size_t rxbytes;

    /* Make sure no more 'Transfer complete' interrupts.*/
    dp->CR1 &= ~I2C_CR1_TCIE;

    rxbytes = dmaStreamGetTransactionSize(i2cp->dmarx);
    if (rxbytes > 0) {
      i2cp->state = I2C_ACTIVE_RX;

      /* Enable RX DMA */
      dmaStreamEnable(i2cp->dmarx);

      dp->CR2 &= ~I2C_CR2_NBYTES;
      dp->CR2 |= rxbytes << 16;

      /* Starts the read operation.*/
      dp->CR2 |= I2C_CR2_RD_WRN;
      dp->CR2 |= I2C_CR2_START;
    }
    else {
      /* Nothing to receive - send STOP immediately.*/
      dp->CR2 |= I2C_CR2_STOP;
    }
  }
  if (isr & I2C_ISR_NACKF) {
    /* Starts a STOP sequence immediately on error.*/
    dp->CR2 |= I2C_CR2_STOP;

    i2cp->errors |= I2CD_ACK_FAILURE;
  }
  if (isr & I2C_ISR_STOPF) {
    /* Stops the associated DMA streams.*/
    dmaStreamDisable(i2cp->dmatx);
    dmaStreamDisable(i2cp->dmarx);

    if (i2cp->errors) {
      wakeup_isr(i2cp, RDY_RESET);
    }
    else {
      wakeup_isr(i2cp, RDY_OK);
    }
  }
}

/**
 * @brief   DMA RX end IRQ handler.
 *
 * @param[in] i2cp      pointer to the @p I2CDriver object
 * @param[in] flags     pre-shifted content of the ISR register
 *
 * @notapi
 */
static void i2c_lld_serve_rx_end_irq(I2CDriver *i2cp, uint32_t flags) {
  I2C_TypeDef *dp = i2cp->i2c;

  /* DMA errors handling.*/
#if defined(STM32_I2C_DMA_ERROR_HOOK)
  if ((flags & (STM32_DMA_ISR_TEIF | STM32_DMA_ISR_DMEIF)) != 0) {
    STM32_I2C_DMA_ERROR_HOOK(i2cp);
  }
#else
  (void)flags;
#endif

  dmaStreamDisable(i2cp->dmarx);
  dp->CR2 |= I2C_CR2_STOP;
  wakeup_isr(i2cp, RDY_OK);
}

/**
 * @brief    DMA TX end IRQ handler.
 *
 * @param[in] i2cp      pointer to the @p I2CDriver object
 *
 * @notapi
 */
static void i2c_lld_serve_tx_end_irq(I2CDriver *i2cp, uint32_t flags) {

  /* DMA errors handling.*/
#if defined(STM32_I2C_DMA_ERROR_HOOK)
  if ((flags & (STM32_DMA_ISR_TEIF | STM32_DMA_ISR_DMEIF)) != 0) {
    STM32_I2C_DMA_ERROR_HOOK(i2cp);
  }
#else
  (void)flags;
#endif

  dmaStreamDisable(i2cp->dmatx);
}

/**
 * @brief   I2C error handler.
 *
 * @param[in] i2cp      pointer to the @p I2CDriver object
 * @param[in] isr       content of the ISR register to be decoded
 *
 * @notapi
 */
static void i2c_lld_serve_error_interrupt(I2CDriver *i2cp, uint32_t isr) {

  /* Clears interrupt flags just to be safe.*/
  dmaStreamDisable(i2cp->dmatx);
  dmaStreamDisable(i2cp->dmarx);

  if (isr & I2C_ISR_BERR)
    i2cp->errors |= I2CD_BUS_ERROR;

  if (isr & I2C_ISR_ARLO)
    i2cp->errors |= I2CD_ARBITRATION_LOST;

  if (isr & I2C_ISR_OVR)
    i2cp->errors |= I2CD_OVERRUN;

  if (isr & I2C_ISR_TIMEOUT)
    i2cp->errors |= I2CD_TIMEOUT;

  /* If some error has been identified then sends wakes the waiting thread.*/
  if (i2cp->errors != I2CD_NO_ERROR)
    wakeup_isr(i2cp, RDY_RESET);
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

#if STM32_I2C_USE_I2C1 || defined(__DOXYGEN__)
#if defined(STM32_I2C1_GLOBAL_HANDLER) || defined(__DOXYGEN__)
/**
 * @brief   I2C1 event interrupt handler.
 *
 * @notapi
 */
CH_IRQ_HANDLER(STM32_I2C1_GLOBAL_HANDLER) {
  uint32_t isr = I2CD1.i2c->ISR;

  CH_IRQ_PROLOGUE();

  /* Clearing IRQ bits.*/
  I2CD1.i2c->ICR = isr;

  if (isr & I2C_ERROR_MASK)
    i2c_lld_serve_error_interrupt(&I2CD1, isr);
  else if (isr & I2C_INT_MASK)
    i2c_lld_serve_interrupt(&I2CD1, isr);

  CH_IRQ_EPILOGUE();
}

#elif defined(STM32_I2C1_EVENT_HANDLER) && defined(STM32_I2C1_ERROR_HANDLER)
CH_IRQ_HANDLER(STM32_I2C1_EVENT_HANDLER) {
  uint32_t isr = I2CD1.i2c->ISR;

  CH_IRQ_PROLOGUE();

  /* Clearing IRQ bits.*/
  I2CD1.i2c->ICR = isr & I2C_INT_MASK;

  i2c_lld_serve_interrupt(&I2CD1, isr);

  CH_IRQ_EPILOGUE();
}

CH_IRQ_HANDLER(STM32_I2C1_ERROR_HANDLER) {
  uint32_t isr = I2CD1.i2c->ISR;

  CH_IRQ_PROLOGUE();

  /* Clearing IRQ bits.*/
  I2CD1.i2c->ICR = isr & I2C_ERROR_MASK;

  i2c_lld_serve_error_interrupt(&I2CD1, isr);

  CH_IRQ_EPILOGUE();
}

#else
#error "I2C1 interrupt handlers not defined"
#endif
#endif /* STM32_I2C_USE_I2C1 */

#if STM32_I2C_USE_I2C2 || defined(__DOXYGEN__)
#if defined(STM32_I2C2_GLOBAL_HANDLER) || defined(__DOXYGEN__)
/**
 * @brief   I2C2 event interrupt handler.
 *
 * @notapi
 */
CH_IRQ_HANDLER(STM32_I2C2_GLOBAL_HANDLER) {
  uint32_t isr = I2CD2.i2c->ISR;

  CH_IRQ_PROLOGUE();

  /* Clearing IRQ bits.*/
  I2CD2.i2c->ICR = isr;

  if (isr & I2C_ERROR_MASK)
    i2c_lld_serve_error_interrupt(&I2CD2, isr);
  else if (isr & I2C_INT_MASK)
    i2c_lld_serve_interrupt(&I2CD2, isr);

  CH_IRQ_EPILOGUE();
}

#elif defined(STM32_I2C2_EVENT_HANDLER) && defined(STM32_I2C2_ERROR_HANDLER)
CH_IRQ_HANDLER(STM32_I2C2_EVENT_HANDLER) {
  uint32_t isr = I2CD2.i2c->ISR;

  CH_IRQ_PROLOGUE();

  /* Clearing IRQ bits.*/
  I2CD2.i2c->ICR = isr & I2C_INT_MASK;

  i2c_lld_serve_interrupt(&I2CD2, isr);

  CH_IRQ_EPILOGUE();
}

CH_IRQ_HANDLER(STM32_I2C2_ERROR_HANDLER) {
  uint32_t isr = I2CD2.i2c->ISR;

  CH_IRQ_PROLOGUE();

  /* Clearing IRQ bits.*/
  I2CD2.i2c->ICR = isr & I2C_ERROR_MASK;

  i2c_lld_serve_error_interrupt(&I2CD2, isr);

  CH_IRQ_EPILOGUE();
}

#else
#error "I2C2 interrupt handlers not defined"
#endif
#endif /* STM32_I2C_USE_I2C2 */

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level I2C driver initialization.
 *
 * @notapi
 */
void i2c_lld_init(void) {

#if STM32_I2C_USE_I2C1
  i2cObjectInit(&I2CD1);
  I2CD1.thread = NULL;
  I2CD1.i2c    = I2C1;
  I2CD1.dmarx  = STM32_DMA_STREAM(STM32_I2C_I2C1_RX_DMA_STREAM);
  I2CD1.dmatx  = STM32_DMA_STREAM(STM32_I2C_I2C1_TX_DMA_STREAM);
#endif /* STM32_I2C_USE_I2C1 */

#if STM32_I2C_USE_I2C2
  i2cObjectInit(&I2CD2);
  I2CD2.thread = NULL;
  I2CD2.i2c    = I2C2;
  I2CD2.dmarx  = STM32_DMA_STREAM(STM32_I2C_I2C2_RX_DMA_STREAM);
  I2CD2.dmatx  = STM32_DMA_STREAM(STM32_I2C_I2C2_TX_DMA_STREAM);
#endif /* STM32_I2C_USE_I2C2 */
}

/**
 * @brief   Configures and activates the I2C peripheral.
 *
 * @param[in] i2cp      pointer to the @p I2CDriver object
 *
 * @notapi
 */
void i2c_lld_start(I2CDriver *i2cp) {
  I2C_TypeDef *dp = i2cp->i2c;

  i2cp->txdmamode = DMAMODE_COMMON | STM32_DMA_CR_DIR_M2P;
  i2cp->rxdmamode = DMAMODE_COMMON | STM32_DMA_CR_DIR_P2M;

  /* Make sure I2C peripheral is disabled */
  dp->CR1 &= ~I2C_CR1_PE;

  /* If in stopped state then enables the I2C and DMA clocks.*/
  if (i2cp->state == I2C_STOP) {

#if STM32_I2C_USE_I2C1
    if (&I2CD1 == i2cp) {
      bool_t b;

      rccResetI2C1();
      b = dmaStreamAllocate(i2cp->dmarx,
                            STM32_I2C_I2C1_IRQ_PRIORITY,
                            (stm32_dmaisr_t)i2c_lld_serve_rx_end_irq,
                            (void *)i2cp);
      chDbgAssert(!b, "i2c_lld_start(), #1", "stream already allocated");
      b = dmaStreamAllocate(i2cp->dmatx,
                            STM32_I2C_I2C1_IRQ_PRIORITY,
                            (stm32_dmaisr_t)i2c_lld_serve_tx_end_irq,
                            (void *)i2cp);
      chDbgAssert(!b, "i2c_lld_start(), #2", "stream already allocated");
      rccEnableI2C1(FALSE);

#if defined(STM32_I2C1_GLOBAL_NUMBER) || defined(__DOXYGEN__)
      nvicEnableVector(STM32_I2C1_GLOBAL_NUMBER,
                       CORTEX_PRIORITY_MASK(STM32_I2C_I2C1_IRQ_PRIORITY));
#elif defined(STM32_I2C1_EVENT_NUMBER) && defined(STM32_I2C1_ERROR_NUMBER)
      nvicEnableVector(STM32_I2C1_EVENT_NUMBER,
                       CORTEX_PRIORITY_MASK(STM32_I2C_I2C1_IRQ_PRIORITY));
      nvicEnableVector(STM32_I2C1_ERROR_NUMBER,
                       CORTEX_PRIORITY_MASK(STM32_I2C_I2C1_IRQ_PRIORITY));
#else
#error "I2C1 interrupt numbers not defined"
#endif

      i2cp->rxdmamode |= STM32_DMA_CR_CHSEL(I2C1_RX_DMA_CHANNEL) |
                         STM32_DMA_CR_PL(STM32_I2C_I2C1_DMA_PRIORITY);
      i2cp->txdmamode |= STM32_DMA_CR_CHSEL(I2C1_TX_DMA_CHANNEL) |
                         STM32_DMA_CR_PL(STM32_I2C_I2C1_DMA_PRIORITY);
    }
#endif /* STM32_I2C_USE_I2C1 */

#if STM32_I2C_USE_I2C2
    if (&I2CD2 == i2cp) {
      bool_t b;

      rccResetI2C2();
      b = dmaStreamAllocate(i2cp->dmarx,
                            STM32_I2C_I2C2_IRQ_PRIORITY,
                            (stm32_dmaisr_t)i2c_lld_serve_rx_end_irq,
                            (void *)i2cp);
      chDbgAssert(!b, "i2c_lld_start(), #3", "stream already allocated");
      b = dmaStreamAllocate(i2cp->dmatx,
                            STM32_I2C_I2C2_IRQ_PRIORITY,
                            (stm32_dmaisr_t)i2c_lld_serve_tx_end_irq,
                            (void *)i2cp);
      chDbgAssert(!b, "i2c_lld_start(), #4", "stream already allocated");
      rccEnableI2C2(FALSE);

#if defined(STM32_I2C2_GLOBAL_NUMBER) || defined(__DOXYGEN__)
      nvicEnableVector(STM32_I2C2_GLOBAL_NUMBER,
                       CORTEX_PRIORITY_MASK(STM32_I2C_I2C2_IRQ_PRIORITY));
#elif defined(STM32_I2C2_EVENT_NUMBER) && defined(STM32_I2C2_ERROR_NUMBER)
      nvicEnableVector(STM32_I2C2_EVENT_NUMBER,
                       CORTEX_PRIORITY_MASK(STM32_I2C_I2C2_IRQ_PRIORITY));
      nvicEnableVector(STM32_I2C2_ERROR_NUMBER,
                       CORTEX_PRIORITY_MASK(STM32_I2C_I2C2_IRQ_PRIORITY));
#else
#error "I2C2 interrupt numbers not defined"
#endif

      i2cp->rxdmamode |= STM32_DMA_CR_CHSEL(I2C2_RX_DMA_CHANNEL) |
                         STM32_DMA_CR_PL(STM32_I2C_I2C2_DMA_PRIORITY);
      i2cp->txdmamode |= STM32_DMA_CR_CHSEL(I2C2_TX_DMA_CHANNEL) |
                         STM32_DMA_CR_PL(STM32_I2C_I2C2_DMA_PRIORITY);
    }
#endif /* STM32_I2C_USE_I2C2 */
  }

  /* I2C registers pointed by the DMA.*/
  dmaStreamSetPeripheral(i2cp->dmarx, &dp->RXDR);
  dmaStreamSetPeripheral(i2cp->dmatx, &dp->TXDR);

  /* Reset i2c peripheral, the TCIE bit will be handled separately.*/
  dp->CR1 = i2cp->config->cr1 | I2C_CR1_ERRIE | I2C_CR1_STOPIE |
            I2C_CR1_NACKIE | I2C_CR1_TXDMAEN | I2C_CR1_RXDMAEN;

  /* Set slave address field (master mode) */
  dp->CR2 = (i2cp->config->cr2 & ~I2C_CR2_SADD);

  /* Setup I2C parameters.*/
  dp->TIMINGR = i2cp->config->timingr;

  /* Ready to go.*/
  dp->CR1 |= I2C_CR1_PE;
}

/**
 * @brief   Deactivates the I2C peripheral.
 *
 * @param[in] i2cp      pointer to the @p I2CDriver object
 *
 * @notapi
 */
void i2c_lld_stop(I2CDriver *i2cp) {

  /* If not in stopped state then disables the I2C clock.*/
  if (i2cp->state != I2C_STOP) {

    /* I2C disable.*/
    i2c_lld_abort_operation(i2cp);
    dmaStreamRelease(i2cp->dmatx);
    dmaStreamRelease(i2cp->dmarx);

#if STM32_I2C_USE_I2C1
    if (&I2CD1 == i2cp) {
#if defined(STM32_I2C1_GLOBAL_NUMBER) || defined(__DOXYGEN__)
      nvicDisableVector(STM32_I2C1_GLOBAL_NUMBER);
#elif defined(STM32_I2C1_EVENT_NUMBER) && defined(STM32_I2C1_ERROR_NUMBER)
      nvicDisableVector(STM32_I2C1_EVENT_NUMBER);
      nvicDisableVector(STM32_I2C1_ERROR_NUMBER);
#else
#error "I2C1 interrupt numbers not defined"
#endif

      rccDisableI2C1(FALSE);
    }
#endif

#if STM32_I2C_USE_I2C2
    if (&I2CD2 == i2cp) {
#if defined(STM32_I2C2_GLOBAL_NUMBER) || defined(__DOXYGEN__)
      nvicDisableVector(STM32_I2C2_GLOBAL_NUMBER);
#elif defined(STM32_I2C2_EVENT_NUMBER) && defined(STM32_I2C2_ERROR_NUMBER)
      nvicDisableVector(STM32_I2C2_EVENT_NUMBER);
      nvicDisableVector(STM32_I2C2_ERROR_NUMBER);
#else
#error "I2C2 interrupt numbers not defined"
#endif

      rccDisableI2C2(FALSE);
    }
#endif
  }
}

/**
 * @brief   Receives data via the I2C bus as master.
 * @details Number of receiving bytes must be more than 1 on STM32F1x. This is
 *          hardware restriction.
 *
 * @param[in] i2cp      pointer to the @p I2CDriver object
 * @param[in] addr      slave device address
 * @param[out] rxbuf    pointer to the receive buffer
 * @param[in] rxbytes   number of bytes to be received
 * @param[in] timeout   the number of ticks before the operation timeouts,
 *                      the following special values are allowed:
 *                      - @a TIME_INFINITE no timeout.
 *                      .
 * @return              The operation status.
 * @retval RDY_OK       if the function succeeded.
 * @retval RDY_RESET    if one or more I2C errors occurred, the errors can
 *                      be retrieved using @p i2cGetErrors().
 * @retval RDY_TIMEOUT  if a timeout occurred before operation end. <b>After a
 *                      timeout the driver must be stopped and restarted
 *                      because the bus is in an uncertain state</b>.
 *
 * @notapi
 */
msg_t i2c_lld_master_receive_timeout(I2CDriver *i2cp, i2caddr_t addr,
                                     uint8_t *rxbuf, size_t rxbytes,
                                     systime_t timeout) {
  I2C_TypeDef *dp = i2cp->i2c;
  VirtualTimer vt;
  uint32_t addr_cr2 = addr & I2C_CR2_SADD;

  chDbgCheck((rxbytes > 0), "i2c_lld_master_receive_timeout");

  /* Resetting error flags for this transfer.*/
  i2cp->errors = I2CD_NO_ERROR;

  /* Global timeout for the whole operation.*/
  if (timeout != TIME_INFINITE)
    chVTSetI(&vt, timeout, i2c_lld_safety_timeout, (void *)i2cp);

  /* Releases the lock from high level driver.*/
  chSysUnlock();

  /* Waits until BUSY flag is reset and the STOP from the previous operation
     is completed, alternatively for a timeout condition.*/
  while (dp->ISR & I2C_ISR_BUSY) {
    chSysLock();
    if ((timeout != TIME_INFINITE) && !chVTIsArmedI(&vt))
      return RDY_TIMEOUT;
    chSysUnlock();
  }

  /* This lock will be released in high level driver.*/
  chSysLock();

  /* Adjust slave address (master mode) for 7-bit address mode */
  if ((i2cp->config->cr2 & I2C_CR2_ADD10) == 0)
    addr_cr2 = (addr_cr2 & 0x7f) << 1;

  /* Set slave address field (master mode) */
  dp->CR2 &= ~(I2C_CR2_SADD | I2C_CR2_NBYTES);
  dp->CR2 |= (rxbytes << 16) | addr_cr2;

  /* Initializes driver fields */
  i2cp->errors = 0;

  /* RX DMA setup.*/
  dmaStreamSetMode(i2cp->dmarx, i2cp->rxdmamode);
  dmaStreamSetMemory0(i2cp->dmarx, rxbuf);
  dmaStreamSetTransactionSize(i2cp->dmarx, rxbytes);

  /* Enable RX DMA */
  dmaStreamEnable(i2cp->dmarx);

  /* Atomic check on the timer in order to make sure that a timeout didn't
     happen outside the critical zone.*/
  if ((timeout != TIME_INFINITE) && !chVTIsArmedI(&vt))
    return RDY_TIMEOUT;

  /* Starts the operation.*/
  dp->CR2 |= I2C_CR2_RD_WRN;
  dp->CR2 |= I2C_CR2_START;

  /* Waits for the operation completion or a timeout.*/
  i2cp->thread = chThdSelf();
  chSchGoSleepS(THD_STATE_SUSPENDED);
  if ((timeout != TIME_INFINITE) && chVTIsArmedI(&vt))
    chVTResetI(&vt);

  return chThdSelf()->p_u.rdymsg;
}

/**
 * @brief   Transmits data via the I2C bus as master.
 * @details Number of receiving bytes must be 0 or more than 1 on STM32F1x.
 *          This is hardware restriction.
 *
 * @param[in] i2cp      pointer to the @p I2CDriver object
 * @param[in] addr      slave device address
 * @param[in] txbuf     pointer to the transmit buffer
 * @param[in] txbytes   number of bytes to be transmitted
 * @param[out] rxbuf    pointer to the receive buffer
 * @param[in] rxbytes   number of bytes to be received
 * @param[in] timeout   the number of ticks before the operation timeouts,
 *                      the following special values are allowed:
 *                      - @a TIME_INFINITE no timeout.
 *                      .
 * @return              The operation status.
 * @retval RDY_OK       if the function succeeded.
 * @retval RDY_RESET    if one or more I2C errors occurred, the errors can
 *                      be retrieved using @p i2cGetErrors().
 * @retval RDY_TIMEOUT  if a timeout occurred before operation end. <b>After a
 *                      timeout the driver must be stopped and restarted
 *                      because the bus is in an uncertain state</b>.
 *
 * @notapi
 */
msg_t i2c_lld_master_transmit_timeout(I2CDriver *i2cp, i2caddr_t addr,
                                      const uint8_t *txbuf, size_t txbytes,
                                      uint8_t *rxbuf, size_t rxbytes,
                                      systime_t timeout) {
  I2C_TypeDef *dp = i2cp->i2c;
  VirtualTimer vt;
  uint32_t addr_cr2 = addr & I2C_CR2_SADD;

  chDbgCheck(((rxbytes == 0) || ((rxbytes > 0) && (rxbuf != NULL))),
             "i2c_lld_master_transmit_timeout");

  /* Resetting error flags for this transfer.*/
  i2cp->errors = I2CD_NO_ERROR;

  /* Global timeout for the whole operation.*/
  if (timeout != TIME_INFINITE)
    chVTSetI(&vt, timeout, i2c_lld_safety_timeout, (void *)i2cp);

  /* Releases the lock from high level driver.*/
  chSysUnlock();

  /* Waits until BUSY flag is reset and the STOP from the previous operation
     is completed, alternatively for a timeout condition.*/
  while (dp->ISR & I2C_ISR_BUSY) {
    chSysLock();
    if ((timeout != TIME_INFINITE) && !chVTIsArmedI(&vt))
      return RDY_TIMEOUT;
    chSysUnlock();
  }

  /* This lock will be released in high level driver.*/
  chSysLock();

  /* Adjust slave address (master mode) for 7-bit address mode */
  if ((i2cp->config->cr2 & I2C_CR2_ADD10) == 0)
    addr_cr2 = (addr_cr2 & 0x7f) << 1;

  /* Set slave address field (master mode) */
  dp->CR2 &= ~(I2C_CR2_SADD | I2C_CR2_NBYTES);
  dp->CR2 |= (txbytes << 16) | addr_cr2;

  /* Initializes driver fields */
  i2cp->errors = 0;

  /* TX DMA setup.*/
  dmaStreamSetMode(i2cp->dmatx, i2cp->txdmamode);
  dmaStreamSetMemory0(i2cp->dmatx, txbuf);
  dmaStreamSetTransactionSize(i2cp->dmatx, txbytes);

  /* RX DMA setup.*/
  dmaStreamSetMode(i2cp->dmarx, i2cp->rxdmamode);
  dmaStreamSetMemory0(i2cp->dmarx, rxbuf);
  dmaStreamSetTransactionSize(i2cp->dmarx, rxbytes);

  /* Enable TX DMA */
  dmaStreamEnable(i2cp->dmatx);

  /* Atomic check on the timer in order to make sure that a timeout didn't
     happen outside the critical zone.*/
  if ((timeout != TIME_INFINITE) && !chVTIsArmedI(&vt))
    return RDY_TIMEOUT;

  /* Transmission complete interrupt enabled.*/
  dp->CR1 |= I2C_CR1_TCIE;

  /* Starts the operation as the very last thing.*/
  dp->CR2 &= ~I2C_CR2_RD_WRN;
  dp->CR2 |= I2C_CR2_START;

  /* Waits for the operation completion or a timeout.*/
  i2cp->thread = chThdSelf();
  chSchGoSleepS(THD_STATE_SUSPENDED);
  if ((timeout != TIME_INFINITE) && chVTIsArmedI(&vt))
    chVTResetI(&vt);

  return chThdSelf()->p_u.rdymsg;
}

#endif /* HAL_USE_I2C */

/** @} */
