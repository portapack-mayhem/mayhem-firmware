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
 * @file    STM32/I2Cv1/i2c_lld.c
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

#define I2C3_RX_DMA_CHANNEL                                                 \
  STM32_DMA_GETCHANNEL(STM32_I2C_I2C3_RX_DMA_STREAM,                        \
                       STM32_I2C3_RX_DMA_CHN)

#define I2C3_TX_DMA_CHANNEL                                                 \
  STM32_DMA_GETCHANNEL(STM32_I2C_I2C3_TX_DMA_STREAM,                        \
                       STM32_I2C3_TX_DMA_CHN)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

#define I2C_EV5_MASTER_MODE_SELECT                                          \
  ((uint32_t)(((I2C_SR2_MSL | I2C_SR2_BUSY) << 16) | I2C_SR1_SB))

#define I2C_EV6_MASTER_TRA_MODE_SELECTED                                    \
  ((uint32_t)(((I2C_SR2_MSL | I2C_SR2_BUSY | I2C_SR2_TRA) << 16) |          \
              I2C_SR1_ADDR | I2C_SR1_TXE))

#define I2C_EV6_MASTER_REC_MODE_SELECTED                                    \
  ((uint32_t)(((I2C_SR2_MSL | I2C_SR2_BUSY)<< 16) | I2C_SR1_ADDR))

#define I2C_EV8_2_MASTER_BYTE_TRANSMITTED                                   \
  ((uint32_t)(((I2C_SR2_MSL | I2C_SR2_BUSY | I2C_SR2_TRA) << 16) |          \
              I2C_SR1_BTF | I2C_SR1_TXE))

#define I2C_EV9_MASTER_ADD10                                                \
  ((uint32_t)(((I2C_SR2_MSL | I2C_SR2_BUSY) << 16) | I2C_SR1_ADD10))

#define I2C_EV_MASK 0x00FFFFFF

#define I2C_ERROR_MASK                                                      \
  ((uint16_t)(I2C_SR1_BERR | I2C_SR1_ARLO | I2C_SR1_AF | I2C_SR1_OVR |      \
              I2C_SR1_PECERR | I2C_SR1_TIMEOUT | I2C_SR1_SMBALERT))

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

/** @brief I2C3 driver identifier.*/
#if STM32_I2C_USE_I2C3 || defined(__DOXYGEN__)
I2CDriver I2CD3;
#endif

/*===========================================================================*/
/* Driver local variables and types.                                         */
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

  /* Stops the I2C peripheral.*/
  dp->CR1 = I2C_CR1_SWRST;
  dp->CR1 = 0;
  dp->CR2 = 0;
  dp->SR1 = 0;

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
 * @brief   Set clock speed.
 *
 * @param[in] i2cp      pointer to the @p I2CDriver object
 *
 * @notapi
 */
