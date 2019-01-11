/*
    ChibiOS/RT - Copyright (C) 2006-2013 Giovanni Di Sirio
                 Copyright (C) 2014 Jared Boone, ShareBrained Technology

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
 * @file    LPC43xx/i2c_lld.c
 * @brief   LPC43xx I2C subsystem low level driver source.
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

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/** @brief   START transmitted.*/
#define I2C_START                  0x08
/** @brief   Repeated START transmitted.*/
#define I2C_REPEAT_START           0x10
/** @brief   Arbitration Lost.*/
#define I2C_ARBITRATION_LOST       0x38
/** @brief   Bus errors.*/
#define I2C_BUS_ERROR              0x00

/** @brief   SLA+W transmitted with ACK response.*/
#define I2C_MASTER_TX_ADDR_ACK     0x18
/** @brief   SLA+W transmitted with NACK response.*/
#define I2C_MASTER_TX_ADDR_NACK    0x20
/** @brief   DATA transmitted with ACK response.*/
#define I2C_MASTER_TX_DATA_ACK     0x28
/** @brief   DATA transmitted with NACK response.*/
#define I2C_MASTER_TX_DATA_NACK    0x30

/** @brief   SLA+R transmitted with ACK response.*/
#define I2C_MASTER_RX_ADDR_ACK     0x40
/** @brief   SLA+R transmitted with NACK response.*/
#define I2C_MASTER_RX_ADDR_NACK    0x48
/** @brief   DATA received with ACK response.*/
#define I2C_MASTER_RX_DATA_ACK     0x50
/** @brief   DATA received with NACK response.*/
#define I2C_MASTER_RX_DATA_NACK    0x58

/** @brief   Send I2C NACK. */
#define I2C_NACK                   0
/** @brief   Send I2C ACK. */
#define I2C_ACK                    1

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/** @brief I2C0 driver identifier.*/
#if LPC43XX_I2C_USE_I2C0 || defined(__DOXYGEN__)
I2CDriver I2CD0;
#endif

/** @brief I2C1 driver identifier.*/
#if LPC43XX_I2C_USE_I2C1 || defined(__DOXYGEN__)
I2CDriver I2CD1;
#endif

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

#if LPC43XX_I2C_USE_I2C0
static const i2c_resources_t i2c0_resources = {
  .base = { .clk = &LPC_CGU->BASE_APB1_CLK, .stat = &LPC_CCU1->BASE_STAT, .stat_mask = (1 << 1) },
  .branch = { .cfg = &LPC_CCU1->CLK_APB1_I2C0_CFG, .stat = &LPC_CCU1->CLK_APB1_I2C0_STAT },
  .reset = { .output_index = 48 },
};
#endif /* LPC43XX_I2C_USE_I2C0 */

#if LPC43XX_I2C_USE_I2C1
static const i2c_resources_t i2c1_resources = {
  .base = { .clk = &LPC_CGU->BASE_APB3_CLK, .stat = &LPC_CCU1->BASE_STAT, .stat_mask = (1 << 0) },
  .branch = { .cfg = &LPC_CCU1->CLK_APB3_I2C1_CFG, .stat = &LPC_CCU1->CLK_APB3_I2C1_STAT },
  .reset = { .output_index = 49 },
};
#endif /* LPC43XX_I2C_USE_I2C1 */

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
 * @brief   Set clock speed.
 *
 * @param[in] i2cp      pointer to the @p I2CDriver object
 *
 * @notapi
 */
static void i2c_periph_set_clock(I2C_TypeDef dp, uint16_t high_count, uint16_t low_count) {
  dp->SCLH = (uint32_t)high_count;
  dp->SCLL = (uint32_t)low_count;
}

/* Peripheral functions */

static void i2c_periph_enable(I2C_TypeDef dp) {
  dp->CONSET =
      I2C_CONSET_I2EN
    ;
}

static void i2c_periph_disable(I2C_TypeDef dp) {
  dp->CONCLR =
      I2C_CONCLR_AAC
    | I2C_CONCLR_SIC
    | I2C_CONCLR_STAC
    | I2C_CONCLR_I2ENC
    ;
}
/*
static void i2c_periph_clear_start(I2C_TypeDef dp) {
  dp->CONCLR =
      I2C_CONCLR_STAC
    ;
}
*/
static void i2c_periph_clear_all(I2C_TypeDef dp) {
  dp->CONCLR =
      I2C_CONCLR_SIC
    | I2C_CONCLR_STAC
    ;
}

