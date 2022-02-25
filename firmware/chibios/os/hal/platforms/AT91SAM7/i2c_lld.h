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
 * @file    AT91SAM7/i2c_lld.h
 * @brief   AT91SAM7 I2C subsystem low level driver header.
 *
 * @addtogroup I2C
 * @{
 */

#ifndef _I2C_LLD_H_
#define _I2C_LLD_H_

#if HAL_USE_I2C || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @brief   Peripheral clock frequency.
 */
#define I2C_CLK_FREQ  ((STM32_PCLK1) / 1000000)

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    Configuration options
 * @{
 */
/**
 * @brief   I2C1 driver enable switch.
 * @details If set to @p TRUE the support for I2C1 is included.
 * @note    The default is @p FALSE.
 */
#if !defined(SAM7_I2C_USE_I2C1) || defined(__DOXYGEN__)
#define SAM7_I2C_USE_I2C1              FALSE
#endif

/**
 * @brief   I2C1 interrupt priority level setting.
 */
#if !defined(SAM7_I2C_I2C1_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define SAM7_I2C_I2C1_IRQ_PRIORITY     (AT91C_AIC_PRIOR_HIGHEST - 2)
#endif

/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/** @brief  error checks */
#if !SAM7_I2C_USE_I2C1
#error "I2C driver activated but no I2C peripheral assigned"
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Type representing I2C address.
 */
typedef uint16_t i2caddr_t;

/**
 * @brief   I2C Driver condition flags type.
 */
typedef uint32_t i2cflags_t;

/**
 * @brief Driver configuration structure.
 */
typedef struct {
  /**
   * @brief   CWGR regitster content.
   */
  uint32_t                  cwgr;
} I2CConfig;

/**
 * @brief   Type of a structure representing an I2C driver.
 */
typedef struct I2CDriver I2CDriver;

/**
 * @brief Structure representing an I2C driver.
 */
struct I2CDriver{
  /**
   * @brief   Driver state.
   */
  i2cstate_t                state;
  /**
   * @brief   Current configuration data.
   */
  const I2CConfig           *config;
  /**
   * @brief   Error flags.
   */
  i2cflags_t                errors;
  /**
   * @brief   Pointer to receive buffer.
   */
  uint8_t                   *rxbuf;
  /**
   * @brief   Pointer to transmit buffer.
   */
  const uint8_t             *txbuf;
  /**
   * @brief   Bytes count to be received.
   */
  size_t                    rxbytes;
  /**
   * @brief   Bytes count to be transmitted.
   */
  size_t                    txbytes;
#if I2C_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
#if CH_USE_MUTEXES || defined(__DOXYGEN__)
  /**
   * @brief   Mutex protecting the bus.
   */
  Mutex                     mutex;
#elif CH_USE_SEMAPHORES
  Semaphore                 semaphore;
#endif
#endif /* I2C_USE_MUTUAL_EXCLUSION */
#if defined(I2C_DRIVER_EXT_FIELDS)
  I2C_DRIVER_EXT_FIELDS
#endif
  /* End of the mandatory fields.*/
  /**
   * @brief   Thread waiting for I/O completion.
   */
  Thread                    *thread;
};

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/**
 * @brief   Get errors from I2C driver.
 *
 * @param[in] i2cp      pointer to the @p I2CDriver object
 *
 * @notapi
 */
#define i2c_lld_get_errors(i2cp) ((i2cp)->errors)

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if !defined(__DOXYGEN__)
#if SAM7_I2C_USE_I2C1
extern I2CDriver I2CD1;
#endif

#endif /* !defined(__DOXYGEN__) */

#ifdef __cplusplus
extern "C" {
#endif
  void i2c_lld_init(void);
  void i2c_lld_start(I2CDriver *i2cp);
  void i2c_lld_stop(I2CDriver *i2cp);
  msg_t i2c_lld_master_transmit_timeout(I2CDriver *i2cp, i2caddr_t addr,
                                        const uint8_t *txbuf, size_t txbytes,
                                        uint8_t *rxbuf, size_t rxbytes,
                                        systime_t timeout);
  msg_t i2c_lld_master_receive_timeout(I2CDriver *i2cp, i2caddr_t addr,
                                       uint8_t *rxbuf, size_t rxbytes,
                                       systime_t timeout);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_I2C  */

#endif /* _I2C_LLD_H_ */

/** @} */