static void i2c_lld_set_clock(I2CDriver *i2cp) {
  I2C_TypeDef *dp = i2cp->i2c;
  uint16_t regCCR, clock_div;
  int32_t clock_speed = i2cp->config->clock_speed;
  i2cdutycycle_t duty = i2cp->config->duty_cycle;

  chDbgCheck((i2cp != NULL) && (clock_speed > 0) && (clock_speed <= 4000000),
             "i2c_lld_set_clock");

  /* CR2 Configuration.*/
  dp->CR2 &= (uint16_t)~I2C_CR2_FREQ;
  dp->CR2 |= (uint16_t)I2C_CLK_FREQ;

  /* CCR Configuration.*/
  regCCR = 0;
  clock_div = I2C_CCR_CCR;

  if (clock_speed <= 100000) {
    /* Configure clock_div in standard mode.*/
    chDbgAssert(duty == STD_DUTY_CYCLE,
                "i2c_lld_set_clock(), #1",
                "Invalid standard mode duty cycle");

    /* Standard mode clock_div calculate: Tlow/Thigh = 1/1.*/
    chDbgAssert((STM32_PCLK1 % (clock_speed * 2)) == 0,
                "i2c_lld_set_clock(), #2",
                "PCLK1 must be divided without remainder");
    clock_div = (uint16_t)(STM32_PCLK1 / (clock_speed * 2));

    chDbgAssert(clock_div >= 0x04,
                "i2c_lld_set_clock(), #3",
                "Clock divider less then 0x04 not allowed");
    regCCR |= (clock_div & I2C_CCR_CCR);

    /* Sets the Maximum Rise Time for standard mode.*/
    dp->TRISE = I2C_CLK_FREQ + 1;
  }
  else if (clock_speed <= 400000) {
    /* Configure clock_div in fast mode.*/
    chDbgAssert((duty == FAST_DUTY_CYCLE_2) || (duty == FAST_DUTY_CYCLE_16_9),
                "i2c_lld_set_clock(), #4",
                "Invalid fast mode duty cycle");

    if (duty == FAST_DUTY_CYCLE_2) {
      /* Fast mode clock_div calculate: Tlow/Thigh = 2/1.*/
      chDbgAssert((STM32_PCLK1 % (clock_speed * 3)) == 0,
                  "i2c_lld_set_clock(), #5",
                  "PCLK1 must be divided without remainder");
      clock_div = (uint16_t)(STM32_PCLK1 / (clock_speed * 3));
    }
    else if (duty == FAST_DUTY_CYCLE_16_9) {
      /* Fast mode clock_div calculate: Tlow/Thigh = 16/9.*/
      chDbgAssert((STM32_PCLK1 % (clock_speed * 25)) == 0,
                  "i2c_lld_set_clock(), #6",
                  "PCLK1 must be divided without remainder");
      clock_div = (uint16_t)(STM32_PCLK1 / (clock_speed * 25));
      regCCR |= I2C_CCR_DUTY;
    }

    chDbgAssert(clock_div >= 0x01,
                    "i2c_lld_set_clock(), #7",
                    "Clock divider less then 0x04 not allowed");
    regCCR |= (I2C_CCR_FS | (clock_div & I2C_CCR_CCR));

    /* Sets the Maximum Rise Time for fast mode.*/
    dp->TRISE = (I2C_CLK_FREQ * 300 / 1000) + 1;
  }

  chDbgAssert((clock_div <= I2C_CCR_CCR),
              "i2c_lld_set_clock(), #8", "the selected clock is too low");

  dp->CCR = regCCR;
}

/**
 * @brief   Set operation mode of I2C hardware.
 *
 * @param[in] i2cp      pointer to the @p I2CDriver object
 *
 * @notapi
 */
static void i2c_lld_set_opmode(I2CDriver *i2cp) {
  I2C_TypeDef *dp = i2cp->i2c;
  i2copmode_t opmode = i2cp->config->op_mode;
  uint16_t regCR1;

  regCR1 = dp->CR1;
  switch (opmode) {
  case OPMODE_I2C:
    regCR1 &= (uint16_t)~(I2C_CR1_SMBUS|I2C_CR1_SMBTYPE);
    break;
  case OPMODE_SMBUS_DEVICE:
    regCR1 |= I2C_CR1_SMBUS;
    regCR1 &= (uint16_t)~(I2C_CR1_SMBTYPE);
    break;
  case OPMODE_SMBUS_HOST:
    regCR1 |= (I2C_CR1_SMBUS|I2C_CR1_SMBTYPE);
    break;
  }
  dp->CR1 = regCR1;
}

/**
 * @brief   I2C shared ISR code.
 *
 * @param[in] i2cp      pointer to the @p I2CDriver object
 *
 * @notapi
 */
