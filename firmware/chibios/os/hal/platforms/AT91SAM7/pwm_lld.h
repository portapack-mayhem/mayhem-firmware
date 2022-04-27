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
 * @file    AT91SAM7/pwm_lld.h
 * @brief   AT91SAM7 PWM Driver subsystem low level driver header.
 *
 * @addtogroup PWM
 * @{
 */

#ifndef _PWM_LLD_H_
#define _PWM_LLD_H_

#if HAL_USE_PWM || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @brief   Number of PWM channels per PWM driver.
 */
#define PWM_CHANNELS        1

/**
 * @brief   PWM device interrupt priority level setting.
 */
#if !defined(AT91SAM7_PWM_PRIORITY) || defined(__DOXYGEN__)
#define AT91SAM7_PWM_PRIORITY          (AT91C_AIC_PRIOR_HIGHEST - 4)
#endif

/**
 * @brief   PWMD1 driver enable switch.
 * @details If set to @p TRUE the support for PWMD1 is included.
 * @note    The default is @p TRUE.
 */
#if !defined(PWM_USE_PWM1) || defined(__DOXYGEN__)
#define PWM_USE_PWM1					TRUE
#endif

/**
 * @brief   PWMD2 driver enable switch.
 * @details If set to @p TRUE the support for PWMD1 is included.
 * @note    The default is @p TRUE.
 */
#if !defined(PWM_USE_PWM2) || defined(__DOXYGEN__)
#define PWM_USE_PWM2					TRUE
#endif

/**
 * @brief   PWMD3 driver enable switch.
 * @details If set to @p TRUE the support for PWMD1 is included.
 * @note    The default is @p TRUE.
 */
#if !defined(PWM_USE_PWM3) || defined(__DOXYGEN__)
#define PWM_USE_PWM3					TRUE
#endif

/**
 * @brief   PWMD4 driver enable switch.
 * @details If set to @p TRUE the support for PWMD1 is included.
 * @note    The default is @p TRUE.
 */
#if !defined(PWM_USE_PWM4) || defined(__DOXYGEN__)
#define PWM_USE_PWM4					TRUE
#endif

/**
 * @brief   PWM left (count up) logic
 */
#define PWM_OUTPUT_LEFT					0x00000000

/**
 * @brief   PWM center (count up-down) logic. Gives symetric waveform
 */
#define PWM_OUTPUT_CENTER				0x00000010

/**
 * @brief   PWM Master Clock = MCK / 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, CLKA or CLKB. CLKA or CLKB uses the frequency field
 */
#define PWM_MCK_DIV_1					0x00000000
#define PWM_MCK_DIV_2					0x00000100
#define PWM_MCK_DIV_4					0x00000200
#define PWM_MCK_DIV_8					0x00000300
#define PWM_MCK_DIV_16					0x00000400
#define PWM_MCK_DIV_32					0x00000500
#define PWM_MCK_DIV_64					0x00000600
#define PWM_MCK_DIV_128					0x00000700
#define PWM_MCK_DIV_256					0x00000800
#define PWM_MCK_DIV_512					0x00000900
#define PWM_MCK_DIV_1024				0x00000A00
#define PWM_MCK_DIV_CLKA				0x00000B00
#define PWM_MCK_DIV_CLKB				0x00000C00

/**
 * @brief   Which PWM output pins to turn on. PIN1 is the lowest numbered pin, PIN2 next lowest, and then on some packages PIN3.
 */
#define PWM_OUTPUT_PIN1					0x00001000
#define PWM_OUTPUT_PIN2					0x00002000
#define PWM_OUTPUT_PIN3					0x00004000

/**
 * @brief   Which PWM output pins should have pullups disabled.
 */
#define PWM_DISABLEPULLUP_PIN1			0x00010000
#define PWM_DISABLEPULLUP_PIN2			0x00020000
#define PWM_DISABLEPULLUP_PIN3			0x00040000

 /*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

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
 * @note    Some architectures may not be able to support the channel mode
 *          or the callback, in this case the fields are ignored.
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
 * @brief   Driver configuration structure.
 * @note    Implementations may extend this structure to contain more,
 *          architecture dependent, fields.
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
} PWMConfig;

/**
 * @brief   Structure representing an PWM driver.
 * @note    Implementations may extend this structure to contain more,
 *          architecture dependent, fields.
 */
struct PWMDriver {
  /**
   * @brief Driver state.
   */
  pwmstate_t                state;
  /**
   * @brief Current configuration data.
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
   * @brief The PWM internal channel number as a bit mask (1, 2, 4 or 8).
   */
  uint32_t					chbit;
  /**
   * @brief Pointer to the PWMCx registers block.
   */
  AT91S_PWMC_CH             *reg;
  /**
   * @brief Pointer to the output pins descriptor.
   */
  const struct pwmpindefs	*pins;
};

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if PWM_USE_PWM1 && !defined(__DOXYGEN__)
extern PWMDriver PWMD1;
#endif

#if PWM_USE_PWM2 && !defined(__DOXYGEN__)
extern PWMDriver PWMD2;
#endif

#if PWM_USE_PWM3 && !defined(__DOXYGEN__)
extern PWMDriver PWMD3;
#endif

#if PWM_USE_PWM4 && !defined(__DOXYGEN__)
extern PWMDriver PWMD4;
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void pwm_lld_init(void);
  void pwm_lld_start(PWMDriver *pwmp);
  void pwm_lld_stop(PWMDriver *pwmp);
  void pwm_lld_change_period(PWMDriver *pwmp, pwmcnt_t period);
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
