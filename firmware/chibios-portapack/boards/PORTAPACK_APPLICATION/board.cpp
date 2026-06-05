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

// Declare wrapper function. board.cpp to avoid conflicting gpio_t definitions.
bool hackrf_r9;

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
        {
            // GPIO0
            .data
#ifdef PRALINE
            = (0 << 7)     // P2_7:  Input GND
              | (0 << 8)   // P1_1:  Input GND
              | (1 << 9)   // P1_2:  Input VCC
              | (0 << 14)  // P2_10: P1_CTRL0 (Output GND)
              | (0 << 15)  // P1_20: CLKIN_CTRL - start low
#else
            = (1 << 14)    // P2_10: AMP_BYPASS
              | (1 << 15)  // P1_20: CS_XCVR
#endif
              | (1 << 11)  // P1_4:  SSP1_MOSI
              | (0 << 13)  // P1_18: SGPIO12, HOST_Q_INVERT
              | (0 << 12)  // P1_17: SGPIO11, HOST_DIRECTION, Praline: FPGA HOST_DIRECTION
              | (1 << 10)  // P1_3:  SSP1_MISO
              | (0 << 9)   // P1_2:  Varies by revision, float until detection
              | (0 << 8)   // P1_1:  Varies by revision, float until detection
              | (0 << 7)   // P2_7:  Varies by revision, float until detection
              | (0 << 6)   // P3_6:  SPIFI_MISO
              | (1 << 5)   // P6_6:  SGPIO5, HOST_DATA5, Praline: FPGA HOST_DATA5
              | (1 << 4)   // P1_0:  SGPIO7, HOST_DATA7, Praline: FPGA HOST_DATA7
              | (1 << 3)   // P1_16: SGPIO3, HOST_DATA3, Praline: FPGA HOST_DATA3
              | (1 << 2)   // P1_15: SGPIO2, HOST_DATA2, Praline: FPGA HOST_DATA2
              | (1 << 1)   // P0_1:  SGPIO1, HOST_DATA1, Praline: FPGA HOST_DATA1
              | (1 << 0)   // P0_0:  SGPIO0, HOST_DATA0, Praline: FPGA HOST_DATA0
            ,
            .dir
#ifdef PRALINE
            = (0 << 7)     // P2_7:  Input
              | (0 << 8)   // P1_1:  Input
              | (0 << 9)   // P1_2:  Input
              | (1 << 14)  // P2_10: P1_CTRL0
              | (1 << 15)  // P1_20: CLKIN_CTRL
#else
            = (1 << 14)    // P2_10: AMP_BYPASS
              | (1 << 15)  // P1_20: CS_XCVR
#endif
              | (0 << 11)  // P1_4:  SSP1_MOSI
              | (1 << 13)  // P1_18: SGPIO12, HOST_Q_INVERT
              | (0 << 12)  // P1_17: SGPIO11, HOST_DIRECTION
              | (0 << 10)  // P1_3:  SSP1_MISO
              | (0 << 9)   // P1_2:  Varies by revision, float until detection
              | (0 << 8)   // P1_1:  Varies by revision, float until detection
              | (0 << 7)   // P2_7:  Varies by revision, float until detection
              | (0 << 6)   // P3_6:  SPIFI_MISO
              | (0 << 5)   // P6_6:  SGPIO5, HOST_DATA5
              | (0 << 4)   // P1_0:  SGPIO7, HOST_DATA7
              | (0 << 3)   // P1_16: SGPIO3, HOST_DATA3
              | (0 << 2)   // P1_15: SGPIO2, HOST_DATA2
              | (0 << 1)   // P0_1:  SGPIO1, HOST_DATA1
              | (0 << 0)   // P0_0:  SGPIO0, HOST_DATA0
        },
        {
            // GPIO1
            .data = (1 << 15)    // P3_5:  SPIFI_SIO2
                    | (1 << 14)  // P3_4:  SPIFI_SIO3
                    | (1 << 13)  // P2_13: PortaPack DIR
#ifdef PRALINE
                    | (1 << 12)  // P2_12: BIAS_EN
                    | (0 << 11)  // P2_11: BIAS_OC
                    | (0 << 7)   // P1_14: AA_EN
                    | (0 << 0)   // P1_7:  Output GND
#else
                    | (1 << 12)  // P2_12: !RX_AMP_PWR
                    | (0 << 11)  // P2_11: RX_AMP
                    | (1 << 7)   // P1_14: SGPIO10, HOST_DISABLE
                    | (0 << 0)   // P1_7:  !MIX_BYPASS
#endif
                    | (0 << 10)  // P2_9:  10K PD, BOOT3, PortaPack LCD_WRX
                    | (1 << 9)   // P1_6:  SD_CMD
                    | (1 << 8)   // P1_5:  SD_POW, PortaPack CPLD.TDO(O)
                    | (1 << 6)   // P1_13: SD_CD
                    | (1 << 5)   // P1_12: SD_DAT3
                    | (1 << 4)   // P1_11: SD_DAT2
                    | (1 << 3)   // P1_10: SD_DAT1
                    | (1 << 2)   // P1_9:  SD_DAT0
                    | (1 << 1)   // P1_8:  PortaPack CPLD.TMS(I)
            ,
            .dir = (0 << 15)    // P3_5:  SPIFI_SIO2
                   | (0 << 14)  // P3_4:  SPIFI_SIO3
                   | (0 << 13)  // P2_13: PortaPack DIR
#ifdef PRALINE
                   | (1 << 12)  // P2_12: !BIAS_EN
                   | (0 << 11)  // P2_11: BIAS_OC
                   | (1 << 7)   // P1_14: AA_EN
                   | (1 << 0)   // P1_7:  Output
#else
                   | (1 << 12)  // P2_12: !RX_AMP_PWR
                   | (1 << 11)  // P2_11: RX_AMP
                   | (0 << 7)   // P1_14: SGPIO10, HOST_DISABLE
                   | (1 << 0)   // P1_7:  !MIX_BYPASS
#endif
                   | (0 << 10)  // P2_9:  10K PD, BOOT3, PortaPack LCD_WRX
                   | (0 << 9)   // P1_6:  SD_CMD
                   | (0 << 8)   // P1_5:  SD_POW, PortaPack CPLD.TDO(O)
                   | (0 << 6)   // P1_13: SD_CD
                   | (0 << 5)   // P1_12: SD_DAT3
                   | (0 << 4)   // P1_11: SD_DAT2
                   | (0 << 3)   // P1_10: SD_DAT1
                   | (0 << 2)   // P1_9:  SD_DAT0
                   | (0 << 1)   // P1_8:  PortaPack CPLD.TMS(I)
        },
        {
            // GPIO2
            .data = (0 << 15)    // P5_6:  TX_AMP, unused on PRALINE
                    | (1 << 14)  // P5_5:  MIXER_RESETX, 10K PU
#ifdef PRALINE
                    | (1 << 13)  // P5_4:  RFFC5072 ENX
                    | (0 << 12)  // P5_3:  unused on PRALINE
                    | (0 << 11)  // P5_2:  FPGA_CRESET
                    | (1 << 10)  // P5_1:  FPGA_SPI_CS
                    | (0 << 4)   // P4_4:  unused on PRALINE
                    | (0 << 0)   // P4_0:  unused on PRALINE
                    | (0 << 9)   // P5_0:  unused on PRALINE
                    | (0 << 3)   // P4_3:  VBUSCTRL input GND
                    | (1 << 6)   // P4_6:  Input VCC (10K PU)
#else
                    | (1 << 13)  // P5_4:  MIXER_ENX, 10K PU
                    | (1 << 12)  // P5_3:  RX_MIX_BP
                    | (0 << 11)  // P5_2:  TX_MIX_BP
                    | (0 << 10)  // P5_1:  LP
                    | (0 << 4)   // P4_4:  Varies by revision
                    | (0 << 9)   // P5_0:  Varies by revision
                    | (1 << 0)   // P4_0:  HP
                    | (1 << 3)   // P4_3:  SGPIO9, HOST_CAPTURE
                    | (0 << 6)   // P4_6:  XCVR_EN, 10K PD
#endif
                    | (0 << 8)  // P6_12: LED3 (TX)
                    | (1 << 7)  // P5_7:  CS_AD
                    | (0 << 5)  // P4_5:  RXENABLE
                    | (0 << 2)  // P4_2:  LED2 (RX)
                    | (0 << 1)  // P4_1:  LED1 (USB)
            ,
            .dir = (1 << 15)  // P5_6:  TX_AMP, unused on PRALINE
#ifdef PRALINE
                   | (1 << 13)  // P5_4:  RFFC5072 ENX
                   | (1 << 12)  // P5_3:  unused on PRALINE
                   | (1 << 11)  // P5_2:  FPGA_CRESET
                   | (1 << 10)  // P5_1:  FPGA_SPI_CS
                   | (1 << 4)   // P4_4:  unused on PRALINE
                   | (1 << 9)   // P5_0:  unused on PRALINE
                   | (1 << 0)   // P4_0:  unused on PRALINE
                   | (0 << 3)   // P4_3:  VBUSCTRL input
                   | (0 << 6)   // P4_6:  Input
#else
                   | (1 << 13)  // P5_4:  MIXER_ENX, 10K PU
                   | (1 << 12)  // P5_3:  RX_MIX_BP
                   | (1 << 11)  // P5_2:  TX_MIX_BP
                   | (1 << 10)  // P5_1:  LP
                   | (0 << 4)   // P4_4:  Varies by revision
                   | (1 << 9)   // P5_0:  Varies by revision
                   | (1 << 0)   // P4_0:  HP
                   | (0 << 3)   // P4_3:  SGPIO9, HOST_CAPTURE
                   | (1 << 6)   // P4_6:  XCVR_EN, 10K PD
#endif
                   | (1 << 14)  // P5_5:  MIXER_RESETX, 10K PU
                   | (1 << 8)   // P6_12: LED3 (TX)
                   | (1 << 7)   // P5_7:  CS_AD
                   | (1 << 5)   // P4_5:  RXENABLE
                   | (1 << 2)   // P4_2:  LED2 (RX)
                   | (1 << 1)   // P4_1:  LED1 (USB)
        },
        {
            // GPIO3
            .data = (1 << 15)    // P7_7:  PortaPack GPIO3_15(IO)
                    | (1 << 14)  // P7_6:  PortaPack GPIO3_14(IO)
                    | (1 << 13)  // P7_5:  PortaPack GPIO3_13(IO)
                    | (1 << 12)  // P7_4:  PortaPack GPIO3_12(IO)
                    | (1 << 11)  // P7_3:  PortaPack GPIO3_11(IO)
                    | (1 << 10)  // P7_2:  PortaPack GPIO3_10(IO)
                    | (1 << 9)   // P7_1:  PortaPack GPIO3_9(IO)
                    | (1 << 8)   // P7_0:  PortaPack GPIO3_8(IO)
#ifdef PRALINE
                    | (0 << 4)  // P6_5:  TX_ENABLE
                    | (1 << 2)  // P6_3:  MIX_ENABLE_N
                    | (0 << 6)  // P6_10: unused on PRALINE
                    | (1 << 5)  // P6_9:  P1_CTRL2
#else
                    | (1 << 7)  // P6_11: VREGMODE
                    | (0 << 6)  // P6_10: Varies by revision
                    | (1 << 2)  // P6_3:  SGPIO4, HOST_DATA4
                    | (1 << 5)  // P6_9:  !TX_AMP_PWR, 10K PU
#endif
                    | (1 << 4)  // P6_5:  HackRF CPLD.TMS(I)
                    | (1 << 3)  // P6_4:  MIXER_SDATA
                    | (1 << 1)  // P6_2:  HackRF CPLD.TDI(I)
                    | (1 << 0)  // P6_1:  HackRF CPLD.TCK(I)
            ,
            .dir = (0 << 15)    // P7_7:  PortaPack GPIO3_15(IO)
                   | (0 << 14)  // P7_6:  PortaPack GPIO3_14(IO)
                   | (0 << 13)  // P7_5:  PortaPack GPIO3_13(IO)
                   | (0 << 12)  // P7_4:  PortaPack GPIO3_12(IO)
                   | (0 << 11)  // P7_3:  PortaPack GPIO3_11(IO)
                   | (0 << 10)  // P7_2:  PortaPack GPIO3_10(IO)
                   | (0 << 9)   // P7_1:  PortaPack GPIO3_9(IO)
                   | (0 << 8)   // P7_0:  PortaPack GPIO3_8(IO)
#ifdef PRALINE
                   | (0 << 7)  // P6_11: 3V3AUX_OC
                   | (1 << 4)  // P6_5:  TX_ENABLE
                   | (1 << 2)  // P6_3:  MIX_ENABLE_N
                   | (0 << 6)  // P6_10: unused on PRALINE
                   | (1 << 5)  // P6_9:  P1_CTRL2
#else
                   | (1 << 7)  // P6_11: VREGMODE
                   | (0 << 6)  // P6_10: Varies by revision
                   | (0 << 2)  // P6_3:  SGPIO4, HOST_DATA4
                   | (1 << 5)  // P6_9:  !TX_AMP_PWR, 10K PU
#endif
                   | (0 << 4)  // P6_5:  HackRF CPLD.TMS(I)
                   | (0 << 3)  // P6_4:  MIXER_SDATA
                   | (0 << 1)  // P6_2:  HackRF CPLD.TDI(I)
                   | (0 << 0)  // P6_1:  HackRF CPLD.TCK(I)
        },
        {
            // GPIO4
            .data =
#ifdef PRALINE
                (0 << 9)     // PA_2: RF_AMP_EN
                | (0 << 8)   // PA_1: LPF_EN
                | (0 << 7)   // P8_7: 1V2_EN
                | (1 << 6)   // P8_6: LED4
                | (0 << 5)   // P8_5: VIN_IN_EN
                | (0 << 4)   // P8_4: VBUS_IN_EN
                | (1 << 1)   // P8_1: VAA_EN
                | (0 << 10)  // PA_3: Output GND
                | (0 << 11)  // P9_6: MAX2831 LD
                | (0 << 12)  // P9_0: RF5072 enable
                | (0 << 13)  // P9_1: Output GND
                | (0 << 14)  // P9_2: RFFC5072 SDATA
                | (0 << 3)   // P8_3: Output GND
#else
                (1 << 11)  // P9_6: SGPIO8, SGPIO_CLK, HackRF MAX2831 LD
#endif
            ,
            .dir =
#ifdef PRALINE
                (1 << 9)     // PA_2: RF_AMP_EN
                | (1 << 8)   // PA_1: LPF_EN
                | (1 << 7)   // P8_7: 1V2_EN
                | (1 << 6)   // P8_6: LED4
                | (1 << 5)   // P8_5: VIN_IN_EN
                | (1 << 4)   // P8_4: VBUS_IN_EN
                | (1 << 1)   // P8_1: VAA_EN
                | (1 << 10)  // PA_3: Output
                | (0 << 11)  // P9_6: MAX2831 LD
                | (1 << 12)  // P9_0: RF5072 enable
                | (1 << 13)  // P9_1: Output
                | (0 << 14)  // P9_2: RFFC5072 SDATA (Bidirectional)
                | (1 << 3)   // P8_3: Output
#else
                (1 << 11)  // P9_6: SGPIO8, SGPIO_CLK, HackRF MAX2831 LD
#endif
        },
        {
            // GPIO5
            .data =
#ifdef PRALINE
                (0 << 18)    // P9_5: RFF5072 SCLK
                | (0 << 6)   // P2_6: Trigger out
                | (0 << 14)  // P4_10: FPGA CDONE
                | (1 << 15)  // P6_7: 3V3 AUX_ENABLE
                | (0 << 16)  // P6_8: P1_CTRL1
                | (0 << 17)  // P9_4: FPGA SGPIO
                | (0 << 19)  // PA_4: Output GND
                | (0 << 21)  // PB_1: SPI FLASH SCK
                | (0 << 22)  // PB_2: Unused IN
                | (0 << 23)  // PB_3: Output GND
                | (0 << 24)  // PB_4: Unused IN
                | (0 << 25)  // PB_5: Output GND
                | (0 << 5)   // P2_5: PPS OUT/IN
                | (0 << 12)  // P4_8: Output GND
                | (0 << 13)  // P4_9: TPS62410 VREGMODE
#else
                (1 << 18)    // P9_5: HackRF CPLD.TDO(O)
                | (0 << 6)   // P2_6: MIXER_SCLK
                | (1 << 14)  // P4_10: SGPIO15, CPLD (unused)
                | (0 << 15)  // P6_7: Varies by revision
                | (0 << 5)   // P2_5: Varies by revision
                | (1 << 16)  // P6_8: MIX_BYPASS
                | (1 << 13)  // P4_9: SGPIO14, CPLD (unused)
                | (0 << 12)  // P4_8: Varies by revision
#endif
                | (1 << 11)  // P3_8: SPIFI_CS
                | (1 << 10)  // P3_7: SPIFI_MOSI
                | (1 << 9)   // P3_2: I2S0_RX_SDA
                | (1 << 8)   // P3_1: I2S0_RX_WS
                | (0 << 7)   // P2_8: BOOT2
                | (1 << 4)   // P2_4: PortaPack LCD_RDX
                | (0 << 3)   // P2_3: PortaPack LCD_TE
                | (1 << 2)   // P2_2: SGPIO6, HOST_DATA6
                | (0 << 1)   // P2_1: PortaPack ADDR
                | (1 << 0)   // P2_0: PortaPack IO_STBX
            ,
            .dir =
#ifdef PRALINE
                (1 << 18)    // P9_5: RFF5072 SCLK
                | (1 << 6)   // P2_6: MIXER_SCLK
                | (0 << 14)  // P4_10: FPGA CDONE
                | (1 << 15)  // P6_7: 3V3 AUX_ENABLE
                | (1 << 16)  // P6_8: P1_CTRL1
                | (0 << 17)  // P9_4: FPGA SGPIO
                | (1 << 19)  // PA_4: Output
                | (1 << 21)  // PB_1: SPI FLASH SCK
                | (0 << 22)  // PB_2: Unused
                | (1 << 23)  // PB_3: Output
                | (0 << 24)  // PB_4: Unused
                | (1 << 25)  // PB_5: Output
                | (0 << 5)   // P2_5: PPS (Bidirectional)
                | (1 << 12)  // P4_8: Output
                | (1 << 13)  // P4_9: TPS62410 VREGMODE
#else
                (0 << 18)    // P9_5: HackRF CPLD.TDO(O)
                | (1 << 6)   // P2_6: MIXER_SCLK
                | (0 << 14)  // P4_10: SGPIO15, CPLD
                | (0 << 15)  // P6_7: Varies by revision
                | (0 << 5)   // P2_5: Varies by revision
                | (1 << 16)  // P6_8: MIX_BYPASS
                | (0 << 13)  // P4_9: SGPIO14, CPLD
                | (0 << 12)  // P4_8: Varies by revision
#endif
                | (0 << 11)  // P3_8: SPIFI_CS
                | (0 << 10)  // P3_7: SPIFI_MOSI
                | (0 << 9)   // P3_2: I2S0_RX_SDA
                | (0 << 8)   // P3_1: I2S0_RX_WS
                | (0 << 7)   // P2_8: BOOT2
                | (0 << 4)   // P2_4: PortaPack LCD_RDX
                | (0 << 3)   // P2_3: PortaPack LCD_TE
                | (0 << 2)   // P2_2: SGPIO6, HOST_DATA6
                | (0 << 1)   // P2_1: PortaPack ADDR
                | (0 << 0)   // P2_0: PortaPack IO_STBX
        },
        {
// GPIO6
#ifdef PRALINE
            .data = (1 << 28)    // PD_14: MAX2831 chip select
                    | (0 << 29)  // PD_15: MAX2831 RXHP control RXHP low = 100 Hz HPF
                    | (1 << 30)  // PD_16: MAX5864 chip select
                    | (1 << 25)  // PD_11: RFFC5072 Lock Detect
                    | (0 << 26)  // PD_12: TRIGGER IN
            ,
            .dir = (1 << 28)    // PD_14: MAX2831 chip select
                   | (1 << 29)  // PD_15: MAX2831 RXHP control
                   | (1 << 30)  // PD_16: MAX5864 chip select
                   | (0 << 25)  // PD_11: RFFC5072 Lock Detect
                   | (0 << 26)  // PD_12: TRIGGER IN
#else
            .data = 0,
            .dir = 0
#endif
        },
        {
// GPIO7
#ifdef PRALINE
            .data = (0 << 0)    // PE_0: Output
                    | (0 << 1)  // PE_1: MAX2831 !SHDN
                    | (0 << 2)  // PE_2: MAX2831 RXTX
                    | (0 << 3)  // PE_3: P2_ctrl0
                    | (0 << 4)  // PE_4: P2_ctrl1
            ,
            .dir = (1 << 0)    // PE_0: Output
                   | (1 << 1)  // PE_1: MAX2831 !SHDN
                   | (1 << 2)  // PE_2: MAX2831 RXTX
                   | (1 << 3)  // PE_3: P2_ctrl0
                   | (1 << 4)  // PE_4: P2_ctrl1
#else
            .data = 0,
            .dir = 0
#endif
        },
    },
    .SCU = {

        // SClock LEDs

        {4, 7, scu_config_normal_drive_t{.mode = 1, .epd = 0, .epun = 0, .ehs = 0, .ezi = 1, .zif = 1}},   // GP_CLKIN: SI5351C.CLK7(O)
        {4, 1, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 0, .ehs = 0, .ezi = 0, .zif = 0}},   // LED1: USB
        {4, 2, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 0, .ehs = 0, .ezi = 0, .zif = 0}},   // LED2: RX
        {6, 12, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 0, .ehs = 0, .ezi = 0, .zif = 0}},  // LED3: TX
