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
 * @file    AT91SAM7/i2c_lld.c
 * @brief   AT91SAM7 I2C subsystem low level driver source.
 * @note    I2C peripheral interrupts on AT91SAM7 platform must have highest
 *          priority in system.
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

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/** @brief I2C1 driver identifier.*/
#if SAM7_I2C_USE_I2C1 || defined(__DOXYGEN__)
I2CDriver I2CD1;
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
 * @brief   Helper function.
 *
 * @param[in] i2cp      pointer to the @p I2CDriver object
 *
 * @notapi
 */
static void _i2c_lld_serve_rx_interrupt(I2CDriver *i2cp){
  if (i2cp->rxbytes == 1)
    AT91C_BASE_TWI->TWI_CR |= AT91C_TWI_STOP;

  *(i2cp->rxbuf) = AT91C_BASE_TWI->TWI_RHR;
  i2cp->rxbuf++;
  i2cp->rxbytes--;
  if (i2cp->rxbytes == 0){
    AT91C_BASE_TWI->TWI_IDR = AT91C_TWI_RXRDY;
    AT91C_BASE_TWI->TWI_IER = AT91C_TWI_TXCOMP;
  }
}

/**
 * @brief   Helper function.
 * @note    During write operation you do not need to set STOP manually.
 *          It sets automatically when THR and shift registers becomes empty.
 *
 * @param[in] i2cp      pointer to the @p I2CDriver object
 *
 * @notapi
 */
static void _i2c_lld_serve_tx_interrupt(I2CDriver *i2cp){

  if (i2cp->txbytes == 0){
    AT91C_BASE_TWI->TWI_IDR = AT91C_TWI_TXRDY;
    AT91C_BASE_TWI->TWI_IER = AT91C_TWI_TXCOMP;
  }
  else{
    AT91C_BASE_TWI->TWI_THR = *(i2cp->txbuf);
    i2cp->txbuf++;
    i2cp->txbytes--;
  }
}

/**
 * @brief   I2C shared ISR code.
 *
 * @param[in] i2cp      pointer to the @p I2CDriver object
 *
 * @notapi
 */
