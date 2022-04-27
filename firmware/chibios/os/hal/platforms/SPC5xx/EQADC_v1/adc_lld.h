/*
    SPC5 HAL - Copyright (C) 2013 STMicroelectronics

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
 * @file    SPC5xx/EQADC_v1/adc_lld.c
 * @brief   SPC5xx low level ADC driver header.
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
 * @name    Analog channel identifiers
 * @{
 */
#define ADC_CHN_AN0                 0U
#define ADC_CHN_AN1                 1U
#define ADC_CHN_AN2                 2U
#define ADC_CHN_AN3                 3U
#define ADC_CHN_AN4                 4U
#define ADC_CHN_AN5                 5U
#define ADC_CHN_AN6                 6U
#define ADC_CHN_AN7                 7U
#define ADC_CHN_AN8                 8U
#define ADC_CHN_AN9                 9U
#define ADC_CHN_AN10                10U
#define ADC_CHN_AN11                11U
#define ADC_CHN_AN12                12U
#define ADC_CHN_AN13                13U
#define ADC_CHN_AN14                14U
#define ADC_CHN_AN15                15U
#define ADC_CHN_AN16                16U
#define ADC_CHN_AN17                17U
#define ADC_CHN_AN18                18U
#define ADC_CHN_AN19                19U
#define ADC_CHN_AN20                20U
#define ADC_CHN_AN21                21U
#define ADC_CHN_AN22                22U
#define ADC_CHN_AN23                23U
#define ADC_CHN_AN24                24U
#define ADC_CHN_AN25                25U
#define ADC_CHN_AN26                26U
#define ADC_CHN_AN27                27U
#define ADC_CHN_AN28                28U
#define ADC_CHN_AN29                29U
#define ADC_CHN_AN30                30U
#define ADC_CHN_AN31                31U
#define ADC_CHN_AN32                32U
#define ADC_CHN_AN33                33U
#define ADC_CHN_AN34                34U
#define ADC_CHN_AN35                35U
#define ADC_CHN_AN36                36U
#define ADC_CHN_AN37                37U
#define ADC_CHN_AN38                38U
#define ADC_CHN_AN39                39U
#define ADC_CHN_VRH                 40U
#define ADC_CHN_VRL                 41U
#define ADC_CHN_VREF50              42U
#define ADC_CHN_VREF75              43U
#define ADC_CHN_VREF25              44U
#define ADC_CHN_BANDGAP             45U
#define ADC_CHN_DAN0                96U
#define ADC_CHN_DAN1                97U
#define ADC_CHN_DAN2                98U
#define ADC_CHN_DAN3                99U
#define ADC_CHN_TEMP_SENSOR         128U
#define ADC_CHN_SPARE               129U
/** @} */

/**
 * @name    Internal registers indexes
 * @{
 */
#define ADC_REG_CR                  0x1
#define ADC_REG_TSCR                0x2
#define ADC_REG_TBCR                0x3
#define ADC_REG_GCCR                0x4
#define ADC_REG_OCCR                0x5
#define ADC_REG_AC1GCCR             0x31
#define ADC_REG_AC1OCCR             0x32
#define ADC_REG_AC2GCCR             0x35
#define ADC_REG_AC2OCCR             0x36
#define ADC_REG_AC1CR               0x30
#define ADC_REG_AC2CR               0x34
#define ADC_REG_AC3CR               0x38
#define ADC_REG_AC4CR               0x3C
#define ADC_REG_AC5CR               0x40
#define ADC_REG_AC6CR               0x44
#define ADC_REG_AC7CR               0x48
#define ADC_REG_AC8CR               0x4C
#define ADC_REG_PUDCR(n)            (0x70 + (n))
#define ADC_REG_PUDCR0              0x70UL
#define ADC_REG_PUDCR1              0x71UL
#define ADC_REG_PUDCR2              0x72UL
#define ADC_REG_PUDCR3              0x73UL
#define ADC_REG_PUDCR4              0x74UL
#define ADC_REG_PUDCR5              0x75UL
#define ADC_REG_PUDCR6              0x76UL
#define ADC_REG_PUDCR7              0x77UL
/** @} */

/**
 * @name    EQADC IDCR registers definitions
 * @{
 */
