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
 * @file    AT91SAM7/mac_lld.c
 * @brief   AT91SAM7 low level MAC driver code.
 *
 * @addtogroup MAC
 * @{
 */

#include <string.h>

#include "ch.h"
#include "hal.h"
#include "mii.h"
#include "at91sam7_mii.h"

#if HAL_USE_MAC || defined(__DOXYGEN__)

#define EMAC_PIN_MASK (AT91C_PB0_ETXCK_EREFCK  | AT91C_PB1_ETXEN         | \
                       AT91C_PB2_ETX0          | AT91C_PB3_ETX1          | \
                       AT91C_PB4_ECRS          | AT91C_PB5_ERX0          | \
                       AT91C_PB6_ERX1          | AT91C_PB7_ERXER         | \
                       AT91C_PB8_EMDC          | AT91C_PB9_EMDIO         | \
                       AT91C_PB10_ETX2         | AT91C_PB11_ETX3         | \
                       AT91C_PB12_ETXER        | AT91C_PB13_ERX2         | \
                       AT91C_PB14_ERX3         | AT91C_PB15_ERXDV_ECRSDV | \
                       AT91C_PB16_ECOL         | AT91C_PB17_ERXCK)

#define RSR_BITS (AT91C_EMAC_BNA | AT91C_EMAC_REC | AT91C_EMAC_OVR)

#define TSR_BITS (AT91C_EMAC_UBR | AT91C_EMAC_COL | AT91C_EMAC_RLES | \
                  AT91C_EMAC_BEX | AT91C_EMAC_COMP | AT91C_EMAC_UND)

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/**
 * @brief   Ethernet driver 1.
 */
MACDriver ETHD1;

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

#ifndef __DOXYGEN__

static uint8_t default_mac[] = {0xAA, 0x55, 0x13, 0x37, 0x01, 0x10};

static EMACDescriptor *rxptr;
static EMACDescriptor *txptr;
static EMACDescriptor rd[EMAC_RECEIVE_DESCRIPTORS]
    __attribute__((aligned(8)));
static EMACDescriptor td[EMAC_TRANSMIT_DESCRIPTORS]
    __attribute__((aligned(8)));
static uint8_t rb[EMAC_RECEIVE_DESCRIPTORS * EMAC_RECEIVE_BUFFERS_SIZE]
    __attribute__((aligned(8)));
static uint8_t tb[EMAC_TRANSMIT_DESCRIPTORS * EMAC_TRANSMIT_BUFFERS_SIZE]
    __attribute__((aligned(8)));
#endif

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/**
 * @brief   IRQ handler.
 */
/** @cond never*/
__attribute__((noinline))
/** @endcond*/
static void serve_interrupt(void) {
  uint32_t isr, rsr, tsr;

  /* Fix for the EMAC errata */
  isr = AT91C_BASE_EMAC->EMAC_ISR;
  rsr = AT91C_BASE_EMAC->EMAC_RSR;
  tsr = AT91C_BASE_EMAC->EMAC_TSR;

  if ((isr & AT91C_EMAC_RCOMP) || (rsr & RSR_BITS)) {
    if (rsr & AT91C_EMAC_REC) {
      chSysLockFromIsr();
      chSemResetI(&ETHD1.rdsem, 0);
#if MAC_USE_EVENTS
      chEvtBroadcastI(&ETHD1.rdevent);
#endif
      chSysUnlockFromIsr();
    }
    AT91C_BASE_EMAC->EMAC_RSR = RSR_BITS;
  }

  if ((isr & AT91C_EMAC_TCOMP) || (tsr & TSR_BITS)) {
    if (tsr & AT91C_EMAC_COMP) {
      chSysLockFromIsr();
      chSemResetI(&ETHD1.tdsem, 0);
      chSysUnlockFromIsr();
    }
    AT91C_BASE_EMAC->EMAC_TSR = TSR_BITS;
  }
  AT91C_BASE_AIC->AIC_EOICR = 0;
}

