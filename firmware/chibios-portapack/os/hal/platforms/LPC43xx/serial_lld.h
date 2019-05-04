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
 * @file    LPC43xx/serial_lld.h
 * @brief   LPC43xx low level serial driver header.
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

#define IIR_SRC_MASK    0x0F
#define IIR_SRC_NONE    0x01
#define IIR_SRC_MODEM   0x00
#define IIR_SRC_TX      0x02
#define IIR_SRC_RX      0x04
#define IIR_SRC_ERROR   0x06
#define IIR_SRC_TIMEOUT 0x0C

#define IER_RBR         1
#define IER_THRE        2
#define IER_STATUS      4
#define IER_MS          8

#define LCR_WL5         0
#define LCR_WL6         1
#define LCR_WL7         2
#define LCR_WL8         3
#define LCR_STOP1       0
#define LCR_STOP2       4
#define LCR_NOPARITY    0
#define LCR_PARITYODD   0x08
#define LCR_PARITYEVEN  0x18
#define LCR_PARITYONE   0x28
#define LCR_PARITYZERO  0x38
#define LCR_BREAK_ON    0x40
#define LCR_DLAB        0x80

#define FCR_ENABLE      1
#define FCR_RXRESET     2
#define FCR_TXRESET     4
#define FCR_DMAMODE     8
#define FCR_TRIGGER0    0
#define FCR_TRIGGER1    0x40
#define FCR_TRIGGER2    0x80
#define FCR_TRIGGER3    0xC0

#define LSR_RBR_FULL    1
#define LSR_OVERRUN     2
#define LSR_PARITY      4
#define LSR_FRAMING     8
#define LSR_BREAK       0x10
#define LSR_THRE        0x20
#define LSR_TEMT        0x40
#define LSR_RXFE        0x80
#define LSR_TXERR       0x100   /* Smart Card T=0 mode only. */

#define TER_ENABLE      0x01

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @brief   USART0 driver enable switch.
 * @details If set to @p TRUE the support for USART0 is included.
 * @note    The default is @p TRUE .
 */
#if !defined(LPC_SERIAL_USE_USART0) || defined(__DOXYGEN__)
#define LPC_SERIAL_USE_USART0           FALSE
#endif

/**
 * @brief   UART1 driver enable switch.
 * @details If set to @p TRUE the support for UART1 is included.
 * @note    The default is @p TRUE .
 */
#if !defined(LPC_SERIAL_USE_UART1) || defined(__DOXYGEN__)
#define LPC_SERIAL_USE_UART1            FALSE
#endif

/**
 * @brief   USART2 driver enable switch.
 * @details If set to @p TRUE the support for USART2 is included.
 * @note    The default is @p TRUE .
 */
#if !defined(LPC_SERIAL_USE_USART2) || defined(__DOXYGEN__)
#define LPC_SERIAL_USE_USART2           FALSE
#endif

/**
 * @brief   USART3 driver enable switch.
 * @details If set to @p TRUE the support for USART3 is included.
 * @note    The default is @p TRUE .
 */
#if !defined(LPC_SERIAL_USE_USART3) || defined(__DOXYGEN__)
#define LPC_SERIAL_USE_USART3           FALSE
#endif

/**
 * @brief   FIFO preload parameter.
 * @details Configuration parameter, this values defines how many bytes are
 *          preloaded in the HW transmit FIFO for each interrupt, the maximum
 *          value is 16 the minimum is 1.
 * @note    An high value reduces the number of interrupts generated but can
 *          also increase the worst case interrupt response time because the
 *          preload loops.
 */
#if !defined(LPC_SERIAL_FIFO_PRELOAD) || defined(__DOXYGEN__)
#define LPC_SERIAL_FIFO_PRELOAD         16
#endif

/**
 * @brief   USART0 PCLK divider.
 */
#if !defined(LPC_SERIAL_USART0CLKDIV) || defined(__DOXYGEN__)
#define LPC_SERIAL_USART0CLKDIV         1
#endif

/**
 * @brief   UART1 PCLK divider.
 */
#if !defined(LPC_SERIAL_UART1CLKDIV) || defined(__DOXYGEN__)
#define LPC_SERIAL_UART1CLKDIV          1
#endif

/**
 * @brief   USART2 PCLK divider.
 */
#if !defined(LPC_SERIAL_USART2CLKDIV) || defined(__DOXYGEN__)
#define LPC_SERIAL_USART2CLKDIV         1
#endif

/**
 * @brief   USART3 PCLK divider.
 */
#if !defined(LPC_SERIAL_USART3CLKDIV) || defined(__DOXYGEN__)
#define LPC_SERIAL_USART3CLKDIV         1
#endif

/**
 * @brief   USART0 interrupt priority level setting.
 */
