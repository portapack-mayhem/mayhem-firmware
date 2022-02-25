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
 * @file    STM32/mac_lld.c
 * @brief   STM32 low level MAC driver code.
 *
 * @addtogroup MAC
 * @{
 */

#include <string.h>

#include "ch.h"
#include "hal.h"
#include "mii.h"

#if HAL_USE_MAC || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

#define BUFFER_SIZE ((((STM32_MAC_BUFFERS_SIZE - 1) | 3) + 1) / 4)

/* MII divider optimal value.*/
#if (STM32_HCLK >= 150000000)
#define MACMIIDR_CR ETH_MACMIIAR_CR_Div102
#elif (STM32_HCLK >= 100000000)
#define MACMIIDR_CR ETH_MACMIIAR_CR_Div62
#elif (STM32_HCLK >= 60000000)
#define MACMIIDR_CR     ETH_MACMIIAR_CR_Div42
#elif (STM32_HCLK >= 35000000)
#define MACMIIDR_CR     ETH_MACMIIAR_CR_Div26
#elif (STM32_HCLK >= 20000000)
#define MACMIIDR_CR     ETH_MACMIIAR_CR_Div16
#else
#error "STM32_HCLK below minimum frequency for ETH operations (20MHz)"
#endif

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

static const uint8_t default_mac_address[] = {0xAA, 0x55, 0x13,
                                              0x37, 0x01, 0x10};

static stm32_eth_rx_descriptor_t rd[STM32_MAC_RECEIVE_BUFFERS];
static stm32_eth_tx_descriptor_t td[STM32_MAC_TRANSMIT_BUFFERS];

static uint32_t rb[STM32_MAC_RECEIVE_BUFFERS][BUFFER_SIZE];
static uint32_t tb[STM32_MAC_TRANSMIT_BUFFERS][BUFFER_SIZE];

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/**
 * @brief   Writes a PHY register.
 *
 * @param[in] macp      pointer to the @p MACDriver object
 * @param[in] reg       register number
 * @param[in] value     new register value
 *
 * @notapi
 */
void mii_write(MACDriver *macp, uint32_t reg, uint32_t value) {

  ETH->MACMIIDR = value;
  ETH->MACMIIAR = macp->phyaddr | (reg << 6) | MACMIIDR_CR |
                  ETH_MACMIIAR_MW | ETH_MACMIIAR_MB;
  while ((ETH->MACMIIAR & ETH_MACMIIAR_MB) != 0)
    ;
}

/**
 * @brief   Reads a PHY register.
 *
 * @param[in] macp      pointer to the @p MACDriver object
 * @param[in] reg       register number
 *
 * @return              The PHY register content.
 *
 * @notapi
 */
uint32_t mii_read(MACDriver *macp, uint32_t reg) {

  ETH->MACMIIAR = macp->phyaddr | (reg << 6) | MACMIIDR_CR | ETH_MACMIIAR_MB;
  while ((ETH->MACMIIAR & ETH_MACMIIAR_MB) != 0)
    ;
  return ETH->MACMIIDR;
}

#if !defined(BOARD_PHY_ADDRESS)
/**
 * @brief   PHY address detection.
 *
 * @param[in] macp      pointer to the @p MACDriver object
 */
static void mii_find_phy(MACDriver *macp) {
  uint32_t i;

#if STM32_MAC_PHY_TIMEOUT > 0
  halrtcnt_t start = halGetCounterValue();
  halrtcnt_t timeout  = start + MS2RTT(STM32_MAC_PHY_TIMEOUT);
  while (halIsCounterWithin(start, timeout)) {
#endif
    for (i = 0; i < 31; i++) {
      macp->phyaddr = i << 11;
      ETH->MACMIIDR = (i << 6) | MACMIIDR_CR;
      if ((mii_read(macp, MII_PHYSID1) == (BOARD_PHY_ID >> 16)) &&
          ((mii_read(macp, MII_PHYSID2) & 0xFFF0) == (BOARD_PHY_ID & 0xFFF0))) {
        return;
      }
    }
#if STM32_MAC_PHY_TIMEOUT > 0
  }
#endif
  /* Wrong or defective board.*/
  chSysHalt();
}
#endif

/**
 * @brief   MAC address setup.
 *
 * @param[in] p         pointer to a six bytes buffer containing the MAC
 *                      address
 */
static void mac_lld_set_address(const uint8_t *p) {

  /* MAC address configuration, only a single address comparator is used,
     hash table not used.*/
  ETH->MACA0HR   = ((uint32_t)p[5] << 8) |
                   ((uint32_t)p[4] << 0);
  ETH->MACA0LR   = ((uint32_t)p[3] << 24) |
                   ((uint32_t)p[2] << 16) |
                   ((uint32_t)p[1] << 8) |
                   ((uint32_t)p[0] << 0);
  ETH->MACA1HR   = 0x0000FFFF;
  ETH->MACA1LR   = 0xFFFFFFFF;
  ETH->MACA2HR   = 0x0000FFFF;
  ETH->MACA2LR   = 0xFFFFFFFF;
  ETH->MACA3HR   = 0x0000FFFF;
  ETH->MACA3LR   = 0xFFFFFFFF;
  ETH->MACHTHR   = 0;
  ETH->MACHTLR   = 0;
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

CH_IRQ_HANDLER(ETH_IRQHandler) {
  uint32_t dmasr;

  CH_IRQ_PROLOGUE();

  dmasr = ETH->DMASR;
  ETH->DMASR = dmasr; /* Clear status bits.*/

  if (dmasr & ETH_DMASR_RS) {
    /* Data Received.*/
    chSysLockFromIsr();
    chSemResetI(&ETHD1.rdsem, 0);
#if MAC_USE_EVENTS
    chEvtBroadcastI(&ETHD1.rdevent);
#endif
    chSysUnlockFromIsr();
  }

  if (dmasr & ETH_DMASR_TS) {
    /* Data Transmitted.*/
    chSysLockFromIsr();
    chSemResetI(&ETHD1.tdsem, 0);
    chSysUnlockFromIsr();
  }

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
  unsigned i;

  macObjectInit(&ETHD1);
  ETHD1.link_up = FALSE;

  /* Descriptor tables are initialized in chained mode, note that the first
     word is not initialized here but in mac_lld_start().*/
  for (i = 0; i < STM32_MAC_RECEIVE_BUFFERS; i++) {
    rd[i].rdes1 = STM32_RDES1_RCH | STM32_MAC_BUFFERS_SIZE;
    rd[i].rdes2 = (uint32_t)rb[i];
    rd[i].rdes3 = (uint32_t)&rd[(i + 1) % STM32_MAC_RECEIVE_BUFFERS];
  }
  for (i = 0; i < STM32_MAC_TRANSMIT_BUFFERS; i++) {
    td[i].tdes1 = 0;
    td[i].tdes2 = (uint32_t)tb[i];
    td[i].tdes3 = (uint32_t)&td[(i + 1) % STM32_MAC_TRANSMIT_BUFFERS];
  }

  /* Selection of the RMII or MII mode based on info exported by board.h.*/
#if defined(STM32F10X_CL)
#if defined(BOARD_PHY_RMII)
  AFIO->MAPR |= AFIO_MAPR_MII_RMII_SEL;
#else
  AFIO->MAPR &= ~AFIO_MAPR_MII_RMII_SEL;
#endif
#elif defined(STM32F2XX) || defined(STM32F4XX)
#if defined(BOARD_PHY_RMII)
  SYSCFG->PMC |= SYSCFG_PMC_MII_RMII_SEL;
#else
  SYSCFG->PMC &= ~SYSCFG_PMC_MII_RMII_SEL;
#endif
#else
#error "unsupported STM32 platform for MAC driver"
#endif

  /* Reset of the MAC core.*/
  rccResetETH();

  /* MAC clocks temporary activation.*/
  rccEnableETH(FALSE);

  /* PHY address setup.*/
#if defined(BOARD_PHY_ADDRESS)
  ETHD1.phyaddr = BOARD_PHY_ADDRESS << 11;
#else
  mii_find_phy(&ETHD1);
#endif

#if defined(BOARD_PHY_RESET)
  /* PHY board-specific reset procedure.*/
  BOARD_PHY_RESET();
#else
  /* PHY soft reset procedure.*/
  mii_write(&ETHD1, MII_BMCR, BMCR_RESET);
#if defined(BOARD_PHY_RESET_DELAY)
  halPolledDelay(BOARD_PHY_RESET_DELAY);
#endif
  while (mii_read(&ETHD1, MII_BMCR) & BMCR_RESET)
    ;
#endif

#if STM32_MAC_ETH1_CHANGE_PHY_STATE
  /* PHY in power down mode until the driver will be started.*/
  mii_write(&ETHD1, MII_BMCR, mii_read(&ETHD1, MII_BMCR) | BMCR_PDOWN);
#endif

  /* MAC clocks stopped again.*/
  rccDisableETH(FALSE);
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

  /* Resets the state of all descriptors.*/
  for (i = 0; i < STM32_MAC_RECEIVE_BUFFERS; i++)
    rd[i].rdes0 = STM32_RDES0_OWN;
  macp->rxptr = (stm32_eth_rx_descriptor_t *)rd;
  for (i = 0; i < STM32_MAC_TRANSMIT_BUFFERS; i++)
    td[i].tdes0 = STM32_TDES0_TCH;
  macp->txptr = (stm32_eth_tx_descriptor_t *)td;

  /* MAC clocks activation and commanded reset procedure.*/
  rccEnableETH(FALSE);
#if defined(STM32_MAC_DMABMR_SR)
  ETH->DMABMR |= ETH_DMABMR_SR;
  while(ETH->DMABMR & ETH_DMABMR_SR)
    ;
#endif

  /* ISR vector enabled.*/
  nvicEnableVector(ETH_IRQn,
                   CORTEX_PRIORITY_MASK(STM32_MAC_ETH1_IRQ_PRIORITY));

#if STM32_MAC_ETH1_CHANGE_PHY_STATE
  /* PHY in power up mode.*/
  mii_write(macp, MII_BMCR, mii_read(macp, MII_BMCR) & ~BMCR_PDOWN);
#endif

  /* MAC configuration.*/
  ETH->MACFFR    = 0;
  ETH->MACFCR    = 0;
  ETH->MACVLANTR = 0;

  /* MAC address setup.*/
  if (macp->config->mac_address == NULL)
    mac_lld_set_address(default_mac_address);
  else
    mac_lld_set_address(macp->config->mac_address);

  /* Transmitter and receiver enabled.
     Note that the complete setup of the MAC is performed when the link
     status is detected.*/
#if STM32_MAC_IP_CHECKSUM_OFFLOAD
  ETH->MACCR = ETH_MACCR_IPCO | ETH_MACCR_RE | ETH_MACCR_TE;
#else
  ETH->MACCR =                  ETH_MACCR_RE | ETH_MACCR_TE;
#endif

  /* DMA configuration:
     Descriptor chains pointers.*/
  ETH->DMARDLAR = (uint32_t)rd;
  ETH->DMATDLAR = (uint32_t)td;

  /* Enabling required interrupt sources.*/
  ETH->DMASR    = ETH->DMASR;
  ETH->DMAIER   = ETH_DMAIER_NISE | ETH_DMAIER_RIE | ETH_DMAIER_TIE;

  /* DMA general settings.*/
  ETH->DMABMR   = ETH_DMABMR_AAB | ETH_DMABMR_RDP_1Beat | ETH_DMABMR_PBL_1Beat;

  /* Transmit FIFO flush.*/
  ETH->DMAOMR   = ETH_DMAOMR_FTF;
  while (ETH->DMAOMR & ETH_DMAOMR_FTF)
    ;

  /* DMA final configuration and start.*/
  ETH->DMAOMR   = ETH_DMAOMR_DTCEFD | ETH_DMAOMR_RSF | ETH_DMAOMR_TSF |
                  ETH_DMAOMR_ST | ETH_DMAOMR_SR;
}

/**
 * @brief   Deactivates the MAC peripheral.
 *
 * @param[in] macp      pointer to the @p MACDriver object
 *
 * @notapi
 */
void mac_lld_stop(MACDriver *macp) {

  if (macp->state != MAC_STOP) {
#if STM32_MAC_ETH1_CHANGE_PHY_STATE
    /* PHY in power down mode until the driver will be restarted.*/
    mii_write(macp, MII_BMCR, mii_read(macp, MII_BMCR) | BMCR_PDOWN);
#endif

    /* MAC and DMA stopped.*/
    ETH->MACCR    = 0;
    ETH->DMAOMR   = 0;
    ETH->DMAIER   = 0;
    ETH->DMASR    = ETH->DMASR;

    /* MAC clocks stopped.*/
    rccDisableETH(FALSE);

    /* ISR vector disabled.*/
    nvicDisableVector(ETH_IRQn);
  }
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
  stm32_eth_tx_descriptor_t *tdes;

  if (!macp->link_up)
    return RDY_TIMEOUT;

  chSysLock();

  /* Get Current TX descriptor.*/
  tdes = macp->txptr;

  /* Ensure that descriptor isn't owned by the Ethernet DMA or locked by
     another thread.*/
  if (tdes->tdes0 & (STM32_TDES0_OWN | STM32_TDES0_LOCKED)) {
    chSysUnlock();
    return RDY_TIMEOUT;
  }

  /* Marks the current descriptor as locked using a reserved bit.*/
  tdes->tdes0 |= STM32_TDES0_LOCKED;

  /* Next TX descriptor to use.*/
  macp->txptr = (stm32_eth_tx_descriptor_t *)tdes->tdes3;

  chSysUnlock();

  /* Set the buffer size and configuration.*/
  tdp->offset   = 0;
  tdp->size     = STM32_MAC_BUFFERS_SIZE;
  tdp->physdesc = tdes;

  return RDY_OK;
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

  chDbgAssert(!(tdp->physdesc->tdes0 & STM32_TDES0_OWN),
              "mac_lld_release_transmit_descriptor(), #1",
              "attempt to release descriptor already owned by DMA");

  chSysLock();

  /* Unlocks the descriptor and returns it to the DMA engine.*/
  tdp->physdesc->tdes1 = tdp->offset;
  tdp->physdesc->tdes0 = STM32_TDES0_CIC(STM32_MAC_IP_CHECKSUM_OFFLOAD) |
                         STM32_TDES0_IC | STM32_TDES0_LS | STM32_TDES0_FS |
                         STM32_TDES0_TCH | STM32_TDES0_OWN;

  /* If the DMA engine is stalled then a restart request is issued.*/
  if ((ETH->DMASR & ETH_DMASR_TPS) == ETH_DMASR_TPS_Suspended) {
    ETH->DMASR   = ETH_DMASR_TBUS;
    ETH->DMATPDR = ETH_DMASR_TBUS; /* Any value is OK.*/
  }

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
  stm32_eth_rx_descriptor_t *rdes;

  chSysLock();

  /* Get Current RX descriptor.*/
  rdes = macp->rxptr;

  /* Iterates through received frames until a valid one is found, invalid
     frames are discarded.*/
  while (!(rdes->rdes0 & STM32_RDES0_OWN)) {
    if (!(rdes->rdes0 & (STM32_RDES0_AFM | STM32_RDES0_ES))
#if STM32_MAC_IP_CHECKSUM_OFFLOAD
        && (rdes->rdes0 & STM32_RDES0_FT)
        && !(rdes->rdes0 & (STM32_RDES0_IPHCE | STM32_RDES0_PCE))
#endif
        && (rdes->rdes0 & STM32_RDES0_FS) && (rdes->rdes0 & STM32_RDES0_LS)) {
      /* Found a valid one.*/
      rdp->offset   = 0;
      rdp->size     = ((rdes->rdes0 & STM32_RDES0_FL_MASK) >> 16) - 4;
      rdp->physdesc = rdes;
      macp->rxptr   = (stm32_eth_rx_descriptor_t *)rdes->rdes3;

      chSysUnlock();
      return RDY_OK;
    }
    /* Invalid frame found, purging.*/
    rdes->rdes0 = STM32_RDES0_OWN;
    rdes = (stm32_eth_rx_descriptor_t *)rdes->rdes3;
  }

  /* Next descriptor to check.*/
  macp->rxptr = rdes;

  chSysUnlock();
  return RDY_TIMEOUT;
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

  chDbgAssert(!(rdp->physdesc->rdes0 & STM32_RDES0_OWN),
              "mac_lld_release_receive_descriptor(), #1",
              "attempt to release descriptor already owned by DMA");

  chSysLock();

  /* Give buffer back to the Ethernet DMA.*/
  rdp->physdesc->rdes0 = STM32_RDES0_OWN;

  /* If the DMA engine is stalled then a restart request is issued.*/
  if ((ETH->DMASR & ETH_DMASR_RPS) == ETH_DMASR_RPS_Suspended) {
    ETH->DMASR   = ETH_DMASR_RBUS;
    ETH->DMARPDR = ETH_DMASR_RBUS; /* Any value is OK.*/
  }

  chSysUnlock();
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
  uint32_t maccr, bmsr, bmcr;

  maccr = ETH->MACCR;

  /* PHY CR and SR registers read.*/
  (void)mii_read(macp, MII_BMSR);
  bmsr = mii_read(macp, MII_BMSR);
  bmcr = mii_read(macp, MII_BMCR);

  /* Check on auto-negotiation mode.*/
  if (bmcr & BMCR_ANENABLE) {
    uint32_t lpa;

    /* Auto-negotiation must be finished without faults and link established.*/
    if ((bmsr & (BMSR_LSTATUS | BMSR_RFAULT | BMSR_ANEGCOMPLETE)) !=
        (BMSR_LSTATUS | BMSR_ANEGCOMPLETE))
      return macp->link_up = FALSE;

    /* Auto-negotiation enabled, checks the LPA register.*/
    lpa = mii_read(macp, MII_LPA);

    /* Check on link speed.*/
    if (lpa & (LPA_100HALF | LPA_100FULL | LPA_100BASE4))
      maccr |= ETH_MACCR_FES;
    else
      maccr &= ~ETH_MACCR_FES;

    /* Check on link mode.*/
    if (lpa & (LPA_10FULL | LPA_100FULL))
      maccr |= ETH_MACCR_DM;
    else
      maccr &= ~ETH_MACCR_DM;
  }
  else {
    /* Link must be established.*/
    if (!(bmsr & BMSR_LSTATUS))
      return macp->link_up = FALSE;

    /* Check on link speed.*/
    if (bmcr & BMCR_SPEED100)
      maccr |= ETH_MACCR_FES;
    else
      maccr &= ~ETH_MACCR_FES;

    /* Check on link mode.*/
    if (bmcr & BMCR_FULLDPLX)
      maccr |= ETH_MACCR_DM;
    else
      maccr &= ~ETH_MACCR_DM;
  }

  /* Changes the mode in the MAC.*/
  ETH->MACCR = maccr;

  /* Returns the link status.*/
  return macp->link_up = TRUE;
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

  chDbgAssert(!(tdp->physdesc->tdes0 & STM32_TDES0_OWN),
              "mac_lld_write_transmit_descriptor(), #1",
              "attempt to write descriptor already owned by DMA");

  if (size > tdp->size - tdp->offset)
    size = tdp->size - tdp->offset;

  if (size > 0) {
    memcpy((uint8_t *)(tdp->physdesc->tdes2) + tdp->offset, buf, size);
    tdp->offset += size;
  }
  return size;
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

  chDbgAssert(!(rdp->physdesc->rdes0 & STM32_RDES0_OWN),
              "mac_lld_read_receive_descriptor(), #1",
              "attempt to read descriptor already owned by DMA");

  if (size > rdp->size - rdp->offset)
    size = rdp->size - rdp->offset;

  if (size > 0) {
    memcpy(buf, (uint8_t *)(rdp->physdesc->rdes2) + rdp->offset, size);
    rdp->offset += size;
  }
  return size;
}

#if MAC_USE_ZERO_COPY || defined(__DOXYGEN__)
/**
 * @brief   Returns a pointer to the next transmit buffer in the descriptor
 *          chain.
 * @note    The API guarantees that enough buffers can be requested to fill
 *          a whole frame.
 *
 * @param[in] tdp       pointer to a @p MACTransmitDescriptor structure
 * @param[in] size      size of the requested buffer. Specify the frame size
 *                      on the first call then scale the value down subtracting
 *                      the amount of data already copied into the previous
 *                      buffers.
 * @param[out] sizep    pointer to variable receiving the buffer size, it is
 *                      zero when the last buffer has already been returned.
 *                      Note that a returned size lower than the amount
 *                      requested means that more buffers must be requested
 *                      in order to fill the frame data entirely.
 * @return              Pointer to the returned buffer.
 * @retval NULL         if the buffer chain has been entirely scanned.
 *
 * @notapi
 */
uint8_t *mac_lld_get_next_transmit_buffer(MACTransmitDescriptor *tdp,
                                          size_t size,
                                          size_t *sizep) {

  if (tdp->offset == 0) {
    *sizep      = tdp->size;
    tdp->offset = size;
    return (uint8_t *)tdp->physdesc->tdes2;
  }
  *sizep = 0;
  return NULL;
}

/**
 * @brief   Returns a pointer to the next receive buffer in the descriptor
 *          chain.
 * @note    The API guarantees that the descriptor chain contains a whole
 *          frame.
 *
 * @param[in] rdp       pointer to a @p MACReceiveDescriptor structure
 * @param[out] sizep    pointer to variable receiving the buffer size, it is
 *                      zero when the last buffer has already been returned.
 * @return              Pointer to the returned buffer.
 * @retval NULL         if the buffer chain has been entirely scanned.
 *
 * @notapi
 */
const uint8_t *mac_lld_get_next_receive_buffer(MACReceiveDescriptor *rdp,
                                               size_t *sizep) {

  if (rdp->size > 0) {
    *sizep      = rdp->size;
    rdp->offset = rdp->size;
    rdp->size   = 0;
    return (uint8_t *)rdp->physdesc->rdes2;
  }
  *sizep = 0;
  return NULL;
}
#endif /* MAC_USE_ZERO_COPY */

#endif /* HAL_USE_MAC */

/** @} */