#ifdef PRALINE
        {8, 6, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 0, .ehs = 0, .ezi = 0, .zif = 0}},  // LED4: PRALINE Custom
#endif

        // POWER MANAGEMENT (Regulators, Bias, Current Limits)

        {6, 11, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},  // VREGMODE: TPS62410
#ifdef PRALINE
        {8, 1, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},   // P8_1: VAA_EN
        {8, 4, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},   // P8_4: VBUS_IN_EN
        {8, 5, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},   // P8_5: VIN_IN_EN
        {8, 7, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},   // P8_7: 1V2_EN
        {6, 7, scu_config_normal_drive_t{.mode = 4, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},   // P6_7: 3.3V Aux Enable
        {4, 9, scu_config_normal_drive_t{.mode = 4, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},   // P4_9: TPS62410 mode
        {2, 10, scu_config_normal_drive_t{.mode = 0, .epd = 1, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},  // P2_10: P1_CTRL0
        {6, 9, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 0, .ehs = 0, .ezi = 0, .zif = 0}},   // P6_9: P1_CTRL2 (Output VCC, PU ON,)
        {14, 3, scu_config_normal_drive_t{.mode = 4, .epd = 1, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},  // PE_3: P2_CTRL0
        {14, 4, scu_config_normal_drive_t{.mode = 4, .epd = 1, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},  // PE_4: P2_CTRL1
        {4, 3, scu_config_normal_drive_t{.mode = 0, .epd = 1, .epun = 1, .ehs = 0, .ezi = 1, .zif = 0}},   // P4_3: VBUSCTRL Input GND
#endif

        /* HackRF: I2C0 */
        /* Glitch filter operates at 3ns instead of 50ns due to the WM8731
         * returning an ACK very fast (170ns) and confusing the I2C state
         * machine into thinking there was a bus error. It looks like the
         * MCU sees SDA fall before SCL falls, indicating a START at the
         * point an ACK is expected. With the glitch filter off or set to
         * 3ns, it's probably still a bit tight timing-wise, but improves
         * reliability on some problem units.
         */
        {25, 1,
         scu_config_sfsi2c0_t{
             .scl_efp = 1,  // SCL: 3ns glitch
             .scl_ehd = 0,  // SCL: Standard/Fast mode
             .scl_ezi = 1,  // SCL: Input enabled
             .scl_zif = 0,  // SCL: Enable input glitch filter
             .sda_efp = 1,  // SDA: 3ns glitch
             .sda_ehd = 0,  // SDA: Standard/Fast mode
             .sda_ezi = 1,  // SDA: Input enabled
             .sda_zif = 0   // SDA: Enable input glitch filter
         }},
        // FPGA & HIGH-SPEED DATA (SGPIO, Config, Triggers)

        {0, 0, scu_config_normal_drive_t{.mode = 3, .epd = 0, .epun = 1, .ehs = 1, .ezi = 1, .zif = 1}},   // SGPIO0: HOST_DATA0(IO)
        {0, 1, scu_config_normal_drive_t{.mode = 3, .epd = 0, .epun = 1, .ehs = 1, .ezi = 1, .zif = 1}},   // SGPIO1: HOST_DATA1(IO)
        {1, 15, scu_config_normal_drive_t{.mode = 2, .epd = 0, .epun = 1, .ehs = 1, .ezi = 1, .zif = 1}},  // SGPIO2: HOST_DATA2(IO)
        {1, 16, scu_config_normal_drive_t{.mode = 2, .epd = 0, .epun = 1, .ehs = 1, .ezi = 1, .zif = 1}},  // SGPIO3: HOST_DATA3(IO)
        {6, 6, scu_config_normal_drive_t{.mode = 2, .epd = 0, .epun = 1, .ehs = 1, .ezi = 1, .zif = 1}},   // SGPIO5: HOST_DATA5(IO)
        {2, 2, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 1, .ezi = 1, .zif = 1}},   // SGPIO6: HOST_DATA6(IO)
        {1, 0, scu_config_normal_drive_t{.mode = 6, .epd = 0, .epun = 1, .ehs = 1, .ezi = 1, .zif = 1}},   // SGPIO7: HOST_DATA7(IO)
        {1, 18, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},  // SGPIO12: HOST_INVERT(I)
#ifdef PRALINE
        {8, 0, scu_config_normal_drive_t{.mode = 4, .epd = 0, .epun = 1, .ehs = 1, .ezi = 1, .zif = 1}},    // SGPIO8: P8_0 func 4 (CLK)
        {8, 2, scu_config_normal_drive_t{.mode = 4, .epd = 0, .epun = 1, .ehs = 1, .ezi = 1, .zif = 1}},    // SGPIO10: P8_2 func 4 (DISABLE)
        {9, 3, scu_config_normal_drive_t{.mode = 6, .epd = 0, .epun = 1, .ehs = 1, .ezi = 1, .zif = 1}},    // SGPIO9: P9_3 func 6 (CAPTURE)
        {9, 4, scu_config_normal_drive_t{.mode = 6, .epd = 0, .epun = 1, .ehs = 1, .ezi = 1, .zif = 1}},    // SGPIO4: P9_4 func 6
        {1, 17, scu_config_normal_drive_t{.mode = 6, .epd = 0, .epun = 1, .ehs = 1, .ezi = 1, .zif = 1}},   // SGPIO11: P1_17 func 6 (DIRECTION)
        {6, 4, scu_config_normal_drive_t{.mode = 1, .epd = 0, .epun = 1, .ehs = 1, .ezi = 1, .zif = 1}},    // P6_4: SCT_CLK IN (Fast clock input, Mode 1)
        {4, 10, scu_config_normal_drive_t{.mode = 7, .epd = 1, .epun = 1, .ehs = 0, .ezi = 1, .zif = 0}},   // P4_10: FPGA Config Done (Input GND)
        {13, 12, scu_config_normal_drive_t{.mode = 4, .epd = 1, .epun = 1, .ehs = 0, .ezi = 1, .zif = 0}},  // PD_12: TRIGGER IN (Input GND, Mode 4)
        {2, 6, scu_config_normal_drive_t{.mode = 4, .epd = 0, .epun = 1, .ehs = 1, .ezi = 0, .zif = 0}},    // P2_6: TRIGGER_OUT
        {2, 5, scu_config_normal_drive_t{.mode = 4, .epd = 1, .epun = 1, .ehs = 0, .ezi = 1, .zif = 0}},    // P2_5: PPS OUT/IN (Mode 4 per Suppl. Data)
#else
        ///////////////ezeket át kell nézni mert a kommentek nem ülnek
        {9, 6, scu_config_normal_drive_t{.mode = 6, .epd = 0, .epun = 0, .ehs = 0, .ezi = 1, .zif = 1}},   // SGPIO8: SI5351C.CLK2(O)
        {4, 3, scu_config_normal_drive_t{.mode = 7, .epd = 0, .epun = 1, .ehs = 0, .ezi = 1, .zif = 1}},   // SGPIO9: HOST_CAPTURE(O)
        {1, 14, scu_config_normal_drive_t{.mode = 6, .epd = 0, .epun = 0, .ehs = 1, .ezi = 0, .zif = 0}},  // SGPIO10: HOST_DISABLE(I)
        {1, 17, scu_config_normal_drive_t{.mode = 6, .epd = 1, .epun = 1, .ehs = 1, .ezi = 0, .zif = 0}},  // SGPIO11: HOST_DIRECTION(I)
        {6, 3, scu_config_normal_drive_t{.mode = 2, .epd = 0, .epun = 1, .ehs = 1, .ezi = 1, .zif = 1}},   // SGPIO4: HOST_DATA4(IO)
        {4, 9, scu_config_normal_drive_t{.mode = 7, .epd = 0, .epun = 0, .ehs = 0, .ezi = 0, .zif = 0}},   // SGPIO14/BANK2F3M4: CPLD_P81
        {4, 10, scu_config_normal_drive_t{.mode = 7, .epd = 0, .epun = 0, .ehs = 0, .ezi = 0, .zif = 0}},  // SGPIO15/BANK2F3M6: CPLD_P78
        {2, 6, scu_config_normal_drive_t{.mode = 4, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 1}},   // MIXER_SCLK/P31: 33pF, RFFC5072.SCLK(I)
        {4, 5, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},   // RXENABLE
        {4, 6, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},   // XCVR_EN: 10K PD
#endif

        // RADIO & RF PATH (MAX2831, RFFC5072, Mixers, Switches, Amps)

        {1, 3, scu_config_normal_drive_t{.mode = 1, .epd = 0, .epun = 0, .ehs = 0, .ezi = 1, .zif = 0}},   // P1_3 SSP1_MISO: MAX2837.DOUT(O)
        {1, 4, scu_config_normal_drive_t{.mode = 1, .epd = 0, .epun = 0, .ehs = 0, .ezi = 0, .zif = 0}},   // P1_4 SSP1_MOSI: MAX2837.DIN(I)
        {1, 19, scu_config_normal_drive_t{.mode = 1, .epd = 0, .epun = 0, .ehs = 0, .ezi = 1, .zif = 1}},  // P1_19 SSP1_SCK: MAX2837.SCLK(I)
        {1, 20, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},  // P1_20 CS_XCVR: MAX2837.CS(I)
        {2, 11, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},  // P2_11 RX_AMP
        {2, 12, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},  // P2_12 !RX_AMP_PWR
        {4, 0, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},   // P4_0 HP
        {5, 1, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},   // P5_1 LP
        {5, 2, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},   // P5_2 TX_MIX_BP
        {5, 3, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},   // P5_3 RX_MIX_BP
        {5, 6, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},   // P5_6 TX_AMP
        {5, 7, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},   // P5_7 CS_AD, PRALINE: RFFC5072 CS
        {6, 8, scu_config_normal_drive_t{.mode = 4, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},   // P6_8 MIX_BYPASS, PRALINE: P1_CTRL1 (Output GND, Mode 4)

#ifdef PRALINE
        {5, 4, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 0, .ehs = 0, .ezi = 0, .zif = 0}},    // P5_4: RFFC ENX
        {5, 5, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 0, .ehs = 0, .ezi = 0, .zif = 0}},    // P5_5: RFFC RESETX
        {9, 5, scu_config_normal_drive_t{.mode = 4, .epd = 0, .epun = 1, .ehs = 1, .ezi = 1, .zif = 1}},    // P9_5: RFFC5072 SCLK
        {9, 2, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 0, .ehs = 1, .ezi = 1, .zif = 0}},    // P9_2: RFFC5072 DATA (Bidirectional)
        {13, 11, scu_config_normal_drive_t{.mode = 4, .epd = 0, .epun = 0, .ehs = 0, .ezi = 1, .zif = 0}},  // PD_11: RFFC Lock Detect
        {13, 14, scu_config_normal_drive_t{.mode = 4, .epd = 0, .epun = 1, .ehs = 1, .ezi = 0, .zif = 0}},  // PD_14: MAX2831 CS
        {13, 15, scu_config_normal_drive_t{.mode = 4, .epd = 0, .epun = 1, .ehs = 1, .ezi = 1, .zif = 1}},  // PD_15: MAX2831 RXHP
        {13, 16, scu_config_normal_drive_t{.mode = 4, .epd = 0, .epun = 1, .ehs = 1, .ezi = 0, .zif = 0}},  // PD_16: MAX5864 CS

        {14, 1, scu_config_normal_drive_t{.mode = 4, .epd = 0, .epun = 1, .ehs = 1, .ezi = 0, .zif = 0}},  // PE_1: MAX2831 !SHDN
        {14, 2, scu_config_normal_drive_t{.mode = 4, .epd = 0, .epun = 1, .ehs = 1, .ezi = 1, .zif = 1}},  // PE_2: MAX2831 RXTX
        {6, 3, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 0, .ehs = 1, .ezi = 0, .zif = 0}},   // P6_3: MIX_ENABLE_N
        {1, 14, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 1, .ezi = 1, .zif = 0}},  // P1_14: AA_EN
        {9, 0, scu_config_normal_drive_t{.mode = 0, .epd = 1, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},   // P9_0: RF5072 MIX EN
        {9, 6, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 0, .ehs = 0, .ezi = 0, .zif = 0}},   // P9_6: MAX2831 LD Input
        {6, 5, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 1, .ezi = 0, .zif = 0}},   // P6_5: TX enable
        {10, 1, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 1, .ezi = 0, .zif = 0}},  // PA_1: LPF enable
        {10, 2, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 1, .ezi = 0, .zif = 0}},  // PA_2: RF amp enable
