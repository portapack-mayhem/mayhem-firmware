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
 * @file    STM32F37x/ext_lld_isr.c
 * @brief   STM32F37x EXT subsystem low level driver ISR code.
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

#if !defined(STM32_DISABLE_EXTI0_HANDLER)
/**
 * @brief   EXTI[0] interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(Vector58) {

  CH_IRQ_PROLOGUE();

  EXTI->PR = (1 << 0);
  EXTD1.config->channels[0].cb(&EXTD1, 0);

  CH_IRQ_EPILOGUE();
}
#endif

#if !defined(STM32_DISABLE_EXTI1_HANDLER)
/**
 * @brief   EXTI[1] interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(Vector5C) {

  CH_IRQ_PROLOGUE();

  EXTI->PR = (1 << 1);
  EXTD1.config->channels[1].cb(&EXTD1, 1);

  CH_IRQ_EPILOGUE();
}
#endif

#if !defined(STM32_DISABLE_EXTI2_HANDLER)
/**
 * @brief   EXTI[2] interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(Vector60) {

  CH_IRQ_PROLOGUE();

  EXTI->PR = (1 << 2);
  EXTD1.config->channels[2].cb(&EXTD1, 2);

  CH_IRQ_EPILOGUE();
}
#endif

#if !defined(STM32_DISABLE_EXTI3_HANDLER)
/**
 * @brief   EXTI[3] interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(Vector64) {

  CH_IRQ_PROLOGUE();

  EXTI->PR = (1 << 3);
  EXTD1.config->channels[3].cb(&EXTD1, 3);

  CH_IRQ_EPILOGUE();
}
#endif

#if !defined(STM32_DISABLE_EXTI4_HANDLER)
/**
 * @brief   EXTI[4] interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(Vector68) {

  CH_IRQ_PROLOGUE();

  EXTI->PR = (1 << 4);
  EXTD1.config->channels[4].cb(&EXTD1, 4);

  CH_IRQ_EPILOGUE();
}
#endif

#if !defined(STM32_DISABLE_EXTI5_9_HANDLER)
/**
 * @brief   EXTI[5]...EXTI[9] interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(Vector9C) {
  uint32_t pr;

  CH_IRQ_PROLOGUE();

  pr = EXTI->PR & ((1 << 5) | (1 << 6) | (1 << 7) | (1 << 8) | (1 << 9));
  EXTI->PR = pr;
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

  CH_IRQ_EPILOGUE();
}
#endif

#if !defined(STM32_DISABLE_EXTI10_15_HANDLER)
/**
 * @brief   EXTI[10]...EXTI[15] interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(VectorE0) {
  uint32_t pr;

  CH_IRQ_PROLOGUE();

  pr = EXTI->PR & ((1 << 10) | (1 << 11) | (1 << 12) | (1 << 13) | (1 << 14) |
                   (1 << 15));
  EXTI->PR = pr;
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
#endif

#if !defined(STM32_DISABLE_EXTI16_HANDLER)
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
#endif

#if !defined(STM32_DISABLE_EXTI17_HANDLER)
/**
 * @brief   EXTI[17] interrupt handler (RTC Alarm).
 *
 * @isr
 */
CH_IRQ_HANDLER(VectorE4) {

  CH_IRQ_PROLOGUE();

  EXTI->PR = (1 << 17);
  EXTD1.config->channels[17].cb(&EXTD1, 17);

  CH_IRQ_EPILOGUE();
}
#endif

#if !defined(STM32_DISABLE_EXTI18_HANDLER)
/**
 * @brief   EXTI[18] interrupt handler (USB Wakeup).
 *
 * @isr
 */
CH_IRQ_HANDLER(VectorE8) {

  CH_IRQ_PROLOGUE();

  EXTI->PR = (1 << 18);
  EXTD1.config->channels[18].cb(&EXTD1, 18);

  CH_IRQ_EPILOGUE();
}
#endif

#if !defined(STM32_DISABLE_EXTI19_HANDLER)
/**
 * @brief   EXTI[19] interrupt handler (Tamper TimeStamp).
 *
 * @isr
 */
CH_IRQ_HANDLER(Vector48) {

  CH_IRQ_PROLOGUE();

  EXTI->PR = (1 << 19);
  EXTD1.config->channels[19].cb(&EXTD1, 19);

  CH_IRQ_EPILOGUE();
}
#endif

