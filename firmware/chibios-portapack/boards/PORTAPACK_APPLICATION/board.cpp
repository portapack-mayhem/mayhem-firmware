/*
    ChibiOS/RT - Copyright (C) 2006-2013 Giovanni Di Sirio
                 Copyright (C) 2014 Jared Boone, ShareBrained Technology

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

#include <array>

#if HAL_USE_PAL || defined(__DOXYGEN__)
/**
 * @brief   PAL setup.
 * @details Digital I/O ports static configuration as defined in @p board.h.
 *          This variable is used by the HAL when initializing the PAL driver.
 */
/**
 * CoolRunner (HackRF) CPLD:
 * CoolRunner-II devices have internal pull-ups on TDI, TMS, and TCK.
 * It is not necessary to externally terminate JTAG pins with internal termination; they can be
 * left floating. External pull-ups on pins with internal termination is allowed, but not
 * necessary. External pull-down termination is not recommended as it would conflict with
 * the internal pull-ups
 *
 * LPC43xx pull-ups come on line when 3V3 supply voltage reaches about 2V.
 * 
 * 3V3 supply:
 * Ramps up in about 1ms.
 *
 * 1V8 supply:
 * Ramps up in about 1ms.
 * EN1V8 has a 10K pull-down on the HackRF and is pulled up (very gently) by the LPC43xx
 * bootloader at boot time. So until the EN1V8 pin is reconfigured as an output, the enable
 * pin on the 1V8 supply sits at about 0.55V, which feels untidy...
 * 1V8 supply is activated when GPIO is driven high by user code.
 */
