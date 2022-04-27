/*
    ChibiOS/RT - Copyright (C) 2006-2013 Giovanni Di Sirio

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
 * @file    AT91SAM7/gpt_lld.c
 * @brief   AT91SAM7 GPT subsystem low level driver source.
 *
 * @addtogroup GPT
 * @{
 */

#include "ch.h"
#include "hal.h"

#if HAL_USE_GPT || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/**
 * @brief   GPTD1 driver identifier.
 * @note    The driver GPTD1 allocates the complex timer TC0 when enabled.
 */
#if AT91_GPT_USE_TC0 || defined(__DOXYGEN__)
	GPTDriver GPTD1;
#endif

/**
 * @brief   GPTD2 driver identifier.
 * @note    The driver GPTD2 allocates the timer TC1 when enabled.
 */
#if AT91_GPT_USE_TC1 || defined(__DOXYGEN__)
	GPTDriver GPTD2;
#endif

/**
 * @brief   GPTD3 driver identifier.
 * @note    The driver GPTD3 allocates the timer TC2 when enabled.
 */
#if AT91_GPT_USE_TC2 || defined(__DOXYGEN__)
	GPTDriver GPTD3;
#endif

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/**
 * @brief   Shared IRQ handler.
 *
 * @param[in] gptp      pointer to a @p GPTDriver object
 */
static void gpt_lld_serve_interrupt(GPTDriver *gptp) {
	// Read the status to clear the interrupts
	{ uint32_t	isr = gptp->tc->TC_SR; (void) isr; }

	// Do the callback
	gptp->config->callback(gptp);
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

#if AT91_GPT_USE_TC0
	/**
	 * @brief   TC1 interrupt handler.
	 *
	 * @isr
	 */
	CH_IRQ_HANDLER(TC0_IRQHandler) {
		CH_IRQ_PROLOGUE();
		gpt_lld_serve_interrupt(&GPTD1);
		AT91C_BASE_AIC->AIC_EOICR = 0;
		CH_IRQ_EPILOGUE();
	}
#endif /* AT91_GPT_USE_TC0 */

#if AT91_GPT_USE_TC1
	/**
	 * @brief   TC1 interrupt handler.
	 *
	 * @isr
	 */
	CH_IRQ_HANDLER(TC1_IRQHandler) {
		CH_IRQ_PROLOGUE();
		gpt_lld_serve_interrupt(&GPTD2);
		AT91C_BASE_AIC->AIC_EOICR = 0;
		CH_IRQ_EPILOGUE();
	}
#endif /* AT91_GPT_USE_TC1 */

#if AT91_GPT_USE_TC2
	/**
	 * @brief   TC1 interrupt handler.
	 *
	 * @isr
	 */
	CH_IRQ_HANDLER(TC2_IRQHandler) {
		CH_IRQ_PROLOGUE();
		gpt_lld_serve_interrupt(&GPTD2);
		AT91C_BASE_AIC->AIC_EOICR = 0;
		CH_IRQ_EPILOGUE();
	}
}
#endif /* AT91_GPT_USE_TC2 */

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level GPT driver initialization.
 *
 * @notapi
 */