#else
        {5, 4, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},   // MIXER_ENX
        {5, 5, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},   // MIXER_RESETX
        {6, 4, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 0, .ehs = 0, .ezi = 1, .zif = 0}},   // MIXER_SDATA
        {1, 7, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},   // !MIX_BYPASS
        {2, 10, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},  // AMP_BYPASS
        {6, 9, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},   // !TX_AMP_PWR
#endif

// SAFE TERMINATION (Unused pins grounded to prevent noise & save power)

#ifdef PRALINE
        {1, 1, scu_config_normal_drive_t{.mode = 0, .epd = 1, .epun = 1, .ehs = 0, .ezi = 1, .zif = 0}},   // P1_1: Input GND
        {2, 7, scu_config_normal_drive_t{.mode = 0, .epd = 1, .epun = 1, .ehs = 0, .ezi = 1, .zif = 0}},   // P2_7: Input GND
        {1, 2, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 0, .ehs = 0, .ezi = 1, .zif = 0}},   // P1_2: Input VCC
        {4, 6, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 0, .ehs = 0, .ezi = 1, .zif = 0}},   // P4_6: Input VCC
        {6, 10, scu_config_normal_drive_t{.mode = 0, .epd = 1, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},  // P6_10: Output GND
        {1, 7, scu_config_normal_drive_t{.mode = 0, .epd = 1, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},   // P1_7: Output GND
        {4, 8, scu_config_normal_drive_t{.mode = 4, .epd = 1, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},   // P4_8: Output GND
        {9, 1, scu_config_normal_drive_t{.mode = 4, .epd = 1, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},   // P9_1: Output GND
        {10, 3, scu_config_normal_drive_t{.mode = 0, .epd = 1, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},  // PA_3: Output GND
        {10, 4, scu_config_normal_drive_t{.mode = 0, .epd = 1, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},  // PA_4: Output GND
        {11, 3, scu_config_normal_drive_t{.mode = 4, .epd = 1, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},  // PB_3: Output GND
        {11, 5, scu_config_normal_drive_t{.mode = 4, .epd = 1, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},  // PB_5: Output GND
        {14, 0, scu_config_normal_drive_t{.mode = 4, .epd = 1, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},  // PE_0: Output GND
        {8, 3, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},   // P8_3: Output GND
        {8, 8, scu_config_normal_drive_t{.mode = 0, .epd = 1, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},   // P8_8: Unused
        {10, 0, scu_config_normal_drive_t{.mode = 0, .epd = 1, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},  // PA_0: Unused
        {11, 2, scu_config_normal_drive_t{.mode = 4, .epd = 1, .epun = 1, .ehs = 0, .ezi = 1, .zif = 0}},  // PB_2: Unused IN
        {11, 4, scu_config_normal_drive_t{.mode = 4, .epd = 1, .epun = 1, .ehs = 0, .ezi = 1, .zif = 0}},  // PB_4: Unused IN
#else
        {1, 1, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},  // P1_1
        {1, 2, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},  // P1_2
        {2, 5, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},  // P2_5
        {2, 7, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},  // P2_7
        {4, 8, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},  // P4_8
#endif

        // PORTAPACK SPECIFIC & JTAG (CPLD, UI, Audio, SD)

        {6, 0, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 0, .ehs = 0, .ezi = 0, .zif = 0}},  // I2S0_RX_MCLK: Unused

#ifndef PRALINE
        {9, 5, scu_config_normal_drive_t{.mode = 4, .epd = 0, .epun = 0, .ehs = 0, .ezi = 1, .zif = 0}},   // CPLD_TDO: HackRF CPLD.TDO(O)
        {6, 5, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},   // CPLD_TMS: HackRF CPLD.TMS(I)
        {15, 4, scu_config_normal_drive_t{.mode = 7, .epd = 0, .epun = 0, .ehs = 0, .ezi = 0, .zif = 0}},  // I2S0_RX_SCK: Unused
#endif
        {1, 5, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 0, .ehs = 0, .ezi = 1, .zif = 0}},  // SD_POW: PortaPack CPLD.TDO(O)
        {1, 8, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 0, .ehs = 0, .ezi = 0, .zif = 0}},  // SD_VOLT0: PortaPack CPLD.TMS(I)
        {6, 1, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},  // CPLD_TCK: PortaPack CPLD.TCK(I)
        {6, 2, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 1, .zif = 0}},  // CPLD_TDI: PortaPack CPLD.TDI(I)

    }};

/* Additional GPIO configuration for HackRF OG */
static const std::array<gpio_setup_t, 6> gpio_setup_og{{
    {
        // GPIO0
        .data = (0 << 9)    // P1_2:  10K PD, BOOT1
                | (1 << 8)  // P1_1:  10K PU, BOOT0
                | (1 << 7)  // P2_7:  10K PU, ISP
        ,
        .dir = (0 << 9)    // P1_2:  10K PD, BOOT1
               | (0 << 8)  // P1_1:  10K PU, BOOT0
               | (0 << 7)  // P2_7:  10K PU, ISP
    },
    {// GPIO1
     .data = 0,
     .dir = 0},
    {
        // GPIO2
        .data = (1 << 9)    // P5_0:  !VAA_ENABLE
                | (0 << 4)  // P4_4:  TXENABLE
        ,
        .dir = (1 << 9)    // P5_0:  !VAA_ENABLE
               | (1 << 4)  // P4_4:  TXENABLE
    },
    {
        // GPIO3
        .data = (0 << 6)  // P6_10: EN1V8, 10K PD
        ,
        .dir = (1 << 6)  // P6_10: EN1V8, 10K PD
    },
    {// GPIO4
     .data = 0,
     .dir = 0},
    {
        // GPIO5
        .data = (0 << 15)    // P6_7:  TX
                | (0 << 12)  // P4_8:  SGPIO13, HOST_SYNC_EN
                | (1 << 5)   // P2_5:  RX
        ,
        .dir = (1 << 15)    // P6_7:  TX
               | (0 << 12)  // P4_8:  SGPIO13, HOST_SYNC_EN
               | (1 << 5)   // P2_5:  RX
    },
}};

/* Additional GPIO configuration for HackRF r9 */
static const std::array<gpio_setup_t, 6> gpio_setup_r9{{
    {
        // GPIO0
        .data = (0 << 9)    // P1_2:  10K PD, BOOT1, CLKOUT_EN
                | (1 << 8)  // P1_1:  10K PU, BOOT0, MCU_CLK_EN
                | (1 << 7)  // P2_7:  10K PU, ISP, RX
        ,
        .dir = (0 << 9)    // P1_2:  10K PD, BOOT1, CLKOUT_EN
               | (0 << 8)  // P1_1:  10K PU, BOOT0, MCU_CLK_EN
               | (0 << 7)  // P2_7:  10K PU, ISP, RX
    },
    {// GPIO1
     .data = 0,
     .dir = 0},
    {
        // GPIO2
        .data = (1 << 9)    // P5_0:  EN1V8, 10K PD
                | (1 << 4)  // P4_4:  !ANT_BIAS
        ,
        .dir = (1 << 9)    // P5_0:  EN1V8, 10K PD
               | (1 << 4)  // P4_4:  !ANT_BIAS
    },
    {
        // GPIO3
        .data = (1 << 6)  // P6_10: !VAA_ENABLE
        ,
        .dir = (1 << 6)  // P6_10: !VAA_ENABLE
    },
    {// GPIO4
     .data = 0,
     .dir = 0},
    {
        // GPIO5
        .data = (0 << 15)    // P6_7:  CLKIN_EN
                | (0 << 12)  // P4_8:  CLKIN_DETECT
                | (0 << 5)   // P2_5:  HOST_SYNC_EN
        ,
        .dir = (1 << 15)    // P6_7:  CLKIN_EN
               | (0 << 12)  // P4_8:  CLKIN_DETECT
               | (0 << 5)   // P2_5:  HOST_SYNC_EN
    },
}};

/* Additional SCU configuration for HackRF OG */
static const std::array<scu_setup_t, 9> pins_setup_og{{
    /* Power control */
    {5, 0, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},  /* !VAA_ENABLE: 10K PU, Q3.G(I), power to VAA */
    {6, 10, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}}, /* EN1V8/P70: 10K PD, TPS62410.EN2(I), 1V8LED.A(I) */

    /* Radio section control */
    {2, 5, scu_config_normal_drive_t{.mode = 4, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}}, /* RX/P43: U7.VCTL1(I), U10.VCTL1(I), U2.VCTL1(I) */
    {4, 4, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}}, /* TXENABLE/P55: MAX2837.TXENABLE(I) */
    {6, 7, scu_config_normal_drive_t{.mode = 4, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}}, /* TX/P42: U7.VCTL2(I), U10.VCTL2(I), U2.VCTL2(I) */

    /* SGPIO for sample transfer interface to HackRF CPLD. */
    {4, 8, scu_config_normal_drive_t{.mode = 4, .epd = 1, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}}, /* SGPIO13/BANK2F3M2: CPLD.90/HOST_SYNC_EN(I) */

    /* Miscellaneous */
    {1, 1, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}}, /* P1_1/P74: 10K PU, BOOT0 */
    {1, 2, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}}, /* P1_2/P73: 10K PD, BOOT1 */
    {2, 7, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}}, /* ISP: 10K PU, Unused */
}};

