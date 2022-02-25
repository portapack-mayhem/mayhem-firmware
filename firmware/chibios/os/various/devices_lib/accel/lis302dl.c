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
 * @file    lis302dl.c
 * @brief   LIS302DL MEMS interface module through SPI code.
 *
 * @addtogroup lis302dl
 * @{
 */

#include "ch.h"
#include "hal.h"
#include "lis302dl.h"

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

static uint8_t txbuf[2];
static uint8_t rxbuf[2];

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Reads a register value.
 * @pre     The SPI interface must be initialized and the driver started.
 *
 * @param[in] spip      pointer to the SPI initerface
 * @param[in] reg       register number
 * @return              The register value.
 */
uint8_t lis302dlReadRegister(SPIDriver *spip, uint8_t reg) {

  spiSelect(spip);
  txbuf[0] = 0x80 | reg;
  txbuf[1] = 0xff;
  spiExchange(spip, 2, txbuf, rxbuf);
  spiUnselect(spip);
  return rxbuf[1];
}

/**
 * @brief   Writes a value into a register.
 * @pre     The SPI interface must be initialized and the driver started.
 *
 * @param[in] spip      pointer to the SPI initerface
 * @param[in] reg       register number
 * @param[in] value     the value to be written
 */
void lis302dlWriteRegister(SPIDriver *spip, uint8_t reg, uint8_t value) {

  switch (reg) {
  default:
    /* Reserved register must not be written, according to the datasheet
       this could permanently damage the device.*/
    chDbgAssert(FALSE, "lis302dlWriteRegister(), #1", "reserved register");
  case LIS302DL_WHO_AM_I:
  case LIS302DL_HP_FILTER_RESET:
  case LIS302DL_STATUS_REG:
  case LIS302DL_OUTX:
  case LIS302DL_OUTY:
  case LIS302DL_OUTZ:
  case LIS302DL_FF_WU_SRC1:
  case LIS302DL_FF_WU_SRC2:
  case LIS302DL_CLICK_SRC:
    /* Read only registers cannot be written, the command is ignored.*/
    return;
  case LIS302DL_CTRL_REG1:
  case LIS302DL_CTRL_REG2:
  case LIS302DL_CTRL_REG3:
  case LIS302DL_FF_WU_CFG1:
  case LIS302DL_FF_WU_THS1:
  case LIS302DL_FF_WU_DURATION1:
  case LIS302DL_FF_WU_CFG2:
  case LIS302DL_FF_WU_THS2:
  case LIS302DL_FF_WU_DURATION2:
  case LIS302DL_CLICK_CFG:
  case LIS302DL_CLICK_THSY_X:
  case LIS302DL_CLICK_THSZ:
  case LIS302DL_CLICK_TIMELIMIT:
  case LIS302DL_CLICK_LATENCY:
  case LIS302DL_CLICK_WINDOW:
    spiSelect(spip);
    txbuf[0] = reg;
    txbuf[1] = value;
    spiSend(spip, 2, txbuf);
    spiUnselect(spip);
  }
}

/** @} */
