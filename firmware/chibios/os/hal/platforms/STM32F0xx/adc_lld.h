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
 * @file    STM32F0xx/adc_lld.h
 * @brief   STM32F0xx ADC subsystem low level driver header.
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
 * @name    Sampling rates
 * @{
 */
#define ADC_SMPR_SMP_1P5        0   /**< @brief 14 cycles conversion time   */
#define ADC_SMPR_SMP_7P5        1   /**< @brief 21 cycles conversion time.  */
#define ADC_SMPR_SMP_13P5       2   /**< @brief 28 cycles conversion time.  */
#define ADC_SMPR_SMP_28P5       3   /**< @brief 41 cycles conversion time.  */
#define ADC_SMPR_SMP_41P5       4   /**< @brief 54 cycles conversion time.  */
#define ADC_SMPR_SMP_55P5       5   /**< @brief 68 cycles conversion time.  */
#define ADC_SMPR_SMP_71P5       6   /**< @brief 84 cycles conversion time.  */
#define ADC_SMPR_SMP_239P5      7   /**< @brief 252 cycles conversion time. */
/** @} */

/**
 * @name    CFGR1 register configuration helpers
 * @{
 */
#define ADC_CFGR1_RES_12BIT             (0 << 3)
#define ADC_CFGR1_RES_10BIT             (1 << 3)
#define ADC_CFGR1_RES_8BIT              (2 << 3)
#define ADC_CFGR1_RES_6BIT              (3 << 3)

#define ADC_CFGR1_EXTSEL_MASK           (15 << 6)
#define ADC_CFGR1_EXTSEL_SRC(n)         ((n) << 6)

#define ADC_CFGR1_EXTEN_MASK            (3 << 10)
#define ADC_CFGR1_EXTEN_DISABLED        (0 << 10)
#define ADC_CFGR1_EXTEN_RISING          (1 << 10)
#define ADC_CFGR1_EXTEN_FALLING         (2 << 10)
#define ADC_CFGR1_EXTEN_BOTH            (3 << 10)
/** @} */

/**
 * @name    Threashold register initializer
 * @{
 */
#define ADC_TR(low, high)               (((uint32_t)(high) << 16) |         \
                                         (uint32_t)(low))
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
 * @note    The default is @p FALSE.
 */
#if !defined(STM32_ADC_USE_ADC1) || defined(__DOXYGEN__)
#define STM32_ADC_USE_ADC1                  FALSE
#endif

/**
 * @brief   ADC1 DMA priority (0..3|lowest..highest).
 */
#if !defined(STM32_ADC_ADC1_DMA_PRIORITY) || defined(__DOXYGEN__)
#define STM32_ADC_ADC1_DMA_PRIORITY         2
#endif

/**
 * @brief   ADC interrupt priority level setting.
 */
#if !defined(STM32_ADC_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define STM32_ADC_IRQ_PRIORITY              2
#endif

/**
 * @brief   ADC1 DMA interrupt priority level setting.
 */
#if !defined(STM32_ADC_ADC1_DMA_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define STM32_ADC_ADC1_DMA_IRQ_PRIORITY     2
#endif

/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if STM32_ADC_USE_ADC1 && !STM32_HAS_ADC1
#error "ADC1 not present in the selected device"
#endif

#if !STM32_ADC_USE_ADC1
#error "ADC driver activated but no ADC peripheral assigned"
#endif

#if STM32_ADC_USE_ADC1 &&                                                   \
    !CORTEX_IS_VALID_KERNEL_PRIORITY(STM32_ADC_IRQ_PRIORITY)
#error "Invalid IRQ priority assigned to ADC1"
#endif

#if STM32_ADC_USE_ADC1 &&                                                   \
    !CORTEX_IS_VALID_KERNEL_PRIORITY(STM32_ADC_ADC1_DMA_IRQ_PRIORITY)
#error "Invalid IRQ priority assigned to ADC1 DMA"
#endif

#if STM32_ADC_USE_ADC1 &&                                                   \
    !STM32_DMA_IS_VALID_PRIORITY(STM32_ADC_ADC1_DMA_PRIORITY)
#error "Invalid DMA priority assigned to ADC1"
#endif

#if !defined(STM32_DMA_REQUIRED)
#define STM32_DMA_REQUIRED
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   ADC sample data type.
 */
typedef uint16_t adcsample_t;

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
  ADC_ERR_DMAFAILURE = 0,                   /**< DMA operations failure.    */
  ADC_ERR_OVERFLOW = 1,                     /**< ADC overflow condition.    */
  ADC_ERR_AWD = 2                           /**< Analog watchdog triggered. */
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
   * @brief   ADC CFGR1 register initialization data.
   * @note    The bits DMAEN and DMACFG are enforced internally
   *          to the driver, keep them to zero.
   * @note    The bits @p ADC_CFGR1_CONT or @p ADC_CFGR1_DISCEN must be
   *          specified in continuous more or if the buffer depth is
   *          greater than one.
   */
  uint32_t                  cfgr1;
  /**
   * @brief   ADC TR register initialization data.
   */
  uint32_t                  tr;
  /**
   * @brief   ADC SMPR register initialization data.
   */
  uint32_t                  smpr;
  /**
   * @brief   ADC CHSELR register initialization data.
   * @details The number of bits at logic level one in this register must
   *          be equal to the number in the @p num_channels field.
   */
  uint32_t                  chselr;
} ADCConversionGroup;

/**
 * @brief   Driver configuration structure.
 * @note    It could be empty on some architectures.
 */
typedef struct {
  uint32_t                  dummy;
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
  /**
   * @brief Pointer to the ADCx registers block.
   */
  ADC_TypeDef               *adc;
  /**
   * @brief Pointer to associated DMA channel.
   */
  const stm32_dma_stream_t  *dmastp;
  /**
   * @brief DMA mode bit mask.
   */
  uint32_t                  dmamode;
};

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/**
 * @brief   Changes the value of the ADC CCR register.
 * @details Use this function in order to enable or disable the internal
 *          analog sources. See the documentation in the STM32F0xx Reference
 *          Manual.
 */
#define adcSTM32SetCCR(ccr) (ADC->CCR = (ccr))

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if STM32_ADC_USE_ADC1 && !defined(__DOXYGEN__)
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