#define EQADC_IDCR_NCIE             (1U << 15)
#define EQADC_IDCR_TORIE            (1U << 14)
#define EQADC_IDCR_PIE              (1U << 13)
#define EQADC_IDCR_EOQIE            (1U << 12)
#define EQADC_IDCR_CFUIE            (1U << 11)
#define EQADC_IDCR_CFFE             (1U << 9)
#define EQADC_IDCR_CFFS             (1U << 8)
#define EQADC_IDCR_RFOIE            (1U << 3)
#define EQADC_IDCR_RFDE             (1U << 1)
#define EQADC_IDCR_RFDS             (1U << 0)
/** @} */

/**
 * @name    EQADC CFCR registers definitions
 * @{
 */
#define EQADC_CFCR_CFEEE0           (1U << 12)
#define EQADC_CFCR_STRME0           (1U << 11)
#define EQADC_CFCR_SSE              (1U << 10)
#define EQADC_CFCR_CFINV            (1U << 9)
#define EQADC_CFCR_MODE_MASK        (15U << 4)
#define EQADC_CFCR_MODE(n)          ((n) << 4)
#define EQADC_CFCR_MODE_DISABLED    EQADC_CFCR_MODE(0)
#define EQADC_CFCR_MODE_SWSS        EQADC_CFCR_MODE(1)
#define EQADC_CFCR_MODE_HWSS_LL     EQADC_CFCR_MODE(2)
#define EQADC_CFCR_MODE_HWSS_HL     EQADC_CFCR_MODE(3)
#define EQADC_CFCR_MODE_HWSS_FE     EQADC_CFCR_MODE(4)
#define EQADC_CFCR_MODE_HWSS_RE     EQADC_CFCR_MODE(5)
#define EQADC_CFCR_MODE_HWSS_BE     EQADC_CFCR_MODE(6)
#define EQADC_CFCR_MODE_SWCS        EQADC_CFCR_MODE(9)
#define EQADC_CFCR_MODE_HWCS_LL     EQADC_CFCR_MODE(10)
#define EQADC_CFCR_MODE_HWCS_HL     EQADC_CFCR_MODE(11)
#define EQADC_CFCR_MODE_HWCS_FE     EQADC_CFCR_MODE(12)
#define EQADC_CFCR_MODE_HWCS_RE     EQADC_CFCR_MODE(13)
#define EQADC_CFCR_MODE_HWCS_BE     EQADC_CFCR_MODE(14)
#define EQADC_CFCR_AMODE0_MASK      (15U << 0)
#define EQADC_CFCR_AMODE0(n)        ((n) << 0)
/** @} */

/**
 * @name    EQADC FISR registers definitions
 * @{
 */
#define EQADC_FISR_POPNXTPTR_MASK   (15U << 0)
#define EQADC_FISR_RFCTR_MASK       (15U << 4)
#define EQADC_FISR_TNXTPTR_MASK     (15U << 8)
#define EQADC_FISR_CFCTR_MASK       (15U << 12)
#define EQADC_FISR_RFDF             (1U << 17)
#define EQADC_FISR_RFOF             (1U << 19)
#define EQADC_FISR_CFFF             (1U << 25)
#define EQADC_FISR_SSS              (1U << 26)
#define EQADC_FISR_CFUF             (1U << 27)
#define EQADC_FISR_EOQF             (1U << 28)
#define EQADC_FISR_PF               (1U << 29)
#define EQADC_FISR_TORF             (1U << 30)
#define EQADC_FISR_NCF              (1U << 31)
#define EQADC_FISR_CLEAR_MASK       (EQADC_FISR_NCF  | EQADC_FISR_TORF |    \
                                     EQADC_FISR_PF   | EQADC_FISR_EOQF |    \
                                     EQADC_FISR_CFUF | EQADC_FISR_RFOF |    \
                                     EQADC_FISR_RFDF)
/** @} */

/**
 * @name    EQADC conversion/configuration commands
 * @{
 */
