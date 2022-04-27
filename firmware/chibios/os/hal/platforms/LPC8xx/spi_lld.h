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
 * @file    LPC8xx/spi_lld.h
 * @brief   LPC8xx low level SPI driver header.
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

#define SPI_CFG_ENABLE               (1<<0)
#define SPI_CFG_MASTER               (1<<2)
#define SPI_CFG_LSBF                 (1<<3)
#define SPI_CFG_CHPA                 (1<<4)
#define SPI_CFG_CPOL                 (1<<5)
#define SPI_CFG_LOOP                 (1<<7)
#define SPI_CFG_SPOL                 (1<<8)

#define SPI_DLY_PRE(n)               (((n)&0x0f)<< 0)
#define SPI_DLY_POST(n)              (((n)&0x0f)<< 4)
#define SPI_DLY_FRAME(n)             (((n)&0x0f)<< 8)
#define SPI_DLY_TFER(n)              (((n)&0x0f)<<12)

#define SPI_STAT_RXRDY                (1<<0)
#define SPI_STAT_TXRDY                (1<<1)
#define SPI_STAT_RXOV                 (1<<2)
#define SPI_STAT_TXUR                 (1<<3)
#define SPI_STAT_SSA                  (1<<4)
#define SPI_STAT_SSD                  (1<<5)
#define SPI_STAT_STALL                (1<<6)
#define SPI_STAT_EOT                  (1<<7)
#define SPI_STAT_IDLE                 (1<<8)

#define SPI_TXCTRL_TXSSELN          (1<<16)
#define SPI_TXCTRL_EOT              (1<<20)
#define SPI_TXCTRL_EOF              (1<<21)
#define SPI_TXCTRL_RXIGNORE         (1<<22)
#define SPI_TXCTRL_FLEN(n)          (((n)-1)<<24)
#define SPI_TXCTRL_FLEN_MASK        (0x0f<<24)



/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @brief   SPI1 driver enable switch.
 * @details If set to @p TRUE the support for device SPI0 is included.
 * @note    The default is @p TRUE.
 */
#if !defined(LPC8xx_SPI_USE_SPI0) || defined(__DOXYGEN__)
#define LPC8xx_SPI_USE_SPI0                TRUE
#endif

/**
 * @brief   SPI2 driver enable switch.
 * @details If set to @p TRUE the support for device SPI1 is included.
 * @note    The default is @p TRUE.
 */
#if !defined(LPC8xx_SPI_USE_SPI1) || defined(__DOXYGEN__)
#define LPC8xx_SPI_USE_SPI1                FALSE
#endif

/**
 * @brief   SPI0 PCLK divider.
 */
#if !defined(LPC8xx_SPI_SPI0CLKDIV) || defined(__DOXYGEN__)
#define LPC8xx_SPI_SPI0CLKDIV              1
#endif

/**
 * @brief   SPI1 PCLK divider.
 */
#if !defined(LPC8xx_SPI_SPI1CLKDIV) || defined(__DOXYGEN__)
#define LPC8xx_SPI_SPI1CLKDIV              1
#endif

/**
 * @brief   SPI0 interrupt priority level setting.
 */
#if !defined(LPC8xx_SPI_SPI0_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define LPC8xx_SPI_SPI0_IRQ_PRIORITY       1
#endif

/**
 * @brief   SPI1 interrupt priority level setting.
 */
#if !defined(LPC8xx_SPI_SPI1_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define LPC8xx_SPI_SPI1_IRQ_PRIORITY       1
#endif

/**
 * @brief   Overflow error hook.
 * @details The default action is to stop the system.
 */
#if !defined(LPC8xx_SPI_ERROR_HOOK) || defined(__DOXYGEN__)
#define LPC8xx_SPI_ERROR_HOOK(spip)    chSysHalt()
#endif

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if (LPC8xx_SPI_SPI0CLKDIV < 1) || (LPC8xx_SPI_SPI0CLKDIV > 255)
#error "invalid LPC8xx_SPI_SSP0CLKDIV setting"
#endif

#if (LPC8xx_SPI_SPI1CLKDIV < 1) || (LPC8xx_SPI_SPI1CLKDIV > 255)
#error "invalid LPC8xx_SPI_SSP1CLKDIV setting"
#endif

#if !LPC8xx_SPI_USE_SPI0 && !LPC8xx_SPI_USE_SPI1
#error "SPI driver activated but no SPI peripheral assigned"
#endif

/**
 * @brief   SPI0 clock.
 */
#define LPC8xx_SPI_SPI0_PCLK                                               \
  (LPC8xx_SYSCLK / LPC8xx_SPI_SPI0CLKDIV)

/**
 * @brief   SPI1 clock.
 */
#define LPC8xx_SPI_SPI1_PCLK                                               \
  (LPC8xx_SYSCLK / LPC8xx_SPI_SPI1CLKDIV)

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
   * @brief SPI CFG initialization data.
   */
  uint16_t              cfg;
  /**
   * @brief SPI DLY initialization data.
   */
  uint16_t              dly;
  /**
   * @brief SPI TXCTRL initialization data.
   */
  uint32_t              txctrl;
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
   * @brief Pointer to the SPI registers block.
   */
  LPC_SPI_TypeDef       *spi;
  /**
   * @brief Number of words yet to be received.
   */
  uint32_t              rxcnt;
  /**
   * @brief Receive pointer or @p NULL.
   */
  void                  *rxptr;
  /**
   * @brief Number of words yet to be transmitted.
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

#if LPC8xx_SPI_USE_SPI0 && !defined(__DOXYGEN__)
extern SPIDriver SPID1;
#endif

#if LPC8xx_SPI_USE_SPI1 && !defined(__DOXYGEN__)
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
