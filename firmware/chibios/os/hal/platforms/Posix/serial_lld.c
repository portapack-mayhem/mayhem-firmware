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
 * @file    Posix/serial_lld.c
 * @brief   Posix low level simulated serial driver code.
 *
 * @addtogroup POSIX_SERIAL
 * @{
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>

#include "ch.h"
#include "hal.h"

#if HAL_USE_SERIAL || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/** @brief Serial driver 1 identifier.*/
#if USE_SIM_SERIAL1 || defined(__DOXYGEN__)
SerialDriver SD1;
#endif
/** @brief Serial driver 2 identifier.*/
#if USE_SIM_SERIAL2 || defined(__DOXYGEN__)
SerialDriver SD2;
#endif

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/** @brief Driver default configuration.*/
static const SerialConfig default_config = {
};

static u_long nb = 1;

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

static void init(SerialDriver *sdp, uint16_t port) {
  struct sockaddr_in sad;
  struct protoent *prtp;
  int sockval = 1;
  socklen_t socklen = sizeof(sockval);


  if ((prtp = getprotobyname("tcp")) == NULL) {
    printf("%s: Error mapping protocol name to protocol number\n", sdp->com_name);
    goto abort;
  }

  sdp->com_listen = socket(PF_INET, SOCK_STREAM, prtp->p_proto);
  if (sdp->com_listen == INVALID_SOCKET) {
    printf("%s: Error creating simulator socket\n", sdp->com_name);
    goto abort;
  }


  setsockopt(sdp->com_listen, SOL_SOCKET, SO_REUSEADDR, &sockval, socklen);

  if (ioctl(sdp->com_listen, FIONBIO, &nb) != 0) {
    printf("%s: Unable to setup non blocking mode on socket\n", sdp->com_name);
    goto abort;
  }

  memset(&sad, 0, sizeof(sad));
  sad.sin_family = AF_INET;
  sad.sin_addr.s_addr = INADDR_ANY;
  sad.sin_port = htons(port);
  if (bind(sdp->com_listen, (struct sockaddr *)&sad, sizeof(sad))) {
    printf("%s: Error binding socket\n", sdp->com_name);
    goto abort;
  }

  if (listen(sdp->com_listen, 1) != 0) {
    printf("%s: Error listening socket\n", sdp->com_name);
    goto abort;
  }
  printf("Full Duplex Channel %s listening on port %d\n", sdp->com_name, port);
  return;

abort:
  if (sdp->com_listen != INVALID_SOCKET)
    close(sdp->com_listen);
  exit(1);
}

static bool_t connint(SerialDriver *sdp) {

  if (sdp->com_data == INVALID_SOCKET) {
    struct sockaddr addr;
    socklen_t addrlen = sizeof(addr);

    if ((sdp->com_data = accept(sdp->com_listen, &addr, &addrlen)) == INVALID_SOCKET)
      return FALSE;

    if (ioctl(sdp->com_data, FIONBIO, &nb) != 0) {
      printf("%s: Unable to setup non blocking mode on data socket\n", sdp->com_name);
      goto abort;
    }
    chSysLockFromIsr();
    chnAddFlagsI(sdp, CHN_CONNECTED);
    chSysUnlockFromIsr();
    return TRUE;
  }
  return FALSE;
abort:
  if (sdp->com_listen != INVALID_SOCKET)
    close(sdp->com_listen);
  if (sdp->com_data != INVALID_SOCKET)
    close(sdp->com_data);
  exit(1);
}

static bool_t inint(SerialDriver *sdp) {

  if (sdp->com_data != INVALID_SOCKET) {
    int i;
    uint8_t data[32];

    /*
     * Input.
     */
    int n = recv(sdp->com_data, data, sizeof(data), 0);
    switch (n) {
    case 0:
      close(sdp->com_data);
      sdp->com_data = INVALID_SOCKET;
      chSysLockFromIsr();
      chnAddFlagsI(sdp, CHN_DISCONNECTED);
      chSysUnlockFromIsr();
      return FALSE;
    case INVALID_SOCKET:
      if (errno == EWOULDBLOCK)
        return FALSE;
      close(sdp->com_data);
      sdp->com_data = INVALID_SOCKET;
      return FALSE;
    }
    for (i = 0; i < n; i++) {
      chSysLockFromIsr();
      sdIncomingDataI(sdp, data[i]);
      chSysUnlockFromIsr();
    }
    return TRUE;
  }
  return FALSE;
}

static bool_t outint(SerialDriver *sdp) {

  if (sdp->com_data != INVALID_SOCKET) {
    int n;
    uint8_t data[1];

    /*
     * Input.
     */
    chSysLockFromIsr();
    n = sdRequestDataI(sdp);
    chSysUnlockFromIsr();
    if (n < 0)
      return FALSE;
    data[0] = (uint8_t)n;
    n = send(sdp->com_data, data, sizeof(data), 0);
    switch (n) {
    case 0:
      close(sdp->com_data);
      sdp->com_data = INVALID_SOCKET;
      chSysLockFromIsr();
      chnAddFlagsI(sdp, CHN_DISCONNECTED);
      chSysUnlockFromIsr();
      return FALSE;
    case INVALID_SOCKET:
      if (errno == EWOULDBLOCK)
        return FALSE;
      close(sdp->com_data);
      sdp->com_data = INVALID_SOCKET;
      return FALSE;
    }
    return TRUE;
  }
  return FALSE;
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level serial driver initialization.
 */
void sd_lld_init(void) {

#if USE_SIM_SERIAL1
  sdObjectInit(&SD1, NULL, NULL);
  SD1.com_listen = INVALID_SOCKET;
  SD1.com_data = INVALID_SOCKET;
  SD1.com_name = "SD1";
#endif

#if USE_SIM_SERIAL2
  sdObjectInit(&SD2, NULL, NULL);
  SD2.com_listen = INVALID_SOCKET;
  SD2.com_data = INVALID_SOCKET;
  SD2.com_name = "SD2";
#endif
}

/**
 * @brief   Low level serial driver configuration and (re)start.
 *
 * @param[in] sdp       pointer to a @p SerialDriver object
 */
void sd_lld_start(SerialDriver *sdp, const SerialConfig *config) {

  if (config == NULL)
    config = &default_config;

#if USE_SIM_SERIAL1
  if (sdp == &SD1)
    init(&SD1, SIM_SD1_PORT);
#endif

#if USE_SIM_SERIAL2
  if (sdp == &SD2)
    init(&SD2, SIM_SD2_PORT);
#endif
}

/**
 * @brief   Low level serial driver stop.
 * @details De-initializes the USART, stops the associated clock, resets the
 *          interrupt vector.
 *
 * @param[in] sdp       pointer to a @p SerialDriver object
 */
void sd_lld_stop(SerialDriver *sdp) {

  (void)sdp;
}

bool_t sd_lld_interrupt_pending(void) {
  bool_t b;

  CH_IRQ_PROLOGUE();

  b =  connint(&SD1) || connint(&SD2) ||
       inint(&SD1)   || inint(&SD2)   ||
       outint(&SD1)  || outint(&SD2);

  CH_IRQ_EPILOGUE();

  return b;
}

#endif /* HAL_USE_SERIAL */

/** @} */