const PALConfig pal_default_config = {
  .P = {
    {   // GPIO0
        .data
            = (1 << 15) // P1_20: CS_XCVR
            | (1 << 14) // P2_10: AMP_BYPASS
            | (0 << 13) // P1_18: SGPIO12, HOST_Q_INVERT
            | (0 << 12) // P1_17: SGPIO11, HOST_DIRECTION
            | (1 << 11) // P1_4:  SSP1_MOSI
            | (1 << 10) // P1_3:  SSP1_MISO
            | (0 <<  9) // P1_2:  10K PD, BOOT1
            | (1 <<  8) // P1_1:  10K PU, BOOT0
            | (1 <<  7) // P2_7:  10K PU, ISP
            | (0 <<  6) // P3_6:  SPIFI_MISO
            | (1 <<  5) // P6_6:  SGPIO5, HOST_DATA5
            | (1 <<  4) // P1_0:  SGPIO7, HOST_DATA7
            | (1 <<  3) // P1_16: SGPIO3, HOST_DATA3
            | (1 <<  2) // P1_15: SGPIO2, HOST_DATA2
            | (1 <<  1) // P0_1:  SGPIO1, HOST_DATA1
            | (1 <<  0) // P0_0:  SGPIO0, HOST_DATA0
            ,
        .dir
            = (1 << 15) // P1_20: CS_XCVR
            | (1 << 14) // P2_10: AMP_BYPASS
            | (1 << 13) // P1_18: SGPIO12, HOST_Q_INVERT
            | (0 << 12) // P1_17: SGPIO11, HOST_DIRECTION
            | (0 << 11) // P1_4:  SSP1_MOSI
            | (0 << 10) // P1_3:  SSP1_MISO
            | (0 <<  9) // P1_2:  10K PD, BOOT1
            | (0 <<  8) // P1_1:  10K PU, BOOT0
            | (0 <<  7) // P2_7:  10K PU, ISP
            | (0 <<  6) // P3_6:  SPIFI_MISO
            | (0 <<  5) // P6_6:  SGPIO5, HOST_DATA5
            | (0 <<  4) // P1_0:  SGPIO7, HOST_DATA7
            | (0 <<  3) // P1_16: SGPIO3, HOST_DATA3
            | (0 <<  2) // P1_15: SGPIO2, HOST_DATA2
            | (0 <<  1) // P0_1:  SGPIO1, HOST_DATA1
            | (0 <<  0) // P0_0:  SGPIO0, HOST_DATA0
    },
    {   // GPIO1
        .data
            = (1 << 15) // P3_5:  SPIFI_SIO2
            | (1 << 14) // P3_4:  SPIFI_SIO3
            | (1 << 13) // P2_13: PortaPack DIR
            | (1 << 12) // P2_12: !RX_AMP_PWR
            | (0 << 11) // P2_11: RX_AMP
            | (0 << 10) // P2_9:  10K PD, BOOT3, PortaPack LCD_WRX
            | (1 <<  9) // P1_6:  SD_CMD
            | (1 <<  8) // P1_5:  SD_POW, PortaPack CPLD.TDO(O) (input with pull up)
            | (1 <<  7) // P1_14: SGPIO10, HOST_DISABLE
            | (1 <<  6) // P1_13: SD_CD
            | (1 <<  5) // P1_12: SD_DAT3
            | (1 <<  4) // P1_11: SD_DAT2
            | (1 <<  3) // P1_10: SD_DAT1
            | (1 <<  2) // P1_9:  SD_DAT0
            | (1 <<  1) // P1_8:  PortaPack CPLD.TMS(I)
            | (0 <<  0) // P1_7:  !MIX_BYPASS
            ,
        .dir
            = (0 << 15) // P3_5:  SPIFI_SIO2
            | (0 << 14) // P3_4:  SPIFI_SIO3
            | (0 << 13) // P2_13: PortaPack DIR
            | (1 << 12) // P2_12: !RX_AMP_PWR
            | (1 << 11) // P2_11: RX_AMP
            | (0 << 10) // P2_9:  10K PD, BOOT3, PortaPack LCD_WRX
            | (0 <<  9) // P1_6:  SD_CMD
            | (0 <<  8) // P1_5:  SD_POW, PortaPack CPLD.TDO(O) (input with pull up)
            | (0 <<  7) // P1_14: SGPIO10, HOST_DISABLE
            | (0 <<  6) // P1_13: SD_CD
            | (0 <<  5) // P1_12: SD_DAT3
            | (0 <<  4) // P1_11: SD_DAT2
            | (0 <<  3) // P1_10: SD_DAT1
            | (0 <<  2) // P1_9:  SD_DAT0
            | (0 <<  1) // P1_8:  PortaPack CPLD.TMS(I) (output only when needed, pull up internal to CPLD)
            | (1 <<  0) // P1_7:  !MIX_BYPASS
    },
    {   // GPIO2
        .data
            = (0 << 15) // P5_6:  TX_AMP
            | (1 << 14) // P5_5:  MIXER_RESETX, 10K PU
            | (1 << 13) // P5_4:  MIXER_ENX, 10K PU
            | (1 << 12) // P5_3:  RX_MIX_BP
            | (0 << 11) // P5_2:  TX_MIX_BP
            | (0 << 10) // P5_1:  LP
            | (1 <<  9) // P5_0:  !VAA_ENABLE
            | (0 <<  8) // P6_12: LED3 (TX)
            | (1 <<  7) // P5_7:  CS_AD
            | (0 <<  6) // P4_6:  XCVR_EN, 10K PD
            | (0 <<  5) // P4_5:  RXENABLE
            | (0 <<  4) // P4_4:  TXENABLE
            | (1 <<  3) // P4_3:  SGPIO9, HOST_CAPTURE
            | (0 <<  2) // P4_2:  LED2 (RX)
            | (0 <<  1) // P4_1:  LED1 (USB)
            | (1 <<  0) // P4_0:  HP
            ,
        .dir
            = (1 << 15) // P5_6:  TX_AMP
            | (1 << 14) // P5_5:  MIXER_RESETX, 10K PU
            | (1 << 13) // P5_4:  MIXER_ENX, 10K PU
            | (1 << 12) // P5_3:  RX_MIX_BP
            | (1 << 11) // P5_2:  TX_MIX_BP
            | (1 << 10) // P5_1:  LP
            | (1 <<  9) // P5_0:  !VAA_ENABLE
            | (1 <<  8) // P6_12: LED3 (TX)
            | (1 <<  7) // P5_7:  CS_AD
            | (1 <<  6) // P4_6:  XCVR_EN, 10K PD
            | (1 <<  5) // P4_5:  RXENABLE
            | (1 <<  4) // P4_4:  TXENABLE
            | (0 <<  3) // P4_3:  SGPIO9, HOST_CAPTURE
            | (1 <<  2) // P4_2:  LED2 (RX)
            | (1 <<  1) // P4_1:  LED1 (USB)
            | (1 <<  0) // P4_0:  HP
    },
    {   // GPIO3
        .data
            = (1 << 15) // P7_7:  PortaPack GPIO3_15(IO)
            | (1 << 14) // P7_6:  PortaPack GPIO3_14(IO)
            | (1 << 13) // P7_5:  PortaPack GPIO3_13(IO)
            | (1 << 12) // P7_4:  PortaPack GPIO3_12(IO)
            | (1 << 11) // P7_3:  PortaPack GPIO3_11(IO)
            | (1 << 10) // P7_2:  PortaPack GPIO3_10(IO)
            | (1 <<  9) // P7_1:  PortaPack GPIO3_9(IO)
            | (1 <<  8) // P7_0:  PortaPack GPIO3_8(IO)
            | (1 <<  7) // P6_11: VREGMODE
            | (0 <<  6) // P6_10: EN1V8, 10K PD
            | (1 <<  5) // P6_9:  !TX_AMP_PWR, 10K PU
            | (1 <<  4) // P6_5:  HackRF CPLD.TMS(I) (output only when needed, pull-up internal to CPLD when 1V8 present)
            | (1 <<  3) // P6_4:  MIXER_SDATA
            | (1 <<  2) // P6_3:  SGPIO4, HOST_DATA4
            | (1 <<  1) // P6_2:  HackRF CPLD.TDI(I), PortaPack I2S0_RX_SDA(O), PortaPack CPLD.TDI(I) (output only when needed, pull-up internal to CPLD when 1V8 present)
            | (1 <<  0) // P6_1:  HackRF CPLD.TCK(I), PortaPack CPLD.TCK(I) (output only when needed, pull-up internal to CPLD when 1V8 present)
            ,
        .dir
            = (0 << 15) // P7_7:  PortaPack GPIO3_15(IO)
            | (0 << 14) // P7_6:  PortaPack GPIO3_14(IO)
            | (0 << 13) // P7_5:  PortaPack GPIO3_13(IO)
            | (0 << 12) // P7_4:  PortaPack GPIO3_12(IO)
            | (0 << 11) // P7_3:  PortaPack GPIO3_11(IO)
            | (0 << 10) // P7_2:  PortaPack GPIO3_10(IO)
            | (0 <<  9) // P7_1:  PortaPack GPIO3_9(IO)
            | (0 <<  8) // P7_0:  PortaPack GPIO3_8(IO)
            | (1 <<  7) // P6_11: VREGMODE
            | (1 <<  6) // P6_10: EN1V8, 10K PD
            | (1 <<  5) // P6_9:  !TX_AMP_PWR, 10K PU
            | (0 <<  4) // P6_5:  HackRF CPLD.TMS(I) (output only when needed, pull-up internal to CPLD when 1V8 present)
            | (0 <<  3) // P6_4:  MIXER_SDATA
            | (0 <<  2) // P6_3:  SGPIO4, HOST_DATA4
            | (0 <<  1) // P6_2:  HackRF CPLD.TDI(I), PortaPack I2S0_RX_SDA(O), PortaPack CPLD.TDI(I) (output only when needed, pull-up internal to CPLD when 1V8 present)
            | (0 <<  0) // P6_1:  HackRF CPLD.TCK(I), PortaPack CPLD.TCK(I) (output only when needed, pull-up internal to CPLD when 1V8 present)
    },
    {   // GPIO4
        .data
            = (1 << 11) // P9_6:  SGPIO8, SGPIO_CLK
            ,
        .dir
            = (0 << 11) // P9_6:  SGPIO8, SGPIO_CLK
    },
    {   // GPIO5
        .data
            = (1 << 18) // P9:5:  HackRF CPLD.TDO(O) (input with pull up)
            | (1 << 16) // P6_8:  MIX_BYPASS
            | (0 << 15) // P6_7:  TX
            | (1 << 14) // P4_10: SGPIO15, CPLD (unused)
            | (1 << 13) // P4_9:  SGPIO14, CPLD (unused)
            | (0 << 12) // P4_8:  SGPIO13, HOST_SYNC_EN
            | (1 << 11) // P3_8:  SPIFI_CS
            | (1 << 10) // P3_7:  SPIFI_MOSI
            | (1 <<  9) // P3_2:  I2S0_RX_SDA
            | (1 <<  8) // P3_1:  I2S0_RX_WS
            | (0 <<  7) // P2_8:  BOOT2
            | (0 <<  6) // P2_6:  MIXER_SCLK
            | (1 <<  5) // P2_5:  RX
            | (1 <<  4) // P2_4:  PortaPack LCD_RDX
            | (0 <<  3) // P2_3:  PortaPack LCD_TE
            | (1 <<  2) // P2_2:  SGPIO6, HOST_DATA6
            | (0 <<  1) // P2_1:  PortaPack ADDR
            | (1 <<  0) // P2_0:  PortaPack IO_STBX
            ,
        .dir
            = (0 << 18) // P9_5:  HackRF CPLD.TDO(O) (input with pull up)
            | (1 << 16) // P6_8:  MIX_BYPASS
            | (1 << 15) // P6_7:  TX
            | (0 << 14) // P4_10: SGPIO15, CPLD (unused)
            | (0 << 13) // P4_9:  SGPIO14, CPLD (unused)
            | (0 << 12) // P4_8:  SGPIO13, HOST_SYNC_EN
            | (0 << 11) // P3_8:  SPIFI_CS
            | (0 << 10) // P3_7:  SPIFI_MOSI
            | (0 <<  9) // P3_2:  I2S0_RX_SDA
            | (0 <<  8) // P3_1:  I2S0_RX_WS
            | (0 <<  7) // P2_8:  BOOT2
            | (0 <<  6) // P2_6:  MIXER_SCLK
            | (1 <<  5) // P2_5:  RX
            | (0 <<  4) // P2_4:  PortaPack LCD_RDX
            | (0 <<  3) // P2_3:  PortaPack LCD_TE
            | (0 <<  2) // P2_2:  SGPIO6, HOST_DATA6
            | (0 <<  1) // P2_1:  PortaPack ADDR
            | (0 <<  0) // P2_0:  PortaPack IO_STBX
    },
    {   // GPIO6
        .data = 0,
        .dir = 0
    },
    {   // GPIO7
        .data = 0,
        .dir = 0
    },
  },
  .SCU = {
    /* Configure GP_CLKIN as soon as possible. It's an output at boot time, and the Si5351C doesn't
     * reset when the reset button is pressed, so it could still be output enabled.
     */
    {  4,  7, scu_config_normal_drive_t { .mode=1, .epd=0, .epun=0, .ehs=0, .ezi=1, .zif=1 } }, /* GP_CLKIN/P72/MCU_CLK: SI5351C.CLK7(O) */

    /* HackRF: LEDs. Configured early so we can use them to indicate boot status. */
    {  4,  1, scu_config_normal_drive_t { .mode=0, .epd=0, .epun=1, .ehs=0, .ezi=0, .zif=0 } }, /* LED1: LED1.A(I) */
    {  4,  2, scu_config_normal_drive_t { .mode=0, .epd=0, .epun=1, .ehs=0, .ezi=0, .zif=0 } }, /* LED2: LED2.A(I) */
    {  6, 12, scu_config_normal_drive_t { .mode=0, .epd=0, .epun=1, .ehs=0, .ezi=0, .zif=0 } }, /* LED3: LED3.A(I) */

    /* Power control */
    {  5,  0, scu_config_normal_drive_t { .mode=0, .epd=0, .epun=1, .ehs=0, .ezi=0, .zif=0 } }, /* !VAA_ENABLE: 10K PU, Q3.G(I), power to VAA */
    {  6, 10, scu_config_normal_drive_t { .mode=0, .epd=0, .epun=1, .ehs=0, .ezi=0, .zif=0 } }, /* EN1V8/P70: 10K PD, TPS62410.EN2(I), 1V8LED.A(I) */
    {  6, 11, scu_config_normal_drive_t { .mode=0, .epd=0, .epun=1, .ehs=0, .ezi=0, .zif=0 } }, /* VREGMODE/P69: TPS62410.MODE/DATA(I) */

    /* HackRF: I2C0 */
    /* Glitch filter operates at 3ns instead of 50ns due to the WM8731
     * returning an ACK very fast (170ns) and confusing the I2C state
     * machine into thinking there was a bus error. It looks like the
     * MCU sees SDA fall before SCL falls, indicating a START at the
     * point an ACK is expected. With the glitch filter off or set to
     * 3ns, it's probably still a bit tight timing-wise, but improves
     * reliability on some problem units.
     */
    { 25,  1,
        scu_config_sfsi2c0_t {
            .scl_efp=1,    // SCL: 3ns glitch
            .scl_ehd=0,    // SCL: Standard/Fast mode
            .scl_ezi=1,    // SCL: Input enabled
            .scl_zif=0,    // SCL: Enable input glitch filter
            .sda_efp=1,    // SDA: 3ns glitch
            .sda_ehd=0,    // SDA: Standard/Fast mode
            .sda_ezi=1,    // SDA: Input enabled
            .sda_zif=0    // SDA: Enable input glitch filter
        }
    },

    /* Radio section control */
    {  1,  3, scu_config_normal_drive_t { .mode=5, .epd=0, .epun=0, .ehs=0, .ezi=1, .zif=0 } }, /* SSP1_MISO/P41: MAX2837.DOUT(O) */
    {  1,  4, scu_config_normal_drive_t { .mode=5, .epd=0, .epun=0, .ehs=0, .ezi=0, .zif=0 } }, /* SSP1_MOSI/P40: MAX2837.DIN(I), MAX5864.DIN(I) */
    {  1,  7, scu_config_normal_drive_t { .mode=0, .epd=0, .epun=1, .ehs=0, .ezi=0, .zif=0 } }, /* !MIX_BYPASS/P35: U1.VCTL1(I), U11.VCTL2(I), U9.V2(I) */
    {  1, 19, scu_config_normal_drive_t { .mode=1, .epd=0, .epun=0, .ehs=0, .ezi=0, .zif=0 } }, /* SSP1_SCK/P39: MAX2837.SCLK(I), MAX5864.SCLK(I) */
    {  1, 20, scu_config_normal_drive_t { .mode=0, .epd=0, .epun=1, .ehs=0, .ezi=0, .zif=0 } }, /* CS_XCVR/P53: MAX2837.CS(I) */
    {  2,  5, scu_config_normal_drive_t { .mode=4, .epd=0, .epun=1, .ehs=0, .ezi=0, .zif=0 } }, /* RX/P43: U7.VCTL1(I), U10.VCTL1(I), U2.VCTL1(I) */
    {  2,  6, scu_config_normal_drive_t { .mode=4, .epd=0, .epun=1, .ehs=0, .ezi=0, .zif=0 } }, /* MIXER_SCLK/P31: 33pF, RFFC5072.SCLK(I) */
    {  2, 10, scu_config_normal_drive_t { .mode=0, .epd=0, .epun=1, .ehs=0, .ezi=0, .zif=0 } }, /* AMP_BYPASS/P50: U14.V2(I), U12.V2(I) */
    {  2, 11, scu_config_normal_drive_t { .mode=0, .epd=0, .epun=1, .ehs=0, .ezi=0, .zif=0 } }, /* RX_AMP/P49: U12.V1(I), U14.V3(I) */
    {  2, 12, scu_config_normal_drive_t { .mode=0, .epd=0, .epun=1, .ehs=0, .ezi=0, .zif=0 } }, /* !RX_AMP_PWR/P52: 10K PU, Q1.G(I), power to U13 (RX amp) */
    {  4,  0, scu_config_normal_drive_t { .mode=0, .epd=0, .epun=1, .ehs=0, .ezi=0, .zif=0 } }, /* HP/P44: U6.VCTL1(I), U5.VCTL2(I) */
    {  4,  4, scu_config_normal_drive_t { .mode=0, .epd=0, .epun=1, .ehs=0, .ezi=0, .zif=0 } }, /* TXENABLE/P55: MAX2837.TXENABLE(I) */
    {  4,  5, scu_config_normal_drive_t { .mode=0, .epd=0, .epun=1, .ehs=0, .ezi=0, .zif=0 } }, /* RXENABLE/P56: MAX2837.RXENABLE(I) */
    {  4,  6, scu_config_normal_drive_t { .mode=0, .epd=0, .epun=1, .ehs=0, .ezi=0, .zif=0 } }, /* XCVR_EN: 10K PD, MAX2837.ENABLE(I) */
    {  5,  1, scu_config_normal_drive_t { .mode=0, .epd=0, .epun=1, .ehs=0, .ezi=0, .zif=0 } }, /* LP/P45: U6.VCTL2(I), U5.VCTL1(I) */
    {  5,  2, scu_config_normal_drive_t { .mode=0, .epd=0, .epun=1, .ehs=0, .ezi=0, .zif=0 } }, /* TX_MIX_BP/P46: U9.V1(I) */
    {  5,  3, scu_config_normal_drive_t { .mode=0, .epd=0, .epun=1, .ehs=0, .ezi=0, .zif=0 } }, /* RX_MIX_BP/P47: U9.V3(I) */
    {  5,  4, scu_config_normal_drive_t { .mode=0, .epd=0, .epun=1, .ehs=0, .ezi=0, .zif=0 } }, /* MIXER_ENX/P32: 10K PU, 33pF, RFFC5072.ENX(I) */
    {  5,  5, scu_config_normal_drive_t { .mode=0, .epd=0, .epun=1, .ehs=0, .ezi=0, .zif=0 } }, /* MIXER_RESETX/P33: 10K PU, 33pF, RFFC5072.RESETX(I) */
    {  5,  6, scu_config_normal_drive_t { .mode=0, .epd=0, .epun=1, .ehs=0, .ezi=0, .zif=0 } }, /* TX_AMP/P48: U12.V3(I), U14.V1(I) */
    {  5,  7, scu_config_normal_drive_t { .mode=0, .epd=0, .epun=1, .ehs=0, .ezi=0, .zif=0 } }, /* CS_AD/P54: MAX5864.CS(I) */
    {  6,  4, scu_config_normal_drive_t { .mode=0, .epd=0, .epun=0, .ehs=0, .ezi=1, .zif=0 } }, /* MIXER_SDATA/P27: 33pF, RFFC5072.SDATA(IO) */
    {  6,  7, scu_config_normal_drive_t { .mode=4, .epd=0, .epun=1, .ehs=0, .ezi=0, .zif=0 } }, /* TX/P42: U7.VCTL2(I), U10.VCTL2(I), U2.VCTL2(I) */
    {  6,  8, scu_config_normal_drive_t { .mode=4, .epd=0, .epun=1, .ehs=0, .ezi=0, .zif=0 } }, /* MIX_BYPASS/P34: U1.VCTL2(I), U11.VCTL1(I) */
    {  6,  9, scu_config_normal_drive_t { .mode=0, .epd=0, .epun=1, .ehs=0, .ezi=0, .zif=0 } }, /* !TX_AMP_PWR/P51: 10K PU, Q2.G(I), power to U25 (TX amp) */

    /* SGPIO for sample transfer interface to HackRF CPLD. */
    {  0,  0, scu_config_normal_drive_t { .mode=3, .epd=0, .epun=1, .ehs=1, .ezi=1, .zif=1 } }, /* SGPIO0/P75/BANK2F3M3: CPLD.89/HOST_DATA0(IO) */
    {  0,  1, scu_config_normal_drive_t { .mode=3, .epd=0, .epun=1, .ehs=1, .ezi=1, .zif=1 } }, /* SGPIO1/BANK2F3M5: CPLD.79/HOST_DATA1(IO) */
    {  1, 15, scu_config_normal_drive_t { .mode=2, .epd=0, .epun=1, .ehs=1, .ezi=1, .zif=1 } }, /* SGPIO2/BANK2F3M9: CPLD.74/HOST_DATA2(IO) */
    {  1, 16, scu_config_normal_drive_t { .mode=2, .epd=0, .epun=1, .ehs=1, .ezi=1, .zif=1 } }, /* SGPIO3/BANK2F3M10: CPLD.72/HOST_DATA3(IO) */
    {  6,  3, scu_config_normal_drive_t { .mode=2, .epd=0, .epun=1, .ehs=1, .ezi=1, .zif=1 } }, /* SGPIO4/BANK2F3M14: CPLD.67/HOST_DATA4(IO) */
    {  6,  6, scu_config_normal_drive_t { .mode=2, .epd=0, .epun=1, .ehs=1, .ezi=1, .zif=1 } }, /* SGPIO5/BANK2F3M15: CPLD.64/HOST_DATA5(IO) */
    {  2,  2, scu_config_normal_drive_t { .mode=0, .epd=0, .epun=1, .ehs=1, .ezi=1, .zif=1 } }, /* SGPIO6/BANK2F3M16: CPLD.61/HOST_DATA6(IO) */
    {  1,  0, scu_config_normal_drive_t { .mode=6, .epd=0, .epun=1, .ehs=1, .ezi=1, .zif=1 } }, /* SGPIO7/P76/BANK2F3M7: CPLD.77/HOST_DATA7(IO) */
    {  9,  6, scu_config_normal_drive_t { .mode=6, .epd=0, .epun=0, .ehs=0, .ezi=1, .zif=1 } }, /* SGPIO8/SGPIO_CLK/P60: SI5351C.CLK2(O) */
    {  4,  3, scu_config_normal_drive_t { .mode=7, .epd=0, .epun=1, .ehs=0, .ezi=1, .zif=1 } }, /* SGPIO9/P77/BANK2F3M1: CPLD.91/HOST_CAPTURE(O) */
    {  1, 14, scu_config_normal_drive_t { .mode=6, .epd=0, .epun=0, .ehs=1, .ezi=0, .zif=0 } }, /* SGPIO10/P78/BANK2F3M8: CPLD.76/HOST_DISABLE(I) */
    {  1, 17, scu_config_normal_drive_t { .mode=6, .epd=1, .epun=1, .ehs=1, .ezi=0, .zif=0 } }, /* SGPIO11/P79/BANK2F3M11: CPLD.71/HOST_DIRECTION(I) */
    {  1, 18, scu_config_normal_drive_t { .mode=0, .epd=0, .epun=1, .ehs=0, .ezi=0, .zif=0 } }, /* SGPIO12/BANK2F3M12: CPLD.70/HOST_INVERT(I) */
    {  4,  8, scu_config_normal_drive_t { .mode=4, .epd=1, .epun=1, .ehs=0, .ezi=0, .zif=0 } }, /* SGPIO13/BANK2F3M2: CPLD.90/HOST_SYNC_EN(I) */
    {  4,  9, scu_config_normal_drive_t { .mode=4, .epd=0, .epun=0, .ehs=0, .ezi=0, .zif=0 } }, /* SGPIO14/BANK2F3M4: CPLD.81/CPLD_P81 */
    {  4, 10, scu_config_normal_drive_t { .mode=4, .epd=0, .epun=0, .ehs=0, .ezi=0, .zif=0 } }, /* SGPIO15/BANK2F3M6: CPLD.78/CPLD_P78 */

    /* HackRF: CPLD */
    {  6,  1, scu_config_normal_drive_t { .mode=0, .epd=0, .epun=1, .ehs=0, .ezi=0, .zif=0 } }, /* CPLD_TCK: CPLD.TCK(I), PortaPack CPLD.TCK(I) */
    {  6,  2, scu_config_normal_drive_t { .mode=0, .epd=0, .epun=1, .ehs=0, .ezi=1, .zif=0 } }, /* CPLD_TDI: CPLD.TDI(I), PortaPack I2S0_RX_SDA(O), PortaPack CPLD.TDI(I) */
    {  6,  5, scu_config_normal_drive_t { .mode=0, .epd=0, .epun=1, .ehs=0, .ezi=0, .zif=0 } }, /* CPLD_TMS: CPLD.TMS(I) */
    {  9,  5, scu_config_normal_drive_t { .mode=4, .epd=0, .epun=0, .ehs=0, .ezi=1, .zif=0 } }, /* CPLD_TDO: CPLD.TDO(O) */

    /* PortaPack CPLD */
    {  1,  5, scu_config_normal_drive_t { .mode=0, .epd=0, .epun=0, .ehs=0, .ezi=1, .zif=0 } }, /* SD_POW: PortaPack CPLD.TDO(O) */
    {  1,  8, scu_config_normal_drive_t { .mode=0, .epd=0, .epun=0, .ehs=0, .ezi=0, .zif=0 } }, /* SD_VOLT0: PortaPack CPLD.TMS(I) */

    /* Miscellaneous */
    {  1,  1, scu_config_normal_drive_t { .mode=0, .epd=0, .epun=1, .ehs=0, .ezi=0, .zif=0 } }, /* P1_1/P74: 10K PU, BOOT0 */
    {  1,  2, scu_config_normal_drive_t { .mode=0, .epd=0, .epun=1, .ehs=0, .ezi=0, .zif=0 } }, /* P1_2/P73: 10K PD, BOOT1 */
    {  2,  7, scu_config_normal_drive_t { .mode=0, .epd=0, .epun=1, .ehs=0, .ezi=0, .zif=0 } }, /* ISP: 10K PU, Unused */
    {  6,  0, scu_config_normal_drive_t { .mode=0, .epd=0, .epun=0, .ehs=0, .ezi=0, .zif=0 } }, /* I2S0_RX_MCLK: Unused */
    { 15,  4, scu_config_normal_drive_t { .mode=7, .epd=0, .epun=0, .ehs=0, .ezi=0, .zif=0 } }, /* I2S0_RX_SCK: Unused */
  }
};
#endif

