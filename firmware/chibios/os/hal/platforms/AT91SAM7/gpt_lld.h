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
 * @file    AT91SAM7/gpt_lld.h
 * @brief   AT91SAM7 GPT subsystem low level driver header.
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
#if !defined(AT91_GPT_USE_TC0) || defined(__DOXYGEN__)
#define AT91_GPT_USE_TC0                  FALSE
#endif

/**
 * @brief   GPTD2 driver enable switch.
 * @details If set to @p TRUE the support for GPTD2 is included.
 * @note    The default is @p TRUE.
 */
#if !defined(AT91_GPT_USE_TC1) || defined(__DOXYGEN__)
#define AT91_GPT_USE_TC1                  FALSE
#endif

/**
 * @brief   GPTD3 driver enable switch.
 * @details If set to @p TRUE the support for GPTD3 is included.
 * @note    The default is @p TRUE.
 */
#if !defined(AT91_GPT_USE_TC2) || defined(__DOXYGEN__)
#define AT91_GPT_USE_TC3                  FALSE
#endif

/**
 * @brief   GPTD1 interrupt priority level setting.
 */
#if !defined(AT91_GPT_TC0_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define AT91_GPT_TC0_IRQ_PRIORITY         (AT91C_AIC_PRIOR_HIGHEST - 2)
#endif

/**
 * @brief   GPTD2 interrupt priority level setting.
 */
#if !defined(AT91_GPT_TC1_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define AT91_GPT_TC1_IRQ_PRIORITY         (AT91C_AIC_PRIOR_HIGHEST - 2)
#endif

/**
 * @brief   GPTD3 interrupt priority level setting.
 */
#if !defined(AT91_GPT_TC2_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define AT91_GPT_TC2_IRQ_PRIORITY         (AT91C_AIC_PRIOR_HIGHEST - 2)
#endif

