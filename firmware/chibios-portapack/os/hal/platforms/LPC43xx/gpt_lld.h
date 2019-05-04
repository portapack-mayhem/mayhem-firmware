/*
    ChibiOS/RT - Copyright (C) 2006-2013 Giovanni Di Sirio
                 Copyright (C) 2014 Jared Boone, ShareBrained Technology

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
 * @file    LPC43xx/gpt_lld.h
 * @brief   LPC43xx GPT subsystem low level driver header.
 *
 * @addtogroup GPT
 * @{
 */

#ifndef _GPT_LLD_H_
#define _GPT_LLD_H_

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
#if !defined(LPC43XX_GPT_USE_TIMER0) || defined(__DOXYGEN__)
#define LPC43XX_GPT_USE_TIMER0                FALSE
#endif

/**
 * @brief   GPTD2 driver enable switch.
 * @details If set to @p TRUE the support for GPTD2 is included.
 * @note    The default is @p TRUE.
 */
#if !defined(LPC43XX_GPT_USE_TIMER1) || defined(__DOXYGEN__)
#define LPC43XX_GPT_USE_TIMER1                FALSE
#endif

/**
 * @brief   GPTD3 driver enable switch.
 * @details If set to @p TRUE the support for GPTD3 is included.
 * @note    The default is @p TRUE.
 */
#if !defined(LPC43XX_GPT_USE_TIMER2) || defined(__DOXYGEN__)
#define LPC43XX_GPT_USE_TIMER2                FALSE
#endif

/**
 * @brief   GPTD4 driver enable switch.
 * @details If set to @p TRUE the support for GPTD4 is included.
 * @note    The default is @p TRUE.
 */
#if !defined(LPC43XX_GPT_USE_TIMER3) || defined(__DOXYGEN__)
#define LPC43XX_GPT_USE_TIMER3                FALSE
#endif

/**
 * @brief   GPTD1 interrupt priority level setting.
 */
#if !defined(LPC43XX_GPT_TIMER0_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define LPC43XX_GPT_TIMER0_IRQ_PRIORITY       7
#endif

/**
 * @brief   GPTD2 interrupt priority level setting.
 */
#if !defined(LPC43XX_GPT_TIMER1_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define LPC43XX_GPT_TIMER1_IRQ_PRIORITY       7
#endif

/**
 * @brief   GPTD3 interrupt priority level setting.
 */
#if !defined(LPC43XX_GPT_TIMER2_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define LPC43XX_GPT_TIMER2_IRQ_PRIORITY       7
#endif

/**
 * @brief   GPTD4 interrupt priority level setting.
 */
#if !defined(LPC43XX_GPT_TIMER3_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define LPC43XX_GPT_TIMER3_IRQ_PRIORITY       7
#endif

/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if LPC43XX_GPT_USE_TIMER0 &&                                                   \
    !CORTEX_IS_VALID_KERNEL_PRIORITY(LPC43XX_GPT_TIMER0_IRQ_PRIORITY)
#error "Invalid IRQ priority assigned to TIMER0"
#endif

#if LPC43XX_GPT_USE_TIMER1 &&                                                   \
    !CORTEX_IS_VALID_KERNEL_PRIORITY(LPC43XX_GPT_TIMER1_IRQ_PRIORITY)
#error "Invalid IRQ priority assigned to TIMER1"
#endif

#if LPC43XX_GPT_USE_TIMER2 &&                                                   \
    !CORTEX_IS_VALID_KERNEL_PRIORITY(LPC43XX_GPT_TIMER2_IRQ_PRIORITY)
#error "Invalid IRQ priority assigned to TIMER2"
#endif

#if LPC43XX_GPT_USE_TIMER3 &&                                                   \
    !CORTEX_IS_VALID_KERNEL_PRIORITY(LPC43XX_GPT_TIMER3_IRQ_PRIORITY)
#error "Invalid IRQ priority assigned to TIMER3"
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
  // gptfreq_t                 frequency;
  /**
   * @brief   Timer callback pointer.
   * @note    This callback is invoked on GPT counter events.
   */
  gptcallback_t             callback;
  /* End of the mandatory fields.*/
  /**
   * @brief Prescale counter value.
   */
  uint32_t                  pr;
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
  /* End of the mandatory fields.*/
  /**
   * @brief Pointer to the TIMER registers block.
   */
  LPC_TIMER_Type            *tmr;
  /**
   * @brief Pointer to the non-peripheral Timer resources.
   */
  const timer_resources_t * resources;
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
 * @note    The function has effect immediately.
 *
 * @param[in] gptp      pointer to a @p GPTDriver object
 * @param[in] interval  new cycle time in timer ticks
 * @notapi
 */
#define gpt_lld_change_interval(gptp, interval)                               \
  ((gptp)->tmr->MR[0] = ((interval) - 1))

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if LPC43XX_GPT_USE_TIMER0 && !defined(__DOXYGEN__)
extern GPTDriver GPTD1;
#endif

#if LPC43XX_GPT_USE_TIMER1 && !defined(__DOXYGEN__)
extern GPTDriver GPTD2;
#endif

#if LPC43XX_GPT_USE_TIMER2 && !defined(__DOXYGEN__)
extern GPTDriver GPTD3;
#endif

#if LPC43XX_GPT_USE_TIMER3 && !defined(__DOXYGEN__)
extern GPTDriver GPTD4;
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
