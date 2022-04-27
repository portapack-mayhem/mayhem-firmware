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
 * @file    AVR/i2c_lld.c
 * @brief   AVR I2C subsystem low level driver source.
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
/* Driver exported variables.                                                */
/*===========================================================================*/

/** @brief I2C driver identifier.*/
#if USE_AVR_I2C || defined(__DOXYGEN__)
I2CDriver I2CD;
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

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

#if USE_AVR_I2C || defined(__DOXYGEN__)
/**
 * @brief   I2C event interrupt handler.
 *
 * @notapi
 */
CH_IRQ_HANDLER(TWI_vect) {
  CH_IRQ_PROLOGUE();

  I2CDriver *i2cp = &I2CD;

  switch (TWSR & 0xF8) {
  case TWI_START:
  case TWI_REPEAT_START:
    TWDR = (i2cp->addr << 1);
    if ((i2cp->txbuf == NULL) || (i2cp->txbytes == 0) || (i2cp->txidx == i2cp->txbytes)) {
      TWDR |= 0x01;
    }
    TWCR = ((1 << TWINT) | (1 << TWEN) | (1 << TWIE));
    break;
  case TWI_MASTER_TX_ADDR_ACK:
  case TWI_MASTER_TX_DATA_ACK:
    if (i2cp->txidx < i2cp->txbytes) {
      TWDR = i2cp->txbuf[i2cp->txidx++];
      TWCR = ((1 << TWINT) | (1 << TWEN) | (1 << TWIE));
    } else {
      if (i2cp->rxbuf && i2cp->rxbytes) {
        TWCR = ((1 << TWSTA) | (1 << TWINT) | (1 << TWEN) | (1 << TWIE));
      } else {
        TWCR = ((1 << TWSTO) | (1 << TWINT) | (1 << TWEN));
        wakeup_isr(i2cp, RDY_OK);
      }
    }
    break;
  case TWI_MASTER_RX_ADDR_ACK:
    if (i2cp->rxidx == (i2cp->rxbytes - 1)) {
      TWCR = ((1 << TWINT) | (1 << TWEN) | (1 << TWIE));
    } else {
      TWCR = ((1 << TWEA) | (1 << TWINT) | (1 << TWEN) | (1 << TWIE));
    }
    break;
  case TWI_MASTER_RX_DATA_ACK:
    i2cp->rxbuf[i2cp->rxidx++] = TWDR;
    if (i2cp->rxidx == (i2cp->rxbytes - 1)) {
      TWCR = ((1 << TWINT) | (1 << TWEN) | (1 << TWIE));
    } else {
      TWCR = ((1 << TWEA) | (1 << TWINT) | (1 << TWEN) | (1 << TWIE));
    }
    break;
  case TWI_MASTER_RX_DATA_NACK:
    i2cp->rxbuf[i2cp->rxidx] = TWDR;
    TWCR = ((1 << TWSTO) | (1 << TWINT) | (1 << TWEN));
    wakeup_isr(i2cp, RDY_OK);
  case TWI_MASTER_TX_ADDR_NACK:
  case TWI_MASTER_TX_DATA_NACK:
  case TWI_MASTER_RX_ADDR_NACK:
    i2cp->errors |= I2CD_ACK_FAILURE;
    break;
  case TWI_ARBITRATION_LOST:
    i2cp->errors |= I2CD_ARBITRATION_LOST;
    break;
  case TWI_BUS_ERROR:
    i2cp->errors |= I2CD_BUS_ERROR;
    break;
  default:
    /* FIXME: only gets here if there are other MASTERs in the bus */
    TWCR = ((1 << TWSTO) | (1 << TWINT) | (1 << TWEN));
    wakeup_isr(i2cp, RDY_RESET);
  }

  if (i2cp->errors != I2CD_NO_ERROR) {
    TWCR = ((1 << TWSTO) | (1 << TWINT) | (1 << TWEN));
    wakeup_isr(i2cp, RDY_RESET);
  }

  CH_IRQ_EPILOGUE();
}
#endif /* USE_AVR_I2C */

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level I2C driver initialization.
 *
 * @notapi
 */
void i2c_lld_init(void) {
  i2cObjectInit(&I2CD);
}

/**
 * @brief   Configures and activates the I2C peripheral.
 *
 * @param[in] i2cp      pointer to the @p I2CDriver object
 *
 * @notapi
 */
void i2c_lld_start(I2CDriver *i2cp) {

  /* TODO: Test TWI without external pull-ups (use internal) */

  /* Configure prescaler to 1 */
  TWSR &= 0xF8;

  /* Configure baudrate */
  TWBR = ((F_CPU / i2cp->config->clock_speed) - 16) / 2;
}

/**
 * @brief   Deactivates the I2C peripheral.
 *
 * @param[in] i2cp      pointer to the @p I2CDriver object
 *
 * @notapi
 */
void i2c_lld_stop(I2CDriver *i2cp) {

  if (i2cp->state != I2C_STOP) {
    /* Disable TWI subsystem and stop all operations */
    TWCR &= ~(1 << TWEN);
  }
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
  i2cp->addr = addr;
  i2cp->txbuf = NULL;
  i2cp->txbytes = 0;
  i2cp->txidx = 0;
  i2cp->rxbuf = rxbuf;
  i2cp->rxbytes = rxbytes;
  i2cp->rxidx = 0;

  /* Send START */
  TWCR = ((1 << TWSTA) | (1 << TWINT) | (1 << TWEN) | (1 << TWIE));

  chSysLock();
  i2cp->thread = chThdSelf();
  chSchGoSleepS(THD_STATE_SUSPENDED);
  chSysUnlock();

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
  i2cp->addr = addr;
  i2cp->txbuf = txbuf;
  i2cp->txbytes = txbytes;
  i2cp->txidx = 0;
  i2cp->rxbuf = rxbuf;
  i2cp->rxbytes = rxbytes;
  i2cp->rxidx = 0;

  TWCR = ((1 << TWSTA) | (1 << TWINT) | (1 << TWEN) | (1 << TWIE));

  chSysLock();
  i2cp->thread = chThdSelf();
  chSchGoSleepS(THD_STATE_SUSPENDED);
  chSysUnlock();

  return chThdSelf()->p_u.rdymsg;
}

#endif /* HAL_USE_I2C */

/** @} */
