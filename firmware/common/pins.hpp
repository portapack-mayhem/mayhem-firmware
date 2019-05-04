/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __PINS_H__
#define __PINS_H__

#include "gpio.hpp"

namespace lpc43xx {

enum Pins {
	P0_0, P0_1,
	P1_0, P1_1, P1_2, P1_3, P1_4, P1_5, P1_6, P1_7, P1_8, P1_9, P1_10, P1_11, P1_12, P1_13, P1_14, P1_15, P1_16, P1_17, P1_18, P1_19, P1_20,
	P2_0, P2_1, P2_2, P2_3, P2_4, P2_5, P2_6, P2_7, P2_8, P2_9, P2_10, P2_11, P2_12, P2_13,
	P3_0, P3_1, P3_2,
	P4_0, P4_1, P4_2, P4_3, P4_4, P4_5, P4_6, P4_7, P4_8, P4_9, P4_10,
	P5_0, P5_1, P5_2, P5_3, P5_4, P5_5, P5_6, P5_7,
	P6_0, P6_1, P6_2, P6_3, P6_4, P6_5, P6_6, P6_7, P6_8, P6_9, P6_10, P6_11, P6_12,
	P7_0, P7_1, P7_2, P7_3, P7_4, P7_5, P7_6, P7_7,
	P9_5, P9_6,
	PF_4,
	CLK0, CLK2,
};

constexpr Pin pins[] = {
	[P0_0]  = {  0,  0, PinConfig::sgpio_inout_fast(3) }, /* SGPIO0/P75/BANK2F3M3: CPLD.89/HOST_DATA0(IO) */
	[P0_1]  = {  0,  1, PinConfig::sgpio_inout_fast(3) }, /* SGPIO1/BANK2F3M5: CPLD.79/HOST_DATA1(IO) */
	[P1_0]  = {  1,  0, PinConfig::sgpio_inout_fast(6) }, /* SGPIO7/P76/BANK2F3M7: CPLD.77/HOST_DATA7(IO) */
	[P1_1]  = {  1,  1, { .mode=0, .pd=0, .pu=0, .fast=0, .input=0, .ifilt=1 } }, /* P1_1/P74: 10K PU, BOOT0 */
	[P1_2]  = {  1,  2, { .mode=0, .pd=0, .pu=0, .fast=0, .input=0, .ifilt=1 } }, /* P1_2/P73: 10K PD, BOOT1 */
	[P1_3]  = {  1,  3, { .mode=5, .pd=0, .pu=0, .fast=0, .input=1, .ifilt=1 } }, /* SSP1_MISO/P41: MAX2837.DOUT(O) */
	[P1_4]  = {  1,  4, { .mode=5, .pd=1, .pu=0, .fast=0, .input=0, .ifilt=1 } }, /* SSP1_MOSI/P40: MAX2837.DIN(I), MAX5864.DIN(I) */
	[P1_5]  = {  1,  5, { .mode=0, .pd=0, .pu=1, .fast=0, .input=1, .ifilt=1 } }, /* SD_POW: PortaPack CPLD.TDO(O) */
	[P1_6]  = {  1,  6, { .mode=7, .pd=0, .pu=1, .fast=0, .input=1, .ifilt=0 } }, /* SD_CMD: PortaPack SD.CMD(IO)  */
	[P1_7]  = {  1,  7, { .mode=0, .pd=0, .pu=0, .fast=0, .input=0, .ifilt=1 } }, /* !MIX_BYPASS/P35: U1.VCTL1(I), U11.VCTL2(I), U9.V2(I) */
	[P1_8]  = {  1,  8, { .mode=0, .pd=0, .pu=1, .fast=0, .input=0, .ifilt=1 } }, /* SD_VOLT0: PortaPack CPLD.TMS(I) */
	[P1_9]  = {  1,  9, { .mode=7, .pd=0, .pu=1, .fast=0, .input=1, .ifilt=0 } }, /* SD_DAT0: PortaPack SD.DAT0(IO) */
	[P1_10] = {  1, 10, { .mode=7, .pd=0, .pu=1, .fast=0, .input=1, .ifilt=0 } }, /* SD_DAT1: PortaPack SD.DAT1(IO) */
	[P1_11] = {  1, 11, { .mode=7, .pd=0, .pu=1, .fast=0, .input=1, .ifilt=0 } }, /* SD_DAT2: PortaPack SD.DAT2(IO) */
	[P1_12] = {  1, 12, { .mode=7, .pd=0, .pu=1, .fast=0, .input=1, .ifilt=0 } }, /* SD_DAT3: PortaPack SD.DAT3(IO) */
	[P1_13] = {  1, 13, { .mode=7, .pd=0, .pu=1, .fast=0, .input=1, .ifilt=1 } }, /* SD_CD: PortaPack SD.CD(O) */
	[P1_14] = {  1, 14, PinConfig::sgpio_out_fast_with_pullup(6) }, /* SGPIO10/P78/BANK2F3M8: CPLD.76/HOST_DISABLE(I) */
	[P1_15] = {  1, 15, PinConfig::sgpio_inout_fast(2) }, /* SGPIO2/BANK2F3M9: CPLD.74/HOST_DATA2(IO) */
	[P1_16] = {  1, 16, PinConfig::sgpio_inout_fast(2) }, /* SGPIO3/BANK2F3M10: CPLD.72/HOST_DATA3(IO) */
	[P1_17] = {  1, 17, PinConfig::sgpio_out_fast_with_pullup(6) }, /* SGPIO11/P79/BANK2F3M11: CPLD.71/HOST_DIRECTION(I) */
	[P1_18] = {  1, 18, PinConfig::gpio_out_with_pulldown(0) }, /* SGPIO12/BANK2F3M12: CPLD.70/HOST_INVERT(I) */
	[P1_19] = {  1, 19, { .mode=1, .pd=0, .pu=0, .fast=0, .input=0, .ifilt=1 } }, /* SSP1_SCK/P39: MAX2837.SCLK(I), MAX5864.SCLK(I) */
	[P1_20] = {  1, 20, { .mode=0, .pd=0, .pu=0, .fast=0, .input=0, .ifilt=1 } }, /* CS_XCVR/P53: MAX2837.CS(I) */
	[P2_0]  = {  2,  0, { .mode=4, .pd=0, .pu=0, .fast=0, .input=1, .ifilt=1 } }, /* U0_TXD: PortaPack P2_0/IO_STBX */
	[P2_1]  = {  2,  1, { .mode=4, .pd=0, .pu=0, .fast=0, .input=1, .ifilt=1 } }, /* U0_RXD: PortaPack P2_1/ADDR */
	[P2_2]  = {  2,  2, PinConfig::sgpio_inout_fast(0) }, /* SGPIO6/BANK2F3M16: CPLD.61/HOST_DATA6(IO) */
	[P2_3]  = {  2,  3, { .mode=4, .pd=0, .pu=0, .fast=0, .input=1, .ifilt=1 } }, /* I2C1_SDA: PortaPack P2_3/LCD_TE */
	[P2_4]  = {  2,  4, { .mode=4, .pd=0, .pu=0, .fast=0, .input=1, .ifilt=1 } }, /* I2C1_SCL: PortaPack P2_4/LCD_RDX */
	[P2_5]  = {  2,  5, { .mode=4, .pd=0, .pu=1, .fast=0, .input=0, .ifilt=1 } }, /* RX/P43: U7.VCTL1(I), U10.VCTL1(I), U2.VCTL1(I) */
	[P2_6]  = {  2,  6, { .mode=4, .pd=0, .pu=0, .fast=0, .input=0, .ifilt=1 } }, /* MIXER_SCLK/P31: 33pF, RFFC5072.SCLK(I) */
	[P2_7]  = {  2,  7, { .mode=0, .pd=0, .pu=0, .fast=0, .input=0, .ifilt=1 } }, /* ISP: 10K PU, Unused */
	[P2_8]  = {  2,  8, { .mode=4, .pd=0, .pu=0, .fast=0, .input=0, .ifilt=1 } }, /* P2_8: 10K PD, BOOT2, DFU switch, PortaPack P2_8/<unused> */
	[P2_9]  = {  2,  9, { .mode=0, .pd=0, .pu=0, .fast=0, .input=1, .ifilt=1 } }, /* P2_9: 10K PD, BOOT3, PortaPack P2_9/LCD_WRX */
	[P2_10] = {  2, 10, { .mode=0, .pd=0, .pu=0, .fast=0, .input=0, .ifilt=1 } }, /* AMP_BYPASS/P50: U14.V2(I), U12.V2(I) */
	[P2_11] = {  2, 11, { .mode=0, .pd=0, .pu=0, .fast=0, .input=0, .ifilt=1 } }, /* RX_AMP/P49: U12.V1(I), U14.V3(I) */
	[P2_12] = {  2, 12, { .mode=0, .pd=0, .pu=0, .fast=0, .input=0, .ifilt=1 } }, /* !RX_AMP_PWR/P52: 10K PU, Q1.G(I), power to U13 (RX amp) */
	[P2_13] = {  2, 13, { .mode=0, .pd=0, .pu=0, .fast=0, .input=1, .ifilt=1 } }, /* P2_13: PortaPack P2_13/DIR */
	[P3_0]  = {  3,  0, { .mode=2, .pd=0, .pu=1, .fast=0, .input=1, .ifilt=1 } }, /* I2S0_TX_SCK: PortaPack I2S0_TX_SCK(I) */
	[P3_1]  = {  3,  1, { .mode=0, .pd=0, .pu=1, .fast=0, .input=1, .ifilt=1 } }, /* I2S0_RX_WS: PortaPack I2S0_TX_WS(I). Input enabled to fold back into RX. */
	[P3_2]  = {  3,  2, { .mode=0, .pd=0, .pu=1, .fast=0, .input=0, .ifilt=1 } }, /* I2S0_RX_SDA: PortaPack I2S0_TX_SDA(I) */
	[P4_0]  = {  4,  0, { .mode=0, .pd=0, .pu=0, .fast=0, .input=0, .ifilt=1 } }, /* HP/P44: U6.VCTL1(I), U5.VCTL2(I) */
	[P4_1]  = {  4,  1, PinConfig::gpio_led(0) }, /* LED1: LED1.A(I) */
	[P4_2]  = {  4,  2, PinConfig::gpio_led(0) }, /* LED2: LED2.A(I) */
	[P4_3]  = {  4,  3, PinConfig::sgpio_in_fast(7) }, /* SGPIO9/P77/BANK2F3M1: CPLD.91/HOST_CAPTURE(O) */
	[P4_4]  = {  4,  4, { .mode=0, .pd=0, .pu=0, .fast=0, .input=0, .ifilt=1 } }, /* TXENABLE/P55: MAX2837.TXENABLE(I) */
	[P4_5]  = {  4,  5, { .mode=0, .pd=0, .pu=0, .fast=0, .input=0, .ifilt=1 } }, /* RXENABLE/P56: MAX2837.RXENABLE(I) */
	[P4_6]  = {  4,  6, { .mode=0, .pd=0, .pu=0, .fast=0, .input=0, .ifilt=1 } }, /* XCVR_EN: 10K PD, MAX2837.ENABLE(I) */
	[P4_7]  = {  4,  7, { .mode=1, .pd=0, .pu=0, .fast=0, .input=1, .ifilt=0 } }, /* GP_CLKIN/P72/MCU_CLK: SI5351C.CLK7(O) */
	[P4_8]  = {  4,  8, PinConfig::floating_input_with_pull(0, 4) }, /* SGPIO13/BANK2F3M2: CPLD.90/HOST_SYNC_EN(I) */
	[P4_9]  = {  4,  9, PinConfig::floating_input(4) }, /* SGPIO14/BANK2F3M4: CPLD.81(I) */
	[P4_10] = {  4, 10, PinConfig::floating_input(4) }, /* SGPIO15/BANK2F3M6: CPLD.78(I) */
	[P5_0]  = {  5,  0, { .mode=0, .pd=0, .pu=0, .fast=0, .input=0, .ifilt=1 } }, /* !VAA_ENABLE: 10K PU, Q3.G(I), power to VAA */
	[P5_1]  = {  5,  1, { .mode=0, .pd=0, .pu=0, .fast=0, .input=0, .ifilt=1 } }, /* LP/P45: U6.VCTL2(I), U5.VCTL1(I) */
	[P5_2]  = {  5,  2, { .mode=0, .pd=0, .pu=0, .fast=0, .input=0, .ifilt=1 } }, /* TX_MIX_BP/P46: U9.V1(I) */
	[P5_3]  = {  5,  3, { .mode=0, .pd=0, .pu=0, .fast=0, .input=0, .ifilt=1 } }, /* RX_MIX_BP/P47: U9.V3(I) */
	[P5_4]  = {  5,  4, { .mode=0, .pd=0, .pu=0, .fast=0, .input=0, .ifilt=1 } }, /* MIXER_ENX/P32: 10K PU, 33pF, RFFC5072.ENX(I) */
	[P5_5]  = {  5,  5, { .mode=0, .pd=0, .pu=0, .fast=0, .input=0, .ifilt=1 } }, /* MIXER_RESETX/P33: 10K PU, 33pF, RFFC5072.RESETX(I) */
	[P5_6]  = {  5,  6, { .mode=0, .pd=0, .pu=0, .fast=0, .input=0, .ifilt=1 } }, /* TX_AMP/P48: U12.V3(I), U14.V1(I) */
	[P5_7]  = {  5,  7, { .mode=0, .pd=0, .pu=0, .fast=0, .input=0, .ifilt=1 } }, /* CS_AD/P54: MAX5864.CS(I) */
	[P6_0]  = {  6,  0, { .mode=0, .pd=0, .pu=1, .fast=0, .input=0, .ifilt=1 } }, /* I2S0_RX_MCLK: Unused */
	[P6_1]  = {  6,  1, { .mode=0, .pd=0, .pu=0, .fast=0, .input=0, .ifilt=1 } }, /* CPLD_TCK: CPLD.TCK(I), PortaPack CPLD.TCK(I) */
	[P6_2]  = {  6,  2, { .mode=0, .pd=0, .pu=0, .fast=0, .input=1, .ifilt=1 } }, /* CPLD_TDI: CPLD.TDI(I), PortaPack I2S0_RX_SDA(O), PortaPack CPLD.TDI(I) */
	[P6_3]  = {  6,  3, PinConfig::sgpio_inout_fast(2) }, /* SGPIO4/BANK2F3M14: CPLD.67/HOST_DATA4(IO) */
	[P6_4]  = {  6,  4, { .mode=0, .pd=0, .pu=0, .fast=0, .input=1, .ifilt=1 } }, /* MIXER_SDATA/P27: 33pF, RFFC5072.SDATA(IO) */
	[P6_5]  = {  6,  5, { .mode=0, .pd=0, .pu=0, .fast=0, .input=0, .ifilt=1 } }, /* CPLD_TMS: CPLD.TMS(I) */
	[P6_6]  = {  6,  6, PinConfig::sgpio_inout_fast(2) }, /* SGPIO5/BANK2F3M15: CPLD.64/HOST_DATA5(IO) */
	[P6_7]  = {  6,  7, { .mode=4, .pd=0, .pu=0, .fast=0, .input=0, .ifilt=1 } }, /* TX/P42: U7.VCTL2(I), U10.VCTL2(I), U2.VCTL2(I) */
	[P6_8]  = {  6,  8, { .mode=4, .pd=0, .pu=0, .fast=0, .input=0, .ifilt=1 } }, /* MIX_BYPASS/P34: U1.VCTL2(I), U11.VCTL1(I) */
	[P6_9]  = {  6,  9, { .mode=0, .pd=0, .pu=0, .fast=0, .input=0, .ifilt=1 } }, /* !TX_AMP_PWR/P51: 10K PU, Q2.G(I), power to U25 (TX amp) */
	[P6_10] = {  6, 10, { .mode=0, .pd=0, .pu=0, .fast=0, .input=0, .ifilt=1 } }, /* EN1V8/P70: 10K PD, TPS62410.EN2(I), 1V8LED.A(I) */
	[P6_11] = {  6, 11, { .mode=0, .pd=0, .pu=0, .fast=0, .input=0, .ifilt=1 } }, /* VREGMODE/P69: TPS62410.MODE/DATA(I) */
	[P6_12] = {  6, 12, PinConfig::gpio_led(0) }, /* LED3: LED3.A(I) */
	[P7_0]  = {  7,  0, PinConfig::gpio_inout_with_pulldown(0) }, /* GPIO3_8: PortaPack GPIO3_8(IO) */
	[P7_1]  = {  7,  1, PinConfig::gpio_inout_with_pulldown(0) }, /* GPIO3_9: PortaPack GPIO3_9(IO) */
	[P7_2]  = {  7,  2, PinConfig::gpio_inout_with_pulldown(0) }, /* GPIO3_10: PortaPack GPIO3_10(IO) */
	[P7_3]  = {  7,  3, PinConfig::gpio_inout_with_pulldown(0) }, /* GPIO3_11: PortaPack GPIO3_11(IO) */
	[P7_4]  = {  7,  4, PinConfig::gpio_inout_with_pulldown(0) }, /* GPIO3_12: PortaPack GPIO3_12(IO) */
	[P7_5]  = {  7,  5, PinConfig::gpio_inout_with_pulldown(0) }, /* GPIO3_13: PortaPack GPIO3_13(IO) */
	[P7_6]  = {  7,  6, PinConfig::gpio_inout_with_pulldown(0) }, /* GPIO3_14: PortaPack GPIO3_14(IO) */
	[P7_7]  = {  7,  7, PinConfig::gpio_inout_with_pulldown(0) }, /* GPIO3_15: PortaPack GPIO3_15(IO) */
	[P9_5]  = {  9,  5, { .mode=4, .pd=0, .pu=0, .fast=0, .input=1, .ifilt=1 } }, /* CPLD_TDO: CPLD.TDO(O) */
	[P9_6]  = {  9,  6, PinConfig::sgpio_in_fast(6) }, /* SGPIO8/SGPIO_CLK/P60: SI5351C.CLK2(O) */
	[PF_4]  = { 15,  4, { .mode=7, .pd=0, .pu=1, .fast=0, .input=0, .ifilt=1 } }, /* I2S0_RX_SCK: Unused */
	[CLK0]  = { 24,  0, { .mode=4, .pd=1, .pu=0, .fast=0, .input=1, .ifilt=0 } }, /* SD_CLK: PortaPack SD.CLK, enable input buffer for timing feedback? */
	[CLK2]  = { 24,  2, { .mode=6, .pd=1, .pu=0, .fast=0, .input=0, .ifilt=1 } }, /* I2S0_TX_CLK: PortaPack I2S0_TX_MCLK */
};

enum GPIOs {
	GPIO0_0, GPIO0_1, GPIO0_2, GPIO0_3, GPIO0_4, GPIO0_5, /*GPIO0_6,*/ GPIO0_7, GPIO0_8, GPIO0_9, GPIO0_10, GPIO0_11, GPIO0_12, GPIO0_13, GPIO0_14, GPIO0_15,
	GPIO1_0, GPIO1_1, GPIO1_2, GPIO1_3, GPIO1_4, GPIO1_5, GPIO1_6, GPIO1_7, GPIO1_8, GPIO1_9, GPIO1_10, GPIO1_11, GPIO1_12, GPIO1_13, /*GPIO1_14, GPIO1_15,*/
	GPIO2_0, GPIO2_1, GPIO2_2, GPIO2_3, GPIO2_4, GPIO2_5, GPIO2_6, GPIO2_7, GPIO2_8, GPIO2_9, GPIO2_10, GPIO2_11, GPIO2_12, GPIO2_13, GPIO2_14, GPIO2_15,
	GPIO3_0, GPIO3_1, GPIO3_2, GPIO3_3, GPIO3_4, GPIO3_5, GPIO3_6, GPIO3_7, GPIO3_8, GPIO3_9, GPIO3_10, GPIO3_11, GPIO3_12, GPIO3_13, GPIO3_14, GPIO3_15,
	GPIO4_11,
	GPIO5_0, GPIO5_1, GPIO5_2, GPIO5_3, GPIO5_4, GPIO5_5, GPIO5_6, GPIO5_7, GPIO5_8, GPIO5_9, /*GPIO5_10, GPIO5_11,*/ GPIO5_12, GPIO5_13, GPIO5_14, GPIO5_15, GPIO5_16, GPIO5_18,
};

constexpr GPIO gpio[] = {
	[GPIO0_0]  = { pins[P0_0],  0,  0, 0 },
	[GPIO0_1]  = { pins[P0_1],  0,  1, 0 },
	[GPIO0_2]  = { pins[P1_15], 0,  2, 0 },
	[GPIO0_3]  = { pins[P1_16], 0,  3, 0 },
	[GPIO0_4]  = { pins[P1_0],  0,  4, 0 },
	[GPIO0_5]  = { pins[P6_6],  0,  5, 0 },
	//[GPIO0_6]  = { pins[P3_6],  0,  6, 0 },
	[GPIO0_7]  = { pins[P2_7],  0,  7, 0 },
	[GPIO0_8]  = { pins[P1_1],  0,  8, 0 },
	[GPIO0_9]  = { pins[P1_2],  0,  9, 0 },
	[GPIO0_10] = { pins[P1_3],  0, 10, 0 },
	[GPIO0_11] = { pins[P1_4],  0, 11, 0 },
	[GPIO0_12] = { pins[P1_17], 0, 12, 0 },
	[GPIO0_13] = { pins[P1_18], 0, 13, 0 },
	[GPIO0_14] = { pins[P2_10], 0, 14, 0 },
	[GPIO0_15] = { pins[P1_20], 0, 15, 0 },

	[GPIO1_0]  = { pins[P1_7],  1,  0, 0 },
	[GPIO1_1]  = { pins[P1_8],  1,  1, 0 },
	[GPIO1_2]  = { pins[P1_9],  1,  2, 0 },
	[GPIO1_3]  = { pins[P1_10], 1,  3, 0 },
	[GPIO1_4]  = { pins[P1_11], 1,  4, 0 },
	[GPIO1_5]  = { pins[P1_12], 1,  5, 0 },
	[GPIO1_6]  = { pins[P1_13], 1,  6, 0 },
	[GPIO1_7]  = { pins[P1_14], 1,  7, 0 },
	[GPIO1_8]  = { pins[P1_5],  1,  8, 0 },
	[GPIO1_9]  = { pins[P1_6],  1,  9, 0 },
	[GPIO1_10] = { pins[P2_9],  1, 10, 0 },
	[GPIO1_11] = { pins[P2_11], 1, 11, 0 },
	[GPIO1_12] = { pins[P2_12], 1, 12, 0 },
	[GPIO1_13] = { pins[P2_13], 1, 13, 0 },
	//[GPIO1_14] = { pins[P3_4],  1, 14, 0 },
	//[GPIO1_15] = { pins[P3_5],  1, 15, 0 },

	[GPIO2_0]  = { pins[P4_0],  2,  0, 0 },
	[GPIO2_1]  = { pins[P4_1],  2,  1, 0 },
	[GPIO2_2]  = { pins[P4_2],  2,  2, 0 },
	[GPIO2_3]  = { pins[P4_3],  2,  3, 0 },
	[GPIO2_4]  = { pins[P4_4],  2,  4, 0 },
	[GPIO2_5]  = { pins[P4_5],  2,  5, 0 },
	[GPIO2_6]  = { pins[P4_6],  2,  6, 0 },
	[GPIO2_7]  = { pins[P5_7],  2,  7, 0 },
	[GPIO2_8]  = { pins[P6_12], 2,  8, 0 },
	[GPIO2_9]  = { pins[P5_0],  2,  9, 0 },
	[GPIO2_10] = { pins[P5_1],  2, 10, 0 },
	[GPIO2_11] = { pins[P5_2],  2, 11, 0 },
	[GPIO2_12] = { pins[P5_3],  2, 12, 0 },
	[GPIO2_13] = { pins[P5_4],  2, 13, 0 },
	[GPIO2_14] = { pins[P5_5],  2, 14, 0 },
	[GPIO2_15] = { pins[P5_6],  2, 15, 0 },

	[GPIO3_0]  = { pins[P6_1],  3,  0, 0 },
	[GPIO3_1]  = { pins[P6_2],  3,  1, 0 },
	[GPIO3_2]  = { pins[P6_3],  3,  2, 0 },
	[GPIO3_3]  = { pins[P6_4],  3,  3, 0 },
	[GPIO3_4]  = { pins[P6_5],  3,  4, 0 },
	[GPIO3_5]  = { pins[P6_9],  3,  5, 0 },
	[GPIO3_6]  = { pins[P6_10], 3,  6, 0 },
	[GPIO3_7]  = { pins[P6_11], 3,  7, 0 },
	[GPIO3_8]  = { pins[P7_0],  3,  8, 0 },
	[GPIO3_9]  = { pins[P7_1],  3,  9, 0 },
	[GPIO3_10] = { pins[P7_2],  3, 10, 0 },
	[GPIO3_11] = { pins[P7_3],  3, 11, 0 },
	[GPIO3_12] = { pins[P7_4],  3, 12, 0 },
	[GPIO3_13] = { pins[P7_5],  3, 13, 0 },
	[GPIO3_14] = { pins[P7_6],  3, 14, 0 },
	[GPIO3_15] = { pins[P7_7],  3, 15, 0 },

	[GPIO4_11] = { pins[P9_6],  4, 11, 0 },

	[GPIO5_0]  = { pins[P2_0],  5,  0, 4 },
	[GPIO5_1]  = { pins[P2_1],  5,  1, 4 },
	[GPIO5_2]  = { pins[P2_2],  5,  2, 4 },
	[GPIO5_3]  = { pins[P2_3],  5,  3, 4 },
	[GPIO5_4]  = { pins[P2_4],  5,  4, 4 },
	[GPIO5_5]  = { pins[P2_5],  5,  5, 4 },
	[GPIO5_6]  = { pins[P2_6],  5,  6, 4 },
	[GPIO5_7]  = { pins[P2_8],  5,  7, 4 },
	[GPIO5_8]  = { pins[P3_1],  5,  8, 4 },
	[GPIO5_9]  = { pins[P3_2],  5,  9, 4 },
	//[GPIO5_10] = { pins[P3_7],  5, 10, 4 },
	//[GPIO5_11] = { pins[P3_8],  5, 11, 4 },
	[GPIO5_12] = { pins[P4_8],  5, 12, 4 },
	[GPIO5_13] = { pins[P4_9],  5, 13, 4 },
	[GPIO5_14] = { pins[P4_10], 5, 14, 4 },
	[GPIO5_15] = { pins[P6_7],  5, 15, 4 },
	[GPIO5_16] = { pins[P6_8],  5, 16, 4 },
	[GPIO5_18] = { pins[P9_5],  5, 18, 4 },
};

}

#endif/*__PINS_H__*/