/* Additional SCU configuration for HackRF r9 */
static const std::array<scu_setup_t, 9> pins_setup_r9{{
    /* Power control */
    {6, 10, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}}, /* !VAA_ENABLE: 10K PU, Q3.G(I), power to VAA */
    {5, 0, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},  /* EN1V8: 10K PD, TPS62410.EN2(I), 1V8LED.A(I) */

    /* Radio section control */
    {2, 7, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}}, /* RX/ISP/P96: U7.VCTL(I), U10.VCTL(I), U2.VCTL(I) */
    {4, 4, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}}, /* !ANT_BIAS: 10K PU, Q4.G(I) */

    /* SGPIO for sample transfer interface to HackRF CPLD. */
    {2, 5, scu_config_normal_drive_t{.mode = 4, .epd = 1, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}}, /* BANK2F3M2: CPLD.90/HOST_SYNC_EN(I) */

    /* Clock control */
    {1, 1, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}}, /* MCU_CLK_EN/BOOT0: 10K PU, U28.1A(I) */
    {1, 2, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}}, /* CLKOUT_EN/BOOT1: 10K PD, U28.2A(I) */
    {6, 7, scu_config_normal_drive_t{.mode = 4, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}}, /* CLKIN_EN: U16.SEL(I), U26.1A(I) */

    /* Miscellaneous */
    {4, 8, scu_config_normal_drive_t{.mode = 1, .epd = 1, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}}, /* CLKIN_DETECT: U26.2Y(O) */
}};