static const std::array<scu_setup_t, 26> pins_setup_portapack { {
    {  2,  0, scu_config_normal_drive_t { .mode=4, .epd=0, .epun=1, .ehs=0, .ezi=1, .zif=0 } }, /* U0_TXD: PortaPack P2_0/IO_STBX */
    {  2,  1, scu_config_normal_drive_t { .mode=4, .epd=0, .epun=1, .ehs=0, .ezi=1, .zif=0 } }, /* U0_RXD: PortaPack P2_1/ADDR */
    {  2,  3, scu_config_normal_drive_t { .mode=4, .epd=0, .epun=1, .ehs=0, .ezi=1, .zif=0 } }, /* I2C1_SDA: PortaPack P2_3/LCD_TE */
    {  2,  4, scu_config_normal_drive_t { .mode=4, .epd=0, .epun=1, .ehs=0, .ezi=1, .zif=0 } }, /* I2C1_SCL: PortaPack P2_4/LCD_RDX */
    {  2,  8, scu_config_normal_drive_t { .mode=4, .epd=0, .epun=1, .ehs=0, .ezi=0, .zif=0 } }, /* P2_8: 10K PD, BOOT2, DFU switch, PortaPack P2_8/<unused> */
    {  2,  9, scu_config_normal_drive_t { .mode=0, .epd=0, .epun=1, .ehs=0, .ezi=1, .zif=0 } }, /* P2_9: 10K PD, BOOT3, PortaPack P2_9/LCD_WRX */
    {  2, 13, scu_config_normal_drive_t { .mode=0, .epd=0, .epun=1, .ehs=0, .ezi=1, .zif=0 } }, /* P2_13: PortaPack P2_13/DIR */
    {  7,  0, scu_config_normal_drive_t { .mode=0, .epd=0, .epun=1, .ehs=0, .ezi=1, .zif=0 } }, /* GPIO3_8: PortaPack GPIO3_8(IO) */
    {  7,  1, scu_config_normal_drive_t { .mode=0, .epd=0, .epun=1, .ehs=0, .ezi=1, .zif=0 } }, /* GPIO3_9: PortaPack GPIO3_9(IO) */
    {  7,  2, scu_config_normal_drive_t { .mode=0, .epd=0, .epun=1, .ehs=0, .ezi=1, .zif=0 } }, /* GPIO3_10: PortaPack GPIO3_10(IO) */
    {  7,  3, scu_config_normal_drive_t { .mode=0, .epd=0, .epun=1, .ehs=0, .ezi=1, .zif=0 } }, /* GPIO3_11: PortaPack GPIO3_11(IO) */
    {  7,  4, scu_config_normal_drive_t { .mode=0, .epd=0, .epun=1, .ehs=0, .ezi=1, .zif=0 } }, /* GPIO3_12: PortaPack GPIO3_12(IO) */
    {  7,  5, scu_config_normal_drive_t { .mode=0, .epd=0, .epun=1, .ehs=0, .ezi=1, .zif=0 } }, /* GPIO3_13: PortaPack GPIO3_13(IO) */
    {  7,  6, scu_config_normal_drive_t { .mode=0, .epd=0, .epun=1, .ehs=0, .ezi=1, .zif=0 } }, /* GPIO3_14: PortaPack GPIO3_14(IO) */
    {  7,  7, scu_config_normal_drive_t { .mode=0, .epd=0, .epun=1, .ehs=0, .ezi=1, .zif=0 } }, /* GPIO3_15: PortaPack GPIO3_15(IO) */

    /* PortaPack: Audio */
    {  3,  0, scu_config_normal_drive_t { .mode=2, .epd=0, .epun=0, .ehs=0, .ezi=1, .zif=0 } }, /* I2S0_TX_SCK: PortaPack I2S0_TX_SCK(I) */
    {  3,  1, scu_config_normal_drive_t { .mode=0, .epd=0, .epun=0, .ehs=0, .ezi=1, .zif=0 } }, /* I2S0_RX_WS: PortaPack I2S0_TX_WS(I). Input enabled to fold back into RX. */
    {  3,  2, scu_config_normal_drive_t { .mode=0, .epd=0, .epun=0, .ehs=0, .ezi=0, .zif=0 } }, /* I2S0_RX_SDA: PortaPack I2S0_TX_SDA(I) */
    { 24,  2, scu_config_normal_drive_t { .mode=6, .epd=1, .epun=1, .ehs=0, .ezi=0, .zif=0 } }, /* I2S0_TX_CLK: PortaPack I2S0_TX_MCLK */

    /* PortaPack: SD card socket */
    { 24,  0, scu_config_normal_drive_t { .mode=4, .epd=1, .epun=1, .ehs=0, .ezi=1, .zif=1 } }, /* SD_CLK: PortaPack SD.CLK, enable input buffer for timing feedback? */
    {  1,  6, scu_config_normal_drive_t { .mode=7, .epd=0, .epun=0, .ehs=0, .ezi=1, .zif=1 } }, /* SD_CMD: PortaPack SD.CMD(IO)  */
    {  1,  9, scu_config_normal_drive_t { .mode=7, .epd=0, .epun=0, .ehs=0, .ezi=1, .zif=1 } }, /* SD_DAT0: PortaPack SD.DAT0(IO) */
    {  1, 10, scu_config_normal_drive_t { .mode=7, .epd=0, .epun=0, .ehs=0, .ezi=1, .zif=1 } }, /* SD_DAT1: PortaPack SD.DAT1(IO) */
    {  1, 11, scu_config_normal_drive_t { .mode=7, .epd=0, .epun=0, .ehs=0, .ezi=1, .zif=1 } }, /* SD_DAT2: PortaPack SD.DAT2(IO) */
    {  1, 12, scu_config_normal_drive_t { .mode=7, .epd=0, .epun=0, .ehs=0, .ezi=1, .zif=1 } }, /* SD_DAT3: PortaPack SD.DAT3(IO) */
    {  1, 13, scu_config_normal_drive_t { .mode=7, .epd=0, .epun=0, .ehs=0, .ezi=1, .zif=0 } }, /* SD_CD: PortaPack SD.CD(O) */
} };

