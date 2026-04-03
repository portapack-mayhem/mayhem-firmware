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
 * @file    STM32F0xx/ext_lld_isr.c
 * @brief   STM32F0xx EXT subsystem low level driver ISR code.
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
/* Driver local variables.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/**
 * @brief   EXTI[0]...EXTI[1] interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(Vector54) {
  uint32_t pr;

  CH_IRQ_PROLOGUE();

  pr = EXTI->PR & ((1 << 0) | (1 << 1));
  EXTI->PR = pr;
  if (pr & (1 << 0))
    EXTD1.config->channels[0].cb(&EXTD1, 0);
  if (pr & (1 << 1))
    EXTD1.config->channels[1].cb(&EXTD1, 1);

  CH_IRQ_EPILOGUE();
}

/**
 * @brief   EXTI[2]...EXTI[3] interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(Vector58) {
  uint32_t pr;

  CH_IRQ_PROLOGUE();

  pr = EXTI->PR & ((1 << 2) | (1 << 3));
  EXTI->PR = pr;
  if (pr & (1 << 2))
    EXTD1.config->channels[2].cb(&EXTD1, 2);
  if (pr & (1 << 3))
    EXTD1.config->channels[3].cb(&EXTD1, 3);

  CH_IRQ_EPILOGUE();
}

/**
 * @brief   EXTI[4]...EXTI[15] interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(Vector5C) {
  uint32_t pr;

  CH_IRQ_PROLOGUE();

  pr = EXTI->PR & ((1 << 4)  | (1 << 5)  | (1 << 6)  | (1 << 7)  | (1 << 8)  |
                   (1 << 9)  | (1 << 10) | (1 << 11) | (1 << 12) | (1 << 13) |
                   (1 << 14) | (1 << 15));
  EXTI->PR = pr;
  if (pr & (1 << 4))
    EXTD1.config->channels[4].cb(&EXTD1, 4);
  if (pr & (1 << 5))
    EXTD1.config->channels[5].cb(&EXTD1, 5);
  if (pr & (1 << 6))
    EXTD1.config->channels[6].cb(&EXTD1, 6);
  if (pr & (1 << 7))
    EXTD1.config->channels[7].cb(&EXTD1, 7);
  if (pr & (1 << 8))
    EXTD1.config->channels[8].cb(&EXTD1, 8);
  if (pr & (1 << 9))
    EXTD1.config->channels[9].cb(&EXTD1, 9);
  if (pr & (1 << 10))
    EXTD1.config->channels[10].cb(&EXTD1, 10);
  if (pr & (1 << 11))
    EXTD1.config->channels[11].cb(&EXTD1, 11);
  if (pr & (1 << 12))
    EXTD1.config->channels[12].cb(&EXTD1, 12);
  if (pr & (1 << 13))
    EXTD1.config->channels[13].cb(&EXTD1, 13);
  if (pr & (1 << 14))
    EXTD1.config->channels[14].cb(&EXTD1, 14);
  if (pr & (1 << 15))
    EXTD1.config->channels[15].cb(&EXTD1, 15);

  CH_IRQ_EPILOGUE();
}

/**
 * @brief   EXTI[16] interrupt handler (PVD).
 *
 * @isr
 */
CH_IRQ_HANDLER(Vector44) {

  CH_IRQ_PROLOGUE();

  EXTI->PR = (1 << 16);
  EXTD1.config->channels[16].cb(&EXTD1, 16);

  CH_IRQ_EPILOGUE();
}

/**
 * @brief   EXTI[17] interrupt handler (RTC).
 *
 * @isr
 */
CH_IRQ_HANDLER(Vector48) {

  CH_IRQ_PROLOGUE();

  EXTI->PR = (1 << 17);
  EXTD1.config->channels[17].cb(&EXTD1, 17);

  CH_IRQ_EPILOGUE();
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Enables EXTI IRQ sources.
 *
 * @notapi
 */
void ext_lld_exti_irq_enable(void) {

  nvicEnableVector(EXTI0_1_IRQn,
                   CORTEX_PRIORITY_MASK(STM32_EXT_EXTI0_1_IRQ_PRIORITY));
  nvicEnableVector(EXTI2_3_IRQn,
                   CORTEX_PRIORITY_MASK(STM32_EXT_EXTI2_3_IRQ_PRIORITY));
  nvicEnableVector(EXTI4_15_IRQn,
                   CORTEX_PRIORITY_MASK(STM32_EXT_EXTI4_15_IRQ_PRIORITY));
#if !defined(STM32F030)
  nvicEnableVector(PVD_IRQn,
                   CORTEX_PRIORITY_MASK(STM32_EXT_EXTI16_IRQ_PRIORITY));
#endif
  nvicEnableVector(RTC_IRQn,
                   CORTEX_PRIORITY_MASK(STM32_EXT_EXTI17_IRQ_PRIORITY));
}

/**
 * @brief   Disables EXTI IRQ sources.
 *
 * @notapi
 */
void ext_lld_exti_irq_disable(void) {

  nvicDisableVector(EXTI0_1_IRQn);
  nvicDisableVector(EXTI2_3_IRQn);
  nvicDisableVector(EXTI4_15_IRQn);
#if !defined(STM32F030)
  nvicDisableVector(PVD_IRQn);
#endif
  nvicDisableVector(RTC_IRQn);
}

#endif /* HAL_USE_EXT */

/** @} */
