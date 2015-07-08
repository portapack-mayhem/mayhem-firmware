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
 * @file    AT91SAM7/adc_lld.c
 * @brief   AT91SAM7 ADC subsystem low level driver source.
 *
 * @addtogroup ADC
 * @{
 */

#include "ch.h"
#include "hal.h"

#if HAL_USE_ADC || defined(__DOXYGEN__)

/**
 * @brief	ADC1 Prescaler
 * @detail	Prescale = RoundUp(MCK / 2 / ADCClock - 1)
 */
#if ((((MCK/2)+(AT91_ADC1_CLOCK-1))/AT91_ADC1_CLOCK)-1) > 255
	#define AT91_ADC1_PRESCALE	255
#else
	#define AT91_ADC1_PRESCALE	((((MCK/2)+(AT91_ADC1_CLOCK-1))/AT91_ADC1_CLOCK)-1)
#endif

/**
 * @brief	ADC1 Startup Time
 * @details	Startup = RoundUp(ADCClock / 400,000 - 1)
 * @note	Corresponds to a startup delay > 20uS (as required from the datasheet)
 */
#if (((AT91_ADC1_CLOCK+399999)/400000)-1) > 127
	#define AT91_ADC1_STARTUP	127
#else
	#define AT91_ADC1_STARTUP	(((AT91_ADC1_CLOCK+399999)/400000)-1)
#endif

#if AT91_ADC1_RESOLUTION == 8
	#define AT91_ADC1_MAINMODE		(((AT91_ADC1_SHTM & 0x0F) << 24) | ((AT91_ADC1_STARTUP & 0x7F) << 16) | ((AT91_ADC1_PRESCALE & 0xFF) << 8) | AT91C_ADC_LOWRES_8_BIT)
#else
	#define AT91_ADC1_MAINMODE		(((AT91_ADC1_SHTM & 0x0F) << 24) | ((AT91_ADC1_STARTUP & 0x7F) << 16) | ((AT91_ADC1_PRESCALE & 0xFF) << 8) | AT91C_ADC_LOWRES_10_BIT)
#endif

#if AT91_ADC1_TIMER < 0 || AT91_ADC1_TIMER > 2
	#error "Unknown Timer specified for ADC1"
#endif

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

#if !ADC_USE_ADC1
	#error "You must specify ADC_USE_ADC1 if you have specified HAL_USE_ADC"
#endif

/** @brief ADC1 driver identifier.*/
ADCDriver ADCD1;

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

#define ADCReg1		((AT91S_ADC *)AT91C_ADC_CR)

#if AT91_ADC1_MAINMODE == 2
	#define ADCTimer1				((AT91S_TC *)AT91C_TC2_CCR)
	#define AT91_ADC1_TIMERMODE		AT91C_ADC_TRGSEL_TIOA2
	#define AT91_ADC1_TIMERID		AT91C_ID_TC2
#elif AT91_ADC1_MAINMODE == 1
	#define ADCTimer1				((AT91S_TC *)AT91C_TC1_CCR)
	#define AT91_ADC1_TIMERMODE		AT91C_ADC_TRGSEL_TIOA1
	#define AT91_ADC1_TIMERID		AT91C_ID_TC1
#else
	#define ADCTimer1				((AT91S_TC *)AT91C_TC0_CCR)
	#define AT91_ADC1_TIMERMODE		AT91C_ADC_TRGSEL_TIOA0
	#define AT91_ADC1_TIMERID		AT91C_ID_TC0
#endif

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

#define adc_sleep()				ADCReg1->ADC_MR = (AT91_ADC1_MAINMODE | AT91C_ADC_SLEEP_MODE | AT91C_ADC_TRGEN_DIS)
#define adc_wake()				ADCReg1->ADC_MR = (AT91_ADC1_MAINMODE | AT91C_ADC_SLEEP_NORMAL_MODE | AT91C_ADC_TRGEN_DIS)
#define adc_disable()			{																\
									ADCReg1->ADC_IDR = 0xFFFFFFFF;								\
									ADCReg1->ADC_PTCR = AT91C_PDC_RXTDIS | AT91C_PDC_TXTDIS;	\
									adc_wake();													\
									ADCReg1->ADC_CHDR = 0xFF;									\
								}
