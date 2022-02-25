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
 * @file    SPC5xx/LINFlex_v1/serial_lld.c
 * @brief   SPC5xx low level serial driver code.
 *
 * @addtogroup SERIAL
 * @{
 */

#include "ch.h"
#include "hal.h"

#if HAL_USE_SERIAL || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/**
 * @brief   LIINFlex-0 serial driver identifier.
 */
#if SPC5_SERIAL_USE_LINFLEX0 || defined(__DOXYGEN__)
SerialDriver SD1;
#endif

/**
 * @brief   LIINFlex-1 serial driver identifier.
 */
#if SPC5_SERIAL_USE_LINFLEX1 || defined(__DOXYGEN__)
SerialDriver SD2;
#endif

/**
 * @brief   LIINFlex-2 serial driver identifier.
 */
#if SPC5_SERIAL_USE_LINFLEX2 || defined(__DOXYGEN__)
SerialDriver SD3;
#endif

/**
 * @brief   LIINFlex-3 serial driver identifier.
 */
#if SPC5_SERIAL_USE_LINFLEX3 || defined(__DOXYGEN__)
SerialDriver SD4;
#endif

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/**
 * @brief   Driver default configuration.
 */
static const SerialConfig default_config = {
  SERIAL_DEFAULT_BITRATE,
  SD_MODE_8BITS_PARITY_NONE
};

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/**
 * @brief   LINFlex initialization.
 * @details This function must be invoked with interrupts disabled.
 *
 * @param[in] sdp       pointer to a @p SerialDriver object
 * @param[in] config    the architecture-dependent serial driver configuration
 */
static void spc5_linflex_init(SerialDriver *sdp, const SerialConfig *config) {
  uint32_t div;
  volatile struct LINFLEX_tag *linflexp = sdp->linflexp;

  /* Enters the configuration mode.*/
  linflexp->LINCR1.R  = 1;                      /* INIT bit.                */

  /* Configures the LINFlex in UART mode with all the required
     parameters.*/
  linflexp->UARTCR.R  = SPC5_UARTCR_UART;       /* UART mode FIRST.         */
  linflexp->UARTCR.R  = SPC5_UARTCR_UART | SPC5_UARTCR_RXEN | config->mode;
  div = SPC5_LINFLEX0_CLK / config->speed;
  linflexp->LINFBRR.R = (uint16_t)(div & 15);   /* Fractional divider.      */
  linflexp->LINIBRR.R = (uint16_t)(div >> 4);   /* Integer divider.         */
  linflexp->UARTSR.R  = 0xFFFF;                 /* Clearing UARTSR register.*/
  linflexp->LINIER.R  = SPC5_LINIER_DTIE | SPC5_LINIER_DRIE |
                        SPC5_LINIER_BOIE | SPC5_LINIER_FEIE |
                        SPC5_LINIER_SZIE;       /* Interrupts enabled.      */

  /* Leaves the configuration mode.*/
  linflexp->LINCR1.R  = 0;
}

/**
 * @brief   LINFlex de-initialization.
 * @details This function must be invoked with interrupts disabled.
 *
 * @param[in] linflexp  pointer to a LINFlex I/O block
 */
static void spc5_linflex_deinit(volatile struct LINFLEX_tag *linflexp) {

  /* Enters the configuration mode.*/
  linflexp->LINCR1.R  = 1;                      /* INIT bit.                */

  /* Resets the LINFlex registers.*/
  linflexp->LINFBRR.R = 0;                      /* Fractional divider.      */
  linflexp->LINIBRR.R = 0;                      /* Integer divider.         */
  linflexp->UARTSR.R  = 0xFFFF;                 /* Clearing UARTSR register.*/
  linflexp->UARTCR.R  = SPC5_UARTCR_UART;
  linflexp->LINIER.R  = 0;                      /* Interrupts disabled.     */

  /* Leaves the configuration mode.*/
  linflexp->LINCR1.R  = 0;
}

/**
 * @brief   Common RXI IRQ handler.
 *
 * @param[in] sdp       pointer to a @p SerialDriver object
 */
static void spc5xx_serve_rxi_interrupt(SerialDriver *sdp) {
  flagsmask_t sts = 0;
  uint16_t sr = sdp->linflexp->UARTSR.R;

  sdp->linflexp->UARTSR.R = SPC5_UARTSR_NF | SPC5_UARTSR_DRF |
                            SPC5_UARTSR_PE0;
  if (sr & SPC5_UARTSR_NF)
    sts |= SD_NOISE_ERROR;
  if (sr & SPC5_UARTSR_PE0)
    sts |= SD_PARITY_ERROR;
  chSysLockFromIsr();
  if (sts)
    chnAddFlagsI(sdp, sts);
  if (sr & SPC5_UARTSR_DRF) {
    sdIncomingDataI(sdp, sdp->linflexp->BDRM.B.DATA4);
    sdp->linflexp->UARTSR.R = SPC5_UARTSR_RMB;
  }
  chSysUnlockFromIsr();
}

/**
 * @brief   Common TXI IRQ handler.
 *
 * @param[in] sdp       pointer to a @p SerialDriver object
 */
static void spc5xx_serve_txi_interrupt(SerialDriver *sdp) {
  msg_t b;

  sdp->linflexp->UARTSR.R = SPC5_UARTSR_DTF;
  chSysLockFromIsr();
  b = chOQGetI(&sdp->oqueue);
  if (b < Q_OK) {
    chnAddFlagsI(sdp, CHN_OUTPUT_EMPTY);
    sdp->linflexp->UARTCR.B.TXEN = 0;
  }
  else
    sdp->linflexp->BDRL.B.DATA0 = b;
  chSysUnlockFromIsr();
}

/**
 * @brief   Common ERR IRQ handler.
 *
 * @param[in] sdp       pointer to a @p SerialDriver object
 */
static void spc5xx_serve_err_interrupt(SerialDriver *sdp) {
  flagsmask_t sts = 0;
  uint16_t sr = sdp->linflexp->UARTSR.R;

  sdp->linflexp->UARTSR.R = SPC5_UARTSR_BOF | SPC5_UARTSR_FEF |
                            SPC5_UARTSR_SZF;
  if (sr & SPC5_UARTSR_BOF)
    sts |= SD_OVERRUN_ERROR;
  if (sr & SPC5_UARTSR_FEF)
    sts |= SD_FRAMING_ERROR;
  if (sr & SPC5_UARTSR_SZF)
    sts |= SD_BREAK_DETECTED;
  chSysLockFromIsr();
  chnAddFlagsI(sdp, sts);
  chSysUnlockFromIsr();
}

#if SPC5_SERIAL_USE_LINFLEX0 || defined(__DOXYGEN__)
static void notify1(GenericQueue *qp) {

  (void)qp;
  if (!SD1.linflexp->UARTCR.B.TXEN) {
    msg_t b = sdRequestDataI(&SD1);
    if (b != Q_EMPTY) {
      SD1.linflexp->UARTCR.B.TXEN = 1;
      SD1.linflexp->BDRL.B.DATA0 = b;
    }
  }
}
#endif

#if SPC5_SERIAL_USE_LINFLEX1 || defined(__DOXYGEN__)
static void notify2(GenericQueue *qp) {

  (void)qp;
  if (!SD2.linflexp->UARTCR.B.TXEN) {
    msg_t b = sdRequestDataI(&SD2);
    if (b != Q_EMPTY) {
      SD2.linflexp->UARTCR.B.TXEN = 1;
      SD2.linflexp->BDRL.B.DATA0 = b;
    }
  }
}
#endif

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

#if SPC5_SERIAL_USE_LINFLEX0 || defined(__DOXYGEN__)
#if !defined(SPC5_LINFLEX0_RXI_HANDLER)
#error "SPC5_LINFLEX0_RXI_HANDLER not defined"
#endif
/**
 * @brief   LINFlex-0 RXI interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(SPC5_LINFLEX0_RXI_HANDLER) {

  CH_IRQ_PROLOGUE();

  spc5xx_serve_rxi_interrupt(&SD1);

  CH_IRQ_EPILOGUE();
}

#if !defined(SPC5_LINFLEX0_TXI_HANDLER)
#error "SPC5_LINFLEX0_TXI_HANDLER not defined"
#endif
/**
 * @brief   LINFlex-0 TXI interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(SPC5_LINFLEX0_TXI_HANDLER) {

  CH_IRQ_PROLOGUE();

  spc5xx_serve_txi_interrupt(&SD1);

  CH_IRQ_EPILOGUE();
}

#if !defined(SPC5_LINFLEX0_ERR_HANDLER)
#error "SPC5_LINFLEX0_ERR_HANDLER not defined"
#endif
/**
 * @brief   LINFlex-0 ERR interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(SPC5_LINFLEX0_ERR_HANDLER) {

  CH_IRQ_PROLOGUE();

  spc5xx_serve_err_interrupt(&SD1);

  CH_IRQ_EPILOGUE();
}
#endif

#if SPC5_SERIAL_USE_LINFLEX1 || defined(__DOXYGEN__)
#if !defined(SPC5_LINFLEX1_RXI_HANDLER)
#error "SPC5_LINFLEX1_RXI_HANDLER not defined"
#endif
/**
 * @brief   LINFlex-1 RXI interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(SPC5_LINFLEX1_RXI_HANDLER) {

  CH_IRQ_PROLOGUE();

  spc5xx_serve_rxi_interrupt(&SD2);

  CH_IRQ_EPILOGUE();
}

#if !defined(SPC5_LINFLEX1_TXI_HANDLER)
#error "SPC5_LINFLEX1_TXI_HANDLER not defined"
#endif
/**
 * @brief   LINFlex-1 TXI interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(SPC5_LINFLEX1_TXI_HANDLER) {

  CH_IRQ_PROLOGUE();

  spc5xx_serve_txi_interrupt(&SD2);

  CH_IRQ_EPILOGUE();
}

#if !defined(SPC5_LINFLEX1_ERR_HANDLER)
#error "SPC5_LINFLEX1_ERR_HANDLER not defined"
#endif
/**
 * @brief   LINFlex-1 ERR interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(SPC5_LINFLEX1_ERR_HANDLER) {

  CH_IRQ_PROLOGUE();

  spc5xx_serve_err_interrupt(&SD2);

  CH_IRQ_EPILOGUE();
}
#endif

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level serial driver initialization.
 *
 * @notapi
 */
void sd_lld_init(void) {

#if SPC5_SERIAL_USE_LINFLEX0
  sdObjectInit(&SD1, NULL, notify1);
  SD1.linflexp = &LINFLEX_0;
  INTC.PSR[SPC5_LINFLEX0_RXI_NUMBER].R = SPC5_SERIAL_LINFLEX0_PRIORITY;
  INTC.PSR[SPC5_LINFLEX0_TXI_NUMBER].R = SPC5_SERIAL_LINFLEX0_PRIORITY;
  INTC.PSR[SPC5_LINFLEX0_ERR_NUMBER].R = SPC5_SERIAL_LINFLEX0_PRIORITY;
#endif

#if SPC5_SERIAL_USE_LINFLEX1
  sdObjectInit(&SD2, NULL, notify2);
  SD2.linflexp = &LINFLEX_1;
  INTC.PSR[SPC5_LINFLEX1_RXI_NUMBER].R = SPC5_SERIAL_LINFLEX1_PRIORITY;
  INTC.PSR[SPC5_LINFLEX1_TXI_NUMBER].R = SPC5_SERIAL_LINFLEX1_PRIORITY;
  INTC.PSR[SPC5_LINFLEX1_ERR_NUMBER].R = SPC5_SERIAL_LINFLEX1_PRIORITY;
#endif
}

/**
 * @brief   Low level serial driver configuration and (re)start.
 *
 * @param[in] sdp       pointer to a @p SerialDriver object
 * @param[in] config    the architecture-dependent serial driver configuration.
 *                      If this parameter is set to @p NULL then a default
 *                      configuration is used.
 *
 * @notapi
 */
void sd_lld_start(SerialDriver *sdp, const SerialConfig *config) {

  if (config == NULL)
    config = &default_config;

  if (sdp->state == SD_STOP) {
#if SPC5_SERIAL_USE_LINFLEX0
    if (&SD1 == sdp) {
      halSPCSetPeripheralClockMode(SPC5_LINFLEX0_PCTL,
                                   SPC5_SERIAL_LINFLEX0_START_PCTL);
    }
#endif
#if SPC5_SERIAL_USE_LINFLEX1
    if (&SD2 == sdp) {
      halSPCSetPeripheralClockMode(SPC5_LINFLEX1_PCTL,
                                   SPC5_SERIAL_LINFLEX1_START_PCTL);
    }
#endif
  }
  spc5_linflex_init(sdp, config);
}

/**
 * @brief   Low level serial driver stop.
 *
 * @param[in] sdp       pointer to a @p SerialDriver object
 *
 * @notapi
 */
void sd_lld_stop(SerialDriver *sdp) {

  if (sdp->state == SD_READY) {
    spc5_linflex_deinit(sdp->linflexp);

#if SPC5_SERIAL_USE_LINFLEX0
    if (&SD1 == sdp) {
      halSPCSetPeripheralClockMode(SPC5_LINFLEX0_PCTL,
                                   SPC5_SERIAL_LINFLEX0_STOP_PCTL);
      return;
    }
#endif
#if SPC5_SERIAL_USE_LINFLEX1
    if (&SD2 == sdp) {
      halSPCSetPeripheralClockMode(SPC5_LINFLEX1_PCTL,
                                   SPC5_SERIAL_LINFLEX1_STOP_PCTL);
      return;
    }
#endif
  }
}

#endif /* HAL_USE_SERIAL */

/** @} */
