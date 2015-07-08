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
 * @file    STM32F1xx/adc_lld.h
 * @brief   STM32F1xx ADC subsystem low level driver header.
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
 * @name    Triggers selection
 * @{
 */
#define ADC_CR2_EXTSEL_SRC(n)   ((n) << 17) /**< @brief Trigger source.     */
#define ADC_CR2_EXTSEL_SWSTART  (7 << 17)   /**< @brief Software trigger.   */
/** @} */

/**
 * @name    Available analog channels
 * @{
 */
#define ADC_CHANNEL_IN0         0   /**< @brief External analog input 0.    */
#define ADC_CHANNEL_IN1         1   /**< @brief External analog input 1.    */
#define ADC_CHANNEL_IN2         2   /**< @brief External analog input 2.    */
#define ADC_CHANNEL_IN3         3   /**< @brief External analog input 3.    */
#define ADC_CHANNEL_IN4         4   /**< @brief External analog input 4.    */
#define ADC_CHANNEL_IN5         5   /**< @brief External analog input 5.    */
#define ADC_CHANNEL_IN6         6   /**< @brief External analog input 6.    */
#define ADC_CHANNEL_IN7         7   /**< @brief External analog input 7.    */
#define ADC_CHANNEL_IN8         8   /**< @brief External analog input 8.    */
#define ADC_CHANNEL_IN9         9   /**< @brief External analog input 9.    */
#define ADC_CHANNEL_IN10        10  /**< @brief External analog input 10.   */
#define ADC_CHANNEL_IN11        11  /**< @brief External analog input 11.   */
#define ADC_CHANNEL_IN12        12  /**< @brief External analog input 12.   */
#define ADC_CHANNEL_IN13        13  /**< @brief External analog input 13.   */
#define ADC_CHANNEL_IN14        14  /**< @brief External analog input 14.   */
#define ADC_CHANNEL_IN15        15  /**< @brief External analog input 15.   */
#define ADC_CHANNEL_SENSOR      16  /**< @brief Internal temperature sensor.*/
#define ADC_CHANNEL_VREFINT     17  /**< @brief Internal reference.         */
/** @} */

/**
 * @name    Sampling rates
 * @{
 */
#define ADC_SAMPLE_1P5          0   /**< @brief 1.5 cycles sampling time.   */
#define ADC_SAMPLE_7P5          1   /**< @brief 7.5 cycles sampling time.   */
#define ADC_SAMPLE_13P5         2   /**< @brief 13.5 cycles sampling time.  */
#define ADC_SAMPLE_28P5         3   /**< @brief 28.5 cycles sampling time.  */
#define ADC_SAMPLE_41P5         4   /**< @brief 41.5 cycles sampling time.  */
#define ADC_SAMPLE_55P5         5   /**< @brief 55.5 cycles sampling time.  */
#define ADC_SAMPLE_71P5         6   /**< @brief 71.5 cycles sampling time.  */
#define ADC_SAMPLE_239P5        7   /**< @brief 239.5 cycles sampling time. */
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
 * @brief   ADC1 interrupt priority level setting.
 */
#if !defined(STM32_ADC_ADC1_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define STM32_ADC_ADC1_IRQ_PRIORITY         5
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
  ADC_ERR_DMAFAILURE = 0                    /**< DMA operations failure.    */
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
   * @brief   ADC CR1 register initialization data.
   * @note    All the required bits must be defined into this field except
   *          @p ADC_CR1_SCAN that is enforced inside the driver.
   */
  uint32_t                  cr1;
  /**
   * @brief   ADC CR2 register initialization data.
   * @note    All the required bits must be defined into this field except
   *          @p ADC_CR2_DMA, @p ADC_CR2_CONT and @p ADC_CR2_ADON that are
   *          enforced inside the driver.
   */
  uint32_t                  cr2;
  /**
   * @brief   ADC SMPR1 register initialization data.
   * @details In this field must be specified the sample times for channels
   *          10...17.
   */
  uint32_t                  smpr1;
  /**
   * @brief   ADC SMPR2 register initialization data.
   * @details In this field must be specified the sample times for channels
   *          0...9.
   */
  uint32_t                  smpr2;
  /**
   * @brief   ADC SQR1 register initialization data.
   * @details Conversion group sequence 13...16 + sequence length.
   */
  uint32_t                  sqr1;
  /**
   * @brief   ADC SQR2 register initialization data.
   * @details Conversion group sequence 7...12.
   */
  uint32_t                  sqr2;
  /**
   * @brief   ADC SQR3 register initialization data.
   * @details Conversion group sequence 1...6.
   */
  uint32_t                  sqr3;
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
 * @name    Sequences building helper macros
 * @{
 */
/**
 * @brief   Number of channels in a conversion sequence.
 */
#define ADC_SQR1_NUM_CH(n)      (((n) - 1) << 20)

#define ADC_SQR3_SQ1_N(n)       ((n) << 0)  /**< @brief 1st channel in seq. */
#define ADC_SQR3_SQ2_N(n)       ((n) << 5)  /**< @brief 2nd channel in seq. */
#define ADC_SQR3_SQ3_N(n)       ((n) << 10) /**< @brief 3rd channel in seq. */
#define ADC_SQR3_SQ4_N(n)       ((n) << 15) /**< @brief 4th channel in seq. */
#define ADC_SQR3_SQ5_N(n)       ((n) << 20) /**< @brief 5th channel in seq. */
#define ADC_SQR3_SQ6_N(n)       ((n) << 25) /**< @brief 6th channel in seq. */

#define ADC_SQR2_SQ7_N(n)       ((n) << 0)  /**< @brief 7th channel in seq. */
#define ADC_SQR2_SQ8_N(n)       ((n) << 5)  /**< @brief 8th channel in seq. */
#define ADC_SQR2_SQ9_N(n)       ((n) << 10) /**< @brief 9th channel in seq. */
#define ADC_SQR2_SQ10_N(n)      ((n) << 15) /**< @brief 10th channel in seq.*/
#define ADC_SQR2_SQ11_N(n)      ((n) << 20) /**< @brief 11th channel in seq.*/
#define ADC_SQR2_SQ12_N(n)      ((n) << 25) /**< @brief 12th channel in seq.*/

#define ADC_SQR1_SQ13_N(n)      ((n) << 0)  /**< @brief 13th channel in seq.*/
#define ADC_SQR1_SQ14_N(n)      ((n) << 5)  /**< @brief 14th channel in seq.*/
#define ADC_SQR1_SQ15_N(n)      ((n) << 10) /**< @brief 15th channel in seq.*/
#define ADC_SQR1_SQ16_N(n)      ((n) << 15) /**< @brief 16th channel in seq.*/
/** @} */

/**
 * @name    Sampling rate settings helper macros
 * @{
 */
#define ADC_SMPR2_SMP_AN0(n)    ((n) << 0)  /**< @brief AN0 sampling time.  */
#define ADC_SMPR2_SMP_AN1(n)    ((n) << 3)  /**< @brief AN1 sampling time.  */
#define ADC_SMPR2_SMP_AN2(n)    ((n) << 6)  /**< @brief AN2 sampling time.  */
#define ADC_SMPR2_SMP_AN3(n)    ((n) << 9)  /**< @brief AN3 sampling time.  */
#define ADC_SMPR2_SMP_AN4(n)    ((n) << 12) /**< @brief AN4 sampling time.  */
#define ADC_SMPR2_SMP_AN5(n)    ((n) << 15) /**< @brief AN5 sampling time.  */
#define ADC_SMPR2_SMP_AN6(n)    ((n) << 18) /**< @brief AN6 sampling time.  */
#define ADC_SMPR2_SMP_AN7(n)    ((n) << 21) /**< @brief AN7 sampling time.  */
#define ADC_SMPR2_SMP_AN8(n)    ((n) << 24) /**< @brief AN8 sampling time.  */
#define ADC_SMPR2_SMP_AN9(n)    ((n) << 27) /**< @brief AN9 sampling time.  */

#define ADC_SMPR1_SMP_AN10(n)   ((n) << 0)  /**< @brief AN10 sampling time. */
#define ADC_SMPR1_SMP_AN11(n)   ((n) << 3)  /**< @brief AN11 sampling time. */
#define ADC_SMPR1_SMP_AN12(n)   ((n) << 6)  /**< @brief AN12 sampling time. */
#define ADC_SMPR1_SMP_AN13(n)   ((n) << 9)  /**< @brief AN13 sampling time. */
#define ADC_SMPR1_SMP_AN14(n)   ((n) << 12) /**< @brief AN14 sampling time. */
#define ADC_SMPR1_SMP_AN15(n)   ((n) << 15) /**< @brief AN15 sampling time. */
#define ADC_SMPR1_SMP_SENSOR(n) ((n) << 18) /**< @brief Temperature Sensor
                                                 sampling time.             */
#define ADC_SMPR1_SMP_VREF(n)   ((n) << 21) /**< @brief Voltage Reference
                                                 sampling time.             */
/** @} */

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
