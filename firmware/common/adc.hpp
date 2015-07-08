/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __ADC_H__
#define __ADC_H__

#include "hal.h"

namespace lpc43xx {
namespace adc {

constexpr size_t clock_rate_max = 4500000U;

struct CR {
	uint32_t sel;
	uint32_t clkdiv;
	uint32_t resolution;
	uint32_t edge;

	constexpr operator uint32_t() const {
		return
			  ((sel & 0xff) << 0)
			| ((clkdiv & 0xff) << 8)
			| ((0 & 1) << 16)
			| (((10 - resolution) & 7) << 17)
			| ((1 & 1) << 21)
			| ((0 & 7) << 24)
			| ((edge & 1) << 27)
			;
	}
};

struct Config {
	uint32_t cr;
};

class ADC {
public:
	constexpr ADC(
		LPC_ADCx_Type* adcp
	) : adcp(adcp)
	{
	}

	void power_up(const Config config) const {
		adcp->CR = config.cr;
	}

	void clock_enable() const {
		if( adcp == LPC_ADC0 ) {
			LPC_CCU1->CLK_APB3_ADC0_CFG.AUTO = 1;
			LPC_CCU1->CLK_APB3_ADC0_CFG.RUN = 1;
		}
		if( adcp == LPC_ADC1 ) {
			LPC_CCU1->CLK_APB3_ADC1_CFG.AUTO = 1;
			LPC_CCU1->CLK_APB3_ADC1_CFG.RUN = 1;
		}
	}

	void clock_disable() const {
		if( adcp == LPC_ADC0 ) {
			LPC_CCU1->CLK_APB3_ADC0_CFG.RUN = 0;
		}  
		if( adcp == LPC_ADC1 ) {
			LPC_CCU1->CLK_APB3_ADC1_CFG.RUN = 0;
		}  
	}

	void disable() const {
		adcp->INTEN = 0;
		adcp->CR = 0;

		clock_disable();
	}

	void interrupts_disable() const {
		adcp->INTEN = 0;
	}

	void interrupts_enable(const uint32_t mask) const {
		adcp->INTEN = mask;
	}

	void start_burst() const {
		adcp->CR |= (1U << 16);
	}

	void start_once() const {
		adcp->CR |= (1U << 24);
	}

	void start_once(size_t n) const {
		uint32_t cr = adcp->CR;
		cr &= ~(0xffU);
		cr |= (1 << 24) | (1 << n);
		adcp->CR = cr;
	}

	void stop_burst() const {
		adcp->CR &= ~(1U << 16);
	}

	uint32_t convert(size_t n) const {
		start_once(n);
		while(true) {
			const uint32_t data = adcp->DR[n];
			if( (data >> 31) & 1 ) {
				return (data >> 6) & 0x3ff;
			}
		}
	}
	
private:
	LPC_ADCx_Type* const adcp;
};

} /* namespace adc */
} /* namespace lpc43xx */

#endif/*__ADC_H__*/
