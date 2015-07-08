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
 * Not responding slave test
 */

#include <stdlib.h>

#include "ch.h"
#include "hal.h"

#include "fake.h"


/* input buffer */
static uint8_t rx_data[2];

/* temperature value */
static int16_t temperature = 0;

static i2cflags_t errors = 0;

#define addr 0b1001100

/* This is main function. */
void request_fake(void){
  msg_t status = RDY_OK;
  systime_t tmo = MS2ST(4);

  i2cAcquireBus(&I2CD1);
  status = i2cMasterReceiveTimeout(&I2CD1, addr, rx_data, 2, tmo);
  i2cReleaseBus(&I2CD1);

  if (status == RDY_RESET){
    errors = i2cGetErrors(&I2CD1);
    if (errors == I2CD_ACK_FAILURE){
      /* there is no slave with given address on the bus, or it was die */
      return;
    }
  }

  else{
    temperature = (rx_data[0] << 8) + rx_data[1];
  }
}


