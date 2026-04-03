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

#ifndef _AT91SAM7_H_
#define _AT91SAM7_H_

/*
 * Supported platforms.
 */
#define SAM7S64         0
#define SAM7S128        1
#define SAM7S256        2
#define SAM7S512        3
#define SAM7X128        4
#define SAM7X256        5
#define SAM7X512        6
#define SAM7A3          7

#ifndef SAM7_PLATFORM
#error "SAM7 platform not defined"
#endif

#if SAM7_PLATFORM == SAM7S64
#include "at91lib/AT91SAM7S64.h"
#elif SAM7_PLATFORM == SAM7S128
#include "at91lib/AT91SAM7S128.h"
#elif SAM7_PLATFORM == SAM7S256
#include "at91lib/AT91SAM7S256.h"
#elif SAM7_PLATFORM == SAM7S512
#include "at91lib/AT91SAM7S512.h"
#elif SAM7_PLATFORM == SAM7X128
#include "at91lib/AT91SAM7X128.h"
#elif SAM7_PLATFORM == SAM7X256
#include "at91lib/AT91SAM7X256.h"
#elif SAM7_PLATFORM == SAM7X512
#include "at91lib/AT91SAM7X512.h"
#elif SAM7_PLATFORM == SAM7A3
#include "at91lib/AT91SAM7A3.h"
#else
#error "SAM7 platform not supported"
#endif

#endif /* _AT91SAM7_H_ */
