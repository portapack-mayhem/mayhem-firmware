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
 * @file    STM32/USARTv1/uart_lld.h
 * @brief   STM32 low level UART driver header.
 *
 * @addtogroup UART
 * @{
 */

#ifndef _UART_LLD_H_
#define _UART_LLD_H_

#if HAL_USE_UART || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    Configuration options
 * @{
 */
/**
 * @brief   UART driver on USART1 enable switch.
 * @details If set to @p TRUE the support for USART1 is included.
 * @note    The default is @p FALSE.
 */
#if !defined(STM32_UART_USE_USART1) || defined(__DOXYGEN__)
#define STM32_UART_USE_USART1               FALSE
#endif

/**
 * @brief   UART driver on USART2 enable switch.
 * @details If set to @p TRUE the support for USART2 is included.
 * @note    The default is @p FALSE.
 */
#if !defined(STM32_UART_USE_USART2) || defined(__DOXYGEN__)
#define STM32_UART_USE_USART2               FALSE
#endif

/**
 * @brief   UART driver on USART3 enable switch.
 * @details If set to @p TRUE the support for USART3 is included.
 * @note    The default is @p FALSE.
 */
#if !defined(STM32_UART_USE_USART3) || defined(__DOXYGEN__)
#define STM32_UART_USE_USART3               FALSE
#endif

/**
 * @brief   UART driver on UART4 enable switch.
 * @details If set to @p TRUE the support for UART4 is included.
 * @note    The default is @p FALSE.
 */
#if !defined(STM32_UART_USE_UART4) || defined(__DOXYGEN__)
#define STM32_UART_USE_UART4                FALSE
#endif

/**
 * @brief   UART driver on UART5 enable switch.
 * @details If set to @p TRUE the support for UART5 is included.
 * @note    The default is @p FALSE.
 */
#if !defined(STM32_UART_USE_UART5) || defined(__DOXYGEN__)
#define STM32_UART_USE_UART5                FALSE
#endif

/**
 * @brief   UART driver on USART6 enable switch.
 * @details If set to @p TRUE the support for USART6 is included.
 * @note    The default is @p FALSE.
 */
#if !defined(STM32_UART_USE_USART6) || defined(__DOXYGEN__)
#define STM32_UART_USE_USART6               FALSE
#endif

/**
 * @brief   USART1 interrupt priority level setting.
 */
#if !defined(STM32_UART_USART1_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define STM32_UART_USART1_IRQ_PRIORITY      12
#endif

/**
 * @brief   USART2 interrupt priority level setting.
 */
#if !defined(STM32_UART_USART2_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define STM32_UART_USART2_IRQ_PRIORITY      12
#endif

/**
 * @brief   USART3 interrupt priority level setting.
 */
#if !defined(STM32_UART_USART3_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define STM32_UART_USART3_IRQ_PRIORITY      12
#endif

/**
 * @brief   UART4 interrupt priority level setting.
 */
#if !defined(STM32_UART_UART4_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define STM32_UART_UART4_IRQ_PRIORITY       12
#endif

/**
 * @brief   UART5 interrupt priority level setting.
 */
#if !defined(STM32_UART_UART5_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define STM32_UART_UART5_IRQ_PRIORITY       12
#endif

/**
 * @brief   USART6 interrupt priority level setting.
 */
#if !defined(STM32_UART_USART6_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define STM32_UART_USART6_IRQ_PRIORITY      12
#endif

/**
 * @brief   USART1 DMA priority (0..3|lowest..highest).
 * @note    The priority level is used for both the TX and RX DMA channels but
 *          because of the channels ordering the RX channel has always priority
 *          over the TX channel.
 */
#if !defined(STM32_UART_USART1_DMA_PRIORITY) || defined(__DOXYGEN__)
#define STM32_UART_USART1_DMA_PRIORITY      0
#endif

/**
 * @brief   USART2 DMA priority (0..3|lowest..highest).
 * @note    The priority level is used for both the TX and RX DMA channels but
 *          because of the channels ordering the RX channel has always priority
 *          over the TX channel.
 */
#if !defined(STM32_UART_USART2_DMA_PRIORITY) || defined(__DOXYGEN__)
#define STM32_UART_USART2_DMA_PRIORITY      0
#endif

/**
 * @brief   USART3 DMA priority (0..3|lowest..highest).
 * @note    The priority level is used for both the TX and RX DMA channels but
 *          because of the channels ordering the RX channel has always priority
 *          over the TX channel.
 */
#if !defined(STM32_UART_USART3_DMA_PRIORITY) || defined(__DOXYGEN__)
#define STM32_UART_USART3_DMA_PRIORITY      0
#endif

/**
 * @brief   UART4 DMA priority (0..3|lowest..highest).
 * @note    The priority level is used for both the TX and RX DMA channels but
 *          because of the channels ordering the RX channel has always priority
 *          over the TX channel.
 */
#if !defined(STM32_UART_UART4_DMA_PRIORITY) || defined(__DOXYGEN__)
#define STM32_UART_UART4_DMA_PRIORITY       0
#endif

/**
 * @brief   UART5 DMA priority (0..3|lowest..highest).
 * @note    The priority level is used for both the TX and RX DMA channels but
 *          because of the channels ordering the RX channel has always priority
 *          over the TX channel.
 */
#if !defined(STM32_UART_UART5_DMA_PRIORITY) || defined(__DOXYGEN__)
#define STM32_UART_UART5_DMA_PRIORITY       0
#endif

/**
 * @brief   USART6 DMA priority (0..3|lowest..highest).
 * @note    The priority level is used for both the TX and RX DMA channels but
 *          because of the channels ordering the RX channel has always priority
 *          over the TX channel.
 */
#if !defined(STM32_UART_USART6_DMA_PRIORITY) || defined(__DOXYGEN__)
#define STM32_UART_USART6_DMA_PRIORITY      0
#endif

/**
 * @brief   USART DMA error hook.
 * @note    The default action for DMA errors is a system halt because DMA
 *          error can only happen because programming errors.
 */
#if !defined(STM32_UART_DMA_ERROR_HOOK) || defined(__DOXYGEN__)
#define STM32_UART_DMA_ERROR_HOOK(uartp)    chSysHalt()
#endif

#if STM32_ADVANCED_DMA || defined(__DOXYGEN__)

/**
 * @brief   DMA stream used for USART1 RX operations.
 * @note    This option is only available on platforms with enhanced DMA.
 */
#if !defined(STM32_UART_USART1_RX_DMA_STREAM) || defined(__DOXYGEN__)
#define STM32_UART_USART1_RX_DMA_STREAM     STM32_DMA_STREAM_ID(2, 5)
#endif

/**
 * @brief   DMA stream used for USART1 TX operations.
 * @note    This option is only available on platforms with enhanced DMA.
 */
#if !defined(STM32_UART_USART1_TX_DMA_STREAM) || defined(__DOXYGEN__)
#define STM32_UART_USART1_TX_DMA_STREAM     STM32_DMA_STREAM_ID(2, 7)
#endif

/**
 * @brief   DMA stream used for USART2 RX operations.
 * @note    This option is only available on platforms with enhanced DMA.
 */
#if !defined(STM32_UART_USART2_RX_DMA_STREAM) || defined(__DOXYGEN__)
#define STM32_UART_USART2_RX_DMA_STREAM     STM32_DMA_STREAM_ID(1, 5)
#endif

/**
 * @brief   DMA stream used for USART2 TX operations.
 * @note    This option is only available on platforms with enhanced DMA.
 */
#if !defined(STM32_UART_USART2_TX_DMA_STREAM) || defined(__DOXYGEN__)
#define STM32_UART_USART2_TX_DMA_STREAM     STM32_DMA_STREAM_ID(1, 6)
#endif

/**
 * @brief   DMA stream used for USART3 RX operations.
 * @note    This option is only available on platforms with enhanced DMA.
 */
#if !defined(STM32_UART_USART3_RX_DMA_STREAM) || defined(__DOXYGEN__)
#define STM32_UART_USART3_RX_DMA_STREAM     STM32_DMA_STREAM_ID(1, 1)
#endif

/**
 * @brief   DMA stream used for USART3 TX operations.
 * @note    This option is only available on platforms with enhanced DMA.
 */
#if !defined(STM32_UART_USART3_TX_DMA_STREAM) || defined(__DOXYGEN__)
#define STM32_UART_USART3_TX_DMA_STREAM     STM32_DMA_STREAM_ID(1, 3)
#endif

/**
 * @brief   DMA stream used for UART4 RX operations.
 * @note    This option is only available on platforms with enhanced DMA.
 */
#if !defined(STM32_UART_UART4_RX_DMA_STREAM) || defined(__DOXYGEN__)
#define STM32_UART_UART4_RX_DMA_STREAM     STM32_DMA_STREAM_ID(1, 2)
#endif

/**
 * @brief   DMA stream used for UART4 TX operations.
 * @note    This option is only available on platforms with enhanced DMA.
 */
#if !defined(STM32_UART_UART4_TX_DMA_STREAM) || defined(__DOXYGEN__)
#define STM32_UART_UART4_TX_DMA_STREAM     STM32_DMA_STREAM_ID(1, 4)
#endif

/**
 * @brief   DMA stream used for UART5 RX operations.
 * @note    This option is only available on platforms with enhanced DMA.
 */
#if !defined(STM32_UART_UART5_RX_DMA_STREAM) || defined(__DOXYGEN__)
#define STM32_UART_UART5_RX_DMA_STREAM     STM32_DMA_STREAM_ID(1, 0)
#endif

/**
 * @brief   DMA stream used for UART5 TX operations.
 * @note    This option is only available on platforms with enhanced DMA.
 */
#if !defined(STM32_UART_UART5_TX_DMA_STREAM) || defined(__DOXYGEN__)
#define STM32_UART_UART5_TX_DMA_STREAM     STM32_DMA_STREAM_ID(1, 7)
#endif

/**
 * @brief   DMA stream used for USART6 RX operations.
 * @note    This option is only available on platforms with enhanced DMA.
 */
#if !defined(STM32_UART_USART6_RX_DMA_STREAM) || defined(__DOXYGEN__)
#define STM32_UART_USART6_RX_DMA_STREAM     STM32_DMA_STREAM_ID(2, 2)
#endif

/**
 * @brief   DMA stream used for USART6 TX operations.
 * @note    This option is only available on platforms with enhanced DMA.
 */
#if !defined(STM32_UART_USART6_TX_DMA_STREAM) || defined(__DOXYGEN__)
#define STM32_UART_USART6_TX_DMA_STREAM     STM32_DMA_STREAM_ID(2, 7)
#endif

#else /* !STM32_ADVANCED_DMA */

/* Fixed streams for platforms using the old DMA peripheral, the values are
   valid for both STM32F1xx and STM32L1xx.*/
#define STM32_UART_USART1_RX_DMA_STREAM     STM32_DMA_STREAM_ID(1, 5)
#define STM32_UART_USART1_TX_DMA_STREAM     STM32_DMA_STREAM_ID(1, 4)
#define STM32_UART_USART2_RX_DMA_STREAM     STM32_DMA_STREAM_ID(1, 6)
#define STM32_UART_USART2_TX_DMA_STREAM     STM32_DMA_STREAM_ID(1, 7)
#define STM32_UART_USART3_RX_DMA_STREAM     STM32_DMA_STREAM_ID(1, 3)
#define STM32_UART_USART3_TX_DMA_STREAM     STM32_DMA_STREAM_ID(1, 2)
#define STM32_UART_UART4_RX_DMA_STREAM      STM32_DMA_STREAM_ID(2, 3)
#define STM32_UART_UART4_TX_DMA_STREAM      STM32_DMA_STREAM_ID(2, 5)

#endif /* !STM32_ADVANCED_DMA*/
/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if STM32_UART_USE_USART1 && !STM32_HAS_USART1
#error "USART1 not present in the selected device"
#endif

#if STM32_UART_USE_USART2 && !STM32_HAS_USART2
#error "USART2 not present in the selected device"
#endif

#if STM32_UART_USE_USART3 && !STM32_HAS_USART3
#error "USART3 not present in the selected device"
#endif

#if STM32_UART_USE_UART4 
#if !STM32_HAS_UART4
#error "UART4 not present in the selected device"
#endif

#if !defined(STM32F2XX) && !defined(STM32F4XX)
#error "UART4 DMA access not supported in this platform"
#endif
#endif /* STM32_UART_USE_UART4 */

#if STM32_UART_USE_UART5
#if !STM32_HAS_UART5
#error "UART5 not present in the selected device"
#endif

#if !defined(STM32F2XX) && !defined(STM32F4XX)
#error "UART5 DMA access not supported in this platform"
#endif
#endif /* STM32_UART_USE_UART5 */

#if STM32_UART_USE_USART6 && !STM32_HAS_USART6
#error "USART6 not present in the selected device"
#endif

#if !STM32_UART_USE_USART1 && !STM32_UART_USE_USART2 &&                     \
    !STM32_UART_USE_USART3 && !STM32_UART_USE_UART4 &&                      \
    !STM32_UART_USE_UART5  && !STM32_UART_USE_USART6
#error "UART driver activated but no USART/UART peripheral assigned"
#endif

#if STM32_UART_USE_USART1 &&                                                \
    !CORTEX_IS_VALID_KERNEL_PRIORITY(STM32_UART_USART1_IRQ_PRIORITY)
#error "Invalid IRQ priority assigned to USART1"
#endif

#if STM32_UART_USE_USART2 &&                                                \
    !CORTEX_IS_VALID_KERNEL_PRIORITY(STM32_UART_USART2_IRQ_PRIORITY)
#error "Invalid IRQ priority assigned to USART2"
#endif

#if STM32_UART_USE_USART3 &&                                                \
    !CORTEX_IS_VALID_KERNEL_PRIORITY(STM32_UART_USART3_IRQ_PRIORITY)
#error "Invalid IRQ priority assigned to USART3"
#endif

#if STM32_UART_USE_UART4 &&                                                 \
    !CORTEX_IS_VALID_KERNEL_PRIORITY(STM32_UART_UART4_IRQ_PRIORITY)
#error "Invalid IRQ priority assigned to UART4"
#endif

#if STM32_UART_USE_UART5 &&                                                 \
    !CORTEX_IS_VALID_KERNEL_PRIORITY(STM32_UART_UART5_IRQ_PRIORITY)
#error "Invalid IRQ priority assigned to UART5"
#endif

#if STM32_UART_USE_USART6 &&                                                \
    !CORTEX_IS_VALID_KERNEL_PRIORITY(STM32_UART_USART6_IRQ_PRIORITY)
#error "Invalid IRQ priority assigned to USART6"
#endif

#if STM32_UART_USE_USART1 &&                                                \
    !STM32_DMA_IS_VALID_PRIORITY(STM32_UART_USART1_DMA_PRIORITY)
#error "Invalid DMA priority assigned to USART1"
#endif

#if STM32_UART_USE_USART2 &&                                                \
    !STM32_DMA_IS_VALID_PRIORITY(STM32_UART_USART2_DMA_PRIORITY)
#error "Invalid DMA priority assigned to USART2"
#endif

#if STM32_UART_USE_USART3 &&                                                \
    !STM32_DMA_IS_VALID_PRIORITY(STM32_UART_USART3_DMA_PRIORITY)
#error "Invalid DMA priority assigned to USART3"
#endif

#if STM32_UART_USE_UART4 &&                                                 \
    !STM32_DMA_IS_VALID_PRIORITY(STM32_UART_UART4_DMA_PRIORITY)
#error "Invalid DMA priority assigned to UART4"
#endif

#if STM32_UART_USE_UART5 &&                                                 \
    !STM32_DMA_IS_VALID_PRIORITY(STM32_UART_UART5_DMA_PRIORITY)
#error "Invalid DMA priority assigned to UART5"
#endif

#if STM32_UART_USE_USART6 &&                                                \
    !STM32_DMA_IS_VALID_PRIORITY(STM32_UART_USART6_DMA_PRIORITY)
#error "Invalid DMA priority assigned to USART6"
#endif

#if STM32_UART_USE_USART1 &&                                                \
    !STM32_DMA_IS_VALID_ID(STM32_UART_USART1_RX_DMA_STREAM,                 \
                           STM32_USART1_RX_DMA_MSK)
#error "invalid DMA stream associated to USART1 RX"
#endif

#if STM32_UART_USE_USART1 &&                                                \
    !STM32_DMA_IS_VALID_ID(STM32_UART_USART1_TX_DMA_STREAM,                 \
                           STM32_USART1_TX_DMA_MSK)
#error "invalid DMA stream associated to USART1 TX"
#endif

#if STM32_UART_USE_USART2 &&                                                \
    !STM32_DMA_IS_VALID_ID(STM32_UART_USART2_RX_DMA_STREAM,                 \
                           STM32_USART2_RX_DMA_MSK)
#error "invalid DMA stream associated to USART2 RX"
#endif

#if STM32_UART_USE_USART2 &&                                                \
    !STM32_DMA_IS_VALID_ID(STM32_UART_USART2_TX_DMA_STREAM,                 \
                           STM32_USART2_TX_DMA_MSK)
#error "invalid DMA stream associated to USART2 TX"
#endif

#if STM32_UART_USE_USART3 &&                                                \
    !STM32_DMA_IS_VALID_ID(STM32_UART_USART3_RX_DMA_STREAM,                 \
                           STM32_USART3_RX_DMA_MSK)
#error "invalid DMA stream associated to USART3 RX"
#endif

#if STM32_UART_USE_USART3 &&                                                \
    !STM32_DMA_IS_VALID_ID(STM32_UART_USART3_TX_DMA_STREAM,                 \
                           STM32_USART3_TX_DMA_MSK)
#error "invalid DMA stream associated to USART3 TX"
#endif

#if STM32_UART_USE_UART4 &&                                                 \
    !STM32_DMA_IS_VALID_ID(STM32_UART_UART4_RX_DMA_STREAM,                  \
                           STM32_UART4_RX_DMA_MSK)
#error "invalid DMA stream associated to UART4 RX"
#endif

#if STM32_UART_USE_UART4 &&                                                 \
    !STM32_DMA_IS_VALID_ID(STM32_UART_UART4_TX_DMA_STREAM,                  \
                           STM32_UART4_TX_DMA_MSK)
#error "invalid DMA stream associated to UART4 TX"
#endif

#if STM32_UART_USE_UART5 &&                                                 \
    !STM32_DMA_IS_VALID_ID(STM32_UART_UART5_RX_DMA_STREAM,                  \
                           STM32_UART5_RX_DMA_MSK)
#error "invalid DMA stream associated to UART5 RX"
#endif

#if STM32_UART_USE_UART5 &&                                                 \
    !STM32_DMA_IS_VALID_ID(STM32_UART_UART5_TX_DMA_STREAM,                  \
                           STM32_UART5_TX_DMA_MSK)
#error "invalid DMA stream associated to UART5 TX"
#endif

#if STM32_UART_USE_USART6 &&                                                \
    !STM32_DMA_IS_VALID_ID(STM32_UART_USART6_RX_DMA_STREAM,                 \
                           STM32_USART6_RX_DMA_MSK)
#error "invalid DMA stream associated to USART6 RX"
#endif

#if STM32_UART_USE_USART6 &&                                                \
    !STM32_DMA_IS_VALID_ID(STM32_UART_USART6_TX_DMA_STREAM,                 \
                           STM32_USART6_TX_DMA_MSK)
#error "invalid DMA stream associated to USART6 TX"
#endif

#if !defined(STM32_DMA_REQUIRED)
#define STM32_DMA_REQUIRED
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   UART driver condition flags type.
 */
typedef uint32_t uartflags_t;

/**
 * @brief   Structure representing an UART driver.
 */
typedef struct UARTDriver UARTDriver;

/**
 * @brief   Generic UART notification callback type.
 *
 * @param[in] uartp     pointer to the @p UARTDriver object
 */
typedef void (*uartcb_t)(UARTDriver *uartp);

/**
 * @brief   Character received UART notification callback type.
 *
 * @param[in] uartp     pointer to the @p UARTDriver object
 * @param[in] c         received character
 */
typedef void (*uartccb_t)(UARTDriver *uartp, uint16_t c);

/**
 * @brief   Receive error UART notification callback type.
 *
 * @param[in] uartp     pointer to the @p UARTDriver object
 * @param[in] e         receive error mask
 */
typedef void (*uartecb_t)(UARTDriver *uartp, uartflags_t e);

/**
 * @brief   Driver configuration structure.
 * @note    It could be empty on some architectures.
 */
typedef struct {
  /**
   * @brief End of transmission buffer callback.
   */
  uartcb_t                  txend1_cb;
  /**
   * @brief Physical end of transmission callback.
   */
  uartcb_t                  txend2_cb;
  /**
   * @brief Receive buffer filled callback.
   */
  uartcb_t                  rxend_cb;
  /**
   * @brief Character received while out if the @p UART_RECEIVE state.
   */
  uartccb_t                 rxchar_cb;
  /**
   * @brief Receive error callback.
   */
  uartecb_t                 rxerr_cb;
  /* End of the mandatory fields.*/
  /**
   * @brief Bit rate.
   */
  uint32_t                  speed;
  /**
   * @brief Initialization value for the CR1 register.
   */
  uint16_t                  cr1;
  /**
   * @brief Initialization value for the CR2 register.
   */
  uint16_t                  cr2;
  /**
   * @brief Initialization value for the CR3 register.
   */
  uint16_t                  cr3;
} UARTConfig;

/**
 * @brief   Structure representing an UART driver.
 */
struct UARTDriver {
  /**
   * @brief Driver state.
   */
  uartstate_t               state;
  /**
   * @brief Transmitter state.
   */
  uarttxstate_t             txstate;
  /**
   * @brief Receiver state.
   */
  uartrxstate_t             rxstate;
  /**
   * @brief Current configuration data.
   */
  const UARTConfig          *config;
#if defined(UART_DRIVER_EXT_FIELDS)
  UART_DRIVER_EXT_FIELDS
#endif
  /* End of the mandatory fields.*/
  /**
   * @brief Pointer to the USART registers block.
   */
  USART_TypeDef             *usart;
  /**
   * @brief DMA mode bit mask.
   */
  uint32_t                  dmamode;
  /**
   * @brief Receive DMA channel.
   */
  const stm32_dma_stream_t  *dmarx;
  /**
   * @brief Transmit DMA channel.
   */
  const stm32_dma_stream_t  *dmatx;
  /**
   * @brief Default receive buffer while into @p UART_RX_IDLE state.
   */
  volatile uint16_t         rxbuf;
};

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if STM32_UART_USE_USART1 && !defined(__DOXYGEN__)
extern UARTDriver UARTD1;
#endif

#if STM32_UART_USE_USART2 && !defined(__DOXYGEN__)
extern UARTDriver UARTD2;
#endif

#if STM32_UART_USE_USART3 && !defined(__DOXYGEN__)
extern UARTDriver UARTD3;
#endif

#if STM32_UART_USE_UART4 && !defined(__DOXYGEN__)
extern UARTDriver UARTD4;
#endif

#if STM32_UART_USE_UART5 && !defined(__DOXYGEN__)
extern UARTDriver UARTD5;
#endif

#if STM32_UART_USE_USART6 && !defined(__DOXYGEN__)
extern UARTDriver UARTD6;
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void uart_lld_init(void);
  void uart_lld_start(UARTDriver *uartp);
  void uart_lld_stop(UARTDriver *uartp);
  void uart_lld_start_send(UARTDriver *uartp, size_t n, const void *txbuf);
  size_t uart_lld_stop_send(UARTDriver *uartp);
  void uart_lld_start_receive(UARTDriver *uartp, size_t n, void *rxbuf);
  size_t uart_lld_stop_receive(UARTDriver *uartp);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_UART */

#endif /* _UART_LLD_H_ */

/** @} */