static const std::array<scu_setup_t, 6> pins_setup_spifi { {
    {  3,  3, scu_config_normal_drive_t { .mode=3, .epd=0, .epun=1, .ehs=1, .ezi=1, .zif=1 } }, /* SPIFI_SCK: W25Q80BV.CLK(I), enable input buffer for timing feedback */
    {  3,  4, scu_config_normal_drive_t { .mode=3, .epd=0, .epun=1, .ehs=1, .ezi=1, .zif=1 } }, /* SPIFI_SIO3/P82: W25Q80BV.HOLD(IO) */
    {  3,  5, scu_config_normal_drive_t { .mode=3, .epd=0, .epun=1, .ehs=1, .ezi=1, .zif=1 } }, /* SPIFI_SIO2/P81: W25Q80BV.WP(IO) */
    {  3,  6, scu_config_normal_drive_t { .mode=3, .epd=0, .epun=1, .ehs=1, .ezi=1, .zif=1 } }, /* SPIFI_MISO: W25Q80BV.DO(IO) */
    {  3,  7, scu_config_normal_drive_t { .mode=3, .epd=0, .epun=1, .ehs=1, .ezi=1, .zif=1 } }, /* SPIFI_MOSI: W25Q80BV.DI(IO) */
    {  3,  8, scu_config_normal_drive_t { .mode=3, .epd=0, .epun=1, .ehs=1, .ezi=1, .zif=1 } }, /* SPIFI_CS/P68: W25Q80BV.CS(I) */
} };