void gpt_lld_init(void) {

	#if AT91_GPT_USE_TC0
		AT91C_BASE_PMC->PMC_PCER = (1 << AT91C_ID_TC0);			// Turn on the power
		GPTD1.tc = AT91C_BASE_TC0;
		gptObjectInit(&GPTD1);
		gpt_lld_stop(&GPTD1);									// Make sure it is disabled
		AIC_ConfigureIT(AT91C_ID_TC0, AT91C_AIC_SRCTYPE_HIGH_LEVEL | AT91_GPT_TC0_IRQ_PRIORITY, TC0_IRQHandler);
		AIC_EnableIT(AT91C_ID_TC0);
	#endif

	#if AT91_GPT_USE_TC1
		AT91C_BASE_PMC->PMC_PCER = (1 << AT91C_ID_TC1);			// Turn on the power
		GPTD2.tc = AT91C_BASE_TC1;
		gptObjectInit(&GPTD2);
		gpt_lld_stop(&GPTD2);									// Make sure it is disabled
		AIC_ConfigureIT(AT91C_ID_TC1, AT91C_AIC_SRCTYPE_HIGH_LEVEL | AT91_GPT_TC1_IRQ_PRIORITY, TC1_IRQHandler);
		AIC_EnableIT(AT91C_ID_TC1);
	#endif

	#if AT91_GPT_USE_TC2
		AT91C_BASE_PMC->PMC_PCER = (1 << AT91C_ID_TC2);			// Turn on the power
		GPTD3.tc = AT91C_BASE_TC2;
		gptObjectInit(&GPTD3);
		gpt_lld_stop(&GPTD3);									// Make sure it is disabled
		AIC_ConfigureIT(AT91C_ID_TC2, AT91C_AIC_SRCTYPE_HIGH_LEVEL | AT91_GPT_TC2_IRQ_PRIORITY, TC2_IRQHandler);
		AIC_EnableIT(AT91C_ID_TC2);
	#endif
}

/**
 * @brief   Configures and activates the GPT peripheral.
 *
 * @param[in] gptp      pointer to the @p GPTDriver object
 *
 * @notapi
 */
