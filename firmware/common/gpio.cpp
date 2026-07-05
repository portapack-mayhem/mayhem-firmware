/*
 * Copyright (C) 2026 Pezsma
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

#include "gpio.hpp"

#include "platform.hpp"

using namespace gpio_control;

namespace power_control {

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
    VAA_en.setActive();

#else
    if (hackrf_r9) {
        /*
         * There is enough VCC->VAA leakage prior to VAA activation from IO pins on
         * HackRF One r9 that slowing down activation like this isn't necessary, but
         * we do it just in case a different start-up sequence in the future results
         * in less leakage.
         */
        setup_pin(pin_setup_vaa_enablex_gpio_r9);  // P6_10 GPIO3[ 6]: !VAA_ENABLE, 10K PU
        for (uint32_t i = 0; i < 1000; i++) {
            LPC_GPIO->W3[6] = 1;
            LPC_GPIO->W3[6] = 0;
        }
    } else {
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
        setup_pin(pin_setup_vaa_enablex_pwm);  // P5_0 /GPIO2[ 9]/MCOB2: !VAA_ENABLE, 10K PU

        /* Start the PWM operation. */
        LPC_MCPWM->CON_SET = (1 << 16);

        /* Wait until VAA rises to approximately 90% of final voltage. */
        /* Timing assumes we're running immediately after the bootloader: 96 MHz from IRC+PLL1
         */
        while (enable_period < cycle_period) {
            {
                volatile uint32_t delay = 2000;
                while (delay--);
            }
            enable_period <<= 1;
            LPC_MCPWM->MAT2 = cycle_period - enable_period;
        }

        /* Hold !VAA_ENABLE active using a GPIO, so we can reclaim and shut down the MOTOCONPWM peripheral. */
        og_VAA_en.output();
        og_VAA_en.setActive();
        setup_pin(pin_setup_vaa_enablex_gpio_og);  // P5_0 /GPIO2[ 9]/MCOB2: !VAA_ENABLE, 10K PU

        peripheral_reset(&motocon_pwm_resources.reset);
        branch_clock_disable(&motocon_pwm_resources.branch);
        base_clock_disable(&motocon_pwm_resources.base);
    }

#endif
}

void vaa_power_off(void) {
    /* TODO: There's a lot of other stuff that must be done to prevent
     * leakage from +3V3 into VAA.
     */
#ifdef PRALINE
    /* Safe state: OFF (VAA RF is active LOW, so Set = OFF) */
    VAA_en.setInactive();

#else
    if (hackrf_r9) {
        r9_VAA_en.setInactive();  // Turn OFF VAA for r9 P6_10
    } else {
        og_VAA_en.setInactive();  // Turn OFF VAA for OG P5_0
    }
#endif
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

void core_power_on(void) {
#ifdef PRALINE
    // 1.2V FPGA - P8_7 = GPIO4[7], Active HIGH (Set = ON)
    en_1v2.setActive();

#else
    if (hackrf_r9) {
        r9_1v8_en.setActive();
    } else {
        og_1v8_en.setActive();
    }
#endif
}

void core_power_off(void) {
#ifdef PRALINE
    en_1v2.setInactive();

#else
    if (hackrf_r9) {
        r9_1v8_en.setInactive();
    } else {
        og_1v8_en.setInactive();
    }
#endif
}
}  // namespace power_control