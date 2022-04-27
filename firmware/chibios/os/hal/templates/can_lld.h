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
 * @file    templates/can_lld.h
 * @brief   CAN Driver subsystem low level driver header template.
 *
 * @addtogroup CAN
 * @{
 */

#ifndef _CAN_LLD_H_
#define _CAN_LLD_H_

#if HAL_USE_CAN || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @brief   This switch defines whether the driver implementation supports
 *          a low power switch mode with automatic an wakeup feature.
 */
#define CAN_SUPPORTS_SLEEP          TRUE

/**
 * @brief   This implementation supports three transmit mailboxes.
 */
#define CAN_TX_MAILBOXES            3

/**
 * @brief   This implementation supports two receive mailboxes.
 */
#define CAN_RX_MAILBOXES            2

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    Configuration options
 * @{
 */
/**
 * @brief   CAN1 driver enable switch.
 * @details If set to @p TRUE the support for CAN1 is included.
 */
#if !defined(PLATFORM_CAN_USE_CAN1) || defined(__DOXYGEN__)
#define PLATFORM_CAN_USE_CAN1               FALSE
#endif
/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if CAN_USE_SLEEP_MODE && !CAN_SUPPORTS_SLEEP
#error "CAN sleep mode not supported in this architecture"
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Type of a transmission mailbox index.
 */
typedef uint32_t canmbx_t;

/**
 * @brief   CAN transmission frame.
 * @note    Accessing the frame data as word16 or word32 is not portable because
 *          machine data endianness, it can be still useful for a quick filling.
 */
typedef struct {
  struct {
    uint8_t                 DLC:4;          /**< @brief Data length.        */
    uint8_t                 RTR:1;          /**< @brief Frame type.         */
    uint8_t                 IDE:1;          /**< @brief Identifier type.    */
  };
  union {
    struct {
      uint32_t              SID:11;         /**< @brief Standard identifier.*/
    };
    struct {
      uint32_t              EID:29;         /**< @brief Extended identifier.*/
    };
  };
  union {
    uint8_t                 data8[8];       /**< @brief Frame data.         */
    uint16_t                data16[4];      /**< @brief Frame data.         */
    uint32_t                data32[2];      /**< @brief Frame data.         */
  };
} CANTxFrame;

/**
 * @brief   CAN received frame.
 * @note    Accessing the frame data as word16 or word32 is not portable because
 *          machine data endianness, it can be still useful for a quick filling.
 */
typedef struct {
  struct {
    uint8_t                 DLC:4;          /**< @brief Data length.        */
    uint8_t                 RTR:1;          /**< @brief Frame type.         */
    uint8_t                 IDE:1;          /**< @brief Identifier type.    */
  };
  union {
    struct {
      uint32_t              SID:11;         /**< @brief Standard identifier.*/
    };
    struct {
      uint32_t              EID:29;         /**< @brief Extended identifier.*/
    };
  };
  union {
    uint8_t                 data8[8];       /**< @brief Frame data.         */
    uint16_t                data16[4];      /**< @brief Frame data.         */
    uint32_t                data32[2];      /**< @brief Frame data.         */
  };
} CANRxFrame;

/**
 * @brief   CAN filter.
 * @note    Implementations may extend this structure to contain more,
 *          architecture dependent, fields.
 * @note    It could not be present on some architectures.
 */
typedef struct {
  uint32_t                  dummy;
} CANFilter;

/**
 * @brief   Driver configuration structure.
 * @note    Implementations may extend this structure to contain more,
 *          architecture dependent, fields.
 * @note    It could be empty on some architectures.
 */
typedef struct {
  uint32_t                  dummy;
} CANConfig;

/**
 * @brief   Structure representing an CAN driver.
 */
typedef struct {
  /**
   * @brief   Driver state.
   */
  canstate_t                state;
  /**
   * @brief   Current configuration data.
   */
  const CANConfig           *config;
  /**
   * @brief   Transmission queue semaphore.
   */
  Semaphore                 txsem;
  /**
   * @brief   Receive queue semaphore.
   */
  Semaphore                 rxsem;
  /**
   * @brief   One or more frames become available.
   * @note    After broadcasting this event it will not be broadcasted again
   *          until the received frames queue has been completely emptied. It
   *          is <b>not</b> broadcasted for each received frame. It is
   *          responsibility of the application to empty the queue by
   *          repeatedly invoking @p chReceive() when listening to this event.
   *          This behavior minimizes the interrupt served by the system
   *          because CAN traffic.
   * @note    The flags associated to the listeners will indicate which
   *          receive mailboxes become non-empty.
   */
  EventSource               rxfull_event;
  /**
   * @brief   One or more transmission mailbox become available.
   * @note    The flags associated to the listeners will indicate which
   *          transmit mailboxes become empty.
   *
   */
  EventSource               txempty_event;
  /**
   * @brief   A CAN bus error happened.
   * @note    The flags associated to the listeners will indicate the
   *          error(s) that have occurred.
   */
  EventSource               error_event;
#if CAN_USE_SLEEP_MODE || defined (__DOXYGEN__)
  /**
   * @brief   Entering sleep state event.
   */
  EventSource               sleep_event;
  /**
   * @brief   Exiting sleep state event.
   */
  EventSource               wakeup_event;
#endif /* CAN_USE_SLEEP_MODE */
  /* End of the mandatory fields.*/
} CANDriver;

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if PLATFORM_CAN_USE_CAN1 && !defined(__DOXYGEN__)
extern CANDriver CAND1;
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void can_lld_init(void);
  void can_lld_start(CANDriver *canp);
  void can_lld_stop(CANDriver *canp);
  bool_t can_lld_is_tx_empty(CANDriver *canp,
                             canmbx_t mailbox);
  void can_lld_transmit(CANDriver *canp,
                        canmbx_t mailbox,
                        const CANTxFrame *crfp);
  bool_t can_lld_is_rx_nonempty(CANDriver *canp,
                                canmbx_t mailbox);
  void can_lld_receive(CANDriver *canp,
                       canmbx_t mailbox,
                       CANRxFrame *ctfp);
#if CAN_USE_SLEEP_MODE
  void can_lld_sleep(CANDriver *canp);
  void can_lld_wakeup(CANDriver *canp);
#endif /* CAN_USE_SLEEP_MODE */
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_CAN */

#endif /* _CAN_LLD_H_ */

/** @} */