#if !defined(STM32_DISABLE_EXTI20_HANDLER)
/**
 * @brief   EXTI[20] interrupt handler (RTC Wakeup).
 *
 * @isr
 */
CH_IRQ_HANDLER(Vector4C) {

  CH_IRQ_PROLOGUE();

  EXTI->PR = (1 << 20);
  EXTD1.config->channels[20].cb(&EXTD1, 20);

  CH_IRQ_EPILOGUE();
}
#endif

#if !defined(STM32_DISABLE_EXTI21_22_HANDLER)
/**
 * @brief   EXTI[21]..EXTI[22] interrupt handler (COMP1, COMP2).
 *
 * @isr
 */
CH_IRQ_HANDLER(Vector140) {
  uint32_t pr;

  CH_IRQ_PROLOGUE();

  pr = EXTI->PR & ((1 << 21) | (1 << 22));
  EXTI->PR = pr;
  if (pr & (1 << 21))
    EXTD1.config->channels[21].cb(&EXTD1, 21);
  if (pr & (1 << 22))
    EXTD1.config->channels[22].cb(&EXTD1, 22);

  CH_IRQ_EPILOGUE();
}
#endif

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Enables EXTI IRQ sources.
 *
 * @notapi
 */
void ext_lld_exti_irq_enable(void) {

  nvicEnableVector(EXTI0_IRQn,
                   CORTEX_PRIORITY_MASK(STM32_EXT_EXTI0_IRQ_PRIORITY));
  nvicEnableVector(EXTI1_IRQn,
                   CORTEX_PRIORITY_MASK(STM32_EXT_EXTI1_IRQ_PRIORITY));
  nvicEnableVector(EXTI2_TS_IRQn,
                   CORTEX_PRIORITY_MASK(STM32_EXT_EXTI2_IRQ_PRIORITY));
  nvicEnableVector(EXTI3_IRQn,
                   CORTEX_PRIORITY_MASK(STM32_EXT_EXTI3_IRQ_PRIORITY));
  nvicEnableVector(EXTI4_IRQn,
                   CORTEX_PRIORITY_MASK(STM32_EXT_EXTI4_IRQ_PRIORITY));
  nvicEnableVector(EXTI9_5_IRQn,
                   CORTEX_PRIORITY_MASK(STM32_EXT_EXTI5_9_IRQ_PRIORITY));
  nvicEnableVector(EXTI15_10_IRQn,
                   CORTEX_PRIORITY_MASK(STM32_EXT_EXTI10_15_IRQ_PRIORITY));
  nvicEnableVector(PVD_IRQn,
                   CORTEX_PRIORITY_MASK(STM32_EXT_EXTI16_IRQ_PRIORITY));
  nvicEnableVector(RTC_Alarm_IRQn,
                   CORTEX_PRIORITY_MASK(STM32_EXT_EXTI17_IRQ_PRIORITY));
  nvicEnableVector(USBWakeUp_IRQn,
                   CORTEX_PRIORITY_MASK(STM32_EXT_EXTI18_IRQ_PRIORITY));
  nvicEnableVector(TAMPER_STAMP_IRQn,
                   CORTEX_PRIORITY_MASK(STM32_EXT_EXTI19_IRQ_PRIORITY));
  nvicEnableVector(RTC_WKUP_IRQn,
                   CORTEX_PRIORITY_MASK(STM32_EXT_EXTI20_IRQ_PRIORITY));
  nvicEnableVector(COMP_IRQn,
                   CORTEX_PRIORITY_MASK(STM32_EXT_EXTI21_22_IRQ_PRIORITY));
}

/**
 * @brief   Disables EXTI IRQ sources.
 *
 * @notapi
 */
void ext_lld_exti_irq_disable(void) {

  nvicDisableVector(EXTI0_IRQn);
  nvicDisableVector(EXTI1_IRQn);
  nvicDisableVector(EXTI2_TS_IRQn);
  nvicDisableVector(EXTI3_IRQn);
  nvicDisableVector(EXTI4_IRQn);
  nvicDisableVector(EXTI9_5_IRQn);
  nvicDisableVector(EXTI15_10_IRQn);
  nvicDisableVector(PVD_IRQn);
  nvicDisableVector(RTC_Alarm_IRQn);
  nvicDisableVector(USBWakeUp_IRQn);
  nvicDisableVector(TAMPER_STAMP_IRQn);
  nvicDisableVector(RTC_WKUP_IRQn);
  nvicDisableVector(COMP_IRQn);
}

#endif /* HAL_USE_EXT */

/** @} */
