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
 * @file    eTimer_v1/icu_lld.c
 * @brief   SPC5xx low level ICU driver code.
 *
 * @addtogroup ICU
 * @{
 */

#include "ch.h"
#include "hal.h"

#if HAL_USE_ICU || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/**
 * @brief   ICUD1 driver identifier.
 * @note    The driver ICUD1 allocates the complex timer SMOD0 when enabled.
 */
#if SPC5_ICU_USE_SMOD0 || defined(__DOXYGEN__)
ICUDriver ICUD1;
#endif

/**
 * @brief   ICUD2 driver identifier.
 * @note    The driver ICUD2 allocates the complex timer SMOD1 when enabled.
 */
#if SPC5_ICU_USE_SMOD1 || defined(__DOXYGEN__)
ICUDriver ICUD2;
#endif

/**
 * @brief   ICUD3 driver identifier.
 * @note    The driver ICUD3 allocates the complex timer SMOD2 when enabled.
 */
#if SPC5_ICU_USE_SMOD2 || defined(__DOXYGEN__)
ICUDriver ICUD3;
#endif

/**
 * @brief   ICUD4 driver identifier.
 * @note    The driver ICUD4 allocates the complex timer SMOD3 when enabled.
 */
#if SPC5_ICU_USE_SMOD3 || defined(__DOXYGEN__)
ICUDriver ICUD4;
#endif

/**
 * @brief   ICUD5 driver identifier.
 * @note    The driver ICUD5 allocates the complex timer SMOD4 when enabled.
 */
#if SPC5_ICU_USE_SMOD4 || defined(__DOXYGEN__)
ICUDriver ICUD5;
#endif

/**
 * @brief   ICUD6 driver identifier.
 * @note    The driver ICUD6 allocates the complex timer SMOD5 when enabled.
 */
#if SPC5_ICU_USE_SMOD5 || defined(__DOXYGEN__)
ICUDriver ICUD6;
#endif

/**
 * @brief   ICUD7 driver identifier.
 * @note    The driver ICUD7 allocates the complex timer SMOD6 when enabled.
 */
#if SPC5_ICU_USE_SMOD6 || defined(__DOXYGEN__)
ICUDriver ICUD7;
#endif

/**
 * @brief   ICUD8 driver identifier.
 * @note    The driver ICUD8 allocates the complex timer SMOD7 when enabled.
 */
#if SPC5_ICU_USE_SMOD7 || defined(__DOXYGEN__)
ICUDriver ICUD8;
#endif

/**
 * @brief   ICUD9 driver identifier.
 * @note    The driver ICUD9 allocates the complex timer SMOD8 when enabled.
 */
#if SPC5_ICU_USE_SMOD8 || defined(__DOXYGEN__)
ICUDriver ICUD9;
#endif

/**
 * @brief   ICUD10 driver identifier.
 * @note    The driver ICUD10 allocates the complex timer SMOD9 when enabled.
 */
#if SPC5_ICU_USE_SMOD9 || defined(__DOXYGEN__)
ICUDriver ICUD10;
#endif

/**
 * @brief   ICUD11 driver identifier.
 * @note    The driver ICUD11 allocates the complex timer SMOD10 when enabled.
 */
#if SPC5_ICU_USE_SMOD10 || defined(__DOXYGEN__)
ICUDriver ICUD11;
#endif

/**
 * @brief   ICUD12 driver identifier.
 * @note    The driver ICUD12 allocates the complex timer SMOD11 when enabled.
 */
#if SPC5_ICU_USE_SMOD11 || defined(__DOXYGEN__)
ICUDriver ICUD12;
#endif

/**
 * @brief   ICUD13 driver identifier.
 * @note    The driver ICUD13 allocates the complex timer SMOD12 when enabled.
 */
#if SPC5_ICU_USE_SMOD12 || defined(__DOXYGEN__)
ICUDriver ICUD13;
#endif

/**
 * @brief   ICUD14 driver identifier.
 * @note    The driver ICUD14 allocates the complex timer SMOD13 when enabled.
 */
#if SPC5_ICU_USE_SMOD13 || defined(__DOXYGEN__)
ICUDriver ICUD14;
#endif

/**
 * @brief   ICUD15 driver identifier.
 * @note    The driver ICUD15 allocates the complex timer SMOD14 when enabled.
 */
#if SPC5_ICU_USE_SMOD14 || defined(__DOXYGEN__)
ICUDriver ICUD15;
#endif

/**
 * @brief   ICUD16 driver identifier.
 * @note    The driver ICUD16 allocates the complex timer SMOD15 when enabled.
 */
#if SPC5_ICU_USE_SMOD15 || defined(__DOXYGEN__)
ICUDriver ICUD16;
#endif

/**
 * @brief   ICUD17 driver identifier.
 * @note    The driver ICUD17 allocates the complex timer SMOD16 when enabled.
 */
#if SPC5_ICU_USE_SMOD16 || defined(__DOXYGEN__)
ICUDriver ICUD17;
#endif

/**
 * @brief   ICUD18 driver identifier.
 * @note    The driver ICUD18 allocates the complex timer SMOD17 when enabled.
 */
#if SPC5_ICU_USE_SMOD17 || defined(__DOXYGEN__)
ICUDriver ICUD18;
#endif

/*===========================================================================*/
/* Driver local variables.                                                   */
/*===========================================================================*/

/**
 * @brief   Number of active eTimer Submodules.
 */
static uint32_t icu_active_submodules0;
static uint32_t icu_active_submodules1;
static uint32_t icu_active_submodules2;

/**
 * @brief   Width and Period registers.
 */
uint16_t width;
uint16_t period;

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/**
 * @brief   Shared IRQ handler.
 *
 * @param[in] icup      pointer to the @p ICUDriver object
 */
static void icu_lld_serve_interrupt(ICUDriver *icup) {
  uint16_t sr = icup->etimerp->CHANNEL[icup->smod_number].STS.R &
                icup->etimerp->CHANNEL[icup->smod_number].INTDMA.R;

  if (ICU_SKIP_FIRST_CAPTURE) {
    if ((sr & 0x0008) != 0) { /* TOF */
      icup->etimerp->CHANNEL[icup->smod_number].STS.B.TOF = 1U;
      _icu_isr_invoke_overflow_cb(icup);
    }
    if ((sr & 0x0040) != 0) { /* ICF1 */
      if (icup->etimerp->CHANNEL[icup->smod_number].CTRL.B.CNTMODE ==
          SPC5_ETIMER_CNTMODE_RFE_SIHA) {
        icup->etimerp->CHANNEL[icup->smod_number].STS.B.ICF1 = 1U;
        icup->etimerp->CHANNEL[icup->smod_number].CTRL.B.CNTMODE =
            SPC5_ETIMER_CNTMODE_RE;
      }
      else {
        icup->etimerp->CHANNEL[icup->smod_number].STS.B.ICF1 = 1U;
        if (icup->etimerp->CHANNEL[icup->smod_number].CTRL3.B.C1FCNT == 2) {
          period = icup->etimerp->CHANNEL[icup->smod_number].CAPT1.R;
          period = icup->etimerp->CHANNEL[icup->smod_number].CAPT1.R;
        } else {
          period = icup->etimerp->CHANNEL[icup->smod_number].CAPT1.R;
        }
        _icu_isr_invoke_period_cb(icup);
      }
    }
    else if ((sr & 0x0080) != 0) { /* ICF2 */
      if (icup->etimerp->CHANNEL[icup->smod_number].CTRL.B.CNTMODE ==
          SPC5_ETIMER_CNTMODE_RFE_SIHA) {
        icup->etimerp->CHANNEL[icup->smod_number].STS.B.ICF2 = 1U;
        icup->etimerp->CHANNEL[icup->smod_number].CNTR.R = 0;
      }
      else {
        icup->etimerp->CHANNEL[icup->smod_number].STS.B.ICF2 = 1U;
        if (icup->etimerp->CHANNEL[icup->smod_number].CTRL3.B.C2FCNT == 2) {
          width = icup->etimerp->CHANNEL[icup->smod_number].CAPT2.R;
          width = icup->etimerp->CHANNEL[icup->smod_number].CAPT2.R;
        } else {
          width = icup->etimerp->CHANNEL[icup->smod_number].CAPT2.R;
        }
        _icu_isr_invoke_width_cb(icup);
      }
    }
  } else { /* ICU_SKIP_FIRST_CAPTURE = TRUE*/
    if ((sr & 0x0008) != 0) { /* TOF */
      icup->etimerp->CHANNEL[icup->smod_number].STS.B.TOF = 1U;
      _icu_isr_invoke_overflow_cb(icup);
    }
    if ((sr & 0x0040) != 0) { /* ICF1 */
      icup->etimerp->CHANNEL[icup->smod_number].STS.B.ICF1 = 1U;
      if (icup->etimerp->CHANNEL[icup->smod_number].CTRL3.B.C1FCNT == 2) {
        period = icup->etimerp->CHANNEL[icup->smod_number].CAPT1.R;
        period = icup->etimerp->CHANNEL[icup->smod_number].CAPT1.R;
      } else {
        period = icup->etimerp->CHANNEL[icup->smod_number].CAPT1.R;
      }
      _icu_isr_invoke_period_cb(icup);
    }
    else if ((sr & 0x0080) != 0) { /* ICF2 */
      icup->etimerp->CHANNEL[icup->smod_number].STS.B.ICF2 = 1U;
      if (icup->etimerp->CHANNEL[icup->smod_number].CTRL3.B.C2FCNT == 2) {
        width = icup->etimerp->CHANNEL[icup->smod_number].CAPT2.R;
        width = icup->etimerp->CHANNEL[icup->smod_number].CAPT2.R;
      } else {
        width = icup->etimerp->CHANNEL[icup->smod_number].CAPT2.R;
      }
      _icu_isr_invoke_width_cb(icup);
    }
  } /* ICU_SKIP_FIRST_CAPTURE = FALSE */
}

/**
 * @brief   eTimer SubModules initialization.
 * @details This function must be invoked with interrupts disabled.
 *
 * @param[in] sdp       pointer to a @p ICUDriver object
 * @param[in] config    the architecture-dependent ICU driver configuration
 */
static void spc5_icu_smod_init(ICUDriver *icup) {
  uint32_t psc = (icup->clock / icup->config->frequency);

  chDbgAssert((psc <= 0xFFFF) &&
              ((psc * icup->config->frequency) == icup->clock) &&
              ((psc == 1) || (psc == 2) || (psc == 4) ||
               (psc == 8) || (psc == 16) || (psc == 32) ||
               (psc == 64) || (psc == 128)),
              "spc5_icu_smod_init(), #1", "invalid frequency");

  /* Set primary source and clock prescaler.*/
  switch (psc) {
  case 1:
    icup->etimerp->CHANNEL[icup->smod_number].CTRL.B.PRISRC =
        SPC5_ETIMER_IP_BUS_CLOCK_DIVIDE_BY_1;
    break;
  case 2:
    icup->etimerp->CHANNEL[icup->smod_number].CTRL.B.PRISRC =
        SPC5_ETIMER_IP_BUS_CLOCK_DIVIDE_BY_2;
    break;
  case 4:
    icup->etimerp->CHANNEL[icup->smod_number].CTRL.B.PRISRC =
        SPC5_ETIMER_IP_BUS_CLOCK_DIVIDE_BY_4;
    break;
  case 8:
    icup->etimerp->CHANNEL[icup->smod_number].CTRL.B.PRISRC =
        SPC5_ETIMER_IP_BUS_CLOCK_DIVIDE_BY_8;
    break;
  case 16:
    icup->etimerp->CHANNEL[icup->smod_number].CTRL.B.PRISRC =
        SPC5_ETIMER_IP_BUS_CLOCK_DIVIDE_BY_16;
    break;
  case 32:
    icup->etimerp->CHANNEL[icup->smod_number].CTRL.B.PRISRC =
        SPC5_ETIMER_IP_BUS_CLOCK_DIVIDE_BY_32;
    break;
  case 64:
    icup->etimerp->CHANNEL[icup->smod_number].CTRL.B.PRISRC =
        SPC5_ETIMER_IP_BUS_CLOCK_DIVIDE_BY_64;
    break;
  case 128:
    icup->etimerp->CHANNEL[icup->smod_number].CTRL.B.PRISRC =
        SPC5_ETIMER_IP_BUS_CLOCK_DIVIDE_BY_128;
    break;
  }

  /* Set control registers.*/
  icup->etimerp->CHANNEL[icup->smod_number].CTRL.B.ONCE = 0U;
  icup->etimerp->CHANNEL[icup->smod_number].CTRL.B.LENGTH = 0U;
  icup->etimerp->CHANNEL[icup->smod_number].CTRL.B.DIR = 0U;
  icup->etimerp->CHANNEL[icup->smod_number].CTRL2.B.PIPS = 0U;

  /* Set secondary source.*/
  switch (icup->smod_number) {
  case 0:
    icup->etimerp->CHANNEL[icup->smod_number].CTRL.B.SECSRC =
        SPC5_ETIMER_COUNTER_0_INPUT_PIN;
    break;
  case 1:
    icup->etimerp->CHANNEL[icup->smod_number].CTRL.B.SECSRC =
        SPC5_ETIMER_COUNTER_1_INPUT_PIN;
    break;
  case 2:
    icup->etimerp->CHANNEL[icup->smod_number].CTRL.B.SECSRC =
        SPC5_ETIMER_COUNTER_2_INPUT_PIN;
    break;
  case 3:
    icup->etimerp->CHANNEL[icup->smod_number].CTRL.B.SECSRC =
        SPC5_ETIMER_COUNTER_3_INPUT_PIN;
    break;
  case 4:
    icup->etimerp->CHANNEL[icup->smod_number].CTRL.B.SECSRC =
        SPC5_ETIMER_COUNTER_4_INPUT_PIN;
    break;
  case 5:
    icup->etimerp->CHANNEL[icup->smod_number].CTRL.B.SECSRC =
        SPC5_ETIMER_COUNTER_5_INPUT_PIN;
    break;
  }

  /* Set secondary source polarity.*/
  if (icup->config->mode == ICU_INPUT_ACTIVE_HIGH) {
    icup->etimerp->CHANNEL[icup->smod_number].CTRL2.B.SIPS = 0U;
  }
  else {
    icup->etimerp->CHANNEL[icup->smod_number].CTRL2.B.SIPS = 1U;
  }

  /* Direct pointers to the capture registers in order to make reading
     data faster from within callbacks.*/
  icup->pccrp = &period;
  icup->wccrp = &width;

  /* Enable channel.*/
  icup->etimerp->ENBL.B.ENBL |= 1U << (icup->smod_number);
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

#if SPC5_ICU_USE_SMOD0
#if !defined(SPC5_ETIMER0_TC0IR_HANDLER)
#error "SPC5_ETIMER0_TC0IR_HANDLER not defined"
#endif
/**
 * @brief   eTimer0 Channel 0 interrupt handler.
 * @note    It is assumed that the various sources are only activated if the
 *          associated callback pointer is not equal to @p NULL in order to not
 *          perform an extra check in a potentially critical interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(SPC5_ETIMER0_TC0IR_HANDLER) {

  CH_IRQ_PROLOGUE();

  icu_lld_serve_interrupt(&ICUD1);

  CH_IRQ_EPILOGUE();
}
#endif /* SPC5_ICU_USE_SMOD0 */

#if SPC5_ICU_USE_SMOD1
#if !defined(SPC5_ETIMER0_TC1IR_HANDLER)
#error "SPC5_ETIMER0_TC1IR_HANDLER not defined"
#endif
/**
 * @brief   eTimer0 Channel 1 interrupt handler.
 * @note    It is assumed that the various sources are only activated if the
 *          associated callback pointer is not equal to @p NULL in order to not
 *          perform an extra check in a potentially critical interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(SPC5_ETIMER0_TC1IR_HANDLER) {

  CH_IRQ_PROLOGUE();

  icu_lld_serve_interrupt(&ICUD2);

  CH_IRQ_EPILOGUE();
}
#endif /* SPC5_ICU_USE_SMOD1 */

#if SPC5_ICU_USE_SMOD2
#if !defined(SPC5_ETIMER0_TC2IR_HANDLER)
#error "SPC5_ETIMER0_TC2IR_HANDLER not defined"
#endif
/**
 * @brief   eTimer0 Channel 2 interrupt handler.
 * @note    It is assumed that the various sources are only activated if the
 *          associated callback pointer is not equal to @p NULL in order to not
 *          perform an extra check in a potentially critical interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(SPC5_ETIMER0_TC2IR_HANDLER) {

  CH_IRQ_PROLOGUE();

  icu_lld_serve_interrupt(&ICUD3);

  CH_IRQ_EPILOGUE();
}
#endif /* SPC5_ICU_USE_SMOD2 */

#if SPC5_ICU_USE_SMOD3
#if !defined(SPC5_ETIMER0_TC3IR_HANDLER)
#error "SPC5_ETIMER0_TC3IR_HANDLER not defined"
#endif
/**
 * @brief   eTimer0 Channel 3 interrupt handler.
 * @note    It is assumed that the various sources are only activated if the
 *          associated callback pointer is not equal to @p NULL in order to not
 *          perform an extra check in a potentially critical interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(SPC5_ETIMER0_TC3IR_HANDLER) {

  CH_IRQ_PROLOGUE();

  icu_lld_serve_interrupt(&ICUD4);

  CH_IRQ_EPILOGUE();
}
#endif /* SPC5_ICU_USE_SMOD3 */

#if SPC5_ICU_USE_SMOD4
#if !defined(SPC5_ETIMER0_TC4IR_HANDLER)
#error "SPC5_ETIMER0_TC4IR_HANDLER not defined"
#endif
/**
 * @brief   eTimer0 Channel 4 interrupt handler.
 * @note    It is assumed that the various sources are only activated if the
 *          associated callback pointer is not equal to @p NULL in order to not
 *          perform an extra check in a potentially critical interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(SPC5_ETIMER0_TC4IR_HANDLER) {

  CH_IRQ_PROLOGUE();

  icu_lld_serve_interrupt(&ICUD5);

  CH_IRQ_EPILOGUE();
}
#endif /* SPC5_ICU_USE_SMOD4 */

#if SPC5_ICU_USE_SMOD5
#if !defined(SPC5_ETIMER0_TC5IR_HANDLER)
#error "SPC5_ETIMER0_TC5IR_HANDLER not defined"
#endif
/**
 * @brief   eTimer0 Channel 5 interrupt handler.
 * @note    It is assumed that the various sources are only activated if the
 *          associated callback pointer is not equal to @p NULL in order to not
 *          perform an extra check in a potentially critical interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(SPC5_ETIMER0_TC5IR_HANDLER) {

  CH_IRQ_PROLOGUE();

  icu_lld_serve_interrupt(&ICUD6);

  CH_IRQ_EPILOGUE();
}
#endif /* SPC5_ICU_USE_SMOD5 */

#if SPC5_ICU_USE_SMOD6
#if !defined(SPC5_ETIMER1_TC0IR_HANDLER)
#error "SPC5_ETIMER1_TC0IR_HANDLER not defined"
#endif
/**
 * @brief   eTimer1 Channel 0 interrupt handler.
 * @note    It is assumed that the various sources are only activated if the
 *          associated callback pointer is not equal to @p NULL in order to not
 *          perform an extra check in a potentially critical interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(SPC5_ETIMER1_TC0IR_HANDLER) {

  CH_IRQ_PROLOGUE();

  icu_lld_serve_interrupt(&ICUD7);

  CH_IRQ_EPILOGUE();
}
#endif /* SPC5_ICU_USE_SMOD6 */

#if SPC5_ICU_USE_SMOD7
#if !defined(SPC5_ETIMER1_TC1IR_HANDLER)
#error "SPC5_ETIMER1_TC1IR_HANDLER not defined"
#endif
/**
 * @brief   eTimer1 Channel 1 interrupt handler.
 * @note    It is assumed that the various sources are only activated if the
 *          associated callback pointer is not equal to @p NULL in order to not
 *          perform an extra check in a potentially critical interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(SPC5_ETIMER1_TC1IR_HANDLER) {

  CH_IRQ_PROLOGUE();

  icu_lld_serve_interrupt(&ICUD8);

  CH_IRQ_EPILOGUE();
}
#endif /* SPC5_ICU_USE_SMOD7 */

#if SPC5_ICU_USE_SMOD8
#if !defined(SPC5_ETIMER1_TC2IR_HANDLER)
#error "SPC5_ETIMER1_TC2IR_HANDLER not defined"
#endif
/**
 * @brief   eTimer1 Channel 2 interrupt handler.
 * @note    It is assumed that the various sources are only activated if the
 *          associated callback pointer is not equal to @p NULL in order to not
 *          perform an extra check in a potentially critical interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(SPC5_ETIMER1_TC2IR_HANDLER) {

  CH_IRQ_PROLOGUE();

  icu_lld_serve_interrupt(&ICUD9);

  CH_IRQ_EPILOGUE();
}
#endif /* SPC5_ICU_USE_SMOD8 */

#if SPC5_ICU_USE_SMOD9
#if !defined(SPC5_ETIMER1_TC3IR_HANDLER)
#error "SPC5_ETIMER1_TC3IR_HANDLER not defined"
#endif
/**
 * @brief   eTimer1 Channel 3 interrupt handler.
 * @note    It is assumed that the various sources are only activated if the
 *          associated callback pointer is not equal to @p NULL in order to not
 *          perform an extra check in a potentially critical interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(SPC5_ETIMER1_TC3IR_HANDLER) {

  CH_IRQ_PROLOGUE();

  icu_lld_serve_interrupt(&ICUD10);

  CH_IRQ_EPILOGUE();
}
#endif /* SPC5_ICU_USE_SMOD9 */

#if SPC5_ICU_USE_SMOD10
#if !defined(SPC5_ETIMER1_TC4IR_HANDLER)
#error "SPC5_ETIMER1_TC4IR_HANDLER not defined"
#endif
/**
 * @brief   eTimer1 Channel 4 interrupt handler.
 * @note    It is assumed that the various sources are only activated if the
 *          associated callback pointer is not equal to @p NULL in order to not
 *          perform an extra check in a potentially critical interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(SPC5_ETIMER1_TC4IR_HANDLER) {

  CH_IRQ_PROLOGUE();

  icu_lld_serve_interrupt(&ICUD11);

  CH_IRQ_EPILOGUE();
}
#endif /* SPC5_ICU_USE_SMOD10 */

#if SPC5_ICU_USE_SMOD11
#if !defined(SPC5_ETIMER1_TC5IR_HANDLER)
#error "SPC5_ETIMER1_TC5IR_HANDLER not defined"
#endif
/**
 * @brief   eTimer1 Channel 5 interrupt handler.
 * @note    It is assumed that the various sources are only activated if the
 *          associated callback pointer is not equal to @p NULL in order to not
 *          perform an extra check in a potentially critical interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(SPC5_ETIMER1_TC5IR_HANDLER) {

  CH_IRQ_PROLOGUE();

  icu_lld_serve_interrupt(&ICUD12);

  CH_IRQ_EPILOGUE();
}
#endif /* SPC5_ICU_USE_SMOD11 */

#if SPC5_ICU_USE_SMOD12
#if !defined(SPC5_ETIMER2_TC0IR_HANDLER)
#error "SPC5_ETIMER2_TC0IR_HANDLER not defined"
#endif
/**
 * @brief   eTimer2 Channel 0 interrupt handler.
 * @note    It is assumed that the various sources are only activated if the
 *          associated callback pointer is not equal to @p NULL in order to not
 *          perform an extra check in a potentially critical interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(SPC5_ETIMER2_TC0IR_HANDLER) {

  CH_IRQ_PROLOGUE();

  icu_lld_serve_interrupt(&ICUD13);

  CH_IRQ_EPILOGUE();
}
#endif /* SPC5_ICU_USE_SMOD12 */

#if SPC5_ICU_USE_SMOD13
#if !defined(SPC5_ETIMER2_TC1IR_HANDLER)
#error "SPC5_ETIMER2_TC1IR_HANDLER not defined"
#endif
/**
 * @brief   eTimer2 Channel 1 interrupt handler.
 * @note    It is assumed that the various sources are only activated if the
 *          associated callback pointer is not equal to @p NULL in order to not
 *          perform an extra check in a potentially critical interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(SPC5_ETIMER2_TC1IR_HANDLER) {

  CH_IRQ_PROLOGUE();

  icu_lld_serve_interrupt(&ICUD14);

  CH_IRQ_EPILOGUE();
}
#endif /* SPC5_ICU_USE_SMOD13 */

#if SPC5_ICU_USE_SMOD14
#if !defined(SPC5_ETIMER2_TC2IR_HANDLER)
#error "SPC5_ETIMER2_TC2IR_HANDLER not defined"
#endif
/**
 * @brief   eTimer2 Channel 2 interrupt handler.
 * @note    It is assumed that the various sources are only activated if the
 *          associated callback pointer is not equal to @p NULL in order to not
 *          perform an extra check in a potentially critical interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(SPC5_ETIMER2_TC2IR_HANDLER) {

  CH_IRQ_PROLOGUE();

  icu_lld_serve_interrupt(&ICUD15);

  CH_IRQ_EPILOGUE();
}
#endif /* SPC5_ICU_USE_SMOD14 */

#if SPC5_ICU_USE_SMOD15
#if !defined(SPC5_ETIMER2_TC3IR_HANDLER)
#error "SPC5_ETIMER2_TC3IR_HANDLER not defined"
#endif
/**
 * @brief   eTimer2 Channel 3 interrupt handler.
 * @note    It is assumed that the various sources are only activated if the
 *          associated callback pointer is not equal to @p NULL in order to not
 *          perform an extra check in a potentially critical interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(SPC5_ETIMER2_TC3IR_HANDLER) {

  CH_IRQ_PROLOGUE();

  icu_lld_serve_interrupt(&ICUD16);

  CH_IRQ_EPILOGUE();
}
#endif /* SPC5_ICU_USE_SMOD15 */

#if SPC5_ICU_USE_SMOD16
#if !defined(SPC5_ETIMER2_TC4IR_HANDLER)
#error "SPC5_ETIMER2_TC4IR_HANDLER not defined"
#endif
/**
 * @brief   eTimer2 Channel 4 interrupt handler.
 * @note    It is assumed that the various sources are only activated if the
 *          associated callback pointer is not equal to @p NULL in order to not
 *          perform an extra check in a potentially critical interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(SPC5_ETIMER2_TC4IR_HANDLER) {

  CH_IRQ_PROLOGUE();

  icu_lld_serve_interrupt(&ICUD17);

  CH_IRQ_EPILOGUE();
}
#endif /* SPC5_ICU_USE_SMOD16 */

#if SPC5_ICU_USE_SMOD17
#if !defined(SPC5_ETIMER2_TC5IR_HANDLER)
#error "SPC5_ETIMER2_TC5IR_HANDLER not defined"
#endif
/**
 * @brief   eTimer2 Channel 5 interrupt handler.
 * @note    It is assumed that the various sources are only activated if the
 *          associated callback pointer is not equal to @p NULL in order to not
 *          perform an extra check in a potentially critical interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(SPC5_ETIMER2_TC5IR_HANDLER) {

  CH_IRQ_PROLOGUE();

  icu_lld_serve_interrupt(&ICUD18);

  CH_IRQ_EPILOGUE();
}
#endif /* SPC5_ICU_USE_SMOD17 */

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level ICU driver initialization.
 *
 * @notapi
 */
void icu_lld_init(void) {

  /* Submodules initially all not in use.*/
  icu_active_submodules0 = 0;
  icu_active_submodules1 = 0;
  icu_active_submodules2 = 0;

  /* Reset width and period registers.*/
  width = 0;
  period = 0;

#if SPC5_ICU_USE_SMOD0
  /* Driver initialization.*/
  icuObjectInit(&ICUD1);
  ICUD1.etimerp = &SPC5_ETIMER_0;
  ICUD1.smod_number = 0U;
  ICUD1.clock = SPC5_ETIMER0_CLK;
#endif

#if SPC5_ICU_USE_SMOD1
  /* Driver initialization.*/
  icuObjectInit(&ICUD2);
  ICUD2.etimerp = &SPC5_ETIMER_0;
  ICUD2.smod_number = 1U;
  ICUD2.clock = SPC5_ETIMER0_CLK;
#endif

#if SPC5_ICU_USE_SMOD2
  /* Driver initialization.*/
  icuObjectInit(&ICUD3);
  ICUD3.etimerp = &SPC5_ETIMER_0;
  ICUD3.smod_number = 2U;
  ICUD3.clock = SPC5_ETIMER0_CLK;
#endif

#if SPC5_ICU_USE_SMOD3
  /* Driver initialization.*/
  icuObjectInit(&ICUD4);
  ICUD4.etimerp = &SPC5_ETIMER_0;
  ICUD4.smod_number = 3U;
  ICUD4.clock = SPC5_ETIMER0_CLK;
#endif

#if SPC5_ICU_USE_SMOD4
  /* Driver initialization.*/
  icuObjectInit(&ICUD5);
  ICUD5.etimerp = &SPC5_ETIMER_0;
  ICUD5.smod_number = 4U;
  ICUD5.clock = SPC5_ETIMER0_CLK;
#endif

#if SPC5_ICU_USE_SMOD5
  /* Driver initialization.*/
  icuObjectInit(&ICUD6);
  ICUD6.etimerp = &SPC5_ETIMER_0;
  ICUD6.smod_number = 5U;
  ICUD6.clock = SPC5_ETIMER0_CLK;
#endif

#if SPC5_ICU_USE_SMOD6
  /* Driver initialization.*/
  icuObjectInit(&ICUD7);
  ICUD7.etimerp = &SPC5_ETIMER_1;
  ICUD7.smod_number = 0U;
  ICUD7.clock = SPC5_ETIMER1_CLK;
#endif

#if SPC5_ICU_USE_SMOD7
  /* Driver initialization.*/
  icuObjectInit(&ICUD8);
  ICUD8.etimerp = &SPC5_ETIMER_1;
  ICUD8.smod_number = 1U;
  ICUD8.clock = SPC5_ETIMER1_CLK;
#endif

#if SPC5_ICU_USE_SMOD8
  /* Driver initialization.*/
  icuObjectInit(&ICUD9);
  ICUD9.etimerp = &SPC5_ETIMER_1;
  ICUD9.smod_number = 2U;
  ICUD9.clock = SPC5_ETIMER1_CLK;
#endif

#if SPC5_ICU_USE_SMOD9
  /* Driver initialization.*/
  icuObjectInit(&ICUD10);
  ICUD10.etimerp = &SPC5_ETIMER_1;
  ICUD10.smod_number = 3U;
  ICUD10.clock = SPC5_ETIMER1_CLK;
#endif

#if SPC5_ICU_USE_SMOD10
  /* Driver initialization.*/
  icuObjectInit(&ICUD11);
  ICUD11.etimerp = &SPC5_ETIMER_1;
  ICUD11.smod_number = 4U;
  ICUD11.clock = SPC5_ETIMER1_CLK;
#endif

#if SPC5_ICU_USE_SMOD11
  /* Driver initialization.*/
  icuObjectInit(&ICUD12);
  ICUD12.etimerp = &SPC5_ETIMER_1;
  ICUD12.smod_number = 5U;
  ICUD12.clock = SPC5_ETIMER1_CLK;
#endif

#if SPC5_ICU_USE_SMOD12
  /* Driver initialization.*/
  icuObjectInit(&ICUD13);
  ICUD13.etimerp = &SPC5_ETIMER_2;
  ICUD13.smod_number = 0U;
  ICUD13.clock = SPC5_ETIMER2_CLK;
#endif

#if SPC5_ICU_USE_SMOD13
  /* Driver initialization.*/
  icuObjectInit(&ICUD14);
  ICUD14.etimerp = &SPC5_ETIMER_2;
  ICUD14.smod_number = 1U;
  ICUD14.clock = SPC5_ETIMER2_CLK;
#endif

#if SPC5_ICU_USE_SMOD14
  /* Driver initialization.*/
  icuObjectInit(&ICUD15);
  ICUD15.etimerp = &SPC5_ETIMER_2;
  ICUD15.smod_number = 2U;
  ICUD15.clock = SPC5_ETIMER2_CLK;
#endif

#if SPC5_ICU_USE_SMOD15
  /* Driver initialization.*/
  icuObjectInit(&ICUD16);
  ICUD16.etimerp = &SPC5_ETIMER_2;
  ICUD16.smod_number = 3U;
  ICUD16.clock = SPC5_ETIMER2_CLK;
#endif

#if SPC5_ICU_USE_SMOD16
  /* Driver initialization.*/
  icuObjectInit(&ICUD17);
  ICUD17.etimerp = &SPC5_ETIMER_2;
  ICUD17.smod_number = 4U;
  ICUD17.clock = SPC5_ETIMER2_CLK;
#endif

#if SPC5_ICU_USE_SMOD17
  /* Driver initialization.*/
  icuObjectInit(&ICUD18);
  ICUD18.etimerp = &SPC5_ETIMER_2;
  ICUD18.smod_number = 5U;
  ICUD18.clock = SPC5_ETIMER2_CLK;
#endif

#if SPC5_ICU_USE_ETIMER0

  INTC.PSR[SPC5_ETIMER0_TC0IR_NUMBER].R = SPC5_ICU_ETIMER0_PRIORITY;
  INTC.PSR[SPC5_ETIMER0_TC1IR_NUMBER].R = SPC5_ICU_ETIMER0_PRIORITY;
  INTC.PSR[SPC5_ETIMER0_TC2IR_NUMBER].R = SPC5_ICU_ETIMER0_PRIORITY;
  INTC.PSR[SPC5_ETIMER0_TC3IR_NUMBER].R = SPC5_ICU_ETIMER0_PRIORITY;
  INTC.PSR[SPC5_ETIMER0_TC4IR_NUMBER].R = SPC5_ICU_ETIMER0_PRIORITY;
  INTC.PSR[SPC5_ETIMER0_TC5IR_NUMBER].R = SPC5_ICU_ETIMER0_PRIORITY;
  INTC.PSR[SPC5_ETIMER0_WTIF_NUMBER].R = SPC5_ICU_ETIMER0_PRIORITY;
  INTC.PSR[SPC5_ETIMER0_RCF_NUMBER].R = SPC5_ICU_ETIMER0_PRIORITY;

#endif

#if SPC5_ICU_USE_ETIMER1

  INTC.PSR[SPC5_ETIMER1_TC0IR_NUMBER].R = SPC5_ICU_ETIMER1_PRIORITY;
  INTC.PSR[SPC5_ETIMER1_TC1IR_NUMBER].R = SPC5_ICU_ETIMER1_PRIORITY;
  INTC.PSR[SPC5_ETIMER1_TC2IR_NUMBER].R = SPC5_ICU_ETIMER1_PRIORITY;
  INTC.PSR[SPC5_ETIMER1_TC3IR_NUMBER].R = SPC5_ICU_ETIMER1_PRIORITY;
  INTC.PSR[SPC5_ETIMER1_TC4IR_NUMBER].R = SPC5_ICU_ETIMER1_PRIORITY;
  INTC.PSR[SPC5_ETIMER1_TC5IR_NUMBER].R = SPC5_ICU_ETIMER1_PRIORITY;
  INTC.PSR[SPC5_ETIMER1_RCF_NUMBER].R = SPC5_ICU_ETIMER1_PRIORITY;

#endif

#if SPC5_ICU_USE_ETIMER2

  INTC.PSR[SPC5_ETIMER2_TC0IR_NUMBER].R = SPC5_ICU_ETIMER2_PRIORITY;
  INTC.PSR[SPC5_ETIMER2_TC1IR_NUMBER].R = SPC5_ICU_ETIMER2_PRIORITY;
  INTC.PSR[SPC5_ETIMER2_TC2IR_NUMBER].R = SPC5_ICU_ETIMER2_PRIORITY;
  INTC.PSR[SPC5_ETIMER2_TC3IR_NUMBER].R = SPC5_ICU_ETIMER2_PRIORITY;
  INTC.PSR[SPC5_ETIMER2_TC4IR_NUMBER].R = SPC5_ICU_ETIMER2_PRIORITY;
  INTC.PSR[SPC5_ETIMER2_TC5IR_NUMBER].R = SPC5_ICU_ETIMER2_PRIORITY;
  INTC.PSR[SPC5_ETIMER2_RCF_NUMBER].R = SPC5_ICU_ETIMER2_PRIORITY;

#endif
}

/**
 * @brief   Configures and activates the ICU peripheral.
 *
 * @param[in] icup      pointer to the @p ICUDriver object
 *
 * @notapi
 */
void icu_lld_start(ICUDriver *icup) {

  chDbgAssert(icu_active_submodules0 < 6, "icu_lld_start(), #1",
              "too many submodules");
  chDbgAssert(icu_active_submodules1 < 6, "icu_lld_start(), #2",
              "too many submodules");
  chDbgAssert(icu_active_submodules2 < 6, "icu_lld_start(), #3",
              "too many submodules");

  if (icup->state == ICU_STOP) {
#if SPC5_ICU_USE_SMOD0
    if (&ICUD1 == icup)
      icu_active_submodules0++;
#endif
#if SPC5_ICU_USE_SMOD1
    if (&ICUD2 == icup)
      icu_active_submodules0++;
#endif
#if SPC5_ICU_USE_SMOD2
    if (&ICUD3 == icup)
      icu_active_submodules0++;
#endif
#if SPC5_ICU_USE_SMOD3
    if (&ICUD4 == icup)
      icu_active_submodules0++;
#endif
#if SPC5_ICU_USE_SMOD4
    if (&ICUD5 == icup)
      icu_active_submodules0++;
#endif
#if SPC5_ICU_USE_SMOD5
    if (&ICUD6 == icup)
      icu_active_submodules0++;
#endif
#if SPC5_ICU_USE_SMOD6
    if (&ICUD7 == icup)
      icu_active_submodules1++;
#endif
#if SPC5_ICU_USE_SMOD7
    if (&ICUD8 == icup)
      icu_active_submodules1++;
#endif
#if SPC5_ICU_USE_SMOD8
    if (&ICUD9 == icup)
      icu_active_submodules1++;
#endif
#if SPC5_ICU_USE_SMOD9
    if (&ICUD10 == icup)
      icu_active_submodules1++;
#endif
#if SPC5_ICU_USE_SMOD10
    if (&ICUD11 == icup)
      icu_active_submodules1++;
#endif
#if SPC5_ICU_USE_SMOD11
    if (&ICUD12 == icup)
      icu_active_submodules1++;
#endif
#if SPC5_ICU_USE_SMOD12
    if (&ICUD13 == icup)
      icu_active_submodules2++;
#endif
#if SPC5_ICU_USE_SMOD13
    if (&ICUD14 == icup)
      icu_active_submodules2++;
#endif
#if SPC5_ICU_USE_SMOD14
    if (&ICUD15 == icup)
      icu_active_submodules2++;
#endif
#if SPC5_ICU_USE_SMOD15
    if (&ICUD16 == icup)
      icu_active_submodules2++;
#endif
#if SPC5_ICU_USE_SMOD16
    if (&ICUD17 == icup)
      icu_active_submodules2++;
#endif
#if SPC5_ICU_USE_SMOD17
    if (&ICUD18 == icup)
      icu_active_submodules2++;
#endif

    /* Set eTimer0 Clock.*/
#if SPC5_ICU_USE_ETIMER0

    /* If this is the first Submodule activated then the eTimer0 is enabled.*/
    if (icu_active_submodules0 == 1) {
      halSPCSetPeripheralClockMode(SPC5_ETIMER0_PCTL,
                                   SPC5_ICU_ETIMER0_START_PCTL);
    }
#endif

    /* Set eTimer1 Clock.*/
#if SPC5_ICU_USE_ETIMER1
    /* If this is the first Submodule activated then the eTimer1 is enabled.*/
    if (icu_active_submodules1 == 1) {
      halSPCSetPeripheralClockMode(SPC5_ETIMER1_PCTL,
                                   SPC5_ICU_ETIMER1_START_PCTL);
    }
#endif

    /* Set eTimer2 Clock.*/
#if SPC5_ICU_USE_ETIMER2
    /* If this is the first Submodule activated then the eTimer2 is enabled.*/
    if (icu_active_submodules2 == 1) {
      halSPCSetPeripheralClockMode(SPC5_ETIMER2_PCTL,
                                   SPC5_ICU_ETIMER2_START_PCTL);
    }
#endif

    /* Timer disabled.*/
    icup->etimerp->CHANNEL[icup->smod_number].CTRL.B.CNTMODE =
        SPC5_ETIMER_CNTMODE_NO_OPERATION;

    /* Clear pending IRQs (if any).*/
    icup->etimerp->CHANNEL[icup->smod_number].STS.R = 0xFFFF;

    /* All IRQs and DMA requests disabled.*/
    icup->etimerp->CHANNEL[icup->smod_number].INTDMA.R = 0U;

    /* Compare Load 1 and Compare Load 2 disabled.*/
    icup->etimerp->CHANNEL[icup->smod_number].CCCTRL.B.CLC1 = 0U;
    icup->etimerp->CHANNEL[icup->smod_number].CCCTRL.B.CLC2 = 0U;

    /* Capture 1 and Capture 2 disabled.*/
    icup->etimerp->CHANNEL[icup->smod_number].CCCTRL.B.CPT1MODE =
        SPC5_ETIMER_CPT1MODE_DISABLED;
    icup->etimerp->CHANNEL[icup->smod_number].CCCTRL.B.CPT2MODE =
        SPC5_ETIMER_CPT2MODE_DISABLED;

    /* Counter reset to zero.*/
    icup->etimerp->CHANNEL[icup->smod_number].CNTR.R = 0U;
  }

  /* Configuration.*/
  spc5_icu_smod_init(icup);
}

/**
 * @brief   Deactivates the ICU peripheral.
 *
 * @param[in] icup      pointer to the @p ICUDriver object
 *
 * @notapi
 */
void icu_lld_stop(ICUDriver *icup) {

  chDbgAssert(icu_active_submodules0 < 6, "icu_lld_stop(), #1",
              "too many submodules");
  chDbgAssert(icu_active_submodules1 < 6, "icu_lld_stop(), #2",
              "too many submodules");
  chDbgAssert(icu_active_submodules2 < 6, "icu_lld_stop(), #3",
              "too many submodules");

  if (icup->state == ICU_READY) {

#if SPC5_ICU_USE_SMOD0
    if (&ICUD1 == icup) {
      /* Disable channel.*/
      icup->etimerp->ENBL.B.ENBL &= 0xFE;
      icu_active_submodules0--;
    }
#endif
#if SPC5_ICU_USE_SMOD1
    if (&ICUD2 == icup) {
      /* Disable channel.*/
      icup->etimerp->ENBL.B.ENBL &= 0xFD;
      icu_active_submodules0--;
    }
#endif
#if SPC5_ICU_USE_SMOD2
    if (&ICUD3 == icup) {
      /* Disable channel.*/
      icup->etimerp->ENBL.B.ENBL &= 0xFB;
      icu_active_submodules0--;
    }
#endif
#if SPC5_ICU_USE_SMOD3
    if (&ICUD4 == icup) {
      /* Disable channel.*/
      icup->etimerp->ENBL.B.ENBL &= 0xF7;
      icu_active_submodules0--;
    }
#endif
#if SPC5_ICU_USE_SMOD4
    if (&ICUD5 == icup) {
      /* Disable channel.*/
      icup->etimerp->ENBL.B.ENBL &= 0xEF;
      icu_active_submodules0--;
    }
#endif
#if SPC5_ICU_USE_SMOD5
    if (&ICUD6 == icup) {
      /* Disable channel.*/
      icup->etimerp->ENBL.B.ENBL &= 0xDF;
      icu_active_submodules0--;
    }
#endif
#if SPC5_ICU_USE_SMOD6
    if (&ICUD7 == icup) {
      /* Disable channel.*/
      icup->etimerp->ENBL.B.ENBL &= 0xFE;
      icu_active_submodules1--;
    }
#endif
#if SPC5_ICU_USE_SMOD7
    if (&ICUD8 == icup) {
      /* Disable channel.*/
      icup->etimerp->ENBL.B.ENBL &= 0xFD;
      icu_active_submodules1--;
    }
#endif
#if SPC5_ICU_USE_SMOD8
    if (&ICUD9 == icup) {
      /* Disable channel.*/
      icup->etimerp->ENBL.B.ENBL &= 0xFB;
      icu_active_submodules1--;
    }
#endif
#if SPC5_ICU_USE_SMOD9
    if (&ICUD10 == icup) {
      /* Disable channel.*/
      icup->etimerp->ENBL.B.ENBL &= 0xF7;
      icu_active_submodules1--;
    }
#endif
#if SPC5_ICU_USE_SMOD10
    if (&ICUD11 == icup) {
      /* Disable channel.*/
      icup->etimerp->ENBL.B.ENBL &= 0xEF;
      icu_active_submodules1--;
    }
#endif
#if SPC5_ICU_USE_SMOD11
    if (&ICUD12 == icup) {
      /* Disable channel.*/
      icup->etimerp->ENBL.B.ENBL &= 0xDF;
      icu_active_submodules1--;
    }
#endif
#if SPC5_ICU_USE_SMOD12
    if (&ICUD13 == icup) {
      /* Disable channel.*/
      icup->etimerp->ENBL.B.ENBL &= 0xFE;
      icu_active_submodules2--;
    }
#endif
#if SPC5_ICU_USE_SMOD13
    if (&ICUD14 == icup) {
      /* Disable channel.*/
      icup->etimerp->ENBL.B.ENBL &= 0xFD;
      icu_active_submodules2--;
    }
#endif
#if SPC5_ICU_USE_SMOD14
    if (&ICUD15 == icup) {
      /* Disable channel.*/
      icup->etimerp->ENBL.B.ENBL &= 0xFB;
      icu_active_submodules2--;
    }
#endif
#if SPC5_ICU_USE_SMOD15
    if (&ICUD16 == icup) {
      /* Disable channel.*/
      icup->etimerp->ENBL.B.ENBL &= 0xF7;
      icu_active_submodules2--;
    }
#endif
#if SPC5_ICU_USE_SMOD16
    if (&ICUD17 == icup) {
      /* Disable channel.*/
      icup->etimerp->ENBL.B.ENBL &= 0xEF;
      icu_active_submodules2--;
    }
#endif
#if SPC5_ICU_USE_SMOD17
    if (&ICUD18 == icup) {
      /* Disable channel.*/
      icup->etimerp->ENBL.B.ENBL &= 0xDF;
      icu_active_submodules2--;
    }
#endif
    /* eTimer0 clock deactivation.*/
#if SPC5_ICU_USE_ETIMER0
    /* If it is the last active submodules then the eTimer0 is disabled.*/
    if (icu_active_submodules0 == 0) {
      if (icup->etimerp->ENBL.B.ENBL == 0) {
        halSPCSetPeripheralClockMode(SPC5_ETIMER0_PCTL,
                                     SPC5_ICU_ETIMER0_STOP_PCTL);
      }
    }
#endif

    /* eTimer1 clock deactivation.*/
#if SPC5_ICU_USE_ETIMER1
    /* If it is the last active submodules then the eTimer1 is disabled.*/
    if (icu_active_submodules1 == 0) {
      if (icup->etimerp->ENBL.B.ENBL == 0) {
        halSPCSetPeripheralClockMode(SPC5_ETIMER1_PCTL,
                                     SPC5_ICU_ETIMER1_STOP_PCTL);
      }
    }
#endif

    /* eTimer2 clock deactivation.*/
#if SPC5_ICU_USE_ETIMER2
    /* If it is the last active submodules then the eTimer2 is disabled.*/
    if (icu_active_submodules2 == 0) {
      if (icup->etimerp->ENBL.B.ENBL == 0) {
        halSPCSetPeripheralClockMode(SPC5_ETIMER2_PCTL,
                                     SPC5_ICU_ETIMER2_STOP_PCTL);
      }
    }
#endif
  }
}

/**
 * @brief   Enables the input capture.
 *
 * @param[in] icup      pointer to the @p ICUDriver object
 *
 * @notapi
 */
void icu_lld_enable(ICUDriver *icup) {

  /* Clear pending IRQs (if any).*/
  icup->etimerp->CHANNEL[icup->smod_number].STS.R = 0xFFFF;

  /* Set Capture 1 and Capture 2 Mode.*/
  icup->etimerp->CHANNEL[icup->smod_number].CCCTRL.B.CPT1MODE =
      SPC5_ETIMER_CPT1MODE_RISING_EDGE;
  icup->etimerp->CHANNEL[icup->smod_number].CTRL3.B.ROC =
      SPC5_ETIMER_ROC_REL_ON_CAP1;
  icup->etimerp->CHANNEL[icup->smod_number].CCCTRL.B.CPT2MODE =
      SPC5_ETIMER_CPT2MODE_FALLING_EDGE;

  /* Active interrupts.*/
  if (icup->config->period_cb != NULL || icup->config->width_cb != NULL) {
    icup->etimerp->CHANNEL[icup->smod_number].INTDMA.B.ICF1IE = 1U;
    icup->etimerp->CHANNEL[icup->smod_number].INTDMA.B.ICF2IE = 1U;
  }
  if (icup->config->overflow_cb != NULL) {
    icup->etimerp->CHANNEL[icup->smod_number].INTDMA.B.TOFIE = 1U;
  }

  /* Set Capture FIFO Water Mark.*/
  icup->etimerp->CHANNEL[icup->smod_number].CCCTRL.B.CFWM = 0U;

  /* Enable Counter.*/
  if (ICU_SKIP_FIRST_CAPTURE) {
    icup->etimerp->CHANNEL[icup->smod_number].CTRL.B.CNTMODE =
        SPC5_ETIMER_CNTMODE_RFE_SIHA;
  }
  else {
    icup->etimerp->CHANNEL[icup->smod_number].CTRL.B.CNTMODE =
        SPC5_ETIMER_CNTMODE_RE;
  }

  /* Enable Capture process.*/
  icup->etimerp->CHANNEL[icup->smod_number].CCCTRL.B.ARM = 1U;
}

/**
 * @brief   Disables the input capture.
 *
 * @param[in] icup      pointer to the @p ICUDriver object
 *
 * @notapi
 */
void icu_lld_disable(ICUDriver *icup) {

  /* Disable Capture process.*/
  icup->etimerp->CHANNEL[icup->smod_number].CCCTRL.B.ARM = 0U;

  /* Clear pending IRQs (if any).*/
  icup->etimerp->CHANNEL[icup->smod_number].STS.R = 0xFFFF;

  /* Set Capture 1 and Capture 2 Mode to Disabled.*/
  icup->etimerp->CHANNEL[icup->smod_number].CCCTRL.B.CPT1MODE =
      SPC5_ETIMER_CPT1MODE_DISABLED;
  icup->etimerp->CHANNEL[icup->smod_number].CCCTRL.B.CPT2MODE =
      SPC5_ETIMER_CPT2MODE_DISABLED;

  /* Disable interrupts.*/
  if (icup->config->period_cb != NULL || icup->config->width_cb != NULL) {
    icup->etimerp->CHANNEL[icup->smod_number].INTDMA.B.ICF1IE = 0U;
    icup->etimerp->CHANNEL[icup->smod_number].INTDMA.B.ICF2IE = 0U;
  }
  if (icup->config->overflow_cb != NULL)
    icup->etimerp->CHANNEL[icup->smod_number].INTDMA.B.TOFIE = 0U;
}

#endif /* HAL_USE_ICU */

/** @} */
