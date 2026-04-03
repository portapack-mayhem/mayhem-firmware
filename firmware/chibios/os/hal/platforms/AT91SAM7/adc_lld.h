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
/*
   This file has been contributed by:
		Andrew Hannam aka inmarket.
*/
/**
 * @file    AT91SAM7/adc_lld.h
 * @brief   AT91SAM7 ADC subsystem low level driver header.
 *
 * @addtogroup ADC
 * @{
 */

#ifndef _ADC_LLD_H_
#define _ADC_LLD_H_

#if HAL_USE_ADC || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @name    Trigger Sources
 * @{
 */
#define ADC_TRIGGER_SOFTWARE	0x8000   				/**< @brief Software Triggering - Can be combined with another value */
#define ADC_TRIGGER_TIMER		0x0001					/**< @brief TIO Timer Counter Channel  */
#define ADC_TRIGGER_EXTERNAL	0x0002					/**< @brief External Trigger  */
/** @} */

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    Configuration options
 * @{
 */
/**
 * @brief   ADC1 driver enable switch.
 * @details If set to @p TRUE the support for ADC1 is included.
 * @note    The default is @p TRUE.
 */
#if !defined(ADC_USE_ADC1) || defined(__DOXYGEN__)
#define ADC_USE_ADC1                  TRUE
#endif

/**
 * @brief   ADC1 Timer to use when a periodic conversion is requested.
 * @details Should be set to 0..2
 * @note    The default is 0
 */
#if !defined(AT91_ADC1_TIMER) || defined(__DOXYGEN__)
#define AT91_ADC1_TIMER		0
#endif

/**
 * @brief   ADC1 Resolution.
 * @details Either 8 or 10 bits
 * @note    The default is 10 bits.
 */
#if !defined(AT91_ADC1_RESOLUTION) || defined(__DOXYGEN__)
#define AT91_ADC1_RESOLUTION	10
#endif

/**
 * @brief	ADC1 Clock
 * @details	Maximum is 5MHz for 10bit or 8MHz for 8bit
 * @note	The default is calculated from AT91_ADC1_RESOLUTION to give the fastest possible ADCClock
 */
#if !defined(AT91_ADC1_CLOCK) || defined(__DOXYGEN__)
	#if AT91_ADC1_RESOLUTION == 8
		#define AT91_ADC1_CLOCK	8000000
	#else
		#define AT91_ADC1_CLOCK	5000000
	#endif
#endif

/**
 * @brief	ADC1 Sample and Hold Time
 * @details	SHTM = RoundUp(ADCClock * SampleHoldTime). Range = RoundUp(ADCClock / 1,666,666) to 15
 * @note	Default corresponds to the minimum sample and hold time (600nS from the datasheet)
 * @note	Increasing the Sample Hold Time increases the ADC input impedance
 */
#if !defined(AT91_ADC1_SHTM) || defined(__DOXYGEN__)
	#define AT91_ADC1_SHTM	0
#endif
#if AT91_ADC1_SHTM < ((AT91_ADC1_CLOCK+1666665)/1666666)
	#undef AT91_ADC1_SHTM
	#define AT91_ADC1_SHTM	((AT91_ADC1_CLOCK+1666665)/1666666)
#endif
#if AT91_ADC1_SHTM > 15
	#undef AT91_ADC1_SHTM
	#define AT91_ADC1_SHTM	15
#endif

/**
 * @brief   ADC interrupt priority level setting.
 */
#if !defined(AT91_ADC_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define AT91_ADC_IRQ_PRIORITY              (AT91C_AIC_PRIOR_HIGHEST - 2)
#endif

/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if !defined(AT91_DMA_REQUIRED)
#define AT91_DMA_REQUIRED
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   ADC sample data type.
 */
#if AT91_ADC1_RESOLUTION == AT91C_ADC_LOWRES_8_BIT
	typedef uint8_t adcsample_t;
#else
	typedef uint16_t adcsample_t;
#endif

/**
 * @brief   Channels number in a conversion group.
 */
typedef uint16_t adc_channels_num_t;

/**
 * @brief   Possible ADC failure causes.
 * @note    Error codes are architecture dependent and should not relied
 *          upon.
 */
typedef enum {
  ADC_ERR_OVERFLOW = 0,                     /**< ADC overflow condition. Something is not working fast enough. */
} adcerror_t;

/**
 * @brief   Type of a structure representing an ADC driver.
 */
typedef struct ADCDriver ADCDriver;

/**
 * @brief   ADC notification callback type.
 *
 * @param[in] adcp      pointer to the @p ADCDriver object triggering the
 *                      callback
 * @param[in] buffer    pointer to the most recent samples data
 * @param[in] n         number of buffer rows available starting from @p buffer
 */
typedef void (*adccallback_t)(ADCDriver *adcp, adcsample_t *buffer, size_t n);

/**
 * @brief   ADC error callback type.
 *
 * @param[in] adcp      pointer to the @p ADCDriver object triggering the
 *                      callback
 * @param[in] err       ADC error code
 */
typedef void (*adcerrorcallback_t)(ADCDriver *adcp, adcerror_t err);

/**
 * @brief   Conversion group configuration structure.
 * @details This implementation-dependent structure describes a conversion
 *          operation.
 * @note    The use of this configuration structure requires knowledge of
 *          STM32 ADC cell registers interface, please refer to the STM32
 *          reference manual for details.
 */
typedef struct {
  /**
   * @brief   Enables the circular buffer mode for the group.
   */
  bool_t                    circular;
  /**
   * @brief   Number of the analog channels belonging to the conversion group.
   */
  adc_channels_num_t        num_channels;
  /**
   * @brief   Callback function associated to the group or @p NULL.
   */
  adccallback_t             end_cb;
  /**
   * @brief   Error callback or @p NULL.
   */
  adcerrorcallback_t        error_cb;
  /* End of the mandatory fields.*/
  /**
   * @brief   Select the ADC Channels to read.
   * @details The number of bits at logic level one in this register must
   *          be equal to the number in the @p num_channels field.
   */
  uint16_t                  channelselects;
  /**
   * @brief   Select how to trigger the conversion.
   */
  uint16_t                  trigger;
  /**
   * @brief   When in ADC_TRIGGER_TIMER trigger mode - what frequency?
   */
  uint32_t                  frequency;
} ADCConversionGroup;

/**
 * @brief   Driver configuration structure.
 * @note    It could be empty on some architectures.
 */
typedef struct {
} ADCConfig;

/**
 * @brief   Structure representing an ADC driver.
 */
struct ADCDriver {
  /**
   * @brief Driver state.
   */
  adcstate_t                state;
  /**
   * @brief Current configuration data.
   */
  const ADCConfig           *config;
  /**
   * @brief Current samples buffer pointer or @p NULL.
   */
  adcsample_t               *samples;
  /**
   * @brief Current samples buffer depth or @p 0.
   */
  size_t                    depth;
  /**
   * @brief Current conversion group pointer or @p NULL.
   */
  const ADCConversionGroup  *grpp;
#if ADC_USE_WAIT || defined(__DOXYGEN__)
  /**
   * @brief Waiting thread.
   */
  Thread                    *thread;
#endif
#if ADC_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
#if CH_USE_MUTEXES || defined(__DOXYGEN__)
  /**
   * @brief Mutex protecting the peripheral.
   */
  Mutex                     mutex;
#elif CH_USE_SEMAPHORES
  Semaphore                 semaphore;
#endif
#endif /* ADC_USE_MUTUAL_EXCLUSION */
#if defined(ADC_DRIVER_EXT_FIELDS)
  ADC_DRIVER_EXT_FIELDS
#endif
  /* End of the mandatory fields.*/
};

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if ADC_USE_ADC1 && !defined(__DOXYGEN__)
extern ADCDriver ADCD1;
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void adc_lld_init(void);
  void adc_lld_start(ADCDriver *adcp);
  void adc_lld_stop(ADCDriver *adcp);
  void adc_lld_start_conversion(ADCDriver *adcp);
  void adc_lld_stop_conversion(ADCDriver *adcp);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_ADC */

#endif /* _ADC_LLD_H_ */

/** @} */