#endif

#ifdef PRALINE

static const std::array<scu_setup_t, 30> pins_setup_portapack{{
    {2, 0, scu_config_normal_drive_t{.mode = 4, .epd = 0, .epun = 1, .ehs = 0, .ezi = 1, .zif = 0}},  /* U0_TXD: PortaPack P2_0/IO_STBX */
    {2, 1, scu_config_normal_drive_t{.mode = 4, .epd = 0, .epun = 1, .ehs = 0, .ezi = 1, .zif = 0}},  /* U0_RXD: PortaPack P2_1/ADDR */
    {2, 3, scu_config_normal_drive_t{.mode = 4, .epd = 0, .epun = 1, .ehs = 0, .ezi = 1, .zif = 1}},  /* I2C1_SDA: PortaPack P2_3/LCD_TE */
    {2, 4, scu_config_normal_drive_t{.mode = 4, .epd = 0, .epun = 1, .ehs = 0, .ezi = 1, .zif = 1}},  /* I2C1_SCL: PortaPack P2_4/LCD_RDX */
    {2, 8, scu_config_normal_drive_t{.mode = 1, .epd = 0, .epun = 0, .ehs = 0, .ezi = 1, .zif = 1}},  /* P2_8: 10K PD, BOOT2, DFU switch, PortaPack P2_8/<unused> */
    {2, 9, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 1, .zif = 0}},  /* P2_9: 10K PD, BOOT3, PortaPack P2_9/LCD_WRX */
    {2, 13, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 1, .zif = 0}}, /* P2_13: PortaPack P2_13/DIR */
    {7, 0, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 1, .zif = 0}},  /* GPIO3_8: PortaPack GPIO3_8(IO) */
    {7, 1, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 1, .zif = 0}},  /* GPIO3_9: PortaPack GPIO3_9(IO) */
    {7, 2, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 1, .zif = 0}},  /* GPIO3_10: PortaPack GPIO3_10(IO) */
    {7, 3, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 1, .zif = 0}},  /* GPIO3_11: PortaPack GPIO3_11(IO) */
    {7, 4, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 1, .zif = 0}},  /* GPIO3_12: PortaPack GPIO3_12(IO) */
    {7, 5, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 1, .zif = 0}},  /* GPIO3_13: PortaPack GPIO3_13(IO) */
    {7, 6, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 1, .zif = 0}},  /* GPIO3_14: PortaPack GPIO3_14(IO) */
    {7, 7, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 1, .zif = 0}},  /* GPIO3_15: PortaPack GPIO3_15(IO) */

    /* PortaPack: Audio */
    {3, 0, scu_config_normal_drive_t{.mode = 2, .epd = 0, .epun = 0, .ehs = 0, .ezi = 1, .zif = 0}},  /* I2S0_TX_SCK: PortaPack I2S0_TX_SCK(I) */
    {3, 1, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 0, .ehs = 0, .ezi = 1, .zif = 0}},  /* I2S0_RX_WS: PortaPack I2S0_TX_WS(I). Input enabled to fold back into RX. */
    {3, 2, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 0, .ehs = 0, .ezi = 0, .zif = 0}},  /* I2S0_RX_SDA: PortaPack I2S0_TX_SDA(I) */
    {24, 2, scu_config_normal_drive_t{.mode = 6, .epd = 1, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}}, /* I2S0_TX_CLK: PortaPack I2S0_TX_MCLK */

    /* PortaPack: SD card socket */
    {24, 0, scu_config_normal_drive_t{.mode = 4, .epd = 1, .epun = 1, .ehs = 0, .ezi = 1, .zif = 1}}, /* SD_CLK: PortaPack SD.CLK, enable input buffer for timing feedback? */
    {1, 6, scu_config_normal_drive_t{.mode = 7, .epd = 0, .epun = 0, .ehs = 0, .ezi = 1, .zif = 1}},  /* SD_CMD: PortaPack SD.CMD(IO)  */
    {1, 9, scu_config_normal_drive_t{.mode = 7, .epd = 0, .epun = 0, .ehs = 0, .ezi = 1, .zif = 1}},  /* SD_DAT0: PortaPack SD.DAT0(IO) */
    {1, 10, scu_config_normal_drive_t{.mode = 7, .epd = 0, .epun = 0, .ehs = 0, .ezi = 1, .zif = 1}}, /* SD_DAT1: PortaPack SD.DAT1(IO) */
    {1, 11, scu_config_normal_drive_t{.mode = 7, .epd = 0, .epun = 0, .ehs = 0, .ezi = 1, .zif = 1}}, /* SD_DAT2: PortaPack SD.DAT2(IO) */
    {1, 12, scu_config_normal_drive_t{.mode = 7, .epd = 0, .epun = 0, .ehs = 0, .ezi = 1, .zif = 1}}, /* SD_DAT3: PortaPack SD.DAT3(IO) */
    {1, 13, scu_config_normal_drive_t{.mode = 7, .epd = 0, .epun = 0, .ehs = 0, .ezi = 1, .zif = 0}}, /* SD_CD: PortaPack SD.CD(O) */

    // for pro touch
    // those pins should choose reserved function mode
    // and the behavior same as hackrf one(YN YP XN XP)
    {11, 6, scu_config_normal_drive_t{.mode = 7, .epd = 0, .epun = 0, .ehs = 0, .ezi = 0, .zif = 0}}, /* PB_6:  ADC0_0 yp*/
    {4, 5, scu_config_normal_drive_t{.mode = 3, .epd = 0, .epun = 0, .ehs = 0, .ezi = 0, .zif = 0}},  /* P4_5:  ADC0_2 yn*/
    {4, 4, scu_config_normal_drive_t{.mode = 3, .epd = 0, .epun = 0, .ehs = 0, .ezi = 0, .zif = 0}},  /* P4_4:  ADC0_5 xp */
    {15, 4, scu_config_normal_drive_t{.mode = 3, .epd = 0, .epun = 0, .ehs = 0, .ezi = 0, .zif = 0}}, /* PF_4:  ADC0_6 xn*/
}};

