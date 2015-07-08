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
 * @file    STM32/gpt_lld.h
 * @brief   STM32 GPT subsystem low level driver header.
 *
 * @addtogroup GPT
 * @{
 */

#ifndef _GPT_LLD_H_
#define _GPT_LLD_H_

#include "stm32_tim.h"

#if HAL_USE_GPT || defined(__DOXYGEN__)

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
 * @brief   GPTD1 driver enable switch.
 * @details If set to @p TRUE the support for GPTD1 is included.
 * @note    The default is @p TRUE.
 */
#if !defined(STM32_GPT_USE_TIM1) || defined(__DOXYGEN__)
#define STM32_GPT_USE_TIM1                  FALSE
#endif

/**
 * @brief   GPTD2 driver enable switch.
 * @details If set to @p TRUE the support for GPTD2 is included.
 * @note    The default is @p TRUE.
 */
#if !defined(STM32_GPT_USE_TIM2) || defined(__DOXYGEN__)
#define STM32_GPT_USE_TIM2                  FALSE
#endif

/**
 * @brief   GPTD3 driver enable switch.
 * @details If set to @p TRUE the support for GPTD3 is included.
 * @note    The default is @p TRUE.
 */
#if !defined(STM32_GPT_USE_TIM3) || defined(__DOXYGEN__)
#define STM32_GPT_USE_TIM3                  FALSE
#endif

/**
 * @brief   GPTD4 driver enable switch.
 * @details If set to @p TRUE the support for GPTD4 is included.
 * @note    The default is @p TRUE.
 */
#if !defined(STM32_GPT_USE_TIM4) || defined(__DOXYGEN__)
#define STM32_GPT_USE_TIM4                  FALSE
#endif

/**
 * @brief   GPTD5 driver enable switch.
 * @details If set to @p TRUE the support for GPTD5 is included.
 * @note    The default is @p TRUE.
 */
#if !defined(STM32_GPT_USE_TIM5) || defined(__DOXYGEN__)
#define STM32_GPT_USE_TIM5                  FALSE
#endif

/**
 * @brief   GPTD6 driver enable switch.
 * @details If set to @p TRUE the support for GPTD6 is included.
 * @note    The default is @p TRUE.
 */
#if !defined(STM32_GPT_USE_TIM6) || defined(__DOXYGEN__)
#define STM32_GPT_USE_TIM6                  FALSE
#endif

/**
 * @brief   GPTD7 driver enable switch.
 * @details If set to @p TRUE the support for GPTD7 is included.
 * @note    The default is @p TRUE.
 */
#if !defined(STM32_GPT_USE_TIM7) || defined(__DOXYGEN__)
#define STM32_GPT_USE_TIM7                  FALSE
#endif

/**
 * @brief   GPTD8 driver enable switch.
 * @details If set to @p TRUE the support for GPTD8 is included.
 * @note    The default is @p TRUE.
 */
#if !defined(STM32_GPT_USE_TIM8) || defined(__DOXYGEN__)
#define STM32_GPT_USE_TIM8                  FALSE
#endif

/**
 * @brief   GPTD9 driver enable switch.
 * @details If set to @p TRUE the support for GPTD9 is included.
 * @note    The default is @p TRUE.
 */
#if !defined(STM32_GPT_USE_TIM9) || defined(__DOXYGEN__)
#define STM32_GPT_USE_TIM9                  FALSE
#endif

/**
 * @brief   GPTD11 driver enable switch.
 * @details If set to @p TRUE the support for GPTD11 is included.
 * @note    The default is @p TRUE.
 */
#if !defined(STM32_GPT_USE_TIM11) || defined(__DOXYGEN__)
#define STM32_GPT_USE_TIM11                 FALSE
#endif

/**
 * @brief   GPTD12 driver enable switch.
 * @details If set to @p TRUE the support for GPTD12 is included.
 * @note    The default is @p TRUE.
 */
#if !defined(STM32_GPT_USE_TIM12) || defined(__DOXYGEN__)
#define STM32_GPT_USE_TIM12                 FALSE
#endif

/**
 * @brief   GPTD14 driver enable switch.
 * @details If set to @p TRUE the support for GPTD14 is included.
 * @note    The default is @p TRUE.
 */
#if !defined(STM32_GPT_USE_TIM14) || defined(__DOXYGEN__)
#define STM32_GPT_USE_TIM14                 FALSE
#endif

/**
 * @brief   GPTD1 interrupt priority level setting.
 */
#if !defined(STM32_GPT_TIM1_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define STM32_GPT_TIM1_IRQ_PRIORITY         7
#endif

/**
 * @brief   GPTD2 interrupt priority level setting.
 */
#if !defined(STM32_GPT_TIM2_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define STM32_GPT_TIM2_IRQ_PRIORITY         7
#endif

/**
 * @brief   GPTD3 interrupt priority level setting.
 */
#if !defined(STM32_GPT_TIM3_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define STM32_GPT_TIM3_IRQ_PRIORITY         7
#endif

/**
 * @brief   GPTD4 interrupt priority level setting.
 */
#if !defined(STM32_GPT_TIM4_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define STM32_GPT_TIM4_IRQ_PRIORITY         7
#endif

/**
 * @brief   GPTD5 interrupt priority level setting.
 */
#if !defined(STM32_GPT_TIM5_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define STM32_GPT_TIM5_IRQ_PRIORITY         7
#endif

/**
 * @brief   GPTD6 interrupt priority level setting.
 */
#if !defined(STM32_GPT_TIM6_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define STM32_GPT_TIM6_IRQ_PRIORITY         7
#endif

/**
 * @brief   GPTD7 interrupt priority level setting.
 */
#if !defined(STM32_GPT_TIM7_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define STM32_GPT_TIM7_IRQ_PRIORITY         7
#endif

/**
 * @brief   GPTD8 interrupt priority level setting.
 */
#if !defined(STM32_GPT_TIM8_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define STM32_GPT_TIM8_IRQ_PRIORITY         7
#endif

/**
 * @brief   GPTD9 interrupt priority level setting.
 */
#if !defined(STM32_GPT_TIM9_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define STM32_GPT_TIM9_IRQ_PRIORITY         7
#endif

/**
 * @brief   GPTD11 interrupt priority level setting.
 */
#if !defined(STM32_GPT_TIM11_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define STM32_GPT_TIM11_IRQ_PRIORITY        7
#endif

/**
 * @brief   GPTD12 interrupt priority level setting.
 */
#if !defined(STM32_GPT_TIM12_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define STM32_GPT_TIM12_IRQ_PRIORITY        7
#endif

/**
 * @brief   GPTD14 interrupt priority level setting.
 */
#if !defined(STM32_GPT_TIM14_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define STM32_GPT_TIM14_IRQ_PRIORITY        7
#endif
/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if STM32_GPT_USE_TIM1 && !STM32_HAS_TIM1
#error "TIM1 not present in the selected device"
#endif

#if STM32_GPT_USE_TIM2 && !STM32_HAS_TIM2
#error "TIM2 not present in the selected device"
#endif

#if STM32_GPT_USE_TIM3 && !STM32_HAS_TIM3
#error "TIM3 not present in the selected device"
#endif

#if STM32_GPT_USE_TIM4 && !STM32_HAS_TIM4
#error "TIM4 not present in the selected device"
#endif

#if STM32_GPT_USE_TIM5 && !STM32_HAS_TIM5
#error "TIM5 not present in the selected device"
#endif

#if STM32_GPT_USE_TIM6 && !STM32_HAS_TIM6
#error "TIM6 not present in the selected device"
#endif

#if STM32_GPT_USE_TIM7 && !STM32_HAS_TIM7
#error "TIM7 not present in the selected device"
#endif

#if STM32_GPT_USE_TIM8 && !STM32_HAS_TIM8
#error "TIM8 not present in the selected device"
#endif

#if STM32_GPT_USE_TIM9 && !STM32_HAS_TIM9
#error "TIM9 not present in the selected device"
#endif

#if STM32_GPT_USE_TIM11 && !STM32_HAS_TIM11
#error "TIM11 not present in the selected device"
#endif

#if STM32_GPT_USE_TIM12 && !STM32_HAS_TIM12
#error "TIM12 not present in the selected device"
#endif

#if STM32_GPT_USE_TIM14 && !STM32_HAS_TIM14
#error "TIM14 not present in the selected device"
#endif

#if !STM32_GPT_USE_TIM1 && !STM32_GPT_USE_TIM2 &&                           \
    !STM32_GPT_USE_TIM3 && !STM32_GPT_USE_TIM4 &&  \
    !STM32_GPT_USE_TIM5 && !STM32_GPT_USE_TIM6 &&  \
    !STM32_GPT_USE_TIM7 && !STM32_GPT_USE_TIM8 &&  \
    !STM32_GPT_USE_TIM9 && !STM32_GPT_USE_TIM11 && \
    !STM32_GPT_USE_TIM12 && !STM32_GPT_USE_TIM14
#error "GPT driver activated but no TIM peripheral assigned"
#endif

#if STM32_GPT_USE_TIM1 &&                                                   \
    !CORTEX_IS_VALID_KERNEL_PRIORITY(STM32_GPT_TIM1_IRQ_PRIORITY)
#error "Invalid IRQ priority assigned to TIM1"
#endif

#if STM32_GPT_USE_TIM2 &&                                                   \
    !CORTEX_IS_VALID_KERNEL_PRIORITY(STM32_GPT_TIM2_IRQ_PRIORITY)
#error "Invalid IRQ priority assigned to TIM2"
#endif

#if STM32_GPT_USE_TIM3 &&                                                   \
    !CORTEX_IS_VALID_KERNEL_PRIORITY(STM32_GPT_TIM3_IRQ_PRIORITY)
#error "Invalid IRQ priority assigned to TIM3"
#endif

#if STM32_GPT_USE_TIM4 &&                                                   \
    !CORTEX_IS_VALID_KERNEL_PRIORITY(STM32_GPT_TIM4_IRQ_PRIORITY)
#error "Invalid IRQ priority assigned to TIM4"
#endif

#if STM32_GPT_USE_TIM5 &&                                                   \
    !CORTEX_IS_VALID_KERNEL_PRIORITY(STM32_GPT_TIM5_IRQ_PRIORITY)
#error "Invalid IRQ priority assigned to TIM5"
#endif

#if STM32_GPT_USE_TIM6 &&                                                   \
    !CORTEX_IS_VALID_KERNEL_PRIORITY(STM32_GPT_TIM6_IRQ_PRIORITY)
#error "Invalid IRQ priority assigned to TIM6"
#endif

#if STM32_GPT_USE_TIM7 &&                                                   \
    !CORTEX_IS_VALID_KERNEL_PRIORITY(STM32_GPT_TIM7_IRQ_PRIORITY)
#error "Invalid IRQ priority assigned to TIM7"
#endif

#if STM32_GPT_USE_TIM8 &&                                                   \
    !CORTEX_IS_VALID_KERNEL_PRIORITY(STM32_GPT_TIM8_IRQ_PRIORITY)
#error "Invalid IRQ priority assigned to TIM8"
#endif

#if STM32_GPT_USE_TIM9 &&                                                   \
    !CORTEX_IS_VALID_KERNEL_PRIORITY(STM32_GPT_TIM9_IRQ_PRIORITY)
#error "Invalid IRQ priority assigned to TIM9"
#endif

#if STM32_GPT_USE_TIM11 &&                                                   \
    !CORTEX_IS_VALID_KERNEL_PRIORITY(STM32_GPT_TIM11_IRQ_PRIORITY)
#error "Invalid IRQ priority assigned to TIM11"
#endif

#if STM32_GPT_USE_TIM12 &&                                                   \
    !CORTEX_IS_VALID_KERNEL_PRIORITY(STM32_GPT_TIM12_IRQ_PRIORITY)
#error "Invalid IRQ priority assigned to TIM12"
#endif

#if STM32_GPT_USE_TIM14 &&                                                   \
    !CORTEX_IS_VALID_KERNEL_PRIORITY(STM32_GPT_TIM14_IRQ_PRIORITY)
#error "Invalid IRQ priority assigned to TIM14"
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   GPT frequency type.
 */
typedef uint32_t gptfreq_t;

/**
 * @brief   GPT counter type.
 */
typedef uint32_t gptcnt_t;

/**
 * @brief   Driver configuration structure.
 * @note    It could be empty on some architectures.
 */
typedef struct {
  /**
   * @brief   Timer clock in Hz.
   * @note    The low level can use assertions in order to catch invalid
   *          frequency specifications.
   */
  gptfreq_t                 frequency;
  /**
   * @brief   Timer callback pointer.
   * @note    This callback is invoked on GPT counter events.
   */
  gptcallback_t             callback;
  /* End of the mandatory fields.*/
  /**
   * @brief TIM DIER register initialization data.
   * @note  The value of this field should normally be equal to zero.
   * @note  Only the DMA-related bits can be specified in this field.
   */
  uint32_t                  dier;
} GPTConfig;

/**
 * @brief   Structure representing a GPT driver.
 */
struct GPTDriver {
  /**
   * @brief Driver state.
   */
  gptstate_t                state;
  /**
   * @brief Current configuration data.
   */
  const GPTConfig           *config;
#if defined(GPT_DRIVER_EXT_FIELDS)
  GPT_DRIVER_EXT_FIELDS
#endif
  /* End of the mandatory fields.*/
  /**
   * @brief Timer base clock.
   */
  uint32_t                  clock;
  /**
   * @brief Pointer to the TIMx registers block.
   */
  stm32_tim_t               *tim;
};

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/**
 * @brief   Changes the interval of GPT peripheral.
 * @details This function changes the interval of a running GPT unit.
 * @pre     The GPT unit must have been activated using @p gptStart().
 * @pre     The GPT unit must have been running in continuous mode using
 *          @p gptStartContinuous().
 * @post    The GPT unit interval is changed to the new value.
 * @note    The function has effect at the next cycle start.
 *
 * @param[in] gptp      pointer to a @p GPTDriver object
 * @param[in] interval  new cycle time in timer ticks
 * @notapi
 */
#define gpt_lld_change_interval(gptp, interval)                               \
  ((gptp)->tim->ARR = (uint32_t)((interval) - 1))

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if STM32_GPT_USE_TIM1 && !defined(__DOXYGEN__)
extern GPTDriver GPTD1;
#endif

#if STM32_GPT_USE_TIM2 && !defined(__DOXYGEN__)
extern GPTDriver GPTD2;
#endif

#if STM32_GPT_USE_TIM3 && !defined(__DOXYGEN__)
extern GPTDriver GPTD3;
#endif

#if STM32_GPT_USE_TIM4 && !defined(__DOXYGEN__)
extern GPTDriver GPTD4;
#endif

#if STM32_GPT_USE_TIM5 && !defined(__DOXYGEN__)
extern GPTDriver GPTD5;
#endif

#if STM32_GPT_USE_TIM6 && !defined(__DOXYGEN__)
extern GPTDriver GPTD6;
#endif

#if STM32_GPT_USE_TIM7 && !defined(__DOXYGEN__)
extern GPTDriver GPTD7;
#endif

#if STM32_GPT_USE_TIM8 && !defined(__DOXYGEN__)
extern GPTDriver GPTD8;
#endif

#if STM32_GPT_USE_TIM9 && !defined(__DOXYGEN__)
extern GPTDriver GPTD9;
#endif

#if STM32_GPT_USE_TIM11 && !defined(__DOXYGEN__)
extern GPTDriver GPTD11;
#endif

#if STM32_GPT_USE_TIM12 && !defined(__DOXYGEN__)
extern GPTDriver GPTD12;
#endif

#if STM32_GPT_USE_TIM14 && !defined(__DOXYGEN__)
extern GPTDriver GPTD14;
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void gpt_lld_init(void);
  void gpt_lld_start(GPTDriver *gptp);
  void gpt_lld_stop(GPTDriver *gptp);
  void gpt_lld_start_timer(GPTDriver *gptp, gptcnt_t period);
  void gpt_lld_stop_timer(GPTDriver *gptp);
  void gpt_lld_polled_delay(GPTDriver *gptp, gptcnt_t interval);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_GPT */

#endif /* _GPT_LLD_H_ */

/** @} */
