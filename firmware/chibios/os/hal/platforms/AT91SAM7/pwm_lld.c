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
/*
   This file has been contributed by:
		Andrew Hannam aka inmarket.
*/

/**
 * @file    AT91SAM7/pwm_lld.c
 * @brief   AT91SAM7 PWM Driver subsystem low level driver source.
 *
 * @addtogroup PWM
 * @{
 */

#include "ch.h"
#include "hal.h"

#if HAL_USE_PWM || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

#ifdef UNUSED 
#elif defined(__GNUC__) 
# define UNUSED(x) UNUSED_ ## x __attribute__((unused)) 
#elif defined(__LCLINT__) 
# define UNUSED(x) /*@unused@*/ x 
#else 
# define UNUSED(x) x 
#endif 

#define PWMC_M	((AT91S_PWMC *)AT91C_PWMC_MR)

#define PWM_MCK_MASK					0x0F00
#define PWM_MCK_SHIFT					8

typedef struct pindef {
	uint32_t	portpin;	/* Set to 0 if this pin combination is invalid */
	AT91S_PIO	*pio;
	AT91_REG	*perab;
} pindef_t;

typedef struct pwmpindefs {
	pindef_t	pin[3];
} pwmpindefs_t;

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

#if PWM_USE_PWM1 && !defined(__DOXYGEN__)
PWMDriver PWMD1;
#endif

#if PWM_USE_PWM2 && !defined(__DOXYGEN__)
PWMDriver PWMD2;
#endif

#if PWM_USE_PWM3 && !defined(__DOXYGEN__)
PWMDriver PWMD3;
#endif

#if PWM_USE_PWM4 && !defined(__DOXYGEN__)
PWMDriver PWMD4;
#endif

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

#if (SAM7_PLATFORM == SAM7S64) || (SAM7_PLATFORM == SAM7S128) ||            \
    (SAM7_PLATFORM == SAM7S256) || (SAM7_PLATFORM == SAM7S512)

#if PWM_USE_PWM1 && !defined(__DOXYGEN__)
static const pwmpindefs_t PWMP1 = {{
		{ AT91C_PA0_PWM0 , AT91C_BASE_PIOA, &AT91C_BASE_PIOA->PIO_ASR },
		{ AT91C_PA11_PWM0, AT91C_BASE_PIOA, &AT91C_BASE_PIOA->PIO_BSR },
		{ AT91C_PA23_PWM0, AT91C_BASE_PIOA, &AT91C_BASE_PIOA->PIO_BSR },
	}};
#endif
#if PWM_USE_PWM2 && !defined(__DOXYGEN__)
static const pwmpindefs_t PWMP2 = {{
		{ AT91C_PA1_PWM1 , AT91C_BASE_PIOA, &AT91C_BASE_PIOA->PIO_ASR },
		{ AT91C_PA12_PWM1, AT91C_BASE_PIOA, &AT91C_BASE_PIOA->PIO_BSR },
		{ AT91C_PA24_PWM1, AT91C_BASE_PIOA, &AT91C_BASE_PIOA->PIO_BSR },
	}};
#endif
#if PWM_USE_PWM3 && !defined(__DOXYGEN__)
static const pwmpindefs_t PWMP3 = {{
		{ AT91C_PA2_PWM2 , AT91C_BASE_PIOA, &AT91C_BASE_PIOA->PIO_ASR },
		{ AT91C_PA13_PWM2, AT91C_BASE_PIOA, &AT91C_BASE_PIOA->PIO_BSR },
		{ AT91C_PA25_PWM2, AT91C_BASE_PIOA, &AT91C_BASE_PIOA->PIO_BSR },
	}};
#endif
#if PWM_USE_PWM4 && !defined(__DOXYGEN__)
static const pwmpindefs_t PWMP4 = {{
		{ AT91C_PA7_PWM3 , AT91C_BASE_PIOA, &AT91C_BASE_PIOA->PIO_BSR },
		{ AT91C_PA14_PWM3, AT91C_BASE_PIOA, &AT91C_BASE_PIOA->PIO_BSR },
		{ 0, 0, 0 },
	}};