static void setup_pin(const scu_setup_t& pin_setup) {
    LPC_SCU->SFSP[pin_setup.port][pin_setup.pin] = pin_setup.config;
}

template<size_t N>
void setup_pins(const std::array<scu_setup_t, N>& pins_setup) {
    for(const auto& pin_setup : pins_setup) {
        setup_pin(pin_setup);
    }
}

static void configure_spifi(void) {
    setup_pins(pins_setup_spifi);

    /* Tweak SPIFI mode */
    LPC_SPIFI->CTRL =
          (0xffff <<  0)    /* Timeout */
        | (0x1    << 16)    /* CS high time in "clocks - 1" */
        | (0      << 21)    /* 0: Attempt speculative prefetch on data accesses */
        | (0      << 22)    /* 0: No interrupt on command ended */
        | (0      << 23)    /* 0: SCK driven low after rising edge at which last bit of command is captured. Stays low while CS# is high. */
        | (0      << 27)    /* 0: Cache prefetching enabled */
        | (0      << 28)    /* 0: Quad protocol, IO3:0 */
        | (1      << 29)    /* 1: Read data sampled on falling edge of clock */
        | (1      << 30)    /* 1: Read data is sampled using feedback clock from SCK pin */
        | (0      << 31)    /* 0: DMA request disabled */
        ;

    /* Throttle up the SPIFI interface to 96MHz (IDIVA=PLL1 / 3) */
    LPC_CGU->IDIVB_CTRL.word =
          ( 0 <<  0)    /* PD */
        | ( 2 <<  2)    /* IDIV (/3) */
        | ( 1 << 11)    /* AUTOBLOCK */
        | ( 9 << 24)    /* PLL1 */
        ;
}

