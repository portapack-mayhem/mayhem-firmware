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
 * @file    STM32/SPIv2/spi_lld.h
 * @brief   STM32 SPI subsystem low level driver header.
 *
 * @addtogroup SPI
 * @{
 */

#ifndef _SPI_LLD_H_
#define _SPI_LLD_H_

#if HAL_USE_SPI || defined(__DOXYGEN__)

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
 * @brief   SPI1 driver enable switch.
 * @details If set to @p TRUE the support for SPI1 is included.
 * @note    The default is @p FALSE.
 */
#if !defined(STM32_SPI_USE_SPI1) || defined(__DOXYGEN__)
#define STM32_SPI_USE_SPI1                  FALSE
#endif

/**
 * @brief   SPI2 driver enable switch.
 * @details If set to @p TRUE the support for SPI2 is included.
 * @note    The default is @p FALSE.
 */
#if !defined(STM32_SPI_USE_SPI2) || defined(__DOXYGEN__)
#define STM32_SPI_USE_SPI2                  FALSE
#endif

/**
 * @brief   SPI3 driver enable switch.
 * @details If set to @p TRUE the support for SPI3 is included.
 * @note    The default is @p FALSE.
 */
#if !defined(STM32_SPI_USE_SPI3) || defined(__DOXYGEN__)
#define STM32_SPI_USE_SPI3                  FALSE
#endif

/**
 * @brief   SPI1 interrupt priority level setting.
 */
#if !defined(STM32_SPI_SPI1_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define STM32_SPI_SPI1_IRQ_PRIORITY         10
#endif

/**
 * @brief   SPI2 interrupt priority level setting.
 */
#if !defined(STM32_SPI_SPI2_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define STM32_SPI_SPI2_IRQ_PRIORITY         10
#endif

/**
 * @brief   SPI3 interrupt priority level setting.
 */
#if !defined(STM32_SPI_SPI3_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define STM32_SPI_SPI3_IRQ_PRIORITY         10
#endif

/**
 * @brief   SPI1 DMA priority (0..3|lowest..highest).
 * @note    The priority level is used for both the TX and RX DMA streams but
 *          because of the streams ordering the RX stream has always priority
 *          over the TX stream.
 */
#if !defined(STM32_SPI_SPI1_DMA_PRIORITY) || defined(__DOXYGEN__)
#define STM32_SPI_SPI1_DMA_PRIORITY         1
#endif

/**
 * @brief   SPI2 DMA priority (0..3|lowest..highest).
 * @note    The priority level is used for both the TX and RX DMA streams but
 *          because of the streams ordering the RX stream has always priority
 *          over the TX stream.
 */
#if !defined(STM32_SPI_SPI2_DMA_PRIORITY) || defined(__DOXYGEN__)
#define STM32_SPI_SPI2_DMA_PRIORITY         1
#endif

/**
 * @brief   SPI3 DMA priority (0..3|lowest..highest).
 * @note    The priority level is used for both the TX and RX DMA streams but
 *          because of the streams ordering the RX stream has always priority
 *          over the TX stream.
 */
#if !defined(STM32_SPI_SPI3_DMA_PRIORITY) || defined(__DOXYGEN__)
#define STM32_SPI_SPI3_DMA_PRIORITY         1
#endif

/**
 * @brief   SPI DMA error hook.
 */
#if !defined(STM32_SPI_DMA_ERROR_HOOK) || defined(__DOXYGEN__)
#define STM32_SPI_DMA_ERROR_HOOK(spip)      chSysHalt()
#endif

#if STM32_ADVANCED_DMA || defined(__DOXYGEN__)

/**
 * @brief   DMA stream used for SPI1 RX operations.
 * @note    This option is only available on platforms with enhanced DMA.
 */
#if !defined(STM32_SPI_SPI1_RX_DMA_STREAM) || defined(__DOXYGEN__)
#define STM32_SPI_SPI1_RX_DMA_STREAM        STM32_DMA_STREAM_ID(2, 0)
#endif

/**
 * @brief   DMA stream used for SPI1 TX operations.
 * @note    This option is only available on platforms with enhanced DMA.
 */
#if !defined(STM32_SPI_SPI1_TX_DMA_STREAM) || defined(__DOXYGEN__)
#define STM32_SPI_SPI1_TX_DMA_STREAM        STM32_DMA_STREAM_ID(2, 3)
#endif

/**
 * @brief   DMA stream used for SPI2 RX operations.
 * @note    This option is only available on platforms with enhanced DMA.
 */
#if !defined(STM32_SPI_SPI2_RX_DMA_STREAM) || defined(__DOXYGEN__)
#define STM32_SPI_SPI2_RX_DMA_STREAM        STM32_DMA_STREAM_ID(1, 3)
#endif

/**
 * @brief   DMA stream used for SPI2 TX operations.
 * @note    This option is only available on platforms with enhanced DMA.
 */
#if !defined(STM32_SPI_SPI2_TX_DMA_STREAM) || defined(__DOXYGEN__)
#define STM32_SPI_SPI2_TX_DMA_STREAM        STM32_DMA_STREAM_ID(1, 4)
#endif

/**
 * @brief   DMA stream used for SPI3 RX operations.
 * @note    This option is only available on platforms with enhanced DMA.
 */
#if !defined(STM32_SPI_SPI3_RX_DMA_STREAM) || defined(__DOXYGEN__)
#define STM32_SPI_SPI3_RX_DMA_STREAM        STM32_DMA_STREAM_ID(1, 0)
#endif

/**
 * @brief   DMA stream used for SPI3 TX operations.
 * @note    This option is only available on platforms with enhanced DMA.
 */
#if !defined(STM32_SPI_SPI3_TX_DMA_STREAM) || defined(__DOXYGEN__)
#define STM32_SPI_SPI3_TX_DMA_STREAM        STM32_DMA_STREAM_ID(1, 7)
#endif

#else /* !STM32_ADVANCED_DMA */

#if defined(STM32F0XX)
/* Fixed values for STM32F0xx devices.*/
#define STM32_SPI_SPI1_RX_DMA_STREAM        STM32_DMA_STREAM_ID(1, 2)
#define STM32_SPI_SPI1_TX_DMA_STREAM        STM32_DMA_STREAM_ID(1, 3)
#define STM32_SPI_SPI2_RX_DMA_STREAM        STM32_DMA_STREAM_ID(1, 4)
#define STM32_SPI_SPI2_TX_DMA_STREAM        STM32_DMA_STREAM_ID(1, 5)
#endif /* defined(STM32F0XX) */

#if defined(STM32F30X) || defined(STM32F37X)
/* Fixed values for STM32F3xx devices.*/
#define STM32_SPI_SPI1_RX_DMA_STREAM        STM32_DMA_STREAM_ID(1, 2)
#define STM32_SPI_SPI1_TX_DMA_STREAM        STM32_DMA_STREAM_ID(1, 3)
#define STM32_SPI_SPI2_RX_DMA_STREAM        STM32_DMA_STREAM_ID(1, 4)
#define STM32_SPI_SPI2_TX_DMA_STREAM        STM32_DMA_STREAM_ID(1, 5)
#define STM32_SPI_SPI3_RX_DMA_STREAM        STM32_DMA_STREAM_ID(2, 1)
#define STM32_SPI_SPI3_TX_DMA_STREAM        STM32_DMA_STREAM_ID(2, 2)
#endif /* defined(STM32F30X) */

#endif /* !STM32_ADVANCED_DMA*/
/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if STM32_SPI_USE_SPI1 && !STM32_HAS_SPI1
#error "SPI1 not present in the selected device"
#endif

#if STM32_SPI_USE_SPI2 && !STM32_HAS_SPI2
#error "SPI2 not present in the selected device"
#endif

#if STM32_SPI_USE_SPI3 && !STM32_HAS_SPI3
#error "SPI3 not present in the selected device"
#endif

#if !STM32_SPI_USE_SPI1 && !STM32_SPI_USE_SPI2 && !STM32_SPI_USE_SPI3
#error "SPI driver activated but no SPI peripheral assigned"
#endif

#if STM32_SPI_USE_SPI1 &&                                                   \
    !CORTEX_IS_VALID_KERNEL_PRIORITY(STM32_SPI_SPI1_IRQ_PRIORITY)
#error "Invalid IRQ priority assigned to SPI1"
#endif

#if STM32_SPI_USE_SPI2 &&                                                   \
    !CORTEX_IS_VALID_KERNEL_PRIORITY(STM32_SPI_SPI2_IRQ_PRIORITY)
#error "Invalid IRQ priority assigned to SPI2"
#endif

#if STM32_SPI_USE_SPI3 &&                                                   \
    !CORTEX_IS_VALID_KERNEL_PRIORITY(STM32_SPI_SPI3_IRQ_PRIORITY)
#error "Invalid IRQ priority assigned to SPI3"
#endif

#if STM32_SPI_USE_SPI1 &&                                                   \
    !STM32_DMA_IS_VALID_PRIORITY(STM32_SPI_SPI1_DMA_PRIORITY)
#error "Invalid DMA priority assigned to SPI1"
#endif

#if STM32_SPI_USE_SPI2 &&                                                   \
    !STM32_DMA_IS_VALID_PRIORITY(STM32_SPI_SPI2_DMA_PRIORITY)
#error "Invalid DMA priority assigned to SPI2"
#endif

#if STM32_SPI_USE_SPI3 &&                                                   \
    !STM32_DMA_IS_VALID_PRIORITY(STM32_SPI_SPI3_DMA_PRIORITY)
#error "Invalid DMA priority assigned to SPI3"
#endif

#if STM32_SPI_USE_SPI1 &&                                                   \
    !STM32_DMA_IS_VALID_ID(STM32_SPI_SPI1_RX_DMA_STREAM, STM32_SPI1_RX_DMA_MSK)
#error "invalid DMA stream associated to SPI1 RX"
#endif

#if STM32_SPI_USE_SPI1 &&                                                   \
    !STM32_DMA_IS_VALID_ID(STM32_SPI_SPI1_TX_DMA_STREAM, STM32_SPI1_TX_DMA_MSK)
#error "invalid DMA stream associated to SPI1 TX"
#endif

#if STM32_SPI_USE_SPI2 &&                                                   \
    !STM32_DMA_IS_VALID_ID(STM32_SPI_SPI2_RX_DMA_STREAM, STM32_SPI2_RX_DMA_MSK)
#error "invalid DMA stream associated to SPI2 RX"
#endif

#if STM32_SPI_USE_SPI2 &&                                                   \
    !STM32_DMA_IS_VALID_ID(STM32_SPI_SPI2_TX_DMA_STREAM, STM32_SPI2_TX_DMA_MSK)
#error "invalid DMA stream associated to SPI2 TX"
#endif

#if STM32_SPI_USE_SPI3 &&                                                   \
    !STM32_DMA_IS_VALID_ID(STM32_SPI_SPI3_RX_DMA_STREAM, STM32_SPI3_RX_DMA_MSK)
#error "invalid DMA stream associated to SPI3 RX"
#endif

#if STM32_SPI_USE_SPI3 &&                                                   \
    !STM32_DMA_IS_VALID_ID(STM32_SPI_SPI3_TX_DMA_STREAM, STM32_SPI3_TX_DMA_MSK)
#error "invalid DMA stream associated to SPI3 TX"
#endif

#if !defined(STM32_DMA_REQUIRED)
#define STM32_DMA_REQUIRED
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Type of a structure representing an SPI driver.
 */
typedef struct SPIDriver SPIDriver;

/**
 * @brief   SPI notification callback type.
 *
 * @param[in] spip      pointer to the @p SPIDriver object triggering the
 *                      callback
 */
typedef void (*spicallback_t)(SPIDriver *spip);

/**
 * @brief   Driver configuration structure.
 */
typedef struct {
  /**
   * @brief Operation complete callback or @p NULL.
   */
  spicallback_t             end_cb;
  /* End of the mandatory fields.*/
  /**
   * @brief The chip select line port.
   */
  ioportid_t                ssport;
  /**
   * @brief The chip select line pad number.
   */
  uint16_t                  sspad;
  /**
   * @brief SPI CR1 register initialization data.
   */
  uint16_t                  cr1;
  /**
   * @brief SPI CR2 register initialization data.
   */
  uint16_t                  cr2;
} SPIConfig;

/**
 * @brief   Structure representing a SPI driver.
 */
struct SPIDriver {
  /**
   * @brief Driver state.
   */
  spistate_t                state;
  /**
   * @brief Current configuration data.
   */
  const SPIConfig           *config;
#if SPI_USE_WAIT || defined(__DOXYGEN__)
  /**
   * @brief Waiting thread.
   */
  Thread                    *thread;
#endif /* SPI_USE_WAIT */
#if SPI_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
#if CH_USE_MUTEXES || defined(__DOXYGEN__)
  /**
   * @brief Mutex protecting the bus.
   */
  Mutex                     mutex;
#elif CH_USE_SEMAPHORES
  Semaphore                 semaphore;
#endif
#endif /* SPI_USE_MUTUAL_EXCLUSION */
#if defined(SPI_DRIVER_EXT_FIELDS)
  SPI_DRIVER_EXT_FIELDS
#endif
  /* End of the mandatory fields.*/
  /**
   * @brief Pointer to the SPIx registers block.
   */
  SPI_TypeDef               *spi;
  /**
   * @brief Receive DMA stream.
   */
  const stm32_dma_stream_t  *dmarx;
  /**
   * @brief Transmit DMA stream.
   */
  const stm32_dma_stream_t  *dmatx;
  /**
   * @brief RX DMA mode bit mask.
   */
  uint32_t                  rxdmamode;
  /**
   * @brief TX DMA mode bit mask.
   */
  uint32_t                  txdmamode;
};

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if STM32_SPI_USE_SPI1 && !defined(__DOXYGEN__)
extern SPIDriver SPID1;
#endif

#if STM32_SPI_USE_SPI2 && !defined(__DOXYGEN__)
extern SPIDriver SPID2;
#endif

#if STM32_SPI_USE_SPI3 && !defined(__DOXYGEN__)
extern SPIDriver SPID3;
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void spi_lld_init(void);
  void spi_lld_start(SPIDriver *spip);
  void spi_lld_stop(SPIDriver *spip);
  void spi_lld_select(SPIDriver *spip);
  void spi_lld_unselect(SPIDriver *spip);
  void spi_lld_ignore(SPIDriver *spip, size_t n);
  void spi_lld_exchange(SPIDriver *spip, size_t n,
                        const void *txbuf, void *rxbuf);
  void spi_lld_send(SPIDriver *spip, size_t n, const void *txbuf);
  void spi_lld_receive(SPIDriver *spip, size_t n, void *rxbuf);
  uint16_t spi_lld_polled_exchange(SPIDriver *spip, uint16_t frame);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_SPI */

#endif /* _SPI_LLD_H_ */

/** @} */