static void i2c_lld_serve_event_interrupt(I2CDriver *i2cp) {
  I2C_TypeDef *dp = i2cp->i2c;
  uint32_t regSR2 = dp->SR2;
  uint32_t event = dp->SR1;

  /* Interrupts are disabled just before dmaStreamEnable() because there
     is no need of interrupts until next transaction begin. All the work is
     done by the DMA.*/
  switch (I2C_EV_MASK & (event | (regSR2 << 16))) {
  case I2C_EV5_MASTER_MODE_SELECT:
    if ((i2cp->addr >> 8) > 0) { 
      /* 10-bit address: 1 1 1 1 0 X X R/W */
      dp->DR = 0xF0 | (0x6 & (i2cp->addr >> 8)) | (0x1 & i2cp->addr);
    } else {
      dp->DR = i2cp->addr;
    }
    break;
  case I2C_EV9_MASTER_ADD10:
    /* Set second addr byte (10-bit addressing)*/
    dp->DR = (0xFF & (i2cp->addr >> 1));
    break;
  case I2C_EV6_MASTER_REC_MODE_SELECTED:
    dp->CR2 &= ~I2C_CR2_ITEVTEN;
    dmaStreamEnable(i2cp->dmarx);
    dp->CR2 |= I2C_CR2_LAST;                 /* Needed in receiver mode. */
    if (dmaStreamGetTransactionSize(i2cp->dmarx) < 2)
      dp->CR1 &= ~I2C_CR1_ACK;
    break;
  case I2C_EV6_MASTER_TRA_MODE_SELECTED:
    dp->CR2 &= ~I2C_CR2_ITEVTEN;
    dmaStreamEnable(i2cp->dmatx);
    break;
  case I2C_EV8_2_MASTER_BYTE_TRANSMITTED:
    /* Catches BTF event after the end of transmission.*/
    if (dmaStreamGetTransactionSize(i2cp->dmarx) > 0) {
      /* Starts "read after write" operation, LSB = 1 -> receive.*/
      i2cp->addr |= 0x01;
      dp->CR1 |= I2C_CR1_START | I2C_CR1_ACK;
      return;
    }
    dp->CR2 &= ~I2C_CR2_ITEVTEN;
    dp->CR1 |= I2C_CR1_STOP;
    wakeup_isr(i2cp, RDY_OK);
    break;
  default:
    break;
  }
  /* Clear ADDR flag. */
  if (event & (I2C_SR1_ADDR | I2C_SR1_ADD10))
    (void)dp->SR2;
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

  dp->CR2 &= ~I2C_CR2_LAST;
  dp->CR1 &= ~I2C_CR1_ACK;
  dp->CR1 |= I2C_CR1_STOP;
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
  I2C_TypeDef *dp = i2cp->i2c;

  /* DMA errors handling.*/
#if defined(STM32_I2C_DMA_ERROR_HOOK)
  if ((flags & (STM32_DMA_ISR_TEIF | STM32_DMA_ISR_DMEIF)) != 0) {
    STM32_I2C_DMA_ERROR_HOOK(i2cp);
  }
#else
  (void)flags;
#endif

  dmaStreamDisable(i2cp->dmatx);
  /* Enables interrupts to catch BTF event meaning transmission part complete.
     Interrupt handler will decide to generate STOP or to begin receiving part
     of R/W transaction itself.*/
  dp->CR2 |= I2C_CR2_ITEVTEN;
}

/**
 * @brief   I2C error handler.
 *
 * @param[in] i2cp      pointer to the @p I2CDriver object
 * @param[in]  sr       content of the SR1 register to be decoded
 *
 * @notapi
 */
