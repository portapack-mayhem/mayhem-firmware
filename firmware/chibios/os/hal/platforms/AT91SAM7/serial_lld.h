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
 * @file    AT91SAM7/serial_lld.h
 * @brief   AT91SAM7 low level serial driver header.
 *
 * @addtogroup SERIAL
 * @{
 */

#ifndef _SERIAL_LLD_H_
#define _SERIAL_LLD_H_

#if HAL_USE_SERIAL || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @brief   UART0 driver enable switch.
 * @details If set to @p TRUE the support for USART1 is included.
 * @note    The default is @p TRUE.
 */
#if !defined(USE_SAM7_USART0) || defined(__DOXYGEN__)
#define USE_SAM7_USART0             TRUE
#endif

/**
 * @brief   UART1 driver enable switch.
 * @details If set to @p TRUE the support for USART2 is included.
 * @note    The default is @p TRUE.
 */
#if !defined(USE_SAM7_USART1) || defined(__DOXYGEN__)
#define USE_SAM7_USART1             TRUE
#endif

#if (SAM7_PLATFORM == SAM7A3)
/**
 * @brief   UART2 driver enable switch.
 * @details If set to @p TRUE the support for USART3 is included.
 * @note    The default is @p TRUE.
 */
#if !defined(USE_SAM7_USART2) || defined(__DOXYGEN__)
#define USE_SAM7_USART2             TRUE
#endif
#endif /* (SAM7_PLATFORM == SAM7A3) */

/**
 * @brief   DBGU UART driver enable switch.
 * @details If set to @p TRUE the support for the DBGU UART is included.
 * @note    The default is @p TRUE.
 */
#if !defined(USE_SAM7_DBGU_UART) || defined(__DOXYGEN__)
#define USE_SAM7_DBGU_UART          TRUE
#endif

/**
 * @brief   UART1 interrupt priority level setting.
 */
#if !defined(SAM7_USART0_PRIORITY) || defined(__DOXYGEN__)
#define SAM7_USART0_PRIORITY        (AT91C_AIC_PRIOR_HIGHEST - 2)
#endif

/**
 * @brief   UART2 interrupt priority level setting.
 */
#if !defined(SAM7_USART1_PRIORITY) || defined(__DOXYGEN__)
#define SAM7_USART1_PRIORITY        (AT91C_AIC_PRIOR_HIGHEST - 2)
#endif

#if (SAM7_PLATFORM == SAM7A3)
/**
 * @brief   UART2 interrupt priority level setting.
 */
#if !defined(SAM7_USART2_PRIORITY) || defined(__DOXYGEN__)
#define SAM7_USART2_PRIORITY        (AT91C_AIC_PRIOR_HIGHEST - 2)
#endif
#endif /* (SAM7_PLATFORM == SAM7A3) */

/**
 * @brief   DBGU_UART interrupt priority level setting.
 */
#if !defined(SAM7_DBGU_UART_PRIORITY) || defined(__DOXYGEN__)
#define SAM7_DBGU_UART_PRIORITY        (AT91C_AIC_PRIOR_HIGHEST - 2)
#endif

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   AT91SAM7 Serial Driver configuration structure.
 * @details An instance of this structure must be passed to @p sdStart()
 *          in order to configure and start a serial driver operations.
 */
typedef struct {
  /**
   * @brief   Bit rate.
   * @details This is written to the US_BRGR register of the appropriate AT91S_USART
   */
  uint32_t                  sc_speed;
  /**
   * @brief   Initialization value for the MR register.
   * @details This is written to the US_MR register of the appropriate AT91S_USART
   */
  uint32_t                  sc_mr;
} SerialConfig;

/**
 * @brief   @p SerialDriver specific data.
 */
#define _serial_driver_data                                                 \
  _base_asynchronous_channel_data                                           \
  /* Driver state.*/                                                        \
  sdstate_t                 state;                                          \
  /* Input queue.*/                                                         \
  InputQueue                iqueue;                                         \
  /* Output queue.*/                                                        \
  OutputQueue               oqueue;                                         \
  /* Input circular buffer.*/                                               \
  uint8_t                   ib[SERIAL_BUFFERS_SIZE];                        \
  /* Output circular buffer.*/                                              \
  uint8_t                   ob[SERIAL_BUFFERS_SIZE];                        \
  /* End of the mandatory fields.*/                                         \
  /* Pointer to the USART registers block.*/                                \
  AT91PS_USART              usart;

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if USE_SAM7_USART0 && !defined(__DOXYGEN__)
extern SerialDriver SD1;
#endif
#if USE_SAM7_USART1 && !defined(__DOXYGEN__)
extern SerialDriver SD2;
#endif
#if (SAM7_PLATFORM == SAM7A3)
#if USE_SAM7_USART2 && !defined(__DOXYGEN__)
extern SerialDriver SD3;
#endif
#endif
#if USE_SAM7_DBGU_UART
extern SerialDriver SDDBG;
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void sd_lld_init(void);
  void sd_lld_start(SerialDriver *sdp, const SerialConfig *config);
  void sd_lld_stop(SerialDriver *sdp);
#if USE_SAM7_DBGU_UART
  void sd_lld_serve_interrupt(SerialDriver *sdp);
#endif
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_SERIAL */

#endif /* _SERIAL_LLD_H_ */

/** @} */
