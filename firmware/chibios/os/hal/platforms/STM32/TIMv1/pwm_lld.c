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
 * @file    STM32/pwm_lld.c
 * @brief   STM32 PWM subsystem low level driver header.
 *
 * @addtogroup PWM
 * @{
 */

#include "ch.h"
#include "hal.h"

#if HAL_USE_PWM || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/**
 * @brief   PWMD1 driver identifier.
 * @note    The driver PWMD1 allocates the complex timer TIM1 when enabled.
 */
#if STM32_PWM_USE_TIM1 || defined(__DOXYGEN__)
PWMDriver PWMD1;
#endif

/**
 * @brief   PWMD2 driver identifier.
 * @note    The driver PWMD2 allocates the timer TIM2 when enabled.
 */
#if STM32_PWM_USE_TIM2 || defined(__DOXYGEN__)
PWMDriver PWMD2;
#endif

/**
 * @brief   PWMD3 driver identifier.
 * @note    The driver PWMD3 allocates the timer TIM3 when enabled.
 */
#if STM32_PWM_USE_TIM3 || defined(__DOXYGEN__)
PWMDriver PWMD3;
#endif

/**
 * @brief   PWMD4 driver identifier.
 * @note    The driver PWMD4 allocates the timer TIM4 when enabled.
 */
#if STM32_PWM_USE_TIM4 || defined(__DOXYGEN__)
PWMDriver PWMD4;
#endif

/**
 * @brief   PWMD5 driver identifier.
 * @note    The driver PWMD5 allocates the timer TIM5 when enabled.
 */
#if STM32_PWM_USE_TIM5 || defined(__DOXYGEN__)
PWMDriver PWMD5;
#endif

/**
 * @brief   PWMD8 driver identifier.
 * @note    The driver PWMD8 allocates the timer TIM8 when enabled.
 */
#if STM32_PWM_USE_TIM8 || defined(__DOXYGEN__)
PWMDriver PWMD8;
#endif

/**
 * @brief   PWMD9 driver identifier.
 * @note    The driver PWMD9 allocates the timer TIM9 when enabled.
 */
#if STM32_PWM_USE_TIM9 || defined(__DOXYGEN__)
PWMDriver PWMD9;
#endif

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

#if STM32_PWM_USE_TIM2 || STM32_PWM_USE_TIM3 || STM32_PWM_USE_TIM4 ||       \
    STM32_PWM_USE_TIM5 || STM32_PWM_USE_TIM9 || defined(__DOXYGEN__)
/**
 * @brief   Common TIM2...TIM5,TIM9 IRQ handler.
 * @note    It is assumed that the various sources are only activated if the
 *          associated callback pointer is not equal to @p NULL in order to not
 *          perform an extra check in a potentially critical interrupt handler.
 *
 * @param[in] pwmp      pointer to a @p PWMDriver object
 */
static void pwm_lld_serve_interrupt(PWMDriver *pwmp) {
  uint16_t sr;

  sr  = pwmp->tim->SR;
  sr &= pwmp->tim->DIER & STM32_TIM_DIER_IRQ_MASK;
  pwmp->tim->SR = ~sr;
  if ((sr & STM32_TIM_SR_CC1IF) != 0)
    pwmp->config->channels[0].callback(pwmp);
  if ((sr & STM32_TIM_SR_CC2IF) != 0)
    pwmp->config->channels[1].callback(pwmp);
  if ((sr & STM32_TIM_SR_CC3IF) != 0)
    pwmp->config->channels[2].callback(pwmp);
  if ((sr & STM32_TIM_SR_CC4IF) != 0)
    pwmp->config->channels[3].callback(pwmp);
  if ((sr & STM32_TIM_SR_UIF) != 0)
    pwmp->config->callback(pwmp);
}
#endif /* STM32_PWM_USE_TIM2 || ... || STM32_PWM_USE_TIM5 */

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

#if STM32_PWM_USE_TIM1
#if !defined(STM32_TIM1_UP_HANDLER)
#error "STM32_TIM1_UP_HANDLER not defined"
#endif
/**
 * @brief   TIM1 update interrupt handler.
 * @note    It is assumed that this interrupt is only activated if the callback
 *          pointer is not equal to @p NULL in order to not perform an extra
 *          check in a potentially critical interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(STM32_TIM1_UP_HANDLER) {

  CH_IRQ_PROLOGUE();

  STM32_TIM1->SR = ~STM32_TIM_SR_UIF;
  PWMD1.config->callback(&PWMD1);

  CH_IRQ_EPILOGUE();
}

#if !defined(STM32_TIM1_CC_HANDLER)
#error "STM32_TIM1_CC_HANDLER not defined"
#endif
/**
 * @brief   TIM1 compare interrupt handler.
 * @note    It is assumed that the various sources are only activated if the
 *          associated callback pointer is not equal to @p NULL in order to not
 *          perform an extra check in a potentially critical interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(STM32_TIM1_CC_HANDLER) {
  uint16_t sr;

  CH_IRQ_PROLOGUE();

  sr = STM32_TIM1->SR & STM32_TIM1->DIER & STM32_TIM_DIER_IRQ_MASK;
  STM32_TIM1->SR = ~sr;
  if ((sr & STM32_TIM_SR_CC1IF) != 0)
    PWMD1.config->channels[0].callback(&PWMD1);
  if ((sr & STM32_TIM_SR_CC2IF) != 0)
    PWMD1.config->channels[1].callback(&PWMD1);
  if ((sr & STM32_TIM_SR_CC3IF) != 0)
    PWMD1.config->channels[2].callback(&PWMD1);
  if ((sr & STM32_TIM_SR_CC4IF) != 0)
    PWMD1.config->channels[3].callback(&PWMD1);

  CH_IRQ_EPILOGUE();
}
#endif /* STM32_PWM_USE_TIM1 */

#if STM32_PWM_USE_TIM2
#if !defined(STM32_TIM2_HANDLER)
#error "STM32_TIM2_HANDLER not defined"
#endif
/**
 * @brief   TIM2 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(STM32_TIM2_HANDLER) {

  CH_IRQ_PROLOGUE();

  pwm_lld_serve_interrupt(&PWMD2);

  CH_IRQ_EPILOGUE();
}
#endif /* STM32_PWM_USE_TIM2 */

#if STM32_PWM_USE_TIM3
#if !defined(STM32_TIM3_HANDLER)
#error "STM32_TIM3_HANDLER not defined"
#endif
/**
 * @brief   TIM3 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(STM32_TIM3_HANDLER) {

  CH_IRQ_PROLOGUE();

  pwm_lld_serve_interrupt(&PWMD3);

  CH_IRQ_EPILOGUE();
}
#endif /* STM32_PWM_USE_TIM3 */

#if STM32_PWM_USE_TIM4
#if !defined(STM32_TIM4_HANDLER)
#error "STM32_TIM4_HANDLER not defined"
#endif
/**
 * @brief   TIM4 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(STM32_TIM4_HANDLER) {

  CH_IRQ_PROLOGUE();

  pwm_lld_serve_interrupt(&PWMD4);

  CH_IRQ_EPILOGUE();
}
#endif /* STM32_PWM_USE_TIM4 */

#if STM32_PWM_USE_TIM5
#if !defined(STM32_TIM5_HANDLER)
#error "STM32_TIM5_HANDLER not defined"
#endif
/**
 * @brief   TIM5 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(STM32_TIM5_HANDLER) {

  CH_IRQ_PROLOGUE();

  pwm_lld_serve_interrupt(&PWMD5);

  CH_IRQ_EPILOGUE();
}
#endif /* STM32_PWM_USE_TIM5 */

#if STM32_PWM_USE_TIM8
#if !defined(STM32_TIM8_UP_HANDLER)
#error "STM32_TIM8_UP_HANDLER not defined"
#endif
/**
 * @brief   TIM8 update interrupt handler.
 * @note    It is assumed that this interrupt is only activated if the callback
 *          pointer is not equal to @p NULL in order to not perform an extra
 *          check in a potentially critical interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(STM32_TIM8_UP_HANDLER) {

  CH_IRQ_PROLOGUE();

  STM32_TIM8->SR = ~TIM_SR_UIF;
  PWMD8.config->callback(&PWMD8);

  CH_IRQ_EPILOGUE();
}

#if !defined(STM32_TIM8_CC_HANDLER)
#error "STM32_TIM8_CC_HANDLER not defined"
#endif
/**
 * @brief   TIM8 compare interrupt handler.
 * @note    It is assumed that the various sources are only activated if the
 *          associated callback pointer is not equal to @p NULL in order to not
 *          perform an extra check in a potentially critical interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(STM32_TIM8_CC_HANDLER) {
  uint16_t sr;

  CH_IRQ_PROLOGUE();

  sr = STM32_TIM8->SR & STM32_TIM8->DIER & STM32_TIM_DIER_IRQ_MASK;
  STM32_TIM8->SR = ~sr;
  if ((sr & STM32_TIM_SR_CC1IF) != 0)
    PWMD8.config->channels[0].callback(&PWMD8);
  if ((sr & STM32_TIM_SR_CC2IF) != 0)
    PWMD8.config->channels[1].callback(&PWMD8);
  if ((sr & STM32_TIM_SR_CC3IF) != 0)
    PWMD8.config->channels[2].callback(&PWMD8);
  if ((sr & STM32_TIM_SR_CC4IF) != 0)
    PWMD8.config->channels[3].callback(&PWMD8);

  CH_IRQ_EPILOGUE();
}
#endif /* STM32_PWM_USE_TIM8 */

#if STM32_PWM_USE_TIM9
#if !defined(STM32_TIM9_HANDLER)
#error "STM32_TIM9_HANDLER not defined"
#endif
/**
 * @brief   TIM9 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(STM32_TIM9_HANDLER) {

  CH_IRQ_PROLOGUE();

  pwm_lld_serve_interrupt(&PWMD9);

  CH_IRQ_EPILOGUE();
}
#endif /* STM32_PWM_USE_TIM9 */

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level PWM driver initialization.
 *
 * @notapi
 */
void pwm_lld_init(void) {

#if STM32_PWM_USE_TIM1
  /* Driver initialization.*/
  pwmObjectInit(&PWMD1);
  PWMD1.tim = STM32_TIM1;
#endif

#if STM32_PWM_USE_TIM2
  /* Driver initialization.*/
  pwmObjectInit(&PWMD2);
  PWMD2.tim = STM32_TIM2;
#endif

#if STM32_PWM_USE_TIM3
  /* Driver initialization.*/
  pwmObjectInit(&PWMD3);
  PWMD3.tim = STM32_TIM3;
#endif

#if STM32_PWM_USE_TIM4
  /* Driver initialization.*/
  pwmObjectInit(&PWMD4);
  PWMD4.tim = STM32_TIM4;
#endif

#if STM32_PWM_USE_TIM5
  /* Driver initialization.*/
  pwmObjectInit(&PWMD5);
  PWMD5.tim = STM32_TIM5;
#endif

#if STM32_PWM_USE_TIM8
  /* Driver initialization.*/
  pwmObjectInit(&PWMD8);
  PWMD8.tim = STM32_TIM8;
#endif

#if STM32_PWM_USE_TIM9
  /* Driver initialization.*/
  pwmObjectInit(&PWMD9);
  PWMD9.tim = STM32_TIM9;
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
  uint32_t psc;
  uint32_t ccer;

  if (pwmp->state == PWM_STOP) {
    /* Clock activation and timer reset.*/
#if STM32_PWM_USE_TIM1
    if (&PWMD1 == pwmp) {
      rccEnableTIM1(FALSE);
      rccResetTIM1();
      nvicEnableVector(STM32_TIM1_UP_NUMBER,
                       CORTEX_PRIORITY_MASK(STM32_PWM_TIM1_IRQ_PRIORITY));
      nvicEnableVector(STM32_TIM1_CC_NUMBER,
                       CORTEX_PRIORITY_MASK(STM32_PWM_TIM1_IRQ_PRIORITY));
#if defined(STM32_TIM1CLK)
      pwmp->clock = STM32_TIM1CLK;
#else
      pwmp->clock = STM32_TIMCLK2;
#endif
    }
#endif
#if STM32_PWM_USE_TIM2
    if (&PWMD2 == pwmp) {
      rccEnableTIM2(FALSE);
      rccResetTIM2();
      nvicEnableVector(STM32_TIM2_NUMBER,
                       CORTEX_PRIORITY_MASK(STM32_PWM_TIM2_IRQ_PRIORITY));
      pwmp->clock = STM32_TIMCLK1;
    }
#endif
#if STM32_PWM_USE_TIM3
    if (&PWMD3 == pwmp) {
      rccEnableTIM3(FALSE);
      rccResetTIM3();
      nvicEnableVector(STM32_TIM3_NUMBER,
                       CORTEX_PRIORITY_MASK(STM32_PWM_TIM3_IRQ_PRIORITY));
      pwmp->clock = STM32_TIMCLK1;
    }
#endif
#if STM32_PWM_USE_TIM4
    if (&PWMD4 == pwmp) {
      rccEnableTIM4(FALSE);
      rccResetTIM4();
      nvicEnableVector(STM32_TIM4_NUMBER,
                       CORTEX_PRIORITY_MASK(STM32_PWM_TIM4_IRQ_PRIORITY));
      pwmp->clock = STM32_TIMCLK1;
    }
#endif

#if STM32_PWM_USE_TIM5
    if (&PWMD5 == pwmp) {
      rccEnableTIM5(FALSE);
      rccResetTIM5();
      nvicEnableVector(STM32_TIM5_NUMBER,
                       CORTEX_PRIORITY_MASK(STM32_PWM_TIM5_IRQ_PRIORITY));
      pwmp->clock = STM32_TIMCLK1;
    }
#endif
#if STM32_PWM_USE_TIM8
    if (&PWMD8 == pwmp) {
      rccEnableTIM8(FALSE);
      rccResetTIM8();
      nvicEnableVector(STM32_TIM8_UP_NUMBER,
                       CORTEX_PRIORITY_MASK(STM32_PWM_TIM8_IRQ_PRIORITY));
      nvicEnableVector(STM32_TIM8_CC_NUMBER,
                       CORTEX_PRIORITY_MASK(STM32_PWM_TIM8_IRQ_PRIORITY));
#if defined(STM32_TIM8CLK)
      pwmp->clock = STM32_TIM8CLK;
#else
      pwmp->clock = STM32_TIMCLK2;
#endif
    }
#endif
#if STM32_PWM_USE_TIM9
    if (&PWMD9 == pwmp) {
      rccEnableTIM9(FALSE);
      rccResetTIM9();
      nvicEnableVector(STM32_TIM9_NUMBER,
                       CORTEX_PRIORITY_MASK(STM32_PWM_TIM9_IRQ_PRIORITY));
      pwmp->clock = STM32_TIMCLK2;
    }
#endif

    /* All channels configured in PWM1 mode with preload enabled and will
       stay that way until the driver is stopped.*/
    pwmp->tim->CCMR1 = STM32_TIM_CCMR1_OC1M(6) | STM32_TIM_CCMR1_OC1PE |
                       STM32_TIM_CCMR1_OC2M(6) | STM32_TIM_CCMR1_OC2PE;
    pwmp->tim->CCMR2 = STM32_TIM_CCMR2_OC3M(6) | STM32_TIM_CCMR2_OC3PE |
                       STM32_TIM_CCMR2_OC4M(6) | STM32_TIM_CCMR2_OC4PE;
  }
  else {
    /* Driver re-configuration scenario, it must be stopped first.*/
    pwmp->tim->CR1    = 0;                  /* Timer disabled.              */
    pwmp->tim->CCR[0] = 0;                  /* Comparator 1 disabled.       */
    pwmp->tim->CCR[1] = 0;                  /* Comparator 2 disabled.       */
    pwmp->tim->CCR[2] = 0;                  /* Comparator 3 disabled.       */
    pwmp->tim->CCR[3] = 0;                  /* Comparator 4 disabled.       */
    pwmp->tim->CNT  = 0;                    /* Counter reset to zero.       */
  }

  /* Timer configuration.*/
  psc = (pwmp->clock / pwmp->config->frequency) - 1;
  chDbgAssert((psc <= 0xFFFF) &&
              ((psc + 1) * pwmp->config->frequency) == pwmp->clock,
              "pwm_lld_start(), #1", "invalid frequency");
  pwmp->tim->PSC  = (uint16_t)psc;
  pwmp->tim->ARR  = (uint16_t)(pwmp->period - 1);
  pwmp->tim->CR2  = pwmp->config->cr2;

  /* Output enables and polarities setup.*/
  ccer = 0;
  switch (pwmp->config->channels[0].mode & PWM_OUTPUT_MASK) {
  case PWM_OUTPUT_ACTIVE_LOW:
    ccer |= STM32_TIM_CCER_CC1P;
  case PWM_OUTPUT_ACTIVE_HIGH:
    ccer |= STM32_TIM_CCER_CC1E;
  default:
    ;
  }
  switch (pwmp->config->channels[1].mode & PWM_OUTPUT_MASK) {
  case PWM_OUTPUT_ACTIVE_LOW:
    ccer |= STM32_TIM_CCER_CC2P;
  case PWM_OUTPUT_ACTIVE_HIGH:
    ccer |= STM32_TIM_CCER_CC2E;
  default:
    ;
  }
  switch (pwmp->config->channels[2].mode & PWM_OUTPUT_MASK) {
  case PWM_OUTPUT_ACTIVE_LOW:
    ccer |= STM32_TIM_CCER_CC3P;
  case PWM_OUTPUT_ACTIVE_HIGH:
    ccer |= STM32_TIM_CCER_CC3E;
  default:
    ;
  }
  switch (pwmp->config->channels[3].mode & PWM_OUTPUT_MASK) {
  case PWM_OUTPUT_ACTIVE_LOW:
    ccer |= STM32_TIM_CCER_CC4P;
  case PWM_OUTPUT_ACTIVE_HIGH:
    ccer |= STM32_TIM_CCER_CC4E;
  default:
    ;
  }
#if STM32_PWM_USE_ADVANCED
#if STM32_PWM_USE_TIM1 && !STM32_PWM_USE_TIM8
  if (&PWMD1 == pwmp) {
#endif
#if !STM32_PWM_USE_TIM1 && STM32_PWM_USE_TIM8
  if (&PWMD8 == pwmp) {
#endif
#if STM32_PWM_USE_TIM1 && STM32_PWM_USE_TIM8
  if ((&PWMD1 == pwmp) || (&PWMD8 == pwmp)) {
#endif
    switch (pwmp->config->channels[0].mode & PWM_COMPLEMENTARY_OUTPUT_MASK) {
    case PWM_COMPLEMENTARY_OUTPUT_ACTIVE_LOW:
      ccer |= STM32_TIM_CCER_CC1NP;
    case PWM_COMPLEMENTARY_OUTPUT_ACTIVE_HIGH:
      ccer |= STM32_TIM_CCER_CC1NE;
    default:
      ;
    }
    switch (pwmp->config->channels[1].mode & PWM_COMPLEMENTARY_OUTPUT_MASK) {
    case PWM_COMPLEMENTARY_OUTPUT_ACTIVE_LOW:
      ccer |= STM32_TIM_CCER_CC2NP;
    case PWM_COMPLEMENTARY_OUTPUT_ACTIVE_HIGH:
      ccer |= STM32_TIM_CCER_CC2NE;
    default:
      ;
    }
    switch (pwmp->config->channels[2].mode & PWM_COMPLEMENTARY_OUTPUT_MASK) {
    case PWM_COMPLEMENTARY_OUTPUT_ACTIVE_LOW:
      ccer |= STM32_TIM_CCER_CC3NP;
    case PWM_COMPLEMENTARY_OUTPUT_ACTIVE_HIGH:
      ccer |= STM32_TIM_CCER_CC3NE;
    default:
      ;
    }
  }
#endif /* STM32_PWM_USE_ADVANCED*/

  pwmp->tim->CCER  = ccer;
  pwmp->tim->EGR   = STM32_TIM_EGR_UG;      /* Update event.                */
  pwmp->tim->SR    = 0;                     /* Clear pending IRQs.          */
  pwmp->tim->DIER  = (pwmp->config->callback == NULL ? 0 : STM32_TIM_DIER_UIE) |
                     (pwmp->config->dier & ~STM32_TIM_DIER_IRQ_MASK);
#if STM32_PWM_USE_TIM1 || STM32_PWM_USE_TIM8
#if STM32_PWM_USE_ADVANCED
  pwmp->tim->BDTR  = pwmp->config->bdtr | STM32_TIM_BDTR_MOE;
#else
  pwmp->tim->BDTR  = STM32_TIM_BDTR_MOE;
#endif
#endif
  /* Timer configured and started.*/
  pwmp->tim->CR1   = STM32_TIM_CR1_ARPE | STM32_TIM_CR1_URS |
                     STM32_TIM_CR1_CEN;
}

/**
 * @brief   Deactivates the PWM peripheral.
 *
 * @param[in] pwmp      pointer to a @p PWMDriver object
 *
 * @notapi
 */
void pwm_lld_stop(PWMDriver *pwmp) {

  /* If in ready state then disables the PWM clock.*/
  if (pwmp->state == PWM_READY) {
    pwmp->tim->CR1  = 0;                    /* Timer disabled.              */
    pwmp->tim->DIER = 0;                    /* All IRQs disabled.           */
    pwmp->tim->SR   = 0;                    /* Clear eventual pending IRQs. */
#if STM32_PWM_USE_TIM1 || STM32_PWM_USE_TIM8
    pwmp->tim->BDTR  = 0;
#endif

#if STM32_PWM_USE_TIM1
    if (&PWMD1 == pwmp) {
      nvicDisableVector(STM32_TIM1_UP_NUMBER);
      nvicDisableVector(STM32_TIM1_CC_NUMBER);
      rccDisableTIM1(FALSE);
    }
#endif
#if STM32_PWM_USE_TIM2
    if (&PWMD2 == pwmp) {
      nvicDisableVector(STM32_TIM2_NUMBER);
      rccDisableTIM2(FALSE);
    }
#endif
#if STM32_PWM_USE_TIM3
    if (&PWMD3 == pwmp) {
      nvicDisableVector(STM32_TIM3_NUMBER);
      rccDisableTIM3(FALSE);
    }
#endif
#if STM32_PWM_USE_TIM4
    if (&PWMD4 == pwmp) {
      nvicDisableVector(STM32_TIM4_NUMBER);
      rccDisableTIM4(FALSE);
    }
#endif
#if STM32_PWM_USE_TIM5
    if (&PWMD5 == pwmp) {
      nvicDisableVector(STM32_TIM5_NUMBER);
      rccDisableTIM5(FALSE);
    }
#endif
#if STM32_PWM_USE_TIM8
    if (&PWMD8 == pwmp) {
      nvicDisableVector(STM32_TIM8_UP_NUMBER);
      nvicDisableVector(STM32_TIM8_CC_NUMBER);
      rccDisableTIM8(FALSE);
    }
#endif
#if STM32_PWM_USE_TIM9
    if (&PWMD9 == pwmp) {
      nvicDisableVector(STM32_TIM9_NUMBER);
      rccDisableTIM9(FALSE);
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

  pwmp->tim->CCR[channel] = width;                  /* New duty cycle.      */
  /* If there is a callback defined for the channel then the associated
     interrupt must be enabled.*/
  if (pwmp->config->channels[channel].callback != NULL) {
    uint32_t dier = pwmp->tim->DIER;
    /* If the IRQ is not already enabled care must be taken to clear it,
       it is probably already pending because the timer is running.*/
    if ((dier & (2 << channel)) == 0) {
      pwmp->tim->DIER = dier | (2 << channel);
      pwmp->tim->SR   = ~(2 << channel);
    }
  }
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

  pwmp->tim->CCR[channel] = 0;
  pwmp->tim->DIER &= ~(2 << channel);
}

#endif /* HAL_USE_PWM */

/** @} */