#else
static const std::array<scu_setup_t, 26> pins_setup_portapack{{
    {2, 0, scu_config_normal_drive_t{.mode = 4, .epd = 0, .epun = 1, .ehs = 0, .ezi = 1, .zif = 0}},  /* U0_TXD: PortaPack P2_0/IO_STBX */
    {2, 1, scu_config_normal_drive_t{.mode = 4, .epd = 0, .epun = 1, .ehs = 0, .ezi = 1, .zif = 0}},  /* U0_RXD: PortaPack P2_1/ADDR */
    {2, 3, scu_config_normal_drive_t{.mode = 4, .epd = 0, .epun = 1, .ehs = 0, .ezi = 1, .zif = 0}},  /* I2C1_SDA: PortaPack P2_3/LCD_TE */
    {2, 4, scu_config_normal_drive_t{.mode = 4, .epd = 0, .epun = 1, .ehs = 0, .ezi = 1, .zif = 0}},  /* I2C1_SCL: PortaPack P2_4/LCD_RDX */
    {2, 8, scu_config_normal_drive_t{.mode = 1, .epd = 0, .epun = 0, .ehs = 0, .ezi = 1, .zif = 1}},  /* P2_8: 10K PD, BOOT2, DFU switch, PortaPack P2_8/<unused> */
    {2, 9, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 1, .zif = 0}},  /* P2_9: 10K PD, BOOT3, PortaPack P2_9/LCD_WRX */
    {2, 13, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 1, .zif = 0}}, /* P2_13: PortaPack P2_13/DIR */
    {7, 0, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 1, .zif = 0}},  /* GPIO3_8: PortaPack GPIO3_8(IO) */
    {7, 1, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 1, .zif = 0}},  /* GPIO3_9: PortaPack GPIO3_9(IO) */
    {7, 2, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 1, .zif = 0}},  /* GPIO3_10: PortaPack GPIO3_10(IO) */
    {7, 3, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 1, .zif = 0}},  /* GPIO3_11: PortaPack GPIO3_11(IO) */
    {7, 4, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 1, .zif = 0}},  /* GPIO3_12: PortaPack GPIO3_12(IO) */
    {7, 5, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 1, .zif = 0}},  /* GPIO3_13: PortaPack GPIO3_13(IO) */
    {7, 6, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 1, .zif = 0}},  /* GPIO3_14: PortaPack GPIO3_14(IO) */
    {7, 7, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 1, .zif = 0}},  /* GPIO3_15: PortaPack GPIO3_15(IO) */

    /* PortaPack: Audio */
    {3, 0, scu_config_normal_drive_t{.mode = 2, .epd = 0, .epun = 0, .ehs = 0, .ezi = 1, .zif = 0}},  /* I2S0_TX_SCK: PortaPack I2S0_TX_SCK(I) */
    {3, 1, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 0, .ehs = 0, .ezi = 1, .zif = 0}},  /* I2S0_RX_WS: PortaPack I2S0_TX_WS(I). Input enabled to fold back into RX. */
    {3, 2, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 0, .ehs = 0, .ezi = 0, .zif = 0}},  /* I2S0_RX_SDA: PortaPack I2S0_TX_SDA(I) */
    {24, 2, scu_config_normal_drive_t{.mode = 6, .epd = 1, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}}, /* I2S0_TX_CLK: PortaPack I2S0_TX_MCLK */

    /* PortaPack: SD card socket */
    {24, 0, scu_config_normal_drive_t{.mode = 4, .epd = 1, .epun = 1, .ehs = 0, .ezi = 1, .zif = 1}}, /* SD_CLK: PortaPack SD.CLK, enable input buffer for timing feedback? */
    {1, 6, scu_config_normal_drive_t{.mode = 7, .epd = 0, .epun = 0, .ehs = 0, .ezi = 1, .zif = 1}},  /* SD_CMD: PortaPack SD.CMD(IO)  */
    {1, 9, scu_config_normal_drive_t{.mode = 7, .epd = 0, .epun = 0, .ehs = 0, .ezi = 1, .zif = 1}},  /* SD_DAT0: PortaPack SD.DAT0(IO) */
    {1, 10, scu_config_normal_drive_t{.mode = 7, .epd = 0, .epun = 0, .ehs = 0, .ezi = 1, .zif = 1}}, /* SD_DAT1: PortaPack SD.DAT1(IO) */
    {1, 11, scu_config_normal_drive_t{.mode = 7, .epd = 0, .epun = 0, .ehs = 0, .ezi = 1, .zif = 1}}, /* SD_DAT2: PortaPack SD.DAT2(IO) */
    {1, 12, scu_config_normal_drive_t{.mode = 7, .epd = 0, .epun = 0, .ehs = 0, .ezi = 1, .zif = 1}}, /* SD_DAT3: PortaPack SD.DAT3(IO) */
    {1, 13, scu_config_normal_drive_t{.mode = 7, .epd = 0, .epun = 0, .ehs = 0, .ezi = 1, .zif = 0}}, /* SD_CD: PortaPack SD.CD(O) */
}};
#endif