#define EQADC_CONV_CONFIG_STD   (0U << 0)   /**< @brief Alt.config.1.       */
#define EQADC_CONV_CONFIG_SEL1  (8U << 0)   /**< @brief Alt.config.1.       */
#define EQADC_CONV_CONFIG_SEL2  (9U << 0)   /**< @brief Alt.config.2.       */
#define EQADC_CONV_CONFIG_SEL3  (10U << 0)  /**< @brief Alt.config.3.       */
#define EQADC_CONV_CONFIG_SEL4  (11U << 0)  /**< @brief Alt.config.4.       */
#define EQADC_CONV_CONFIG_SEL5  (12U << 0)  /**< @brief Alt.config.5.       */
#define EQADC_CONV_CONFIG_SEL6  (13U << 0)  /**< @brief Alt.config.6.       */
#define EQADC_CONV_CONFIG_SEL7  (14U << 0)  /**< @brief Alt.config.7.       */
#define EQADC_CONV_CONFIG_SEL8  (15U << 0)  /**< @brief Alt.config.8.       */
#define EQADC_CONV_CHANNEL_MASK (255U << 8) /**< @brief Channel number mask.*/
#define EQADC_CONV_CHANNEL(n)   ((n) << 8)  /**< @brief Channel number.     */
#define EQADC_CONV_FMT_RJU      (0U << 16)  /**< @brief Unsigned samples.   */
#define EQADC_CONV_FMT_RJS      (1U << 16)  /**< @brief Signed samples.     */
#define EQADC_CONV_TSR          (1U << 17)  /**< @brief Time stamp request. */
#define EQADC_CONV_LST_MASK     (3U << 18)  /**< @brief Sample time.        */
#define EQADC_CONV_LST_2        (0U << 18)  /**< @brief 2 clock cycles.     */
#define EQADC_CONV_LST_8        (1U << 18)  /**< @brief 8 clock cycles.     */
#define EQADC_CONV_LST_64       (2U << 18)  /**< @brief 64 clock cycles.    */
#define EQADC_CONV_LST_128      (3U << 18)  /**< @brief 128 clock cycles.   */
#define EQADC_CONV_MSG_MASK     (15U << 20) /**< @brief Message mask.       */
#define EQADC_CONV_MSG_RFIFO(n) ((n) << 20) /**< @brief Result in RFIFO0..5.*/
#define EQADC_CONV_MSG_NULL     (6U << 20)  /**< @brief Null message.       */
#define EQADC_CONV_CAL          (1U << 24)  /**< @brief Calibrated result.  */
#define EQADC_CONV_BN_MASK      (1U << 25)  /**< @brief Buffer number mask. */
#define EQADC_CONV_BN_ADC0      (0U << 25)  /**< @brief ADC0 selection.     */
#define EQADC_CONV_BN_ADC1      (1U << 25)  /**< @brief ADC1 selection.     */
#define EQADC_CONV_REP          (1U << 29)  /**< @brief Repeat loop flag.   */
#define EQADC_CONV_PAUSE        (1U << 30)  /**< @brief Pause flag.         */
#define EQADC_CONV_EOQ          (1U << 31)  /**< @brief End of queue flag.  */
/** @} */

/**
 * @name    EQADC read/write commands
 * @{
 */
#define EQADC_RW_REG_ADDR_MASK  (255U << 0)
#define EQADC_RW_REG_ADDR(n)    ((n) << 0)
#define EQADC_RW_VALUE_MASK     (0xFFFF << 8)
#define EQADC_RW_VALUE(n)       ((n) << 8)
#define EQADC_RW_WRITE          (0U << 24)
#define EQADC_RW_READ           (1U << 24)
#define EQADC_RW_BN_ADC0        (0U << 25)
#define EQADC_RW_BN_ADC1        (1U << 25)
#define EQADC_RW_REP            (1U << 29)
#define EQADC_RW_PAUSE          (1U << 30)
#define EQADC_RW_EOQ            (1U << 31)
/** @} */

/**
 * @name    ADC CR register definitions
 * @{
 */
#define ADC_CR_CLK_PS_MASK      (31U << 0)
#define ADC_CR_CLK_PS(n)        ((((n) >> 1) - 1) | ((n) & 1 ? ADC_CR_ODD_PS\
                                                             : 0))
#define ADC_CR_CLK_SEL          (1U << 5)
#define ADC_CR_CLK_DTY          (1U << 6)
#define ADC_CR_ODD_PS           (1U << 7)
#define ADC_CR_TBSEL_MASK       (3U << 8)
#define ADC_CR_TBSEL(n)         ((n) << 8)
#define ADC_CR_EMUX             (1U << 11)
#define ADC_CR_EN               (1U << 15)
/** @} */