void gpt_lld_start(GPTDriver *gptp) {
	uint32_t	cmr, bmr;

	bmr = *AT91C_TCB_BMR;
	cmr = (AT91C_TC_ASWTRG_CLEAR | AT91C_TC_ACPC_CLEAR | AT91C_TC_ACPA_SET |
            AT91C_TC_WAVE  | AT91C_TC_WAVESEL_UP_AUTO);

	// Calculate clock
	switch(gptp->config->clocksource) {
	case GPT_CLOCK_MCLK:
		switch(gptp->config->frequency) {
		case MCK/2:		cmr |= AT91C_TC_CLKS_TIMER_DIV1_CLOCK;	break;
		case MCK/8:		cmr |= AT91C_TC_CLKS_TIMER_DIV2_CLOCK;	break;
		case MCK/32:	cmr |= AT91C_TC_CLKS_TIMER_DIV3_CLOCK;	break;
		case MCK/128:	cmr |= AT91C_TC_CLKS_TIMER_DIV4_CLOCK;	break;
		case MCK/1024:	cmr |= AT91C_TC_CLKS_TIMER_DIV5_CLOCK;	break;
		default:
			chDbgAssert(TRUE, "gpt_lld_start(), #1", "invalid frequency");
			cmr |= AT91C_TC_CLKS_TIMER_DIV5_CLOCK;
			break;
		}
		break;
	case GPT_CLOCK_FREQUENCY:
	    /* The mode and period will be calculated when the timer is started */
		cmr |= AT91C_TC_CLKS_TIMER_DIV5_CLOCK;
		break;
	case GPT_CLOCK_RE_TCLK0:
	case GPT_CLOCK_FE_TCLK0:
		if ((gptp->config->clocksource & 1)) cmr |= AT91C_TC_CLKI;
		cmr |= AT91C_TC_CLKS_XC0;
		#if AT91_GPT_USE_TC0
			if (gptp == &GPTD1) bmr = (bmr & ~AT91C_TCB_TC0XC0S) | AT91C_TCB_TC0XC0S_TCLK0;
		#endif
		break;
	case GPT_CLOCK_RE_TCLK1:
	case GPT_CLOCK_FE_TCLK1:
		if ((gptp->config->clocksource & 1)) cmr |= AT91C_TC_CLKI;
		cmr |= AT91C_TC_CLKS_XC1;
		#if AT91_GPT_USE_TC1
			if (gptp == &GPTD2) bmr = (bmr & ~AT91C_TCB_TC1XC1S) | AT91C_TCB_TC1XC1S_TCLK1;
		#endif
		break;
	case GPT_CLOCK_RE_TCLK2:
	case GPT_CLOCK_FE_TCLK2:
		if ((gptp->config->clocksource & 1)) cmr |= AT91C_TC_CLKI;
		cmr |= AT91C_TC_CLKS_XC2;
		#if AT91_GPT_USE_TC2
			if (gptp == &GPTD3) bmr = (bmr & ~AT91C_TCB_TC2XC2S) | AT91C_TCB_TC2XC2S_TCLK2;
		#endif
		break;
	case GPT_CLOCK_RE_TC0:
	case GPT_CLOCK_FE_TC0:
		if ((gptp->config->clocksource & 1)) cmr |= AT91C_TC_CLKI;
		#if AT91_GPT_USE_TC0
			if (gptp == &GPTD1) {
				chDbgAssert(TRUE, "gpt_lld_start(), #2", "invalid clock");
				cmr |= AT91C_TC_CLKS_XC0;
				bmr = (bmr & ~AT91C_TCB_TC0XC0S) | AT91C_TCB_TC0XC0S_NONE;
				break;
			}
		#endif
		#if AT91_GPT_USE_TC1
			if (gptp == &GPTD2) {
				cmr |= AT91C_TC_CLKS_XC1;
				bmr = (bmr & ~AT91C_TCB_TC1XC1S) | AT91C_TCB_TC1XC1S_TIOA0;
				break;
			}
		#endif
		#if AT91_GPT_USE_TC2
			if (gptp == &GPTD3) {
				cmr |= AT91C_TC_CLKS_XC2;
				bmr = (bmr & ~AT91C_TCB_TC2XC2S) | AT91C_TCB_TC2XC2S_TIOA0;
				break;
			}
		#endif
		chDbgAssert(TRUE, "gpt_lld_start(), #3", "invalid GPT device");
		cmr |= AT91C_TC_CLKS_TIMER_DIV5_CLOCK;
		break;
	case GPT_CLOCK_RE_TC1:
	case GPT_CLOCK_FE_TC1:
		if ((gptp->config->clocksource & 1)) cmr |= AT91C_TC_CLKI;
		#if AT91_GPT_USE_TC0
			if (gptp == &GPTD1) {
				cmr |= AT91C_TC_CLKS_XC0;
				bmr = (bmr & ~AT91C_TCB_TC0XC0S) | AT91C_TCB_TC0XC0S_TIOA1;
				break;
			}
		#endif
		#if AT91_GPT_USE_TC1
			if (gptp == &GPTD2) {
				chDbgAssert(TRUE, "gpt_lld_start(), #4", "invalid clock");
				cmr |= AT91C_TC_CLKS_XC1;
				bmr = (bmr & ~AT91C_TCB_TC1XC1S) | AT91C_TCB_TC1XC1S_NONE;
				break;
			}
		#endif
		#if AT91_GPT_USE_TC2
			if (gptp == &GPTD3) {
				cmr |= AT91C_TC_CLKS_XC2;
				bmr = (bmr & ~AT91C_TCB_TC2XC2S) | AT91C_TCB_TC2XC2S_TIOA1;
				break;
			}
		#endif
		chDbgAssert(TRUE, "gpt_lld_start(), #5", "invalid GPT device");
		cmr |= AT91C_TC_CLKS_TIMER_DIV5_CLOCK;
		break;
	case GPT_CLOCK_RE_TC2:
	case GPT_CLOCK_FE_TC2:
		if ((gptp->config->clocksource & 1)) cmr |= AT91C_TC_CLKI;
		#if AT91_GPT_USE_TC0
			if (gptp == &GPTD1) {
				cmr |= AT91C_TC_CLKS_XC0;
				bmr = (bmr & ~AT91C_TCB_TC0XC0S) | AT91C_TCB_TC0XC0S_TIOA2;
				break;
			}
		#endif
		#if AT91_GPT_USE_TC1
			if (gptp == &GPTD2) {
				cmr |= AT91C_TC_CLKS_XC1;
				bmr = (bmr & ~AT91C_TCB_TC1XC1S) | AT91C_TCB_TC1XC1S_TIOA2;
				break;
			}
		#endif
		#if AT91_GPT_USE_TC2
			if (gptp == &GPTD3) {
				chDbgAssert(TRUE, "gpt_lld_start(), #6", "invalid clock");
				cmr |= AT91C_TC_CLKS_XC2;
				bmr = (bmr & ~AT91C_TCB_TC2XC2S) | AT91C_TCB_TC2XC2S_NONE;
				break;
			}
		#endif
		chDbgAssert(TRUE, "gpt_lld_start(), #7", "invalid GPT device");
		cmr |= AT91C_TC_CLKS_TIMER_DIV5_CLOCK;
		break;
	default:
		chDbgAssert(TRUE, "gpt_lld_start(), #8", "invalid clock");
		cmr |= AT91C_TC_CLKS_TIMER_DIV5_CLOCK;
		break;
	}

	// Calculate clock gating
	chDbgAssert(gptp->config->clockgate == GPT_GATE_NONE || gptp->config->clockgate == GPT_GATE_TCLK0
			|| gptp->config->clockgate == GPT_GATE_TCLK1 || gptp->config->clockgate == GPT_GATE_TCLK2
			, "gpt_lld_start(), #9", "invalid clockgate");
	cmr |= ((uint32_t)(gptp->config->clockgate & 0x03)) << 4;		// special magic numbers here

	// Calculate triggers
	chDbgAssert(gptp->config->trigger == GPT_TRIGGER_NONE
			|| gptp->config->trigger == GPT_TRIGGER_RE_TIOB  || gptp->config->trigger == GPT_TRIGGER_FE_TIOB  || gptp->config->trigger == GPT_TRIGGER_BE_TIOB
			|| gptp->config->trigger == GPT_TRIGGER_RE_TCLK0 || gptp->config->trigger == GPT_TRIGGER_FE_TCLK0 || gptp->config->trigger == GPT_TRIGGER_BE_TCLK0
			|| gptp->config->trigger == GPT_TRIGGER_RE_TCLK1 || gptp->config->trigger == GPT_TRIGGER_FE_TCLK1 || gptp->config->trigger == GPT_TRIGGER_BE_TCLK1
			|| gptp->config->trigger == GPT_TRIGGER_RE_TCLK2 || gptp->config->trigger == GPT_TRIGGER_FE_TCLK2 || gptp->config->trigger == GPT_TRIGGER_BE_TCLK2
			, "gpt_lld_start(), #10", "invalid trigger");
	cmr |= ((uint32_t)(gptp->config->trigger & 0x03)) << 10;			// special magic numbers here
	cmr |= ((uint32_t)(gptp->config->trigger & 0x30)) << (8-4);		// special magic numbers here

    /* Set everything up but disabled */
	gptp->tc->TC_CCR = AT91C_TC_CLKDIS;
	gptp->tc->TC_IDR = 0xFFFFFFFF;
	gptp->tc->TC_CMR = cmr;
	gptp->tc->TC_RC = 65535;
	gptp->tc->TC_RA = 32768;
	*AT91C_TCB_BMR = bmr;
	cmr = gptp->tc->TC_SR;			// Clear any pending interrupts
}