#define adc_clrint()			{																\
									uint32_t	isr, dummy;										\
																								\
									isr = ADCReg1->ADC_SR;										\
									if ((isr & AT91C_ADC_DRDY)) dummy = ADCReg1->ADC_LCDR;		\
									if ((isr & AT91C_ADC_EOC0)) dummy = ADCReg1->ADC_CDR0;		\
									if ((isr & AT91C_ADC_EOC1)) dummy = ADCReg1->ADC_CDR1;		\
									if ((isr & AT91C_ADC_EOC2)) dummy = ADCReg1->ADC_CDR2;		\
									if ((isr & AT91C_ADC_EOC3)) dummy = ADCReg1->ADC_CDR3;		\
									if ((isr & AT91C_ADC_EOC4)) dummy = ADCReg1->ADC_CDR4;		\
									if ((isr & AT91C_ADC_EOC5)) dummy = ADCReg1->ADC_CDR5;		\
									if ((isr & AT91C_ADC_EOC6)) dummy = ADCReg1->ADC_CDR6;		\
									if ((isr & AT91C_ADC_EOC7)) dummy = ADCReg1->ADC_CDR7;		\
									(void) dummy;												\
								}
#define adc_stop()				{																\
									adc_disable();												\
									adc_clrint();												\
								}

/**
 * We must keep stack usage to a minimum - the default AT91SAM7 isr stack size is very small.
 * We sacrifice some speed and code size in order to achieve this by accessing the structure
 * and registers directly rather than through the passed in pointers. This works because the
 * AT91SAM7 supports only a single ADC device (although with 8 channels).
 */