/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if !AT91_GPT_USE_TC0 && !AT91_GPT_USE_TC1 && !AT91_GPT_USE_TC2
#error "GPT driver activated but no TC peripheral assigned"
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
typedef uint16_t gptcnt_t;

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
   * @brief Timer Clock Source.
   */
  uint8_t                  clocksource;
	#define GPT_CLOCK_MCLK			0				// @< Internal clock. frequency must = MCLK/2, MCLK/8, MCLK/32, MCLK/128 or MCLK/1024
	#define GPT_CLOCK_FREQUENCY		1				// @< Internal clock. interval is ignored. frequency determines rate
	#define GPT_CLOCK_RE_TCLK0		2				// @< External TCLK0. Rising Edge
	#define GPT_CLOCK_FE_TCLK0		3				// @< External TCLK0. Falling Edge
	#define GPT_CLOCK_RE_TCLK1		4				// @< External TCLK1. Rising Edge
	#define GPT_CLOCK_FE_TCLK1		5				// @< External TCLK1. Falling Edge
	#define GPT_CLOCK_RE_TCLK2		6				// @< External TCLK2. Rising Edge
	#define GPT_CLOCK_FE_TCLK2		7				// @< External TCLK2. Falling Edge
	#define GPT_CLOCK_RE_TC0		8				// @< TC0 output. Rising Edge. Do not use on TC0
	#define GPT_CLOCK_FE_TC0		9				// @< TC0 output. Falling Edge. Do not use on TC0
	#define GPT_CLOCK_RE_TC1		10				// @< TC1 output. Rising Edge. Do not use on TC1
	#define GPT_CLOCK_FE_TC1		11				// @< TC1 output. Falling Edge. Do not use on TC1
	#define GPT_CLOCK_RE_TC2		12				// @< TC2 output. Rising Edge. Do not use on TC2
	#define GPT_CLOCK_FE_TC2		13				// @< TC2 output. Falling Edge. Do not use on TC2
  uint8_t					clockgate;
	#define GPT_GATE_NONE			0				// @< Clock gating off
	#define GPT_GATE_TCLK0			1				// @< Clock on TCLK0 active high signal. If using this on TC0 with GPT_CLOCK_xx_TIMx, the TIMx output will be used instead.
	#define GPT_GATE_TCLK1			2				// @< Clock on TCLK1 active high signal. If using this on TC1 with GPT_CLOCK_xx_TIMx, the TIMx output will be used instead.
	#define GPT_GATE_TCLK2			3				// @< Clock on TCLK2 active high signal. If using this on TC2 with GPT_CLOCK_xx_TIMx, the TIMx output will be used instead.
  uint8_t					trigger;
	#define GPT_TRIGGER_NONE		0x00			// @< Start immediately
	#define GPT_TRIGGER_RE_TIOB		0x10			// @< Start on TIOB  signal. Rising Edge.
	#define GPT_TRIGGER_FE_TIOB		0x20			// @< Start on TIOB  signal. Falling Edge.
	#define GPT_TRIGGER_BE_TIOB		0x30			// @< Start on TIOB  signal. Both Edges.
	#define GPT_TRIGGER_RE_TCLK0	0x11			// @< Start on TCLK0 signal. Rising Edge.  If using this on TC0 with GPT_CLOCK_xx_TIMx, the TIMx output will be used instead.
	#define GPT_TRIGGER_FE_TCLK0	0x21			// @< Start on TCLK0 signal. Falling Edge. If using this on TC0 with GPT_CLOCK_xx_TIMx, the TIMx output will be used instead.
	#define GPT_TRIGGER_BE_TCLK0	0x31			// @< Start on TCLK0 signal. Both Edges.   If using this on TC0 with GPT_CLOCK_xx_TIMx, the TIMx output will be used instead.
	#define GPT_TRIGGER_RE_TCLK1	0x12			// @< Start on TCLK1 signal. Rising Edge.  If using this on TC1 with GPT_CLOCK_xx_TIMx, the TIMx output will be used instead.
	#define GPT_TRIGGER_FE_TCLK1	0x22			// @< Start on TCLK1 signal. Falling Edge. If using this on TC1 with GPT_CLOCK_xx_TIMx, the TIMx output will be used instead.
	#define GPT_TRIGGER_BE_TCLK1	0x32			// @< Start on TCLK1 signal. Both Edges.   If using this on TC1 with GPT_CLOCK_xx_TIMx, the TIMx output will be used instead.
	#define GPT_TRIGGER_RE_TCLK2	0x13			// @< Start on TCLK2 signal. Rising Edge.  If using this on TC2 with GPT_CLOCK_xx_TIMx, the TIMx output will be used instead.
	#define GPT_TRIGGER_FE_TCLK2	0x23			// @< Start on TCLK2 signal. Falling Edge. If using this on TC2 with GPT_CLOCK_xx_TIMx, the TIMx output will be used instead.
	#define GPT_TRIGGER_BE_TCLK2	0x33			// @< Start on TCLK2 signal. Both Edges.   If using this on TC2 with GPT_CLOCK_xx_TIMx, the TIMx output will be used instead.
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
   * @brief Pointer to the TCx registers block.
   */
  AT91S_TC               *tc;
};

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if AT91_GPT_USE_TC0 && !defined(__DOXYGEN__)
extern GPTDriver GPTD1;
#endif

#if AT91_GPT_USE_TC1 && !defined(__DOXYGEN__)
extern GPTDriver GPTD2;
#endif

#if AT91_GPT_USE_TC2 && !defined(__DOXYGEN__)
extern GPTDriver GPTD3;
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void gpt_lld_init(void);
  void gpt_lld_start(GPTDriver *gptp);
  void gpt_lld_stop(GPTDriver *gptp);
  void gpt_lld_start_timer(GPTDriver *gptp, gptcnt_t period);
  void gpt_lld_stop_timer(GPTDriver *gptp);
  void gpt_lld_change_interval(GPTDriver *gptp, gptcnt_t interval);
  void gpt_lld_polled_delay(GPTDriver *gptp, gptcnt_t interval);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_GPT */

#endif /* _GPT_LLD_H_ */

/** @} */
