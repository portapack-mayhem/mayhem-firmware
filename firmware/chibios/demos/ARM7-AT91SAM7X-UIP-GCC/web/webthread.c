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

#include <string.h>

#include "ch.h"
#include "hal.h"
#include "evtimer.h"

#include "uip.h"
#include "uip_arp.h"
#include "httpd.h"
#include "clock-arch.h"

#define IPADDR0  192
#define IPADDR1  168
#define IPADDR2  1
#define IPADDR3  20

#define SEND_TIMEOUT 50

static const struct uip_eth_addr macaddr = {
  {0xC2, 0xAF, 0x51, 0x03, 0xCF, 0x46}
};

static const MACConfig mac_config = {macaddr.addr};

#define BUF ((struct uip_eth_hdr *)&uip_buf[0])

/*
 * uIP send function wrapping the EMAC functions.
 */
static void network_device_send(void) {
  MACTransmitDescriptor td;

  if (macWaitTransmitDescriptor(&ETHD1, &td, MS2ST(SEND_TIMEOUT)) == RDY_OK) {
    if(uip_len <= UIP_LLH_LEN + UIP_TCPIP_HLEN)
      macWriteTransmitDescriptor(&td, uip_buf, uip_len);
    else {
      macWriteTransmitDescriptor(&td, uip_buf, UIP_LLH_LEN + UIP_TCPIP_HLEN);
      macWriteTransmitDescriptor(&td, uip_appdata,
                                 uip_len - (UIP_LLH_LEN + UIP_TCPIP_HLEN));
    }
    macReleaseTransmitDescriptor(&td);
  }
  /* Dropped... */
}

/*
 * uIP receive function wrapping the EMAC function.
 */
static size_t network_device_read(void) {
  MACReceiveDescriptor rd;
  size_t size;

  if (macWaitReceiveDescriptor(&ETHD1, &rd, TIME_IMMEDIATE) != RDY_OK)
    return 0;
  size = rd.size;
  macReadReceiveDescriptor(&rd, uip_buf, size);
  macReleaseReceiveDescriptor(&rd);
  return size;
}

void clock_init(void) {}

clock_time_t clock_time( void )
{
  return chTimeNow();
}

/*
 * TCP/IP periodic timer.
 */
static void PeriodicTimerHandler(eventid_t id) {
  int i;

  (void)id;
  for (i = 0; i < UIP_CONNS; i++) {
    uip_periodic(i);
    if (uip_len > 0) {
      uip_arp_out();
      network_device_send();
    }
  }
}

/*
 * ARP periodic timer.
 */
static void ARPTimerHandler(eventid_t id) {

  (void)id;
  (void)macPollLinkStatus(&ETHD1);
  uip_arp_timer();
}

/*
 * Ethernet frame received.
 */
static void FrameReceivedHandler(eventid_t id) {

  (void)id;
  while ((uip_len = network_device_read()) > 0) {
    if (BUF->type == HTONS(UIP_ETHTYPE_IP)) {
      uip_arp_ipin();
      uip_input();
      if (uip_len > 0) {
        uip_arp_out();
        network_device_send();
      }
    }
    else if (BUF->type == HTONS(UIP_ETHTYPE_ARP)) {
      uip_arp_arpin();
      if (uip_len > 0)
        network_device_send();
    }
  }
}

#define FRAME_RECEIVED_ID       0
#define PERIODIC_TIMER_ID       1
#define ARP_TIMER_ID            2

static const evhandler_t evhndl[] = {
  FrameReceivedHandler,
  PeriodicTimerHandler,
  ARPTimerHandler
};

msg_t WebThread(void *p) {
  EvTimer evt1, evt2;
  EventListener el0, el1, el2;
  uip_ipaddr_t ipaddr;

  (void)p;

  /*
   * Event sources setup.
   */
  chEvtRegister(macGetReceiveEventSource(&ETHD1), &el0, FRAME_RECEIVED_ID);
  chEvtAddEvents(EVENT_MASK(FRAME_RECEIVED_ID)); /* In case some frames are already buffered */

  evtInit(&evt1, MS2ST(500));
  evtStart(&evt1);
  chEvtRegister(&evt1.et_es, &el1, PERIODIC_TIMER_ID);

  evtInit(&evt2, S2ST(10));
  evtStart(&evt2);
  chEvtRegister(&evt2.et_es, &el2, ARP_TIMER_ID);

  /*
   * EMAC driver start.
   */
  macStart(&ETHD1, &mac_config);
  (void)macPollLinkStatus(&ETHD1);

  /*
   * uIP initialization.
   */
  uip_init();
  uip_setethaddr(macaddr);
  uip_ipaddr(ipaddr, IPADDR0, IPADDR1, IPADDR2, IPADDR3);
  uip_sethostaddr(ipaddr);
  httpd_init();

  while (TRUE) {
    chEvtDispatch(evhndl, chEvtWaitOne(ALL_EVENTS));
  }
  return 0;
}
