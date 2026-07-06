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

#include "gpio.hpp"
using namespace gpio_control;

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
            = boot_bit(p1_ctrl0, 0)      // P2_10: P1_CTRL0, start low
              | boot_bit(clkin_ctrl, 0)  // P1_20: CLKIN_CTRL - start low
#else
            = boot_bit(amp_bypass, 1)  // P2_10: AMP_BYPASS
              | (1 << 15)              // P1_20: CS_XCVR
#endif
              | boot_bit(dfu_boot_0, 0)  // P1_1:  10K PU, BOOT0
              | boot_bit(dfu_boot_1, 0)  // P1_2:  10K PD, BOOT1
              | boot_bit(dfu_isp, 0)     // P2_7:  10K PU, ISP
              | (1 << 11)                // P1_4:  SSP1_MOSI
              | boot_bit(sgpio_12, 0)    // P1_18: SGPIO12, HOST_Q_INVERT
              | boot_bit(sgpio_11, 0)    // P1_17: SGPIO11, HOST_DIRECTION, Praline: FPGA HOST_DIRECTION
              | (1 << 10)                // P1_3:  SSP1_MISO
              | (0 << 6)                 // P3_6:  SPIFI_MISO
              | boot_bit(sgpio_5, 1)     // P6_6:  SGPIO5, HOST_DATA5, Praline: FPGA HOST_DATA5
              | boot_bit(sgpio_7, 1)     // P1_0:  SGPIO7, HOST_DATA7, Praline: FPGA HOST_DATA7
              | boot_bit(sgpio_3, 1)     // P1_16: SGPIO3, HOST_DATA3, Praline: FPGA HOST_DATA3
              | boot_bit(sgpio_2, 1)     // P1_15: SGPIO2, HOST_DATA2, Praline: FPGA HOST_DATA2
              | boot_bit(sgpio_1, 1)     // P0_1:  SGPIO1, HOST_DATA1, Praline: FPGA HOST_DATA1
              | boot_bit(sgpio_0, 1)     // P0_0:  SGPIO0, HOST_DATA0, Praline: FPGA HOST_DATA0
            ,
            .dir
#ifdef PRALINE
            = boot_bit(p1_ctrl0, 1)      // P2_10: P1_CTRL0
              | boot_bit(clkin_ctrl, 1)  // P1_20: CLKIN_CTRL
#else
            = boot_bit(amp_bypass, 1)  // P2_10: AMP_BYPASS
              | (1 << 15)              // P1_20: CS_XCVR