static void i2c_lld_serve_error_interrupt(I2CDriver *i2cp, uint16_t sr) {

  /* Clears interrupt flags just to be safe.*/
  dmaStreamDisable(i2cp->dmatx);
  dmaStreamDisable(i2cp->dmarx);

  i2cp->errors = I2CD_NO_ERROR;

  if (sr & I2C_SR1_BERR)                            /* Bus error.           */
    i2cp->errors |= I2CD_BUS_ERROR;

  if (sr & I2C_SR1_ARLO)                            /* Arbitration lost.    */
    i2cp->errors |= I2CD_ARBITRATION_LOST;

  if (sr & I2C_SR1_AF) {                            /* Acknowledge fail.    */
    i2cp->i2c->CR2 &= ~I2C_CR2_ITEVTEN;
    i2cp->i2c->CR1 |= I2C_CR1_STOP;                 /* Setting stop bit.    */
    i2cp->errors |= I2CD_ACK_FAILURE;
  }

  if (sr & I2C_SR1_OVR)                             /* Overrun.             */
    i2cp->errors |= I2CD_OVERRUN;

  if (sr & I2C_SR1_TIMEOUT)                         /* SMBus Timeout.       */
    i2cp->errors |= I2CD_TIMEOUT;

  if (sr & I2C_SR1_PECERR)                          /* PEC error.           */
    i2cp->errors |= I2CD_PEC_ERROR;

  if (sr & I2C_SR1_SMBALERT)                        /* SMBus alert.         */
    i2cp->errors |= I2CD_SMB_ALERT;

  /* If some error has been identified then sends wakes the waiting thread.*/
  if (i2cp->errors != I2CD_NO_ERROR)
    wakeup_isr(i2cp, RDY_RESET);
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

#if STM32_I2C_USE_I2C1 || defined(__DOXYGEN__)
/**
 * @brief   I2C1 event interrupt handler.
 *
 * @notapi
 */
CH_IRQ_HANDLER(I2C1_EV_IRQHandler) {

  CH_IRQ_PROLOGUE();

  i2c_lld_serve_event_interrupt(&I2CD1);

  CH_IRQ_EPILOGUE();
}

/**
 * @brief   I2C1 error interrupt handler.
 */
CH_IRQ_HANDLER(I2C1_ER_IRQHandler) {
  uint16_t sr = I2CD1.i2c->SR1;

  CH_IRQ_PROLOGUE();

  I2CD1.i2c->SR1 = ~(sr & I2C_ERROR_MASK);
  i2c_lld_serve_error_interrupt(&I2CD1, sr);

  CH_IRQ_EPILOGUE();
}
#endif /* STM32_I2C_USE_I2C1 */

#if STM32_I2C_USE_I2C2 || defined(__DOXYGEN__)
/**
 * @brief   I2C2 event interrupt handler.
 *
 * @notapi
 */
CH_IRQ_HANDLER(I2C2_EV_IRQHandler) {

  CH_IRQ_PROLOGUE();

  i2c_lld_serve_event_interrupt(&I2CD2);

  CH_IRQ_EPILOGUE();
}

/**
 * @brief   I2C2 error interrupt handler.
 *
 * @notapi
 */
CH_IRQ_HANDLER(I2C2_ER_IRQHandler) {
  uint16_t sr = I2CD2.i2c->SR1;

  CH_IRQ_PROLOGUE();

  I2CD2.i2c->SR1 = ~(sr & I2C_ERROR_MASK);
  i2c_lld_serve_error_interrupt(&I2CD2, sr);

  CH_IRQ_EPILOGUE();
}
#endif /* STM32_I2C_USE_I2C2 */

#if STM32_I2C_USE_I2C3 || defined(__DOXYGEN__)
/**
 * @brief   I2C3 event interrupt handler.
 *
 * @notapi
 */
CH_IRQ_HANDLER(I2C3_EV_IRQHandler) {

  CH_IRQ_PROLOGUE();

  i2c_lld_serve_event_interrupt(&I2CD3);

  CH_IRQ_EPILOGUE();
}

/**
 * @brief   I2C3 error interrupt handler.
 *
 * @notapi
 */
CH_IRQ_HANDLER(I2C3_ER_IRQHandler) {
  uint16_t sr = I2CD3.i2c->SR1;

  CH_IRQ_PROLOGUE();

  I2CD3.i2c->SR1 = ~(sr & I2C_ERROR_MASK);
  i2c_lld_serve_error_interrupt(&I2CD3, sr);

  CH_IRQ_EPILOGUE();
}
#endif /* STM32_I2C_USE_I2C3 */

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

#if STM32_I2C_USE_I2C3
  i2cObjectInit(&I2CD3);
  I2CD3.thread = NULL;
  I2CD3.i2c    = I2C3;
  I2CD3.dmarx  = STM32_DMA_STREAM(STM32_I2C_I2C3_RX_DMA_STREAM);
  I2CD3.dmatx  = STM32_DMA_STREAM(STM32_I2C_I2C3_TX_DMA_STREAM);
#endif /* STM32_I2C_USE_I2C3 */
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

  i2cp->txdmamode = STM32_DMA_CR_PSIZE_BYTE | STM32_DMA_CR_MSIZE_BYTE |
                    STM32_DMA_CR_MINC       | STM32_DMA_CR_DMEIE |
                    STM32_DMA_CR_TEIE       | STM32_DMA_CR_TCIE |
                    STM32_DMA_CR_DIR_M2P;
  i2cp->rxdmamode = STM32_DMA_CR_PSIZE_BYTE | STM32_DMA_CR_MSIZE_BYTE |
                    STM32_DMA_CR_MINC       | STM32_DMA_CR_DMEIE |
                    STM32_DMA_CR_TEIE       | STM32_DMA_CR_TCIE |
                    STM32_DMA_CR_DIR_P2M;

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
      nvicEnableVector(I2C1_EV_IRQn,
          CORTEX_PRIORITY_MASK(STM32_I2C_I2C1_IRQ_PRIORITY));
      nvicEnableVector(I2C1_ER_IRQn,
          CORTEX_PRIORITY_MASK(STM32_I2C_I2C1_IRQ_PRIORITY));

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
      nvicEnableVector(I2C2_EV_IRQn,
          CORTEX_PRIORITY_MASK(STM32_I2C_I2C2_IRQ_PRIORITY));
      nvicEnableVector(I2C2_ER_IRQn,
          CORTEX_PRIORITY_MASK(STM32_I2C_I2C2_IRQ_PRIORITY));

      i2cp->rxdmamode |= STM32_DMA_CR_CHSEL(I2C2_RX_DMA_CHANNEL) |
                       STM32_DMA_CR_PL(STM32_I2C_I2C2_DMA_PRIORITY);
      i2cp->txdmamode |= STM32_DMA_CR_CHSEL(I2C2_TX_DMA_CHANNEL) |
                       STM32_DMA_CR_PL(STM32_I2C_I2C2_DMA_PRIORITY);
    }
