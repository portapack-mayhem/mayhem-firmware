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

#include "ch.hpp"
#include "hal.h"
#include "test.h"

using namespace chibios_rt;

/*
 * LED blink sequences.
 * NOTE: Sequences must always be terminated by a GOTO instruction.
 * NOTE: The sequencer language could be easily improved but this is outside
 *       the scope of this demo.
 */
#define SLEEP           0
#define GOTO            1
#define STOP            2
#define BITCLEAR        3
#define BITSET          4

typedef struct {
  uint8_t       action;
  uint32_t      value;
} seqop_t;

// Flashing sequence for LED3.
static const seqop_t LED3_sequence[] =
{
  {BITSET, PAL_PORT_BIT(GPIOD_LED3)},
  {SLEEP,    800},
  {BITCLEAR,   PAL_PORT_BIT(GPIOD_LED3)},
  {SLEEP,    200},
  {GOTO,     0}
};

// Flashing sequence for LED4.
static const seqop_t LED4_sequence[] =
{
  {BITSET, PAL_PORT_BIT(GPIOD_LED4)},
  {SLEEP,    600},
  {BITCLEAR,   PAL_PORT_BIT(GPIOD_LED4)},
  {SLEEP,    400},
  {GOTO,     0}
};

// Flashing sequence for LED5.
static const seqop_t LED5_sequence[] =
{
  {BITSET, PAL_PORT_BIT(GPIOD_LED5)},
  {SLEEP,    400},
  {BITCLEAR,   PAL_PORT_BIT(GPIOD_LED5)},
  {SLEEP,    600},
  {GOTO,     0}
};

// Flashing sequence for LED6.
static const seqop_t LED6_sequence[] =
{
  {BITSET, PAL_PORT_BIT(GPIOD_LED6)},
  {SLEEP,    200},
  {BITCLEAR,   PAL_PORT_BIT(GPIOD_LED6)},
  {SLEEP,    800},
  {GOTO,     0}
};

/*
 * Sequencer thread class. It can drive LEDs or other output pins.
 * Any sequencer is just an instance of this class, all the details are
 * totally encapsulated and hidden to the application level.
 */
class SequencerThread : public BaseStaticThread<128> {
private:
  const seqop_t *base, *curr;                   // Thread local variables.

protected:
  virtual msg_t main(void) {

    setName("sequencer");

    while (true) {
      switch(curr->action) {
      case SLEEP:
        sleep(curr->value);
        break;
      case GOTO:
        curr = &base[curr->value];
        continue;
      case STOP:
        return 0;
      case BITCLEAR:
        palClearPort(GPIOD, curr->value);
        break;
      case BITSET:
        palSetPort(GPIOD, curr->value);
        break;
      }
      curr++;
    }
  }

public:
  SequencerThread(const seqop_t *sequence) : BaseStaticThread<128>() {

    base = curr = sequence;
  }
};

/*
 * Tester thread class. This thread executes the test suite.
 */
class TesterThread : public BaseStaticThread<256> {

protected:
  virtual msg_t main(void) {

    setName("tester");

    return TestThread(&SD2);
  }

public:
  TesterThread(void) : BaseStaticThread<256>() {
  }
};

/* Static threads instances.*/
static TesterThread tester;
static SequencerThread blinker1(LED3_sequence);
static SequencerThread blinker2(LED4_sequence);
static SequencerThread blinker3(LED5_sequence);
static SequencerThread blinker4(LED6_sequence);

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
  System::init();

  /*
   * Activates the serial driver 2 using the driver default configuration.
   * PA2(TX) and PA3(RX) are routed to USART2.
   */
  sdStart(&SD2, NULL);
  palSetPadMode(GPIOA, 2, PAL_MODE_ALTERNATE(7));
  palSetPadMode(GPIOA, 3, PAL_MODE_ALTERNATE(7));

  /*
   * Starts several instances of the SequencerThread class, each one operating
   * on a different LED.
   */
  blinker1.start(NORMALPRIO + 10);
  blinker2.start(NORMALPRIO + 10);
  blinker3.start(NORMALPRIO + 10);
  blinker4.start(NORMALPRIO + 10);

  /*
   * Serves timer events.
   */
  while (true) {
    if (palReadPad(GPIOA, GPIOA_BUTTON)) {
      tester.start(NORMALPRIO);
      tester.wait();
    };
    BaseThread::sleep(MS2ST(500));
  }

  return 0;
}
