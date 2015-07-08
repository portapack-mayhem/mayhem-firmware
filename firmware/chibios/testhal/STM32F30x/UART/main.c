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

#include "ch.h"
#include "hal.h"

static VirtualTimer vt1, vt2;

static void restart(void *p) {

  (void)p;

  chSysLockFromIsr();
  uartStartSendI(&UARTD1, 14, "Hello World!\r\n");
  chSysUnlockFromIsr();
}

static void ledoff(void *p) {

  (void)p;
  palClearPad(GPIOE, GPIOE_LED3_RED);
}

/*
 * This callback is invoked when a transmission buffer has been completely
 * read by the driver.
 */
static void txend1(UARTDriver *uartp) {

  (void)uartp;
  palSetPad(GPIOE, GPIOE_LED3_RED);
}

/*
 * This callback is invoked when a transmission has physically completed.
 */
static void txend2(UARTDriver *uartp) {

  (void)uartp;
  palClearPad(GPIOE, GPIOE_LED3_RED);
  chSysLockFromIsr();
  if (chVTIsArmedI(&vt1))
    chVTResetI(&vt1);
  chVTSetI(&vt1, MS2ST(5000), restart, NULL);
  chSysUnlockFromIsr();
}

/*
 * This callback is invoked on a receive error, the errors mask is passed
 * as parameter.
 */
static void rxerr(UARTDriver *uartp, uartflags_t e) {

  (void)uartp;
  (void)e;
}

/*
 * This callback is invoked when a character is received but the application
 * was not ready to receive it, the character is passed as parameter.
 */
static void rxchar(UARTDriver *uartp, uint16_t c) {

  (void)uartp;
  (void)c;
  /* Flashing the LED each time a character is received.*/
  palSetPad(GPIOE, GPIOE_LED3_RED);
  chSysLockFromIsr();
  if (chVTIsArmedI(&vt2))
    chVTResetI(&vt2);
  chVTSetI(&vt2, MS2ST(200), ledoff, NULL);
  chSysUnlockFromIsr();
}

/*
 * This callback is invoked when a receive buffer has been completely written.
 */
static void rxend(UARTDriver *uartp) {

  (void)uartp;
}

/*
 * UART driver configuration structure.
 */
static UARTConfig uart_cfg_1 = {
  txend1,
  txend2,
  rxend,
  rxchar,
  rxerr,
  38400,
  0,
  USART_CR2_LINEN,
  0
};

/*
 * Application entry point.
 */
int main(void) {

  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  halInit();
  chSysInit();

  /*
   * Activates the serial driver 1, PA9 and PA10 are routed to USART1.
   */
  uartStart(&UARTD1, &uart_cfg_1);
  palSetPadMode(GPIOA, 9, PAL_MODE_ALTERNATE(7));       /* USART1 TX.       */
  palSetPadMode(GPIOA, 10, PAL_MODE_ALTERNATE(7));      /* USART1 RX.       */

  /*
   * Starts the transmission, it will be handled entirely in background.
   */
  uartStartSend(&UARTD1, 13, "Starting...\r\n");

  /*
   * Normal main() thread activity, in this demo it does nothing.
   */
  while (TRUE) {
    chThdSleepMilliseconds(500);
  }
}