#if !defined(LPC_SERIAL_USART0_IRQ_PRIORITY) || defined(__DOXYGEN__)
  #if defined(LPC43XX_M4)
    #define LPC_SERIAL_USART0_IRQ_PRIORITY  3
  #else
    #define LPC_SERIAL_USART0_IRQ_PRIORITY  7
  #endif
#endif

/**
 * @brief   UART1 interrupt priority level setting.
 */
#if !defined(LPC_SERIAL_UART1_IRQ_PRIORITY) || defined(__DOXYGEN__)
  #if defined(LPC43XX_M4)
    #define LPC_SERIAL_UART1_IRQ_PRIORITY  3
  #else
    #define LPC_SERIAL_UART1_IRQ_PRIORITY  7
  #endif
#endif

/**
 * @brief   USART2 interrupt priority level setting.
 */
#if !defined(LPC_SERIAL_USART2_IRQ_PRIORITY) || defined(__DOXYGEN__)
  #if defined(LPC43XX_M4)
    #define LPC_SERIAL_USART2_IRQ_PRIORITY  3
  #else
    #define LPC_SERIAL_USART2_IRQ_PRIORITY  7
  #endif
#endif

/**
 * @brief   USART3 interrupt priority level setting.
 */
#if !defined(LPC_SERIAL_USART3_IRQ_PRIORITY) || defined(__DOXYGEN__)
  #if defined(LPC43XX_M4)
    #define LPC_SERIAL_USART3_IRQ_PRIORITY  3
  #else
    #define LPC_SERIAL_USART3_IRQ_PRIORITY  7
  #endif
#endif

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if (LPC_SERIAL_USART0CLKDIV < 1) || (LPC_SERIAL_USART0CLKDIV > 255)
#error "invalid LPC_SERIAL_USART0CLKDIV setting"
#endif

#if (LPC_SERIAL_UART1CLKDIV < 1) || (LPC_SERIAL_UART1CLKDIV > 255)
#error "invalid LPC_SERIAL_UART1CLKDIV setting"
#endif

#if (LPC_SERIAL_USART2CLKDIV < 1) || (LPC_SERIAL_USART2CLKDIV > 255)
#error "invalid LPC_SERIAL_USART2CLKDIV setting"
#endif

#if (LPC_SERIAL_USART3CLKDIV < 1) || (LPC_SERIAL_USART3CLKDIV > 255)
#error "invalid LPC_SERIAL_USART3CLKDIV setting"
#endif

#if (LPC_SERIAL_FIFO_PRELOAD < 1) || (LPC_SERIAL_FIFO_PRELOAD > 16)
#error "invalid LPC_SERIAL_FIFO_PRELOAD setting"
#endif

/**
 * @brief   USART0 clock.
 */
#define  LPC_SERIAL_USART0_PCLK                                         \
  (LPC_MAINCLK / LPC_SERIAL_USART0CLKDIV)

/**
 * @brief   UART1 clock.
 */
#define  LPC_SERIAL_UART1_PCLK                                          \
  (LPC_MAINCLK / LPC_SERIAL_UART1CLKDIV)

/**
 * @brief   USART2 clock.
 */
#define  LPC_SERIAL_USART2_PCLK                                         \
  (LPC_MAINCLK / LPC_SERIAL_USART2CLKDIV)

/**
 * @brief   USART3 clock.
 */
#define  LPC_SERIAL_USART3_PCLK                                         \
  (LPC_MAINCLK / LPC_SERIAL_USART3CLKDIV)

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Structure used for UART configuration.
 */
/**
 * @brief   LPC Serial Driver configuration structure.
 * @details An instance of this structure must be passed to @p sdStart()
 *          in order to configure and start a serial driver operations.
 */
typedef struct {
  /**
   * @brief Divisor value.
   */
  uint32_t                  sc_div;
  /**
   * @brief Initialization value for the LCR register.
   */
  uint32_t                  sc_lcr;
  /**
   * @brief Initialization value for the FCR register.
   */
  uint32_t                  sc_fcr;
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
  /* Pointer to the UART registers block.*/                                 \
  LPC_USART_Type        *uart;                                              \
  /* Pointer to the non-peripheral SSP resources.*/                         \
  const uart_resources_t * resources;

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if LPC_SERIAL_USE_USART0 && !defined(__DOXYGEN__)
extern SerialDriver SD1;
#endif

#if LPC_SERIAL_USE_UART1 && !defined(__DOXYGEN__)
extern SerialDriver SD2;
#endif

#if LPC_SERIAL_USE_USART2 && !defined(__DOXYGEN__)
extern SerialDriver SD3;
#endif

#if LPC_SERIAL_USE_USART3 && !defined(__DOXYGEN__)
extern SerialDriver SD4;
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void sd_lld_init(void);
  void sd_lld_start(SerialDriver *sdp, const SerialConfig *config);
  void sd_lld_stop(SerialDriver *sdp);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_SERIAL */

#endif /* _SERIAL_LLD_H_ */

/** @} */