static void i2c_lld_serve_interrupt(I2CDriver *i2cp) {

  uint32_t sr;
  sr = AT91C_BASE_TWI->TWI_SR;
  /* this masking doing in official Atmel driver. Is it needed ??? */
  sr &= AT91C_BASE_TWI->TWI_IMR;

  if (sr & AT91C_TWI_NACK){
    i2cp->errors |= I2CD_ACK_FAILURE;
    wakeup_isr(i2cp, RDY_RESET);
    return;
  }
  if (sr & AT91C_TWI_RXRDY){
    _i2c_lld_serve_rx_interrupt(i2cp);
  }
  else if (sr & AT91C_TWI_TXRDY){
    _i2c_lld_serve_tx_interrupt(i2cp);
  }
  else if (sr & AT91C_TWI_TXCOMP){
    AT91C_BASE_TWI->TWI_IDR = AT91C_TWI_TXCOMP;
    wakeup_isr(i2cp, RDY_OK);
  }
  else
    chDbgPanic("Invalid value");
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

#if SAM7_I2C_USE_I2C1 || defined(__DOXYGEN__)
/**
 * @brief   I2C1 event interrupt handler.
 *
 * @notapi
 */
CH_IRQ_HANDLER(TWI_IRQHandler) {

  CH_IRQ_PROLOGUE();
  i2c_lld_serve_interrupt(&I2CD1);
  AT91C_BASE_AIC->AIC_EOICR = 0;
  CH_IRQ_EPILOGUE();
}
#endif /* STM32_I2C_USE_I2C1 */

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level I2C driver initialization.
 *
 * @notapi
 */
void i2c_lld_init(void) {

#if SAM7_I2C_USE_I2C1
  i2cObjectInit(&I2CD1);
  I2CD1.thread  = NULL;
  I2CD1.txbuf   = NULL;
  I2CD1.rxbuf   = NULL;
  I2CD1.txbytes = 0;
  I2CD1.rxbytes = 0;

#if SAM7_PLATFORM == SAM7S512 || SAM7_PLATFORM == SAM7S256 || SAM7_PLATFORM == SAM7S128 || SAM7_PLATFORM == SAM7S64
  AT91C_BASE_PIOA->PIO_PDR   = AT91C_PA3_TWD | AT91C_PA4_TWCK;
  AT91C_BASE_PIOA->PIO_ASR   = AT91C_PA3_TWD | AT91C_PA4_TWCK;
  AT91C_BASE_PIOA->PIO_MDER  = AT91C_PA3_TWD | AT91C_PA4_TWCK;
  AT91C_BASE_PIOA->PIO_PPUDR = AT91C_PA3_TWD | AT91C_PA4_TWCK;
#elif SAM7_PLATFORM == SAM7X512 || SAM7_PLATFORM == SAM7X256 || SAM7_PLATFORM == SAM7X128
  AT91C_BASE_PIOA->PIO_PDR   = AT91C_PA10_TWD | AT91C_PA11_TWCK;
  AT91C_BASE_PIOA->PIO_ASR   = AT91C_PA10_TWD | AT91C_PA11_TWCK;
  AT91C_BASE_PIOA->PIO_MDER  = AT91C_PA10_TWD | AT91C_PA11_TWCK;
  AT91C_BASE_PIOA->PIO_PPUDR = AT91C_PA10_TWD | AT91C_PA11_TWCK;
#elif SAM7_PLATFORM == SAM7A3
  AT91C_BASE_PIOA->PIO_PDR   = AT91C_PA0_TWD | AT91C_PA1_TWCK;
  AT91C_BASE_PIOA->PIO_ASR   = AT91C_PA0_TWD | AT91C_PA1_TWCK;
  AT91C_BASE_PIOA->PIO_MDER  = AT91C_PA0_TWD | AT91C_PA1_TWCK;
  AT91C_BASE_PIOA->PIO_PPUDR = AT91C_PA0_TWD | AT91C_PA1_TWCK;
#else
#error "SAM7 platform not supported"
#endif

  AIC_ConfigureIT(AT91C_ID_TWI,
                  AT91C_AIC_SRCTYPE_INT_HIGH_LEVEL | SAM7_I2C_I2C1_IRQ_PRIORITY,
                  TWI_IRQHandler);
#endif /* STM32_I2C_USE_I2C1 */
}

/**
 * @brief   Configures and activates the I2C peripheral.
 *
 * @param[in] i2cp      pointer to the @p I2CDriver object
 *
 * @notapi
 */
void i2c_lld_start(I2CDriver *i2cp) {

  volatile uint32_t fake;

  /* If in stopped state then enables the I2C clocks.*/
  if (i2cp->state == I2C_STOP) {

#if SAM7_I2C_USE_I2C1
    if (&I2CD1 == i2cp) {
      /* enable peripheral clock */
      AT91C_BASE_PMC->PMC_PCER = (1 << AT91C_ID_TWI);

      /* Enables associated interrupt vector.*/
      AIC_EnableIT(AT91C_ID_TWI);

      /* Reset */
      AT91C_BASE_TWI->TWI_CR = AT91C_TWI_SWRST;
      fake = AT91C_BASE_TWI->TWI_RHR;

      /* Set master mode */
      AT91C_BASE_TWI->TWI_CR = AT91C_TWI_MSDIS;
      AT91C_BASE_TWI->TWI_CR = AT91C_TWI_MSEN;

      /* Setup I2C parameters. */
      AT91C_BASE_TWI->TWI_CWGR = i2cp->config->cwgr;
    }
#endif /* STM32_I2C_USE_I2C1 */
  }

  (void)fake;
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

#if SAM7_I2C_USE_I2C1
    if (&I2CD1 == i2cp) {
      AT91C_BASE_TWI->TWI_IDR = AT91C_TWI_TXCOMP | AT91C_TWI_RXRDY |
                                 AT91C_TWI_TXRDY | AT91C_TWI_NACK;
      AT91C_BASE_PMC->PMC_PCDR = (1 << AT91C_ID_TWI);
      AIC_DisableIT(AT91C_ID_TWI);
    }
#endif
  }
}

