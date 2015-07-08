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
/*
   Concepts and parts of this file have been contributed by Uladzimir Pylinsky
   aka barthess.
 */

/**
 * This demo acquire data from accelerometer and prints it in shell.
 */

#include <stdlib.h>

#include "ch.h"
#include "hal.h"

/* buffers depth */
#define ACCEL_RX_DEPTH 6
#define ACCEL_TX_DEPTH 4

/* mma8451q specific addresses */
#define ACCEL_OUT_DATA    0x01
#define ACCEL_CTRL_REG1   0x2A

static uint8_t rxbuf[ACCEL_RX_DEPTH];
static uint8_t txbuf[ACCEL_TX_DEPTH];
static i2cflags_t errors = 0;
static int16_t acceleration_x, acceleration_y, acceleration_z;
#define mma8451_addr 0b0011100

/**
 *
 */
static void print(char *p) {

  while (*p) {
    sdPut(&SD2, *p++);
  }
}

/**
 *
 */
static void println(char *p) {

  while (*p) {
    sdPut(&SD2, *p++);
  }
  sdWriteTimeout(&SD2, (uint8_t *)"\r\n", 2, TIME_INFINITE);
}

/**
 *
 */
static void printn(int16_t n) {
  char buf[16], *p;

  if (n > 0)
    sdPut(&SD2, '+');
  else{
    sdPut(&SD2, '-');
    n = abs(n);
  }

  if (!n)
    sdPut(&SD2, '0');
  else {
    p = buf;
    while (n)
      *p++ = (n % 10) + '0', n /= 10;
    while (p > buf)
      sdPut(&SD2, *--p);
  }
}

/**
 * Converts data from 2complemented representation to signed integer
 */
int16_t complement2signed(uint8_t msb, uint8_t lsb){
  uint16_t word = 0;
  word = (msb << 8) + lsb;
  if (msb > 0x7F){
    return -1 * ((int16_t)((~word) + 1));
  }
  return (int16_t)word;
}

/* I2C interface #2 */
static const I2CConfig i2cfg2 = {
    OPMODE_I2C,
    400000,
    FAST_DUTY_CYCLE_2,
};

/*
 * Application entry point.
 */
int main(void) {
  msg_t status = RDY_OK;
  systime_t tmo = MS2ST(4);

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
   * Starts I2C
   */
  i2cStart(&I2CD2, &i2cfg2);

  /*
   * Prepares the Serial driver 2
   */
  sdStart(&SD2, NULL);          /* Default is 38400-8-N-1.*/
  palSetPadMode(GPIOA, 2, PAL_MODE_ALTERNATE(7));
  palSetPadMode(GPIOA, 3, PAL_MODE_ALTERNATE(7));

  /**
   * Prepares the accelerometer
   */
  txbuf[0] = ACCEL_CTRL_REG1; /* register address */
  txbuf[1] = 0x1;
  i2cAcquireBus(&I2CD2);
  status = i2cMasterTransmitTimeout(&I2CD2, mma8451_addr, txbuf, 2, rxbuf, 0, tmo);
  i2cReleaseBus(&I2CD2);

  if (status != RDY_OK){
    errors = i2cGetErrors(&I2CD2);
  }

  /*
   * Normal main() thread activity, nothing in this test.
   */
  while (TRUE) {
    palTogglePad(GPIOB, GPIOB_LED_B);
    chThdSleepMilliseconds(100);

    txbuf[0] = ACCEL_OUT_DATA; /* register address */
    i2cAcquireBus(&I2CD2);
    status = i2cMasterTransmitTimeout(&I2CD2, mma8451_addr, txbuf, 1, rxbuf, 6, tmo);
    i2cReleaseBus(&I2CD2);

    if (status != RDY_OK){
      errors = i2cGetErrors(&I2CD2);
    }

    acceleration_x = complement2signed(rxbuf[0], rxbuf[1]);
    acceleration_y = complement2signed(rxbuf[2], rxbuf[3]);
    acceleration_z = complement2signed(rxbuf[4], rxbuf[5]);

    print("x: ");
    printn(acceleration_x);
    print(" y: ");
    printn(acceleration_y);
    print(" z: ");
    printn(acceleration_z);
    println("");
  }
}



