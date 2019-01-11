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

/**
 * @file    templates/pal_lld.c
 * @brief   PAL subsystem low level driver template.
 *
 * @addtogroup PAL
 * @{
 */

#include "ch.h"
#include "hal.h"

#if HAL_USE_PAL || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

static const scu_config_t pin_config_vaa_enablex_pwm  = { .MODE=1, .EPD=0, .EPUN=1, .EHS=0, .EZI=0, .ZIF=0 };
static const scu_config_t pin_config_vaa_enablex_gpio = { .MODE=0, .EPD=0, .EPUN=1, .EHS=0, .EZI=0, .ZIF=0 };

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/


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
static void vaa_power_on(void) {
  /* Very twitchy process for powering up VAA without glitching the 3.3V rail, which can send the
   * microcontroller into reset.
   *
   * Controlling timing while running from SPIFI flash is tricky, hence use of a PWM peripheral...
   */

  /* Configure and enable MOTOCONPWM peripheral clocks.
   * Assume IDIVC is running the post-bootloader configuration, outputting 96MHz derived from PLL1.
   */
  LPC_CCU1->CLK_APB1_MOTOCON_PWM_CFG.RUN = true;

  /* Combination of pulse duration and duty cycle was arrived at empirically, to keep supply glitching
   * to +/- 0.15V.
   */
  const uint32_t cycle_period = 128;
  const uint32_t enable_period = 10;
  LPC_MCPWM->TC2 = 0;
  LPC_MCPWM->MAT2 = cycle_period - enable_period;
  LPC_MCPWM->LIM2 = cycle_period;

  /* Switch !VAA_ENABLE pin from GPIO to MOTOCONPWM peripheral output, now that the peripheral is configured. */
  LPC_SCU->SFSP[5][ 0] = pin_config_vaa_enablex_pwm.word; // P5_0 /GPIO2[ 9]/MCOB2: !VAA_ENABLE, 10K PU

  /* Start the PWM operation. */
  LPC_MCPWM->CON_SET = (1 << 16);

  /* Wait until VAA rises to approximately 90% of final voltage. */
  /* Timing assumes we're running immediately after the bootloader: 96 MHz from IRC+PLL1
   */
  { volatile uint32_t delay = 12000; while(delay--); }

  /* Hold !VAA_ENABLE active using a GPIO, so we can reclaim and shut down the MOTOCONPWM peripheral. */
  LPC_GPIO->CLR[2]  = (1 << 9); // !VAA_ENABLE
  LPC_GPIO->DIR[2] |= (1 << 9);
  LPC_SCU->SFSP[5][ 0] = pin_config_vaa_enablex_gpio.word; // P5_0 /GPIO2[ 9]/MCOB2: !VAA_ENABLE, 10K PU

  /* Reset the MOTOCONPWM peripheral. */
  LPC_RGU->RESET_CTRL[1] = (1U << 6);

  /* Shut down the MOTOCONPWM clocks. */
  LPC_CCU1->CLK_APB1_MOTOCON_PWM_CFG.RUN = false;
}

static void vaa_power_off(void) {
  // TODO: There's a lot of other stuff that must be done to prevent
  // leakage from +3V3 into VAA.
  LPC_GPIO->W2[9] = 1;
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))

/**
 * @brief   LPC43xx I/O ports configuration.
 * @details Ports 0 through 8.
 *
 * @param[in] config    the STM32 ports configuration
 *
 * @notapi
 */
void _pal_lld_init(const PALConfig *config) {
  for(size_t i=0; i<8; i++) {
    LPC_GPIO->PIN[i] = config->P[i].data;
    LPC_GPIO->DIR[i] = config->P[i].dir;
  }

  for(size_t i=0; i<ARRAY_SIZE(config->SCU); i++) {
    LPC_SCU->SFSP[config->SCU[i].port][config->SCU[i].pin] = config->SCU[i].config.word;
  }

  vaa_power_on();
}

/**
 * @brief   Pads mode setup.
 * @details This function programs a pads group belonging to the same port
 *          with the specified mode.
 *
 * @param[in] port      the port identifier
 * @param[in] mask      the group mask
 * @param[in] mode      the mode
 *
 * @notapi
 */
void _pal_lld_setgroupmode(ioportid_t port,
                           ioportmask_t mask,
                           iomode_t mode) {
  /* TODO: Handling pull-up, pull-down modes not implemented, as it would
   * require a map from GPIO to SCU pins. Not today.
   */
  if( mode == PAL_MODE_OUTPUT_PUSHPULL ) {
    LPC_GPIO->DIR[port] |= mask;
  } else {
    LPC_GPIO->DIR[port] &= ~mask;
  }
}

#endif /* HAL_USE_PAL */

/** @} */
