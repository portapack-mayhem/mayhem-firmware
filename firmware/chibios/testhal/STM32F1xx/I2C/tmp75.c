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
 * TMP75 is most simple I2C device in our case. It is already useful with
 * default settings after powerup.
 * You only must read 2 sequential bytes from it.
 */

#include <stdlib.h>

#include "ch.h"
#include "hal.h"

#include "tmp75.h"


/* input buffer */
static uint8_t tmp75_rx_data[TMP75_RX_DEPTH];

/* temperature value */
static int16_t temperature = 0;

static i2cflags_t errors = 0;

#define tmp75_addr 0b1001000

/* This is main function. */
void request_temperature(void){
  int16_t t_int = 0, t_frac = 0;
  msg_t status = RDY_OK;
  systime_t tmo = MS2ST(4);

  i2cAcquireBus(&I2CD1);
  status = i2cMasterReceiveTimeout(&I2CD1, tmp75_addr, tmp75_rx_data, 2, tmo);
  i2cReleaseBus(&I2CD1);

  if (status != RDY_OK){
    errors = i2cGetErrors(&I2CD1);
  }

  t_int = tmp75_rx_data[0] * 100;
  t_frac = (tmp75_rx_data[1] * 100) >> 8;
  temperature = t_int + t_frac;
}


