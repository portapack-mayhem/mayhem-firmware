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
 * @file    FlexPWM_v1/pwm_lld.c
 * @brief   SPC5xx low level PWM driver code.
 *
 * @addtogroup PWM
 * @{
 */

#include "ch.h"
#include "hal.h"

#if HAL_USE_PWM || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/**
 * @brief   PWMD1 driver identifier.
 * @note    The driver PWMD1 allocates the complex timer TIM1 when enabled.
 */
#if SPC5_PWM_USE_SMOD0 || defined(__DOXYGEN__)
PWMDriver PWMD1;
#endif

/**
 * @brief   PWMD2 driver identifier.
 * @note    The driver PWMD2 allocates the timer TIM2 when enabled.
 */
#if SPC5_PWM_USE_SMOD1 || defined(__DOXYGEN__)
PWMDriver PWMD2;
#endif

/**
 * @brief   PWMD3 driver identifier.
 * @note    The driver PWMD3 allocates the timer TIM3 when enabled.
 */
#if SPC5_PWM_USE_SMOD2 || defined(__DOXYGEN__)
PWMDriver PWMD3;
#endif

/**
 * @brief   PWMD4 driver identifier.
 * @note    The driver PWMD4 allocates the timer TIM4 when enabled.
 */
#if SPC5_PWM_USE_SMOD3 || defined(__DOXYGEN__)
PWMDriver PWMD4;
#endif

/**
 * @brief   PWMD5 driver identifier.
 * @note    The driver PWMD5 allocates the timer TIM5 when enabled.
 */
#if SPC5_PWM_USE_SMOD4 || defined(__DOXYGEN__)
PWMDriver PWMD5;
#endif

/**
 * @brief   PWMD6 driver identifier.
 * @note    The driver PWMD6 allocates the timer TIM6 when enabled.
 */
#if SPC5_PWM_USE_SMOD5 || defined(__DOXYGEN__)
PWMDriver PWMD6;
#endif

/**
 * @brief   PWMD7 driver identifier.
 * @note    The driver PWMD7 allocates the timer TIM7 when enabled.
 */
#if SPC5_PWM_USE_SMOD6 || defined(__DOXYGEN__)
PWMDriver PWMD7;
#endif

/**
 * @brief   PWMD8 driver identifier.
 * @note    The driver PWMD8 allocates the timer TIM8 when enabled.
 */
#if SPC5_PWM_USE_SMOD7 || defined(__DOXYGEN__)
PWMDriver PWMD8;
#endif

/*===========================================================================*/
/* Driver local variables.                                                   */
/*===========================================================================*/

/**
 * @brief   Number of active FlexPWM0 submodules.
 */
static uint32_t flexpwm_active_submodules0;

/**
 * @brief   Number of active FlexPWM1 submodules.
 */
static uint32_t flexpwm_active_submodules1;

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/**
 * @brief   Configures and activates the PWM peripheral submodule.
 *
 * @param[in] pwmp      pointer to a @p PWMDriver object
 * @param[in] sid    PWM submodule identifier
 *
 * @notapi
 */
void pwm_lld_start_submodule(PWMDriver *pwmp, uint8_t sid) {
  pwmcnt_t pwmperiod;
  uint32_t psc;

  /* Clears Status Register.*/
  pwmp->flexpwmp->SUB[sid].STS.R = 0xFFFF;

  /* Clears LDOK and initializes the registers.*/
  pwmp->flexpwmp->MCTRL.B.CLDOK |= 1U << sid;

  /* Setting PWM clock frequency and submodule prescaler.*/
  psc = SPC5_FLEXPWM0_CLK / pwmp->config->frequency;

  chDbgAssert((psc <= 0xFFFF) &&
              (((psc) * pwmp->config->frequency) == SPC5_FLEXPWM0_CLK) &&
              ((psc == 1) || (psc == 2) || (psc == 4) || (psc == 8) ||
               (psc == 16) || (psc == 32) ||
               (psc == 64) || (psc == 128)),
              "pwm_lld_start_submodule(), #1", "invalid frequency");

  switch (psc) {
  case 1:
    pwmp->flexpwmp->SUB[sid].CTRL.B.PRSC = SPC5_FLEXPWM_PSC_1;
    break;
  case 2:
    pwmp->flexpwmp->SUB[sid].CTRL.B.PRSC = SPC5_FLEXPWM_PSC_2;
    break;
  case 4:
    pwmp->flexpwmp->SUB[sid].CTRL.B.PRSC = SPC5_FLEXPWM_PSC_4;
    break;
  case 8:
    pwmp->flexpwmp->SUB[sid].CTRL.B.PRSC = SPC5_FLEXPWM_PSC_8;
    break;
  case 16:
    pwmp->flexpwmp->SUB[sid].CTRL.B.PRSC = SPC5_FLEXPWM_PSC_16;
    break;
  case 32:
    pwmp->flexpwmp->SUB[sid].CTRL.B.PRSC = SPC5_FLEXPWM_PSC_32;
    break;
  case 64:
    pwmp->flexpwmp->SUB[sid].CTRL.B.PRSC = SPC5_FLEXPWM_PSC_64;
    break;
  case 128:
    pwmp->flexpwmp->SUB[sid].CTRL.B.PRSC = SPC5_FLEXPWM_PSC_128;
    break;
  }

  /* Disables PWM FAULT function. */
  pwmp->flexpwmp->SUB[sid].DISMAP.R = 0;
  pwmp->flexpwmp->SUB[sid].CTRL2.B.INDEP = 1U;

  /* Sets PWM period.*/
  pwmperiod = pwmp->period;
  pwmp->flexpwmp->SUB[sid].INIT.R = ~(pwmperiod / 2) + 1U;
  pwmp->flexpwmp->SUB[sid].VAL[0].R = 0;
  pwmp->flexpwmp->SUB[sid].VAL[1].R = pwmperiod / 2;

  /* Sets the submodule channels.*/
  switch (pwmp->config->mode & PWM_ALIGN_MASK) {
  case PWM_ALIGN_EDGE:
    /* Setting reloads.*/
    pwmp->flexpwmp->SUB[sid].CTRL.B.HALF = 0;
    pwmp->flexpwmp->SUB[sid].CTRL.B.FULL = 1;

    /* Setting active front of PWM channels.*/
    pwmp->flexpwmp->SUB[sid].VAL[2].R = ~(pwmperiod / 2) + 1U;
    pwmp->flexpwmp->SUB[sid].VAL[4].R = ~(pwmperiod / 2) + 1U;
    break;
  case PWM_ALIGN_CENTER:
    /* Setting reloads.*/
    pwmp->flexpwmp->SUB[sid].CTRL.B.HALF = 1;
    pwmp->flexpwmp->SUB[sid].CTRL.B.FULL = 0;
    break;
  default:
    ;
  }

  /* Polarities setup.*/
  switch (pwmp->config->channels[0].mode & PWM_OUTPUT_MASK) {
  case PWM_OUTPUT_ACTIVE_LOW:
    pwmp->flexpwmp->SUB[sid].OCTRL.B.POLA = 1;

    /* Enables CHA mask and CHA.*/
    pwmp->flexpwmp->MASK.B.MASKA |= 1U << sid;
    pwmp->flexpwmp->OUTEN.B.PWMA_EN |= 1U << sid;

    break;
  case PWM_OUTPUT_ACTIVE_HIGH:
    pwmp->flexpwmp->SUB[sid].OCTRL.B.POLA = 0;

    /* Enables CHA mask and CHA.*/
    pwmp->flexpwmp->MASK.B.MASKA |= 1U << sid;
    pwmp->flexpwmp->OUTEN.B.PWMA_EN |= 1U << sid;

    break;
  case PWM_OUTPUT_DISABLED:
    /* Enables CHA mask.*/
    pwmp->flexpwmp->MASK.B.MASKA |= 1U << sid;

    break;
  default:
    ;
  }
  switch (pwmp->config->channels[1].mode & PWM_OUTPUT_MASK) {
  case PWM_OUTPUT_ACTIVE_LOW:
    pwmp->flexpwmp->SUB[sid].OCTRL.B.POLB = 1;

    /* Enables CHB mask and CHB.*/
    pwmp->flexpwmp->MASK.B.MASKB |= 1U << sid;
    pwmp->flexpwmp->OUTEN.B.PWMB_EN |= 1U << sid;

    break;
  case PWM_OUTPUT_ACTIVE_HIGH:
    pwmp->flexpwmp->SUB[sid].OCTRL.B.POLB = 0;

    /* Enables CHB mask and CHB.*/
    pwmp->flexpwmp->MASK.B.MASKB |= 1U << sid;
    pwmp->flexpwmp->OUTEN.B.PWMB_EN |= 1U << sid;

    break;
  case PWM_OUTPUT_DISABLED:
    /* Enables CHB mask.*/
    pwmp->flexpwmp->MASK.B.MASKB |= 1U << sid;

    break;
  default:
    ;
  }

  /* Complementary output setup.*/
  switch (pwmp->config->channels[0].mode & PWM_COMPLEMENTARY_OUTPUT_MASK) {
  case PWM_COMPLEMENTARY_OUTPUT_ACTIVE_LOW:
    chDbgAssert(pwmp->config->channels[1].mode == PWM_OUTPUT_ACTIVE_LOW,
                "pwm_lld_start_submodule(), #2",
                "the PWM chB must be set in PWM_OUTPUT_ACTIVE_LOW");
    pwmp->flexpwmp->SUB[sid].OCTRL.B.POLA = 1;
    pwmp->flexpwmp->SUB[sid].CTRL2.B.INDEP = 0;
    pwmp->flexpwmp->OUTEN.B.PWMA_EN |= 1U << sid;
    break;
  case PWM_COMPLEMENTARY_OUTPUT_ACTIVE_HIGH:
    chDbgAssert(pwmp->config->channels[1].mode == PWM_OUTPUT_ACTIVE_HIGH,
                "pwm_lld_start_submodule(), #3",
                "the PWM chB must be set in PWM_OUTPUT_ACTIVE_HIGH");
    pwmp->flexpwmp->SUB[sid].CTRL2.B.INDEP = 0;
    pwmp->flexpwmp->OUTEN.B.PWMA_EN |= 1U << sid;
    break;
  default:
    ;
  }

  switch (pwmp->config->channels[1].mode & PWM_COMPLEMENTARY_OUTPUT_MASK) {
  case PWM_COMPLEMENTARY_OUTPUT_ACTIVE_LOW:
    chDbgAssert(pwmp->config->channels[0].mode == PWM_OUTPUT_ACTIVE_LOW,
                "pwm_lld_start_submodule(), #4",
                "the PWM chA must be set in PWM_OUTPUT_ACTIVE_LOW");
    pwmp->flexpwmp->SUB[sid].CTRL2.B.INDEP = 0;
    pwmp->flexpwmp->MCTRL.B.IPOL &= ~ (1U << sid);
    pwmp->flexpwmp->SUB[sid].OCTRL.B.POLB = 1;
    pwmp->flexpwmp->OUTEN.B.PWMB_EN |= 1U << sid;
    break;
  case PWM_COMPLEMENTARY_OUTPUT_ACTIVE_HIGH:
    chDbgAssert(pwmp->config->channels[0].mode == PWM_OUTPUT_ACTIVE_HIGH,
                "pwm_lld_start_submodule(), #5",
                "the PWM chA must be set in PWM_OUTPUT_ACTIVE_HIGH");
    pwmp->flexpwmp->SUB[sid].CTRL2.B.INDEP = 0;
    pwmp->flexpwmp->MCTRL.B.IPOL |= 1U << sid;
    pwmp->flexpwmp->OUTEN.B.PWMB_EN |= 1U << sid;
    break;
  default:
    ;
  }

  /* Sets the INIT and MASK registers.*/
  pwmp->flexpwmp->SUB[sid].CTRL2.B.FRCEN = 1U;
  pwmp->flexpwmp->SUB[sid].CTRL2.B.FORCE = 1U;

  /* Updates SMOD registers and starts SMOD.*/
  pwmp->flexpwmp->MCTRL.B.LDOK |= 1U << sid;
  pwmp->flexpwmp->MCTRL.B.RUN |= 1U << sid;
}

/**
 * @brief   Enables a PWM channel of a submodule.
 *
 * @param[in] pwmp      pointer to a @p PWMDriver object
 * @param[in] channel   PWM channel identifier (0...PWM_CHANNELS-1)
 * @param[in] width     PWM pulse width as clock pulses number
 * @param[in] sid       PWM submodule id
 *
 * @notapi
 */
void pwm_lld_enable_submodule_channel(PWMDriver *pwmp,
                                  pwmchannel_t channel,
                                  pwmcnt_t width, uint8_t sid) {
  pwmcnt_t pwmperiod;
  int16_t nwidth;
  pwmperiod = pwmp->period;
  nwidth = width - (pwmperiod / 2);

  /* Clears LDOK.*/
  pwmp->flexpwmp->MCTRL.B.CLDOK |= 1U << sid;

  /* Active the width interrupt.*/
  if (channel == 0) {
    if (pwmp->config->channels[0].callback != NULL) {
      if ((pwmp->flexpwmp->SUB[sid].INTEN.B.CMPIE & 0x08) == 0) {
        pwmp->flexpwmp->SUB[sid].INTEN.B.CMPIE |= 0x08;
      }
    }

    /* Sets the channel width.*/
    switch (pwmp->config->mode & PWM_ALIGN_MASK) {
    case PWM_ALIGN_EDGE:
      if (nwidth >= 0)
        pwmp->flexpwmp->SUB[sid].VAL[3].R = nwidth;
      else
        pwmp->flexpwmp->SUB[sid].VAL[3].R = ~((pwmperiod / 2) - width) + 1U;
      break;
    case PWM_ALIGN_CENTER:
      pwmp->flexpwmp->SUB[sid].VAL[3].R = width / 2;
      pwmp->flexpwmp->SUB[sid].VAL[2].R = ~(width / 2) + 1U;
      break;
    default:
      ;
    }

    /* Removes the channel mask if it is necessary.*/
    if ((pwmp->flexpwmp->MASK.B.MASKA & (1U << sid)) == 1)
      pwmp->flexpwmp->MASK.B.MASKA &= ~ (1U << sid);

    if ((pwmp->config->channels[0].mode & PWM_COMPLEMENTARY_OUTPUT_MASK) != 0) {
      pwmp->flexpwmp->MASK.B.MASKB &= ~ (1U << sid);
    }
  }
  /* Active the width interrupt.*/
  else if (channel == 1) {
    if (pwmp->config->channels[1].callback != NULL) {
      if ((pwmp->flexpwmp->SUB[sid].INTEN.B.CMPIE & 0x20) == 0) {
        pwmp->flexpwmp->SUB[sid].INTEN.B.CMPIE |= 0x20;
      }
    }
    /* Sets the channel width.*/
    switch (pwmp->config->mode & PWM_ALIGN_MASK) {
    case PWM_ALIGN_EDGE:
      if (nwidth >= 0)
        pwmp->flexpwmp->SUB[sid].VAL[5].R = nwidth;
      else
        pwmp->flexpwmp->SUB[sid].VAL[5].R = ~((pwmperiod / 2) - width) + 1U;
      break;
    case PWM_ALIGN_CENTER:
      pwmp->flexpwmp->SUB[sid].VAL[5].R = width / 2;
      pwmp->flexpwmp->SUB[sid].VAL[4].R = ~(width / 2) + 1U;
      break;
    default:
      ;
    }

    /* Removes the channel mask if it is necessary.*/
    if ((pwmp->flexpwmp->MASK.B.MASKB & (1U << sid)) == 1)
      pwmp->flexpwmp->MASK.B.MASKB &= ~ (1U << sid);

    if ((pwmp->config->channels[1].mode & PWM_COMPLEMENTARY_OUTPUT_MASK) != 0) {
      pwmp->flexpwmp->MASK.B.MASKA &= ~ (1U << sid);
    }
  }

  /* Active the periodic interrupt.*/
  if (pwmp->flexpwmp->SUB[sid].INTEN.B.RIE != 1U) {
    if (pwmp->config->callback != NULL) {
      pwmp->flexpwmp->SUB[sid].INTEN.B.RIE = 1;
    }
  }

  /* Sets the MASK registers.*/
  pwmp->flexpwmp->SUB[sid].CTRL2.B.FRCEN = 1U;
  pwmp->flexpwmp->SUB[sid].CTRL2.B.FORCE = 1U;

  /* Forces reload of the VALUE registers.*/
  pwmp->flexpwmp->MCTRL.B.LDOK |= 1U << sid;
}

/**
 * @brief   Disables a PWM channel of a submodule.
 *
 * @param[in] pwmp      pointer to a @p PWMDriver object
 * @param[in] channel   PWM channel identifier (0...PWM_CHANNELS-1)
 * @param[in] sid       PWM submodule id
 *
 * @notapi
 */
void pwm_lld_disable_submodule_channel(PWMDriver *pwmp,
                                   pwmchannel_t channel,
                                   uint8_t sid) {

  pwmp->flexpwmp->MCTRL.B.CLDOK |= 1U << sid;

  /* Disable the width interrupt.*/
  if (channel == 0) {
    if (pwmp->config->channels[0].callback != NULL) {
      if ((pwmp->flexpwmp->SUB[sid].INTEN.B.CMPIE & 0x08) == 1) {
        pwmp->flexpwmp->SUB[sid].INTEN.B.CMPIE &= 0x37;
      }
    }

    /* Active the channel mask.*/
    if ((pwmp->config->channels[0].mode & PWM_COMPLEMENTARY_OUTPUT_MASK) != 0) {
      pwmp->flexpwmp->MASK.B.MASKA |= 1U << sid;
      pwmp->flexpwmp->MASK.B.MASKB |= 1U << sid;
    }
    else
      pwmp->flexpwmp->MASK.B.MASKA |= 1U << sid;
  }
  /* Disable the width interrupt.*/
  else if (channel == 1) {
    if (pwmp->config->channels[1].callback != NULL) {
      if ((pwmp->flexpwmp->SUB[sid].INTEN.B.CMPIE & 0x20) == 1) {
        pwmp->flexpwmp->SUB[sid].INTEN.B.CMPIE &= 0x1F;
      }
    }

    /* Active the channel mask.*/
    if ((pwmp->config->channels[1].mode & PWM_COMPLEMENTARY_OUTPUT_MASK) != 0) {
      pwmp->flexpwmp->MASK.B.MASKA |= 1U << sid;
      pwmp->flexpwmp->MASK.B.MASKB |= 1U << sid;
    }
    else
      pwmp->flexpwmp->MASK.B.MASKB |= 1U << sid;
  }

  /* Sets the MASK registers.*/
  pwmp->flexpwmp->SUB[sid].CTRL2.B.FRCEN = 1U;
  pwmp->flexpwmp->SUB[sid].CTRL2.B.FORCE = 1U;

  /* Disable RIE interrupt to prevent reload interrupt.*/
  if ((pwmp->flexpwmp->MASK.B.MASKA & (1U << sid)) &&
      (pwmp->flexpwmp->MASK.B.MASKB & (1U << sid)) == 1) {
    pwmp->flexpwmp->SUB[sid].INTEN.B.RIE = 0;

    /* Clear the reload flag.*/
    pwmp->flexpwmp->SUB[sid].STS.B.RF = 1U;
    }

  pwmp->flexpwmp->MCTRL.B.LDOK |= (1U << sid);
}

/**
 * @brief   Common SMOD0...SMOD7 IRQ handler.
 * @note    It is assumed that the various sources are only activated if the
 *          associated callback pointer is not equal to @p NULL in order to not
 *          perform an extra check in a potentially critical interrupt handler.
 *
 * @param[in] pwmp      pointer to a @p PWMDriver object
 */
static void pwm_lld_serve_interrupt(PWMDriver *pwmp) {
  uint16_t sr;

#if SPC5_PWM_USE_SMOD0
  if (&PWMD1 == pwmp) {
    sr = pwmp->flexpwmp->SUB[0].STS.R & pwmp->flexpwmp->SUB[0].INTEN.R;
    if ((sr & SPC5_FLEXPWM_STS_CMPF3) != 0) {
      pwmp->flexpwmp->SUB[0].STS.B.CMPF |= 0x08;
      pwmp->config->channels[0].callback(pwmp);
    }
    if ((sr & SPC5_FLEXPWM_STS_CMPF5) != 0) {
      pwmp->flexpwmp->SUB[0].STS.B.CMPF |= 0x20;
      pwmp->config->channels[1].callback(pwmp);
    }
    if ((sr & SPC5_FLEXPWM_STS_RF) != 0) {
      pwmp->flexpwmp->SUB[0].STS.B.RF = 1U;
      pwmp->config->callback(pwmp);
    }
  }
#endif
#if SPC5_PWM_USE_SMOD1
  if (&PWMD2 == pwmp) {
    sr = pwmp->flexpwmp->SUB[1].STS.R & pwmp->flexpwmp->SUB[1].INTEN.R;
    if ((sr & SPC5_FLEXPWM_STS_CMPF3) != 0) {
      pwmp->flexpwmp->SUB[1].STS.B.CMPF |= 0x08;
      pwmp->config->channels[0].callback(pwmp);
    }
    if ((sr & SPC5_FLEXPWM_STS_CMPF5) != 0) {
      pwmp->flexpwmp->SUB[1].STS.B.CMPF |= 0x20;
      pwmp->config->channels[1].callback(pwmp);
    }
    if ((sr & SPC5_FLEXPWM_STS_RF) != 0) {
      pwmp->flexpwmp->SUB[1].STS.B.RF = 1U;
      pwmp->config->callback(pwmp);
    }
  }
#endif
#if SPC5_PWM_USE_SMOD2
  if (&PWMD3 == pwmp) {
    sr = pwmp->flexpwmp->SUB[2].STS.R & pwmp->flexpwmp->SUB[2].INTEN.R;
    if ((sr & SPC5_FLEXPWM_STS_CMPF3) != 0) {
      pwmp->flexpwmp->SUB[2].STS.B.CMPF |= 0x08;
      pwmp->config->channels[0].callback(pwmp);
    }
    if ((sr & SPC5_FLEXPWM_STS_CMPF5) != 0) {
      pwmp->flexpwmp->SUB[2].STS.B.CMPF |= 0x20;
      pwmp->config->channels[1].callback(pwmp);
    }
    if ((sr & SPC5_FLEXPWM_STS_RF) != 0) {
      pwmp->flexpwmp->SUB[2].STS.B.RF = 1U;
      pwmp->config->callback(pwmp);
    }
  }
#endif
#if SPC5_PWM_USE_SMOD3
  if (&PWMD4 == pwmp) {
    sr = pwmp->flexpwmp->SUB[3].STS.R & pwmp->flexpwmp->SUB[3].INTEN.R;
    if ((sr & SPC5_FLEXPWM_STS_CMPF3) != 0) {
      pwmp->flexpwmp->SUB[3].STS.B.CMPF |= 0x08;
      pwmp->config->channels[0].callback(pwmp);
    }
    if ((sr & SPC5_FLEXPWM_STS_CMPF5) != 0) {
      pwmp->flexpwmp->SUB[3].STS.B.CMPF |= 0x20;
      pwmp->config->channels[1].callback(pwmp);
    }
    if ((sr & SPC5_FLEXPWM_STS_RF) != 0) {
      pwmp->flexpwmp->SUB[3].STS.B.RF = 1U;
      pwmp->config->callback(pwmp);
    }
  }
#endif
#if SPC5_PWM_USE_SMOD4
  if (&PWMD5 == pwmp) {
    sr = pwmp->flexpwmp->SUB[0].STS.R & pwmp->flexpwmp->SUB[0].INTEN.R;
    if ((sr & SPC5_FLEXPWM_STS_CMPF3) != 0) {
      pwmp->flexpwmp->SUB[0].STS.B.CMPF |= 0x08;
      pwmp->config->channels[0].callback(pwmp);
    }
    if ((sr & SPC5_FLEXPWM_STS_CMPF5) != 0) {
      pwmp->flexpwmp->SUB[0].STS.B.CMPF |= 0x20;
      pwmp->config->channels[1].callback(pwmp);
    }
    if ((sr & SPC5_FLEXPWM_STS_RF) != 0) {
      pwmp->flexpwmp->SUB[0].STS.B.RF = 1U;
      pwmp->config->callback(pwmp);
    }
  }
#endif
#if SPC5_PWM_USE_SMOD5
  if (&PWMD6 == pwmp) {
    sr = pwmp->flexpwmp->SUB[1].STS.R & pwmp->flexpwmp->SUB[1].INTEN.R;
    if ((sr & SPC5_FLEXPWM_STS_CMPF3) != 0) {
      pwmp->flexpwmp->SUB[1].STS.B.CMPF |= 0x08;
      pwmp->config->channels[0].callback(pwmp);
    }
    if ((sr & SPC5_FLEXPWM_STS_CMPF5) != 0) {
      pwmp->flexpwmp->SUB[1].STS.B.CMPF |= 0x20;
      pwmp->config->channels[1].callback(pwmp);
    }
    if ((sr & SPC5_FLEXPWM_STS_RF) != 0) {
      pwmp->flexpwmp->SUB[1].STS.B.RF = 1U;
      pwmp->config->callback(pwmp);
    }
  }
#endif
#if SPC5_PWM_USE_SMOD6
  if (&PWMD7 == pwmp) {
    sr = pwmp->flexpwmp->SUB[2].STS.R & pwmp->flexpwmp->SUB[2].INTEN.R;
    if ((sr & SPC5_FLEXPWM_STS_CMPF3) != 0) {
      pwmp->flexpwmp->SUB[2].STS.B.CMPF |= 0x08;
      pwmp->config->channels[0].callback(pwmp);
    }
    if ((sr & SPC5_FLEXPWM_STS_CMPF5) != 0) {
      pwmp->flexpwmp->SUB[2].STS.B.CMPF |= 0x20;
      pwmp->config->channels[1].callback(pwmp);
    }
    if ((sr & SPC5_FLEXPWM_STS_RF) != 0) {
      pwmp->flexpwmp->SUB[2].STS.B.RF = 1U;
      pwmp->config->callback(pwmp);
    }
  }
#endif
#if SPC5_PWM_USE_SMOD7
  if (&PWMD8 == pwmp) {
    sr = pwmp->flexpwmp->SUB[3].STS.R & pwmp->flexpwmp->SUB[3].INTEN.R;
    if ((sr & SPC5_FLEXPWM_STS_CMPF3) != 0) {
      pwmp->flexpwmp->SUB[3].STS.B.CMPF |= 0x08;
      pwmp->config->channels[0].callback(pwmp);
    }
    if ((sr & SPC5_FLEXPWM_STS_CMPF5) != 0) {
      pwmp->flexpwmp->SUB[3].STS.B.CMPF |= 0x20;
      pwmp->config->channels[1].callback(pwmp);
    }
    if ((sr & SPC5_FLEXPWM_STS_RF) != 0) {
      pwmp->flexpwmp->SUB[3].STS.B.RF = 1U;
      pwmp->config->callback(pwmp);
    }
  }
#endif
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

#if SPC5_PWM_USE_SMOD0 || defined(__DOXYGEN__)
#if !defined(SPC5_FLEXPWM0_RF0_HANDLER)
#error "SPC5_FLEXPWM0_RF0_HANDLER not defined"
#endif
/**
 * @brief   FlexPWM0-SMOD0 RF0 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(SPC5_FLEXPWM0_RF0_HANDLER) {

  CH_IRQ_PROLOGUE();

  pwm_lld_serve_interrupt(&PWMD1);

  CH_IRQ_EPILOGUE();
}

#if !defined(SPC5_FLEXPWM0_COF0_HANDLER)
#error "SPC5_FLEXPWM0_COF0_HANDLER not defined"
#endif
/**
 * @brief   FlexPWM0-SMOD0 COF0 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(SPC5_FLEXPWM0_COF0_HANDLER) {

  CH_IRQ_PROLOGUE();

  pwm_lld_serve_interrupt(&PWMD1);

  CH_IRQ_EPILOGUE();
}
#endif

#if SPC5_PWM_USE_SMOD1 || defined(__DOXYGEN__)
#if !defined(SPC5_FLEXPWM0_RF1_HANDLER)
#error "SPC5_FLEXPWM0_RF1_HANDLER not defined"
#endif
/**
 * @brief   FlexPWM0-SMOD1 RF1 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(SPC5_FLEXPWM0_RF1_HANDLER) {

  CH_IRQ_PROLOGUE();

  pwm_lld_serve_interrupt(&PWMD2);

  CH_IRQ_EPILOGUE();
}

#if !defined(SPC5_FLEXPWM0_COF1_HANDLER)
#error "SPC5_FLEXPWM0_COF1_HANDLER not defined"
#endif
/**
 * @brief   FlexPWM0-SMOD1 COF1 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(SPC5_FLEXPWM0_COF1_HANDLER) {

  CH_IRQ_PROLOGUE();

  pwm_lld_serve_interrupt(&PWMD2);

  CH_IRQ_EPILOGUE();
}
#endif

#if SPC5_PWM_USE_SMOD2 || defined(__DOXYGEN__)
#if !defined(SPC5_FLEXPWM0_RF2_HANDLER)
#error "SPC5_FLEXPWM0_RF2_HANDLER not defined"
#endif
/**
 * @brief   FlexPWM0-SMOD2 RF2 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(SPC5_FLEXPWM0_RF2_HANDLER) {

  CH_IRQ_PROLOGUE();

  pwm_lld_serve_interrupt(&PWMD3);

  CH_IRQ_EPILOGUE();
}

#if !defined(SPC5_FLEXPWM0_COF2_HANDLER)
#error "SPC5_FLEXPWM0_COF2_HANDLER not defined"
#endif
/**
 * @brief   FlexPWM0-SMOD2 COF2 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(SPC5_FLEXPWM0_COF2_HANDLER) {

  CH_IRQ_PROLOGUE();

  pwm_lld_serve_interrupt(&PWMD3);

  CH_IRQ_EPILOGUE();
}
#endif

#if SPC5_PWM_USE_SMOD3 || defined(__DOXYGEN__)
#if !defined(SPC5_FLEXPWM0_RF3_HANDLER)
#error "SPC5_FLEXPWM0_RF3_HANDLER not defined"
#endif
/**
 * @brief   FlexPWM0-SMOD1 RF3 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(SPC5_FLEXPWM0_RF3_HANDLER) {

  CH_IRQ_PROLOGUE();

  pwm_lld_serve_interrupt(&PWMD4);

  CH_IRQ_EPILOGUE();
}

#if !defined(SPC5_FLEXPWM0_COF3_HANDLER)
#error "SPC5_FLEXPWM0_COF3_HANDLER not defined"
#endif
/**
 * @brief   FlexPWM0-SMOD1 COF3 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(SPC5_FLEXPWM0_COF3_HANDLER) {

  CH_IRQ_PROLOGUE();

  pwm_lld_serve_interrupt(&PWMD4);

  CH_IRQ_EPILOGUE();
}
#endif

#if SPC5_PWM_USE_SMOD4 || defined(__DOXYGEN__)
#if !defined(SPC5_FLEXPWM1_RF0_HANDLER)
#error "SPC5_FLEXPWM0_RF1_HANDLER not defined"
#endif
/**
 * @brief   FlexPWM1-SMOD0 RF0 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(SPC5_FLEXPWM1_RF0_HANDLER) {

  CH_IRQ_PROLOGUE();

  pwm_lld_serve_interrupt(&PWMD5);

  CH_IRQ_EPILOGUE();
}

#if !defined(SPC5_FLEXPWM1_COF0_HANDLER)
#error "SPC5_FLEXPWM1_COF0_HANDLER not defined"
#endif
/**
 * @brief   FlexPWM1-SMOD0 COF0 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(SPC5_FLEXPWM1_COF0_HANDLER) {

  CH_IRQ_PROLOGUE();

  pwm_lld_serve_interrupt(&PWMD5);

  CH_IRQ_EPILOGUE();
}
#endif

#if SPC5_PWM_USE_SMOD5 || defined(__DOXYGEN__)
#if !defined(SPC5_FLEXPWM1_RF1_HANDLER)
#error "SPC5_FLEXPWM1_RF1_HANDLER not defined"
#endif
/**
 * @brief   FlexPWM1-SMOD1 RF1 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(SPC5_FLEXPWM1_RF1_HANDLER) {

  CH_IRQ_PROLOGUE();

  pwm_lld_serve_interrupt(&PWMD6);

  CH_IRQ_EPILOGUE();
}

#if !defined(SPC5_FLEXPWM1_COF1_HANDLER)
#error "SPC5_FLEXPWM1_COF1_HANDLER not defined"
#endif
/**
 * @brief   FlexPWM1-SMOD1 COF1 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(SPC5_FLEXPWM1_COF1_HANDLER) {

  CH_IRQ_PROLOGUE();

  pwm_lld_serve_interrupt(&PWMD6);

  CH_IRQ_EPILOGUE();
}
#endif

#if SPC5_PWM_USE_SMOD6 || defined(__DOXYGEN__)
#if !defined(SPC5_FLEXPWM1_RF2_HANDLER)
#error "SPC5_FLEXPWM1_RF2_HANDLER not defined"
#endif
/**
 * @brief   FlexPWM1-SMOD2 RF2 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(SPC5_FLEXPWM1_RF2_HANDLER) {

  CH_IRQ_PROLOGUE();

  pwm_lld_serve_interrupt(&PWMD7);

  CH_IRQ_EPILOGUE();
}

#if !defined(SPC5_FLEXPWM1_COF2_HANDLER)
#error "SPC5_FLEXPWM1_COF2_HANDLER not defined"
#endif
/**
 * @brief   FlexPWM1-SMOD2 COF2 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(SPC5_FLEXPWM1_COF2_HANDLER) {

  CH_IRQ_PROLOGUE();

  pwm_lld_serve_interrupt(&PWMD7);

  CH_IRQ_EPILOGUE();
}
#endif

#if SPC5_PWM_USE_SMOD7 || defined(__DOXYGEN__)
#if !defined(SPC5_FLEXPWM1_RF3_HANDLER)
#error "SPC5_FLEXPWM1_RF3_HANDLER not defined"
#endif
/**
 * @brief   FlexPWM1-SMOD3 RF3 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(SPC5_FLEXPWM1_RF3_HANDLER) {

  CH_IRQ_PROLOGUE();

  pwm_lld_serve_interrupt(&PWMD8);

  CH_IRQ_EPILOGUE();
}

#if !defined(SPC5_FLEXPWM1_COF3_HANDLER)
#error "SPC5_FLEXPWM1_COF3_HANDLER not defined"
#endif
/**
 * @brief   FlexPWM1-SMOD3 COF3 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(SPC5_FLEXPWM1_COF3_HANDLER) {

  CH_IRQ_PROLOGUE();

  pwm_lld_serve_interrupt(&PWMD8);

  CH_IRQ_EPILOGUE();
}
#endif

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level PWM driver initialization.
 *
 * @notapi
 */
void pwm_lld_init(void) {

  /* FlexPWM initially all not in use.*/
  flexpwm_active_submodules0 = 0;
  flexpwm_active_submodules1 = 0;

#if (SPC5_PWM_USE_SMOD0)
  /* Driver initialization.*/
  pwmObjectInit(&PWMD1);
  PWMD1.flexpwmp = &SPC5_FLEXPWM_0;
  INTC.PSR[SPC5_FLEXPWM0_RF0_NUMBER].R = SPC5_PWM_SMOD0_PRIORITY;
  INTC.PSR[SPC5_FLEXPWM0_COF0_NUMBER].R = SPC5_PWM_SMOD0_PRIORITY;
  INTC.PSR[SPC5_FLEXPWM0_CAF0_NUMBER].R = SPC5_PWM_SMOD0_PRIORITY;
  INTC.PSR[SPC5_FLEXPWM0_FFLAG_NUMBER].R = SPC5_PWM_SMOD0_PRIORITY;
  INTC.PSR[SPC5_FLEXPWM0_REF_NUMBER].R = SPC5_PWM_SMOD0_PRIORITY;
#endif

#if (SPC5_PWM_USE_SMOD1)
  /* Driver initialization.*/
  pwmObjectInit(&PWMD2);
  PWMD2.flexpwmp = &SPC5_FLEXPWM_0;
  INTC.PSR[SPC5_FLEXPWM0_RF1_NUMBER].R = SPC5_PWM_SMOD1_PRIORITY;
  INTC.PSR[SPC5_FLEXPWM0_COF1_NUMBER].R = SPC5_PWM_SMOD1_PRIORITY;
  INTC.PSR[SPC5_FLEXPWM0_CAF1_NUMBER].R = SPC5_PWM_SMOD1_PRIORITY;
  INTC.PSR[SPC5_FLEXPWM0_FFLAG_NUMBER].R = SPC5_PWM_SMOD1_PRIORITY;
  INTC.PSR[SPC5_FLEXPWM0_REF_NUMBER].R = SPC5_PWM_SMOD1_PRIORITY;
#endif

#if (SPC5_PWM_USE_SMOD2)
  /* Driver initialization.*/
  pwmObjectInit(&PWMD3);
  PWMD3.flexpwmp = &SPC5_FLEXPWM_0;
  INTC.PSR[SPC5_FLEXPWM0_RF2_NUMBER].R = SPC5_PWM_SMOD2_PRIORITY;
  INTC.PSR[SPC5_FLEXPWM0_COF2_NUMBER].R = SPC5_PWM_SMOD2_PRIORITY;
  INTC.PSR[SPC5_FLEXPWM0_CAF2_NUMBER].R = SPC5_PWM_SMOD2_PRIORITY;
  INTC.PSR[SPC5_FLEXPWM0_FFLAG_NUMBER].R = SPC5_PWM_SMOD2_PRIORITY;
  INTC.PSR[SPC5_FLEXPWM0_REF_NUMBER].R = SPC5_PWM_SMOD2_PRIORITY;
#endif

#if (SPC5_PWM_USE_SMOD3)
  /* Driver initialization.*/
  pwmObjectInit(&PWMD4);
  PWMD4.flexpwmp = &SPC5_FLEXPWM_0;
  INTC.PSR[SPC5_FLEXPWM0_RF3_NUMBER].R = SPC5_PWM_SMOD3_PRIORITY;
  INTC.PSR[SPC5_FLEXPWM0_COF3_NUMBER].R = SPC5_PWM_SMOD3_PRIORITY;
  INTC.PSR[SPC5_FLEXPWM0_CAF3_NUMBER].R = SPC5_PWM_SMOD3_PRIORITY;
  INTC.PSR[SPC5_FLEXPWM0_FFLAG_NUMBER].R = SPC5_PWM_SMOD3_PRIORITY;
  INTC.PSR[SPC5_FLEXPWM0_REF_NUMBER].R = SPC5_PWM_SMOD3_PRIORITY;
#endif

#if (SPC5_PWM_USE_SMOD4)
  /* Driver initialization.*/
  pwmObjectInit(&PWMD5);
  PWMD5.flexpwmp = &SPC5_FLEXPWM_1;
  INTC.PSR[SPC5_FLEXPWM1_RF0_NUMBER].R = SPC5_PWM_SMOD4_PRIORITY;
  INTC.PSR[SPC5_FLEXPWM1_COF0_NUMBER].R = SPC5_PWM_SMOD4_PRIORITY;
  INTC.PSR[SPC5_FLEXPWM1_CAF0_NUMBER].R = SPC5_PWM_SMOD4_PRIORITY;
  INTC.PSR[SPC5_FLEXPWM1_FFLAG_NUMBER].R = SPC5_PWM_SMOD4_PRIORITY;
  INTC.PSR[SPC5_FLEXPWM1_REF_NUMBER].R = SPC5_PWM_SMOD4_PRIORITY;
#endif

#if (SPC5_PWM_USE_SMOD5)
  /* Driver initialization.*/
  pwmObjectInit(&PWMD6);
  PWMD6.flexpwmp = &SPC5_FLEXPWM_1;
  INTC.PSR[SPC5_FLEXPWM1_RF1_NUMBER].R = SPC5_PWM_SMOD5_PRIORITY;
  INTC.PSR[SPC5_FLEXPWM1_COF1_NUMBER].R = SPC5_PWM_SMOD5_PRIORITY;
  INTC.PSR[SPC5_FLEXPWM1_CAF1_NUMBER].R = SPC5_PWM_SMOD5_PRIORITY;
  INTC.PSR[SPC5_FLEXPWM1_FFLAG_NUMBER].R = SPC5_PWM_SMOD5_PRIORITY;
  INTC.PSR[SPC5_FLEXPWM1_REF_NUMBER].R = SPC5_PWM_SMOD5_PRIORITY;
#endif

#if (SPC5_PWM_USE_SMOD6)
  /* Driver initialization.*/
  pwmObjectInit(&PWMD7);
  PWMD7.flexpwmp = &SPC5_FLEXPWM_1;
  INTC.PSR[SPC5_FLEXPWM1_RF2_NUMBER].R = SPC5_PWM_SMOD6_PRIORITY;
  INTC.PSR[SPC5_FLEXPWM1_COF2_NUMBER].R = SPC5_PWM_SMOD6_PRIORITY;
  INTC.PSR[SPC5_FLEXPWM1_CAF2_NUMBER].R = SPC5_PWM_SMOD6_PRIORITY;
  INTC.PSR[SPC5_FLEXPWM1_FFLAG_NUMBER].R = SPC5_PWM_SMOD6_PRIORITY;
  INTC.PSR[SPC5_FLEXPWM1_REF_NUMBER].R = SPC5_PWM_SMOD6_PRIORITY;
#endif

#if (SPC5_PWM_USE_SMOD7)
  /* Driver initialization.*/
  pwmObjectInit(&PWMD8);
  PWMD8.flexpwmp = &SPC5_FLEXPWM_1;
  INTC.PSR[SPC5_FLEXPWM1_RF3_NUMBER].R = SPC5_PWM_SMOD7_PRIORITY;
  INTC.PSR[SPC5_FLEXPWM1_COF3_NUMBER].R = SPC5_PWM_SMOD7_PRIORITY;
  INTC.PSR[SPC5_FLEXPWM1_CAF3_NUMBER].R = SPC5_PWM_SMOD7_PRIORITY;
  INTC.PSR[SPC5_FLEXPWM1_FFLAG_NUMBER].R = SPC5_PWM_SMOD7_PRIORITY;
  INTC.PSR[SPC5_FLEXPWM1_REF_NUMBER].R = SPC5_PWM_SMOD7_PRIORITY;
#endif
}

/**
 * @brief   Configures and activates the PWM peripheral.
 * @note    Starting a driver that is already in the @p PWM_READY state
 *          disables all the active channels.
 *
 * @param[in] pwmp      pointer to a @p PWMDriver object
 *
 * @notapi
 */
void pwm_lld_start(PWMDriver *pwmp) {

  chDbgAssert(flexpwm_active_submodules0 < 5,
              "pwm_lld_start(), #1", "too many submodules");
  chDbgAssert(flexpwm_active_submodules1 < 5,
              "pwm_lld_start(), #2", "too many submodules");

  if (pwmp->state == PWM_STOP) {
#if SPC5_PWM_USE_SMOD0
    if (&PWMD1 == pwmp) {
      flexpwm_active_submodules0++;
    }
#endif /* SPC5_PWM_USE_SMOD0 */

#if SPC5_PWM_USE_SMOD1
    if (&PWMD2 == pwmp) {
      flexpwm_active_submodules0++;
    }
#endif /* SPC5_PWM_USE_SMOD1 */

#if SPC5_PWM_USE_SMOD2
    if (&PWMD3 == pwmp) {
      flexpwm_active_submodules0++;
    }
#endif /* SPC5_PWM_USE_SMOD2 */

#if SPC5_PWM_USE_SMOD3
    if (&PWMD4 == pwmp) {
      flexpwm_active_submodules0++;
    }
#endif /* SPC5_PWM_USE_SMOD3 */

#if SPC5_PWM_USE_SMOD4
    if (&PWMD5 == pwmp) {
      flexpwm_active_submodules1++;
    }
#endif /* SPC5_PWM_USE_SMOD4 */

#if SPC5_PWM_USE_SMOD5
    if (&PWMD6 == pwmp) {
      flexpwm_active_submodules1++;
    }
#endif /* SPC5_PWM_USE_SMOD5 */

#if SPC5_PWM_USE_SMOD6
    if (&PWMD7 == pwmp) {
      flexpwm_active_submodules1++;
    }
#endif /* SPC5_PWM_USE_SMOD6 */

#if SPC5_PWM_USE_SMOD7
    if (&PWMD8 == pwmp) {
      flexpwm_active_submodules1++;
    }
#endif /* SPC5_PWM_USE_SMOD7 */

    /**
     * If this is the first FlexPWM0 submodule
     * activated then the FlexPWM0 is enabled.
     */
#if SPC5_PWM_USE_FLEXPWM0
    /* Set Peripheral Clock.*/
    if (flexpwm_active_submodules0 == 1) {
      halSPCSetPeripheralClockMode(SPC5_FLEXPWM0_PCTL,
          SPC5_PWM_FLEXPWM0_START_PCTL);
    }
#endif

#if SPC5_PWM_USE_FLEXPWM1
    /* Set Peripheral Clock.*/
    if (flexpwm_active_submodules1 == 1) {
      halSPCSetPeripheralClockMode(SPC5_FLEXPWM1_PCTL,
          SPC5_PWM_FLEXPWM1_START_PCTL);
    }
#endif

#if SPC5_PWM_USE_SMOD0
    if (&PWMD1 == pwmp) {
      pwm_lld_start_submodule(pwmp, 0);
    }
#endif
#if SPC5_PWM_USE_SMOD1
    if (&PWMD2 == pwmp) {
      pwm_lld_start_submodule(pwmp, 1);
    }
#endif
#if SPC5_PWM_USE_SMOD2
    if (&PWMD3 == pwmp) {
      pwm_lld_start_submodule(pwmp, 2);
    }
#endif
#if SPC5_PWM_USE_SMOD3
    if (&PWMD4 == pwmp) {
      pwm_lld_start_submodule(pwmp, 3);
    }
#endif
#if SPC5_PWM_USE_SMOD4
    if (&PWMD5 == pwmp) {
      pwm_lld_start_submodule(pwmp, 0);
    }
#endif
#if SPC5_PWM_USE_SMOD5
    if (&PWMD6 == pwmp) {
      pwm_lld_start_submodule(pwmp, 1);
    }
#endif
#if SPC5_PWM_USE_SMOD6
    if (&PWMD7 == pwmp) {
      pwm_lld_start_submodule(pwmp, 2);
    }
#endif
#if SPC5_PWM_USE_SMOD7
    if (&PWMD8 == pwmp) {
      pwm_lld_start_submodule(pwmp, 3);
    }
#endif
  }
  else {
    /* Driver re-configuration scenario, it must be stopped first.*/
#if SPC5_PWM_USE_SMOD0
    if (&PWMD1 == pwmp) {
      /* Disable the interrupts.*/
      pwmp->flexpwmp->SUB[0].INTEN.R = 0;

      /* Disable the submodule.*/
      pwmp->flexpwmp->OUTEN.B.PWMA_EN &= 0xE;
      pwmp->flexpwmp->OUTEN.B.PWMB_EN &= 0xE;

      /* Active the submodule masks.*/
      pwmp->flexpwmp->MASK.B.MASKA &= 0xE;
      pwmp->flexpwmp->MASK.B.MASKB &= 0xE;

      /* Sets the MASK registers.*/
      pwmp->flexpwmp->SUB[0].CTRL2.B.FRCEN = 1U;
      pwmp->flexpwmp->SUB[0].CTRL2.B.FORCE = 1U;
    }
#endif
#if SPC5_PWM_USE_SMOD1
    if (&PWMD2 == pwmp) {
      /* Disable the interrupts.*/
      pwmp->flexpwmp->SUB[1].INTEN.R = 0;

      /* Disable the submodule.*/
      pwmp->flexpwmp->OUTEN.B.PWMA_EN &= 0xD;
      pwmp->flexpwmp->OUTEN.B.PWMB_EN &= 0xD;

      /* Active the submodule masks.*/
      pwmp->flexpwmp->MASK.B.MASKA &= 0xD;
      pwmp->flexpwmp->MASK.B.MASKB &= 0xD;

      /* Sets the MASK registers.*/
      pwmp->flexpwmp->SUB[1].CTRL2.B.FRCEN = 1U;
      pwmp->flexpwmp->SUB[1].CTRL2.B.FORCE = 1U;
    }
#endif
#if SPC5_PWM_USE_SMOD2
    if (&PWMD3 == pwmp) {
      /* Disable the interrupts.*/
      pwmp->flexpwmp->SUB[2].INTEN.R = 0;

      /* Disable the submodule.*/
      pwmp->flexpwmp->OUTEN.B.PWMA_EN &= 0xB;
      pwmp->flexpwmp->OUTEN.B.PWMB_EN &= 0xB;

      /* Active the submodule masks.*/
      pwmp->flexpwmp->MASK.B.MASKA &= 0xB;
      pwmp->flexpwmp->MASK.B.MASKB &= 0xB;

      /* Sets the MASK registers.*/
      pwmp->flexpwmp->SUB[2].CTRL2.B.FRCEN = 1U;
      pwmp->flexpwmp->SUB[2].CTRL2.B.FORCE = 1U;
    }
#endif
#if SPC5_PWM_USE_SMOD3
    if (&PWMD4 == pwmp) {
      /* Disable the interrupts.*/
      pwmp->flexpwmp->SUB[3].INTEN.R = 0;

      /* Disable the submodule.*/
      pwmp->flexpwmp->OUTEN.B.PWMA_EN &= 0x7;
      pwmp->flexpwmp->OUTEN.B.PWMB_EN &= 0x7;

      /* Active the submodule masks.*/
      pwmp->flexpwmp->MASK.B.MASKA &= 0x7;
      pwmp->flexpwmp->MASK.B.MASKB &= 0x7;

      /* Sets the MASK registers.*/
      pwmp->flexpwmp->SUB[3].CTRL2.B.FRCEN = 1U;
      pwmp->flexpwmp->SUB[3].CTRL2.B.FORCE = 1U;
    }
#endif
#if SPC5_PWM_USE_SMOD4
    if (&PWMD5 == pwmp) {
      /* Disable the interrupts.*/
      pwmp->flexpwmp->SUB[0].INTEN.R = 0;

      /* Disable the submodule.*/
      pwmp->flexpwmp->OUTEN.B.PWMA_EN &= 0xE;
      pwmp->flexpwmp->OUTEN.B.PWMB_EN &= 0xE;

      /* Active the submodule masks.*/
      pwmp->flexpwmp->MASK.B.MASKA &= 0xE;
      pwmp->flexpwmp->MASK.B.MASKB &= 0xE;

      /* Sets the MASK registers.*/
      pwmp->flexpwmp->SUB[0].CTRL2.B.FRCEN = 1U;
      pwmp->flexpwmp->SUB[0].CTRL2.B.FORCE = 1U;
    }
#endif
#if SPC5_PWM_USE_SMOD5
    if (&PWMD6 == pwmp) {
      /* Disable the interrupts.*/
      pwmp->flexpwmp->SUB[1].INTEN.R = 0;

      /* Disable the submodule.*/
      pwmp->flexpwmp->OUTEN.B.PWMA_EN &= 0xD;
      pwmp->flexpwmp->OUTEN.B.PWMB_EN &= 0xD;

      /* Active the submodule masks.*/
      pwmp->flexpwmp->MASK.B.MASKA &= 0xD;
      pwmp->flexpwmp->MASK.B.MASKB &= 0xD;

      /* Sets the MASK registers.*/
      pwmp->flexpwmp->SUB[1].CTRL2.B.FRCEN = 1U;
      pwmp->flexpwmp->SUB[1].CTRL2.B.FORCE = 1U;
    }
#endif
#if SPC5_PWM_USE_SMOD6
    if (&PWMD7 == pwmp) {
      /* Disable the interrupts.*/
      pwmp->flexpwmp->SUB[2].INTEN.R = 0;

      /* Disable the submodule.*/
      pwmp->flexpwmp->OUTEN.B.PWMA_EN &= 0xB;
      pwmp->flexpwmp->OUTEN.B.PWMB_EN &= 0xB;

      /* Active the submodule masks.*/
      pwmp->flexpwmp->MASK.B.MASKA &= 0xB;
      pwmp->flexpwmp->MASK.B.MASKB &= 0xB;

      /* Sets the MASK registers.*/
      pwmp->flexpwmp->SUB[2].CTRL2.B.FRCEN = 1U;
      pwmp->flexpwmp->SUB[2].CTRL2.B.FORCE = 1U;
    }
#endif
#if SPC5_PWM_USE_SMOD7
    if (&PWMD8 == pwmp) {
      /* Disable the interrupts.*/
      pwmp->flexpwmp->SUB[3].INTEN.R = 0;

      /* Disable the submodule.*/
      pwmp->flexpwmp->OUTEN.B.PWMA_EN &= 0x7;
      pwmp->flexpwmp->OUTEN.B.PWMB_EN &= 0x7;

      /* Active the submodule masks.*/
      pwmp->flexpwmp->MASK.B.MASKA &= 0x7;
      pwmp->flexpwmp->MASK.B.MASKB &= 0x7;

      /* Sets the MASK registers.*/
      pwmp->flexpwmp->SUB[3].CTRL2.B.FRCEN = 1U;
      pwmp->flexpwmp->SUB[3].CTRL2.B.FORCE = 1U;
    }
#endif
  }
}

/**
 * @brief   Deactivates the PWM peripheral.
 *
 * @param[in] pwmp      pointer to a @p PWMDriver object
 *
 * @notapi
 */
void pwm_lld_stop(PWMDriver *pwmp) {

  chDbgAssert(flexpwm_active_submodules0 < 5,
              "pwm_lld_stop(), #1", "too many submodules");
  chDbgAssert(flexpwm_active_submodules1 < 5,
              "pwm_lld_stop(), #2", "too many submodules");

  /* If in ready state then disables the PWM clock.*/
  if (pwmp->state == PWM_READY) {

#if SPC5_PWM_USE_SMOD0
    if (&PWMD1 == pwmp) {
      flexpwm_active_submodules0--;
    }
#endif /* SPC5_PWM_USE_SMOD0 */

#if SPC5_PWM_USE_SMOD1
    if (&PWMD2 == pwmp) {
      flexpwm_active_submodules0--;
    }
#endif /* SPC5_PWM_USE_SMOD1 */

#if SPC5_PWM_USE_SMOD2
    if (&PWMD3 == pwmp) {
      flexpwm_active_submodules0--;
    }
#endif /* SPC5_PWM_USE_SMOD2 */

#if SPC5_PWM_USE_SMOD3
    if (&PWMD4 == pwmp) {
      flexpwm_active_submodules0--;
    }
#endif /* SPC5_PWM_USE_SMOD3 */

#if SPC5_PWM_USE_SMOD4
    if (&PWMD5 == pwmp) {
      flexpwm_active_submodules1--;
    }
#endif /* SPC5_PWM_USE_SMOD4 */

#if SPC5_PWM_USE_SMOD5
    if (&PWMD6 == pwmp) {
      flexpwm_active_submodules1--;
    }
#endif /* SPC5_PWM_USE_SMOD5 */

#if SPC5_PWM_USE_SMOD6
    if (&PWMD7 == pwmp) {
      flexpwm_active_submodules1--;
    }
#endif /* SPC5_PWM_USE_SMOD6 */

#if SPC5_PWM_USE_SMOD7
    if (&PWMD8 == pwmp) {
      flexpwm_active_submodules1--;
    }
#endif /* SPC5_PWM_USE_SMOD7 */

#if SPC5_PWM_USE_SMOD0
    if (&PWMD1 == pwmp) {
      /* SMOD stop.*/
      pwmp->flexpwmp->MCTRL.B.CLDOK |= 1U;
      pwmp->flexpwmp->SUB[0].INTEN.R = 0;
      pwmp->flexpwmp->SUB[0].STS.R = 0xFFFF;
      pwmp->flexpwmp->OUTEN.B.PWMA_EN &= 0xE;
      pwmp->flexpwmp->OUTEN.B.PWMB_EN &= 0xE;

      pwmp->flexpwmp->MCTRL.B.RUN &= 0xE;
    }
#endif
#if SPC5_PWM_USE_SMOD1
    if (&PWMD2 == pwmp) {
      /* SMOD stop.*/
      pwmp->flexpwmp->MCTRL.B.CLDOK |= 2U;
      pwmp->flexpwmp->SUB[1].INTEN.R = 0;
      pwmp->flexpwmp->SUB[1].STS.R = 0xFFFF;
      pwmp->flexpwmp->OUTEN.B.PWMA_EN &= 0xD;
      pwmp->flexpwmp->OUTEN.B.PWMB_EN &= 0xD;

      pwmp->flexpwmp->MCTRL.B.RUN &= 0xD;
    }
#endif
#if SPC5_PWM_USE_SMOD2
    if (&PWMD3 == pwmp) {
      /* SMOD stop.*/
      pwmp->flexpwmp->MCTRL.B.CLDOK |= 4U;
      pwmp->flexpwmp->SUB[2].INTEN.R = 0;
      pwmp->flexpwmp->SUB[2].STS.R = 0xFFFF;
      pwmp->flexpwmp->OUTEN.B.PWMA_EN &= 0xB;
      pwmp->flexpwmp->OUTEN.B.PWMB_EN &= 0xB;

      pwmp->flexpwmp->MCTRL.B.RUN &= 0xB;
    }
#endif
#if SPC5_PWM_USE_SMOD3
    if (&PWMD4 == pwmp) {
      /* SMOD stop.*/
      pwmp->flexpwmp->MCTRL.B.CLDOK |= 8U;
      pwmp->flexpwmp->SUB[3].INTEN.R = 0;
      pwmp->flexpwmp->SUB[3].STS.R = 0xFFFF;
      pwmp->flexpwmp->OUTEN.B.PWMA_EN &= 0x7;
      pwmp->flexpwmp->OUTEN.B.PWMB_EN &= 0x7;

      pwmp->flexpwmp->MCTRL.B.RUN &= 0x7;
    }
#endif
#if SPC5_PWM_USE_SMOD4
    if (&PWMD5 == pwmp) {
      /* SMOD stop.*/
      pwmp->flexpwmp->MCTRL.B.CLDOK |= 1U;
      pwmp->flexpwmp->SUB[0].INTEN.R = 0;
      pwmp->flexpwmp->SUB[0].STS.R = 0xFFFF;
      pwmp->flexpwmp->OUTEN.B.PWMA_EN &= 0xE;
      pwmp->flexpwmp->OUTEN.B.PWMB_EN &= 0xE;

      pwmp->flexpwmp->MCTRL.B.RUN &= 0xE;
    }
#endif
#if SPC5_PWM_USE_SMOD5
    if (&PWMD6 == pwmp) {
      /* SMOD stop.*/
      pwmp->flexpwmp->MCTRL.B.CLDOK |= 2U;
      pwmp->flexpwmp->SUB[1].INTEN.R = 0;
      pwmp->flexpwmp->SUB[1].STS.R = 0xFFFF;
      pwmp->flexpwmp->OUTEN.B.PWMA_EN &= 0xD;
      pwmp->flexpwmp->OUTEN.B.PWMB_EN &= 0xD;

      pwmp->flexpwmp->MCTRL.B.RUN &= 0xD;
    }
#endif
#if SPC5_PWM_USE_SMOD6
    if (&PWMD7 == pwmp) {
      /* SMOD stop.*/
      pwmp->flexpwmp->MCTRL.B.CLDOK |= 4U;
      pwmp->flexpwmp->SUB[2].INTEN.R = 0;
      pwmp->flexpwmp->SUB[2].STS.R = 0xFFFF;
      pwmp->flexpwmp->OUTEN.B.PWMA_EN &= 0xB;
      pwmp->flexpwmp->OUTEN.B.PWMB_EN &= 0xB;

      pwmp->flexpwmp->MCTRL.B.RUN &= 0xB;
    }
#endif
#if SPC5_PWM_USE_SMOD7
    if (&PWMD8 == pwmp) {
      /* SMOD stop.*/
      pwmp->flexpwmp->MCTRL.B.CLDOK |= 8U;
      pwmp->flexpwmp->SUB[3].INTEN.R = 0;
      pwmp->flexpwmp->SUB[3].STS.R = 0xFFFF;
      pwmp->flexpwmp->OUTEN.B.PWMA_EN &= 0x7;
      pwmp->flexpwmp->OUTEN.B.PWMB_EN &= 0x7;

      pwmp->flexpwmp->MCTRL.B.RUN &= 0x7;
    }
#endif

#if SPC5_PWM_USE_FLEXPWM0
    /* Disable peripheral clock if there is not an activated module.*/
    if (flexpwm_active_submodules0 == 0) {
      halSPCSetPeripheralClockMode(SPC5_FLEXPWM0_PCTL,
                                   SPC5_PWM_FLEXPWM0_STOP_PCTL);
    }
#endif

#if SPC5_PWM_USE_FLEXPWM1
    /* Disable peripheral clock if there is not an activated module.*/
    if (flexpwm_active_submodules1 == 0) {
      halSPCSetPeripheralClockMode(SPC5_FLEXPWM1_PCTL,
                                   SPC5_PWM_FLEXPWM1_STOP_PCTL);
    }
#endif

  }
}

/**
 * @brief   Enables a PWM channel.
 * @pre     The PWM unit must have been activated using @p pwmStart().
 * @post    The channel is active using the specified configuration.
 * @note    The function has effect at the next cycle start.
 *
 * @param[in] pwmp      pointer to a @p PWMDriver object
 * @param[in] channel   PWM channel identifier (0...PWM_CHANNELS-1)
 * @param[in] width     PWM pulse width as clock pulses number
 *
 * @notapi
 */
void pwm_lld_enable_channel(PWMDriver *pwmp,
                            pwmchannel_t channel,
                            pwmcnt_t width) {

#if SPC5_PWM_USE_SMOD0
  if (&PWMD1 == pwmp) {
    pwm_lld_enable_submodule_channel(pwmp, channel, width, 0);
  }
#endif
#if SPC5_PWM_USE_SMOD1
  if (&PWMD2 == pwmp) {
    pwm_lld_enable_submodule_channel(pwmp, channel, width, 1);
  }
#endif
#if SPC5_PWM_USE_SMOD2
  if (&PWMD3 == pwmp) {
    pwm_lld_enable_submodule_channel(pwmp, channel, width, 2);
  }
#endif
#if SPC5_PWM_USE_SMOD3
  if (&PWMD4 == pwmp) {
    pwm_lld_enable_submodule_channel(pwmp, channel, width, 3);
  }
#endif
#if SPC5_PWM_USE_SMOD4
  if (&PWMD5 == pwmp) {
    pwm_lld_enable_submodule_channel(pwmp, channel, width, 0);
  }
#endif
#if SPC5_PWM_USE_SMOD5
  if (&PWMD6 == pwmp) {
    pwm_lld_enable_submodule_channel(pwmp, channel, width, 1);
  }
#endif
#if SPC5_PWM_USE_SMOD6
  if (&PWMD7 == pwmp) {
    pwm_lld_enable_submodule_channel(pwmp, channel, width, 2);
  }
#endif
#if SPC5_PWM_USE_SMOD7
  if (&PWMD8 == pwmp) {
    pwm_lld_enable_submodule_channel(pwmp, channel, width, 3);
  }
#endif
}

/**
 * @brief   Disables a PWM channel.
 * @pre     The PWM unit must have been activated using @p pwmStart().
 * @post    The channel is disabled and its output line returned to the
 *          idle state.
 * @note    The function has effect at the next cycle start.
 *
 * @param[in] pwmp      pointer to a @p PWMDriver object
 * @param[in] channel   PWM channel identifier (0...PWM_CHANNELS-1)
 *
 * @notapi
 */
void pwm_lld_disable_channel(PWMDriver *pwmp, pwmchannel_t channel) {

#if SPC5_PWM_USE_SMOD0
  if (&PWMD1 == pwmp) {
    pwm_lld_disable_submodule_channel(pwmp, channel, 0);
  }
#endif
#if SPC5_PWM_USE_SMOD1
  if (&PWMD2 == pwmp) {
    pwm_lld_disable_submodule_channel(pwmp, channel, 1);
  }
#endif
#if SPC5_PWM_USE_SMOD2
  if (&PWMD3 == pwmp) {
    pwm_lld_disable_submodule_channel(pwmp, channel, 2);
  }
#endif
#if SPC5_PWM_USE_SMOD3
  if (&PWMD4 == pwmp) {
    pwm_lld_disable_submodule_channel(pwmp, channel, 3);
  }
#endif
#if SPC5_PWM_USE_SMOD4
  if (&PWMD5 == pwmp) {
    pwm_lld_disable_submodule_channel(pwmp, channel, 0);
  }
#endif
#if SPC5_PWM_USE_SMOD5
  if (&PWMD6 == pwmp) {
    pwm_lld_disable_submodule_channel(pwmp, channel, 1);
  }
#endif
#if SPC5_PWM_USE_SMOD6
  if (&PWMD7 == pwmp) {
    pwm_lld_disable_submodule_channel(pwmp, channel, 2);
  }
#endif
#if SPC5_PWM_USE_SMOD7
  if (&PWMD8 == pwmp) {
    pwm_lld_disable_submodule_channel(pwmp, channel, 3);
  }
#endif
}

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
void pwm_lld_change_period(PWMDriver *pwmp, pwmcnt_t period) {

  pwmcnt_t pwmperiod = period;
#if SPC5_PWM_USE_SMOD0
  if (&PWMD1 == pwmp) {
    pwmp->flexpwmp->MCTRL.B.CLDOK |= 1U;

    /* Setting PWM period.*/
    pwmp->flexpwmp->SUB[0].INIT.R = ~(pwmperiod / 2) + 1U;
    pwmp->flexpwmp->SUB[0].VAL[0].R = 0;
    pwmp->flexpwmp->SUB[0].VAL[1].R = pwmperiod / 2;

    switch (pwmp->config->mode & PWM_ALIGN_MASK) {
    case PWM_ALIGN_EDGE:
      /* Setting active front of PWM channels.*/
      pwmp->flexpwmp->SUB[0].VAL[2].R = ~(pwmperiod / 2) + 1U;
      pwmp->flexpwmp->SUB[0].VAL[4].R = ~(pwmperiod / 2) + 1U;
      break;
    default:
      ;
    }
    pwmp->flexpwmp->MCTRL.B.LDOK |= 1U;
  }
#endif
#if SPC5_PWM_USE_SMOD1
  if (&PWMD2 == pwmp) {
    pwmp->flexpwmp->MCTRL.B.CLDOK |= 2U;

    /* Setting PWM period.*/
    pwmp->flexpwmp->SUB[1].INIT.R = ~(pwmperiod / 2) + 1U;
    pwmp->flexpwmp->SUB[1].VAL[0].R = 0;
    pwmp->flexpwmp->SUB[1].VAL[1].R = pwmperiod / 2;

    switch (pwmp->config->mode & PWM_ALIGN_MASK) {
    case PWM_ALIGN_EDGE:

      /* Setting active front of PWM channels.*/
      pwmp->flexpwmp->SUB[1].VAL[2].R = ~(pwmperiod / 2) + 1U;
      pwmp->flexpwmp->SUB[1].VAL[4].R = ~(pwmperiod / 2) + 1U;
      break;
    default:
      ;
    }
    pwmp->flexpwmp->MCTRL.B.LDOK |= 2U;
  }
#endif
#if SPC5_PWM_USE_SMOD2
  if (&PWMD3 == pwmp) {
    pwmp->flexpwmp->MCTRL.B.CLDOK |= 4U;

    /* Setting PWM period.*/
    pwmp->flexpwmp->SUB[2].INIT.R = ~(pwmperiod / 2) + 1U;
    pwmp->flexpwmp->SUB[2].VAL[0].R = 0;
    pwmp->flexpwmp->SUB[2].VAL[1].R = pwmperiod / 2;

    switch (pwmp->config->mode & PWM_ALIGN_MASK) {
    case PWM_ALIGN_EDGE:
      /* Setting active front of PWM channels.*/
      pwmp->flexpwmp->SUB[2].VAL[2].R = ~(pwmperiod / 2) + 1U;
      pwmp->flexpwmp->SUB[2].VAL[4].R = ~(pwmperiod / 2) + 1U;
      break;
    default:
      ;
    }
    pwmp->flexpwmp->MCTRL.B.LDOK |= 4U;
  }
#endif
#if SPC5_PWM_USE_SMOD3
  if (&PWMD4 == pwmp) {
    pwmp->flexpwmp->MCTRL.B.CLDOK |= 8U;

    /* Setting PWM period.*/
    pwmp->flexpwmp->SUB[3].INIT.R = ~(pwmperiod / 2) + 1U;
    pwmp->flexpwmp->SUB[3].VAL[0].R = 0;
    pwmp->flexpwmp->SUB[3].VAL[1].R = pwmperiod / 2;

    switch (pwmp->config->mode & PWM_ALIGN_MASK) {
    case PWM_ALIGN_EDGE:
      /* Setting active front of PWM channels.*/
      pwmp->flexpwmp->SUB[3].VAL[2].R = ~(pwmperiod / 2) + 1U;
      pwmp->flexpwmp->SUB[3].VAL[4].R = ~(pwmperiod / 2) + 1U;
      break;
    default:
      ;
    }
    pwmp->flexpwmp->MCTRL.B.LDOK |= 8U;
  }
#endif
#if SPC5_PWM_USE_SMOD4
  if (&PWMD5 == pwmp) {
    pwmp->flexpwmp->MCTRL.B.CLDOK |= 1U;

    /* Setting PWM period.*/
    pwmp->flexpwmp->SUB[0].INIT.R = ~(pwmperiod / 2) + 1U;
    pwmp->flexpwmp->SUB[0].VAL[0].R = 0;
    pwmp->flexpwmp->SUB[0].VAL[1].R = pwmperiod / 2;

    switch (pwmp->config->mode & PWM_ALIGN_MASK) {
    case PWM_ALIGN_EDGE:
      /* Setting active front of PWM channels.*/
      pwmp->flexpwmp->SUB[0].VAL[2].R = ~(pwmperiod / 2) + 1U;
      pwmp->flexpwmp->SUB[0].VAL[4].R = ~(pwmperiod / 2) + 1U;
      break;
    default:
      ;
    }
    pwmp->flexpwmp->MCTRL.B.LDOK |= 1U;
  }
#endif
#if SPC5_PWM_USE_SMOD5
  if (&PWMD6 == pwmp) {
    pwmp->flexpwmp->MCTRL.B.CLDOK |= 2U;

    /* Setting PWM period.*/
    pwmp->flexpwmp->SUB[1].INIT.R = ~(pwmperiod / 2) + 1U;
    pwmp->flexpwmp->SUB[1].VAL[0].R = 0;
    pwmp->flexpwmp->SUB[1].VAL[1].R = pwmperiod / 2;

    switch (pwmp->config->mode & PWM_ALIGN_MASK) {
    case PWM_ALIGN_EDGE:
      /* Setting active front of PWM channels.*/
      pwmp->flexpwmp->SUB[1].VAL[2].R = ~(pwmperiod / 2) + 1U;
      pwmp->flexpwmp->SUB[1].VAL[4].R = ~(pwmperiod / 2) + 1U;
      break;
    default:
      ;
    }
    pwmp->flexpwmp->MCTRL.B.LDOK |= 2U;
  }
#endif
#if SPC5_PWM_USE_SMOD6
  if (&PWMD7 == pwmp) {
    pwmp->flexpwmp->MCTRL.B.CLDOK |= 4U;

    /* Setting PWM period.*/
    pwmp->flexpwmp->SUB[2].INIT.R = ~(pwmperiod / 2) + 1U;
    pwmp->flexpwmp->SUB[2].VAL[0].R = 0;
    pwmp->flexpwmp->SUB[2].VAL[1].R = pwmperiod / 2;

    switch (pwmp->config->mode & PWM_ALIGN_MASK) {
    case PWM_ALIGN_EDGE:
      /* Setting active front of PWM channels.*/
      pwmp->flexpwmp->SUB[2].VAL[2].R = ~(pwmperiod / 2) + 1U;
      pwmp->flexpwmp->SUB[2].VAL[4].R = ~(pwmperiod / 2) + 1U;
      break;
    default:
      ;
    }
    pwmp->flexpwmp->MCTRL.B.LDOK |= 4U;
  }
#endif
#if SPC5_PWM_USE_SMOD7
  if (&PWMD8 == pwmp) {
    pwmp->flexpwmp->MCTRL.B.CLDOK |= 8U;

    /* Setting PWM period.*/
    pwmp->flexpwmp->SUB[3].INIT.R = ~(pwmperiod / 2) + 1U;
    pwmp->flexpwmp->SUB[3].VAL[0].R = 0;
    pwmp->flexpwmp->SUB[3].VAL[1].R = pwmperiod / 2;

    switch (pwmp->config->mode & PWM_ALIGN_MASK) {
    case PWM_ALIGN_EDGE:
      /* Setting active front of PWM channels.*/
      pwmp->flexpwmp->SUB[3].VAL[2].R = ~(pwmperiod / 2) + 1U;
      pwmp->flexpwmp->SUB[3].VAL[4].R = ~(pwmperiod / 2) + 1U;
      break;
    default:
      ;
    }
    pwmp->flexpwmp->MCTRL.B.LDOK |= 8U;
  }
#endif
}

#endif /* HAL_USE_PWM */

/** @} */