#endif

#elif (SAM7_PLATFORM == SAM7X128) || (SAM7_PLATFORM == SAM7X256) ||         \
      (SAM7_PLATFORM == SAM7X512)

#if PWM_USE_PWM1 && !defined(__DOXYGEN__)
static const pwmpindefs_t PWMP1 = {{
		{ AT91C_PB19_PWM0, AT91C_BASE_PIOB, &AT91C_BASE_PIOB->PIO_ASR },
		{ AT91C_PB27_PWM0, AT91C_BASE_PIOB, &AT91C_BASE_PIOB->PIO_BSR },
		{ 0, 0, 0 },
	}};
#endif
#if PWM_USE_PWM2 && !defined(__DOXYGEN__)
static const pwmpindefs_t PWMP2 = {{
		{ AT91C_PB20_PWM1, AT91C_BASE_PIOB, &AT91C_BASE_PIOB->PIO_ASR },
		{ AT91C_PB28_PWM1, AT91C_BASE_PIOB, &AT91C_BASE_PIOB->PIO_BSR },
		{ 0, 0, 0 },
	}};
#endif
#if PWM_USE_PWM3 && !defined(__DOXYGEN__)
static const pwmpindefs_t PWMP3 = {{
		{ AT91C_PB21_PWM2, AT91C_BASE_PIOB, &AT91C_BASE_PIOB->PIO_ASR },
		{ AT91C_PB29_PWM2, AT91C_BASE_PIOB, &AT91C_BASE_PIOB->PIO_BSR },
		{ 0, 0, 0 },
	}};
#endif
#if PWM_USE_PWM4 && !defined(__DOXYGEN__)
static const pwmpindefs_t PWMP4 = {{
		{ AT91C_PB22_PWM3, AT91C_BASE_PIOB, &AT91C_BASE_PIOB->PIO_ASR },
		{ AT91C_PB30_PWM3, AT91C_BASE_PIOB, &AT91C_BASE_PIOB->PIO_BSR },
		{ 0, 0, 0 },
	}};
#endif

#else
	#error "PWM pins not defined for this SAM7 version"
#endif

#if PWM_USE_PWM2 && !defined(__DOXYGEN__)
PWMDriver PWMD2;
#endif

#if PWM_USE_PWM3 && !defined(__DOXYGEN__)
PWMDriver PWMD3;
#endif

#if PWM_USE_PWM4 && !defined(__DOXYGEN__)
PWMDriver PWMD4;
#endif

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

#if defined(__GNUC__)
__attribute__((noinline))
#endif
/**
 * @brief   Common IRQ handler.
 */
static void pwm_lld_serve_interrupt(void) {
	uint32_t	isr;
	
	isr = PWMC_M->PWMC_ISR;
#if PWM_USE_PWM1
	if ((isr & 1) && PWMD1.config->channels[0].callback)
		PWMD1.config->channels[0].callback(&PWMD1);
#endif
#if PWM_USE_PWM2
	if ((isr & 2) && PWMD2.config->channels[0].callback)
		PWMD2.config->channels[0].callback(&PWMD2);
#endif
#if PWM_USE_PWM3
	if ((isr & 4) && PWMD3.config->channels[0].callback)
		PWMD3.config->channels[0].callback(&PWMD3);
#endif
#if PWM_USE_PWM4
	if ((isr & 8) && PWMD4.config->channels[0].callback)
		PWMD4.config->channels[0].callback(&PWMD4);
#endif
}