#endif /* STM32_I2C_USE_I2C2 */

#if STM32_I2C_USE_I2C3
    if (&I2CD3 == i2cp) {
      bool_t b;

      rccResetI2C3();
      b = dmaStreamAllocate(i2cp->dmarx,
                            STM32_I2C_I2C3_IRQ_PRIORITY,
                            (stm32_dmaisr_t)i2c_lld_serve_rx_end_irq,
                            (void *)i2cp);
      chDbgAssert(!b, "i2c_lld_start(), #5", "stream already allocated");
      b = dmaStreamAllocate(i2cp->dmatx,
                            STM32_I2C_I2C3_IRQ_PRIORITY,
                            (stm32_dmaisr_t)i2c_lld_serve_tx_end_irq,
                            (void *)i2cp);
      chDbgAssert(!b, "i2c_lld_start(), #6", "stream already allocated");
      rccEnableI2C3(FALSE);
      nvicEnableVector(I2C3_EV_IRQn,
          CORTEX_PRIORITY_MASK(STM32_I2C_I2C3_IRQ_PRIORITY));
      nvicEnableVector(I2C3_ER_IRQn,
          CORTEX_PRIORITY_MASK(STM32_I2C_I2C3_IRQ_PRIORITY));

      i2cp->rxdmamode |= STM32_DMA_CR_CHSEL(I2C3_RX_DMA_CHANNEL) |
                       STM32_DMA_CR_PL(STM32_I2C_I2C3_DMA_PRIORITY);
      i2cp->txdmamode |= STM32_DMA_CR_CHSEL(I2C3_TX_DMA_CHANNEL) |
                       STM32_DMA_CR_PL(STM32_I2C_I2C3_DMA_PRIORITY);
    }
#endif /* STM32_I2C_USE_I2C3 */
  }

  /* I2C registers pointed by the DMA.*/
  dmaStreamSetPeripheral(i2cp->dmarx, &dp->DR);
  dmaStreamSetPeripheral(i2cp->dmatx, &dp->DR);

  /* Reset i2c peripheral.*/
  dp->CR1 = I2C_CR1_SWRST;
  dp->CR1 = 0;
  dp->CR2 = I2C_CR2_ITERREN | I2C_CR2_DMAEN;

  /* Setup I2C parameters.*/
  i2c_lld_set_clock(i2cp);
  i2c_lld_set_opmode(i2cp);

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
      nvicDisableVector(I2C1_EV_IRQn);
      nvicDisableVector(I2C1_ER_IRQn);
      rccDisableI2C1(FALSE);
    }
#endif

#if STM32_I2C_USE_I2C2
    if (&I2CD2 == i2cp) {
      nvicDisableVector(I2C2_EV_IRQn);
      nvicDisableVector(I2C2_ER_IRQn);
      rccDisableI2C2(FALSE);
    }
#endif

