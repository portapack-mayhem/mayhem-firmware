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

#ifndef __HACKRF_GPIO_H__
#define __HACKRF_GPIO_H__

#include "pins.hpp"
#include "led.hpp"

#include <array>

using namespace lpc43xx;

namespace hackrf {
namespace one {

/* GPIO */

constexpr GPIO gpio_led_usb = gpio[GPIO2_1];
constexpr GPIO gpio_led_rx = gpio[GPIO2_2];
constexpr GPIO gpio_led_tx = gpio[GPIO2_8];

constexpr GPIO gpio_og_1v8_enable = gpio[GPIO3_6];
constexpr GPIO gpio_r9_1v8_enable = gpio[GPIO2_9];
constexpr GPIO gpio_vregmode = gpio[GPIO3_7];
#ifdef PRALINE
// PRALINE uses different power control pins
constexpr GPIO gpio_og_vaa_disable = gpio[GPIO4_1];  // PRALINE VAA disable (P8_1)
constexpr GPIO gpio_r9_vaa_disable = gpio[GPIO4_1];  // PRALINE VAA disable (P8_1)
#else
constexpr GPIO gpio_og_vaa_disable = gpio[GPIO2_9];
constexpr GPIO gpio_r9_vaa_disable = gpio[GPIO3_6];
#endif

constexpr GPIO gpio_rx_mix_bp = gpio[GPIO2_12];
constexpr GPIO gpio_tx_mix_bp = gpio[GPIO2_11];
#ifdef PRALINE
constexpr GPIO gpio_mix_bypass = gpio[GPIO3_2];     // P6_3: PRALINE RF path mixer bypass inverted
constexpr GPIO gpio_mix_en_n_r1_0 = gpio[GPIO5_6];  // P2_6: R1.0 board mixer bypass
#else
constexpr GPIO gpio_mix_bypass = gpio[GPIO5_16];
#endif
constexpr GPIO gpio_not_mix_bypass = gpio[GPIO1_0];

constexpr GPIO gpio_og_rx = gpio[GPIO5_5];
constexpr GPIO gpio_og_tx = gpio[GPIO5_15];
constexpr GPIO gpio_r9_rx = gpio[GPIO0_7];

constexpr GPIO gpio_lp = gpio[GPIO2_10];
constexpr GPIO gpio_hp = gpio[GPIO2_0];

constexpr GPIO gpio_rx_amp = gpio[GPIO1_11];
constexpr GPIO gpio_tx_amp = gpio[GPIO2_15];
constexpr GPIO gpio_amp_bypass = gpio[GPIO0_14];
constexpr GPIO gpio_not_rx_amp_pwr = gpio[GPIO1_12];
constexpr GPIO gpio_not_tx_amp_pwr = gpio[GPIO3_5];

#ifdef PRALINE
// PRALINE: GPIO2[13] is SPI CS only, FPGA controls ENX/RESETX
constexpr GPIO gpio_rffc5072_select = gpio[GPIO2_13];  // P5_4: SPI CS (ENX)
constexpr GPIO gpio_rffc5072_resetx = gpio[GPIO2_14];  // P5_5: LPC43xx controls directly
#else
constexpr GPIO gpio_rffc5072_select = gpio[GPIO2_13];
constexpr GPIO gpio_rffc5072_resetx = gpio[GPIO2_14];
#endif

#ifdef PRALINE
constexpr GPIO gpio_rffc5072_clock = gpio[GPIO5_18];
constexpr GPIO gpio_rffc5072_data = gpio[GPIO4_14];
#else
constexpr GPIO gpio_rffc5072_clock = gpio[GPIO5_6];
constexpr GPIO gpio_rffc5072_data = gpio[GPIO3_3];
#endif

#ifdef PRALINE
constexpr GPIO gpio_max283x_select = gpio[GPIO6_28];
#else
constexpr GPIO gpio_max283x_select = gpio[GPIO0_15];
#endif

#ifdef PRALINE
// PRALINE uses MAX2831 transceiver with different control pins
constexpr GPIO gpio_max283x_enable = gpio[GPIO7_1];     // MAX2831 ENABLE (PE_1)
constexpr GPIO gpio_max2831_enable = gpio[GPIO7_1];     // Alias
constexpr GPIO gpio_max2831_rx_enable = gpio[GPIO7_2];  // MAX2831 RX_ENABLE (PE_2)
constexpr GPIO gpio_max2831_rxhp = gpio[GPIO6_29];      // MAX2831 RXHP (PD_15)
constexpr GPIO gpio_max2831_ld = gpio[GPIO4_11];        // MAX2831 Lock Detect (P9_6)
// Legacy aliases for code compatibility
constexpr GPIO gpio_max2837_rxenable = gpio[GPIO7_2];
constexpr GPIO gpio_max2837_txenable = gpio[GPIO7_2];  // MAX2831 uses single RX/TX control
constexpr GPIO gpio_max2839_rxtx = gpio[GPIO7_2];
#else
constexpr GPIO gpio_max283x_enable = gpio[GPIO2_6];
constexpr GPIO gpio_max2837_rxenable = gpio[GPIO2_5];
constexpr GPIO gpio_max2837_txenable = gpio[GPIO2_4];
constexpr GPIO gpio_max2839_rxtx = gpio[GPIO2_5];
#endif

#ifdef PRALINE
constexpr GPIO gpio_max5864_select = gpio[GPIO6_30];  // PD_16: PRALINE MAX5864 CS
constexpr GPIO gpio_fpga_select = gpio[GPIO2_10];     // FPGA SPI CS (P5_1)
#else
constexpr GPIO gpio_max5864_select = gpio[GPIO2_7];
#endif

constexpr GPIO gpio_q_invert = gpio[GPIO0_13];

#ifdef PRALINE
// PRALINE power control
constexpr GPIO gpio_vaa_disable = gpio[GPIO4_1];      // VAA disable (P8_1)
constexpr GPIO gpio_1v2_enable = gpio[GPIO4_7];       // 1V2 enable (P8_7)
constexpr GPIO gpio_3v3aux_disable = gpio[GPIO5_15];  // 3V3 aux disable (P6_7)
constexpr GPIO gpio_vbus_enable = gpio[GPIO8_4];      // VBUS_IN_EN P8_4 ->LOW
constexpr GPIO gpio_vin_enable = gpio[GPIO8_5];       // VIN_IN_EN P8_5 ->HIGH
// PRALINE RF path control
constexpr GPIO gpio_tx_enable = gpio[GPIO3_4];  // TX enable (P6_5)
// constexpr GPIO gpio_mix_enable_n = gpio[GPIO3_2];       // Mixer enable inverted (P6_3)
constexpr GPIO gpio_lpf_enable = gpio[GPIO4_8];         // LPF enable (PA_1)
constexpr GPIO gpio_rf_amp_enable = gpio[GPIO4_9];      // RF amp enable (PA_2)
constexpr GPIO gpio_ant_bias_disable = gpio[GPIO1_12];  // Antenna bias disable (P2_12)

// PRALINE mixer lock detect (gpio_max2831_ld defined above at line 93)
constexpr GPIO gpio_rffc5072_ld = gpio[GPIO6_25];  // Mixer lock detect (PD_11)

// PRALINE LED4
constexpr GPIO gpio_led4 = gpio[GPIO4_6];  // LED4 (P8_6)

// PRALINE dual port control
constexpr GPIO gpio_p1_ctrl0 = gpio[GPIO0_14];    // P1 control 0 (P2_10)
constexpr GPIO gpio_p1_ctrl1 = gpio[GPIO5_16];    // P1 control 1 (P6_8)
constexpr GPIO gpio_p1_ctrl2 = gpio[GPIO3_5];     // P1 control 2 (P6_9)
constexpr GPIO gpio_p2_ctrl0 = gpio[GPIO7_3];     // P2 control 0 (PE_3)
constexpr GPIO gpio_p2_ctrl1 = gpio[GPIO7_4];     // P2 control 1 (PE_4)
constexpr GPIO gpio_clkin_ctrl = gpio[GPIO0_15];  // CLKIN control (P1_20)

// PRALINE trigger/sync I/O
constexpr GPIO gpio_trigger_in = gpio[GPIO6_26];  // Trigger input (PD_12)
constexpr GPIO gpio_trigger_out = gpio[GPIO5_6];  // Trigger output (P2_6)
constexpr GPIO gpio_pps_out = gpio[GPIO5_5];      // PPS output (P2_5)
#endif

#ifdef PRALINE
/* PRALINE has no HackRF CPLD. These pins are used for RFFC5072 and TX_EN instead.
 * Dummy assignments here allow cpld_update.cpp to compile; the functions
 * that use them are never called on PRALINE. */
constexpr GPIO gpio_cpld_tdo = gpio[GPIO3_0];  // dummy: reuse TCK pin
constexpr GPIO gpio_cpld_tms = gpio[GPIO3_1];  // dummy: reuse TDI pin
#else
constexpr GPIO gpio_cpld_tdo = gpio[GPIO5_18];
constexpr GPIO gpio_cpld_tms = gpio[GPIO3_4];
#endif
constexpr GPIO gpio_cpld_tck = gpio[GPIO3_0];
constexpr GPIO gpio_cpld_tdi = gpio[GPIO3_1];

constexpr GPIO gpio_r9_clkin_en = gpio[GPIO5_15];
constexpr GPIO gpio_r9_clkout_en = gpio[GPIO0_9];
constexpr GPIO gpio_r9_mcu_clk_en = gpio[GPIO0_8];
constexpr GPIO gpio_r9_not_ant_pwr = gpio[GPIO2_4];

/* LEDs */

constexpr LED led_usb{gpio_led_usb};
constexpr LED led_rx{gpio_led_rx};
constexpr LED led_tx{gpio_led_tx};

} /* namespace one */
} /* namespace hackrf */

#endif /*__HACKRF_GPIO_H__*/
