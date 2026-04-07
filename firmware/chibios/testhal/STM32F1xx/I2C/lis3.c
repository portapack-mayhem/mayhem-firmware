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
 * This is device realize "read through write" paradigm. This is not
 * standard, but most of I2C devices use this paradigm.
 * You must write to device reading address, send restart to bus,
 * and then begin reading process.
 */

#include <stdlib.h>

#include "ch.h"
#include "hal.h"

#include "lis3.h"


#define lis3_addr 0b0011101


/* buffers */
static uint8_t accel_rx_data[ACCEL_RX_DEPTH];
static uint8_t accel_tx_data[ACCEL_TX_DEPTH];

static i2cflags_t errors = 0;

static int16_t acceleration_x = 0;
static int16_t acceleration_y = 0;
static int16_t acceleration_z = 0;


/**
 * Init function. Here we will also start personal serving thread.
 */
int init_lis3(void){
  msg_t status = RDY_OK;
  systime_t tmo = MS2ST(4);

  /* configure accelerometer */
  accel_tx_data[0] = ACCEL_CTRL_REG1 | AUTO_INCREMENT_BIT; /* register address */
  accel_tx_data[1] = 0b11100111;
  accel_tx_data[2] = 0b01000001;
  accel_tx_data[3] = 0b00000000;

  /* sending */
  i2cAcquireBus(&I2CD1);
  status = i2cMasterTransmitTimeout(&I2CD1, lis3_addr, accel_tx_data, 4, accel_rx_data, 0, tmo);
  i2cReleaseBus(&I2CD1);

  if (status != RDY_OK){
    errors = i2cGetErrors(&I2CD1);
  }

  return 0;
}

/**
 *
 */
void request_acceleration_data(void){
  msg_t status = RDY_OK;
  systime_t tmo = MS2ST(4);

  accel_tx_data[0] = ACCEL_OUT_DATA | AUTO_INCREMENT_BIT; /* register address */
  i2cAcquireBus(&I2CD1);
  status = i2cMasterTransmitTimeout(&I2CD1, lis3_addr, accel_tx_data, 1, accel_rx_data, 6, tmo);
  i2cReleaseBus(&I2CD1);

  if (status != RDY_OK){
    errors = i2cGetErrors(&I2CD1);
  }

  acceleration_x = accel_rx_data[0] + (accel_rx_data[1] << 8);
  acceleration_y = accel_rx_data[2] + (accel_rx_data[3] << 8);
  acceleration_z = accel_rx_data[4] + (accel_rx_data[5] << 8);
}

