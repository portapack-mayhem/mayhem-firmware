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

#define ADC_GRP1_NUM_CHANNELS   4
#define ADC_GRP1_BUF_DEPTH      8

#define ADC_GRP2_NUM_CHANNELS   16
#define ADC_GRP2_BUF_DEPTH      16

static adcsample_t samples1[ADC_GRP1_NUM_CHANNELS * ADC_GRP1_BUF_DEPTH];
static adcsample_t samples2[ADC_GRP2_NUM_CHANNELS * ADC_GRP2_BUF_DEPTH];

/*
 * ADC streaming callback.
 */
size_t nx = 0, ny = 0;
static void adccallback(ADCDriver *adcp, adcsample_t *buffer, size_t n) {

  (void)adcp;
  if (samples2 == buffer) {
    nx += n;
  }
  else {
    ny += n;
  }
}

static void adcerrorcallback(ADCDriver *adcp, adcerror_t err) {

  (void)adcp;
  (void)err;
}

/*
 * ADC conversion group.
 * Mode:        Linear buffer, 8 samples of 2 channels, SW triggered.
 * Channels:    IN7, IN8.
 */
static const ADCConversionGroup adcgrpcfg1 = {
  FALSE,
  ADC_GRP1_NUM_CHANNELS,
  NULL,
  adcerrorcallback,
  ADC_CFGR_CONT,                                                /* CFGR     */
  ADC_TR(0, 4095),                                              /* TR1      */
  ADC_CCR_DUAL(1),                                              /* CCR      */
  {                                                             /* SMPR[2]  */
    0,
    0
  },
  {                                                             /* SQR[4]   */
    ADC_SQR1_SQ1_N(ADC_CHANNEL_IN7) | ADC_SQR1_SQ2_N(ADC_CHANNEL_IN8),
    0,
    0,
    0
  },
  {                                                             /* SSMPR[2] */
    0,
    0
  },
  {                                                             /* SSQR[4]  */
    ADC_SQR1_SQ1_N(ADC_CHANNEL_IN8) | ADC_SQR1_SQ2_N(ADC_CHANNEL_IN7),
    0,
    0,
    0
  }
};

/*
 * ADC conversion group.
 * Mode:        Continuous, 16 samples of 8 channels, SW triggered.
 * Channels:    IN7, IN8, IN7, IN8, IN7, IN8, Sensor, VBat/2.
 */
static const ADCConversionGroup adcgrpcfg2 = {
  TRUE,
  ADC_GRP2_NUM_CHANNELS,
  adccallback,
  adcerrorcallback,
  ADC_CFGR_CONT,                                                /* CFGR     */
  ADC_TR(0, 4095),                                              /* TR1      */
  ADC_CCR_DUAL(1) | ADC_CCR_TSEN | ADC_CCR_VBATEN,              /* CCR      */
  {                                                             /* SMPR[2]  */
    ADC_SMPR1_SMP_AN7(ADC_SMPR_SMP_19P5)
    | ADC_SMPR1_SMP_AN8(ADC_SMPR_SMP_19P5),
    ADC_SMPR2_SMP_AN16(ADC_SMPR_SMP_61P5)
    | ADC_SMPR2_SMP_AN17(ADC_SMPR_SMP_61P5),
  },
  {                                                             /* SQR[4]   */
    ADC_SQR1_SQ1_N(ADC_CHANNEL_IN7)  | ADC_SQR1_SQ2_N(ADC_CHANNEL_IN8) |
    ADC_SQR1_SQ3_N(ADC_CHANNEL_IN7)  | ADC_SQR1_SQ4_N(ADC_CHANNEL_IN8),
    ADC_SQR2_SQ5_N(ADC_CHANNEL_IN7)  | ADC_SQR2_SQ6_N(ADC_CHANNEL_IN8) |
    ADC_SQR2_SQ7_N(ADC_CHANNEL_IN16) | ADC_SQR2_SQ8_N(ADC_CHANNEL_IN17),
    0,
    0
  },
  {                                                             /* SSMPR[2] */
    ADC_SMPR1_SMP_AN7(ADC_SMPR_SMP_19P5)
    | ADC_SMPR1_SMP_AN8(ADC_SMPR_SMP_19P5),
    ADC_SMPR2_SMP_AN16(ADC_SMPR_SMP_61P5)
    | ADC_SMPR2_SMP_AN17(ADC_SMPR_SMP_61P5),
  },
  {                                                             /* SSQR[4]  */
    ADC_SQR1_SQ1_N(ADC_CHANNEL_IN8)  | ADC_SQR1_SQ2_N(ADC_CHANNEL_IN7) |
    ADC_SQR1_SQ3_N(ADC_CHANNEL_IN8)  | ADC_SQR1_SQ4_N(ADC_CHANNEL_IN7),
    ADC_SQR2_SQ5_N(ADC_CHANNEL_IN8)  | ADC_SQR2_SQ6_N(ADC_CHANNEL_IN7) |
    ADC_SQR2_SQ7_N(ADC_CHANNEL_IN17) | ADC_SQR2_SQ8_N(ADC_CHANNEL_IN16),
    0,
    0
  }
};

/*
 * Red LEDs blinker thread, times are in milliseconds.
 */
static WORKING_AREA(waThread1, 128);
static msg_t Thread1(void *arg) {

  (void)arg;
  chRegSetThreadName("blinker");
  while (TRUE) {
    palSetPad(GPIOE, GPIOE_LED10_RED);
    chThdSleepMilliseconds(500);
    palClearPad(GPIOE, GPIOE_LED10_RED);
    chThdSleepMilliseconds(500);
  }
  return 0;
}

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
   * Setting up analog inputs used by the demo.
   */
  palSetGroupMode(GPIOC, PAL_PORT_BIT(1) | PAL_PORT_BIT(2),
                  0, PAL_MODE_INPUT_ANALOG);

  /*
   * Creates the blinker thread.
   */
  chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO, Thread1, NULL);

  /*
   * Activates the ADC1 driver and the temperature sensor.
   */
  adcStart(&ADCD1, NULL);

  /*
   * Linear conversion.
   */
  adcConvert(&ADCD1, &adcgrpcfg1, samples1, ADC_GRP1_BUF_DEPTH);
  chThdSleepMilliseconds(1000);

  /*
   * Starts an ADC continuous conversion.
   */
  adcStartConversion(&ADCD1, &adcgrpcfg2, samples2, ADC_GRP2_BUF_DEPTH);

  /*
   * Normal main() thread activity, in this demo it does nothing.
   */
  while (TRUE) {
    if (palReadPad(GPIOA, GPIOA_BUTTON)) {
      adcStopConversion(&ADCD1);
    }
    chThdSleepMilliseconds(500);
  }
}