/**
 * @brief   Receives data via the I2C bus as master.
 *
 * @param[in] i2cp      pointer to the @p I2CDriver object
 * @param[in] addr      slave device address
 * @param[out] rxbuf    pointer to the receive buffer
 * @param[in] rxbytes   number of bytes to be received
 * @param[in] timeout   this value is ignored on SAM7 platform.
 *
 * @return              The operation status.
 * @retval RDY_OK       if the function succeeded.
 * @retval RDY_RESET    if one or more I2C errors occurred, the errors can
 *                      be retrieved using @p i2cGetErrors().
 *
 * @notapi
 */
msg_t i2c_lld_master_receive_timeout(I2CDriver *i2cp, i2caddr_t addr,
                                     uint8_t *rxbuf, size_t rxbytes,
                                     systime_t timeout) {
  (void)timeout;

  /* delete trash from RHR*/
  volatile uint32_t fake;
  fake = AT91C_BASE_TWI->TWI_RHR;
  (void)fake;

  /* Initializes driver fields.*/
  i2cp->rxbuf   = rxbuf;
  i2cp->rxbytes = rxbytes;

  i2cp->txbuf   = NULL;
  i2cp->txbytes = 0;

  /* tune master mode register */
  AT91C_BASE_TWI->TWI_MMR = 0;
  AT91C_BASE_TWI->TWI_MMR |= (addr << 16) | AT91C_TWI_MREAD;

  /* enable just needed interrupts */
  AT91C_BASE_TWI->TWI_IER = AT91C_TWI_RXRDY | AT91C_TWI_NACK;

  /* In single data byte master read or write, the START and STOP must both be set. */
  if (rxbytes == 1)
    AT91C_BASE_TWI->TWI_CR = AT91C_TWI_STOP | AT91C_TWI_START;
  else
    AT91C_BASE_TWI->TWI_CR = AT91C_TWI_START;

  /* Waits for the operation completion.*/
  i2cp->thread = chThdSelf();
  chSchGoSleepS(THD_STATE_SUSPENDED);

  return chThdSelf()->p_u.rdymsg;
}

/**
 * @brief   Read data via the I2C bus as master using internal slave addressing.
 * @details Address bytes must be written in special purpose SAM7 registers.
 *
 * @param[in] i2cp      pointer to the @p I2CDriver object
 * @param[in] addr      slave device address
 * @param[in] txbuf     pointer to the transmit buffer
 * @param[in] txbytes   number of bytes to be transmitted
 * @param[out] rxbuf    pointer to the receive buffer
 * @param[in] rxbytes   number of bytes to be received
 * @param[in] timeout   this value is ignored on SAM7 platform.
 *
 * @return              The operation status.
 * @retval RDY_OK       if the function succeeded.
 * @retval RDY_RESET    if one or more I2C errors occurred, the errors can
 *                      be retrieved using @p i2cGetErrors().
 *
 * @notapi
 */
