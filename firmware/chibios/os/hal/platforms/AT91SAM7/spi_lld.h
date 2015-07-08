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
 * @file    AT91SAM7/spi_lld.h
 * @brief   AT91SAM7 low level SPI driver header.
 *
 * @addtogroup SPI
 * @{
 */

#ifndef _SPI_LLD_H_
#define _SPI_LLD_H_

#if HAL_USE_SPI || defined(__DOXYGEN__)

/*===========================================================================*/
/* Device compatibility..                                                    */
/*===========================================================================*/

#if SAM7_PLATFORM == SAM7S512 || SAM7_PLATFORM == SAM7S256 || SAM7_PLATFORM == SAM7S128 || SAM7_PLATFORM == SAM7S64
	#define SPI0_MISO                       AT91C_PA12_MISO
	#define SPI0_MOSI                       AT91C_PA13_MOSI
	#define SPI0_SCK                        AT91C_PA14_SPCK
	#define SPI0_CS0						11					// PA11
#elif SAM7_PLATFORM == SAM7X512 || SAM7_PLATFORM == SAM7X256 || SAM7_PLATFORM == SAM7X128
	#define SPI0_MISO                       AT91C_PA16_SPI0_MISO
	#define SPI0_MOSI                       AT91C_PA17_SPI0_MOSI
	#define SPI0_SCK                        AT91C_PA18_SPI0_SPCK
	#define SPI0_CS0						12					// PA12
	#define SPI1_MISO                       AT91C_PA24_SPI1_MISO
	#define SPI1_MOSI                       AT91C_PA23_SPI1_MOSI
	#define SPI1_SCK                        AT91C_PA22_SPI1_SPCK
	#define SPI1_CS0						21					// PA21
#elif SAM7_PLATFORM == SAM7A3
	#define SPI0_MISO                       AT91C_PA15_SPI0_MISO
	#define SPI0_MOSI                       AT91C_PA16_SPI0_MOSI
	#define SPI0_SCK                        AT91C_PA17_SPI0_SPCK
	#define SPI0_CS0						11					// PA11
	#define SPI1_MISO                       AT91C_PA8_SPI1_MISO
	#define SPI1_MOSI                       AT91C_PA9_SPI1_MOSI
	#define SPI1_SCK                        AT91C_PA10_SPI1_SPCK
	#define SPI1_CS0						4					// PA4
#else
#error "SAM7 platform not supported"
#endif

#if defined (AT91C_BASE_SPI)
#define AT91C_BASE_SPI0                 AT91C_BASE_SPI
#define AT91C_ID_SPI0                   AT91C_ID_SPI
#endif

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @brief   SPID1 enable switch (SPI0 device).
 * @details If set to @p TRUE the support for SPI0 is included.
 * @note    The default is @p TRUE.
 */
#if !defined(AT91SAM7_SPI_USE_SPI0) || defined(__DOXYGEN__)
#define AT91SAM7_SPI_USE_SPI0           TRUE
#endif

/**
 * @brief   SPID2 enable switch (SPI1 device).
 * @details If set to @p TRUE the support for SPI1 is included.
 * @note    The default is @p TRUE.
 */
#if !defined(AT91SAM7_SPI_USE_SPI1) || defined(__DOXYGEN__)
#define AT91SAM7_SPI_USE_SPI1           TRUE
#endif

/**
 * @brief   SPI0 device interrupt priority level setting.
 */
#if !defined(AT91SAM7_SPI0_PRIORITY) || defined(__DOXYGEN__)
#define AT91SAM7_SPI0_PRIORITY          (AT91C_AIC_PRIOR_HIGHEST - 1)
#endif

/**
 * @brief   SPI1 device interrupt priority level setting.
 */
#if !defined(AT91SAM7_SPI1_PRIORITY) || defined(__DOXYGEN__)
#define AT91SAM7_SPI1_PRIORITY          (AT91C_AIC_PRIOR_HIGHEST - 1)
#endif

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if defined (AT91C_BASE_SPI) && AT91SAM7_SPI_USE_SPI1
#error "SPI1 not present"
#endif

#if !AT91SAM7_SPI_USE_SPI0 && !AT91SAM7_SPI_USE_SPI1
#error "SPI driver activated but no SPI peripheral assigned"
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
   * @brief SPI Chip Select Register initialization data.
   */
  uint32_t              csr;
} SPIConfig;

/**
 * @brief Structure representing a SPI driver.
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
   * @brief Pointer to the SPIx registers block.
   */
  AT91PS_SPI            spi;
};

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if AT91SAM7_SPI_USE_SPI0 && !defined(__DOXYGEN__)
extern SPIDriver SPID1;
#endif

#if AT91SAM7_SPI_USE_SPI1 && !defined(__DOXYGEN__)
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