void configure_pins_portapack(void) {
    LPC_GPIO->DIR[1] |= (1 << 13) | (1 << 10);
    LPC_GPIO->DIR[3] |= (0xff << 8);
    LPC_GPIO->DIR[5] |= (1 <<  4) | (1 <<  1) | (1 <<  0);
    setup_pins(pins_setup_portapack);
}

static const motocon_pwm_resources_t motocon_pwm_resources = {
  .base = { .clk = &LPC_CGU->BASE_APB1_CLK, .stat = &LPC_CCU1->BASE_STAT, .stat_mask = (1 << 1) },
  .branch = { .cfg = &LPC_CCU1->CLK_APB1_MOTOCON_PWM_CFG, .stat = &LPC_CCU1->CLK_APB1_MOTOCON_PWM_STAT },
  .reset = { .output_index = 38 },
};

static const scu_setup_t pin_setup_vaa_enablex_pwm  = { 5, 0, scu_config_normal_drive_t { .mode=1, .epd=0, .epun=1, .ehs=0, .ezi=0, .zif=0 } };
static const scu_setup_t pin_setup_vaa_enablex_gpio = { 5, 0, scu_config_normal_drive_t { .mode=0, .epd=0, .epun=1, .ehs=0, .ezi=0, .zif=0 } };

