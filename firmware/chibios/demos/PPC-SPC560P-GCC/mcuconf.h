/*
    SPC5 HAL - Copyright (C) 2013 STMicroelectronics

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

/*
 * SPC560Pxx drivers configuration.
 * The following settings override the default settings present in
 * the various device driver implementation headers.
 * Note that the settings for each driver only have effect if the whole
 * driver is enabled in halconf.h.
 *
 * IRQ priorities:
 * 1...15       Lowest...Highest.
 */

#define SPC560Pxx_MCUCONF

/*
 * HAL driver system settings.
 */
#define SPC5_NO_INIT                        FALSE
#define SPC5_ALLOW_OVERCLOCK                FALSE
#define SPC5_DISABLE_WATCHDOG               TRUE
#define SPC5_FMPLL0_IDF_VALUE               5
#define SPC5_FMPLL0_NDIV_VALUE              32
#define SPC5_FMPLL0_ODF                     SPC5_FMPLL_ODF_DIV4
#define SPC5_FMPLL1_IDF_VALUE               5
#define SPC5_FMPLL1_NDIV_VALUE              60
#define SPC5_FMPLL1_ODF                     SPC5_FMPLL_ODF_DIV4
#define SPC5_AUX0CLK_SRC                    SPC5_CGM_SS_FMPLL1
#define SPC5_MCONTROL_DIVIDER_VALUE         2
#define SPC5_FMPLL1_CLK_DIVIDER_VALUE       2
#define SPC5_AUX2CLK_SRC                    SPC5_CGM_SS_FMPLL1
#define SPC5_SP_CLK_DIVIDER_VALUE           2
#define SPC5_AUX3CLK_SRC                    SPC5_CGM_SS_FMPLL1
#define SPC5_FR_CLK_DIVIDER_VALUE           2
#define SPC5_ME_ME_BITS                     (SPC5_ME_ME_RUN1 |              \
                                             SPC5_ME_ME_RUN2 |              \
                                             SPC5_ME_ME_RUN3 |              \
                                             SPC5_ME_ME_HALT0 |             \
                                             SPC5_ME_ME_STOP0)
#define SPC5_ME_TEST_MC_BITS                (SPC5_ME_MC_SYSCLK_IRC |        \
                                             SPC5_ME_MC_IRCON |             \
                                             SPC5_ME_MC_XOSC0ON |           \
                                             SPC5_ME_MC_PLL0ON |            \
                                             SPC5_ME_MC_PLL1ON |            \
                                             SPC5_ME_MC_CFLAON_NORMAL |     \
                                             SPC5_ME_MC_DFLAON_NORMAL |     \
                                             SPC5_ME_MC_MVRON)
#define SPC5_ME_SAFE_MC_BITS                (SPC5_ME_MC_PDO)
#define SPC5_ME_DRUN_MC_BITS                (SPC5_ME_MC_SYSCLK_FMPLL0 |     \
                                             SPC5_ME_MC_IRCON |             \
                                             SPC5_ME_MC_XOSC0ON |           \
                                             SPC5_ME_MC_PLL0ON |            \
                                             SPC5_ME_MC_PLL1ON |            \
                                             SPC5_ME_MC_CFLAON_NORMAL |     \
                                             SPC5_ME_MC_DFLAON_NORMAL |     \
                                             SPC5_ME_MC_MVRON)
#define SPC5_ME_RUN0_MC_BITS                (SPC5_ME_MC_SYSCLK_FMPLL0 |     \
                                             SPC5_ME_MC_IRCON |             \
                                             SPC5_ME_MC_XOSC0ON |           \
                                             SPC5_ME_MC_PLL0ON |            \
                                             SPC5_ME_MC_PLL1ON |            \
                                             SPC5_ME_MC_CFLAON_NORMAL |     \
                                             SPC5_ME_MC_DFLAON_NORMAL |     \
                                             SPC5_ME_MC_MVRON)
#define SPC5_ME_RUN1_MC_BITS                (SPC5_ME_MC_SYSCLK_FMPLL0 |     \
                                             SPC5_ME_MC_IRCON |             \
                                             SPC5_ME_MC_XOSC0ON |           \
                                             SPC5_ME_MC_PLL0ON |            \
                                             SPC5_ME_MC_PLL1ON |            \
                                             SPC5_ME_MC_CFLAON_NORMAL |     \
                                             SPC5_ME_MC_DFLAON_NORMAL |     \
                                             SPC5_ME_MC_MVRON)
#define SPC5_ME_RUN2_MC_BITS                (SPC5_ME_MC_SYSCLK_FMPLL0 |     \
                                             SPC5_ME_MC_IRCON |             \
                                             SPC5_ME_MC_XOSC0ON |           \
                                             SPC5_ME_MC_PLL0ON |            \
                                             SPC5_ME_MC_PLL1ON |            \
                                             SPC5_ME_MC_CFLAON_NORMAL |     \
                                             SPC5_ME_MC_DFLAON_NORMAL |     \
                                             SPC5_ME_MC_MVRON)
