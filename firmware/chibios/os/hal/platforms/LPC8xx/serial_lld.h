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
 * @file    LPC8xx/serial_lld.h
 * @brief   LPC8xx low level serial driver header.
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

#define CFG_ENA               0x0001
#define CFG_DL7               0x0000
#define CFG_DL8               0x0004
#define CFG_DL9               0x0008
#define CFG_NOPARITY          0x0000
#define CFG_PARITYEVEN        0x0020
#define CFG_PARITYODD         0x0030
#define CFG_STOP1             0x0000
#define CFG_STOP2             0x0040
#define CFG_CTSEN             0x0200
#define CFG_SYNCEN            0x0800
#define CFG_CLKPOL_FALL       0x0000
#define CFG_CLKPOL_RISE       0x1000
#define CFG_SYNC_SLV          0x0000
#define CFG_SYNC_MAST         0x4000
#define CFG_LOOP_EN           0x8000

#define CTRL_TXBRKEN          0x0002
#define CTRL_ADDRDET          0x0004
#define CTRL_TXDIS            0x0040
#define CTRL_CC               0x0100
#define CTRL_CLRCC            0x0200

#define STAT_RXRDY            0x0001
#define STAT_RXIDLE           0x0002
#define STAT_TXRDY            0x0004
#define STAT_TXIDLE           0x0008
#define STAT_CTS              0x0010
#define STAT_DELTACTS         0x0020
#define STAT_TXDIS            0x0040
#define STAT_OVERRUN          0x0100
#define STAT_RXBRK            0x0400
#define STAT_DELTARXBRK       0x0800
#define STAT_START            0x1000
#define STAT_FRAMERR          0x2000
#define STAT_PARITYERR        0x4000
#define STAT_RXNOISE          0x8000


/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @brief   UART0 driver enable switch.
 * @details If set to @p TRUE the support for UART0 is included.
 * @note    The default is @p TRUE .
 */
#if !defined(LPC8xx_SERIAL_USE_UART0) || defined(__DOXYGEN__)
#define LPC8xx_SERIAL_USE_UART0            TRUE
#endif

/**
 * @brief   UART1 driver enable switch.
 * @details If set to @p TRUE the support for UART1 is included.
 * @note    The default is @p FALSE .
 */
#if !defined(LPC8xx_SERIAL_USE_UART1) || defined(__DOXYGEN__)
#define LPC8xx_SERIAL_USE_UART1            FALSE
#endif

/**
 * @brief   UART2 driver enable switch.
 * @details If set to @p TRUE the support for UART2 is included.
 * @note    The default is @p FALSE .
 */
#if !defined(LPC8xx_SERIAL_USE_UART2) || defined(__DOXYGEN__)
#define LPC8xx_SERIAL_USE_UART2            FALSE
#endif

/**
 * @brief   UART0 interrupt priority level setting.
 */
#if !defined(LPC8xx_SERIAL_UART0_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define LPC8xx_SERIAL_UART0_IRQ_PRIORITY   3
#endif

/**
 * @brief   UART1 interrupt priority level setting.
 */
#if !defined(LPC8xx_SERIAL_UART1_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define LPC8xx_SERIAL_UART1_IRQ_PRIORITY   3
#endif

/**
 * @brief   UART2 interrupt priority level setting.
 */
#if !defined(LPC8xx_SERIAL_UART2_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define LPC8xx_SERIAL_UART2_IRQ_PRIORITY   3
#endif


/**
 * @brief   Uart Baud Clock (U_PCLK).
 * @details The Baud clock rate we would like to achieve.
 * @note    The default is @p 11.0592MHz.
 *          A multiple of 1.8432MHz will give accurate
 *          results at all standard baud rates  .
 */
#if !defined(LPC8xx_SERIAL_U_PCLK) || defined(__DOXYGEN__)
#define LPC8xx_SERIAL_U_PCLK           11059200
#endif

/**
 * @brief   UARTCLKDIV divider.
 */
#if !defined(LPC8xx_SERIAL_UARTCLKDIV) || defined(__DOXYGEN__)
#define LPC8xx_SERIAL_UARTCLKDIV      (LPC8xx_MAINCLK/LPC8xx_SERIAL_U_PCLK)
#endif

// Output from uart clock divider
#define LPC8xx_UARTDIVCLK             (LPC8xx_MAINCLK/LPC8xx_SERIAL_UARTCLKDIV)

/**
 * @brief   UARTFRGDIV
 * @details Fractional Baud rate generator denominator.
 * @note    If used, *must* be set to 256, otherwise set to 0
 */
#if !defined(LPC8xx_SERIAL_UARTFRGDIV) || defined(__DOXYGEN__)
 #if (LPC8xx_SERIAL_UARTCLKDIV !=  LPC8xx_SERIAL_U_PCLK)
  #define LPC8xx_SERIAL_UARTFRGDIV          0xFF
 #else
  #define LPC8xx_SERIAL_UARTFRGDIV          0x00
 #endif
#endif

/**
 * @brief   UARTFRGMUL
 * @details Fractional Baud rate generator numerator.
 *          Refer to LPC8xx User Manual 4.6.19 for calculation
 * @note    the *2, +1 and /2 are included to round to the nearest integer.
 */
#if !defined(LPC8xx_SERIAL_UARTFRGMUL) || defined(__DOXYGEN__)
 #if (LPC8xx_SERIAL_UARTCLKDIV !=  LPC8xx_SERIAL_U_PCLK)
  #define LPC8xx_SERIAL_UARTFRGMULT    ( ( ( ( (LPC8xx_UARTDIVCLK-            \
                                               LPC8xx_SERIAL_U_PCLK)          \
                                             *256*2 )                         \
                                           /LPC8xx_SERIAL_U_PCLK )            \
                                         +1 )                                 \
                                       /2 )

 #else
  #define LPC8xx_SERIAL_UARTFRGMULT   0x00
 #endif
#endif

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if (LPC8xx_SERIAL_UARTCLKDIV < 1) || (LPC8xx_SERIAL_UARTCLKDIV > 255)
#error "invalid LPC8xx_SERIAL_UARTCLKDIV setting"
#endif

#if (LPC8xx_SERIAL_UARTFRGDIV != 0) && (LPC8xx_SERIAL_UARTFRGDIV != 255)
#error "invalid LPC8xx_SERIAL_UARTFRGDIV setting"
#endif

#if (LPC8xx_SERIAL_UARTFRGMUL != 0) && (LPC8xx_SERIAL_UARTFRGMUL > 255)
#error "invalid LPC8xx_SERIAL_UARTFRGMUL setting"
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   LPC8xx Serial Driver configuration structure.
 * @details An instance of this structure must be passed to @p sdStart()
 *          in order to configure and start a serial driver operations.
 */
typedef struct {
  /**
   * @brief Bit rate.
   */
  uint32_t                  sc_speed;
  /**
   * @brief Initialization value for the CFG register.
   */
  uint32_t                  sc_cfg;
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
  LPC_USART_TypeDef        *uart;

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if LPC8xx_SERIAL_USE_UART0 && !defined(__DOXYGEN__)
extern SerialDriver SD1;
#endif

#if LPC8xx_SERIAL_USE_UART1 && !defined(__DOXYGEN__)
extern SerialDriver SD2;
#endif

#if LPC8xx_SERIAL_USE_UART2 && !defined(__DOXYGEN__)
extern SerialDriver SD3;
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
