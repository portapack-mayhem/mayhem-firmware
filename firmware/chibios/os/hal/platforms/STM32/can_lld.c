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
 * @file    STM32/can_lld.c
 * @brief   STM32 CAN subsystem low level driver source.
 *
 * @addtogroup CAN
 * @{
 */

#include "ch.h"
#include "hal.h"

#if HAL_USE_CAN || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/** @brief CAN1 driver identifier.*/
#if STM32_CAN_USE_CAN1 || defined(__DOXYGEN__)
CANDriver CAND1;
#endif

/** @brief CAN2 driver identifier.*/
#if STM32_CAN_USE_CAN2 || defined(__DOXYGEN__)
CANDriver CAND2;
#endif

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/**
 * @brief   Programs the filters.
 *
 * @param[in] can2sb    number of the first filter assigned to CAN2
 * @param[in] num       number of entries in the filters array, if zero then
 *                      a default filter is programmed
 * @param[in] cfp       pointer to the filters array, can be @p NULL if
 *                      (num == 0)
 *
 * @notapi
 */
static void can_lld_set_filters(uint32_t can2sb,
                                uint32_t num,
                                const CANFilter *cfp) {

  /* Temporarily enabling CAN1 clock.*/
  rccEnableCAN1(FALSE);

  /* Filters initialization.*/
  CAN1->FMR = (CAN1->FMR & 0xFFFF0000) | (can2sb << 8) | CAN_FMR_FINIT;
  if (num > 0) {
    uint32_t i, fmask;

    /* All filters cleared.*/
    CAN1->FA1R = 0;
    CAN1->FM1R = 0;
    CAN1->FS1R = 0;
    CAN1->FFA1R = 0;
    for (i = 0; i < STM32_CAN_MAX_FILTERS; i++) {
      CAN1->sFilterRegister[i].FR1 = 0;
      CAN1->sFilterRegister[i].FR2 = 0;
    }

    /* Scanning the filters array.*/
    for (i = 0; i < num; i++) {
      fmask = 1 << cfp->filter;
      if (cfp->mode)
        CAN1->FM1R |= fmask;
      if (cfp->scale)
        CAN1->FS1R |= fmask;
      if (cfp->assignment)
        CAN1->FFA1R |= fmask;
      CAN1->sFilterRegister[cfp->filter].FR1 = cfp->register1;
      CAN1->sFilterRegister[cfp->filter].FR2 = cfp->register2;
      CAN1->FA1R |= fmask;
      cfp++;
    }
  }
  else {
    /* Setting up a single default filter that enables everything for both
       CANs.*/
    CAN1->sFilterRegister[0].FR1 = 0;
    CAN1->sFilterRegister[0].FR2 = 0;
#if STM32_HAS_CAN2
    CAN1->sFilterRegister[can2sb].FR1 = 0;
    CAN1->sFilterRegister[can2sb].FR2 = 0;
#endif
    CAN1->FM1R = 0;
    CAN1->FFA1R = 0;
#if STM32_HAS_CAN2
    CAN1->FS1R = 1 | (1 << can2sb);
    CAN1->FA1R = 1 | (1 << can2sb);
#else
    CAN1->FS1R = 1;
    CAN1->FA1R = 1;
#endif
  }
  CAN1->FMR &= ~CAN_FMR_FINIT;

  /* Clock disabled, it will be enabled again in can_lld_start().*/
  rccDisableCAN1(FALSE);
}

/**
 * @brief   Common TX ISR handler.
 *
 * @param[in] canp      pointer to the @p CANDriver object
 *
 * @notapi
 */
static void can_lld_tx_handler(CANDriver *canp) {

  /* No more events until a message is transmitted.*/
  canp->can->TSR = CAN_TSR_RQCP0 | CAN_TSR_RQCP1 | CAN_TSR_RQCP2;
  chSysLockFromIsr();
  while (chSemGetCounterI(&canp->txsem) < 0)
    chSemSignalI(&canp->txsem);
  chEvtBroadcastFlagsI(&canp->txempty_event, CAN_MAILBOX_TO_MASK(1));
  chSysUnlockFromIsr();
}

/**
 * @brief   Common RX0 ISR handler.
 *
 * @param[in] canp      pointer to the @p CANDriver object
 *
 * @notapi
 */
static void can_lld_rx0_handler(CANDriver *canp) {
  uint32_t rf0r;

  rf0r = canp->can->RF0R;
  if ((rf0r & CAN_RF0R_FMP0) > 0) {
    /* No more receive events until the queue 0 has been emptied.*/
    canp->can->IER &= ~CAN_IER_FMPIE0;
    chSysLockFromIsr();
    while (chSemGetCounterI(&canp->rxsem) < 0)
      chSemSignalI(&canp->rxsem);
    chEvtBroadcastFlagsI(&canp->rxfull_event, CAN_MAILBOX_TO_MASK(1));
    chSysUnlockFromIsr();
  }
  if ((rf0r & CAN_RF0R_FOVR0) > 0) {
    /* Overflow events handling.*/
    canp->can->RF0R = CAN_RF0R_FOVR0;
    chSysLockFromIsr();
    chEvtBroadcastFlagsI(&canp->error_event, CAN_OVERFLOW_ERROR);
    chSysUnlockFromIsr();
  }
}

/**
 * @brief   Common RX1 ISR handler.
 *
 * @param[in] canp      pointer to the @p CANDriver object
 *
 * @notapi
 */
static void can_lld_rx1_handler(CANDriver *canp) {
  uint32_t rf1r;

  rf1r = canp->can->RF1R;
  if ((rf1r & CAN_RF1R_FMP1) > 0) {
    /* No more receive events until the queue 0 has been emptied.*/
    canp->can->IER &= ~CAN_IER_FMPIE1;
    chSysLockFromIsr();
    while (chSemGetCounterI(&canp->rxsem) < 0)
      chSemSignalI(&canp->rxsem);
    chEvtBroadcastFlagsI(&canp->rxfull_event, CAN_MAILBOX_TO_MASK(2));
    chSysUnlockFromIsr();
  }
  if ((rf1r & CAN_RF1R_FOVR1) > 0) {
    /* Overflow events handling.*/
    canp->can->RF1R = CAN_RF1R_FOVR1;
    chSysLockFromIsr();
    chEvtBroadcastFlagsI(&canp->error_event, CAN_OVERFLOW_ERROR);
    chSysUnlockFromIsr();
  }
}

/**
 * @brief   Common SCE ISR handler.
 *
 * @param[in] canp      pointer to the @p CANDriver object
 *
 * @notapi
 */
static void can_lld_sce_handler(CANDriver *canp) {
  uint32_t msr;

  msr = canp->can->MSR;
  canp->can->MSR = CAN_MSR_ERRI | CAN_MSR_WKUI | CAN_MSR_SLAKI;
  /* Wakeup event.*/
#if CAN_USE_SLEEP_MODE
  if (msr & CAN_MSR_WKUI) {
    canp->state = CAN_READY;
    canp->can->MCR &= ~CAN_MCR_SLEEP;
    chSysLockFromIsr();
    chEvtBroadcastI(&canp->wakeup_event);
    chSysUnlockFromIsr();
  }
#endif /* CAN_USE_SLEEP_MODE */
  /* Error event.*/
  if (msr & CAN_MSR_ERRI) {
    flagsmask_t flags;
    uint32_t esr = canp->can->ESR;

    canp->can->ESR &= ~CAN_ESR_LEC;
    flags = (flagsmask_t)(esr & 7);
    if ((esr & CAN_ESR_LEC) > 0)
      flags |= CAN_FRAMING_ERROR;

    chSysLockFromIsr();
    /* The content of the ESR register is copied unchanged in the upper
       half word of the listener flags mask.*/
    chEvtBroadcastFlagsI(&canp->error_event, flags | (flagsmask_t)(esr << 16));
    chSysUnlockFromIsr();
  }
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

#if STM32_CAN_USE_CAN1 || defined(__DOXYGEN__)
/**
 * @brief   CAN1 TX interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(STM32_CAN1_TX_HANDLER) {

  CH_IRQ_PROLOGUE();

  can_lld_tx_handler(&CAND1);

  CH_IRQ_EPILOGUE();
}

/*
 * @brief   CAN1 RX0 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(STM32_CAN1_RX0_HANDLER) {

  CH_IRQ_PROLOGUE();

  can_lld_rx0_handler(&CAND1);

  CH_IRQ_EPILOGUE();
}

/**
 * @brief   CAN1 RX1 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(STM32_CAN1_RX1_HANDLER) {

  CH_IRQ_PROLOGUE();

  can_lld_rx1_handler(&CAND1);

  CH_IRQ_EPILOGUE();
}

/**
 * @brief   CAN1 SCE interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(STM32_CAN1_SCE_HANDLER) {

  CH_IRQ_PROLOGUE();

  can_lld_sce_handler(&CAND1);

  CH_IRQ_EPILOGUE();
}
#endif /* STM32_CAN_USE_CAN1 */

#if STM32_CAN_USE_CAN2 || defined(__DOXYGEN__)
/**
 * @brief   CAN2 TX interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(STM32_CAN2_TX_HANDLER) {

  CH_IRQ_PROLOGUE();

  can_lld_tx_handler(&CAND2);

  CH_IRQ_EPILOGUE();
}

/*
 * @brief   CAN2 RX0 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(STM32_CAN2_RX0_HANDLER) {

  CH_IRQ_PROLOGUE();

  can_lld_rx0_handler(&CAND2);

  CH_IRQ_EPILOGUE();
}

/**
 * @brief   CAN2 RX1 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(STM32_CAN2_RX1_HANDLER) {

  CH_IRQ_PROLOGUE();

  can_lld_rx1_handler(&CAND2);

  CH_IRQ_EPILOGUE();
}

/**
 * @brief   CAN2 SCE interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(STM32_CAN2_SCE_HANDLER) {

  CH_IRQ_PROLOGUE();

  can_lld_sce_handler(&CAND2);

  CH_IRQ_EPILOGUE();
}
#endif /* STM32_CAN_USE_CAN2 */

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level CAN driver initialization.
 *
 * @notapi
 */
void can_lld_init(void) {

#if STM32_CAN_USE_CAN1
  /* Driver initialization.*/
  canObjectInit(&CAND1);
  CAND1.can = CAN1;
#endif
#if STM32_CAN_USE_CAN2
  /* Driver initialization.*/
  canObjectInit(&CAND2);
  CAND2.can = CAN2;
#endif

  /* Filters initialization.*/
#if STM32_HAS_CAN2
  can_lld_set_filters(STM32_CAN_MAX_FILTERS / 2, 0, NULL);
#else
  can_lld_set_filters(STM32_CAN_MAX_FILTERS, 0, NULL);
#endif
}

/**
 * @brief   Configures and activates the CAN peripheral.
 *
 * @param[in] canp      pointer to the @p CANDriver object
 *
 * @notapi
 */
void can_lld_start(CANDriver *canp) {

  /* Clock activation.*/
#if STM32_CAN_USE_CAN1
  if (&CAND1 == canp) {
    nvicEnableVector(STM32_CAN1_TX_NUMBER,
                     CORTEX_PRIORITY_MASK(STM32_CAN_CAN1_IRQ_PRIORITY));
    nvicEnableVector(STM32_CAN1_RX0_NUMBER,
                     CORTEX_PRIORITY_MASK(STM32_CAN_CAN1_IRQ_PRIORITY));
    nvicEnableVector(STM32_CAN1_RX1_NUMBER,
                     CORTEX_PRIORITY_MASK(STM32_CAN_CAN1_IRQ_PRIORITY));
    nvicEnableVector(STM32_CAN1_SCE_NUMBER,
                     CORTEX_PRIORITY_MASK(STM32_CAN_CAN1_IRQ_PRIORITY));
    rccEnableCAN1(FALSE);
  }
#endif
#if STM32_CAN_USE_CAN2
  if (&CAND2 == canp) {

    chDbgAssert(CAND1.state != CAN_STOP,
                "can_lld_start(), #1", "CAN1 must be started");

    nvicEnableVector(STM32_CAN2_TX_NUMBER,
                     CORTEX_PRIORITY_MASK(STM32_CAN_CAN2_IRQ_PRIORITY));
    nvicEnableVector(STM32_CAN2_RX0_NUMBER,
                     CORTEX_PRIORITY_MASK(STM32_CAN_CAN2_IRQ_PRIORITY));
    nvicEnableVector(STM32_CAN2_RX1_NUMBER,
                     CORTEX_PRIORITY_MASK(STM32_CAN_CAN2_IRQ_PRIORITY));
    nvicEnableVector(STM32_CAN2_SCE_NUMBER,
                     CORTEX_PRIORITY_MASK(STM32_CAN_CAN2_IRQ_PRIORITY));
    rccEnableCAN2(FALSE);
  }
#endif

  /* Entering initialization mode. */
  canp->state = CAN_STARTING;
  canp->can->MCR = CAN_MCR_INRQ;
  while ((canp->can->MSR & CAN_MSR_INAK) == 0)
    chThdSleepS(1);
  /* BTR initialization.*/
  canp->can->BTR = canp->config->btr;
  /* MCR initialization.*/
  canp->can->MCR = canp->config->mcr;

  /* Interrupt sources initialization.*/
  canp->can->IER = CAN_IER_TMEIE  | CAN_IER_FMPIE0 | CAN_IER_FMPIE1 |
                   CAN_IER_WKUIE  | CAN_IER_ERRIE  | CAN_IER_LECIE  |
                   CAN_IER_BOFIE  | CAN_IER_EPVIE  | CAN_IER_EWGIE  |
                   CAN_IER_FOVIE0 | CAN_IER_FOVIE1;
}

/**
 * @brief   Deactivates the CAN peripheral.
 *
 * @param[in] canp      pointer to the @p CANDriver object
 *
 * @notapi
 */
void can_lld_stop(CANDriver *canp) {

  /* If in ready state then disables the CAN peripheral.*/
  if (canp->state == CAN_READY) {
#if STM32_CAN_USE_CAN1
    if (&CAND1 == canp) {

#if STM32_CAN_USE_CAN2
      chDbgAssert(CAND2.state == CAN_STOP,
                  "can_lld_stop(), #1", "CAN2 must be stopped");
#endif

      CAN1->MCR = 0x00010002;                   /* Register reset value.    */
      CAN1->IER = 0x00000000;                   /* All sources disabled.    */
      nvicDisableVector(STM32_CAN1_TX_NUMBER);
      nvicDisableVector(STM32_CAN1_RX0_NUMBER);
      nvicDisableVector(STM32_CAN1_RX1_NUMBER);
      nvicDisableVector(STM32_CAN1_SCE_NUMBER);
      rccDisableCAN1(FALSE);
    }
#endif
#if STM32_CAN_USE_CAN2
    if (&CAND2 == canp) {
      CAN2->MCR = 0x00010002;                   /* Register reset value.    */
      CAN2->IER = 0x00000000;                   /* All sources disabled.    */
      nvicDisableVector(STM32_CAN2_TX_NUMBER);
      nvicDisableVector(STM32_CAN2_RX0_NUMBER);
      nvicDisableVector(STM32_CAN2_RX1_NUMBER);
      nvicDisableVector(STM32_CAN2_SCE_NUMBER);
      rccDisableCAN2(FALSE);
    }
#endif
  }
}

/**
 * @brief   Determines whether a frame can be transmitted.
 *
 * @param[in] canp      pointer to the @p CANDriver object
 * @param[in] mailbox   mailbox number, @p CAN_ANY_MAILBOX for any mailbox
 *
 * @return              The queue space availability.
 * @retval FALSE        no space in the transmit queue.
 * @retval TRUE         transmit slot available.
 *
 * @notapi
 */
bool_t can_lld_is_tx_empty(CANDriver *canp, canmbx_t mailbox) {

  switch (mailbox) {
  case CAN_ANY_MAILBOX:
    return (canp->can->TSR & CAN_TSR_TME) != 0;
  case 1:
    return (canp->can->TSR & CAN_TSR_TME0) != 0;
  case 2:
    return (canp->can->TSR & CAN_TSR_TME1) != 0;
  case 3:
    return (canp->can->TSR & CAN_TSR_TME2) != 0;
  default:
    return FALSE;
  }
}

/**
 * @brief   Inserts a frame into the transmit queue.
 *
 * @param[in] canp      pointer to the @p CANDriver object
 * @param[in] ctfp      pointer to the CAN frame to be transmitted
 * @param[in] mailbox   mailbox number,  @p CAN_ANY_MAILBOX for any mailbox
 *
 * @notapi
 */
void can_lld_transmit(CANDriver *canp,
                      canmbx_t mailbox,
                      const CANTxFrame *ctfp) {
  uint32_t tir;
  CAN_TxMailBox_TypeDef *tmbp;

  /* Pointer to a free transmission mailbox.*/
  switch (mailbox) {
  case CAN_ANY_MAILBOX:
    tmbp = &canp->can->sTxMailBox[(canp->can->TSR & CAN_TSR_CODE) >> 24];
    break;
  case 1:
    tmbp = &canp->can->sTxMailBox[0];
    break;
  case 2:
    tmbp = &canp->can->sTxMailBox[1];
    break;
  case 3:
    tmbp = &canp->can->sTxMailBox[2];
    break;
  default:
    return;
  }

  /* Preparing the message.*/
  if (ctfp->IDE)
    tir = ((uint32_t)ctfp->EID << 3) | ((uint32_t)ctfp->RTR << 1) |
          CAN_TI0R_IDE;
  else
    tir = ((uint32_t)ctfp->SID << 21) | ((uint32_t)ctfp->RTR << 1);
  tmbp->TDTR = ctfp->DLC;
  tmbp->TDLR = ctfp->data32[0];
  tmbp->TDHR = ctfp->data32[1];
  tmbp->TIR  = tir | CAN_TI0R_TXRQ;
}

/**
 * @brief   Determines whether a frame has been received.
 *
 * @param[in] canp      pointer to the @p CANDriver object
 * @param[in] mailbox   mailbox number, @p CAN_ANY_MAILBOX for any mailbox
 *
 * @return              The queue space availability.
 * @retval FALSE        no space in the transmit queue.
 * @retval TRUE         transmit slot available.
 *
 * @notapi
 */
bool_t can_lld_is_rx_nonempty(CANDriver *canp, canmbx_t mailbox) {

  switch (mailbox) {
  case CAN_ANY_MAILBOX:
    return ((canp->can->RF0R & CAN_RF0R_FMP0) != 0 ||
            (canp->can->RF1R & CAN_RF1R_FMP1) != 0);
  case 1:
    return (canp->can->RF0R & CAN_RF0R_FMP0) != 0;
  case 2:
    return (canp->can->RF1R & CAN_RF1R_FMP1) != 0;
  default:
    return FALSE;
  }
}

/**
 * @brief   Receives a frame from the input queue.
 *
 * @param[in] canp      pointer to the @p CANDriver object
 * @param[in] mailbox   mailbox number, @p CAN_ANY_MAILBOX for any mailbox
 * @param[out] crfp     pointer to the buffer where the CAN frame is copied
 *
 * @notapi
 */
void can_lld_receive(CANDriver *canp,
                     canmbx_t mailbox,
                     CANRxFrame *crfp) {
  uint32_t rir, rdtr;

  if (mailbox == CAN_ANY_MAILBOX) {
    if ((canp->can->RF0R & CAN_RF0R_FMP0) != 0)
      mailbox = 1;
    else if ((canp->can->RF1R & CAN_RF1R_FMP1) != 0)
      mailbox = 2;
    else {
      /* Should not happen, do nothing.*/
      return;
    }
  }
  switch (mailbox) {
  case 1:
    /* Fetches the message.*/
    rir  = canp->can->sFIFOMailBox[0].RIR;
    rdtr = canp->can->sFIFOMailBox[0].RDTR;
    crfp->data32[0] = canp->can->sFIFOMailBox[0].RDLR;
    crfp->data32[1] = canp->can->sFIFOMailBox[0].RDHR;

    /* Releases the mailbox.*/
    canp->can->RF0R = CAN_RF0R_RFOM0;

    /* If the queue is empty re-enables the interrupt in order to generate
       events again.*/
    if ((canp->can->RF0R & CAN_RF0R_FMP0) == 0)
      canp->can->IER |= CAN_IER_FMPIE0;
    break;
  case 2:
    /* Fetches the message.*/
    rir  = canp->can->sFIFOMailBox[1].RIR;
    rdtr = canp->can->sFIFOMailBox[1].RDTR;
    crfp->data32[0] = canp->can->sFIFOMailBox[1].RDLR;
    crfp->data32[1] = canp->can->sFIFOMailBox[1].RDHR;

    /* Releases the mailbox.*/
    canp->can->RF1R = CAN_RF1R_RFOM1;

    /* If the queue is empty re-enables the interrupt in order to generate
       events again.*/
    if ((canp->can->RF1R & CAN_RF1R_FMP1) == 0)
      canp->can->IER |= CAN_IER_FMPIE1;
    break;
  default:
    /* Should not happen, do nothing.*/
    return;
  }

  /* Decodes the various fields in the RX frame.*/
  crfp->RTR = (rir & CAN_RI0R_RTR) >> 1;
  crfp->IDE = (rir & CAN_RI0R_IDE) >> 2;
  if (crfp->IDE)
    crfp->EID = rir >> 3;
  else
    crfp->SID = rir >> 21;
  crfp->DLC = rdtr & CAN_RDT0R_DLC;
  crfp->FMI = (uint8_t)(rdtr >> 8);
  crfp->TIME = (uint16_t)(rdtr >> 16);
}

#if CAN_USE_SLEEP_MODE || defined(__DOXYGEN__)
/**
 * @brief   Enters the sleep mode.
 *
 * @param[in] canp      pointer to the @p CANDriver object
 *
 * @notapi
 */
void can_lld_sleep(CANDriver *canp) {

  canp->can->MCR |= CAN_MCR_SLEEP;
}

/**
 * @brief   Enforces leaving the sleep mode.
 *
 * @param[in] canp      pointer to the @p CANDriver object
 *
 * @notapi
 */
void can_lld_wakeup(CANDriver *canp) {

  canp->can->MCR &= ~CAN_MCR_SLEEP;
}
#endif /* CAN_USE_SLEEP_MODE */

/**
 * @brief   Programs the filters.
 * @note    This is an STM32-specific API.
 *
 * @param[in] can2sb    number of the first filter assigned to CAN2
 * @param[in] num       number of entries in the filters array, if zero then
 *                      a default filter is programmed
 * @param[in] cfp       pointer to the filters array, can be @p NULL if
 *                      (num == 0)
 *
 * @api
 */
void canSTM32SetFilters(uint32_t can2sb, uint32_t num, const CANFilter *cfp) {

  chDbgCheck((can2sb >= 1) && (can2sb < STM32_CAN_MAX_FILTERS) &&
             (num <= STM32_CAN_MAX_FILTERS),
             "canSTM32SetFilters");

#if STM32_CAN_USE_CAN1
  chDbgAssert(CAND1.state == CAN_STOP,
              "canSTM32SetFilters(), #1", "invalid state");
#endif
#if STM32_CAN_USE_CAN2
  chDbgAssert(CAND2.state == CAN_STOP,
              "canSTM32SetFilters(), #2", "invalid state");
#endif

  can_lld_set_filters(can2sb, num, cfp);
}

#endif /* HAL_USE_CAN */

/** @} */
