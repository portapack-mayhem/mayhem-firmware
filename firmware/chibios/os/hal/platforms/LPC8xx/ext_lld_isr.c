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
 * @file    LPC8xx/ext_lld_isr.c
 * @brief   LPC8xx EXT subsystem low level driver ISR code.
 *
 * @addtogroup EXT
 * @{
 */

#include "ch.h"
#include "hal.h"

#if HAL_USE_EXT || defined(__DOXYGEN__)

#include "ext_lld_isr.h"

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/
static void ext_lld_interrupt( uint32_t n ) {
  uint32_t reason;

  reason =  ((LPC_PIN_INT->RISE)>> n   ) & 0x01;
  reason |= ((LPC_PIN_INT->FALL)>>(n-1)) & 0x02;
  LPC_PIN_INT->RISE = (1<<n);
  LPC_PIN_INT->FALL = (1<<n);
  LPC_PIN_INT->IST  = (1<<n);
  EXTD1.config->channels[n].cb(&EXTD1, n, reason);
}

/**
 * @brief   EXT[0] interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(VectorA0) {

  CH_IRQ_PROLOGUE();
  ext_lld_interrupt(0);
  CH_IRQ_EPILOGUE();
}

/**
 * @brief   EXT[1] interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(VectorA4) {

  CH_IRQ_PROLOGUE();
  ext_lld_interrupt(1);
  CH_IRQ_EPILOGUE();
}

/**
 * @brief   EXT[2] interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(VectorA8) {

  CH_IRQ_PROLOGUE();
  ext_lld_interrupt(2);
  CH_IRQ_EPILOGUE();
}

/**
 * @brief   EXT[3] interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(VectorAC) {

  CH_IRQ_PROLOGUE();
  ext_lld_interrupt(3);
  CH_IRQ_EPILOGUE();
}

/**
 * @brief   EXT[4] interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(VectorB0) {

  CH_IRQ_PROLOGUE();
  ext_lld_interrupt(4);
  CH_IRQ_EPILOGUE();
}

/**
 * @brief   EXT[5] interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(VectorB4) {

  CH_IRQ_PROLOGUE();
  ext_lld_interrupt(5);
  CH_IRQ_EPILOGUE();
}

/**
 * @brief   EXT[6] interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(VectorB8) {

  CH_IRQ_PROLOGUE();
  ext_lld_interrupt(6);
  CH_IRQ_EPILOGUE();
}

/**
 * @brief   EXT[7] interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(VectorBC) {

  CH_IRQ_PROLOGUE();
  ext_lld_interrupt(7);
  CH_IRQ_EPILOGUE();
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/
static const uint8_t LPC8xx_EXT_EXTIn_IRQ_PRIORITY[] =
    { LPC8xx_EXT_EXTI0_IRQ_PRIORITY,
      LPC8xx_EXT_EXTI1_IRQ_PRIORITY,
      LPC8xx_EXT_EXTI2_IRQ_PRIORITY,
      LPC8xx_EXT_EXTI3_IRQ_PRIORITY,
      LPC8xx_EXT_EXTI4_IRQ_PRIORITY,
      LPC8xx_EXT_EXTI5_IRQ_PRIORITY,
      LPC8xx_EXT_EXTI6_IRQ_PRIORITY,
      LPC8xx_EXT_EXTI7_IRQ_PRIORITY };

/**
 * @brief   Enables EXTI IRQ sources.
 *
 * @notapi
 */
void ext_lld_exti_irq_enable( uint32_t exti_n ) {

  nvicEnableVector(PININT0_IRQn + exti_n,
                   CORTEX_PRIORITY_MASK(LPC8xx_EXT_EXTIn_IRQ_PRIORITY[exti_n]));
}

/**
 * @brief   Disables EXTI IRQ sources.
 *
 * @notapi
 */
void ext_lld_exti_irq_disable( uint32_t exti_n ) {

  nvicDisableVector(PININT0_IRQn + exti_n);
}

#endif /* HAL_USE_EXT */

/** @} */