/**
 * @brief   Deactivates the GPT peripheral.
 *
 * @param[in] gptp      pointer to the @p GPTDriver object
 *
 * @notapi
 */
void gpt_lld_stop(GPTDriver *gptp) {
	gptp->tc->TC_CCR = AT91C_TC_CLKDIS;
	gptp->tc->TC_IDR = 0xFFFFFFFF;
	{ uint32_t isr = gptp->tc->TC_SR; (void)isr; }
}

/**
 * @brief   Starts the timer in continuous mode.
 *
 * @param[in] gptp      pointer to the @p GPTDriver object
 * @param[in] interval  period in ticks
 *
 * @notapi
 */
void gpt_lld_start_timer(GPTDriver *gptp, gptcnt_t interval) {
	gpt_lld_change_interval(gptp, interval);
	if (gptp->state == GPT_ONESHOT)
		gptp->tc->TC_CMR |= AT91C_TC_CPCDIS;
	else
		gptp->tc->TC_CMR &= ~AT91C_TC_CPCDIS;
	gptp->tc->TC_CCR = AT91C_TC_CLKEN|AT91C_TC_SWTRG;
	if (gptp->config->callback)
		gptp->tc->TC_IER = AT91C_TC_CPCS|AT91C_TC_COVFS;
}

/**
 * @brief   Stops the timer.
 *
 * @param[in] gptp      pointer to the @p GPTDriver object
 *
 * @notapi
 */
