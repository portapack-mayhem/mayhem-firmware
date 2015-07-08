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
 * @file    LPC11Uxx/spi_lld.h
 * @brief   LPC11Uxx low level SPI driver header.
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

/**
 * @brief   Hardware FIFO depth.
 */
#define LPC_SSP_FIFO_DEPTH      8

#define CR0_DSSMASK             0x0F
#define CR0_DSS4BIT             3
#define CR0_DSS5BIT             4
#define CR0_DSS6BIT             5
#define CR0_DSS7BIT             6
#define CR0_DSS8BIT             7
#define CR0_DSS9BIT             8
#define CR0_DSS10BIT            9
#define CR0_DSS11BIT            0xA
#define CR0_DSS12BIT            0xB
#define CR0_DSS13BIT            0xC
#define CR0_DSS14BIT            0xD
#define CR0_DSS15BIT            0xE
#define CR0_DSS16BIT            0xF
#define CR0_FRFSPI              0
#define CR0_FRFSSI              0x10
#define CR0_FRFMW               0x20
#define CR0_CPOL                0x40
#define CR0_CPHA                0x80
#define CR0_CLOCKRATE(n)        ((n) << 8)

#define CR1_LBM                 1
#define CR1_SSE                 2
#define CR1_MS                  4
#define CR1_SOD                 8

#define SR_TFE                  1
#define SR_TNF                  2
#define SR_RNE                  4
#define SR_RFF                  8
#define SR_BSY                  16

#define IMSC_ROR                1
#define IMSC_RT                 2
#define IMSC_RX                 4
#define IMSC_TX                 8

#define RIS_ROR                 1
#define RIS_RT                  2
#define RIS_RX                  4
#define RIS_TX                  8

#define MIS_ROR                 1
#define MIS_RT                  2
#define MIS_RX                  4
#define MIS_TX                  8

#define ICR_ROR                 1
#define ICR_RT                  2

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @brief   SPI1 driver enable switch.
 * @details If set to @p TRUE the support for device SSP0 is included.
 * @note    The default is @p TRUE.
 */
#if !defined(LPC_SPI_USE_SSP0) || defined(__DOXYGEN__)
#define LPC_SPI_USE_SSP0                    TRUE
#endif

/**
 * @brief   SPI2 driver enable switch.
 * @details If set to @p TRUE the support for device SSP1 is included.
 * @note    The default is @p TRUE.
 */
#if !defined(LPC_SPI_USE_SSP1) || defined(__DOXYGEN__)
#define LPC_SPI_USE_SSP1                    TRUE
#endif

/**
 * @brief   SSP0 PCLK divider.
 */
#if !defined(LPC_SPI_SSP0CLKDIV) || defined(__DOXYGEN__)
#define LPC_SPI_SSP0CLKDIV                  1
#endif

/**
 * @brief   SSP1 PCLK divider.
 */
#if !defined(LPC_SPI_SSP1CLKDIV) || defined(__DOXYGEN__)
#define LPC_SPI_SSP1CLKDIV                  1
#endif

/**
 * @brief   SPI0 interrupt priority level setting.
 */
#if !defined(LPC_SPI_SSP0_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define LPC_SPI_SSP0_IRQ_PRIORITY           1
#endif

/**
 * @brief   SPI1 interrupt priority level setting.
 */
#if !defined(LPC_SPI_SSP1_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define LPC_SPI_SSP1_IRQ_PRIORITY           1
#endif

/**
 * @brief   Overflow error hook.
 * @details The default action is to stop the system.
 */
#if !defined(LPC_SPI_SSP_ERROR_HOOK) || defined(__DOXYGEN__)
#define LPC_SPI_SSP_ERROR_HOOK(spip)        chSysHalt()
#endif

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if (LPC_SPI_SSP0CLKDIV < 1) || (LPC_SPI_SSP0CLKDIV > 255)
#error "invalid LPC_SPI_SSP0CLKDIV setting"
#endif

#if (LPC_SPI_SSP1CLKDIV < 1) || (LPC_SPI_SSP1CLKDIV > 255)
#error "invalid LPC_SPI_SSP1CLKDIV setting"
#endif

#if !LPC_SPI_USE_SSP0 && !LPC_SPI_USE_SSP1
#error "SPI driver activated but no SPI peripheral assigned"
#endif

/**
 * @brief   SSP0 clock.
 */
#define LPC_SPI_SSP0_PCLK                                                   \
  (LPC_MAINCLK / LPC_SPI_SSP0CLKDIV)

/**
 * @brief   SSP1 clock.
 */
#define LPC_SPI_SSP1_PCLK                                                   \
  (LPC_MAINCLK / LPC_SPI_SSP1CLKDIV)

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
  spicallback_t         end_cb;
  /* End of the mandatory fields.*/
  /**
   * @brief The chip select line port.
   */
  ioportid_t            ssport;
  /**
   * @brief The chip select line pad number.
   */
  uint16_t              sspad;
  /**
   * @brief SSP CR0 initialization data.
   */
  uint16_t              cr0;
  /**
   * @brief SSP CPSR initialization data.
   */
  uint32_t              cpsr;
} SPIConfig;

/**
 * @brief   Structure representing a SPI driver.
 */
struct SPIDriver {
  /**
   * @brief Driver state.
   */
  spistate_t            state;
  /**
   * @brief Current configuration data.
   */
  const SPIConfig       *config;
#if SPI_USE_WAIT || defined(__DOXYGEN__)
  /**
   * @brief Waiting thread.
   */
  Thread                *thread;
#endif /* SPI_USE_WAIT */
#if SPI_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
#if CH_USE_MUTEXES || defined(__DOXYGEN__)
  /**
   * @brief Mutex protecting the bus.
   */
  Mutex                 mutex;
#elif CH_USE_SEMAPHORES
  Semaphore             semaphore;
#endif
#endif /* SPI_USE_MUTUAL_EXCLUSION */
#if defined(SPI_DRIVER_EXT_FIELDS)
  SPI_DRIVER_EXT_FIELDS
#endif
  /* End of the mandatory fields.*/
  /**
   * @brief Pointer to the SSP registers block.
   */
  LPC_SSPx_Type         *ssp;
  /**
   * @brief Number of bytes yet to be received.
   */
  uint32_t              rxcnt;
  /**
   * @brief Receive pointer or @p NULL.
   */
  void                  *rxptr;
  /**
   * @brief Number of bytes yet to be transmitted.
   */
  uint32_t              txcnt;
  /**
   * @brief Transmit pointer or @p NULL.
   */
  const void            *txptr;
};

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if LPC_SPI_USE_SSP0 && !defined(__DOXYGEN__)
extern SPIDriver SPID1;
#endif

#if LPC_SPI_USE_SSP1 && !defined(__DOXYGEN__)
extern SPIDriver SPID2;
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