#define SPC5_ME_RUN3_MC_BITS                (SPC5_ME_MC_SYSCLK_FMPLL0 |     \
                                             SPC5_ME_MC_IRCON |             \
                                             SPC5_ME_MC_XOSC0ON |           \
                                             SPC5_ME_MC_PLL0ON |            \
                                             SPC5_ME_MC_PLL1ON |            \
                                             SPC5_ME_MC_CFLAON_NORMAL |     \
                                             SPC5_ME_MC_DFLAON_NORMAL |     \
                                             SPC5_ME_MC_MVRON)
#define SPC5_ME_HALT0_MC_BITS               (SPC5_ME_MC_SYSCLK_FMPLL0 |     \
                                             SPC5_ME_MC_IRCON |             \
                                             SPC5_ME_MC_XOSC0ON |           \
                                             SPC5_ME_MC_PLL0ON |            \
                                             SPC5_ME_MC_PLL1ON |            \
                                             SPC5_ME_MC_CFLAON_NORMAL |     \
                                             SPC5_ME_MC_DFLAON_NORMAL |     \
                                             SPC5_ME_MC_MVRON)
#define SPC5_ME_STOP0_MC_BITS               (SPC5_ME_MC_SYSCLK_FMPLL0 |     \
                                             SPC5_ME_MC_IRCON |             \
                                             SPC5_ME_MC_XOSC0ON |           \
                                             SPC5_ME_MC_PLL0ON |            \
                                             SPC5_ME_MC_PLL1ON |            \
                                             SPC5_ME_MC_CFLAON_NORMAL |     \
                                             SPC5_ME_MC_DFLAON_NORMAL |     \
                                             SPC5_ME_MC_MVRON)
#define SPC5_ME_RUN_PC3_BITS                (SPC5_ME_RUN_PC_RUN0 |          \
                                             SPC5_ME_RUN_PC_RUN1 |          \
                                             SPC5_ME_RUN_PC_RUN2 |          \
                                             SPC5_ME_RUN_PC_RUN3)
#define SPC5_ME_RUN_PC4_BITS                (SPC5_ME_RUN_PC_RUN0 |          \
                                             SPC5_ME_RUN_PC_RUN1 |          \
                                             SPC5_ME_RUN_PC_RUN2 |          \
                                             SPC5_ME_RUN_PC_RUN3)
#define SPC5_ME_RUN_PC5_BITS                (SPC5_ME_RUN_PC_RUN0 |          \
                                             SPC5_ME_RUN_PC_RUN1 |          \
                                             SPC5_ME_RUN_PC_RUN2 |          \
                                             SPC5_ME_RUN_PC_RUN3)
#define SPC5_ME_RUN_PC6_BITS                (SPC5_ME_RUN_PC_RUN0 |          \
                                             SPC5_ME_RUN_PC_RUN1 |          \
                                             SPC5_ME_RUN_PC_RUN2 |          \
                                             SPC5_ME_RUN_PC_RUN3)
#define SPC5_ME_RUN_PC7_BITS                (SPC5_ME_RUN_PC_RUN0 |          \
                                             SPC5_ME_RUN_PC_RUN1 |          \
                                             SPC5_ME_RUN_PC_RUN2 |          \
                                             SPC5_ME_RUN_PC_RUN3)
#define SPC5_ME_LP_PC4_BITS                 (SPC5_ME_LP_PC_HALT0 |          \
                                             SPC5_ME_LP_PC_STOP0)
#define SPC5_ME_LP_PC5_BITS                 (SPC5_ME_LP_PC_HALT0 |          \
                                             SPC5_ME_LP_PC_STOP0)
#define SPC5_ME_LP_PC6_BITS                 (SPC5_ME_LP_PC_HALT0 |          \
                                             SPC5_ME_LP_PC_STOP0)
#define SPC5_ME_LP_PC7_BITS                 (SPC5_ME_LP_PC_HALT0 |          \
                                             SPC5_ME_LP_PC_STOP0)
#define SPC5_PIT0_IRQ_PRIORITY              4
#define SPC5_CLOCK_FAILURE_HOOK()           chSysHalt()

/*
 * SERIAL driver system settings.
 */
#define SPC5_SERIAL_USE_LINFLEX0            TRUE
#define SPC5_SERIAL_USE_LINFLEX1            TRUE
#define SPC5_SERIAL_LINFLEX0_PRIORITY       8
#define SPC5_SERIAL_LINFLEX1_PRIORITY       8
#define SPC5_SERIAL_LINFLEX0_START_PCTL     (SPC5_ME_PCTL_RUN(1) |          \
                                             SPC5_ME_PCTL_LP(2))
#define SPC5_SERIAL_LINFLEX0_STOP_PCTL      (SPC5_ME_PCTL_RUN(0) |          \
                                             SPC5_ME_PCTL_LP(0))
#define SPC5_SERIAL_LINFLEX1_START_PCTL     (SPC5_ME_PCTL_RUN(1) |          \
                                             SPC5_ME_PCTL_LP(2))
#define SPC5_SERIAL_LINFLEX1_STOP_PCTL      (SPC5_ME_PCTL_RUN(0) |          \
                                             SPC5_ME_PCTL_LP(0))