void gpt_lld_stop_timer(GPTDriver *gptp) {
	gptp->tc->TC_CCR = AT91C_TC_CLKDIS;
	gptp->tc->TC_IDR = 0xFFFFFFFF;
	{ uint32_t isr = gptp->tc->TC_SR; (void)isr; }
}

/**
 * @brief   Changes the interval of GPT peripheral.
 * @details This function changes the interval of a running GPT unit.
 * @pre     The GPT unit must have been activated using @p gptStart().
 * @pre     The GPT unit must have been running in continuous mode using
 *          @p gptStartContinuous().
 * @post    The GPT unit interval is changed to the new value.
 * @note    The function has effect at the next cycle start.
 *
 * @param[in] gptp      pointer to a @p GPTDriver object
 * @param[in] interval  new cycle time in timer ticks
 * @notapi
 */
void gpt_lld_change_interval(GPTDriver *gptp, gptcnt_t interval) {
	if (gptp->config->clocksource == GPT_CLOCK_FREQUENCY) {
		uint32_t	rc, cmr;

		// Reset the timer to the (possibly) new frequency value
		rc = (MCK/2)/gptp->config->frequency;
		if (rc < (0x10000<<0)) {
			cmr = AT91C_TC_CLKS_TIMER_DIV1_CLOCK;
		} else if (rc < (0x10000<<2)) {
			rc >>= 2;
			cmr = AT91C_TC_CLKS_TIMER_DIV2_CLOCK;
		} else if (rc < (0x10000<<4)) {
			rc >>= 4;
			cmr = AT91C_TC_CLKS_TIMER_DIV3_CLOCK;
		} else if (rc < (0x10000<<6)) {
			rc >>= 6;
			cmr = AT91C_TC_CLKS_TIMER_DIV4_CLOCK;
		} else {
			rc >>= 9;
			cmr = AT91C_TC_CLKS_TIMER_DIV5_CLOCK;
		}
		gptp->tc->TC_CMR = (gptp->tc->TC_CMR & ~AT91C_TC_CLKS) | cmr;
		gptp->tc->TC_RC = rc;
		gptp->tc->TC_RA = rc/2;
	} else {
		gptp->tc->TC_RC = interval;
		gptp->tc->TC_RA = interval/2;
	}
}

/**
 * @brief   Starts the timer in one shot mode and waits for completion.
 * @details This function specifically polls the timer waiting for completion
 *          in order to not have extra delays caused by interrupt servicing,
 *          this function is only recommended for short delays.
 *
 * @param[in] gptp      pointer to the @p GPTDriver object
 * @param[in] interval  time interval in ticks
 *
 * @notapi
 */
void gpt_lld_polled_delay(GPTDriver *gptp, gptcnt_t interval) {

	gpt_lld_change_interval(gptp, interval);
	gptp->tc->TC_CMR |= AT91C_TC_CPCDIS;
	gptp->tc->TC_CCR = AT91C_TC_CLKEN|AT91C_TC_SWTRG;
	while (!(gptp->tc->TC_SR & (AT91C_TC_CPCS|AT91C_TC_COVFS)));
}

#endif /* HAL_USE_GPT */

/** @} */
