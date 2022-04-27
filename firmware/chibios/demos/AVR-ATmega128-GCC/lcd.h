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

#ifndef _LCD_H_
#define _LCD_H_

#define ELOOPVALUE              10

#define LCD_CLEAR               0x01

#define LCD_RETURN_HOME         0x02

#define LCD_SET_INCREMENT_MODE  0x06

#define LCD_SET_DM              0x08
#define LCD_DM_DISPLAY_ON       4
#define LCD_DM_DISPLAY_OFF      0
#define LCD_DM_CURSOR_ON        2
#define LCD_DM_CURSOR_OFF       0
#define LCD_DM_BLINK_ON         1
#define LCD_DM_BLINK_OFF        0

#define LCD_CMD_INIT4           0x28
#define LCD_CMD_INIT8           0x38

#define LCD_SET_DDRAM_ADDRESS   0x80

#define LCD_LINE1               0
#define LCD_LINE2               40

void lcdInit(void);
void lcdCmd(uint8_t cmd);
void lcdPutc(char c);
void lcdPuts(uint8_t pos, char *p);

#endif /* _LCD_H_ */
