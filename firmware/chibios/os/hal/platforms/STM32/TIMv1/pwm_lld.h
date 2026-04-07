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
 * @file    STM32/pwm_lld.h
 * @brief   STM32 PWM subsystem low level driver header.
 *
 * @addtogroup PWM
 * @{
 */

#ifndef _PWM_LLD_H_
#define _PWM_LLD_H_

#include "stm32_tim.h"

#if HAL_USE_PWM || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @brief   Number of PWM channels per PWM driver.
 */
#define PWM_CHANNELS                            4

/**
 * @brief   Complementary output modes mask.
 * @note    This is an STM32-specific setting.
 */
#define PWM_COMPLEMENTARY_OUTPUT_MASK           0xF0

/**
 * @brief   Complementary output not driven.
 * @note    This is an STM32-specific setting.
 */
#define PWM_COMPLEMENTARY_OUTPUT_DISABLED       0x00

/**
 * @brief   Complementary output, active is logic level one.
 * @note    This is an STM32-specific setting.
 * @note    This setting is only available if the configuration option
 *          @p STM32_PWM_USE_ADVANCED is set to TRUE and only for advanced
 *          timers TIM1 and TIM8.
 */
#define PWM_COMPLEMENTARY_OUTPUT_ACTIVE_HIGH    0x10

/**
 * @brief   Complementary output, active is logic level zero.
 * @note    This is an STM32-specific setting.
 * @note    This setting is only available if the configuration option
 *          @p STM32_PWM_USE_ADVANCED is set to TRUE and only for advanced
 *          timers TIM1 and TIM8.
 */
#define PWM_COMPLEMENTARY_OUTPUT_ACTIVE_LOW     0x20

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    Configuration options
 * @{
 */
/**
 * @brief   If advanced timer features switch.
 * @details If set to @p TRUE the advanced features for TIM1 and TIM8 are
 *          enabled.
 * @note    The default is @p TRUE.
 */
#if !defined(STM32_PWM_USE_ADVANCED) || defined(__DOXYGEN__)
#define STM32_PWM_USE_ADVANCED              FALSE
#endif

/**
 * @brief   PWMD1 driver enable switch.
 * @details If set to @p TRUE the support for PWMD1 is included.
 * @note    The default is @p TRUE.
 */
#if !defined(STM32_PWM_USE_TIM1) || defined(__DOXYGEN__)
#define STM32_PWM_USE_TIM1                  FALSE
#endif

/**
 * @brief   PWMD2 driver enable switch.
 * @details If set to @p TRUE the support for PWMD2 is included.
 * @note    The default is @p TRUE.
 */
#if !defined(STM32_PWM_USE_TIM2) || defined(__DOXYGEN__)
#define STM32_PWM_USE_TIM2                  FALSE
#endif

/**
 * @brief   PWMD3 driver enable switch.
 * @details If set to @p TRUE the support for PWMD3 is included.
 * @note    The default is @p TRUE.
 */
#if !defined(STM32_PWM_USE_TIM3) || defined(__DOXYGEN__)
#define STM32_PWM_USE_TIM3                  FALSE
#endif

/**
 * @brief   PWMD4 driver enable switch.
 * @details If set to @p TRUE the support for PWMD4 is included.
 * @note    The default is @p TRUE.
 */
#if !defined(STM32_PWM_USE_TIM4) || defined(__DOXYGEN__)
#define STM32_PWM_USE_TIM4                  FALSE
#endif

/**
 * @brief   PWMD5 driver enable switch.
 * @details If set to @p TRUE the support for PWMD5 is included.
 * @note    The default is @p TRUE.
 */
#if !defined(STM32_PWM_USE_TIM5) || defined(__DOXYGEN__)
#define STM32_PWM_USE_TIM5                  FALSE
#endif

/**
 * @brief   PWMD8 driver enable switch.
 * @details If set to @p TRUE the support for PWMD8 is included.
 * @note    The default is @p TRUE.
 */
#if !defined(STM32_PWM_USE_TIM8) || defined(__DOXYGEN__)
#define STM32_PWM_USE_TIM8                  FALSE
#endif

/**
 * @brief   PWMD9 driver enable switch.
 * @details If set to @p TRUE the support for PWMD9 is included.
 * @note    The default is @p TRUE.
 */
#if !defined(STM32_PWM_USE_TIM9) || defined(__DOXYGEN__)
#define STM32_PWM_USE_TIM9                  FALSE
#endif

/**
 * @brief   PWMD1 interrupt priority level setting.
 */
#if !defined(STM32_PWM_TIM1_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define STM32_PWM_TIM1_IRQ_PRIORITY         7
#endif

/**
 * @brief   PWMD2 interrupt priority level setting.
 */
#if !defined(STM32_PWM_TIM2_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define STM32_PWM_TIM2_IRQ_PRIORITY         7
#endif

/**
 * @brief   PWMD3 interrupt priority level setting.
 */
#if !defined(STM32_PWM_TIM3_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define STM32_PWM_TIM3_IRQ_PRIORITY         7
#endif

/**
 * @brief   PWMD4 interrupt priority level setting.
 */
#if !defined(STM32_PWM_TIM4_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define STM32_PWM_TIM4_IRQ_PRIORITY         7
#endif

/**
 * @brief   PWMD5 interrupt priority level setting.
 */
#if !defined(STM32_PWM_TIM5_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define STM32_PWM_TIM5_IRQ_PRIORITY         7
#endif

/**
 * @brief   PWMD8 interrupt priority level setting.
 */
#if !defined(STM32_PWM_TIM8_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define STM32_PWM_TIM8_IRQ_PRIORITY         7
#endif
/** @} */

/**
 * @brief   PWMD9 interrupt priority level setting.
 */
#if !defined(STM32_PWM_TIM9_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define STM32_PWM_TIM9_IRQ_PRIORITY         7
#endif
/** @} */

/*===========================================================================*/
/* Configuration checks.                                                     */
/*===========================================================================*/

#if STM32_PWM_USE_TIM1 && !STM32_HAS_TIM1
#error "TIM1 not present in the selected device"
#endif

#if STM32_PWM_USE_TIM2 && !STM32_HAS_TIM2
#error "TIM2 not present in the selected device"
#endif

#if STM32_PWM_USE_TIM3 && !STM32_HAS_TIM3
#error "TIM3 not present in the selected device"
#endif

#if STM32_PWM_USE_TIM4 && !STM32_HAS_TIM4
#error "TIM4 not present in the selected device"
#endif

#if STM32_PWM_USE_TIM5 && !STM32_HAS_TIM5
#error "TIM5 not present in the selected device"
#endif

#if STM32_PWM_USE_TIM8 && !STM32_HAS_TIM8
#error "TIM8 not present in the selected device"
#endif

#if STM32_PWM_USE_TIM9 && !STM32_HAS_TIM9
#error "TIM9 not present in the selected device"
#endif

#if !STM32_PWM_USE_TIM1 && !STM32_PWM_USE_TIM2 &&                           \
    !STM32_PWM_USE_TIM3 && !STM32_PWM_USE_TIM4 &&                           \
    !STM32_PWM_USE_TIM5 && !STM32_PWM_USE_TIM8 &&                           \
    !STM32_PWM_USE_TIM9
#error "PWM driver activated but no TIM peripheral assigned"
#endif

#if STM32_PWM_USE_ADVANCED && !STM32_PWM_USE_TIM1 && !STM32_PWM_USE_TIM8
#error "advanced mode selected but no advanced timer assigned"
#endif

#if STM32_PWM_USE_TIM1 &&                                                   \
    !CORTEX_IS_VALID_KERNEL_PRIORITY(STM32_PWM_TIM1_IRQ_PRIORITY)
#error "Invalid IRQ priority assigned to TIM1"
#endif

#if STM32_PWM_USE_TIM2 &&                                                   \
    !CORTEX_IS_VALID_KERNEL_PRIORITY(STM32_PWM_TIM2_IRQ_PRIORITY)
#error "Invalid IRQ priority assigned to TIM2"
#endif

#if STM32_PWM_USE_TIM3 &&                                                   \
    !CORTEX_IS_VALID_KERNEL_PRIORITY(STM32_PWM_TIM3_IRQ_PRIORITY)
#error "Invalid IRQ priority assigned to TIM3"
#endif

#if STM32_PWM_USE_TIM4 &&                                                   \
    !CORTEX_IS_VALID_KERNEL_PRIORITY(STM32_PWM_TIM4_IRQ_PRIORITY)
#error "Invalid IRQ priority assigned to TIM4"
#endif

#if STM32_PWM_USE_TIM5 &&                                                   \
    !CORTEX_IS_VALID_KERNEL_PRIORITY(STM32_PWM_TIM5_IRQ_PRIORITY)
#error "Invalid IRQ priority assigned to TIM5"
#endif

#if STM32_PWM_USE_TIM8 &&                                                   \
    !CORTEX_IS_VALID_KERNEL_PRIORITY(STM32_PWM_TIM8_IRQ_PRIORITY)
#error "Invalid IRQ priority assigned to TIM8"
#endif

#if STM32_PWM_USE_TIM9 &&                                                   \
    !CORTEX_IS_VALID_KERNEL_PRIORITY(STM32_PWM_TIM9_IRQ_PRIORITY)
#error "Invalid IRQ priority assigned to TIM9"
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief PWM mode type.
 */
typedef uint32_t pwmmode_t;

/**
 * @brief   PWM channel type.
 */
typedef uint8_t pwmchannel_t;

/**
 * @brief   PWM counter type.
 */
typedef uint16_t pwmcnt_t;

/**
 * @brief   PWM driver channel configuration structure.
 */
typedef struct {
  /**
   * @brief Channel active logic level.
   */
  pwmmode_t                 mode;
  /**
   * @brief Channel callback pointer.
   * @note  This callback is invoked on the channel compare event. If set to
   *        @p NULL then the callback is disabled.
   */
  pwmcallback_t             callback;
  /* End of the mandatory fields.*/
} PWMChannelConfig;

/**
 * @brief   PWM driver configuration structure.
 */
typedef struct {
  /**
   * @brief   Timer clock in Hz.
   * @note    The low level can use assertions in order to catch invalid
   *          frequency specifications.
   */
  uint32_t                  frequency;
  /**
   * @brief   PWM period in ticks.
   * @note    The low level can use assertions in order to catch invalid
   *          period specifications.
   */
  pwmcnt_t                  period;
  /**
   * @brief Periodic callback pointer.
   * @note  This callback is invoked on PWM counter reset. If set to
   *        @p NULL then the callback is disabled.
   */
  pwmcallback_t             callback;
  /**
   * @brief Channels configurations.
   */
  PWMChannelConfig          channels[PWM_CHANNELS];
  /* End of the mandatory fields.*/
  /**
   * @brief TIM CR2 register initialization data.
   * @note  The value of this field should normally be equal to zero.
   */
  uint32_t                  cr2;
#if STM32_PWM_USE_ADVANCED || defined(__DOXYGEN__)
  /**
   * @brief TIM BDTR (break & dead-time) register initialization data.
   * @note  The value of this field should normally be equal to zero.
   */                                                                     \
   uint32_t                 bdtr;
#endif
   /**
    * @brief TIM DIER register initialization data.
    * @note  The value of this field should normally be equal to zero.
    * @note  Only the DMA-related bits can be specified in this field.
    */
   uint32_t                 dier;
} PWMConfig;

/**
 * @brief   Structure representing a PWM driver.
 */
struct PWMDriver {
  /**
   * @brief Driver state.
   */
  pwmstate_t                state;
  /**
   * @brief Current driver configuration data.
   */
  const PWMConfig           *config;
  /**
   * @brief   Current PWM period in ticks.
   */
  pwmcnt_t                  period;
#if defined(PWM_DRIVER_EXT_FIELDS)
  PWM_DRIVER_EXT_FIELDS
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
 * @brief   Changes the period the PWM peripheral.
 * @details This function changes the period of a PWM unit that has already
 *          been activated using @p pwmStart().
 * @pre     The PWM unit must have been activated using @p pwmStart().
 * @post    The PWM unit period is changed to the new value.
 * @note    The function has effect at the next cycle start.
 * @note    If a period is specified that is shorter than the pulse width
 *          programmed in one of the channels then the behavior is not
 *          guaranteed.
 *
 * @param[in] pwmp      pointer to a @p PWMDriver object
 * @param[in] period    new cycle time in ticks
 *
 * @notapi
 */
#define pwm_lld_change_period(pwmp, period)                                 \
  ((pwmp)->tim->ARR = (uint16_t)((period) - 1))

/**
 * @brief   Returns a PWM channel status.
 * @pre     The PWM unit must have been activated using @p pwmStart().
 *
 * @param[in] pwmp      pointer to a @p PWMDriver object
 * @param[in] channel   PWM channel identifier (0...PWM_CHANNELS-1)
 *
 * @notapi
 */
#define pwm_lld_is_channel_enabled(pwmp, channel)                           \
  (((pwmp)->tim->CCR[channel] != 0) ||                                      \
   (((pwmp)->tim->DIER & (2 << channel)) != 0))

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if STM32_PWM_USE_TIM1 && !defined(__DOXYGEN__)
extern PWMDriver PWMD1;
#endif

#if STM32_PWM_USE_TIM2 && !defined(__DOXYGEN__)
extern PWMDriver PWMD2;
#endif

#if STM32_PWM_USE_TIM3 && !defined(__DOXYGEN__)
extern PWMDriver PWMD3;
#endif

#if STM32_PWM_USE_TIM4 && !defined(__DOXYGEN__)
extern PWMDriver PWMD4;
#endif

#if STM32_PWM_USE_TIM5 && !defined(__DOXYGEN__)
extern PWMDriver PWMD5;
#endif

#if STM32_PWM_USE_TIM8 && !defined(__DOXYGEN__)
extern PWMDriver PWMD8;
#endif

#if STM32_PWM_USE_TIM9 && !defined(__DOXYGEN__)
extern PWMDriver PWMD9;
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void pwm_lld_init(void);
  void pwm_lld_start(PWMDriver *pwmp);
  void pwm_lld_stop(PWMDriver *pwmp);
  void pwm_lld_enable_channel(PWMDriver *pwmp,
                              pwmchannel_t channel,
                              pwmcnt_t width);
  void pwm_lld_disable_channel(PWMDriver *pwmp, pwmchannel_t channel);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_PWM */

#endif /* _PWM_LLD_H_ */

/** @} */
