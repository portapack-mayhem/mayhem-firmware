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
 * @file    STM32/OTGv1/usb_lld.c
 * @brief   STM32 USB subsystem low level driver source.
 *
 * @addtogroup USB
 * @{
 */

#include <string.h>

#include "ch.h"
#include "hal.h"

#if HAL_USE_USB || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

#define TRDT_VALUE              5

#define EP0_MAX_INSIZE          64
#define EP0_MAX_OUTSIZE         64

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/** @brief OTG_FS driver identifier.*/
#if STM32_USB_USE_OTG1 || defined(__DOXYGEN__)
USBDriver USBD1;
#endif

/** @brief OTG_HS driver identifier.*/
#if STM32_USB_USE_OTG2 || defined(__DOXYGEN__)
USBDriver USBD2;
#endif

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/**
 * @brief   EP0 state.
 * @note    It is an union because IN and OUT endpoints are never used at the
 *          same time for EP0.
 */
static union {
  /**
   * @brief   IN EP0 state.
   */
  USBInEndpointState in;
  /**
   * @brief   OUT EP0 state.
   */
  USBOutEndpointState out;
} ep0_state;

/**
 * @brief   Buffer for the EP0 setup packets.
 */
static uint8_t ep0setup_buffer[8];

/**
 * @brief   EP0 initialization structure.
 */
static const USBEndpointConfig ep0config = {
  USB_EP_MODE_TYPE_CTRL,
  _usb_ep0setup,
  _usb_ep0in,
  _usb_ep0out,
  0x40,
  0x40,
  &ep0_state.in,
  &ep0_state.out,
  1,
  ep0setup_buffer
};

#if STM32_USB_USE_OTG1
static const stm32_otg_params_t fsparams = {
  STM32_USB_OTG1_RX_FIFO_SIZE / 4,
  STM32_OTG1_FIFO_MEM_SIZE,
  STM32_OTG1_ENDOPOINTS_NUMBER
};
#endif

#if STM32_USB_USE_OTG2
static const stm32_otg_params_t hsparams = {
  STM32_USB_OTG2_RX_FIFO_SIZE / 4,
  STM32_OTG2_FIFO_MEM_SIZE,
  STM32_OTG2_ENDOPOINTS_NUMBER
};
#endif

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/**
 * @brief   Wakes up the pump thread.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 *
 * @notapi
 */
static void usb_lld_wakeup_pump(USBDriver *usbp) {

  if (usbp->thd_wait != NULL) {
    chThdResumeI(usbp->thd_wait);
    usbp->thd_wait = NULL;
  }
}

static void otg_core_reset(USBDriver *usbp) {
  stm32_otg_t *otgp = usbp->otg;

  halPolledDelay(32);

  /* Core reset and delay of at least 3 PHY cycles.*/
  otgp->GRSTCTL = GRSTCTL_CSRST;
  while ((otgp->GRSTCTL & GRSTCTL_CSRST) != 0)
    ;

  halPolledDelay(12);

  /* Wait AHB idle condition.*/
  while ((otgp->GRSTCTL & GRSTCTL_AHBIDL) == 0)
    ;
}

static void otg_disable_ep(USBDriver *usbp) {
  stm32_otg_t *otgp = usbp->otg;
  unsigned i;

  for (i = 0; i <= usbp->otgparams->num_endpoints; i++) {
    /* Disable only if enabled because this sentence in the manual:
       "The application must set this bit only if Endpoint Enable is
        already set for this endpoint".*/
    if ((otgp->ie[i].DIEPCTL & DIEPCTL_EPENA) != 0) {
      otgp->ie[i].DIEPCTL = DIEPCTL_EPDIS;
      /* Wait for endpoint disable.*/
      while (!(otgp->ie[i].DIEPINT & DIEPINT_EPDISD))
        ;
    }
    else
      otgp->ie[i].DIEPCTL = 0;
    otgp->ie[i].DIEPTSIZ = 0;
    otgp->ie[i].DIEPINT = 0xFFFFFFFF;
    /* Disable only if enabled because this sentence in the manual:
       "The application must set this bit only if Endpoint Enable is
        already set for this endpoint".
       Note that the attempt to disable the OUT EP0 is ignored by the
       hardware but the code is simpler this way.*/
    if ((otgp->oe[i].DOEPCTL & DOEPCTL_EPENA) != 0) {
      otgp->oe[i].DOEPCTL = DOEPCTL_EPDIS;
      /* Wait for endpoint disable.*/
      while (!(otgp->oe[i].DOEPINT & DOEPINT_OTEPDIS))
        ;
    }
    else
      otgp->oe[i].DOEPCTL = 0;
    otgp->oe[i].DOEPTSIZ = 0;
    otgp->oe[i].DOEPINT = 0xFFFFFFFF;
  }
  otgp->DAINTMSK = DAINTMSK_OEPM(0) | DAINTMSK_IEPM(0);
}

static void otg_rxfifo_flush(USBDriver *usbp) {
  stm32_otg_t *otgp = usbp->otg;

  otgp->GRSTCTL = GRSTCTL_RXFFLSH;
  while ((otgp->GRSTCTL & GRSTCTL_RXFFLSH) != 0)
    ;
  /* Wait for 3 PHY Clocks.*/
  halPolledDelay(12);
}

static void otg_txfifo_flush(USBDriver *usbp, uint32_t fifo) {
  stm32_otg_t *otgp = usbp->otg;

  otgp->GRSTCTL = GRSTCTL_TXFNUM(fifo) | GRSTCTL_TXFFLSH;
  while ((otgp->GRSTCTL & GRSTCTL_TXFFLSH) != 0)
    ;
  /* Wait for 3 PHY Clocks.*/
  halPolledDelay(12);
}

/**
 * @brief   Resets the FIFO RAM memory allocator.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 *
 * @notapi
 */
static void otg_ram_reset(USBDriver *usbp) {

  usbp->pmnext = usbp->otgparams->rx_fifo_size;
}

/**
 * @brief   Allocates a block from the FIFO RAM memory.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 * @param[in] size      size of the packet buffer to allocate in words
 *
 * @notapi
 */
static uint32_t otg_ram_alloc(USBDriver *usbp, size_t size) {
  uint32_t next;

  next = usbp->pmnext;
  usbp->pmnext += size;
  chDbgAssert(usbp->pmnext <= usbp->otgparams->otg_ram_size,
              "otg_fifo_alloc(), #1", "OTG FIFO memory overflow");
  return next;
}

/**
 * @brief   Pushes a series of words into a FIFO.
 *
 * @param[in] fifop     pointer to the FIFO register
 * @param[in] buf       pointer to the words buffer, not necessarily word
 *                      aligned
 * @param[in] n         number of words to push
 *
 * @return              A pointer after the last word pushed.
 *
 * @notapi
 */
static uint8_t *otg_do_push(volatile uint32_t *fifop, uint8_t *buf, size_t n) {

  while (n > 0) {
    /* Note, this line relies on the Cortex-M3/M4 ability to perform
       unaligned word accesses and on the LSB-first memory organization.*/
    *fifop = *((PACKED_VAR uint32_t *)buf);
    buf += 4;
    n--;
  }
  return buf;
}

/**
 * @brief   Writes to a TX FIFO.
 *
 * @param[in] fifop     pointer to the FIFO register
 * @param[in] buf       buffer where to copy the endpoint data
 * @param[in] n         maximum number of bytes to copy
 *
 * @notapi
 */
static void otg_fifo_write_from_buffer(volatile uint32_t *fifop,
                                       const uint8_t *buf,
                                       size_t n) {

  otg_do_push(fifop, (uint8_t *)buf, (n + 3) / 4);
}

/**
 * @brief   Writes to a TX FIFO fetching data from a queue.
 *
 * @param[in] fifop     pointer to the FIFO register
 * @param[in] oqp       pointer to an @p OutputQueue object
 * @param[in] n         maximum number of bytes to copy
 *
 * @notapi
 */
static void otg_fifo_write_from_queue(volatile uint32_t *fifop,
                                      OutputQueue *oqp,
                                      size_t n) {
  size_t ntogo;

  ntogo = n;
  while (ntogo > 0) {
    uint32_t w, i;
    size_t nw = ntogo / 4;

    if (nw > 0) {
      size_t streak;
      uint32_t nw2end = (oqp->q_top - oqp->q_rdptr) / 4;

      ntogo -= (streak = nw <= nw2end ? nw : nw2end) * 4;
      oqp->q_rdptr = otg_do_push(fifop, oqp->q_rdptr, streak);
      if (oqp->q_rdptr >= oqp->q_top) {
        oqp->q_rdptr = oqp->q_buffer;
        continue;
      }
    }

    /* If this condition is not satisfied then there is a word lying across
       queue circular buffer boundary or there are some remaining bytes.*/
    if (ntogo <= 0)
      break;

    /* One byte at time.*/
    w = 0;
    i = 0;
    while ((ntogo > 0) && (i < 4)) {
      w |= (uint32_t)*oqp->q_rdptr++ << (i * 8);
      if (oqp->q_rdptr >= oqp->q_top)
        oqp->q_rdptr = oqp->q_buffer;
      ntogo--;
      i++;
    }
    *fifop = w;
  }

  /* Updating queue.*/
  chSysLock();
  oqp->q_counter += n;
  while (notempty(&oqp->q_waiting))
    chSchReadyI(fifo_remove(&oqp->q_waiting))->p_u.rdymsg = Q_OK;
  chSchRescheduleS();
  chSysUnlock();
}

/**
 * @brief   Pops a series of words from a FIFO.
 *
 * @param[in] fifop     pointer to the FIFO register
 * @param[in] buf       pointer to the words buffer, not necessarily word
 *                      aligned
 * @param[in] n         number of words to push
 *
 * @return              A pointer after the last word pushed.
 *
 * @notapi
 */
static uint8_t *otg_do_pop(volatile uint32_t *fifop, uint8_t *buf, size_t n) {

  while (n > 0) {
    uint32_t w = *fifop;
    /* Note, this line relies on the Cortex-M3/M4 ability to perform
       unaligned word accesses and on the LSB-first memory organization.*/
    *((PACKED_VAR uint32_t *)buf) = w;
    buf += 4;
    n--;
  }
  return buf;
}

/**
 * @brief   Reads a packet from the RXFIFO.
 *
 * @param[in] fifop     pointer to the FIFO register
 * @param[out] buf      buffer where to copy the endpoint data
 * @param[in] n         number of bytes to pull from the FIFO
 * @param[in] max       number of bytes to copy into the buffer
 *
 * @notapi
 */
static void otg_fifo_read_to_buffer(volatile uint32_t *fifop,
                                    uint8_t *buf,
                                    size_t n,
                                    size_t max) {

  n = (n + 3) / 4;
  max = (max + 3) / 4;
  while (n) {
    uint32_t w = *fifop;
    if (max) {
      /* Note, this line relies on the Cortex-M3/M4 ability to perform
         unaligned word accesses and on the LSB-first memory organization.*/
      *((PACKED_VAR uint32_t *)buf) = w;
      buf += 4;
      max--;
    }
    n--;
  }
}

/**
 * @brief   Reads a packet from the RXFIFO.
 *
 * @param[in] fifop     pointer to the FIFO register
 * @param[in] iqp       pointer to an @p InputQueue object
 * @param[in] n         number of bytes to pull from the FIFO
 *
 * @notapi
 */
static void otg_fifo_read_to_queue(volatile uint32_t *fifop,
                                   InputQueue *iqp,
                                   size_t n) {
  size_t ntogo;

  ntogo = n;
  while (ntogo > 0) {
    uint32_t w, i;
    size_t nw = ntogo / 4;

    if (nw > 0) {
      size_t streak;
      uint32_t nw2end = (iqp->q_wrptr - iqp->q_wrptr) / 4;

      ntogo -= (streak = nw <= nw2end ? nw : nw2end) * 4;
      iqp->q_wrptr = otg_do_pop(fifop, iqp->q_wrptr, streak);
      if (iqp->q_wrptr >= iqp->q_top) {
        iqp->q_wrptr = iqp->q_buffer;
        continue;
      }
    }

    /* If this condition is not satisfied then there is a word lying across
       queue circular buffer boundary or there are some remaining bytes.*/
    if (ntogo <= 0)
      break;

    /* One byte at time.*/
    w = *fifop;
    i = 0;
    while ((ntogo > 0) && (i < 4)) {
      *iqp->q_wrptr++ = (uint8_t)(w >> (i * 8));
      if (iqp->q_wrptr >= iqp->q_top)
        iqp->q_wrptr = iqp->q_buffer;
      ntogo--;
      i++;
    }
  }

  /* Updating queue.*/
  chSysLock();
  iqp->q_counter += n;
  while (notempty(&iqp->q_waiting))
    chSchReadyI(fifo_remove(&iqp->q_waiting))->p_u.rdymsg = Q_OK;
  chSchRescheduleS();
  chSysUnlock();
}

/**
 * @brief   Incoming packets handler.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 *
 * @notapi
 */
static void otg_rxfifo_handler(USBDriver *usbp) {
  uint32_t sts, cnt, ep;

  sts = usbp->otg->GRXSTSP;
  switch (sts & GRXSTSP_PKTSTS_MASK) {
  case GRXSTSP_SETUP_COMP:
    break;
  case GRXSTSP_SETUP_DATA:
    cnt = (sts & GRXSTSP_BCNT_MASK) >> GRXSTSP_BCNT_OFF;
    ep  = (sts & GRXSTSP_EPNUM_MASK) >> GRXSTSP_EPNUM_OFF;
    otg_fifo_read_to_buffer(usbp->otg->FIFO[0], usbp->epc[ep]->setup_buf,
                            cnt, 8);
    break;
  case GRXSTSP_OUT_DATA:
    cnt = (sts & GRXSTSP_BCNT_MASK) >> GRXSTSP_BCNT_OFF;
    ep  = (sts & GRXSTSP_EPNUM_MASK) >> GRXSTSP_EPNUM_OFF;
    if (usbp->epc[ep]->out_state->rxqueued) {
      /* Queue associated.*/
      otg_fifo_read_to_queue(usbp->otg->FIFO[0],
                             usbp->epc[ep]->out_state->mode.queue.rxqueue,
                             cnt);
    }
    else {
      otg_fifo_read_to_buffer(usbp->otg->FIFO[0],
                              usbp->epc[ep]->out_state->mode.linear.rxbuf,
                              cnt,
                              usbp->epc[ep]->out_state->rxsize -
                              usbp->epc[ep]->out_state->rxcnt);
      usbp->epc[ep]->out_state->mode.linear.rxbuf += cnt;
    }
    usbp->epc[ep]->out_state->rxcnt += cnt;
    break;
  case GRXSTSP_OUT_GLOBAL_NAK:
  case GRXSTSP_OUT_COMP:
  default:
    ;
  }
}

/**
 * @brief   Outgoing packets handler.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 * @param[in] ep        endpoint number
 *
 * @notapi
 */
static bool_t otg_txfifo_handler(USBDriver *usbp, usbep_t ep) {

  /* The TXFIFO is filled until there is space and data to be transmitted.*/
  while (TRUE) {
    uint32_t n;

    /* Transaction end condition.*/
    if (usbp->epc[ep]->in_state->txcnt >= usbp->epc[ep]->in_state->txsize)
      return TRUE;

    /* Number of bytes remaining in current transaction.*/
    n = usbp->epc[ep]->in_state->txsize - usbp->epc[ep]->in_state->txcnt;
    if (n > usbp->epc[ep]->in_maxsize)
      n = usbp->epc[ep]->in_maxsize;

    /* Checks if in the TXFIFO there is enough space to accommodate the
       next packet.*/
    if (((usbp->otg->ie[ep].DTXFSTS & DTXFSTS_INEPTFSAV_MASK) * 4) < n)
      return FALSE;

#if STM32_USB_OTGFIFO_FILL_BASEPRI
    __set_BASEPRI(CORTEX_PRIORITY_MASK(STM32_USB_OTGFIFO_FILL_BASEPRI));
#endif
    /* Handles the two cases: linear buffer or queue.*/
    if (usbp->epc[ep]->in_state->txqueued) {
      /* Queue associated.*/
      otg_fifo_write_from_queue(usbp->otg->FIFO[ep],
                                usbp->epc[ep]->in_state->mode.queue.txqueue,
                                n);
    }
    else {
      /* Linear buffer associated.*/
      otg_fifo_write_from_buffer(usbp->otg->FIFO[ep],
                                 usbp->epc[ep]->in_state->mode.linear.txbuf,
                                 n);
      usbp->epc[ep]->in_state->mode.linear.txbuf += n;
    }
#if STM32_USB_OTGFIFO_FILL_BASEPRI
  __set_BASEPRI(0);
#endif
    usbp->epc[ep]->in_state->txcnt += n;
  }
}

/**
 * @brief   Generic endpoint IN handler.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 * @param[in] ep        endpoint number
 *
 * @notapi
 */
static void otg_epin_handler(USBDriver *usbp, usbep_t ep) {
  stm32_otg_t *otgp = usbp->otg;
  uint32_t epint = otgp->ie[ep].DIEPINT;

  otgp->ie[ep].DIEPINT = epint;

  if (epint & DIEPINT_TOC) {
    /* Timeouts not handled yet, not sure how to handle.*/
  }
  if ((epint & DIEPINT_XFRC) && (otgp->DIEPMSK & DIEPMSK_XFRCM)) {
    /* Transmit transfer complete.*/
    USBInEndpointState *isp = usbp->epc[ep]->in_state;

    if (isp->txsize < isp->totsize) {
      /* In case the transaction covered only part of the total transfer
         then another transaction is immediately started in order to
         cover the remaining.*/
      isp->txsize = isp->totsize - isp->txsize;
      isp->txcnt  = 0;
      usb_lld_prepare_transmit(usbp, ep);
      chSysLockFromIsr();
      usb_lld_start_in(usbp, ep);
      chSysUnlockFromIsr();
    }
    else {
      /* End on IN transfer.*/
      _usb_isr_invoke_in_cb(usbp, ep);
    }
  }
  if ((epint & DIEPINT_TXFE) &&
      (otgp->DIEPEMPMSK & DIEPEMPMSK_INEPTXFEM(ep))) {
    /* The thread is made ready, it will be scheduled on ISR exit.*/
    chSysLockFromIsr();
    usbp->txpending |= (1 << ep);
    otgp->DIEPEMPMSK &= ~(1 << ep);
    usb_lld_wakeup_pump(usbp);
    chSysUnlockFromIsr();
  }
}

/**
 * @brief   Generic endpoint OUT handler.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 * @param[in] ep        endpoint number
 *
 * @notapi
 */
static void otg_epout_handler(USBDriver *usbp, usbep_t ep) {
  stm32_otg_t *otgp = usbp->otg;
  uint32_t epint = otgp->oe[ep].DOEPINT;

  /* Resets all EP IRQ sources.*/
  otgp->oe[ep].DOEPINT = epint;

  if ((epint & DOEPINT_STUP) && (otgp->DOEPMSK & DOEPMSK_STUPM)) {
    /* Setup packets handling, setup packets are handled using a
       specific callback.*/
    _usb_isr_invoke_setup_cb(usbp, ep);

  }
  if ((epint & DOEPINT_XFRC) && (otgp->DOEPMSK & DOEPMSK_XFRCM)) {
    /* Receive transfer complete.*/
    USBOutEndpointState *osp = usbp->epc[ep]->out_state;

    if (osp->rxsize < osp->totsize) {
      /* In case the transaction covered only part of the total transfer
         then another transaction is immediately started in order to
         cover the remaining.*/
      osp->rxsize = osp->totsize - osp->rxsize;
      osp->rxcnt  = 0;
      usb_lld_prepare_receive(usbp, ep);
      chSysLockFromIsr();
      usb_lld_start_out(usbp, ep);
      chSysUnlockFromIsr();
    }
    else {
      /* End on OUT transfer.*/
      _usb_isr_invoke_out_cb(usbp, ep);
    }
  }
}

/**
 * @brief   OTG shared ISR.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 *
 * @notapi
 */
static void usb_lld_serve_interrupt(USBDriver *usbp) {
  stm32_otg_t *otgp = usbp->otg;
  uint32_t sts, src;

  sts  = otgp->GINTSTS;
  sts &= otgp->GINTMSK;
  otgp->GINTSTS = sts;

  /* Reset interrupt handling.*/
  if (sts & GINTSTS_USBRST) {
    _usb_reset(usbp);
    _usb_isr_invoke_event_cb(usbp, USB_EVENT_RESET);
  }

  /* Enumeration done.*/
  if (sts & GINTSTS_ENUMDNE) {
    (void)otgp->DSTS;
  }

  /* SOF interrupt handling.*/
  if (sts & GINTSTS_SOF) {
    _usb_isr_invoke_sof_cb(usbp);
  }

  /* RX FIFO not empty handling.*/
  if (sts & GINTSTS_RXFLVL) {
    /* The interrupt is masked while the thread has control or it would
       be triggered again.*/
    chSysLockFromIsr();
    otgp->GINTMSK &= ~GINTMSK_RXFLVLM;
    usb_lld_wakeup_pump(usbp);
    chSysUnlockFromIsr();
  }

  /* IN/OUT endpoints event handling.*/
  src = otgp->DAINT;
  if (sts & GINTSTS_IEPINT) {
    if (src & (1 << 0))
      otg_epin_handler(usbp, 0);
    if (src & (1 << 1))
      otg_epin_handler(usbp, 1);
    if (src & (1 << 2))
      otg_epin_handler(usbp, 2);
    if (src & (1 << 3))
      otg_epin_handler(usbp, 3);
#if STM32_USB_USE_OTG2
    if (src & (1 << 4))
      otg_epin_handler(usbp, 4);
    if (src & (1 << 5))
      otg_epin_handler(usbp, 5);
#endif
  }
  if (sts & GINTSTS_OEPINT) {
    if (src & (1 << 16))
      otg_epout_handler(usbp, 0);
    if (src & (1 << 17))
      otg_epout_handler(usbp, 1);
    if (src & (1 << 18))
      otg_epout_handler(usbp, 2);
    if (src & (1 << 19))
      otg_epout_handler(usbp, 3);
#if STM32_USB_USE_OTG2
    if (src & (1 << 20))
      otg_epout_handler(usbp, 4);
    if (src & (1 << 21))
      otg_epout_handler(usbp, 5);
#endif
  }
}

/*===========================================================================*/
/* Driver interrupt handlers and threads.                                    */
/*===========================================================================*/

static msg_t usb_lld_pump(void *p) {
  USBDriver *usbp = (USBDriver *)p;
  stm32_otg_t *otgp = usbp->otg;

  chRegSetThreadName("usb_lld_pump");
  chSysLock();
  while (TRUE) {
    usbep_t ep;
    uint32_t epmask;

    /* Nothing to do, going to sleep.*/
    if ((usbp->state == USB_STOP) ||
        ((usbp->txpending == 0) && !(otgp->GINTSTS & GINTSTS_RXFLVL))) {
      otgp->GINTMSK |= GINTMSK_RXFLVLM;
      usbp->thd_wait = chThdSelf();
      chSchGoSleepS(THD_STATE_SUSPENDED);
    }
    chSysUnlock();

    /* Checks if there are TXFIFOs to be filled.*/
    for (ep = 0; ep <= usbp->otgparams->num_endpoints; ep++) {

      /* Empties the RX FIFO.*/
      while (otgp->GINTSTS & GINTSTS_RXFLVL) {
        otg_rxfifo_handler(usbp);
      }

      epmask = (1 << ep);
      if (usbp->txpending & epmask) {
        bool_t done;

        chSysLock();
        /* USB interrupts are globally *suspended* because the peripheral
           does not allow any interference during the TX FIFO filling
           operation.
           Synopsys document: DesignWare Cores USB 2.0 Hi-Speed On-The-Go (OTG)
             "The application has to finish writing one complete packet before
              switching to a different channel/endpoint FIFO. Violating this
              rule results in an error.".*/
        otgp->GAHBCFG &= ~GAHBCFG_GINTMSK;
        usbp->txpending &= ~epmask;
        chSysUnlock();

        done = otg_txfifo_handler(usbp, ep);

        chSysLock();
        otgp->GAHBCFG |= GAHBCFG_GINTMSK;
        if (!done)
          otgp->DIEPEMPMSK |= epmask;
        chSysUnlock();
      }
    }
    chSysLock();
  }
}

#if STM32_USB_USE_OTG1 || defined(__DOXYGEN__)
#if !defined(STM32_OTG1_HANDLER)
#error "STM32_OTG1_HANDLER not defined"
#endif
/**
 * @brief   OTG1 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(STM32_OTG1_HANDLER) {

  CH_IRQ_PROLOGUE();

  usb_lld_serve_interrupt(&USBD1);

  CH_IRQ_EPILOGUE();
}
#endif

#if STM32_USB_USE_OTG2 || defined(__DOXYGEN__)
#if !defined(STM32_OTG2_HANDLER)
#error "STM32_OTG2_HANDLER not defined"
#endif
/**
 * @brief   OTG2 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(STM32_OTG2_HANDLER) {

  CH_IRQ_PROLOGUE();

  usb_lld_serve_interrupt(&USBD2);

  CH_IRQ_EPILOGUE();
}
#endif

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level USB driver initialization.
 *
 * @notapi
 */
void usb_lld_init(void) {

  /* Driver initialization.*/
#if STM32_USB_USE_OTG1
  usbObjectInit(&USBD1);
  USBD1.thd_ptr   = NULL;
  USBD1.thd_wait  = NULL;
  USBD1.otg       = OTG_FS;
  USBD1.otgparams = &fsparams;

  /* Filling the thread working area here because the function
     @p chThdCreateI() does not do it.*/
#if CH_DBG_FILL_THREADS
  {
    void *wsp = USBD1.wa_pump;
    _thread_memfill((uint8_t *)wsp,
                    (uint8_t *)wsp + sizeof(Thread),
                    CH_THREAD_FILL_VALUE);
    _thread_memfill((uint8_t *)wsp + sizeof(Thread),
                    (uint8_t *)wsp + sizeof(USBD1.wa_pump),
                    CH_STACK_FILL_VALUE);
  }
#endif
#endif

#if STM32_USB_USE_OTG2
  usbObjectInit(&USBD2);
  USBD2.thd_ptr   = NULL;
  USBD2.thd_wait  = NULL;
  USBD2.otg       = OTG_HS;
  USBD2.otgparams = &hsparams;

  /* Filling the thread working area here because the function
     @p chThdCreateI() does not do it.*/
#if CH_DBG_FILL_THREADS
  {
    void *wsp = USBD2.wa_pump;
    _thread_memfill((uint8_t *)wsp,
                    (uint8_t *)wsp + sizeof(Thread),
                    CH_THREAD_FILL_VALUE);
    _thread_memfill((uint8_t *)wsp + sizeof(Thread),
                    (uint8_t *)wsp + sizeof(USBD2.wa_pump),
                    CH_STACK_FILL_VALUE);
  }
#endif
#endif
}

/**
 * @brief   Configures and activates the USB peripheral.
 * @note    Starting the OTG cell can be a slow operation carried out with
 *          interrupts disabled, perform it before starting time-critical
 *          operations.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 *
 * @notapi
 */
void usb_lld_start(USBDriver *usbp) {
  stm32_otg_t *otgp = usbp->otg;

  if (usbp->state == USB_STOP) {
    /* Clock activation.*/
#if STM32_USB_USE_OTG1
    if (&USBD1 == usbp) {
      /* OTG FS clock enable and reset.*/
      rccEnableOTG_FS(FALSE);
      rccResetOTG_FS();

      /* Enables IRQ vector.*/
      nvicEnableVector(STM32_OTG1_NUMBER,
                       CORTEX_PRIORITY_MASK(STM32_USB_OTG1_IRQ_PRIORITY));
    }
#endif
#if STM32_USB_USE_OTG2
    if (&USBD2 == usbp) {
      /* OTG HS clock enable and reset.*/
      rccEnableOTG_HS(FALSE);
      rccResetOTG_HS();

      /* Workaround for the problem described here:
         http://forum.chibios.org/phpbb/viewtopic.php?f=16&t=1798 */
      rccDisableOTG_HSULPI(TRUE);

      /* Enables IRQ vector.*/
      nvicEnableVector(STM32_OTG2_NUMBER,
                       CORTEX_PRIORITY_MASK(STM32_USB_OTG2_IRQ_PRIORITY));
    }
#endif

    /* Creates the data pump threads in a suspended state. Note, it is
       created only once, the first time @p usbStart() is invoked.*/
    usbp->txpending = 0;
    if (usbp->thd_ptr == NULL)
      usbp->thd_ptr = usbp->thd_wait = chThdCreateI(usbp->wa_pump,
                                                    sizeof usbp->wa_pump,
                                                    STM32_USB_OTG_THREAD_PRIO,
                                                    usb_lld_pump,
                                                    usbp);

    /* - Forced device mode.
       - USB turn-around time = TRDT_VALUE.
       - Full Speed 1.1 PHY.*/
    otgp->GUSBCFG = GUSBCFG_FDMOD | GUSBCFG_TRDT(TRDT_VALUE) | GUSBCFG_PHYSEL;

    /* 48MHz 1.1 PHY.*/
    otgp->DCFG = 0x02200000 | DCFG_DSPD_FS11;

    /* PHY enabled.*/
    otgp->PCGCCTL = 0;

    /* Internal FS PHY activation.*/
#if defined(BOARD_OTG_NOVBUSSENS)
    otgp->GCCFG = GCCFG_NOVBUSSENS | GCCFG_VBUSASEN | GCCFG_VBUSBSEN |
                  GCCFG_PWRDWN;
#else
    otgp->GCCFG = GCCFG_VBUSASEN | GCCFG_VBUSBSEN | GCCFG_PWRDWN;
#endif

    /* Soft core reset.*/
    otg_core_reset(usbp);

    /* Interrupts on TXFIFOs half empty.*/
    otgp->GAHBCFG = 0;

    /* Endpoints re-initialization.*/
    otg_disable_ep(usbp);

    /* Clear all pending Device Interrupts, only the USB Reset interrupt
       is required initially.*/
    otgp->DIEPMSK  = 0;
    otgp->DOEPMSK  = 0;
    otgp->DAINTMSK = 0;
    if (usbp->config->sof_cb == NULL)
      otgp->GINTMSK  = GINTMSK_ENUMDNEM | GINTMSK_USBRSTM /*| GINTMSK_USBSUSPM |
                       GINTMSK_ESUSPM  |*/;
    else
      otgp->GINTMSK  = GINTMSK_ENUMDNEM | GINTMSK_USBRSTM /*| GINTMSK_USBSUSPM |
                       GINTMSK_ESUSPM */ | GINTMSK_SOFM;
    otgp->GINTSTS  = 0xFFFFFFFF;         /* Clears all pending IRQs, if any. */

    /* Global interrupts enable.*/
    otgp->GAHBCFG |= GAHBCFG_GINTMSK;
  }
}

/**
 * @brief   Deactivates the USB peripheral.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 *
 * @notapi
 */
void usb_lld_stop(USBDriver *usbp) {
  stm32_otg_t *otgp = usbp->otg;

  /* If in ready state then disables the USB clock.*/
  if (usbp->state != USB_STOP) {

    /* Disabling all endpoints in case the driver has been stopped while
       active.*/
    otg_disable_ep(usbp);

    usbp->txpending = 0;

    otgp->DAINTMSK   = 0;
    otgp->GAHBCFG    = 0;
    otgp->GCCFG      = 0;

#if STM32_USB_USE_USB1
    if (&USBD1 == usbp) {
      nvicDisableVector(STM32_OTG1_NUMBER);
      rccDisableOTG1(FALSE);
    }
#endif

#if STM32_USB_USE_USB2
    if (&USBD2 == usbp) {
      nvicDisableVector(STM32_OTG2_NUMBER);
      rccDisableOTG2(FALSE);
    }
#endif
  }
}

/**
 * @brief   USB low level reset routine.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 *
 * @notapi
 */
void usb_lld_reset(USBDriver *usbp) {
  unsigned i;
  stm32_otg_t *otgp = usbp->otg;

  /* Flush the Tx FIFO.*/
  otg_txfifo_flush(usbp, 0);

  /* All endpoints in NAK mode, interrupts cleared.*/
  for (i = 0; i <= usbp->otgparams->num_endpoints; i++) {
    otgp->ie[i].DIEPCTL = DIEPCTL_SNAK;
    otgp->oe[i].DOEPCTL = DOEPCTL_SNAK;
    otgp->ie[i].DIEPINT = 0xFF;
    otgp->oe[i].DOEPINT = 0xFF;
  }

  /* Endpoint interrupts all disabled and cleared.*/
  otgp->DAINT = 0xFFFFFFFF;
  otgp->DAINTMSK = DAINTMSK_OEPM(0) | DAINTMSK_IEPM(0);

  /* Resets the FIFO memory allocator.*/
  otg_ram_reset(usbp);

  /* Receive FIFO size initialization, the address is always zero.*/
  otgp->GRXFSIZ = usbp->otgparams->rx_fifo_size;
  otg_rxfifo_flush(usbp);

  /* Resets the device address to zero.*/
  otgp->DCFG = (otgp->DCFG & ~DCFG_DAD_MASK) | DCFG_DAD(0);

  /* Enables also EP-related interrupt sources.*/
  otgp->GINTMSK  |= GINTMSK_RXFLVLM | GINTMSK_OEPM  | GINTMSK_IEPM;
  otgp->DIEPMSK   = DIEPMSK_TOCM    | DIEPMSK_XFRCM;
  otgp->DOEPMSK   = DOEPMSK_STUPM   | DOEPMSK_XFRCM;

  /* EP0 initialization, it is a special case.*/
  usbp->epc[0] = &ep0config;
  otgp->oe[0].DOEPTSIZ = 0;
  otgp->oe[0].DOEPCTL = DOEPCTL_SD0PID | DOEPCTL_USBAEP | DOEPCTL_EPTYP_CTRL |
                        DOEPCTL_MPSIZ(ep0config.out_maxsize);
  otgp->ie[0].DIEPTSIZ = 0;
  otgp->ie[0].DIEPCTL = DIEPCTL_SD0PID | DIEPCTL_USBAEP | DIEPCTL_EPTYP_CTRL |
                        DIEPCTL_TXFNUM(0) | DIEPCTL_MPSIZ(ep0config.in_maxsize);
  otgp->DIEPTXF0 = DIEPTXF_INEPTXFD(ep0config.in_maxsize / 4) |
                   DIEPTXF_INEPTXSA(otg_ram_alloc(usbp,
                                                  ep0config.in_maxsize / 4));
}

/**
 * @brief   Sets the USB address.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 *
 * @notapi
 */
void usb_lld_set_address(USBDriver *usbp) {
  stm32_otg_t *otgp = usbp->otg;

  otgp->DCFG = (otgp->DCFG & ~DCFG_DAD_MASK) | DCFG_DAD(usbp->address);
}

/**
 * @brief   Enables an endpoint.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 * @param[in] ep        endpoint number
 *
 * @notapi
 */
void usb_lld_init_endpoint(USBDriver *usbp, usbep_t ep) {
  uint32_t ctl, fsize;
  stm32_otg_t *otgp = usbp->otg;

  /* IN and OUT common parameters.*/
  switch (usbp->epc[ep]->ep_mode & USB_EP_MODE_TYPE) {
  case USB_EP_MODE_TYPE_CTRL:
    ctl = DIEPCTL_SD0PID | DIEPCTL_USBAEP | DIEPCTL_EPTYP_CTRL;
    break;
  case USB_EP_MODE_TYPE_ISOC:
    ctl = DIEPCTL_SD0PID | DIEPCTL_USBAEP | DIEPCTL_EPTYP_ISO;
    break;
  case USB_EP_MODE_TYPE_BULK:
    ctl = DIEPCTL_SD0PID | DIEPCTL_USBAEP | DIEPCTL_EPTYP_BULK;
    break;
  case USB_EP_MODE_TYPE_INTR:
    ctl = DIEPCTL_SD0PID | DIEPCTL_USBAEP | DIEPCTL_EPTYP_INTR;
    break;
  default:
    return;
  }

  /* OUT endpoint activation or deactivation.*/
  otgp->oe[ep].DOEPTSIZ = 0;
  if (usbp->epc[ep]->out_cb != NULL) {
    otgp->oe[ep].DOEPCTL = ctl | DOEPCTL_MPSIZ(usbp->epc[ep]->out_maxsize);
    otgp->DAINTMSK |= DAINTMSK_OEPM(ep);
  }
  else {
    otgp->oe[ep].DOEPCTL &= ~DOEPCTL_USBAEP;
    otgp->DAINTMSK &= ~DAINTMSK_OEPM(ep);
  }

  /* IN endpoint activation or deactivation.*/
  otgp->ie[ep].DIEPTSIZ = 0;
  if (usbp->epc[ep]->in_cb != NULL) {
    /* FIFO allocation for the IN endpoint.*/
    fsize = usbp->epc[ep]->in_maxsize / 4;
    if (usbp->epc[ep]->in_multiplier > 1)
      fsize *= usbp->epc[ep]->in_multiplier;
    otgp->DIEPTXF[ep - 1] = DIEPTXF_INEPTXFD(fsize) |
                            DIEPTXF_INEPTXSA(otg_ram_alloc(usbp, fsize));
    otg_txfifo_flush(usbp, ep);

    otgp->ie[ep].DIEPCTL = ctl |
                           DIEPCTL_TXFNUM(ep) |
                           DIEPCTL_MPSIZ(usbp->epc[ep]->in_maxsize);
    otgp->DAINTMSK |= DAINTMSK_IEPM(ep);
  }
  else {
    otgp->DIEPTXF[ep - 1] = 0x02000400; /* Reset value.*/
    otg_txfifo_flush(usbp, ep);
    otgp->ie[ep].DIEPCTL &= ~DIEPCTL_USBAEP;
    otgp->DAINTMSK &= ~DAINTMSK_IEPM(ep);
  }
}

/**
 * @brief   Disables all the active endpoints except the endpoint zero.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 *
 * @notapi
 */
void usb_lld_disable_endpoints(USBDriver *usbp) {

  /* Resets the FIFO memory allocator.*/
  otg_ram_reset(usbp);

  /* Disabling all endpoints.*/
  otg_disable_ep(usbp);
}

/**
 * @brief   Returns the status of an OUT endpoint.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 * @param[in] ep        endpoint number
 * @return              The endpoint status.
 * @retval EP_STATUS_DISABLED The endpoint is not active.
 * @retval EP_STATUS_STALLED  The endpoint is stalled.
 * @retval EP_STATUS_ACTIVE   The endpoint is active.
 *
 * @notapi
 */
usbepstatus_t usb_lld_get_status_out(USBDriver *usbp, usbep_t ep) {
  uint32_t ctl;

  (void)usbp;

  ctl = usbp->otg->oe[ep].DOEPCTL;
  if (!(ctl & DOEPCTL_USBAEP))
    return EP_STATUS_DISABLED;
  if (ctl & DOEPCTL_STALL)
    return EP_STATUS_STALLED;
  return EP_STATUS_ACTIVE;
}

/**
 * @brief   Returns the status of an IN endpoint.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 * @param[in] ep        endpoint number
 * @return              The endpoint status.
 * @retval EP_STATUS_DISABLED The endpoint is not active.
 * @retval EP_STATUS_STALLED  The endpoint is stalled.
 * @retval EP_STATUS_ACTIVE   The endpoint is active.
 *
 * @notapi
 */
usbepstatus_t usb_lld_get_status_in(USBDriver *usbp, usbep_t ep) {
  uint32_t ctl;

  (void)usbp;

  ctl = usbp->otg->ie[ep].DIEPCTL;
  if (!(ctl & DIEPCTL_USBAEP))
    return EP_STATUS_DISABLED;
  if (ctl & DIEPCTL_STALL)
    return EP_STATUS_STALLED;
  return EP_STATUS_ACTIVE;
}

/**
 * @brief   Reads a setup packet from the dedicated packet buffer.
 * @details This function must be invoked in the context of the @p setup_cb
 *          callback in order to read the received setup packet.
 * @pre     In order to use this function the endpoint must have been
 *          initialized as a control endpoint.
 * @post    The endpoint is ready to accept another packet.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 * @param[in] ep        endpoint number
 * @param[out] buf      buffer where to copy the packet data
 *
 * @notapi
 */
void usb_lld_read_setup(USBDriver *usbp, usbep_t ep, uint8_t *buf) {

  memcpy(buf, usbp->epc[ep]->setup_buf, 8);
}

/**
 * @brief   Prepares for a receive operation.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 * @param[in] ep        endpoint number
 *
 * @notapi
 */
void usb_lld_prepare_receive(USBDriver *usbp, usbep_t ep) {
  uint32_t pcnt;
  USBOutEndpointState *osp = usbp->epc[ep]->out_state;

  /* Transfer initialization.*/
  osp->totsize = osp->rxsize;
  if ((ep == 0) && (osp->rxsize  > EP0_MAX_OUTSIZE))
      osp->rxsize = EP0_MAX_OUTSIZE;

  pcnt = (osp->rxsize + usbp->epc[ep]->out_maxsize - 1) /
         usbp->epc[ep]->out_maxsize;
  usbp->otg->oe[ep].DOEPTSIZ = DOEPTSIZ_STUPCNT(3) | DOEPTSIZ_PKTCNT(pcnt) |
                               DOEPTSIZ_XFRSIZ(osp->rxsize);

}

/**
 * @brief   Prepares for a transmit operation.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 * @param[in] ep        endpoint number
 *
 * @notapi
 */
void usb_lld_prepare_transmit(USBDriver *usbp, usbep_t ep) {
  USBInEndpointState *isp = usbp->epc[ep]->in_state;

  /* Transfer initialization.*/
  isp->totsize = isp->txsize;
  if (isp->txsize == 0) {
    /* Special case, sending zero size packet.*/
    usbp->otg->ie[ep].DIEPTSIZ = DIEPTSIZ_PKTCNT(1) | DIEPTSIZ_XFRSIZ(0);
  }
  else {
    if ((ep == 0) && (isp->txsize  > EP0_MAX_INSIZE))
      isp->txsize = EP0_MAX_INSIZE;

    /* Normal case.*/
    uint32_t pcnt = (isp->txsize + usbp->epc[ep]->in_maxsize - 1) /
                    usbp->epc[ep]->in_maxsize;
    usbp->otg->ie[ep].DIEPTSIZ = DIEPTSIZ_PKTCNT(pcnt) |
                                 DIEPTSIZ_XFRSIZ(isp->txsize);
  }
}

/**
 * @brief   Starts a receive operation on an OUT endpoint.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 * @param[in] ep        endpoint number
 *
 * @notapi
 */
void usb_lld_start_out(USBDriver *usbp, usbep_t ep) {

  usbp->otg->oe[ep].DOEPCTL |= DOEPCTL_CNAK;
}

/**
 * @brief   Starts a transmit operation on an IN endpoint.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 * @param[in] ep        endpoint number
 *
 * @notapi
 */
void usb_lld_start_in(USBDriver *usbp, usbep_t ep) {

  usbp->otg->ie[ep].DIEPCTL |= DIEPCTL_EPENA | DIEPCTL_CNAK;
  usbp->otg->DIEPEMPMSK |= DIEPEMPMSK_INEPTXFEM(ep);
}

/**
 * @brief   Brings an OUT endpoint in the stalled state.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 * @param[in] ep        endpoint number
 *
 * @notapi
 */
void usb_lld_stall_out(USBDriver *usbp, usbep_t ep) {

  usbp->otg->oe[ep].DOEPCTL |= DOEPCTL_STALL;
}

/**
 * @brief   Brings an IN endpoint in the stalled state.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 * @param[in] ep        endpoint number
 *
 * @notapi
 */
void usb_lld_stall_in(USBDriver *usbp, usbep_t ep) {

  usbp->otg->ie[ep].DIEPCTL |= DIEPCTL_STALL;
}

/**
 * @brief   Brings an OUT endpoint in the active state.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 * @param[in] ep        endpoint number
 *
 * @notapi
 */
void usb_lld_clear_out(USBDriver *usbp, usbep_t ep) {

  usbp->otg->oe[ep].DOEPCTL &= ~DOEPCTL_STALL;
}

/**
 * @brief   Brings an IN endpoint in the active state.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 * @param[in] ep        endpoint number
 *
 * @notapi
 */
void usb_lld_clear_in(USBDriver *usbp, usbep_t ep) {

  usbp->otg->ie[ep].DIEPCTL &= ~DIEPCTL_STALL;
}

#endif /* HAL_USE_USB */

/** @} */