msg_t i2c_lld_transceive_timeout(I2CDriver *i2cp, i2caddr_t addr,
                                 const uint8_t *txbuf, size_t txbytes,
                                 uint8_t *rxbuf, size_t rxbytes,
                                 systime_t timeout) {
  (void)timeout;

  /* delete trash from RHR*/
  volatile uint32_t fake;
  fake = AT91C_BASE_TWI->TWI_RHR;
  (void)fake;

  /* Initializes driver fields.*/
  i2cp->rxbuf   = rxbuf;
  i2cp->rxbytes = rxbytes;

  /* tune master mode register */
  AT91C_BASE_TWI->TWI_MMR = 0;
  AT91C_BASE_TWI->TWI_MMR |= (addr << 16) | (txbytes << 8) | AT91C_TWI_MREAD;

  /* store internal slave address in TWI_IADR registers */
  AT91C_BASE_TWI->TWI_IADR = 0;
  while (txbytes > 0){
    AT91C_BASE_TWI->TWI_IADR = (AT91C_BASE_TWI->TWI_IADR << 8);
    AT91C_BASE_TWI->TWI_IADR |= *(txbuf++);
    txbytes--;
  }

  /* enable just needed interrupts */
  AT91C_BASE_TWI->TWI_IER = AT91C_TWI_RXRDY | AT91C_TWI_NACK;

  /* Internal address of I2C slave was set in special Atmel registers.
   * Now we must call read function. The I2C cell automatically sends
   * bytes from IADR register to bus and issues repeated start. */
  if (rxbytes == 1)
    AT91C_BASE_TWI->TWI_CR = AT91C_TWI_STOP | AT91C_TWI_START;
  else
    AT91C_BASE_TWI->TWI_CR = AT91C_TWI_START;

  /* Waits for the operation completion.*/
  i2cp->thread = chThdSelf();
  chSchGoSleepS(THD_STATE_SUSPENDED);

  return chThdSelf()->p_u.rdymsg;
}

/**
 * @brief   Transmits data via the I2C bus as master.
 * @details When performing reading through write you can not write more than
 *          3 bytes of data to I2C slave. This is SAM7 platform limitation.
 *
 * @param[in] i2cp      pointer to the @p I2CDriver object
 * @param[in] addr      slave device address
 * @param[in] txbuf     pointer to the transmit buffer
 * @param[in] txbytes   number of bytes to be transmitted
 * @param[out] rxbuf    pointer to the receive buffer
 * @param[in] rxbytes   number of bytes to be received
 * @param[in] timeout   this value is ignored on SAM7 platform.
 *
 * @return              The operation status.
 * @retval RDY_OK       if the function succeeded.
 * @retval RDY_RESET    if one or more I2C errors occurred, the errors can
 *                      be retrieved using @p i2cGetErrors().
 *
 * @notapi
 */
msg_t i2c_lld_master_transmit_timeout(I2CDriver *i2cp, i2caddr_t addr,
                                      const uint8_t *txbuf, size_t txbytes,
                                      uint8_t *rxbuf, size_t rxbytes,
                                      systime_t timeout) {
  (void)timeout;

  /* SAM7 specific check */
  chDbgCheck(((rxbytes == 0) ||
             ((txbytes > 0) && (txbytes < 4) && (rxbuf != NULL))),
               "i2c_lld_master_transmit_timeout");

  /* prepare to read through write operation */
  if (rxbytes > 0){
    return i2c_lld_transceive_timeout(i2cp, addr, txbuf, txbytes, rxbuf,
                                      rxbytes, timeout);
  }
  else{
    if (txbytes == 1){
      /* In single data byte master read or write, the START and STOP
       * must both be set. */
      AT91C_BASE_TWI->TWI_CR |= AT91C_TWI_STOP;
    }
    AT91C_BASE_TWI->TWI_MMR  = 0;
    AT91C_BASE_TWI->TWI_MMR |= addr << 16;

    /* enable just needed interrupts */
    AT91C_BASE_TWI->TWI_IER = AT91C_TWI_TXRDY | AT91C_TWI_NACK;

    /* correct size and pointer because first byte will be written
     * for issue start condition */
    i2cp->txbuf   = txbuf + 1;
    i2cp->txbytes = txbytes - 1;

    /* According to datasheet there is no need to set START manually
     * we just need to write first byte in THR */
    AT91C_BASE_TWI->TWI_THR = txbuf[0];

    /* Waits for the operation completion.*/
    i2cp->thread = chThdSelf();
    chSchGoSleepS(THD_STATE_SUSPENDED);

    return chThdSelf()->p_u.rdymsg;
  }
}

#endif /* HAL_USE_I2C */

/** @} */