static const std::array<scu_setup_t, 6> pins_setup_spifi{{
    {3, 3, scu_config_normal_drive_t{.mode = 3, .epd = 0, .epun = 1, .ehs = 1, .ezi = 1, .zif = 1}}, /* SPIFI_SCK: W25Q80BV.CLK(I), enable input buffer for timing feedback */
    {3, 4, scu_config_normal_drive_t{.mode = 3, .epd = 0, .epun = 1, .ehs = 1, .ezi = 1, .zif = 1}}, /* SPIFI_SIO3/P82: W25Q80BV.HOLD(IO) */
    {3, 5, scu_config_normal_drive_t{.mode = 3, .epd = 0, .epun = 1, .ehs = 1, .ezi = 1, .zif = 1}}, /* SPIFI_SIO2/P81: W25Q80BV.WP(IO) */
    {3, 6, scu_config_normal_drive_t{.mode = 3, .epd = 0, .epun = 1, .ehs = 1, .ezi = 1, .zif = 1}}, /* SPIFI_MISO: W25Q80BV.DO(IO) */
    {3, 7, scu_config_normal_drive_t{.mode = 3, .epd = 0, .epun = 1, .ehs = 1, .ezi = 1, .zif = 1}}, /* SPIFI_MOSI: W25Q80BV.DI(IO) */
    {3, 8, scu_config_normal_drive_t{.mode = 3, .epd = 0, .epun = 1, .ehs = 1, .ezi = 1, .zif = 1}}, /* SPIFI_CS/P68: W25Q80BV.CS(I) */
}};

template <size_t N>
void setup_gpios(const std::array<gpio_setup_t, N>& pins_setup) {
    for (size_t i = 0; i < N; i++) {
        LPC_GPIO->PIN[i] |= pins_setup[i].data;
        LPC_GPIO->DIR[i] |= pins_setup[i].dir;
    }
}

static void setup_pin(const scu_setup_t& pin_setup) {
    LPC_SCU->SFSP[pin_setup.port][pin_setup.pin] = pin_setup.config;
}

template <size_t N>
void setup_pins(const std::array<scu_setup_t, N>& pins_setup) {
    for (const auto& pin_setup : pins_setup) {
        setup_pin(pin_setup);
    }
}

/*
 * HackRF One r9 has a pull-up on GPIO3_6 (P6_10) and a pull-down on GPIO2_9 (P5_0).
 * HackRF One OG has a pull-down on GPIO3_6 (P6_10) and a pull-up on GPIO2_9 (P5_0).
 */
static const scu_setup_t pin_setup_detect{5, 0, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 1, .zif = 0}};

/* Check resistor on GPIO2_9 (P5_0) to detect HackRF hardware revision. */
extern "C" bool detect_hackrf_r9() {
    setup_pin(pin_setup_detect);
    LPC_GPIO->DIR[2] &= ~(1 << 9);
    return LPC_GPIO->W2[9] == 0;
}

static void configure_spifi(void) {
    setup_pins(pins_setup_spifi);

    /* Tweak SPIFI mode */
    LPC_SPIFI->CTRL =
        (0xffff << 0) /* Timeout */
        | (0x1 << 16) /* CS high time in "clocks - 1" */
        | (0 << 21)   /* 0: Attempt speculative prefetch on data accesses */
        | (0 << 22)   /* 0: No interrupt on command ended */
        | (0 << 23)   /* 0: SCK driven low after rising edge at which last bit of command is captured. Stays low while CS# is high. */
        | (0 << 27)   /* 0: Cache prefetching enabled */
        | (0 << 28)   /* 0: Quad protocol, IO3:0 */
        | (1 << 29)   /* 1: Read data sampled on falling edge of clock */
        | (1 << 30)   /* 1: Read data is sampled using feedback clock from SCK pin */
        | (0 << 31)   /* 0: DMA request disabled */
        ;

    /* Throttle up the SPIFI interface to 96MHz (IDIVA=PLL1 / 3) */
    LPC_CGU->IDIVB_CTRL.word =
        (0 << 0)    /* PD */
        | (2 << 2)  /* IDIV (/3) */
        | (1 << 11) /* AUTOBLOCK */
        | (9 << 24) /* PLL1 */
        ;
}

void configure_pins_portapack(void) {
    LPC_GPIO->DIR[1] |= (1 << 13) | (1 << 10);
    LPC_GPIO->DIR[3] |= (0xff << 8);
    LPC_GPIO->DIR[5] |= (1 << 4) | (1 << 1) | (1 << 0);
    setup_pins(pins_setup_portapack);
}

static const motocon_pwm_resources_t motocon_pwm_resources = {
    .base = {.clk = &LPC_CGU->BASE_APB1_CLK, .stat = &LPC_CCU1->BASE_STAT, .stat_mask = (1 << 1)},
    .branch = {.cfg = &LPC_CCU1->CLK_APB1_MOTOCON_PWM_CFG, .stat = &LPC_CCU1->CLK_APB1_MOTOCON_PWM_STAT},
    .reset = {.output_index = 38},
};