/**
 * @name    ADC AxCR registers definitions
 * @{
 */
#define ADC_ACR_PRE_GAIN_MASK   (3U << 0)
#define ADC_ACR_PRE_GAIN_X1     (0U << 0)
#define ADC_ACR_PRE_GAIN_X2     (1U << 0)
#define ADC_ACR_PRE_GAIN_X4     (2U << 0)
#define ADC_ACR_RESSEL_MASK     (3U << 6)
#define ADC_ACR_RESSEL_12BITS   (0U << 6)
#define ADC_ACR_RESSEL_10BITS   (1U << 6)
#define ADC_ACR_RESSEL_8BITS    (2U << 6)
/** @} */

/**
 * @name    ADC PUDCRx registers definitions
 * @{
 */
#define ADC_PUDCR_NONE          0x0000
#define ADC_PUDCR_UP_200K       0x1100
#define ADC_PUDCR_UP_100K       0x1200
#define ADC_PUDCR_UP_5K         0x1300
#define ADC_PUDCR_DOWN_200K     0x2100
#define ADC_PUDCR_DOWN_100K     0x2200
#define ADC_PUDCR_DOWN_5K       0x2300
#define ADC_PUDCR_UP_DOWN_200K  0x3100
#define ADC_PUDCR_UP_DOWN_100K  0x3200
/** @} */

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    Configuration options
 * @{
 */
/**
 * @brief   ADCD1 driver enable switch.
 * @details If set to @p TRUE the support for ADC0 queue 0 is included.
 * @note    The default is @p FALSE.
 */
#if !defined(SPC5_ADC_USE_ADC0_Q0) || defined(__DOXYGEN__)
#define SPC5_ADC_USE_ADC0_Q0                FALSE
#endif

/**
 * @brief   ADCD2 driver enable switch.
 * @details If set to @p TRUE the support for ADC0 queue 1 is included.
 * @note    The default is @p FALSE.
 */
#if !defined(SPC5_ADC_USE_ADC0_Q1) || defined(__DOXYGEN__)
#define SPC5_ADC_USE_ADC0_Q1                FALSE
#endif

/**
 * @brief   ADCD3 driver enable switch.
 * @details If set to @p TRUE the support for ADC0 queue 2 is included.
 * @note    The default is @p FALSE.
 */
#if !defined(SPC5_ADC_USE_ADC0_Q2) || defined(__DOXYGEN__)
#define SPC5_ADC_USE_ADC0_Q2                FALSE
#endif

/**
 * @brief   ADCD4 driver enable switch.
 * @details If set to @p TRUE the support for ADC1 queue 3 is included.
 * @note    The default is @p FALSE.
 */
#if !defined(SPC5_ADC_USE_ADC1_Q3) || defined(__DOXYGEN__)
#define SPC5_ADC_USE_ADC1_Q3                FALSE
#endif

/**
 * @brief   ADCD5 driver enable switch.
 * @details If set to @p TRUE the support for ADC1 queue 4 is included.
 * @note    The default is @p FALSE.
 */
#if !defined(SPC5_ADC_USE_ADC1_Q4) || defined(__DOXYGEN__)
#define SPC5_ADC_USE_ADC1_Q4                FALSE
#endif

/**
 * @brief   ADCD6 driver enable switch.
 * @details If set to @p TRUE the support for ADC1 queue 5 is included.
 * @note    The default is @p FALSE.
 */
#if !defined(SPC5_ADC_USE_ADC1_Q5) || defined(__DOXYGEN__)
#define SPC5_ADC_USE_ADC1_Q5                FALSE
#endif

/**
 * @brief   EQADC CFIFO0 and RFIFO0 DMA priority.
 */
#if !defined(SPC5_ADC_FIFO0_DMA_PRIO) || defined(__DOXYGEN__)
#define SPC5_ADC_FIFO0_DMA_PRIO             12
#endif

/**
 * @brief   EQADC CFIFO1 and RFIFO1 DMA priority.
 */
#if !defined(SPC5_ADC_FIFO1_DMA_PRIO) || defined(__DOXYGEN__)
#define SPC5_ADC_FIFO1_DMA_PRIO             12
#endif

/**
 * @brief   EQADC CFIFO2 and RFIFO2 DMA priority.
 */
#if !defined(SPC5_ADC_FIFO2_DMA_PRIO) || defined(__DOXYGEN__)
#define SPC5_ADC_FIFO2_DMA_PRIO             12
#endif

/**
 * @brief   EQADC CFIFO3 and RFIFO3 DMA priority.
 */
#if !defined(SPC5_ADC_FIFO3_DMA_PRIO) || defined(__DOXYGEN__)
#define SPC5_ADC_FIFO3_DMA_PRIO             12
#endif

/**
 * @brief   EQADC CFIFO4 and RFIFO4 DMA priority.
 */
#if !defined(SPC5_ADC_FIFO4_DMA_PRIO) || defined(__DOXYGEN__)
#define SPC5_ADC_FIFO4_DMA_PRIO             12
#endif

/**
 * @brief   EQADC CFIFO5 and RFIFO5 DMA priority.
 */
#if !defined(SPC5_ADC_FIFO5_DMA_PRIO) || defined(__DOXYGEN__)
#define SPC5_ADC_FIFO5_DMA_PRIO             12
#endif

/**
 * @brief   EQADC CFIFO0 and RFIFO0 DMA IRQ priority.
 */
#if !defined(SPC5_ADC0_FIFO0_DMA_IRQ_PRIO) || defined(__DOXYGEN__)
#define SPC5_ADC_FIFO0_DMA_IRQ_PRIO         12
#endif

/**
 * @brief   EQADC CFIFO1 and RFIFO1 DMA IRQ priority.
 */
#if !defined(SPC5_ADC0_FIFO1_DMA_IRQ_PRIO) || defined(__DOXYGEN__)
#define SPC5_ADC_FIFO1_DMA_IRQ_PRIO         12
#endif

/**
 * @brief   EQADC CFIFO2 and RFIFO2 DMA IRQ priority.
 */
#if !defined(SPC5_ADC0_FIFO2_DMA_IRQ_PRIO) || defined(__DOXYGEN__)
#define SPC5_ADC_FIFO2_DMA_IRQ_PRIO         12
#endif

/**
 * @brief   EQADC CFIFO3 and RFIFO3 DMA IRQ priority.
 */
#if !defined(SPC5_ADC0_FIFO3_DMA_IRQ_PRIO) || defined(__DOXYGEN__)
#define SPC5_ADC_FIFO3_DMA_IRQ_PRIO         12
#endif

/**
 * @brief   EQADC CFIFO4 and RFIFO4 DMA IRQ priority.
 */
#if !defined(SPC5_ADC0_FIFO4_DMA_IRQ_PRIO) || defined(__DOXYGEN__)
#define SPC5_ADC_FIFO4_DMA_IRQ_PRIO         12
#endif

/**
 * @brief   EQADC CFIFO5 and RFIFO5 DMA IRQ priority.
 */
#if !defined(SPC5_ADC0_FIFO5_DMA_IRQ_PRIO) || defined(__DOXYGEN__)
#define SPC5_ADC_FIFO5_DMA_IRQ_PRIO         12
#endif

/**
 * @brief   EQADC clock prescaler value.
 */
#if !defined(SPC5_ADC_CR_CLK_PS) || defined(__DOXYGEN__)
#define SPC5_ADC_CR_CLK_PS                  ADC_CR_CLK_PS(5)
#endif

/**
 * @brief   Initialization value for PUDCRx registers.
 */
#if !defined(SPC5_ADC_PUDCR) || defined(__DOXYGEN__)
#define SPC5_ADC_PUDCR                      {ADC_PUDCR_NONE,                \
                                             ADC_PUDCR_NONE,                \
                                             ADC_PUDCR_NONE,                \
                                             ADC_PUDCR_NONE,                \
                                             ADC_PUDCR_NONE,                \
                                             ADC_PUDCR_NONE,                \
                                             ADC_PUDCR_NONE,                \
                                             ADC_PUDCR_NONE}
