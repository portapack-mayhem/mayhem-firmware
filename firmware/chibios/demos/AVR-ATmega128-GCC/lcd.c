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
#include "lcd.h"

static void e_pulse(void) {
  volatile uint8_t i;

  PORTC |= PORTC_44780_E_MASK;
  for (i = 0; i < ELOOPVALUE; i++);
    ;
  PORTC &= ~PORTC_44780_E_MASK;
}

static void wait_not_busy(void) {

  chThdSleep(2);
}

/*
 * 44780 soft reset procedure.
 */
void lcdInit(void) {

  PORTC = (PORTC & ~(PORTC_44780_DATA_MASK | PORTC_44780_RS_MASK |
                     PORTC_44780_E_MASK | PORTC_44780_RW_MASK)) |
          (LCD_CMD_INIT8 & PORTC_44780_DATA_MASK);
  chThdSleep(50);
  e_pulse();
  chThdSleep(10);
  e_pulse();
  chThdSleep(2);
  e_pulse();
  wait_not_busy();
  PORTC = (PORTC & ~(PORTC_44780_DATA_MASK | PORTC_44780_RS_MASK |
                     PORTC_44780_E_MASK | PORTC_44780_RW_MASK)) |
          (LCD_CMD_INIT4 & PORTC_44780_DATA_MASK);
  e_pulse();
  lcdCmd(LCD_CMD_INIT4);
  lcdCmd(LCD_SET_DM | LCD_DM_DISPLAY_ON);
  lcdCmd(LCD_SET_INCREMENT_MODE);
}

/*
 * Sends a command byte to the 44780.
 */
void lcdCmd(uint8_t cmd) {

  wait_not_busy();
  PORTC = (PORTC | PORTC_44780_DATA_MASK) & (cmd |
                                             (0x0F & ~PORTC_44780_RS_MASK));
  e_pulse();
  PORTC = (PORTC | PORTC_44780_DATA_MASK) & ((cmd << 4) |
                                             (0x0F & ~PORTC_44780_RS_MASK));
  e_pulse();
}

/*
 * Writes a char on the LCD at the current position.
 */
void lcdPutc(char c) {
  uint8_t b;

  wait_not_busy();
  b = c | 0x0F;
  PORTC = (PORTC | PORTC_44780_DATA_MASK | PORTC_44780_RS_MASK) &
          (c | 0x0F);
  e_pulse();
  PORTC = (PORTC | PORTC_44780_DATA_MASK | PORTC_44780_RS_MASK) &
          ((c << 4) | 0x0F);
  e_pulse();
}

/*
 * Writes a string on the LCD at an absolute address.
 */
void lcdPuts(uint8_t pos, char *p) {

  lcdCmd(LCD_SET_DDRAM_ADDRESS | pos);
  while (*p)
    lcdPutc(*p++);
}
