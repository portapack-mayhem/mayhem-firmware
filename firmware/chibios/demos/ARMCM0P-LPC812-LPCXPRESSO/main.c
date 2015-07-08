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
#include "test.h"

/*
 * RGB LED blinker thread, times are in milliseconds.
 */
static WORKING_AREA(waThread1, 128);

static msg_t Thread1(void *arg) {
  (void)arg;
  chRegSetThreadName("blinker");
  
  palTogglePort(GPIO0, PAL_PORT_BIT(LED_GREEN));

  while (TRUE) {
    palTogglePort(GPIO0, PAL_PORT_BIT(LED_GREEN));
    palTogglePort(GPIO0, PAL_PORT_BIT(LED_RED));
    chThdSleepMilliseconds(500);
    
    palTogglePort(GPIO0, PAL_PORT_BIT(LED_RED));
    palTogglePort(GPIO0, PAL_PORT_BIT(LED_BLUE));
    chThdSleepMilliseconds(500);
    
    palTogglePort(GPIO0, PAL_PORT_BIT(LED_BLUE));
    palTogglePort(GPIO0, PAL_PORT_BIT(LED_GREEN));
    chThdSleepMilliseconds(500);
  }
  
  return 0;
}

int main(void){

  halInit();
  chSysInit();

  sdStart(&SD1, NULL);                  /* Default: 9600,8,N,1. */

  chThdCreateStatic(waThread1, sizeof(waThread1),
                    NORMALPRIO, Thread1, NULL);

  chnWrite( &SD1, (const uint8_t *)"\nhello\n", 8 ); 

  do {
    chThdSleepMilliseconds(500);
  } while (TRUE);
  
  return 0;
}

