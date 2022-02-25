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
 * @file    lis302dl.h
 * @brief   LIS302DL MEMS interface module through SPI header.
 *
 * @addtogroup lis302dl
 * @{
 */

#ifndef _LIS302DL_H_
#define _LIS302DL_H_

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @name    LIS302DL register names
 * @{
 */
#define LIS302DL_WHO_AM_I               0x0F
#define LIS302DL_CTRL_REG1              0x20
#define LIS302DL_CTRL_REG2              0x21
#define LIS302DL_CTRL_REG3              0x22
#define LIS302DL_HP_FILTER_RESET        0x23
#define LIS302DL_STATUS_REG             0x27
#define LIS302DL_OUTX                   0x29
#define LIS302DL_OUTY                   0x2B
#define LIS302DL_OUTZ                   0x2D
#define LIS302DL_FF_WU_CFG1             0x30
#define LIS302DL_FF_WU_SRC1             0x31
#define LIS302DL_FF_WU_THS1             0x32
#define LIS302DL_FF_WU_DURATION1        0x33
#define LIS302DL_FF_WU_CFG2             0x34
#define LIS302DL_FF_WU_SRC2             0x35
#define LIS302DL_FF_WU_THS2             0x36
#define LIS302DL_FF_WU_DURATION2        0x37
#define LIS302DL_CLICK_CFG              0x38
#define LIS302DL_CLICK_SRC              0x39
#define LIS302DL_CLICK_THSY_X           0x3B
#define LIS302DL_CLICK_THSZ             0x3C
#define LIS302DL_CLICK_TIMELIMIT        0x3D
#define LIS302DL_CLICK_LATENCY          0x3E
#define LIS302DL_CLICK_WINDOW           0x3F
/** @} */

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  uint8_t lis302dlReadRegister(SPIDriver *spip, uint8_t reg);
  void lis302dlWriteRegister(SPIDriver *spip, uint8_t reg, uint8_t value);
#ifdef __cplusplus
}
#endif

#endif /* _LIS302DL_H_ */

/** @} */