static void handleint(void) {
	uint32_t	isr;

	isr = ADCReg1->ADC_SR;

	if (ADCD1.grpp) {

		/* ADC overflow condition, this could happen only if the DMA is unable to read data fast enough.*/
		if ((isr & AT91C_ADC_GOVRE)) {
			_adc_isr_error_code(&ADCD1, ADC_ERR_OVERFLOW);

		/* Transfer complete processing.*/
		} else if ((isr & AT91C_ADC_RXBUFF)) {
			if (ADCD1.grpp->circular) {
				/* setup the DMA again */
				ADCReg1->ADC_RPR = (uint32_t)ADCD1.samples;
				if (ADCD1.depth <= 1) {
					ADCReg1->ADC_RCR = ADCD1.grpp->num_channels;
					ADCReg1->ADC_RNPR = 0;
					ADCReg1->ADC_RNCR = 0;
				} else {
					ADCReg1->ADC_RCR = ADCD1.depth/2 * ADCD1.grpp->num_channels;
					ADCReg1->ADC_RNPR = (uint32_t)(ADCD1.samples + (ADCD1.depth/2 * ADCD1.grpp->num_channels));
					ADCReg1->ADC_RNCR = (ADCD1.depth - ADCD1.depth/2) * ADCD1.grpp->num_channels;
				}
				ADCReg1->ADC_PTCR = AT91C_PDC_RXTEN;			// DMA enabled
			}
			_adc_isr_full_code(&ADCD1);

		/* Half transfer processing.*/
		} else if ((isr & AT91C_ADC_ENDRX)) {
			// Make sure we get a full complete next time.
			ADCReg1->ADC_RNPR = 0;
			ADCReg1->ADC_RNCR = 0;
			_adc_isr_half_code(&ADCD1);
		}

	} else {
		/* Spurious interrupt - Make sure it doesn't happen again */
		adc_disable();
	}
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/**
 * @brief   ADC interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(ADC_IRQHandler) {
	CH_IRQ_PROLOGUE();

	handleint();

	AT91C_BASE_AIC->AIC_EOICR = 0;
	CH_IRQ_EPILOGUE();
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level ADC driver initialization.
 *
 * @notapi
 */
void adc_lld_init(void) {
	/* Turn on ADC in the power management controller */
	AT91C_BASE_PMC->PMC_PCER = (1 << AT91C_ID_ADC);

	/* Driver object initialization.*/
	adcObjectInit(&ADCD1);

	ADCReg1->ADC_CR = 0;				// 0 or AT91C_ADC_SWRST if you want to do a ADC reset
	adc_stop();
	adc_sleep();

	/* Setup interrupt handler */
	AIC_ConfigureIT(AT91C_ID_ADC,
		  AT91C_AIC_SRCTYPE_HIGH_LEVEL | AT91_ADC_IRQ_PRIORITY,
		  ADC_IRQHandler);
	AIC_EnableIT(AT91C_ID_ADC);
}

/**
 * @brief   Configures and activates the ADC peripheral.
 *
 * @param[in] adcp      pointer to the @p ADCDriver object
 *
 * @notapi
 */
void adc_lld_start(ADCDriver *adcp) {

	/* If in stopped state then wake up the ADC */
	if (adcp->state == ADC_STOP) {

    	/* Take it out of sleep mode */
    	/* We could stay in sleep mode provided total conversion rate < 44kHz but we can't guarantee that here */
    	adc_wake();

    	/* TODO: We really should perform a conversion here just to ensure that we are out of sleep mode */
    }
}

/**
 * @brief   Deactivates the ADC peripheral.
 *
 * @param[in] adcp      pointer to the @p ADCDriver object
 *
 * @notapi
 */
void adc_lld_stop(ADCDriver *adcp) {
	if (adcp->state != ADC_READY) {
		adc_stop();
    	adc_sleep();
	}
}

/**
 * @brief   Starts an ADC conversion.
 *
 * @param[in] adcp      pointer to the @p ADCDriver object
 *
 * @notapi
 */
void adc_lld_start_conversion(ADCDriver *adcp) {
	uint32_t			i;
	(void)				adcp;

	/* Make sure everything is stopped first */
	adc_stop();

	/* Safety check the trigger value */
	switch(ADCD1.grpp->trigger & ~ADC_TRIGGER_SOFTWARE) {
	case ADC_TRIGGER_TIMER:
	case ADC_TRIGGER_EXTERNAL:
		break;
	default:
		((ADCConversionGroup *)ADCD1.grpp)->trigger = ADC_TRIGGER_SOFTWARE;
		ADCD1.depth = 1;
		((ADCConversionGroup *)ADCD1.grpp)->circular = 0;
		break;
	}

	/* Count the real number of activated channels in case the user got it wrong */
	((ADCConversionGroup *)ADCD1.grpp)->num_channels = 0;
	for(i=1; i < 0x100; i <<= 1) {
		if ((ADCD1.grpp->channelselects & i))
			((ADCConversionGroup *)ADCD1.grpp)->num_channels++;
	}

	/* Set the channels */
	ADCReg1->ADC_CHER = ADCD1.grpp->channelselects;

	/* Set up the DMA */
	ADCReg1->ADC_RPR = (uint32_t)ADCD1.samples;
	if (ADCD1.depth <= 1 || !ADCD1.grpp->circular) {
		ADCReg1->ADC_RCR = ADCD1.depth * ADCD1.grpp->num_channels;
		ADCReg1->ADC_RNPR = 0;
		ADCReg1->ADC_RNCR = 0;
	} else {
		ADCReg1->ADC_RCR = ADCD1.depth/2 * ADCD1.grpp->num_channels;
		ADCReg1->ADC_RNPR = (uint32_t)(ADCD1.samples + (ADCD1.depth/2 * ADCD1.grpp->num_channels));
		ADCReg1->ADC_RNCR = (ADCD1.depth - ADCD1.depth/2) * ADCD1.grpp->num_channels;
	}
	ADCReg1->ADC_PTCR = AT91C_PDC_RXTEN;

	/* Set up interrupts */
	ADCReg1->ADC_IER = AT91C_ADC_GOVRE | AT91C_ADC_ENDRX | AT91C_ADC_RXBUFF;

	/* Set the trigger */
	switch(ADCD1.grpp->trigger & ~ADC_TRIGGER_SOFTWARE) {
	case ADC_TRIGGER_TIMER:
		// Set up the timer if ADCD1.grpp->frequency != 0
		if (ADCD1.grpp->frequency) {
			/* Turn on Timer in the power management controller */
			AT91C_BASE_PMC->PMC_PCER = (1 << AT91_ADC1_TIMERID);

		    /* Disable the clock and the interrupts */
			ADCTimer1->TC_CCR = AT91C_TC_CLKDIS;
			ADCTimer1->TC_IDR = 0xFFFFFFFF;

		    /* Set the Mode of the Timer Counter and calculate the period */
			i = (MCK/2)/ADCD1.grpp->frequency;
			if (i < (0x10000<<0)) {
				ADCTimer1->TC_CMR = (AT91C_TC_ASWTRG_CLEAR | AT91C_TC_ACPC_CLEAR | AT91C_TC_ACPA_SET | AT91C_TC_LDRA_RISING |
				                        AT91C_TC_WAVE  | AT91C_TC_WAVESEL_UP_AUTO | AT91C_TC_CLKS_TIMER_DIV1_CLOCK);
			} else if (i < (0x10000<<2)) {
				i >>= 2;
				ADCTimer1->TC_CMR = (AT91C_TC_ASWTRG_CLEAR | AT91C_TC_ACPC_CLEAR | AT91C_TC_ACPA_SET | AT91C_TC_LDRA_RISING |
				                        AT91C_TC_WAVE  | AT91C_TC_WAVESEL_UP_AUTO | AT91C_TC_CLKS_TIMER_DIV2_CLOCK);
			} else if (i < (0x10000<<4)) {
				i >>= 4;
				ADCTimer1->TC_CMR = (AT91C_TC_ASWTRG_CLEAR | AT91C_TC_ACPC_CLEAR | AT91C_TC_ACPA_SET | AT91C_TC_LDRA_RISING |
				                        AT91C_TC_WAVE  | AT91C_TC_WAVESEL_UP_AUTO | AT91C_TC_CLKS_TIMER_DIV3_CLOCK);
			} else if (i < (0x10000<<6)) {
				i >>= 6;
				ADCTimer1->TC_CMR = (AT91C_TC_ASWTRG_CLEAR | AT91C_TC_ACPC_CLEAR | AT91C_TC_ACPA_SET | AT91C_TC_LDRA_RISING |
				                        AT91C_TC_WAVE  | AT91C_TC_WAVESEL_UP_AUTO | AT91C_TC_CLKS_TIMER_DIV4_CLOCK);
			} else {
				i >>= 9;
				ADCTimer1->TC_CMR = (AT91C_TC_ASWTRG_CLEAR | AT91C_TC_ACPC_CLEAR | AT91C_TC_ACPA_SET | AT91C_TC_LDRA_RISING |
				                        AT91C_TC_WAVE  | AT91C_TC_WAVESEL_UP_AUTO | AT91C_TC_CLKS_TIMER_DIV5_CLOCK);
			}

		    /* RC is the period, RC-RA is the pulse width (in this case = 1) */
			ADCTimer1->TC_RC = i;
			ADCTimer1->TC_RA = i - 1;

			/* Start the timer counter */
			ADCTimer1->TC_CCR = (AT91C_TC_CLKEN |AT91C_TC_SWTRG);
		}

		ADCReg1->ADC_MR = AT91_ADC1_MAINMODE | AT91C_ADC_SLEEP_NORMAL_MODE | AT91C_ADC_TRGEN_EN | AT91_ADC1_TIMERMODE;
		break;

	case ADC_TRIGGER_EXTERNAL:
		/* Make sure the ADTRG pin is set as an input - assume pull-ups etc have already been set */
		#if (SAM7_PLATFORM == SAM7S64) || (SAM7_PLATFORM == SAM7S128) || (SAM7_PLATFORM == SAM7S256) || (SAM7_PLATFORM == SAM7S512)
			AT91C_BASE_PIOA->PIO_ODR	= AT91C_PA8_ADTRG;
		#elif (SAM7_PLATFORM == SAM7X128) || (SAM7_PLATFORM == SAM7X256) || (SAM7_PLATFORM == SAM7X512)
			AT91C_BASE_PIOB->PIO_ODR	= AT91C_PB18_ADTRG;
		#endif
    	ADCReg1->ADC_MR = AT91_ADC1_MAINMODE | AT91C_ADC_SLEEP_NORMAL_MODE | AT91C_ADC_TRGEN_EN | AT91C_ADC_TRGSEL_EXT;
    	break;

	default:
    	ADCReg1->ADC_MR = AT91_ADC1_MAINMODE | AT91C_ADC_SLEEP_NORMAL_MODE | AT91C_ADC_TRGEN_DIS;
    	break;
	}

	/* Manually start a conversion if we need to */
	if (ADCD1.grpp->trigger & ADC_TRIGGER_SOFTWARE)
    	ADCReg1->ADC_CR = AT91C_ADC_START;
}

/**
 * @brief   Stops an ongoing conversion.
 *
 * @param[in] adcp      pointer to the @p ADCDriver object
 *
 * @notapi
 */
void adc_lld_stop_conversion(ADCDriver *adcp) {
	(void) adcp;
	adc_stop();
}

#endif /* HAL_USE_ADC */

/** @} */