#endif
              | boot_bit(dfu_boot_0, 0)  // P1_1:  10K PU, BOOT0
              | boot_bit(dfu_boot_1, 0)  // P1_2:  10K PD, BOOT1
              | boot_bit(dfu_isp, 0)     // P2_7:  10K PU, ISP
              | (0 << 10)                // P1_3:  SSP1_MISO
              | (0 << 11)                // P1_4:  SSP1_MOSI
              | boot_bit(sgpio_12, 1)    // P1_18: SGPIO12, HOST_Q_INVERT
              | boot_bit(sgpio_11, 0)    // P1_17: SGPIO11, HOST_DIRECTION, Praline: FPGA HOST_DIRECTION
              | (0 << 6)                 // P3_6:  SPIFI_MISO
              | boot_bit(sgpio_5, 1)     // P6_6:  SGPIO5, HOST_DATA5
              | boot_bit(sgpio_7, 1)     // P1_0:  SGPIO7, HOST_DATA7
              | boot_bit(sgpio_3, 0)     // P1_16: SGPIO3, HOST_DATA3
              | boot_bit(sgpio_2, 0)     // P1_15: SGPIO2, HOST_DATA2
              | boot_bit(sgpio_1, 0)     // P0_1:  SGPIO1, HOST_DATA1
              | boot_bit(sgpio_0, 0)     // P0_0:  SGPIO0, HOST_DATA0
        },
        {
            // GPIO1
            .data = (1 << 15)    // P3_5:  SPIFI_SIO2
                    | (1 << 14)  // P3_4:  SPIFI_SIO3
                    | (1 << 13)  // P2_13: PortaPack DIR
#ifdef PRALINE
                    | boot_bit(ant_bias, 1)     // P2_12: !BIAS_EN
                    | boot_bit(ant_bias_oc, 0)  // P2_11: BIAS_OC
                    | boot_bit(aa_en, 0)        // P1_14: AA_EN
                    | (0 << 0)                  // P1_7:  Output GND
#else
                    | boot_bit(rx_amp_pwr, 1)     // P2_12: !RX_AMP_PWR
                    | boot_bit(rx_amp, 0)         // P2_11: RX_AMP
                    | boot_bit(sgpio_10, 1)       // P1_14: SGPIO10, HOST_DISABLE
                    | boot_bit(tx_mix_bypass, 1)  // P1_7:  !TX MIX BYPASS
#endif
                    | boot_bit(lcd_wrx, 0)  // P2_9:  10K PD, BOOT3, PortaPack LCD_WRX
                    | (1 << 9)              // P1_6:  SD_CMD
                    | (1 << 8)              // P1_5:  SD_POW, PortaPack CPLD.TDO(O)
                    | (1 << 6)              // P1_13: SD_CD
                    | (1 << 5)              // P1_12: SD_DAT3
                    | (1 << 4)              // P1_11: SD_DAT2
                    | (1 << 3)              // P1_10: SD_DAT1
                    | (1 << 2)              // P1_9:  SD_DAT0
                    | (1 << 1)              // P1_8:  PortaPack CPLD.TMS(I)
            ,
            .dir = (0 << 15)    // P3_5:  SPIFI_SIO2
                   | (0 << 14)  // P3_4:  SPIFI_SIO3
                   | (1 << 13)  // P2_13: PortaPack DIR
#ifdef PRALINE
                   | boot_bit(ant_bias, 1)     // P2_12: !BIAS_EN
                   | boot_bit(ant_bias_oc, 0)  // P2_11: BIAS_OC
                   | boot_bit(aa_en, 1)        // P1_14: AA_EN
                   | (1 << 0)                  // P1_7:  Output
#else
                   | boot_bit(rx_amp_pwr, 1)     // P2_12: !RX_AMP_PWR
                   | boot_bit(rx_amp, 1)         // P2_11: RX_AMP
                   | boot_bit(sgpio_10, 0)       // P1_14: SGPIO10, HOST_DISABLE
                   | boot_bit(tx_mix_bypass, 1)  // P1_7:  !TX MIX BYPASS
#endif
                   | boot_bit(lcd_wrx, 1)  // P2_9:  10K PD, BOOT3, PortaPack LCD_WRX
                   | (0 << 9)              // P1_6:  SD_CMD
                   | (0 << 8)              // P1_5:  SD_POW, PortaPack CPLD.TDO(O)
                   | (0 << 6)              // P1_13: SD_CD
                   | (0 << 5)              // P1_12: SD_DAT3
                   | (0 << 4)              // P1_11: SD_DAT2
                   | (0 << 3)              // P1_10: SD_DAT1
                   | (0 << 2)              // P1_9:  SD_DAT0
                   | (0 << 1)              // P1_8:  PortaPack CPLD.TMS(I)
        },
        {
            // GPIO2
            .data = (1 << 14)  // P5_5:  MIXER_RESETX, 10K PU
#ifdef PRALINE
                    | (0 << 15)             // P5_6:  unused on PRALINE
                    | (1 << 13)             // P5_4:  RFFC5072 ENX
                    | (0 << 12)             // P5_3:  unused on PRALINE
                    | (0 << 11)             // P5_2:  FPGA_CRESET
                    | (1 << 10)             // P5_1:  FPGA_SPI_CS
                    | (0 << 4)              // P4_4:  unused on PRALINE
                    | (0 << 0)              // P4_0:  unused on PRALINE
                    | (0 << 9)              // P5_0:  unused on PRALINE
                    | (0 << 3)              // P4_3:  VBUSCTRL input GND
                    | (1 << 6)              // P4_6:  Input VCC (10K PU)
                    | boot_bit(led_tx, 1)   // P6_12: LED3 (TX)
                    | boot_bit(led_rx, 1)   // P4_2:  LED2 (RX)
                    | boot_bit(led_usb, 1)  // P4_1:  LED1 (USB)
#else
                    | boot_bit(tx_amp, 0)  // P5_6:  TX_AMP

                    | (1 << 13)               // P5_4:  MIXER_ENX, 10K PU
                    | boot_bit(rx_mix_bp, 1)  // P5_3:  RX_MIX_BP
                    | boot_bit(tx_mix_bp, 0)  // P5_2:  TX_MIX_BP
                    | boot_bit(lpf, 0)        // P5_1:  LPF
                    | (0 << 9)                // P5_0:  Varies by revision
                    | boot_bit(hpf, 0)        // P4_0:  HPF
                    | boot_bit(sgpio_9, 1)    // P4_3:  SGPIO9, HOST_CAPTURE
                    | (0 << 6)                // P4_6:  XCVR_EN, 10K PD
                    | boot_bit(led_tx, 0)     // P6_12: LED3 (TX)
                    | boot_bit(led_rx, 0)     // P4_2:  LED2 (RX)
                    | boot_bit(led_usb, 0)    // P4_1:  LED1 (USB)
#endif

                    | (1 << 7)  // P5_7:  CS_AD
                    | (0 << 5)  // P4_5:  RXENABLE
            ,
            .dir =
#ifdef PRALINE
                (1 << 15)    // P5_6:  unused on PRALINE
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
                boot_bit(tx_amp, 1)       // P5_6:  TX_AMP
                | (1 << 13)               // P5_4:  MIXER_ENX, 10K PU
                | boot_bit(rx_mix_bp, 1)  // P5_3:  RX_MIX_BP
                | boot_bit(tx_mix_bp, 1)  // P5_2:  TX_MIX_BP
                | boot_bit(lpf, 1)        // P5_1:  LPF
                | (0 << 9)                // P5_0:  Varies by revision
                | boot_bit(hpf, 1)        // P4_0:  HPF
                | boot_bit(sgpio_9, 0)    // P4_3:  SGPIO9, HOST_CAPTURE
                | (1 << 6)                // P4_6:  XCVR_EN, 10K PD
#endif
                | (1 << 14)             // P5_5:  MIXER_RESETX, 10K PU
                | boot_bit(led_tx, 1)   // P6_12: LED3 (TX)
                | (1 << 7)              // P5_7:  CS_AD
                | (1 << 5)              // P4_5:  RXENABLE
                | boot_bit(led_rx, 1)   // P4_2:  LED2 (RX)
                | boot_bit(led_usb, 1)  // P4_1:  LED1 (USB)
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
                    | boot_bit(tx_enable, 0)     // P6_5:  TX_ENABLE
                    | boot_bit(mix_bypass, 1)    // P6_3:  MIX_ENABLE_N
                    | (0 << 6)                   // P6_10: unused on PRALINE
                    | boot_bit(p1_ctrl2, 0)      // P6_9:  P1_CTRL2
                    | boot_bit(aux_power_oc, 0)  // P6_11: AUX overcurrent
#else
                    | (1 << 4)                 // P6_5:  HackRF CPLD.TMS(I)
                    | boot_bit(vregmode, 1)    // P6_11: VREGMODE
                    | (0 << 6)                 // P6_10: Varies by revision
                    | boot_bit(sgpio_4, 1)     // P6_3:  SGPIO4
                    | boot_bit(tx_amp_pwr, 1)  // P6_9:  !TX_AMP_PWR, 10K PU
#endif
                    | (1 << 3)  // P6_4:  MIXER_SDATA
                    | (1 << 1)  // P6_2:  HackRF CPLD.TDI(I)
                    | (1 << 0)  // P6_1:  HackRF CPLD.TCK(I)
            ,
            .dir = (1 << 15)    // P7_7:  PortaPack GPIO3_15(IO)
                   | (1 << 14)  // P7_6:  PortaPack GPIO3_14(IO)
                   | (1 << 13)  // P7_5:  PortaPack GPIO3_13(IO)
                   | (1 << 12)  // P7_4:  PortaPack GPIO3_12(IO)
                   | (1 << 11)  // P7_3:  PortaPack GPIO3_11(IO)
                   | (1 << 10)  // P7_2:  PortaPack GPIO3_10(IO)
                   | (1 << 9)   // P7_1:  PortaPack GPIO3_9(IO)
                   | (1 << 8)   // P7_0:  PortaPack GPIO3_8(IO)
#ifdef PRALINE
                   | boot_bit(aux_power_oc, 0)  // P6_11: AUX overcurrent
                   | boot_bit(tx_enable, 1)     // P6_5:  TX_ENABLE
                   | boot_bit(mix_bypass, 1)    // P6_3:  MIX_ENABLE_N
                   | (0 << 6)                   // P6_10: unused on PRALINE
                   | boot_bit(p1_ctrl2, 1)      // P6_9:  P1_CTRL2
#else
                   | boot_bit(vregmode, 1)    // P6_11: VREGMODE
                   | (0 << 6)                 // P6_10: Varies by revision
                   | boot_bit(sgpio_4, 0)     // P6_3:  SGPIO4
                   | boot_bit(tx_amp_pwr, 1)  // P6_9:  !TX_AMP_PWR, 10K PU
#endif
                   | (0 << 4)  // P6_5:  HackRF CPLD.TMS(I)
                   | (0 << 3)  // P6_4:  MIXER_SDATA
                   | (0 << 1)  // P6_2:  HackRF CPLD.TDI(I)
                   | (0 << 0)  // P6_1:  HackRF CPLD.TCK(I)
        },
        {
            // GPIO4
            .data =
                boot_bit(sgpio_8, 0)  // P9_6: SGPIO8, SGPIO_CLK, PRALINE: P8_0
#ifdef PRALINE
                | boot_bit(rf_amp_enable, 0)  // PA_2: RF_AMP_EN
                | boot_bit(lpf, 0)            // PA_1: LPF_EN
                | boot_bit(en_1v2, 0)         // P8_7: EN_1V2
                | boot_bit(led_mcu, 1)        // P8_6: LED4
                | boot_bit(sgpio_10, 1)       // P8_2: SGPIO10, HOST_DISABLE
                | (0 << 5)                    // P8_5: VIN_IN_EN
                | (0 << 4)                    // P8_4: VBUS_IN_EN
                | boot_bit(VAA_en, 1)         // P8_1: !VAA_EN
                | (0 << 10)                   // PA_3: Output GND
                | (0 << 11)                   // P9_6: MAX2831 LD
                | boot_bit(rf5072_mix_en, 0)  // P9_0: RF5072 enable
                | (0 << 13)                   // P9_1: Output GND
                | (0 << 14)                   // P9_2: RFFC5072 SDATA
                | (0 << 3)                    // P8_3: Output GND
                | boot_bit(sgpio_9, 1)        // P9_3: Capture Input
#endif
            ,
            .dir =
                boot_bit(sgpio_8, 0)  // P9_6: SGPIO8, SGPIO_CLK, PRALINE: P8_0
#ifdef PRALINE
                | boot_bit(rf_amp_enable, 1)  // PA_2: RF_AMP_EN
                | boot_bit(lpf, 1)            // PA_1: LPF_EN
                | boot_bit(en_1v2, 1)         // P8_7: EN_1V2
                | boot_bit(led_mcu, 1)        // P8_6: LED4
                | boot_bit(sgpio_10, 0)       // P8_2: SGPIO10, HOST_DISABLE
                | (1 << 5)                    // P8_5: VIN_IN_EN
                | (1 << 4)                    // P8_4: VBUS_IN_EN
                | boot_bit(VAA_en, 1)         // P8_1: !VAA_EN
                | (1 << 10)                   // PA_3: Output
                | (0 << 11)                   // P9_6: MAX2831 LD
                | boot_bit(rf5072_mix_en, 1)  // P9_0: RF5072 enable
                | (1 << 13)                   // P9_1: Output
                | (0 << 14)                   // P9_2: RFFC5072 SDATA (Bidirectional)
                | (1 << 3)                    // P8_3: Output
                | boot_bit(sgpio_9, 0)        // P9_3: Capture Input
#endif
        },
        {
            // GPIO5
            .data =
#ifdef PRALINE
                (0 << 18)                        // P9_5: RFF5072 SCLK
                | boot_bit(trigger_out, 0)       // P2_6: Trigger out
                | (0 << 14)                      // P4_10: FPGA CDONE
                | boot_bit(aux_power_enable, 1)  // P6_7: !3V3 AUX_ENABLE
                | boot_bit(p1_ctrl1, 0)          // P6_8: P1_CTRL1
                | boot_bit(sgpio_4, 1)           // P9_4: FPGA SGPIO4
                | (0 << 19)                      // PA_4: Output GND
                | (0 << 20)                      // PB_0: Output GND
                | (0 << 21)                      // PB_1: Output GND
                | (0 << 22)                      // PB_2: Unused IN
                | (0 << 23)                      // PB_3: Output GND
                | (0 << 24)                      // PB_4: Unused IN
                | (0 << 25)                      // PB_5: Output GND
                | boot_bit(pps_in_out, 0)        // P2_5: PPS OUT/IN
                | (0 << 12)                      // P4_8: Output GND
                | boot_bit(vregmode, 1)          // P4_9: TPS62410 VREGMODE
#else
                (1 << 18)                     // P9_5: HackRF CPLD.TDO(O)
                | (0 << 6)                    // P2_6: MIXER_SCLK
                | (1 << 14)                   // P4_10: SGPIO15, CPLD (unused)
                | boot_bit(rx_mix_bypass, 1)  // P6_8: RX MIX BYPASS
                | (1 << 13)                   // P4_9: SGPIO14, CPLD (unused)
                | (0 << 12)                   // P4_8: Varies by revision
#endif
                | (1 << 11)                // P3_8: SPIFI_CS
                | (1 << 10)                // P3_7: SPIFI_MOSI
                | (1 << 9)                 // P3_2: I2S0_RX_SDA
                | (1 << 8)                 // P3_1: I2S0_RX_WS
                | boot_bit(dfu_button, 0)  // P2_8: 10K PD, BOOT2, DFU button
                | (1 << 4)                 // P2_4: PortaPack LCD_RDX
                | (0 << 3)                 // P2_3: PortaPack LCD_TE
                | boot_bit(sgpio_6, 1)     // P2_2: SGPIO6, HOST_DATA6
                | (1 << 1)                 // P2_1: PortaPack ADDR
                | (1 << 0)                 // P2_0: PortaPack IO_STBX
            ,
            .dir =
#ifdef PRALINE
                (1 << 18)                        // P9_5: RFF5072 SCLK
                | boot_bit(trigger_out, 1)       // P2_6: Trigger out
                | (0 << 14)                      // P4_10: FPGA CDONE
                | boot_bit(aux_power_enable, 1)  // P6_7: !3V3 AUX_ENABLE
                | boot_bit(p1_ctrl1, 1)          // P6_8: P1_CTRL1
                | boot_bit(sgpio_4, 0)           // P9_4: FPGA SGPIO
                | (1 << 19)                      // PA_4: Output
                | (1 << 20)                      // PB_0: Output GND
                | (1 << 21)                      // PB_1: Output GND
                | (0 << 22)                      // PB_2: Unused
                | (1 << 23)                      // PB_3: Output
                | (0 << 24)                      // PB_4: Unused
                | (1 << 25)                      // PB_5: Output
                | boot_bit(pps_in_out, 0)        // P2_5: PPS (Bidirectional)
                | (1 << 12)                      // P4_8: Output
                | boot_bit(vregmode, 1)          // P4_9: TPS62410 VREGMODE
#else
                (0 << 18)                     // P9_5: HackRF CPLD.TDO(O)
                | (1 << 6)                    // P2_6: MIXER_SCLK
                | (0 << 14)                   // P4_10: SGPIO15, CPLD
                | boot_bit(rx_mix_bypass, 1)  // P6_8: RX MIX BYPASS
                | (0 << 13)                   // P4_9: SGPIO14, CPLD
                | (0 << 12)                   // P4_8: Varies by revision
#endif
                | (0 << 11)                // P3_8: SPIFI_CS
                | (0 << 10)                // P3_7: SPIFI_MOSI
                | (0 << 9)                 // P3_2: I2S0_RX_SDA
                | (0 << 8)                 // P3_1: I2S0_RX_WS
                | boot_bit(dfu_button, 0)  // P2_8: 10K PD, BOOT2, DFU button
                | (0 << 4)                 // P2_4: PortaPack LCD_RDX
                | (0 << 3)                 // P2_3: PortaPack LCD_TE
                | boot_bit(sgpio_6, 0)     // P2_2: SGPIO6, HOST_DATA6
                | (0 << 1)                 // P2_1: PortaPack ADDR
                | (0 << 0)                 // P2_0: PortaPack IO_STBX
        },
        {
// GPIO6
#ifdef PRALINE
            .data = (1 << 28)                  // PD_14: MAX2831 chip select
                    | (0 << 29)                // PD_15: MAX2831 RXHP control RXHP low = 100 Hz HPF
                    | (1 << 30)                // PD_16: MAX5865 chip select
                    | (1 << 25)                // PD_11: RFFC5072 Lock Detect
                    | boot_bit(trigger_in, 0)  // PD_12: TRIGGER IN
            ,
            .dir = (1 << 28)                  // PD_14: MAX2831 chip select
                   | (1 << 29)                // PD_15: MAX2831 RXHP control
                   | (1 << 30)                // PD_16: MAX5865 chip select
                   | (0 << 25)                // PD_11: RFFC5072 Lock Detect
                   | boot_bit(trigger_in, 0)  // PD_12: TRIGGER IN
#else
            .data = 0,
            .dir = 0
#endif
        },
        {
// GPIO7
#ifdef PRALINE
            .data = (0 << 0)                 // PE_0: Output
                    | (0 << 1)               // PE_1: MAX2831 !SHDN
                    | (0 << 2)               // PE_2: MAX2831 RXTX
                    | boot_bit(p2_ctrl0, 0)  // PE_3: P2_ctrl0
                    | boot_bit(p2_ctrl1, 0)  // PE_4: P2_ctrl1
            ,
            .dir = (1 << 0)                 // PE_0: Output
                   | (1 << 1)               // PE_1: MAX2831 !SHDN
                   | (1 << 2)               // PE_2: MAX2831 RXTX
                   | boot_bit(p2_ctrl0, 1)  // PE_3: P2_ctrl0
                   | boot_bit(p2_ctrl1, 1)  // PE_4: P2_ctrl1
#else
            .data = 0,
            .dir = 0
#endif
        },
    },
    .SCU = {

        // SClock LEDs

        {4, 7, scu_config_normal_drive_t{.mode = 1, .epd = 0, .epun = 0, .ehs = 0, .ezi = 1, .zif = 1}},                                                           // GP_CLKIN: SI5351C.CLK7(O)
        {map_led_usb.scu_port, map_led_usb.scu_pin, scu_config_normal_drive_t{.mode = map_led_usb.gpio_mode, .epd = 0, .epun = 0, .ehs = 0, .ezi = 0, .zif = 0}},  // LED1: USB
        {map_led_rx.scu_port, map_led_rx.scu_pin, scu_config_normal_drive_t{.mode = map_led_rx.gpio_mode, .epd = 0, .epun = 0, .ehs = 0, .ezi = 0, .zif = 0}},     // LED2: RX
        {map_led_tx.scu_port, map_led_tx.scu_pin, scu_config_normal_drive_t{.mode = map_led_tx.gpio_mode, .epd = 0, .epun = 0, .ehs = 0, .ezi = 0, .zif = 0}},     // LED3: TX
#ifdef PRALINE
        {map_led_mcu.scu_port, map_led_mcu.scu_pin, scu_config_normal_drive_t{.mode = map_led_mcu.gpio_mode, .epd = 0, .epun = 0, .ehs = 0, .ezi = 0, .zif = 0}},  // LED4: PRALINE Custom
#endif

        // POWER MANAGEMENT (Regulators, Bias, Current Limits)

        {map_vregmode.scu_port, map_vregmode.scu_pin, scu_config_normal_drive_t{.mode = map_vregmode.gpio_mode, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},  // VREGMODE: TPS62410
#ifdef PRALINE
        {map_VAA_en.scu_port, map_VAA_en.scu_pin, scu_config_normal_drive_t{.mode = map_VAA_en.gpio_mode, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},                                // P8_1: !VAA_EN
        {8, 4, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},                                                                                      // P8_4: VBUS_IN_EN
        {8, 5, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},                                                                                      // P8_5: VIN_IN_EN
        {map_en_1v2.scu_port, map_en_1v2.scu_pin, scu_config_normal_drive_t{.mode = map_en_1v2.gpio_mode, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},                                // P8_7: EN_1V2
        {map_aux_power_enable.scu_port, map_aux_power_enable.scu_pin, scu_config_normal_drive_t{.mode = map_aux_power_enable.gpio_mode, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},  // P6_7: !3.3V Aux Enable
        {map_aux_power_oc.scu_port, map_aux_power_oc.scu_pin, scu_config_normal_drive_t{.mode = map_aux_power_oc.gpio_mode, .epd = 0, .epun = 0, .ehs = 0, .ezi = 1, .zif = 0}},              // P6_11: !3.3V Aux overcurrent input
        {4, 9, scu_config_normal_drive_t{.mode = 4, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},                                                                                      // P4_9: TPS62410 mode
        {map_p1_ctrl0.scu_port, map_p1_ctrl0.scu_pin, scu_config_normal_drive_t{.mode = map_p1_ctrl0.gpio_mode, .epd = 1, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},                          // P2_10: P1_CTRL0
        {map_p1_ctrl2.scu_port, map_p1_ctrl2.scu_pin, scu_config_normal_drive_t{.mode = map_p1_ctrl2.gpio_mode, .epd = 0, .epun = 0, .ehs = 0, .ezi = 0, .zif = 0}},                          // P6_9: P1_CTRL2 (Output VCC, PU ON,)
        {map_p2_ctrl0.scu_port, map_p2_ctrl0.scu_pin, scu_config_normal_drive_t{.mode = map_p2_ctrl0.gpio_mode, .epd = 1, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},                          // PE_3: P2_CTRL0
        {map_p2_ctrl1.scu_port, map_p2_ctrl1.scu_pin, scu_config_normal_drive_t{.mode = map_p2_ctrl1.gpio_mode, .epd = 1, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},                          // PE_4: P2_CTRL1
        {map_ant_bias.scu_port, map_ant_bias.scu_pin, scu_config_normal_drive_t{.mode = map_ant_bias.gpio_mode, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},                          // P2_12: !ANT BIAS
        {map_ant_bias_oc.scu_port, map_ant_bias_oc.scu_pin, scu_config_normal_drive_t{.mode = map_ant_bias_oc.gpio_mode, .epd = 0, .epun = 0, .ehs = 0, .ezi = 1, .zif = 0}},                 // P2_11: !ANT BIAS overcurrent input
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

        {map_sgpio_0.scu_port, map_sgpio_0.scu_pin, scu_config_normal_drive_t{.mode = map_sgpio_0.gpio_mode, .epd = 0, .epun = 1, .ehs = 1, .ezi = 1, .zif = 1}},     // SGPIO0: HOST_DATA0(IO)
        {map_sgpio_1.scu_port, map_sgpio_1.scu_pin, scu_config_normal_drive_t{.mode = map_sgpio_1.gpio_mode, .epd = 0, .epun = 1, .ehs = 1, .ezi = 1, .zif = 1}},     // SGPIO1: HOST_DATA1(IO)
        {map_sgpio_2.scu_port, map_sgpio_2.scu_pin, scu_config_normal_drive_t{.mode = map_sgpio_2.gpio_mode, .epd = 0, .epun = 1, .ehs = 1, .ezi = 1, .zif = 1}},     // SGPIO2: HOST_DATA2(IO)
        {map_sgpio_3.scu_port, map_sgpio_3.scu_pin, scu_config_normal_drive_t{.mode = map_sgpio_3.gpio_mode, .epd = 0, .epun = 1, .ehs = 1, .ezi = 1, .zif = 1}},     // SGPIO3: HOST_DATA3(IO)
        {map_sgpio_4.scu_port, map_sgpio_4.scu_pin, scu_config_normal_drive_t{.mode = map_sgpio_4.gpio_mode, .epd = 0, .epun = 1, .ehs = 1, .ezi = 1, .zif = 1}},     // SGPIO4 HOST_DATA4(IO)
        {map_sgpio_5.scu_port, map_sgpio_5.scu_pin, scu_config_normal_drive_t{.mode = map_sgpio_5.gpio_mode, .epd = 0, .epun = 1, .ehs = 1, .ezi = 1, .zif = 1}},     // SGPIO5: HOST_DATA5(IO)
        {map_sgpio_6.scu_port, map_sgpio_6.scu_pin, scu_config_normal_drive_t{.mode = map_sgpio_6.gpio_mode, .epd = 0, .epun = 1, .ehs = 1, .ezi = 1, .zif = 1}},     // SGPIO6: HOST_DATA6(IO)
        {map_sgpio_7.scu_port, map_sgpio_7.scu_pin, scu_config_normal_drive_t{.mode = map_sgpio_7.gpio_mode, .epd = 0, .epun = 1, .ehs = 1, .ezi = 1, .zif = 1}},     // SGPIO7: HOST_DATA7(IO)
        {map_sgpio_8.scu_port, map_sgpio_8.scu_pin, scu_config_normal_drive_t{.mode = map_sgpio_8.gpio_mode, .epd = 0, .epun = 1, .ehs = 1, .ezi = 1, .zif = 1}},     // SGPIO8: CLK
        {map_sgpio_9.scu_port, map_sgpio_9.scu_pin, scu_config_normal_drive_t{.mode = map_sgpio_9.gpio_mode, .epd = 0, .epun = 1, .ehs = 1, .ezi = 1, .zif = 1}},     // SGPIO9: HOST_CAPTURE
        {map_sgpio_10.scu_port, map_sgpio_10.scu_pin, scu_config_normal_drive_t{.mode = map_sgpio_10.gpio_mode, .epd = 0, .epun = 1, .ehs = 1, .ezi = 1, .zif = 1}},  // SGPIO10: DISABLE
        {map_sgpio_11.scu_port, map_sgpio_11.scu_pin, scu_config_normal_drive_t{.mode = map_sgpio_11.gpio_mode, .epd = 0, .epun = 1, .ehs = 1, .ezi = 1, .zif = 1}},  // SGPIO11: HOST_DIRECTION(I)
        {map_sgpio_12.scu_port, map_sgpio_12.scu_pin, scu_config_normal_drive_t{.mode = map_sgpio_12.gpio_mode, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},  // SGPIO12: HOST_INVERT(I)
#ifdef PRALINE

        {6, 4, scu_config_normal_drive_t{.mode = 1, .epd = 0, .epun = 1, .ehs = 1, .ezi = 1, .zif = 1}},                                                                       // P6_4: SCT_CLK IN (Fast clock input, Mode 1)
        {4, 10, scu_config_normal_drive_t{.mode = 7, .epd = 1, .epun = 1, .ehs = 0, .ezi = 1, .zif = 0}},                                                                      // P4_10: FPGA Config Done (Input GND)
        {map_trigger_in.scu_port, map_trigger_in.scu_pin, scu_config_normal_drive_t{.mode = map_trigger_in.gpio_mode, .epd = 1, .epun = 1, .ehs = 0, .ezi = 1, .zif = 0}},     // PD_12: TRIGGER IN (Input GND, Mode 4)
        {map_trigger_out.scu_port, map_trigger_out.scu_pin, scu_config_normal_drive_t{.mode = map_trigger_out.gpio_mode, .epd = 0, .epun = 1, .ehs = 1, .ezi = 0, .zif = 0}},  // P2_6: TRIGGER
        {map_pps_in_out.scu_port, map_pps_in_out.scu_pin, scu_config_normal_drive_t{.mode = map_pps_in_out.gpio_mode, .epd = 0, .epun = 0, .ehs = 1, .ezi = 1, .zif = 0}},     // P2_5 PPS bidirectional
        {map_clkin_ctrl.scu_port, map_clkin_ctrl.scu_pin, scu_config_normal_drive_t{.mode = map_clkin_ctrl.gpio_mode, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},     // P1_20  CLKIN_ctrl
#else

        {4, 9, scu_config_normal_drive_t{.mode = 7, .epd = 0, .epun = 0, .ehs = 0, .ezi = 0, .zif = 0}},                                                                    // SGPIO14/BANK2F3M4: CPLD_P81
        {4, 10, scu_config_normal_drive_t{.mode = 7, .epd = 0, .epun = 0, .ehs = 0, .ezi = 0, .zif = 0}},                                                                   // SGPIO15/BANK2F3M6: CPLD_P78
        {2, 6, scu_config_normal_drive_t{.mode = 4, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 1}},                                                                    // MIXER_SCLK/P31: 33pF, RFFC5072.SCLK(I)
        {4, 5, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},                                                                    // RXENABLE
        {4, 6, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},                                                                    // XCVR_EN: 10K PD
        {1, 20, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},                                                                   // P1_20 CS_XCVR: MAX2837.CS(I)
        {map_hpf.scu_port, map_hpf.scu_pin, scu_config_normal_drive_t{.mode = map_hpf.gpio_mode, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},                       // P4_0 HPF
        {map_rx_amp.scu_port, map_rx_amp.scu_pin, scu_config_normal_drive_t{.mode = map_rx_amp.gpio_mode, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},              // P2_11 RX_AMP
        {map_rx_amp_pwr.scu_port, map_rx_amp_pwr.scu_pin, scu_config_normal_drive_t{.mode = map_rx_amp_pwr.gpio_mode, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},  // P2_12 !RX_AMP_PWR
        {map_tx_mix_bp.scu_port, map_tx_mix_bp.scu_pin, scu_config_normal_drive_t{.mode = map_tx_mix_bp.gpio_mode, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},     // P5_2 TX_MIX_BP
#endif

        // RADIO & RF PATH (MAX2831, RFFC5072, Mixers, Switches, Amps)
        {1, 3, scu_config_normal_drive_t{.mode = 5, .epd = 0, .epun = 0, .ehs = 0, .ezi = 1, .zif = 0}},                                               // P1_3 SSP1_MISO: MAX2837.DOUT(O)
        {1, 4, scu_config_normal_drive_t{.mode = 5, .epd = 0, .epun = 0, .ehs = 0, .ezi = 0, .zif = 0}},                                               // P1_4 SSP1_MOSI: MAX2837.DIN(I)
        {1, 19, scu_config_normal_drive_t{.mode = 1, .epd = 0, .epun = 0, .ehs = 0, .ezi = 1, .zif = 1}},                                              // P1_19 SSP1_SCK: MAX2837.SCLK(I)
        {map_lpf.scu_port, map_lpf.scu_pin, scu_config_normal_drive_t{.mode = map_lpf.gpio_mode, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},  // P5_1 LPF
#ifdef PRALINE
        {5, 4, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 0, .ehs = 0, .ezi = 0, .zif = 0}},                                                                             // P5_4: RFFC ENX
        {5, 5, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 0, .ehs = 0, .ezi = 0, .zif = 0}},                                                                             // P5_5: RFFC RESETX
        {9, 5, scu_config_normal_drive_t{.mode = 4, .epd = 0, .epun = 1, .ehs = 1, .ezi = 1, .zif = 1}},                                                                             // P9_5: RFFC5072 SCLK
        {9, 2, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 0, .ehs = 1, .ezi = 1, .zif = 0}},                                                                             // P9_2: RFFC5072 DATA (Bidirectional)
        {13, 11, scu_config_normal_drive_t{.mode = 4, .epd = 0, .epun = 0, .ehs = 0, .ezi = 1, .zif = 0}},                                                                           // PD_11: RFFC Lock Detect
        {13, 14, scu_config_normal_drive_t{.mode = 4, .epd = 0, .epun = 1, .ehs = 1, .ezi = 0, .zif = 0}},                                                                           // PD_14: MAX2831 CS
        {13, 15, scu_config_normal_drive_t{.mode = 4, .epd = 0, .epun = 1, .ehs = 1, .ezi = 1, .zif = 1}},                                                                           // PD_15: MAX2831 RXHP
        {13, 16, scu_config_normal_drive_t{.mode = 4, .epd = 0, .epun = 1, .ehs = 1, .ezi = 0, .zif = 0}},                                                                           // PD_16: MAX5864 CS
        {14, 1, scu_config_normal_drive_t{.mode = 4, .epd = 0, .epun = 1, .ehs = 1, .ezi = 0, .zif = 0}},                                                                            // PE_1: MAX2831 !SHDN
        {14, 2, scu_config_normal_drive_t{.mode = 4, .epd = 0, .epun = 1, .ehs = 1, .ezi = 1, .zif = 1}},                                                                            // PE_2: MAX2831 RXTX
        {map_p1_ctrl1.scu_port, map_p1_ctrl1.scu_pin, scu_config_normal_drive_t{.mode = map_p1_ctrl1.gpio_mode, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},                 // P6_8 P1_CTRL1 (Output GND, Mode 4)
        {map_aa_en.scu_port, map_aa_en.scu_pin, scu_config_normal_drive_t{.mode = map_aa_en.gpio_mode, .epd = 0, .epun = 1, .ehs = 1, .ezi = 1, .zif = 0}},                          // P1_14: AA_EN
        {map_rf5072_mix_en.scu_port, map_rf5072_mix_en.scu_pin, scu_config_normal_drive_t{.mode = map_rf5072_mix_en.gpio_mode, .epd = 1, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},  // P9_0: RF5072 MIX EN
        {9, 6, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 0, .ehs = 0, .ezi = 0, .zif = 0}},                                                                             // P9_6: MAX2831 LD Input
        {map_tx_enable.scu_port, map_tx_enable.scu_pin, scu_config_normal_drive_t{.mode = map_tx_enable.gpio_mode, .epd = 0, .epun = 1, .ehs = 1, .ezi = 0, .zif = 0}},              // P6_5: TX enable
        {10, 1, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 1, .ezi = 0, .zif = 0}},                                                                            // PA_1: LPF enable
        {10, 2, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 1, .ezi = 0, .zif = 0}},                                                                            // PA_2: RF amp enable
        {map_mix_bypass.scu_port, map_mix_bypass.scu_pin, scu_config_normal_drive_t{.mode = map_mix_bypass.gpio_mode, .epd = 0, .epun = 0, .ehs = 1, .ezi = 0, .zif = 0}},           // P6_3: MIX_ENABLE_N
        {map_rf_amp_enable.scu_port, map_rf_amp_enable.scu_pin, scu_config_normal_drive_t{.mode = map_rf_amp_enable.gpio_mode, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},  // PA_2 RF_AMP_EN
#else
        {map_rx_mix_bp.scu_port, map_rx_mix_bp.scu_pin, scu_config_normal_drive_t{.mode = map_rx_mix_bp.gpio_mode, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},              // P5_3 RX_MIX_BP
        {5, 4, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},                                                                             // MIXER_ENX
        {map_tx_amp.scu_port, map_tx_amp.scu_pin, scu_config_normal_drive_t{.mode = map_tx_amp.gpio_mode, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},                       // P5_6 TX_AMP
        {5, 7, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},                                                                             // P5_7 CS_AD, PRALINE: RFFC5072 CS
        {5, 5, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},                                                                             // MIXER_RESETX
        {6, 4, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 0, .ehs = 0, .ezi = 1, .zif = 0}},                                                                             // MIXER_SDATA
        {map_tx_mix_bypass.scu_port, map_tx_mix_bypass.scu_pin, scu_config_normal_drive_t{.mode = map_tx_mix_bypass.gpio_mode, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},  // P6_8 !TX MIX BYPASS
        {map_amp_bypass.scu_port, map_amp_bypass.scu_pin, scu_config_normal_drive_t{.mode = map_amp_bypass.gpio_mode, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},           // P2_10 AMP_BYPASS
        {map_tx_amp_pwr.scu_port, map_tx_amp_pwr.scu_pin, scu_config_normal_drive_t{.mode = map_tx_amp_pwr.gpio_mode, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},           // !TX_AMP_PWR
        {map_rx_mix_bypass.scu_port, map_rx_mix_bypass.scu_pin, scu_config_normal_drive_t{.mode = map_rx_mix_bypass.gpio_mode, .epd = 0, .epun = 0, .ehs = 1, .ezi = 0, .zif = 0}},  // P6_8 RX MIX BYPASS
#endif

// SAFE TERMINATION (Unused pins grounded to prevent noise & save power)

#ifdef PRALINE
        {4, 6, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 0, .ehs = 0, .ezi = 1, .zif = 0}},   // P4_6: Input VCC
        {6, 10, scu_config_normal_drive_t{.mode = 0, .epd = 1, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},  // P6_10: Output GND
        {4, 8, scu_config_normal_drive_t{.mode = 4, .epd = 1, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},   // P4_8: Output GND
        {8, 3, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},   // P8_3: Output GND
        {8, 8, scu_config_normal_drive_t{.mode = 0, .epd = 1, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},   // P8_8: Unused
        {10, 0, scu_config_normal_drive_t{.mode = 0, .epd = 1, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},  // PA_0: Unused
        {11, 2, scu_config_normal_drive_t{.mode = 4, .epd = 1, .epun = 1, .ehs = 0, .ezi = 1, .zif = 0}},  // PB_2: Unused IN
        {11, 4, scu_config_normal_drive_t{.mode = 4, .epd = 1, .epun = 1, .ehs = 0, .ezi = 1, .zif = 0}},  // PB_4: Unused IN
#else
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

        {map_isp.scu_port, map_isp.scu_pin, scu_config_normal_drive_t{.mode = map_isp.gpio_mode, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},                       // P2_7: ISP: 10K PU, Unused
        {map_dfu_boot_0.scu_port, map_dfu_boot_0.scu_pin, scu_config_normal_drive_t{.mode = map_dfu_boot_0.gpio_mode, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},  // P1_1: 10K PU, BOOT0
        {map_dfu_boot_1.scu_port, map_dfu_boot_1.scu_pin, scu_config_normal_drive_t{.mode = map_dfu_boot_1.gpio_mode, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},  // P1_2: 10K PD, BOOT1
        {map_dfu_button.scu_port, map_dfu_button.scu_pin, scu_config_normal_drive_t{.mode = map_dfu_button.gpio_mode, .epd = 1, .epun = 1, .ehs = 0, .ezi = 1, .zif = 1}},  // P2_8: 10K PD, BOOT2, DFU button
        {map_lcd_wrx.scu_port, map_lcd_wrx.scu_pin, scu_config_normal_drive_t{.mode = map_lcd_wrx.gpio_mode, .epd = 0, .epun = 1, .ehs = 0, .ezi = 1, .zif = 0}},           // P2_9: 10K PD, BOOT3, PortaPack LCD_WRX

    }};

#ifndef PRALINE
/* Additional GPIO configuration for HackRF OG */
static const std::array<gpio_setup_t, 6> gpio_setup_og{{
    {

        // GPIO0
        .data = 0,
        .dir = 0},
    {// GPIO1
     .data = 0,
     .dir = 0},
    {
        // GPIO2
        .data = boot_bit(og_VAA_en, 1)  // P5_0:  !VAA_ENABLE
                | (0 << 4)              // P4_4:  TXENABLE
        ,
        .dir = boot_bit(og_VAA_en, 1)  // P5_0:  !VAA_ENABLE
               | (1 << 4)              // P4_4:  TXENABLE
    },
    {
        // GPIO3
        .data = boot_bit(og_1v8_en, 0)  // P6_10: EN1V8, 10K PD
        ,
        .dir = boot_bit(og_1v8_en, 1)  // P6_10: EN1V8, 10K PD
    },
    {// GPIO4
     .data = 0,
     .dir = 0},
    {
        // GPIO5
        .data = boot_bit(og_tx, 0)       // P6_7:  TX
                | boot_bit(sgpio_13, 0)  // P4_8:  SGPIO13, HOST_SYNC_EN
                | boot_bit(og_rx, 1)     // P2_5:  RX
        ,
        .dir = boot_bit(og_tx, 1)       // P6_7:  TX
               | boot_bit(sgpio_13, 0)  // P4_8:  SGPIO13, HOST_SYNC_EN
               | boot_bit(og_rx, 1)     // P2_5:  RX
    },

}};

/* Additional GPIO configuration for HackRF r9 */
static const std::array<gpio_setup_t, 6> gpio_setup_r9{{
    {
        // GPIO0
        .data = boot_bit(r9_clkout_en, 0)     // P1_2:  10K PD, BOOT1, CLKOUT_EN
                | boot_bit(r9_mcu_clk_en, 1)  // P1_1:  10K PU, BOOT0, MCU_CLK_EN
                | boot_bit(r9_rx, 1)          // P2_7:  10K PU, ISP, RX
        ,
        .dir = boot_bit(r9_clkout_en, 1)     // P1_2:  10K PD, BOOT1, CLKOUT_EN
               | boot_bit(r9_mcu_clk_en, 1)  // P1_1:  10K PU, BOOT0, MCU_CLK_EN
               | boot_bit(r9_rx, 1)          // P2_7:  10K PU, ISP, RX
    },
    {// GPIO1
     .data = 0,
     .dir = 0},
    {
        // GPIO2
        .data = boot_bit(r9_1v8_en, 0)   // P5_0:  EN1V8, 10K PD
                | boot_bit(ant_bias, 1)  // P4_4:  !ANT_BIAS
        ,
        .dir = boot_bit(r9_1v8_en, 1)   // P5_0:  EN1V8, 10K PD
               | boot_bit(ant_bias, 1)  // P4_4:  !ANT_BIAS
    },
    {
        // GPIO3
        .data = boot_bit(r9_VAA_en, 1)  // P6_10: !VAA_ENABLE
        ,
        .dir = boot_bit(r9_VAA_en, 1)  // P6_10: !VAA_ENABLE
    },
    {// GPIO4
     .data = 0,
     .dir = 0},
    {
        // GPIO5
        .data = boot_bit(r9_clkin_en, 0)  // P6_7:  CLKIN_EN
                | (0 << 12)               // P4_8:  CLKIN_DETECT
                | (0 << 5)                // P2_5:  HOST_SYNC_EN
        ,
        .dir = boot_bit(r9_clkin_en, 1)  // P6_7:  CLKIN_EN
               | (0 << 12)               // P4_8:  CLKIN_DETECT
               | (0 << 5)                // P2_5:  HOST_SYNC_EN
    },
}};

/* Additional SCU configuration for HackRF OG */
static const std::array<scu_setup_t, 7> pins_setup_og{{

    /* Power control */
    {map_og_VAA_en.scu_port, map_og_VAA_en.scu_pin, scu_config_normal_drive_t{.mode = map_og_VAA_en.gpio_mode, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}}, /* !VAA_ENABLE: 10K PU, Q3.G(I), power to VAA */
    {map_og_1v8_en.scu_port, map_og_1v8_en.scu_pin, scu_config_normal_drive_t{.mode = map_og_1v8_en.gpio_mode, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}}, /* EN1V8/P70: 10K PD, TPS62410.EN2(I), 1V8LED.A(I) */

    /* Radio section control */
    {2, 5, scu_config_normal_drive_t{.mode = 4, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},                                                     /* RX/P43: U7.VCTL1(I), U10.VCTL1(I), U2.VCTL1(I) */
    {4, 4, scu_config_normal_drive_t{.mode = 0, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},                                                     /* TXENABLE/P55: MAX2837.TXENABLE(I) */
    {map_og_tx.scu_port, map_og_tx.scu_pin, scu_config_normal_drive_t{.mode = map_og_tx.gpio_mode, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},  /* TX/P42: U7.VCTL2(I), U10.VCTL2(I), U2.VCTL2(I) */
    {map_og_rx.scu_port, map_og_rx.scu_pin, scu_config_normal_drive_t{.mode = map_og_rx.gpio_mode, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},  // P2_5
    /* SGPIO for sample transfer interface to HackRF CPLD. */
    {map_sgpio_13.scu_port, map_sgpio_13.scu_pin, scu_config_normal_drive_t{.mode = map_sgpio_13.gpio_mode, .epd = 1, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}}, /* SGPIO13/BANK2F3M2: CPLD.90/HOST_SYNC_EN(I) */
}};

/* Additional SCU configuration for HackRF r9 */
static const std::array<scu_setup_t, 9> pins_setup_r9{{
    /* Power control */
    {map_r9_VAA_en.scu_port, map_r9_VAA_en.scu_pin, scu_config_normal_drive_t{.mode = map_r9_VAA_en.gpio_mode, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},  // !VAA_ENABLE: 10K PU, Q3.G(I), power to VAA
    {map_r9_1v8_en.scu_port, map_r9_1v8_en.scu_pin, scu_config_normal_drive_t{.mode = map_r9_1v8_en.gpio_mode, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},  // EN1V8: 10K PD, TPS62410.EN2(I), 1V8LED.A(I)

    /* Radio section control */
    {map_r9_rx.scu_port, map_r9_rx.scu_pin, scu_config_normal_drive_t{.mode = map_r9_rx.gpio_mode, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},           // RX/ISP/P96: U7.VCTL(I), U10.VCTL(I), U2.VCTL(I)
    {map_ant_bias.scu_port, map_ant_bias.scu_pin, scu_config_normal_drive_t{.mode = map_ant_bias.gpio_mode, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},  // !ANT_BIAS: 10K PU, Q4.G(I)

    /* SGPIO for sample transfer interface to HackRF CPLD. */
    {2, 5, scu_config_normal_drive_t{.mode = 4, .epd = 1, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}}, /* BANK2F3M2: CPLD.90/HOST_SYNC_EN(I) */

    /* Clock control */
    {map_r9_mcu_clk_en.scu_port, map_r9_mcu_clk_en.scu_pin, scu_config_normal_drive_t{.mode = map_r9_mcu_clk_en.gpio_mode, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},  // P1_1: MCU_CLK_EN/BOOT0: 10K PU, U28.1A(I)
    {map_r9_clkout_en.scu_port, map_r9_clkout_en.scu_pin, scu_config_normal_drive_t{.mode = map_r9_clkout_en.gpio_mode, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},     // P1_2: CLKOUT_EN/BOOT1: 10K PD, U28.2A(I)
    {map_r9_clkin_en.scu_port, map_r9_clkin_en.scu_pin, scu_config_normal_drive_t{.mode = map_r9_clkin_en.gpio_mode, .epd = 0, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},        /* CLKIN_EN: U16.SEL(I), U26.1A(I) */

    /* Miscellaneous */
    {4, 8, scu_config_normal_drive_t{.mode = 1, .epd = 1, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}}, /* CLKIN_DETECT: U26.2Y(O) */
}};
#endif  // #ifndef PRALINE

#endif

#ifdef PRALINE

static const std::array<scu_setup_t, 41> pins_setup_portapack{{
    {2, 0, scu_config_normal_drive_t{.mode = 4, .epd = 0, .epun = 1, .ehs = 0, .ezi = 1, .zif = 0}},  /* U0_TXD: PortaPack P2_0/IO_STBX */
    {2, 1, scu_config_normal_drive_t{.mode = 4, .epd = 0, .epun = 1, .ehs = 0, .ezi = 1, .zif = 0}},  /* U0_RXD: PortaPack P2_1/ADDR */
    {2, 3, scu_config_normal_drive_t{.mode = 4, .epd = 0, .epun = 1, .ehs = 0, .ezi = 1, .zif = 1}},  /* I2C1_SDA: PortaPack P2_3/LCD_TE */
    {2, 4, scu_config_normal_drive_t{.mode = 4, .epd = 0, .epun = 1, .ehs = 0, .ezi = 1, .zif = 1}},  /* I2C1_SCL: PortaPack P2_4/LCD_RDX */
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

    {1, 7, scu_config_normal_drive_t{.mode = 0, .epd = 1, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},   // P1_7: Output GND
    {5, 3, scu_config_normal_drive_t{.mode = 0, .epd = 1, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},   // P5_3 Output GND
    {5, 6, scu_config_normal_drive_t{.mode = 0, .epd = 1, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},   // P5_6 Output GND
    {5, 7, scu_config_normal_drive_t{.mode = 0, .epd = 1, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},   // P5_7 Output GND
    {9, 1, scu_config_normal_drive_t{.mode = 4, .epd = 1, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},   // P9_1: Output GND
    {10, 3, scu_config_normal_drive_t{.mode = 0, .epd = 1, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},  // PA_3: Output GND
    {10, 4, scu_config_normal_drive_t{.mode = 0, .epd = 1, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},  // PA_4: Output GND
    {11, 0, scu_config_normal_drive_t{.mode = 4, .epd = 1, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},  // PB_0: Output GND
    {11, 1, scu_config_normal_drive_t{.mode = 4, .epd = 1, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},  // PB_1: Output GND
    {11, 3, scu_config_normal_drive_t{.mode = 4, .epd = 1, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},  // PB_3: Output GND
    {11, 5, scu_config_normal_drive_t{.mode = 4, .epd = 1, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},  // PB_5: Output GND
    {14, 0, scu_config_normal_drive_t{.mode = 4, .epd = 1, .epun = 1, .ehs = 0, .ezi = 0, .zif = 0}},  // PE_0: Output GND
    {4, 3, scu_config_normal_drive_t{.mode = 0, .epd = 1, .epun = 1, .ehs = 0, .ezi = 1, .zif = 0}},   // P4_3: VBUSCTRL Input GND

}};

#else
static const std::array<scu_setup_t, 24> pins_setup_portapack{{
    {2, 0, scu_config_normal_drive_t{.mode = 4, .epd = 0, .epun = 1, .ehs = 0, .ezi = 1, .zif = 0}},  /* U0_TXD: PortaPack P2_0/IO_STBX */
    {2, 1, scu_config_normal_drive_t{.mode = 4, .epd = 0, .epun = 1, .ehs = 0, .ezi = 1, .zif = 0}},  /* U0_RXD: PortaPack P2_1/ADDR */
    {2, 3, scu_config_normal_drive_t{.mode = 4, .epd = 0, .epun = 1, .ehs = 0, .ezi = 1, .zif = 0}},  /* I2C1_SDA: PortaPack P2_3/LCD_TE */
    {2, 4, scu_config_normal_drive_t{.mode = 4, .epd = 0, .epun = 1, .ehs = 0, .ezi = 1, .zif = 0}},  /* I2C1_SCL: PortaPack P2_4/LCD_RDX */
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
    setup_pins(pins_setup_portapack);
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
    const uint32_t CORTEX_M4_CPUID = 0x410fc240;
    const uint32_t CORTEX_M4_CPUID_MASK = 0xff0ffff0;

    if ((SCB->CPUID & CORTEX_M4_CPUID_MASK) == CORTEX_M4_CPUID) {
        /* Enable unaligned exception handler */
        SCB->CCR |= (1 << 3);

        /* Enable MemManage, BusFault, UsageFault exception handlers */
        SCB->SHCSR |= (1 << 18) | (1 << 17) | (1 << 16);

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

/**
 * @brief   Board-specific initialization code.
 * @details Initializes LEDs, power rails, and configures physical pins
 * for both PRALINE and standard HackRF hardware variants.
 */
extern "C" void boardInit(void) {
#ifdef PRALINE
    /* HackRF Pro Specific: Initialize and Load FPGA */
    hackrf_r9 = false;

    power_control::aux_power_on();

    led_mcu.setActive();

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
    power_control::core_power_off();
    power_control::vaa_power_off();

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