/* VAA powers:
 * MAX5864 analog section.
 * MAX2837 registers and other functions.
 * RFFC5072 analog section.
 *
 * Beware that power applied to pins of the MAX2837 may
 * show up on VAA and start powering other components on the
 * VAA net. So turn on VAA before driving pins from MCU to
 * MAX2837.
 */
void vaa_power_on(void) {
  /* Very twitchy process for powering up VAA without glitching the 3.3V rail, which can send the
   * microcontroller into reset.
   *
   * Controlling timing while running from SPIFI flash is tricky, hence use of a PWM peripheral...
   */

  /* Configure and enable MOTOCONPWM peripheral clocks.
   * Assume IDIVC is running the post-bootloader configuration, outputting 96MHz derived from PLL1.
   */
  base_clock_enable(&motocon_pwm_resources.base);
  branch_clock_enable(&motocon_pwm_resources.branch);
  peripheral_reset(&motocon_pwm_resources.reset);

  /* Combination of pulse duration and duty cycle was arrived at empirically, to keep supply glitching
   * to +/- 0.15V.
   */
  const uint32_t cycle_period = 256;
  uint32_t enable_period = 2;
  LPC_MCPWM->TC2 = 0;
  LPC_MCPWM->MAT2 = cycle_period - enable_period;
  LPC_MCPWM->LIM2 = cycle_period;

  /* Switch !VAA_ENABLE pin from GPIO to MOTOCONPWM peripheral output, now that the peripheral is configured. */
  setup_pin(pin_setup_vaa_enablex_pwm); // P5_0 /GPIO2[ 9]/MCOB2: !VAA_ENABLE, 10K PU

  /* Start the PWM operation. */
  LPC_MCPWM->CON_SET = (1 << 16);

  /* Wait until VAA rises to approximately 90% of final voltage. */
  /* Timing assumes we're running immediately after the bootloader: 96 MHz from IRC+PLL1
   */
  while(enable_period < cycle_period) {
    { volatile uint32_t delay = 2000; while(delay--); }
    enable_period <<= 1;
    LPC_MCPWM->MAT2 = cycle_period - enable_period;
  }

  /* Hold !VAA_ENABLE active using a GPIO, so we can reclaim and shut down the MOTOCONPWM peripheral. */
  LPC_GPIO->CLR[2]  = (1 << 9); // !VAA_ENABLE
  LPC_GPIO->DIR[2] |= (1 << 9);
  setup_pin(pin_setup_vaa_enablex_gpio); // P5_0 /GPIO2[ 9]/MCOB2: !VAA_ENABLE, 10K PU

  peripheral_reset(&motocon_pwm_resources.reset);
  branch_clock_disable(&motocon_pwm_resources.branch);
  base_clock_disable(&motocon_pwm_resources.base);
}

void vaa_power_off(void) {
  // TODO: There's a lot of other stuff that must be done to prevent
  // leakage from +3V3 into VAA.
  LPC_GPIO->W2[9] = 1;
}

/**
 * @brief   Early initialization code.
 * @details This initialization must be performed just after stack setup
 *          and before any other initialization.
 */