#endif
/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if !SPC5_HAS_EQADC
#error "EQADC not present in the selected device"
#endif

#define SPC5_ADC_USE_ADC0                   (SPC5_ADC_USE_ADC0_Q0 ||        \
                                             SPC5_ADC_USE_ADC0_Q1 ||        \
                                             SPC5_ADC_USE_ADC0_Q2)
#define SPC5_ADC_USE_ADC1                   (SPC5_ADC_USE_ADC1_Q3 ||        \
                                             SPC5_ADC_USE_ADC1_Q4 ||        \
                                             SPC5_ADC_USE_ADC1_Q5)

#if !SPC5_ADC_USE_ADC0 && !SPC5_ADC_USE_ADC1
#error "ADC driver activated but no ADC peripheral assigned"
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/*!
 * @brief   FIFO unit numeric IDs.
 */
typedef enum {
  ADC_FIFO_0 = 0,
  ADC_FIFO_1 = 1,
  ADC_FIFO_2 = 2,
  ADC_FIFO_3 = 3,
  ADC_FIFO_4 = 4,
  ADC_FIFO_5 = 5
} adcfifo_t;

/**
 * @brief   ADC command data type.
 */
typedef uint32_t adccommand_t;

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
 * @note    Implementations may extend this structure to contain more,
 *          architecture dependent, fields.
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
   * @brief   Initialization value for CFCR register.
   */
  uint16_t                  cfcr;
  /**
   * @brief   SIU ETISR.TSEL value for this queue;
   */
  uint8_t                   tsel;
  /**
   * @brief   SIU ISEL3.ETSEL value for this queue;
   */
  uint8_t                   etsel;
  /**
   * @brief   Number of command iterations stored in @p commands.
   * @note    The total number of array elements must be @p num_channels *
   *          @p num_iterations.
   * @note    This fields is the upper limit of the parameter @p n that can
   *          be passed to the functions @p adcConvert() and
   *          @p adcStartConversion().
   */
  uint32_t                  num_iterations;
  /**
   * @brief   Pointer to an array of low level EQADC commands to be pushed
   *          into the CFIFO during a conversion.
   */
  const adccommand_t        *commands;
} ADCConversionGroup;