static uint_fast8_t i2c_periph_status(I2C_TypeDef dp) {
  return dp->STAT;
}
/*
static void i2c_periph_start(I2C_TypeDef dp) {
  dp->CONSET = I2C_CONSET_STA;
  dp->CONCLR = I2C_CONCLR_SIC;
}
*/
static void i2c_periph_stop(I2C_TypeDef dp) {
  dp->CONSET = I2C_CONSET_STO | I2C_CONSET_AA;
  dp->CONCLR = I2C_CONCLR_SIC;
}

static void i2c_periph_transfer_byte(I2C_TypeDef dp, int ack) {
  dp->CONSET = (ack == I2C_NACK) ? 0 : I2C_CONSET_AA;
  dp->CONCLR = I2C_CONCLR_SIC | ((ack == I2C_NACK) ? I2C_CONCLR_AAC : 0);
}
/*
static void i2c_periph_transmit_byte(I2C_TypeDef dp, uint_fast8_t byte) {
  dp->DAT = byte;
  i2c_periph_transfer_byte(dp, I2C_ACK);
}
*/
static uint_fast8_t i2c_periph_read_byte(I2C_TypeDef dp) {
  return dp->DAT & 0xff;  
}

/* LLD functions */

static void i2c_lld_abort_operation(I2CDriver *i2cp) {
  i2c_periph_stop(i2cp->i2c);
  i2cp->state = I2C_STOP;
}

static bool_t i2c_lld_tx_not_done(I2CDriver *i2cp) {
  return i2cp->txbytes > 0;
}

static bool_t i2c_lld_rx_not_done(I2CDriver *i2cp) {
  return i2cp->rxbytes > 0;
}