#if STM32_I2C_USE_I2C3
    if (&I2CD3 == i2cp) {
      nvicDisableVector(I2C3_EV_IRQn);
      nvicDisableVector(I2C3_ER_IRQn);
      rccDisableI2C3(FALSE);
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

#if defined(STM32F1XX_I2C)
  chDbgCheck((rxbytes > 1), "i2c_lld_master_receive_timeout");
#endif

  /* Global timeout for the whole operation.*/
  if (timeout != TIME_INFINITE)
    chVTSetI(&vt, timeout, i2c_lld_safety_timeout, (void *)i2cp);

  /* Releases the lock from high level driver.*/
  chSysUnlock();

  /* Initializes driver fields, LSB = 1 -> receive.*/
  i2cp->addr = (addr << 1) | 0x01;
  i2cp->errors = 0;

  /* RX DMA setup.*/
  dmaStreamSetMode(i2cp->dmarx, i2cp->rxdmamode);
  dmaStreamSetMemory0(i2cp->dmarx, rxbuf);
  dmaStreamSetTransactionSize(i2cp->dmarx, rxbytes);

  /* Waits until BUSY flag is reset and the STOP from the previous operation
     is completed, alternatively for a timeout condition.*/
  while ((dp->SR2 & I2C_SR2_BUSY) || (dp->CR1 & I2C_CR1_STOP)) {
    chSysLock();
    if ((timeout != TIME_INFINITE) && !chVTIsArmedI(&vt))
      return RDY_TIMEOUT;
    chSysUnlock();
  }

  /* This lock will be released in high level driver.*/
  chSysLock();

  /* Atomic check on the timer in order to make sure that a timeout didn't
     happen outside the critical zone.*/
  if ((timeout != TIME_INFINITE) && !chVTIsArmedI(&vt))
    return RDY_TIMEOUT;

  /* Starts the operation.*/
  dp->CR2 |= I2C_CR2_ITEVTEN;
  dp->CR1 |= I2C_CR1_START | I2C_CR1_ACK;

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

#if defined(STM32F1XX_I2C)
  chDbgCheck(((rxbytes == 0) || ((rxbytes > 1) && (rxbuf != NULL))),
             "i2c_lld_master_transmit_timeout");
#endif

  /* Global timeout for the whole operation.*/
  if (timeout != TIME_INFINITE)
    chVTSetI(&vt, timeout, i2c_lld_safety_timeout, (void *)i2cp);

  /* Releases the lock from high level driver.*/
  chSysUnlock();

  /* Initializes driver fields, LSB = 0 -> write.*/
  i2cp->addr = addr << 1;
  i2cp->errors = 0;

  /* TX DMA setup.*/
  dmaStreamSetMode(i2cp->dmatx, i2cp->txdmamode);
  dmaStreamSetMemory0(i2cp->dmatx, txbuf);
  dmaStreamSetTransactionSize(i2cp->dmatx, txbytes);

  /* RX DMA setup.*/
  dmaStreamSetMode(i2cp->dmarx, i2cp->rxdmamode);
  dmaStreamSetMemory0(i2cp->dmarx, rxbuf);
  dmaStreamSetTransactionSize(i2cp->dmarx, rxbytes);

  /* Waits until BUSY flag is reset and the STOP from the previous operation
     is completed, alternatively for a timeout condition.*/
  while ((dp->SR2 & I2C_SR2_BUSY) || (dp->CR1 & I2C_CR1_STOP)) {
    chSysLock();
    if ((timeout != TIME_INFINITE) && !chVTIsArmedI(&vt))
      return RDY_TIMEOUT;
    chSysUnlock();
  }

  /* This lock will be released in high level driver.*/
  chSysLock();

  /* Atomic check on the timer in order to make sure that a timeout didn't
     happen outside the critical zone.*/
  if ((timeout != TIME_INFINITE) && !chVTIsArmedI(&vt))
    return RDY_TIMEOUT;

  /* Starts the operation.*/
  dp->CR2 |= I2C_CR2_ITEVTEN;
  dp->CR1 |= I2C_CR1_START;

  /* Waits for the operation completion or a timeout.*/
  i2cp->thread = chThdSelf();
  chSchGoSleepS(THD_STATE_SUSPENDED);
  if ((timeout != TIME_INFINITE) && chVTIsArmedI(&vt))
    chVTResetI(&vt);

  return chThdSelf()->p_u.rdymsg;
}

#endif /* HAL_USE_I2C */

/** @} */