extern "C" void __early_init(void) {
    /*
     * Upon exit from bootloader into SPIFI boot mode:
     *
     * Enabled:
     *   PLL1: IRC, M=/24, N=/1, P=/1, autoblock, direct = 288 MHz
     *   IDIVA: IRC /1 = 12 MHz
     *   IDIVB: PLL1 /9, autoblock = 32 MHz
     *   IDIVC: PLL1 /3, autoblock = 96 MHz
     *   IDIVD: IRC /1 = 12 MHz
     *   IDIVE: IRC /1 = 12 MHz
     *   BASE_M4_CLK: IDIVC, autoblock
     *   BASE_SPIFI_CLK: IDIVB, autoblock
     *
     * Disabled:
     *   XTAL_OSC
     *   PLL0USB
     *   PLL0AUDIO
     */
    /* LPC43xx M4 takes about 500 usec to get to __early_init
     * Before __early_init, LPC bootloader runs and starts our code. In user code, the process stack
     * is initialized, hardware floating point is initialized, and stacks are zeroed,
     */
    const uint32_t CORTEX_M4_CPUID      = 0x410fc240;
    const uint32_t CORTEX_M4_CPUID_MASK = 0xff0ffff0;

    if( (SCB->CPUID & CORTEX_M4_CPUID_MASK) == CORTEX_M4_CPUID ) {
        /* Enable unaligned exception handler */
        SCB_CCR |= (1 << 3);

        /* Enable MemManage, BusFault, UsageFault exception handlers */
        SCB_SHCSR |= (1 << 18) | (1 << 17) | (1 << 16);

        /* "The reset delay is counted in IRC clock cycles. If the core frequency
         * CCLK is much higher than the IRC frequency, add a software delay of
         * fCCLK/fIRC clock cycles between resetting and accessing any of the
         * peripheral blocks."
         */

        /* Don't reset these peripherals, as they're operating during initialization:
         *   WWDT, CREG, SCU, SPIFI
         */
        LPC_RGU->RESET_CTRL[0] =
              (1U << 16) // LCD_RST
            | (1U << 17) // USB0_RST
            | (1U << 18) // USB1_RST
            | (1U << 19) // DMA_RST
            | (1U << 20) // SDIO_RST
            | (1U << 21) // EMC_RST
            | (1U << 22) // ETHERNET_RST
            | (1U << 28) // GPIO_RST
            ;
        LPC_RGU->RESET_CTRL[1] =
              (1U <<  0) // TIMER0_RST
            | (1U <<  1) // TIMER1_RST
            | (1U <<  2) // TIMER2_RST
            | (1U <<  3) // TIMER3_RST
            | (1U <<  4) // RITIMER_RST
            | (1U <<  5) // SCT_RST
            | (1U <<  6) // MOTOCONPWM_RST
            | (1U <<  7) // QEI_RST
            | (1U <<  8) // ADC0_RST
            | (1U <<  9) // ADC1_RST
            | (1U << 10) // DAC_RST
            | (1U << 12) // UART0_RST
            | (1U << 13) // UART1_RST
            | (1U << 14) // UART2_RST
            | (1U << 15) // UART3_RST
            | (1U << 16) // I2C0_RST
            | (1U << 17) // I2C1_RST
            | (1U << 18) // SSP0_RST
            | (1U << 19) // SSP1_RST
            | (1U << 20) // I2S_RST
            | (1U << 22) // CAN1_RST
            | (1U << 23) // CAN0_RST
            | (1U << 24) // M0APP_RST
            | (1U << 25) // SGPIO_RST
            | (1U << 26) // SPI_RST
            | (1U << 28) // ADCHS_RST
            ;

        configure_spifi();

        LPC_CCU1->CLK_M4_M0APP_CFG.RUN = true;
        LPC_CREG->M0APPMEMMAP = LPC_SPIFI_DATA_CACHED_BASE + 0x0;
        LPC_RGU->RESET_CTRL[1] = 0;

        /* Prevent the M4 from doing any more initializing by sleep-waiting forever...
         * ...until the M0 resets the M4 with some code to run.
         */
        while(1) {
            __WFE();
        }
    }
}

extern "C" void __late_init(void) {
    /*
     * System initializations.
     * - HAL initialization, this also initializes the configured device drivers
     *   and performs the board-specific initializations.
     * - Kernel initialization, the main() function becomes a thread and the
     *   RTOS is active.
     */
    halInit();

    /* After this call, scheduler, systick, heap, etc. are available. */
    /* By doing chSysInit() here, it runs before C++ constructors, which may
     * require the heap.
     */
    chSysInit();
}

/**
 * @brief   Board-specific initialization code.
 * @todo    Add your board-specific code, if any.
 */
extern "C" void boardInit(void) {
  vaa_power_on();
  LPC_GPIO->W3[6] = 1;
}

extern "C" void _default_exit(void) {
    LPC_GPIO->W3[6] = 0;
    vaa_power_off();

    chSysDisable();

    systick_stop();

    /* Don't reset these peripherals, as they're operating during shutdown:
     *   WWDT, CREG, SCU, SPIFI, GPIO, M0APP
     */
    LPC_RGU->RESET_CTRL[0] =
          (1U << 16) // LCD_RST
        | (1U << 17) // USB0_RST
        | (1U << 18) // USB1_RST
        | (1U << 19) // DMA_RST
        | (1U << 20) // SDIO_RST
        | (1U << 21) // EMC_RST
        | (1U << 22) // ETHERNET_RST
        //| (1U << 28) // GPIO_RST
        ;
    LPC_RGU->RESET_CTRL[1] =
          (1U <<  0) // TIMER0_RST
        | (1U <<  1) // TIMER1_RST
        | (1U <<  2) // TIMER2_RST
        | (1U <<  3) // TIMER3_RST
        | (1U <<  4) // RITIMER_RST
        | (1U <<  5) // SCT_RST
        | (1U <<  6) // MOTOCONPWM_RST
        | (1U <<  7) // QEI_RST
        | (1U <<  8) // ADC0_RST
        | (1U <<  9) // ADC1_RST
        | (1U << 10) // DAC_RST
        | (1U << 12) // UART0_RST
        | (1U << 13) // UART1_RST
        | (1U << 14) // UART2_RST
        | (1U << 15) // UART3_RST
        | (1U << 16) // I2C0_RST
        | (1U << 17) // I2C1_RST
        | (1U << 18) // SSP0_RST
        | (1U << 19) // SSP1_RST
        | (1U << 20) // I2S_RST
        | (1U << 22) // CAN1_RST
        | (1U << 23) // CAN0_RST
        //| (1U << 24) // M0APP_RST
        | (1U << 25) // SGPIO_RST
        | (1U << 26) // SPI_RST
        | (1U << 28) // ADCHS_RST
        ;
}