static bool_t i2c_lld_rx_last_byte(I2CDriver *i2cp) {
  return i2cp->rxbytes == 1;
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
 *
 * @notapi
 */
static void i2c_lld_serve_event_interrupt(I2CDriver *i2cp) {
  I2C_TypeDef dp = i2cp->i2c;

  /* TODO: Likely that bugs remain around clearing STA/STO flags */

  switch (i2c_periph_status(dp)) {
  case I2C_START:                 /* 0x08 */
  case I2C_REPEAT_START:          /* 0x10 */
    //i2c_periph_clear_start(dp);
    dp->DAT = i2cp->addr_r;
    dp->CONCLR = I2C_CONCLR_STAC | I2C_CONCLR_SIC;
    //i2c_periph_transmit_byte(dp, i2cp->addr_r);
    break;

  case I2C_MASTER_TX_ADDR_ACK:    /* 0x18 */
    /* Previous state 0x08 or 0x10, slave address + write has been
     * transmitted. ACK has been received. The first data byte will
     * be transmitted, and an ACK bit will be received.
     */
    /* fall through */
  case I2C_MASTER_TX_DATA_ACK:    /* 0x28 */
    if (i2c_lld_tx_not_done(i2cp)) {
      //i2c_periph_transmit_byte(dp, i2cp->txbuf[i2cp->txidx++]);
      dp->DAT = *i2cp->txbuf++;
      i2cp->txbytes--;
      dp->CONCLR = I2C_CONCLR_SIC;
    } else {
      if (i2c_lld_rx_not_done(i2cp)) {
        dp->CONSET = I2C_CONSET_STA;
        dp->CONCLR = I2C_CONCLR_SIC;
        //i2c_periph_start(dp);
      } else {
        dp->CONSET = I2C_CONSET_STO;
        dp->CONCLR = I2C_CONCLR_SIC;
        //i2c_periph_stop(dp);
        wakeup_isr(i2cp, RDY_OK);
      }
    }
    break;

  case I2C_MASTER_RX_DATA_ACK:    /* 0x50 */
    *i2cp->rxbuf++ = i2c_periph_read_byte(dp);
    i2cp->rxbytes--;
    /* fall through */
  case I2C_MASTER_RX_ADDR_ACK:    /* 0x40 */
    if (i2c_lld_rx_last_byte(i2cp)) {
      i2c_periph_transfer_byte(dp, I2C_NACK);
    } else {
      i2c_periph_transfer_byte(dp, I2C_ACK);
    }
    break;

  case I2C_MASTER_RX_DATA_NACK:   /* 0x58 */
    *i2cp->rxbuf++ = i2c_periph_read_byte(dp);
    i2cp->rxbytes--;
    i2c_periph_stop(dp);
    wakeup_isr(i2cp, RDY_OK);
    /* fall through */
  case I2C_MASTER_TX_ADDR_NACK:   /* 0x20 */
    /* Slave address and R/W transmitted, NACK received (no response) */
  case I2C_MASTER_TX_DATA_NACK:   /* 0x30 */
    /* Data transmitted, NACK received (no response) */
  case I2C_MASTER_RX_ADDR_NACK:   /* 0x48 */
    /* Slave address and R/W transmitted, NACK received (no response) */
    i2cp->errors |= I2CD_ACK_FAILURE;
    break;

  case I2C_ARBITRATION_LOST:      /* 0x38 */
    i2cp->errors |= I2CD_ARBITRATION_LOST;
    break;

  case I2C_BUS_ERROR:             /* 0x00 */
    i2cp->errors |= I2CD_BUS_ERROR;
    break;

  default:
    i2c_periph_stop(dp);
    wakeup_isr(i2cp, RDY_RESET);
    break;
  }

  if (i2cp->errors != I2CD_NO_ERROR) {
    i2c_periph_stop(dp);
    wakeup_isr(i2cp, RDY_RESET);
  }
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

#if defined(LPC43XX_M4)
  #if LPC43XX_I2C_USE_I2C0 || defined(__DOXYGEN__)
  CH_IRQ_HANDLER(I2C0_IRQHandler) {
    CH_IRQ_PROLOGUE();
    i2c_lld_serve_event_interrupt(&I2CD0);
    CH_IRQ_EPILOGUE();
  }
  #endif /* LPC43XX_I2C_USE_I2C0 */

  #if LPC43XX_I2C_USE_I2C1 || defined(__DOXYGEN__)
  CH_IRQ_HANDLER(I2C1_IRQHandler) {
    CH_IRQ_PROLOGUE();
    i2c_lld_serve_event_interrupt(&I2CD1);
    CH_IRQ_EPILOGUE();
  }
  #endif /* LPC43XX_I2C_USE_I2C1 */
#endif

#if defined(LPC43XX_M0)
  CH_IRQ_HANDLER(I2C0_Or_I2C1_IRQHandler) {
    CH_IRQ_PROLOGUE();

    #if LPC43XX_I2C_USE_I2C0 || defined(__DOXYGEN__)
      if( LPC_I2C0->CONSET & I2C_CONSET_SI ) i2c_lld_serve_event_interrupt(&I2CD0);
    #endif

    #if LPC43XX_I2C_USE_I2C1 || defined(__DOXYGEN__)
      if( LPC_I2C1->CONSET & I2C_CONSET_SI ) i2c_lld_serve_event_interrupt(&I2CD1);
    #endif

    CH_IRQ_EPILOGUE();
  }
#endif

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/
/**
 * @brief   Low level I2C driver initialization.
 *
 * @notapi
 */
void i2c_lld_init(void) {
#if LPC43XX_I2C_USE_I2C0 || defined(__DOXYGEN__)
  i2cObjectInit(&I2CD0);
  I2CD0.thread = NULL;
  I2CD0.i2c    = LPC_I2C0;
  I2CD0.resources = &i2c0_resources;
#endif /* LPC43XX_I2C_USE_I2C0 */

#if LPC43XX_I2C_USE_I2C1 || defined(__DOXYGEN__)
  i2cObjectInit(&I2CD1);
  I2CD1.thread = NULL;
  I2CD1.i2c = LPC_I2C1;
  I2CD1.resources = &i2c1_resources;
#endif /* LPC43XX_I2C_USE_I2C1 */
}

/**
 * @brief   Configures and activates the I2C peripheral.
 *
 * @param[in] i2cp      pointer to the @p I2CDriver object
 *
 * @notapi
 */
void i2c_lld_start(I2CDriver *i2cp) {
  I2C_TypeDef dp = i2cp->i2c;

  /* TODO: Reset peripheral, enable clocks? */

  base_clock_enable(&i2cp->resources->base);
  branch_clock_enable(&i2cp->resources->branch);
  peripheral_reset(&i2cp->resources->reset);

  i2c_periph_set_clock(dp, i2cp->config->high_count, i2cp->config->low_count);
  i2c_periph_enable(dp);
  i2c_periph_clear_all(dp);

#if defined(LPC43XX_M4)
  #if LPC43XX_I2C_USE_I2C0 || defined(__DOXYGEN__)
    if (&I2CD0 == i2cp) {
      nvicEnableVector(I2C0_IRQn, CORTEX_PRIORITY_MASK(LPC43XX_M4_I2C_I2C0_IRQ_PRIORITY));
    }
  #endif /* LPC43XX_I2C_USE_I2C0 */

  #if LPC43XX_I2C_USE_I2C1 || defined(__DOXYGEN__)
    if (&I2CD1 == i2cp) {
      nvicEnableVector(I2C1_IRQn, CORTEX_PRIORITY_MASK(LPC43XX_M4_I2C_I2C1_IRQ_PRIORITY));
    }
  #endif /* LPC43XX_I2C_USE_I2C1 */
#endif

#if defined(LPC43XX_M0)
  nvicEnableVector(I2C0_OR_I2C1_IRQn, CORTEX_PRIORITY_MASK(LPC43XX_M0_I2C_I2C0_OR_I2C1_IRQ_PRIORITY));
#endif
}

/**
 * @brief   Deactivates the I2C peripheral.
 *
 * @param[in] i2cp      pointer to the @p I2CDriver object
 *
 * @notapi
 */
void i2c_lld_stop(I2CDriver *i2cp) {
  I2C_TypeDef dp = i2cp->i2c;

  /* If not in stopped state then disables the I2C clock.*/
  if (i2cp->state != I2C_STOP) {
    i2c_lld_abort_operation(i2cp);

#if LPC43XX_I2C_USE_I2C0 || defined(__DOXYGEN__)
    if (&I2CD0 == i2cp) {
#if defined(LPC43XX_M4)
      nvicDisableVector(I2C0_IRQn);
#endif
#if defined(LPC43XX_M0)
#if LPC43XX_I2C_USE_I2C1
      if( I2CD1.state == I2C_STOP ) {
#endif
        // TODO: This won't work if the I2C peripherals are split between cores!
        nvicDisableVector(I2C0_OR_I2C1_IRQn);
#if LPC43XX_I2C_USE_I2C1
      }
#endif
#endif
    }
#endif /* LPC43XX_I2C_USE_I2C0 */

#if LPC43XX_I2C_USE_I2C1 || defined(__DOXYGEN__)
    if (&I2CD1 == i2cp) {
#if defined(LPC43XX_M4)
      nvicDisableVector(I2C1_IRQn);
#endif
#if defined(LPC43XX_M0)
#if LPC43XX_I2C_USE_I2C0
      if( I2CD0.state == I2C_STOP ) {
#endif
        // TODO: This won't work if the I2C peripherals are split between cores!
        nvicDisableVector(I2C0_OR_I2C1_IRQn);
#if LPC43XX_I2C_USE_I2C0
      }
#endif
#endif
    }
#endif /* LPC43XX_I2C_USE_I2C1 */

    i2c_periph_disable(dp);
    peripheral_reset(&i2cp->resources->reset);
    branch_clock_disable(&i2cp->resources->branch);
    base_clock_disable(&i2cp->resources->base);
  }
}

static msg_t i2c_lld_master_start(I2CDriver *i2cp, uint_fast8_t addr_r,
                                 const uint8_t *txbuf, size_t txbytes,
                                 uint8_t *rxbuf, size_t rxbytes,
                                 systime_t timeout) {
  I2C_TypeDef dp = i2cp->i2c;
  VirtualTimer vt;

  /* Resetting error flags for this transfer.*/
  i2cp->errors = I2CD_NO_ERROR;

  /* Global timeout for the whole operation.*/
  if (timeout != TIME_INFINITE)
    chVTSetI(&vt, timeout, i2c_lld_safety_timeout, (void *)i2cp);

  i2cp->addr_r = addr_r;
  i2cp->txbuf = txbuf;
  i2cp->txbytes = txbytes;
  i2cp->rxbuf = rxbuf;
  i2cp->rxbytes = rxbytes;

  /* Atomic check on the timer in order to make sure that a timeout didn't
     happen outside the critical zone.*/
  if ((timeout != TIME_INFINITE) && !chVTIsArmedI(&vt))
    return RDY_TIMEOUT;

  dp->CONCLR =
      I2C_CONCLR_AAC
    | I2C_CONCLR_SIC
    | I2C_CONCLR_STAC
    ;
  dp->CONSET = I2C_CONSET_STA;

  /* Waits for the operation completion or a timeout.*/
  i2cp->thread = chThdSelf();
  chSchGoSleepS(THD_STATE_SUSPENDED);
  if ((timeout != TIME_INFINITE) && chVTIsArmedI(&vt))
    chVTResetI(&vt);

  return chThdSelf()->p_u.rdymsg;
}

/**
 * @brief   Transmits data via the I2C bus as master.
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
  return i2c_lld_master_start(i2cp, (addr << 1) | 0, txbuf, txbytes, rxbuf, rxbytes, timeout);
}

/**
 * @brief   Receives data via the I2C bus as master.
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
  return i2c_lld_master_start(i2cp, (addr << 1) | 1, NULL, 0, rxbuf, rxbytes, timeout);
}

#endif /* HAL_USE_I2C */

/** @} */