/**
 * @brief   Cleans an incomplete frame.
 *
 * @param[in] from      the start position of the incomplete frame
 */
static void cleanup(EMACDescriptor *from) {

  while (from != rxptr) {
    from->w1 &= ~W1_R_OWNERSHIP;
    if (++from >= &rd[EMAC_RECEIVE_DESCRIPTORS])
      from = rd;
  }
}

/**
 * @brief   MAC address setup.
 *
 * @param[in] p         pointer to a six bytes buffer containing the MAC
 *                      address
 */
static void set_address(const uint8_t *p) {

  AT91C_BASE_EMAC->EMAC_SA1L = (AT91_REG)((p[3] << 24) | (p[2] << 16) |
                                          (p[1] << 8) | p[0]);
  AT91C_BASE_EMAC->EMAC_SA1H = (AT91_REG)((p[5] << 8) | p[4]);
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/**
 * @brief   EMAC IRQ handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(irq_handler) {

  CH_IRQ_PROLOGUE();

  serve_interrupt();

  CH_IRQ_EPILOGUE();
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level MAC initialization.
 *
 * @notapi
 */
void mac_lld_init(void) {

  miiInit();
  macObjectInit(&ETHD1);

  /*
   * Associated PHY initialization.
   */
  miiReset(&ETHD1);

  /*
   * EMAC pins setup. Note, PB18 is not included because it is
   * used as #PD control and not as EF100.
   */
  AT91C_BASE_PIOB->PIO_ASR = EMAC_PIN_MASK;
  AT91C_BASE_PIOB->PIO_PDR = EMAC_PIN_MASK;
  AT91C_BASE_PIOB->PIO_PPUDR = EMAC_PIN_MASK;
}

/**
 * @brief   Configures and activates the MAC peripheral.
 *
 * @param[in] macp      pointer to the @p MACDriver object
 *
 * @notapi
 */
void mac_lld_start(MACDriver *macp) {
  unsigned i;

  /*
   * Buffers initialization.
   */
  for (i = 0; i < EMAC_RECEIVE_DESCRIPTORS; i++) {
    rd[i].w1 = (uint32_t)&rb[i * EMAC_RECEIVE_BUFFERS_SIZE];
    rd[i].w2 = 0;
  }
  rd[EMAC_RECEIVE_DESCRIPTORS - 1].w1 |= W1_R_WRAP;
  rxptr = rd;
  for (i = 0; i < EMAC_TRANSMIT_DESCRIPTORS; i++) {
    td[i].w1 = (uint32_t)&tb[i * EMAC_TRANSMIT_BUFFERS_SIZE];
    td[i].w2 = EMAC_TRANSMIT_BUFFERS_SIZE | W2_T_LAST_BUFFER | W2_T_USED;
  }
  td[EMAC_TRANSMIT_DESCRIPTORS - 1].w2 |= W2_T_WRAP;
  txptr = td;

  /*
   * EMAC clock enable.
   */
  AT91C_BASE_PMC->PMC_PCER = 1 << AT91C_ID_EMAC;

  /*
   * EMAC Initial setup.
   */
  AT91C_BASE_EMAC->EMAC_NCR = 0;                /* Stopped but MCE active.*/
  AT91C_BASE_EMAC->EMAC_NCFGR = 2 << 10;        /* MDC-CLK = MCK / 32 */
  AT91C_BASE_EMAC->EMAC_USRIO = AT91C_EMAC_CLKEN;/* Enable EMAC in MII mode.*/
  AT91C_BASE_EMAC->EMAC_RBQP = (AT91_REG)rd;    /* RX descriptors list.*/
  AT91C_BASE_EMAC->EMAC_TBQP = (AT91_REG)td;    /* TX descriptors list.*/
  AT91C_BASE_EMAC->EMAC_RSR = AT91C_EMAC_OVR |
                              AT91C_EMAC_REC |
                              AT91C_EMAC_BNA;   /* Clears RSR.*/
  AT91C_BASE_EMAC->EMAC_NCFGR |= AT91C_EMAC_DRFCS;/* Initial NCFGR settings.*/
  AT91C_BASE_EMAC->EMAC_NCR |= AT91C_EMAC_TE |
                               AT91C_EMAC_RE |
                               AT91C_EMAC_CLRSTAT;/* Initial NCR settings.*/
  if (macp->config->mac_address == NULL)
    set_address(default_mac);
  else
    set_address(macp->config->mac_address);

  /*
   * PHY device identification.
   */
  AT91C_BASE_EMAC->EMAC_NCR |= AT91C_EMAC_MPE;
  if ((miiGet(&ETHD1, MII_PHYSID1) != (PHY_ID >> 16)) ||
      ((miiGet(&ETHD1, MII_PHYSID2) & 0xFFF0) != (PHY_ID & 0xFFF0)))
    chSysHalt();
  AT91C_BASE_EMAC->EMAC_NCR &= ~AT91C_EMAC_MPE;

  /*
   * Interrupt configuration.
   */
  AT91C_BASE_EMAC->EMAC_IER = AT91C_EMAC_RCOMP | AT91C_EMAC_TCOMP;
  AIC_ConfigureIT(AT91C_ID_EMAC,
                  AT91C_AIC_SRCTYPE_INT_HIGH_LEVEL | EMAC_INTERRUPT_PRIORITY,
                  irq_handler);
  AIC_EnableIT(AT91C_ID_EMAC);
}

/**
 * @brief   Deactivates the MAC peripheral.
 *
 * @param[in] macp      pointer to the @p MACDriver object
 *
 * @notapi
 */
void mac_lld_stop(MACDriver *macp) {

  (void)macp;
}

/**
 * @brief   Returns a transmission descriptor.
 * @details One of the available transmission descriptors is locked and
 *          returned.
 *
 * @param[in] macp      pointer to the @p MACDriver object
 * @param[out] tdp      pointer to a @p MACTransmitDescriptor structure
 * @return              The operation status.
 * @retval RDY_OK       the descriptor has been obtained.
 * @retval RDY_TIMEOUT  descriptor not available.
 *
 * @notapi
 */
msg_t mac_lld_get_transmit_descriptor(MACDriver *macp,
                                      MACTransmitDescriptor *tdp) {
  EMACDescriptor *edp;

  (void)macp;

  if (!macp->link_up)
    return RDY_TIMEOUT;

  chSysLock();
  edp = txptr;
  if (!(edp->w2 & W2_T_USED) || (edp->w2 & W2_T_LOCKED)) {
    chSysUnlock();
    return RDY_TIMEOUT;
  }
  /*
   * Set the buffer size and configuration, the buffer is also marked
   * as locked.
   */
  if (++txptr >= &td[EMAC_TRANSMIT_DESCRIPTORS]) {
    edp->w2 = W2_T_LOCKED | W2_T_USED | W2_T_LAST_BUFFER | W2_T_WRAP;
    txptr = td;
  }
  else
    edp->w2 = W2_T_LOCKED | W2_T_USED | W2_T_LAST_BUFFER;
  chSysUnlock();
  tdp->offset = 0;
  tdp->size = EMAC_TRANSMIT_BUFFERS_SIZE;
  tdp->physdesc = edp;
  return RDY_OK;
}

/**
 * @brief   Writes to a transmit descriptor's stream.
 *
 * @param[in] tdp       pointer to a @p MACTransmitDescriptor structure
 * @param[in] buf       pointer to the buffer containing the data to be
 *                      written
 * @param[in] size      number of bytes to be written
 * @return              The number of bytes written into the descriptor's
 *                      stream, this value can be less than the amount
 *                      specified in the parameter @p size if the maximum
 *                      frame size is reached.
 *
 * @notapi
 */
size_t mac_lld_write_transmit_descriptor(MACTransmitDescriptor *tdp,
                                         uint8_t *buf,
                                         size_t size) {

  if (size > tdp->size - tdp->offset)
    size = tdp->size - tdp->offset;
  if (size > 0) {
    memcpy((uint8_t *)(tdp->physdesc->w1 & W1_T_ADDRESS_MASK) +
                      tdp->offset,
           buf, size);
    tdp->offset += size;
  }
  return size;
}

/**
 * @brief   Releases a transmit descriptor and starts the transmission of the
 *          enqueued data as a single frame.
 *
 * @param[in] tdp       the pointer to the @p MACTransmitDescriptor structure
 *
 * @notapi
 */
void mac_lld_release_transmit_descriptor(MACTransmitDescriptor *tdp) {

  chSysLock();
  tdp->physdesc->w2 = (tdp->physdesc->w2 &
                          ~(W2_T_LOCKED | W2_T_USED | W2_T_LENGTH_MASK)) |
                         tdp->offset;
  AT91C_BASE_EMAC->EMAC_NCR |= AT91C_EMAC_TSTART;
  chSysUnlock();
}

/**
 * @brief   Returns a receive descriptor.
 *
 * @param[in] macp      pointer to the @p MACDriver object
 * @param[out] rdp      pointer to a @p MACReceiveDescriptor structure
 * @return              The operation status.
 * @retval RDY_OK       the descriptor has been obtained.
 * @retval RDY_TIMEOUT  descriptor not available.
 *
 * @notapi
 */
msg_t mac_lld_get_receive_descriptor(MACDriver *macp,
                                     MACReceiveDescriptor *rdp) {
  unsigned n;
  EMACDescriptor *edp;

  (void)macp;
  n = EMAC_RECEIVE_DESCRIPTORS;

  /*
   * Skips unused buffers, if any.
   */
skip:
  while ((n > 0) && !(rxptr->w1 & W1_R_OWNERSHIP)) {
    if (++rxptr >= &rd[EMAC_RECEIVE_DESCRIPTORS])
      rxptr = rd;
    n--;
  }

  /*
   * Skips fragments, if any, cleaning them up.
   */
  while ((n > 0) && (rxptr->w1 & W1_R_OWNERSHIP) &&
                    !(rxptr->w2 & W2_R_FRAME_START)) {
    rxptr->w1 &= ~W1_R_OWNERSHIP;
    if (++rxptr >= &rd[EMAC_RECEIVE_DESCRIPTORS])
      rxptr = rd;
    n--;
  }

  /*
   * Now compute the total frame size skipping eventual incomplete frames
   * or holes...
   */
restart:
  edp = rxptr;
  while (n > 0) {
    if (!(rxptr->w1 & W1_R_OWNERSHIP)) {
      /* Empty buffer for some reason... cleaning up the incomplete frame.*/
      cleanup(edp);
      goto skip;
    }
    /*
     * End Of Frame found.
     */
    if (rxptr->w2 & W2_R_FRAME_END) {
      rdp->offset = 0;
      rdp->size = rxptr->w2 & W2_T_LENGTH_MASK;
      rdp->physdesc = edp;
      return RDY_OK;
    }

    if ((edp != rxptr) && (rxptr->w2 & W2_R_FRAME_START)) {
      /* Found another start... cleaning up the incomplete frame.*/
      cleanup(edp);
      goto restart;     /* Another start buffer for some reason... */
    }

    if (++rxptr >= &rd[EMAC_RECEIVE_DESCRIPTORS])
      rxptr = rd;
    n--;
  }
  return RDY_TIMEOUT;
}

/**
 * @brief   Reads from a receive descriptor's stream.
 *
 * @param[in] rdp       pointer to a @p MACReceiveDescriptor structure
 * @param[in] buf       pointer to the buffer that will receive the read data
 * @param[in] size      number of bytes to be read
 * @return              The number of bytes read from the descriptor's
 *                      stream, this value can be less than the amount
 *                      specified in the parameter @p size if there are
 *                      no more bytes to read.
 *
 * @notapi
 */
size_t mac_lld_read_receive_descriptor(MACReceiveDescriptor *rdp,
                                         uint8_t *buf,
                                         size_t size) {
  if (size > rdp->size - rdp->offset)
    size = rdp->size - rdp->offset;
  if (size > 0) {
    uint8_t *src = (uint8_t *)(rdp->physdesc->w1 & W1_R_ADDRESS_MASK) +
                   rdp->offset;
    uint8_t *limit = &rb[EMAC_RECEIVE_DESCRIPTORS * EMAC_RECEIVE_BUFFERS_SIZE];
    if (src >= limit)
      src -= EMAC_RECEIVE_DESCRIPTORS * EMAC_RECEIVE_BUFFERS_SIZE;
    if (src + size > limit ) {
      memcpy(buf, src, (size_t)(limit - src));
      memcpy(buf + (size_t)(limit - src), rb, size - (size_t)(limit - src));
    }
    else
      memcpy(buf, src, size);
    rdp->offset += size;
  }
  return size;
}

/**
 * @brief   Releases a receive descriptor.
 * @details The descriptor and its buffer are made available for more incoming
 *          frames.
 *
 * @param[in] rdp       the pointer to the @p MACReceiveDescriptor structure
 *
 * @notapi
 */
void mac_lld_release_receive_descriptor(MACReceiveDescriptor *rdp) {
  bool_t done;
  EMACDescriptor *edp = rdp->physdesc;

  unsigned n = EMAC_RECEIVE_DESCRIPTORS;
  do {
    done = ((edp->w2 & W2_R_FRAME_END) != 0);
    chDbgAssert(edp->w1 & W1_R_OWNERSHIP,
                "mac_lld_release_receive_descriptor(), #1",
                "found not owned descriptor");
    edp->w1 &= ~(W1_R_OWNERSHIP | W2_R_FRAME_START | W2_R_FRAME_END);
    if (++edp >= &rd[EMAC_RECEIVE_DESCRIPTORS])
      edp = rd;
    n--;
  }
  while ((n > 0) && !done);
  /*
   * Make rxptr point to the descriptor where the next frame will most
   * likely appear.
   */
  rxptr = edp;
}

/**
 * @brief   Updates and returns the link status.
 *
 * @param[in] macp      pointer to the @p MACDriver object
 * @return              The link status.
 * @retval TRUE         if the link is active.
 * @retval FALSE        if the link is down.
 *
 * @notapi
 */
bool_t mac_lld_poll_link_status(MACDriver *macp) {
  uint32_t ncfgr, bmsr, bmcr, lpa;

  AT91C_BASE_EMAC->EMAC_NCR |= AT91C_EMAC_MPE;
  (void)miiGet(macp, MII_BMSR);
  bmsr = miiGet(macp, MII_BMSR);
  if (!(bmsr & BMSR_LSTATUS)) {
    AT91C_BASE_EMAC->EMAC_NCR &= ~AT91C_EMAC_MPE;
    return macp->link_up = FALSE;
  }

  ncfgr = AT91C_BASE_EMAC->EMAC_NCFGR & ~(AT91C_EMAC_SPD | AT91C_EMAC_FD);
  bmcr = miiGet(macp, MII_BMCR);
  if (bmcr & BMCR_ANENABLE) {
    lpa = miiGet(macp, MII_LPA);
    if (lpa & (LPA_100HALF | LPA_100FULL | LPA_100BASE4))
      ncfgr |= AT91C_EMAC_SPD;
    if (lpa & (LPA_10FULL | LPA_100FULL))
      ncfgr |= AT91C_EMAC_FD;
  }
  else {
    if (bmcr & BMCR_SPEED100)
      ncfgr |= AT91C_EMAC_SPD;
    if (bmcr & BMCR_FULLDPLX)
      ncfgr |= AT91C_EMAC_FD;
  }
  AT91C_BASE_EMAC->EMAC_NCFGR = ncfgr;
  AT91C_BASE_EMAC->EMAC_NCR &= ~AT91C_EMAC_MPE;
  return macp->link_up = TRUE;
}

#endif /* HAL_USE_MAC */

/** @} */