CH_IRQ_HANDLER(PWMIrqHandler) {
  CH_IRQ_PROLOGUE();
  pwm_lld_serve_interrupt();
  AT91C_BASE_AIC->AIC_EOICR = 0;
  CH_IRQ_EPILOGUE();
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level PWM driver initialization.
 *
 * @notapi
 */
void pwm_lld_init(void) {

	/* Driver initialization.*/
#if PWM_USE_PWM1 && !defined(__DOXYGEN__)
	pwmObjectInit(&PWMD1);
	PWMD1.chbit = 1;
	PWMD1.reg = AT91C_BASE_PWMC_CH0;
	PWMD1.pins = &PWMP1;
#endif

#if PWM_USE_PWM2 && !defined(__DOXYGEN__)
	pwmObjectInit(&PWMD2);
	PWMD2.chbit = 2;
	PWMD2.reg = AT91C_BASE_PWMC_CH1;
	PWMD2.pins = &PWMP2;
#endif

#if PWM_USE_PWM3 && !defined(__DOXYGEN__)
	pwmObjectInit(&PWMD3);
	PWMD3.chbit = 4;
	PWMD3.reg = AT91C_BASE_PWMC_CH2;
	PWMD3.pins = &PWMP3;
#endif

#if PWM_USE_PWM4 && !defined(__DOXYGEN__)
	pwmObjectInit(&PWMD4);
	PWMD4.chbit = 8;
	PWMD4.reg = AT91C_BASE_PWMC_CH3;
	PWMD4.pins = &PWMP4;
#endif

	/* Turn on PWM in the power management controller */
	AT91C_BASE_PMC->PMC_PCER = (1 << AT91C_ID_PWMC);
  
	/* Setup interrupt handler */
	PWMC_M->PWMC_IDR = 0xFFFFFFFF;
	AIC_ConfigureIT(AT91C_ID_PWMC,
		  AT91C_AIC_SRCTYPE_HIGH_LEVEL | AT91SAM7_PWM_PRIORITY,
		  PWMIrqHandler);
    AIC_EnableIT(AT91C_ID_PWMC);
}

/**
 * @brief   Configures and activates the PWM peripheral.
 *
 * @param[in] pwmp      pointer to the @p PWMDriver object
 *
 * @notapi
 */
void pwm_lld_start(PWMDriver *pwmp) {
	uint32_t	mode, mr, div, pre;

	/* Steps:
		1. Turn the IO pin to a PWM output
		2. Configuration of Clock if DIVA or DIVB used
		3. Selection of the clock for each channel (CPRE field in the PWM_CMRx register)
		4. Configuration of the waveform alignment for each channel (CALG field in the PWM_CMRx register)
		5. Configuration of the output waveform polarity for each channel (CPOL in the PWM_CMRx register)
		6. Configuration of the period for each channel (CPRD in the PWM_CPRDx register). Writing in
			PWM_CPRDx Register is possible while the channel is disabled. After validation of the
			channel, the user must use PWM_CUPDx Register to update PWM_CPRDx
		7. Enable Interrupts (Writing CHIDx in the PWM_IER register)
	*/

	/* Make sure it is off first */
	pwm_lld_disable_channel(pwmp, 0);

	/* Configuration.*/
	mode = pwmp->config->channels[0].mode;

	/* Step 1 */
	if (mode & PWM_OUTPUT_PIN1) {
		pwmp->pins->pin[0].perab[0]		= pwmp->pins->pin[0].portpin;		/* Select A or B peripheral */
		pwmp->pins->pin[0].pio->PIO_PDR	= pwmp->pins->pin[0].portpin;		/* Turn PIO into PWM output */
		pwmp->pins->pin[0].pio->PIO_MDDR	= pwmp->pins->pin[0].portpin;		/* Turn off PIO multi-drive */
		if (mode & PWM_DISABLEPULLUP_PIN1)
			pwmp->pins->pin[0].pio->PIO_PPUDR	= pwmp->pins->pin[0].portpin;	/* Turn off PIO pullup */
		else
			pwmp->pins->pin[0].pio->PIO_PPUER	= pwmp->pins->pin[0].portpin;	/* Turn on PIO pullup */
	}
	if (mode & PWM_OUTPUT_PIN2) {
		pwmp->pins->pin[1].perab[0]		= pwmp->pins->pin[1].portpin;
		pwmp->pins->pin[1].pio->PIO_PDR	= pwmp->pins->pin[1].portpin;
		pwmp->pins->pin[1].pio->PIO_MDDR	= pwmp->pins->pin[1].portpin;
		if (mode & PWM_DISABLEPULLUP_PIN2)
			pwmp->pins->pin[1].pio->PIO_PPUDR	= pwmp->pins->pin[1].portpin;
		else
			pwmp->pins->pin[1].pio->PIO_PPUER	= pwmp->pins->pin[1].portpin;
	}
	if ((mode & PWM_OUTPUT_PIN3) && pwmp->pins->pin[2].portpin) {
		pwmp->pins->pin[2].perab[0]		= pwmp->pins->pin[2].portpin;
		pwmp->pins->pin[2].pio->PIO_PDR	= pwmp->pins->pin[2].portpin;
		pwmp->pins->pin[2].pio->PIO_MDDR	= pwmp->pins->pin[2].portpin;
		if (mode & PWM_DISABLEPULLUP_PIN3)
			pwmp->pins->pin[2].pio->PIO_PPUDR	= pwmp->pins->pin[2].portpin;
		else
			pwmp->pins->pin[2].pio->PIO_PPUER	= pwmp->pins->pin[2].portpin;
	}

	/* Step 2 */
	if ((mode & PWM_MCK_MASK) == PWM_MCK_DIV_CLKA) {
		if (!pwmp->config->frequency) {
			/* As slow as we go */
			PWMC_M->PWMC_MR = (PWMC_M->PWMC_MR & 0xFFFF0000) | (10 << 8) | (255 << 0);
		} else if (pwmp->config->frequency > MCK) {
			/* Just use MCLK */
			mode &= ~PWM_MCK_MASK;
		} else {
			div = MCK / pwmp->config->frequency;
			if (mode & PWM_OUTPUT_CENTER) div >>= 1;
			for(pre = 0; div > 255 && pre < 10; pre++) div >>= 1;
			if (div > 255) div = 255;
			PWMC_M->PWMC_MR = (PWMC_M->PWMC_MR & 0xFFFF0000) | (pre << 8) | (div << 0);
		}
	} else if ((mode & PWM_MCK_MASK) == PWM_MCK_DIV_CLKB) {
		if (!pwmp->config->frequency) {
			/* As slow as we go */
			PWMC_M->PWMC_MR = (PWMC_M->PWMC_MR & 0x0000FFFF) | (10 << 24) | (255 << 16);
		} else if (pwmp->config->frequency > MCK) {
			/* Just use MCLK */
			mode &= ~PWM_MCK_MASK;
		} else {
			div = MCK / pwmp->config->frequency;
			if (mode & PWM_OUTPUT_CENTER) div >>= 1;
			for(pre = 0; div > 255 && pre < 10; pre++) div >>= 1;
			if (div > 255) div = 255;
			PWMC_M->PWMC_MR = (PWMC_M->PWMC_MR & 0x0000FFFF) | (pre << 24) | (div << 16);
		}
	}

	/* Step 3 -> 5 */
	mr = (mode & PWM_MCK_MASK) >> PWM_MCK_SHIFT;
	if (mode & PWM_OUTPUT_CENTER) mr |= AT91C_PWMC_CALG;
	if (mode & PWM_OUTPUT_ACTIVE_HIGH) mr |= AT91C_PWMC_CPOL;
	pwmp->reg->PWMC_CMR = mr;

	/* Step 6 */
	pwmp->reg->PWMC_CPRDR = pwmp->period;
	
	/* Step 7 */
	if (pwmp->config->channels[0].callback)
		PWMC_M->PWMC_IER = pwmp->chbit;
}

/**
 * @brief   Deactivates the PWM peripheral.
 *
 * @param[in] pwmp      pointer to the @p PWMDriver object
 *
 * @notapi
 */
void pwm_lld_stop(PWMDriver *pwmp) {
	/* Make sure it is off */
	pwm_lld_disable_channel(pwmp, 0);
	
	/* Turn the pin back to a PIO pin - we have forgotten pull-up and multi-drive state for the pin though */
	if (pwmp->config->channels[0].mode & PWM_OUTPUT_PIN1)
		pwmp->pins->pin[0].pio->PIO_PER	= pwmp->pins->pin[0].portpin;
	if (pwmp->config->channels[0].mode & PWM_OUTPUT_PIN2)
		pwmp->pins->pin[1].pio->PIO_PER	= pwmp->pins->pin[1].portpin;
	if ((pwmp->config->channels[0].mode & PWM_OUTPUT_PIN3) && pwmp->pins->pin[2].portpin)
		pwmp->pins->pin[2].pio->PIO_PER	= pwmp->pins->pin[2].portpin;
}

/**
 * @brief   Changes the period the PWM peripheral.
 * @details This function changes the period of a PWM unit that has already
 *          been activated using @p pwmStart().
 * @pre     The PWM unit must have been activated using @p pwmStart().
 * @post    The PWM unit period is changed to the new value.
 * @note    The function has effect at the next cycle start.
 * @note    If a period is specified that is shorter than the pulse width
 *          programmed in one of the channels then the behavior is not
 *          guaranteed.
 *
 * @param[in] pwmp      pointer to a @p PWMDriver object
 * @param[in] period    new cycle time in ticks
 *
 * @notapi
 */
void pwm_lld_change_period(PWMDriver *pwmp, pwmcnt_t period) {
	pwmp->period = period;
	
	if (PWMC_M->PWMC_SR & pwmp->chbit) {
		pwmp->reg->PWMC_CMR |= AT91C_PWMC_CPD;
		pwmp->reg->PWMC_CUPDR = period;
	} else {
		pwmp->reg->PWMC_CPRDR = period;
	}
}

/**
 * @brief   Enables a PWM channel.
 * @pre     The PWM unit must have been activated using @p pwmStart().
 * @post    The channel is active using the specified configuration.
 * @note    Depending on the hardware implementation this function has
 *          effect starting on the next cycle (recommended implementation)
 *          or immediately (fallback implementation).
 *
 * @param[in] pwmp      pointer to a @p PWMDriver object
 * @param[in] channel   PWM channel identifier (0...PWM_CHANNELS-1)
 * @param[in] width     PWM pulse width as clock pulses number
 *
 * @notapi
 */
void pwm_lld_enable_channel(PWMDriver *pwmp,
                            pwmchannel_t UNUSED(channel),
                            pwmcnt_t width) {
	/*
		6. Configuration of the duty cycle for each channel (CDTY in the PWM_CDTYx register).
			Writing in PWM_CDTYx Register is possible while the channel is disabled. After validation of
			the channel, the user must use PWM_CUPDx Register to update PWM_CDTYx.
		7. Enable the PWM channel (Writing CHIDx in the PWM_ENA register)
	*/

	/* Step 6 */
	if (PWMC_M->PWMC_SR & pwmp->chbit) {
		pwmp->reg->PWMC_CMR &= ~AT91C_PWMC_CPD;
		pwmp->reg->PWMC_CUPDR = width;
	} else {
		pwmp->reg->PWMC_CDTYR = width;
		PWMC_M->PWMC_ENA = pwmp->chbit;
	}
	
	/* Step 7 */
	PWMC_M->PWMC_ENA = pwmp->chbit;
}

/**
 * @brief   Disables a PWM channel.
 * @pre     The PWM unit must have been activated using @p pwmStart().
 * @post    The channel is disabled and its output line returned to the
 *          idle state.
 * @note    Depending on the hardware implementation this function has
 *          effect starting on the next cycle (recommended implementation)
 *          or immediately (fallback implementation).
 *
 * @param[in] pwmp      pointer to a @p PWMDriver object
 * @param[in] channel   PWM channel identifier (0...PWM_CHANNELS-1)
 *
 * @notapi
 */
void pwm_lld_disable_channel(PWMDriver *pwmp, pwmchannel_t UNUSED(channel)) {
	PWMC_M->PWMC_IDR = pwmp->chbit;
	PWMC_M->PWMC_DIS = pwmp->chbit;
}

#endif /* HAL_USE_PWM */

/** @} */