/**
 * @brief   Driver configuration structure.
 * @note    Empty in this implementation can be ignored.
 */
typedef struct {
  uint32_t                  dummy;
} ADCConfig;

/**
 * @brief   Structure representing an ADC driver.
 */
struct ADCDriver {
  /**
   * @brief   Driver state.
   */
  adcstate_t                state;
  /**
   * @brief   Current configuration data.
   */
  const ADCConfig           *config;
  /**
   * @brief   Current samples buffer pointer or @p NULL.
   */
  adcsample_t               *samples;
  /**
   * @brief   Current samples buffer depth or @p 0.
   */
  size_t                    depth;
  /**
   * @brief   Current conversion group pointer or @p NULL.
   */
  const ADCConversionGroup  *grpp;
#if ADC_USE_WAIT || defined(__DOXYGEN__)
  /**
   * @brief   Waiting thread.
   */
  Thread                    *thread;
#endif
#if ADC_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
#if CH_USE_MUTEXES || defined(__DOXYGEN__)
  /**
   * @brief   Mutex protecting the peripheral.
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
   * @brief   CFIFO/RFIFO used by this instance.
   */
  adcfifo_t                 fifo;
  /**
   * @brief   EDMA channel used for the CFIFO.
   */
  edma_channel_t            cfifo_channel;
  /**
   * @brief   EDMA channel used for the RFIFO.
   */
  edma_channel_t            rfifo_channel;
};

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if SPC5_ADC_USE_ADC0_Q0 && !defined(__DOXYGEN__)
extern ADCDriver ADCD1;
#endif

#if SPC5_ADC_USE_ADC0_Q1 && !defined(__DOXYGEN__)
extern ADCDriver ADCD2;
#endif

#if SPC5_ADC_USE_ADC0_Q2 && !defined(__DOXYGEN__)
extern ADCDriver ADCD3;
#endif

#if SPC5_ADC_USE_ADC1_Q3 && !defined(__DOXYGEN__)
extern ADCDriver ADCD4;
#endif

#if SPC5_ADC_USE_ADC1_Q4 && !defined(__DOXYGEN__)
extern ADCDriver ADCD5;
#endif

#if SPC5_ADC_USE_ADC1_Q5 && !defined(__DOXYGEN__)
extern ADCDriver ADCD6;
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