static const scu_setup_t pin_setup_vaa_enablex_pwm = {5, 0, scu_config_normal_drive_t{.mode = 1, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}};
static const scu_setup_t pin_setup_vaa_enablex_gpio_og = {5, 0, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}};
static const scu_setup_t pin_setup_vaa_enablex_gpio_r9 = {6, 10, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}};

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
    const uint32_t CORTEX_M4_CPUID = 0x410fc240;
    const uint32_t CORTEX_M4_CPUID_MASK = 0xff0ffff0;

    if ((SCB->CPUID & CORTEX_M4_CPUID_MASK) == CORTEX_M4_CPUID) {
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
            (1U << 16)    // LCD_RST
            | (1U << 17)  // USB0_RST
            | (1U << 18)  // USB1_RST
            | (1U << 19)  // DMA_RST
            | (1U << 20)  // SDIO_RST
            | (1U << 21)  // EMC_RST
            | (1U << 22)  // ETHERNET_RST
            | (1U << 28)  // GPIO_RST
            ;
        LPC_RGU->RESET_CTRL[1] =
            (1U << 0)     // TIMER0_RST
            | (1U << 1)   // TIMER1_RST
            | (1U << 2)   // TIMER2_RST
            | (1U << 3)   // TIMER3_RST
            | (1U << 4)   // RITIMER_RST
            | (1U << 5)   // SCT_RST
            | (1U << 6)   // MOTOCONPWM_RST
            | (1U << 7)   // QEI_RST
            | (1U << 8)   // ADC0_RST
            | (1U << 9)   // ADC1_RST
            | (1U << 10)  // DAC_RST
            | (1U << 12)  // UART0_RST
            | (1U << 13)  // UART1_RST
            | (1U << 14)  // UART2_RST
            | (1U << 15)  // UART3_RST
            | (1U << 16)  // I2C0_RST
            | (1U << 17)  // I2C1_RST
            | (1U << 18)  // SSP0_RST
            | (1U << 19)  // SSP1_RST
            | (1U << 20)  // I2S_RST
            | (1U << 22)  // CAN1_RST
            | (1U << 23)  // CAN0_RST
            | (1U << 24)  // M0APP_RST
            | (1U << 25)  // SGPIO_RST
            | (1U << 26)  // SPI_RST
            | (1U << 28)  // ADCHS_RST
            ;

        configure_spifi();

        LPC_CCU1->CLK_M4_M0APP_CFG.RUN = true;
        LPC_CREG->M0APPMEMMAP = LPC_SPIFI_DATA_CACHED_BASE + 0x0;
        LPC_RGU->RESET_CTRL[1] = 0;

        /* Prevent the M4 from doing any more initializing by sleep-waiting forever...
         * ...until the M0 resets the M4 with some code to run.
         */
        while (1) {
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

#ifdef PRALINE

void aux_power_on(void) {
    // 3.3V Aux - P6_7 = GPIO5[15], Active LOW (Clear = ON)
    LPC_GPIO->CLR[5] = (1 << 15);
}

void aux_power_off(void) {
    // 3.3V Aux - P6_7 = GPIO5[15], Active LOW (Set = OFF)
    LPC_GPIO->SET[5] = (1 << 15);
}

#endif /* PRALINE */

void core_power_on(void) {  // Core power enable
#ifdef PRALINE
    // 1.2V FPGA - P8_7 = GPIO4[7], Active HIGH (Set = ON)
    LPC_GPIO->SET[4] = (1 << 7);

#else
    if (hackrf_r9) {
        // P5_0 (GPIO2[9]) is the EN1V8 pin
        LPC_GPIO->SET[2] = (1 << 9);
    } else {
        // On older OG HackRF boards, P6_10 (GPIO3[6]) is the EN1V8 pin
        LPC_GPIO->SET[3] = (1 << 6);
    }
#endif
}

void core_power_off(void) {  // Core power disable
#ifdef PRALINE
    // 1.2V FPGA - P8_7 = GPIO4[7], Active HIGH (Clear = OFF) */
    LPC_GPIO->CLR[4] = (1 << 7);

#else
    if (hackrf_r9) {
        // P5_0 (GPIO2[9]) is the EN1V8 pin
        LPC_GPIO->CLR[2] = (1 << 9);
    } else {
        // On older OG HackRF boards, P6_10 (GPIO3[6]) is the EN1V8 pin
        LPC_GPIO->CLR[3] = (1 << 6);
    }
#endif
}

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
    /* Very twitchy process for powering up VAA without glitching the 3.3V rail,
     * which can send the microcontroller into reset.
     */
#ifdef PRALINE
    /* P8_1 (GPIO4[1]) does not have MOTOCONPWM hardware routing.
     * Using software bit-banging (pseudo-PWM) for VAA soft-start.
     * VAA is active LOW (0 = ON).
     */

    /* Software soft-start loop to prevent brown-out */
    for (uint32_t i = 0; i < 1000; i++) {
        LPC_GPIO->W4[1] = 0; /* Turn ON briefly */
        LPC_GPIO->W4[1] = 1; /* Turn OFF briefly */
    }

    /* Latch VAA to ON state (Active LOW) */
    LPC_GPIO->CLR[4] = (1 << 1);

#else
    if (hackrf_r9) {
        /* Soft-start for HackRF r9 using software bit-banging */
        setup_pin(pin_setup_vaa_enablex_gpio_r9);
        for (uint32_t i = 0; i < 1000; i++) {
            LPC_GPIO->W3[6] = 1;
            LPC_GPIO->W3[6] = 0;
        }
    } else {
        /* Soft-start for HackRF OG using MOTOCONPWM hardware peripheral */
        base_clock_enable(&motocon_pwm_resources.base);
        branch_clock_enable(&motocon_pwm_resources.branch);
        peripheral_reset(&motocon_pwm_resources.reset);

        const uint32_t cycle_period = 256;
        uint32_t enable_period = 2;
        LPC_MCPWM->TC2 = 0;
        LPC_MCPWM->MAT2 = cycle_period - enable_period;
        LPC_MCPWM->LIM2 = cycle_period;

        setup_pin(pin_setup_vaa_enablex_pwm);
        LPC_MCPWM->CON_SET = (1 << 16);

        while (enable_period < cycle_period) {
            {
                volatile uint32_t delay = 2000;
                while (delay--);
            }
            enable_period <<= 1;
            LPC_MCPWM->MAT2 = cycle_period - enable_period;
        }

        LPC_GPIO->CLR[2] = (1 << 9);
        LPC_GPIO->DIR[2] |= (1 << 9);
        setup_pin(pin_setup_vaa_enablex_gpio_og);

        peripheral_reset(&motocon_pwm_resources.reset);
        branch_clock_disable(&motocon_pwm_resources.branch);
        base_clock_disable(&motocon_pwm_resources.base);
    }

    /* Handle VAA Enable Pin Latching */
    if (hackrf_r9) {
        LPC_GPIO->W2[9] = 1;
    } else {
        LPC_GPIO->W3[6] = 1;
    }
#endif
}

void vaa_power_off(void) {
    /* TODO: There's a lot of other stuff that must be done to prevent
     * leakage from +3V3 into VAA.
     */
#ifdef PRALINE
    /* Safe state: OFF (VAA RF is active LOW, so Set = OFF) */
    LPC_GPIO->SET[4] = (1 << 1);

    /* Turn OFF LED3 (TX) */
    LPC_GPIO->SET[2] = (1 << 8);

#else
    if (hackrf_r9) {
        LPC_GPIO->W3[6] = 1;  // Turn OFF VAA for r9 P6_10
    } else {
        LPC_GPIO->W2[9] = 1;  // Turn OFF VAA for OG P5_0
    }
#endif
}

/**
 * @brief   Board-specific initialization code.
 * @details Initializes LEDs, power rails, and configures physical pins
 * for both PRALINE and standard HackRF hardware variants.
 */
extern "C" void boardInit(void) {
#ifdef PRALINE
    /* HackRF Pro Specific: Initialize and Load FPGA */
    hackrf_r9 = false;

    aux_power_on();

    LPC_GPIO->SET[2] = (1 << 1) | (1 << 2) | (1 << 8);
    LPC_GPIO->CLR[4] = (1 << 6);

#else

    /* Detect HackRF variant */
    /* 1. Perform Standard Initialization first */
    /* This configures VAA power, LED pins, and detects board revision */
    /* Let detect_hackrf_r9() run - don't force for PRALINE */
    hackrf_r9 = detect_hackrf_r9();
    /* Configure variant-dependent pins. */
    if (hackrf_r9) {
        setup_gpios(gpio_setup_r9);
        setup_pins(pins_setup_r9);
    } else {
        setup_gpios(gpio_setup_og);
        setup_pins(pins_setup_og);
    }

#endif
}

extern "C" void _default_exit(void) {
    if (hackrf_r9) {
        LPC_GPIO->W2[9] = 0;
    } else {
        LPC_GPIO->W3[6] = 0;
    }

    vaa_power_off();

    chSysDisable();
    systick_stop();

    /* Don't reset these peripherals, as they're operating during shutdown:
     *   WWDT, CREG, SCU, SPIFI, GPIO, M0APP
     */
    LPC_RGU->RESET_CTRL[0] =
        (1U << 16)    // LCD_RST
        | (1U << 17)  // USB0_RST
        | (1U << 18)  // USB1_RST
        | (1U << 19)  // DMA_RST
        | (1U << 20)  // SDIO_RST
        | (1U << 21)  // EMC_RST
        | (1U << 22)  // ETHERNET_RST
        //| (1U << 28) // GPIO_RST
        ;
    LPC_RGU->RESET_CTRL[1] =
        (1U << 0)     // TIMER0_RST
        | (1U << 1)   // TIMER1_RST
        | (1U << 2)   // TIMER2_RST
        | (1U << 3)   // TIMER3_RST
        | (1U << 4)   // RITIMER_RST
        | (1U << 5)   // SCT_RST
        | (1U << 6)   // MOTOCONPWM_RST
        | (1U << 7)   // QEI_RST
        | (1U << 8)   // ADC0_RST
        | (1U << 9)   // ADC1_RST
        | (1U << 10)  // DAC_RST
        | (1U << 12)  // UART0_RST
        | (1U << 13)  // UART1_RST
        | (1U << 14)  // UART2_RST
        | (1U << 15)  // UART3_RST
        | (1U << 16)  // I2C0_RST
        | (1U << 17)  // I2C1_RST
        | (1U << 18)  // SSP0_RST
        | (1U << 19)  // SSP1_RST
        | (1U << 20)  // I2S_RST
        | (1U << 22)  // CAN1_RST
        | (1U << 23)  // CAN0_RST
        //| (1U << 24) // M0APP_RST
        | (1U << 25)  // SGPIO_RST
        | (1U << 26)  // SPI_RST
        | (1U << 28)  // ADCHS_RST
        ;
}