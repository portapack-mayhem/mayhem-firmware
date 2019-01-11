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
            = (1 << 15) // CS_XCVR
            | (1 << 14) // AMP_BYPASS
            | (0 <<  9) // 10K PD, BOOT1
            | (1 <<  8) // 10K PU, BOOT0
            ,
        .dir
            = (1 << 15) // CS_XCVR
            | (1 << 14) // AMP_BYPASS
            | (0 <<  9) // 10K PD, BOOT1
            | (0 <<  8) // 10K PU, BOOT0
    },
    {   // GPIO1
        .data
            = (1 << 13) // PortaPack P2_13/DIR
            | (1 << 12) // !RX_AMP_PWR
            | (0 << 11) // RX_AMP
            | (1 << 10) // 10K PD, BOOT3, PortaPack P2_9/LCD_WRX
            | (1 <<  8) // PortaPack CPLD.TDO(O)
            | (1 <<  1) // PortaPack CPLD.TMS(I)
            | (0 <<  0) // !MIX_BYPASS
            ,
        .dir
            = (1 << 13) // PortaPack P2_13/DIR
            | (1 << 12) // !RX_AMP_PWR
            | (1 << 11) // RX_AMP
            | (1 << 10) // 10K PD, BOOT3, PortaPack P2_9/LCD_WRX
            | (0 <<  8) // PortaPack CPLD.TDO(O) (input with pull up)
            | (0 <<  1) // PortaPack CPLD.TMS(I) (output only when needed, pull up internal to CPLD)
            | (1 <<  0) // !MIX_BYPASS
    },
    {   // GPIO2
        .data
            = (0 << 15) // TX_AMP
            | (0 << 11) // TX_MIX_BP
            | (1 << 14) // MIXER_RESETX, 10K PU
            | (1 << 13) // MIXER_ENX, 10K PU
            | (1 << 12) // RX_MIX_BP
            | (0 << 10) // LP
            | (1 <<  9) // !VAA_ENABLE
            | (0 <<  8) // LED3 (TX)
            | (1 <<  7) // CS_AD
            | (0 <<  6) // XCVR_EN, 10K PD
            | (0 <<  5) // RXENABLE
            | (0 <<  4) // TXENABLE
            | (0 <<  2) // LED2 (RX)
            | (0 <<  1) // LED1 (USB)
            | (1 <<  0) // HP
            ,
        .dir
            = (1 << 15) // TX_AMP
            | (1 << 14) // MIXER_RESETX, 10K PU
            | (1 << 13) // MIXER_ENX, 10K PU
            | (1 << 12) // RX_MIX_BP
            | (1 << 11) // TX_MIX_BP
            | (1 << 10) // LP
            | (1 <<  9) // !VAA_ENABLE
            | (1 <<  8) // LED3 (TX)
            | (1 <<  7) // CS_AD
            | (1 <<  6) // XCVR_EN, 10K PD
            | (1 <<  5) // RXENABLE
            | (1 <<  4) // TXENABLE
            | (1 <<  2) // LED2 (RX)
            | (1 <<  1) // LED1 (USB)
            | (1 <<  0) // HP
    },
    {   // GPIO3
        .data
            = (0 << 15) // PortaPack GPIO3_15(IO)
            | (0 << 14) // PortaPack GPIO3_14(IO)
            | (0 << 13) // PortaPack GPIO3_13(IO)
            | (0 << 12) // PortaPack GPIO3_12(IO)
            | (0 << 11) // PortaPack GPIO3_11(IO)
            | (0 << 10) // PortaPack GPIO3_10(IO)
            | (0 <<  9) // PortaPack GPIO3_9(IO)
            | (0 <<  8) // PortaPack GPIO3_8(IO)
            | (0 <<  7) // VREGMODE
            | (1 <<  6) // EN1V8, 10K PD
            | (1 <<  5) // !TX_AMP_PWR, 10K PU
            | (1 <<  4) // HackRF CPLD.TMS(I) (output only when needed, pull-up internal to CPLD when 1V8 present)
            | (1 <<  1) // HackRF CPLD.TDI(I), PortaPack I2S0_RX_SDA(O), PortaPack CPLD.TDI(I) (output only when needed, pull-up internal to CPLD when 1V8 present)
            | (1 <<  0) // HackRF CPLD.TCK(I), PortaPack CPLD.TCK(I) (output only when needed, pull-up internal to CPLD when 1V8 present)
            ,
        .dir
            = (0 << 15) // PortaPack GPIO3_15(IO)
            | (0 << 14) // PortaPack GPIO3_14(IO)
            | (0 << 13) // PortaPack GPIO3_13(IO)
            | (0 << 12) // PortaPack GPIO3_12(IO)
            | (0 << 11) // PortaPack GPIO3_11(IO)
            | (0 << 10) // PortaPack GPIO3_10(IO)
            | (0 <<  9) // PortaPack GPIO3_9(IO)
            | (0 <<  8) // PortaPack GPIO3_8(IO)
            | (1 <<  7) // VREGMODE
            | (1 <<  6) // EN1V8, 10K PD
            | (1 <<  5) // !TX_AMP_PWR, 10K PU
            | (0 <<  4) // HackRF CPLD.TMS(I) (output only when needed, pull-up internal to CPLD when 1V8 present)
            | (0 <<  1) // HackRF CPLD.TDI(I), PortaPack I2S0_RX_SDA(O), PortaPack CPLD.TDI(I) (output only when needed, pull-up internal to CPLD when 1V8 present)
            | (0 <<  0) // HackRF CPLD.TCK(I), PortaPack CPLD.TCK(I) (output only when needed, pull-up internal to CPLD when 1V8 present)
    },
    {   // GPIO4
            .data = 0,
            .dir = 0
    },
    {   // GPIO5
        .data
            = (1 << 18) // HackRF CPLD.TDO(O) (input with pull up)
            | (0 << 15) // TX
            | (1 << 16) // MIX_BYPASS
            | (1 <<  5) // RX
            | (1 <<  4) // PortaPack P2_4/LCD_RDX
            | (0 <<  3) // PortaPack P2_3/LCD_TE
            | (0 <<  1) // PortaPack P2_1/ADDR
            | (1 <<  0) // PortaPack P2_0/IO_STBX
            ,
        .dir
            = (0 << 18) // HackRF CPLD.TDO(O) (input with pull up)
            | (1 << 16) // MIX_BYPASS
            | (1 << 15) // TX
            | (1 <<  5) // RX
            | (1 <<  4) // PortaPack P2_4/LCD_RDX
            | (0 <<  3) // PortaPack P2_3/LCD_TE
            | (1 <<  1) // PortaPack P2_1/ADDR
            | (1 <<  0) // PortaPack P2_0/IO_STBX
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
    {  4,  7, .config={ .MODE=1, .EPD=0, .EPUN=1, .EHS=0, .EZI=1, .ZIF=1 } }, /* GP_CLKIN/P72/MCU_CLK: SI5351C.CLK7(O) */

    /* HackRF: LEDs. Configured early so we can use them to indicate boot status. */
    {  4,  1, .config={ .MODE=0, .EPD=0, .EPUN=1, .EHS=0, .EZI=0, .ZIF=0 } }, /* LED1: LED1.A(I) */
    {  4,  2, .config={ .MODE=0, .EPD=0, .EPUN=1, .EHS=0, .EZI=0, .ZIF=0 } }, /* LED2: LED2.A(I) */
    {  6, 12, .config={ .MODE=0, .EPD=0, .EPUN=1, .EHS=0, .EZI=0, .ZIF=0 } }, /* LED3: LED3.A(I) */

    /* Power control */
    {  5,  0, .config={ .MODE=0, .EPD=0, .EPUN=1, .EHS=0, .EZI=0, .ZIF=0 } }, /* !VAA_ENABLE: 10K PU, Q3.G(I), power to VAA */
    {  6, 10, .config={ .MODE=0, .EPD=0, .EPUN=1, .EHS=0, .EZI=0, .ZIF=0 } }, /* EN1V8/P70: 10K PD, TPS62410.EN2(I), 1V8LED.A(I) */
    {  6, 11, .config={ .MODE=0, .EPD=0, .EPUN=1, .EHS=0, .EZI=0, .ZIF=0 } }, /* VREGMODE/P69: TPS62410.MODE/DATA(I) */

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
        .config = {
            .word = // SFSI2C0
                  (1U <<  0)    // SCL: 3ns glitch
                | (0U <<  2)    // SCL: Standard/Fast mode
                | (1U <<  3)    // SCL: Input enabled
                | (0U <<  7)    // SCL: Enable input glitch filter
                | (1U <<  8)    // SDA: 3ns glitch
                | (0U << 10)    // SDA: Standard/Fast mode
                | (1U << 11)    // SDA: Input enabled
                | (0U << 15)    // SDA: Enable input glitch filter
        }
    },

    /* Radio section control */
    {  1,  3, .config={ .MODE=5, .EPD=0, .EPUN=1, .EHS=0, .EZI=1, .ZIF=0 } }, /* SSP1_MISO/P41: MAX2837.DOUT(O) */
    {  1,  4, .config={ .MODE=5, .EPD=1, .EPUN=1, .EHS=0, .EZI=0, .ZIF=0 } }, /* SSP1_MOSI/P40: MAX2837.DIN(I), MAX5864.DIN(I) */
    {  1,  7, .config={ .MODE=0, .EPD=0, .EPUN=1, .EHS=0, .EZI=0, .ZIF=0 } }, /* !MIX_BYPASS/P35: U1.VCTL1(I), U11.VCTL2(I), U9.V2(I) */
    {  1, 19, .config={ .MODE=1, .EPD=0, .EPUN=1, .EHS=0, .EZI=0, .ZIF=0 } }, /* SSP1_SCK/P39: MAX2837.SCLK(I), MAX5864.SCLK(I) */
    {  1, 20, .config={ .MODE=0, .EPD=0, .EPUN=1, .EHS=0, .EZI=0, .ZIF=0 } }, /* CS_XCVR/P53: MAX2837.CS(I) */
    {  2,  5, .config={ .MODE=4, .EPD=0, .EPUN=0, .EHS=0, .EZI=0, .ZIF=0 } }, /* RX/P43: U7.VCTL1(I), U10.VCTL1(I), U2.VCTL1(I) */
    {  2,  6, .config={ .MODE=4, .EPD=0, .EPUN=1, .EHS=0, .EZI=0, .ZIF=0 } }, /* MIXER_SCLK/P31: 33pF, RFFC5072.SCLK(I) */
    {  2, 10, .config={ .MODE=0, .EPD=0, .EPUN=1, .EHS=0, .EZI=0, .ZIF=0 } }, /* AMP_BYPASS/P50: U14.V2(I), U12.V2(I) */
    {  2, 11, .config={ .MODE=0, .EPD=0, .EPUN=1, .EHS=0, .EZI=0, .ZIF=0 } }, /* RX_AMP/P49: U12.V1(I), U14.V3(I) */
    {  2, 12, .config={ .MODE=0, .EPD=0, .EPUN=1, .EHS=0, .EZI=0, .ZIF=0 } }, /* !RX_AMP_PWR/P52: 10K PU, Q1.G(I), power to U13 (RX amp) */
    {  4,  0, .config={ .MODE=0, .EPD=0, .EPUN=1, .EHS=0, .EZI=0, .ZIF=0 } }, /* HP/P44: U6.VCTL1(I), U5.VCTL2(I) */
    {  4,  4, .config={ .MODE=0, .EPD=0, .EPUN=1, .EHS=0, .EZI=0, .ZIF=0 } }, /* TXENABLE/P55: MAX2837.TXENABLE(I) */
    {  4,  5, .config={ .MODE=0, .EPD=0, .EPUN=1, .EHS=0, .EZI=0, .ZIF=0 } }, /* RXENABLE/P56: MAX2837.RXENABLE(I) */
    {  4,  6, .config={ .MODE=0, .EPD=0, .EPUN=1, .EHS=0, .EZI=0, .ZIF=0 } }, /* XCVR_EN: 10K PD, MAX2837.ENABLE(I) */
    {  5,  1, .config={ .MODE=0, .EPD=0, .EPUN=1, .EHS=0, .EZI=0, .ZIF=0 } }, /* LP/P45: U6.VCTL2(I), U5.VCTL1(I) */
    {  5,  2, .config={ .MODE=0, .EPD=0, .EPUN=1, .EHS=0, .EZI=0, .ZIF=0 } }, /* TX_MIX_BP/P46: U9.V1(I) */
    {  5,  3, .config={ .MODE=0, .EPD=0, .EPUN=1, .EHS=0, .EZI=0, .ZIF=0 } }, /* RX_MIX_BP/P47: U9.V3(I) */
    {  5,  4, .config={ .MODE=0, .EPD=0, .EPUN=1, .EHS=0, .EZI=0, .ZIF=0 } }, /* MIXER_ENX/P32: 10K PU, 33pF, RFFC5072.ENX(I) */
    {  5,  5, .config={ .MODE=0, .EPD=0, .EPUN=1, .EHS=0, .EZI=0, .ZIF=0 } }, /* MIXER_RESETX/P33: 10K PU, 33pF, RFFC5072.RESETX(I) */
    {  5,  6, .config={ .MODE=0, .EPD=0, .EPUN=1, .EHS=0, .EZI=0, .ZIF=0 } }, /* TX_AMP/P48: U12.V3(I), U14.V1(I) */
    {  5,  7, .config={ .MODE=0, .EPD=0, .EPUN=1, .EHS=0, .EZI=0, .ZIF=0 } }, /* CS_AD/P54: MAX5864.CS(I) */
    {  6,  4, .config={ .MODE=0, .EPD=0, .EPUN=1, .EHS=0, .EZI=1, .ZIF=0 } }, /* MIXER_SDATA/P27: 33pF, RFFC5072.SDATA(IO) */
    {  6,  7, .config={ .MODE=4, .EPD=0, .EPUN=1, .EHS=0, .EZI=0, .ZIF=0 } }, /* TX/P42: U7.VCTL2(I), U10.VCTL2(I), U2.VCTL2(I) */
    {  6,  8, .config={ .MODE=4, .EPD=0, .EPUN=1, .EHS=0, .EZI=0, .ZIF=0 } }, /* MIX_BYPASS/P34: U1.VCTL2(I), U11.VCTL1(I) */
    {  6,  9, .config={ .MODE=0, .EPD=0, .EPUN=1, .EHS=0, .EZI=0, .ZIF=0 } }, /* !TX_AMP_PWR/P51: 10K PU, Q2.G(I), power to U25 (TX amp) */

    /* SGPIO for sample transfer interface to HackRF CPLD. */
    {  0,  0, .config={ .MODE=3, .EPD=0, .EPUN=1, .EHS=1, .EZI=1, .ZIF=1 } }, /* SGPIO0/P75/BANK2F3M3: CPLD.89/HOST_DATA0(IO) */
    {  0,  1, .config={ .MODE=3, .EPD=0, .EPUN=1, .EHS=1, .EZI=1, .ZIF=1 } }, /* SGPIO1/BANK2F3M5: CPLD.79/HOST_DATA1(IO) */
    {  1, 15, .config={ .MODE=2, .EPD=0, .EPUN=1, .EHS=1, .EZI=1, .ZIF=1 } }, /* SGPIO2/BANK2F3M9: CPLD.74/HOST_DATA2(IO) */
    {  1, 16, .config={ .MODE=2, .EPD=0, .EPUN=1, .EHS=1, .EZI=1, .ZIF=1 } }, /* SGPIO3/BANK2F3M10: CPLD.72/HOST_DATA3(IO) */
    {  6,  3, .config={ .MODE=2, .EPD=0, .EPUN=1, .EHS=1, .EZI=1, .ZIF=1 } }, /* SGPIO4/BANK2F3M14: CPLD.67/HOST_DATA4(IO) */
    {  6,  6, .config={ .MODE=2, .EPD=0, .EPUN=1, .EHS=1, .EZI=1, .ZIF=1 } }, /* SGPIO5/BANK2F3M15: CPLD.64/HOST_DATA5(IO) */
    {  2,  2, .config={ .MODE=0, .EPD=0, .EPUN=1, .EHS=1, .EZI=1, .ZIF=1 } }, /* SGPIO6/BANK2F3M16: CPLD.61/HOST_DATA6(IO) */
    {  1,  0, .config={ .MODE=6, .EPD=0, .EPUN=1, .EHS=1, .EZI=1, .ZIF=1 } }, /* SGPIO7/P76/BANK2F3M7: CPLD.77/HOST_DATA7(IO) */
    {  9,  6, .config={ .MODE=6, .EPD=0, .EPUN=1, .EHS=0, .EZI=1, .ZIF=1 } }, /* SGPIO8/SGPIO_CLK/P60: SI5351C.CLK2(O) */
    {  4,  3, .config={ .MODE=7, .EPD=0, .EPUN=1, .EHS=0, .EZI=1, .ZIF=1 } }, /* SGPIO9/P77/BANK2F3M1: CPLD.91/HOST_CAPTURE(O) */
    {  1, 14, .config={ .MODE=6, .EPD=0, .EPUN=0, .EHS=1, .EZI=0, .ZIF=0 } }, /* SGPIO10/P78/BANK2F3M8: CPLD.76/HOST_DISABLE(I) */
    {  1, 17, .config={ .MODE=6, .EPD=0, .EPUN=0, .EHS=1, .EZI=0, .ZIF=0 } }, /* SGPIO11/P79/BANK2F3M11: CPLD.71/HOST_DIRECTION(I) */
    {  1, 18, .config={ .MODE=0, .EPD=1, .EPUN=1, .EHS=0, .EZI=0, .ZIF=0 } }, /* SGPIO12/BANK2F3M12: CPLD.70/HOST_INVERT(I) */
    {  4,  8, .config={ .MODE=4, .EPD=0, .EPUN=0, .EHS=0, .EZI=0, .ZIF=0 } }, /* SGPIO13/BANK2F3M2: CPLD.90/HOST_DECIM_SEL0(I) */
    {  4,  9, .config={ .MODE=4, .EPD=0, .EPUN=0, .EHS=0, .EZI=0, .ZIF=0 } }, /* SGPIO14/BANK2F3M4: CPLD.81/HOST_DECIM_SEL1(I) */
    {  4, 10, .config={ .MODE=4, .EPD=0, .EPUN=0, .EHS=0, .EZI=0, .ZIF=0 } }, /* SGPIO15/BANK2F3M6: CPLD.78/HOST_DECIM_SEL2(I) */

    /* HackRF: CPLD */
    {  6,  1, .config={ .MODE=0, .EPD=0, .EPUN=1, .EHS=0, .EZI=0, .ZIF=0 } }, /* CPLD_TCK: CPLD.TCK(I), PortaPack CPLD.TCK(I) */
    {  6,  2, .config={ .MODE=0, .EPD=0, .EPUN=1, .EHS=0, .EZI=1, .ZIF=0 } }, /* CPLD_TDI: CPLD.TDI(I), PortaPack I2S0_RX_SDA(O), PortaPack CPLD.TDI(I) */
    {  6,  5, .config={ .MODE=0, .EPD=0, .EPUN=1, .EHS=0, .EZI=0, .ZIF=0 } }, /* CPLD_TMS: CPLD.TMS(I) */
    {  9,  5, .config={ .MODE=4, .EPD=0, .EPUN=1, .EHS=0, .EZI=1, .ZIF=0 } }, /* CPLD_TDO: CPLD.TDO(O) */

    /* PortaPack */
    {  1,  5, .config={ .MODE=0, .EPD=0, .EPUN=0, .EHS=0, .EZI=1, .ZIF=0 } }, /* SD_POW: PortaPack CPLD.TDO(O) */
    {  1,  8, .config={ .MODE=0, .EPD=0, .EPUN=0, .EHS=0, .EZI=0, .ZIF=0 } }, /* SD_VOLT0: PortaPack CPLD.TMS(I) */
    {  2,  0, .config={ .MODE=4, .EPD=0, .EPUN=1, .EHS=0, .EZI=1, .ZIF=0 } }, /* U0_TXD: PortaPack P2_0/IO_STBX */
    {  2,  1, .config={ .MODE=4, .EPD=0, .EPUN=1, .EHS=0, .EZI=1, .ZIF=0 } }, /* U0_RXD: PortaPack P2_1/ADDR */
    {  2,  3, .config={ .MODE=4, .EPD=0, .EPUN=1, .EHS=0, .EZI=1, .ZIF=0 } }, /* I2C1_SDA: PortaPack P2_3/LCD_TE */
    {  2,  4, .config={ .MODE=4, .EPD=0, .EPUN=1, .EHS=0, .EZI=1, .ZIF=0 } }, /* I2C1_SCL: PortaPack P2_4/LCD_RDX */
    {  2,  8, .config={ .MODE=4, .EPD=0, .EPUN=1, .EHS=0, .EZI=0, .ZIF=0 } }, /* P2_8: 10K PD, BOOT2, DFU switch, PortaPack P2_8/<unused> */
    {  2,  9, .config={ .MODE=0, .EPD=0, .EPUN=1, .EHS=0, .EZI=1, .ZIF=0 } }, /* P2_9: 10K PD, BOOT3, PortaPack P2_9/LCD_WRX */
    {  2, 13, .config={ .MODE=0, .EPD=0, .EPUN=1, .EHS=0, .EZI=1, .ZIF=0 } }, /* P2_13: PortaPack P2_13/DIR */
    {  7,  0, .config={ .MODE=0, .EPD=1, .EPUN=1, .EHS=0, .EZI=1, .ZIF=0 } }, /* GPIO3_8: PortaPack GPIO3_8(IO) */
    {  7,  1, .config={ .MODE=0, .EPD=1, .EPUN=1, .EHS=0, .EZI=1, .ZIF=0 } }, /* GPIO3_9: PortaPack GPIO3_9(IO) */
    {  7,  2, .config={ .MODE=0, .EPD=1, .EPUN=1, .EHS=0, .EZI=1, .ZIF=0 } }, /* GPIO3_10: PortaPack GPIO3_10(IO) */
    {  7,  3, .config={ .MODE=0, .EPD=1, .EPUN=1, .EHS=0, .EZI=1, .ZIF=0 } }, /* GPIO3_11: PortaPack GPIO3_11(IO) */
    {  7,  4, .config={ .MODE=0, .EPD=1, .EPUN=1, .EHS=0, .EZI=1, .ZIF=0 } }, /* GPIO3_12: PortaPack GPIO3_12(IO) */
    {  7,  5, .config={ .MODE=0, .EPD=1, .EPUN=1, .EHS=0, .EZI=1, .ZIF=0 } }, /* GPIO3_13: PortaPack GPIO3_13(IO) */
    {  7,  6, .config={ .MODE=0, .EPD=1, .EPUN=1, .EHS=0, .EZI=1, .ZIF=0 } }, /* GPIO3_14: PortaPack GPIO3_14(IO) */
    {  7,  7, .config={ .MODE=0, .EPD=1, .EPUN=1, .EHS=0, .EZI=1, .ZIF=0 } }, /* GPIO3_15: PortaPack GPIO3_15(IO) */

    /* PortaPack: Audio */
    {  3,  0, .config={ .MODE=2, .EPD=0, .EPUN=0, .EHS=0, .EZI=1, .ZIF=0 } }, /* I2S0_TX_SCK: PortaPack I2S0_TX_SCK(I) */
    {  3,  1, .config={ .MODE=0, .EPD=0, .EPUN=0, .EHS=0, .EZI=1, .ZIF=0 } }, /* I2S0_RX_WS: PortaPack I2S0_TX_WS(I). Input enabled to fold back into RX. */
    {  3,  2, .config={ .MODE=0, .EPD=0, .EPUN=0, .EHS=0, .EZI=0, .ZIF=0 } }, /* I2S0_RX_SDA: PortaPack I2S0_TX_SDA(I) */
    { 24,  2, .config={ .MODE=6, .EPD=1, .EPUN=1, .EHS=0, .EZI=0, .ZIF=0 } }, /* I2S0_TX_CLK: PortaPack I2S0_TX_MCLK */

    /* PortaPack: SD card socket */
    { 24,  0, .config={ .MODE=4, .EPD=1, .EPUN=1, .EHS=0, .EZI=1, .ZIF=1 } }, /* SD_CLK: PortaPack SD.CLK, enable input buffer for timing feedback? */
    {  1,  6, .config={ .MODE=7, .EPD=0, .EPUN=0, .EHS=0, .EZI=1, .ZIF=1 } }, /* SD_CMD: PortaPack SD.CMD(IO)  */
    {  1,  9, .config={ .MODE=7, .EPD=0, .EPUN=0, .EHS=0, .EZI=1, .ZIF=1 } }, /* SD_DAT0: PortaPack SD.DAT0(IO) */
    {  1, 10, .config={ .MODE=7, .EPD=0, .EPUN=0, .EHS=0, .EZI=1, .ZIF=1 } }, /* SD_DAT1: PortaPack SD.DAT1(IO) */
    {  1, 11, .config={ .MODE=7, .EPD=0, .EPUN=0, .EHS=0, .EZI=1, .ZIF=1 } }, /* SD_DAT2: PortaPack SD.DAT2(IO) */
    {  1, 12, .config={ .MODE=7, .EPD=0, .EPUN=0, .EHS=0, .EZI=1, .ZIF=1 } }, /* SD_DAT3: PortaPack SD.DAT3(IO) */
    {  1, 13, .config={ .MODE=7, .EPD=0, .EPUN=0, .EHS=0, .EZI=1, .ZIF=0 } }, /* SD_CD: PortaPack SD.CD(O) */

    /* Miscellaneous */
    {  1,  1, .config={ .MODE=0, .EPD=0, .EPUN=1, .EHS=0, .EZI=0, .ZIF=0 } }, /* P1_1/P74: 10K PU, BOOT0 */
    {  1,  2, .config={ .MODE=0, .EPD=0, .EPUN=1, .EHS=0, .EZI=0, .ZIF=0 } }, /* P1_2/P73: 10K PD, BOOT1 */
    {  2,  7, .config={ .MODE=0, .EPD=0, .EPUN=1, .EHS=0, .EZI=0, .ZIF=0 } }, /* ISP: 10K PU, Unused */
    {  6,  0, .config={ .MODE=0, .EPD=0, .EPUN=0, .EHS=0, .EZI=0, .ZIF=0 } }, /* I2S0_RX_MCLK: Unused */
    { 15,  4, .config={ .MODE=7, .EPD=0, .EPUN=0, .EHS=0, .EZI=0, .ZIF=0 } }, /* I2S0_RX_SCK: Unused */
  }
};
#endif

/**
 * @brief   Early initialization code.
 * @details This initialization must be performed just after stack setup
 *          and before any other initialization.
 */
// void __early_init(void) {
// }

/**
 * @brief   Board-specific initialization code.
 * @todo    Add your board-specific code, if any.
 */
void boardInit(void) {
}
