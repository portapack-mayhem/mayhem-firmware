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
 * @file    LPC43xx/i2c_lld.h
 * @brief   LPC43xx I2C subsystem low level driver header.
 *
 * @addtogroup I2C
 * @{
 */

#ifndef _I2C_LLD_H_
#define _I2C_LLD_H_

#if HAL_USE_I2C || defined(__DOXYGEN__)

/* I2C peripheral bears a striking resemblence to the AVR I2C peripheral. */

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/
#if 0
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
#endif
/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    Configuration options
 * @{
 */
/**
 * @brief   I2C0 driver enable switch.
 * @details If set to @p TRUE the support for I2C0 is included.
 * @note    The default is @p FALSE.
 */
#if !defined(LPC43XX_I2C_USE_I2C0) || defined(__DOXYGEN__)
#define LPC43XX_I2C_USE_I2C0               FALSE
#endif

/**
 * @brief   I2C1 driver enable switch.
 * @details If set to @p TRUE the support for I2C1 is included.
 * @note    The default is @p FALSE.
 */
#if !defined(LPC43XX_I2C_USE_I2C1) || defined(__DOXYGEN__)
#define LPC43XX_I2C_USE_I2C1               FALSE
#endif
/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Structure used for I2C configuration.
 */
typedef LPC_I2Cx_Type* I2C_TypeDef;

/**
 * @brief   Type representing I2C address.
 */
typedef uint8_t i2caddr_t;

/**
 * @brief   I2C Driver condition flags type.
 */
typedef uint8_t i2cflags_t;

/**
 * @brief   Driver configuration structure.
 * @note    Implementations may extend this structure to contain more,
 *          architecture dependent, fields.
 */
typedef struct {

  /**
   * @brief Specifies the I2C clock frequency and duty cycle.
   */
  uint16_t        high_count;
  uint16_t        low_count;

} I2CConfig;

/**
 * @brief Structure representing an I2C driver.
 */
struct I2CDriver {
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
  /**
   * @brief   Address of slave device.
   */
  uint8_t                   addr_r;
  /**
   * @brief   Pointer to the buffer with data to send.
   */
  const uint8_t             *txbuf;
  /**
   * @brief   Number of bytes of data to send.
   */
  size_t                    txbytes;
  /**
   * @brief   Pointer to the buffer to put received data.
   */
  uint8_t                   *rxbuf;
  /**
   * @brief   Number of bytes of data to receive.
   */
  size_t                    rxbytes;
  /**
   * @brief     Pointer to the I2Cx registers block.
   */
  I2C_TypeDef               i2c;
  /**
   * @brief Pointer to the non-peripheral I2C resources.
   */
  const i2c_resources_t * resources;
};

/**
 * @brief   Type of a structure representing an I2C driver.
 */
typedef struct I2CDriver I2CDriver;

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
#if LPC43XX_I2C_USE_I2C0
extern I2CDriver I2CD0;
#endif
#if LPC43XX_I2C_USE_I2C1
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
