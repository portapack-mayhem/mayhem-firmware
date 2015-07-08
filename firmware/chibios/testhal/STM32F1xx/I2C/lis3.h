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

#include <stdlib.h>
#include "ch.h"

#ifndef LIS3_H_
#define LIS3_H_



/* buffers depth */
#define ACCEL_RX_DEPTH 8
#define ACCEL_TX_DEPTH 8

/* autoincrement bit position. This bit needs to perform reading of
 * multiple bytes at one request */
#define AUTO_INCREMENT_BIT (1<<7)

/* slave specific addresses */
#define ACCEL_STATUS_REG  0x27
#define ACCEL_CTRL_REG1   0x20
#define ACCEL_OUT_DATA    0x28



inline int init_lis3(void);
inline void request_acceleration_data(void);


#endif /* LIS3_H_ */
