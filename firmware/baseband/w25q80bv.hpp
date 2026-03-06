/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2023 Bernd Herzog
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifndef __W25Q80BV_H__
#define __W25Q80BV_H__

#include "ch.h"
#include "hal.h"

#define RGU_BASE 0x40053000
#define RESET_CTRL1 MMIO32(RGU_BASE + 0x104)
#define RESET_CTRL1_SPIFI_RST_SHIFT (21)
#define RESET_CTRL1_SPIFI_RST (1 << RESET_CTRL1_SPIFI_RST_SHIFT)

#define W25Q80BV_WRITE_ENABLE 0x06
#define W25Q80BV_CHIP_ERASE 0xC7
#define W25Q80BV_DEVICE_ID 0xAB
#define W25Q80BV_PAGE_PROGRAM 0x02

#define PERIPH_BASE_APB0 0x40080000
#define SCU_BASE (PERIPH_BASE_APB0 + 0x06000)
#define MMIO32(addr) (*(volatile uint32_t*)(addr))

#define PIN_GROUP3 (SCU_BASE + 0x180)
#define PIN3 0x00C
#define PIN4 0x010
#define PIN5 0x014
#define PIN6 0x018
#define PIN7 0x01C
#define PIN8 0x020

#define BIT0 (1 << 0)
#define BIT1 (1 << 1)
#define BIT2 (1 << 2)
#define BIT3 (1 << 3)
#define BIT4 (1 << 4)
#define BIT5 (1 << 5)
#define BIT6 (1 << 6)
#define BIT7 (1 << 7)
#define BIT8 (1 << 8)

#define SCU_CONF_EPUN_DIS_PULLUP (BIT4)
#define SCU_CONF_EHS_FAST (BIT5)
#define SCU_CONF_EZI_EN_IN_BUFFER (BIT6)
#define SCU_CONF_ZIF_DIS_IN_GLITCH_FILT (BIT7)

#define SCU_GPIO_FAST (SCU_CONF_EPUN_DIS_PULLUP |  \
                       SCU_CONF_EHS_FAST |         \
                       SCU_CONF_EZI_EN_IN_BUFFER | \
                       SCU_CONF_ZIF_DIS_IN_GLITCH_FILT)
#define SCU_SSP_IO SCU_GPIO_FAST

#define SCU_CONF_FUNCTION0 (0x0)
#define SCU_CONF_FUNCTION2 (0x2)
#define SCU_CONF_FUNCTION4 (0x4)
#define SCU_CONF_FUNCTION5 (0x5)

#define SCU_SSP0_CIPO (PIN_GROUP3 + PIN6)
#define SCU_SSP0_COPI (PIN_GROUP3 + PIN7)
#define SCU_SSP0_SCK (PIN_GROUP3 + PIN3)
#define SCU_SSP0_CS (PIN_GROUP3 + PIN8)
#define SCU_FLASH_HOLD (PIN_GROUP3 + PIN4)
#define SCU_FLASH_WP (PIN_GROUP3 + PIN5)

#define W25Q80BV_DEVICE_ID_RES 0x13 /* Expected device_id for W25Q80BV */
#define W25Q16DV_DEVICE_ID_RES 0x14 /* Expected device_id for W25Q16DV */

#define SSP_CR1(port) MMIO32(port + 0x004)
#define PERIPH_BASE_APB0 0x40080000
#define SSP0_BASE (PERIPH_BASE_APB0 + 0x03000)
#define SSP_CPSR(port) MMIO32(port + 0x010)
#define SSP_CR0(port) MMIO32(port + 0x000)
#define SSP_ENABLE BIT1
#define SSP_DATA_8BITS 0x7

#define palSetPad(port, pad) (LPC_GPIO->SET[(port)] = 1 << (pad))
#define palClearPad(port, pad) (LPC_GPIO->CLR[(port)] = 1 << (pad))
#define palOutputPad(port, pad) (LPC_GPIO->DIR[(port)] |= 1 << (pad))
#define palTogglePad(port, pad) (LPC_GPIO->NOT[(port)] |= 1 << (pad))

#define W25Q80BV_SELECT_PORT 5
#define W25Q80BV_SELECT_PAD 11
#define W25Q80BV_WP_PORT 1
#define W25Q80BV_WP_PAD 15
#define W25Q80BV_HOLD_PORT 1
#define W25Q80BV_HOLD_PAD 14

#define LED_PORT 2
#define LEDRX_PAD 2
#define LEDTX_PAD 8

#define W25Q80BV_STATUS_BUSY 0x01
#define W25Q80BV_STATUS_WEL 0x02
#define W25Q80BV_READ_STATUS1 0x05
#define W25Q80BV_READ_STATUS2 0x35

#define PERIPH_BASE_APB0 0x40080000
#define SSP0_BASE (PERIPH_BASE_APB0 + 0x03000)
#define SSP_DR(port) MMIO32(port + 0x008)
#define SSP_SR(port) MMIO32(port + 0x00C)

#define SSP_SR_TNF BIT1
#define SSP_SR_RNE BIT2
#define SSP_SR_BSY BIT4

namespace w25q80bv {
void disable_spifi();
uint8_t get_status();
void wait_for_device();
void wait_not_busy();
uint32_t spi_ssp_transfer_word(const uint32_t data);
void initialite_spi();
void setup();
void remove_write_protection();
uint8_t get_device_id();
void erase_chip();
void write(size_t page_index, uint8_t* data_buffer, size_t length);
}  // namespace w25q80bv

#endif /*__W25Q80BV_H__*/
