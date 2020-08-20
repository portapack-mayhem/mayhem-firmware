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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.	If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifndef __LPC43XX_CPP_H__
#define __LPC43XX_CPP_H__

#include <cstdint>

#include <hal.h>

#include "utility.hpp"

namespace lpc43xx {

#if defined(LPC43XX_M4)
namespace m4 {

static inline bool flag_saturation() {
	return __get_APSR() & (1U << 27);
}

static inline void clear_flag_saturation() {
	uint32_t flags = 1;
	__asm volatile ("MSR APSR_nzcvqg, %0" : : "r" (flags));
}

} /* namespace m4 */
#endif

namespace creg {

static_assert(offsetof(LPC_CREG_Type, CREG0) == 0x004, "CREG0 offset wrong");
static_assert(offsetof(LPC_CREG_Type, M4MEMMAP) == 0x100, "M4MEMMAP offset wrong");
static_assert(offsetof(LPC_CREG_Type, CREG5) == 0x118, "CREG5 offset wrong");
static_assert(offsetof(LPC_CREG_Type, CHIPID) == 0x200, "CHIPID offset wrong");
static_assert(offsetof(LPC_CREG_Type, M0SUBMEMMAP) == 0x308, "M0SUBMEMMAP offset wrong");
static_assert(offsetof(LPC_CREG_Type, M0APPTXEVENT) == 0x400, "M0APPTXEVENT offset wrong");
static_assert(offsetof(LPC_CREG_Type, USB0FLADJ) == 0x500, "USB0FLADJ offset wrong");
static_assert(offsetof(LPC_CREG_Type, USB1FLADJ) == 0x600, "USB1FLADJ offset wrong");

namespace m4txevent {

#if defined(LPC43XX_M0)
inline void enable() {
	nvicEnableVector(M4CORE_IRQn, CORTEX_PRIORITY_MASK(LPC43XX_M4TXEVENT_IRQ_PRIORITY));
}

inline void disable() {
	nvicDisableVector(M4CORE_IRQn);
}
#endif

#if defined(LPC43XX_M4)
inline void assert_event() {
	__SEV();
}
#endif

inline void clear() {
	LPC_CREG->M4TXEVENT = 0;
}

} /* namespace m4txevent */

namespace m0apptxevent {

#if defined(LPC43XX_M4)
inline void enable() {
	nvicEnableVector(M0CORE_IRQn, CORTEX_PRIORITY_MASK(LPC43XX_M0APPTXEVENT_IRQ_PRIORITY));
}

inline void disable() {
	nvicDisableVector(M0CORE_IRQn);
}
#endif

#if defined(LPC43XX_M0)
inline void assert_event() {
	__SEV();
}
#endif

inline void clear() {
	LPC_CREG->M0APPTXEVENT = 0;
}

} /* namespace */

} /* namespace creg */

namespace cgu {

enum class CLK_SEL : uint8_t {
	RTC_32KHZ	= 0x00,
	IRC			= 0x01,
	ENET_RX_CLK = 0x02,
	ENET_TX_CLK = 0x03,
	GP_CLKIN	= 0x04,
	XTAL		= 0x06,
	PLL0USB		= 0x07,
	PLL0AUDIO	= 0x08,
	PLL1		= 0x09,
	IDIVA		= 0x0c,
	IDIVB		= 0x0d,
	IDIVC		= 0x0e,
	IDIVD		= 0x0f,
	IDIVE		= 0x10,
};

struct IDIV_CTRL {
	uint32_t pd;
	uint32_t idiv;
	uint32_t autoblock;
	CLK_SEL clk_sel;

	constexpr operator uint32_t() const {
		return
			  ((pd & 1) << 0)
			| ((idiv & 255) << 2)
			| ((autoblock & 1) << 11)
			| ((toUType(clk_sel) & 0x1f) << 24)
			;
	}
};

namespace pll0audio {

struct CTRL {
	uint32_t pd;
	uint32_t bypass;
	uint32_t directi;
	uint32_t directo;
	uint32_t clken;
	uint32_t frm;
	uint32_t autoblock;
	uint32_t pllfract_req;
	uint32_t sel_ext;
	uint32_t mod_pd;
	CLK_SEL clk_sel;

	constexpr operator uint32_t() const {
		return
			  ((pd & 1) << 0)
			| ((bypass & 1) << 1)
			| ((directi & 1) << 2)
			| ((directo & 1) << 3)
			| ((clken & 1) << 4)
			| ((frm & 1) << 6)
			| ((autoblock & 1) << 11)
			| ((pllfract_req & 1) << 12)
			| ((sel_ext & 1) << 13)
			| ((mod_pd & 1) << 14)
			| ((toUType(clk_sel) & 0x1f) << 24)
			;
	}
};

struct MDIV {
	uint32_t mdec;

	constexpr operator uint32_t() const {
		return ((mdec & 0x1ffff) << 0);
	}
};

struct NP_DIV {
	uint32_t pdec;
	uint32_t ndec;

	constexpr operator uint32_t() const {
		return
			  ((pdec & 0x7f) << 0)
			| ((ndec & 0x3ff) << 12)
			;
	}
};

struct FRAC {
	uint32_t pllfract_ctrl;

	constexpr operator uint32_t() const {
		return ((pllfract_ctrl & 0x3fffff) << 0);
	}
};

inline void ctrl(const CTRL& value) {
	LPC_CGU->PLL0AUDIO_CTRL = value;
}

inline void mdiv(const MDIV& value) {
	LPC_CGU->PLL0AUDIO_MDIV = value;
}

inline void np_div(const NP_DIV& value) {
	LPC_CGU->PLL0AUDIO_NP_DIV = value;
}

inline void frac(const FRAC& value) {
	LPC_CGU->PLL0AUDIO_FRAC = value;
}

inline void power_up() {
	LPC_CGU->PLL0AUDIO_CTRL &= ~(1U << 0);
}

inline void power_down() {
	LPC_CGU->PLL0AUDIO_CTRL |= (1U << 0);
}

inline bool is_locked() {
	return LPC_CGU->PLL0AUDIO_STAT & (1U << 0);
}

inline void clock_enable() {
	LPC_CGU->PLL0AUDIO_CTRL |= (1U << 4);
}

inline void clock_disable() {
	LPC_CGU->PLL0AUDIO_CTRL &= ~(1U << 4);
}

} /* namespace pll0audio */

namespace pll1 {

struct CTRL {
	uint32_t pd;
	uint32_t bypass;
	uint32_t fbsel;
	uint32_t direct;
	uint32_t psel;
	uint32_t autoblock;
	uint32_t nsel;
	uint32_t msel;
	CLK_SEL clk_sel;

	constexpr operator uint32_t() const {
		return
			  ((pd & 1) << 0)
			| ((bypass & 1) << 1)
			| ((fbsel & 1) << 6)
			| ((direct & 1) << 7)
			| ((psel & 3) << 8)
			| ((autoblock & 1) << 11)
			| ((nsel & 3) << 12)
			| ((msel & 0xff) << 16)
			| ((toUType(clk_sel) & 0x1f) << 24)
			;
	}
};

inline void ctrl(const CTRL& value) {
	LPC_CGU->PLL1_CTRL = value;
}

inline void enable() {
	LPC_CGU->PLL1_CTRL &= ~(1U << 0);
}

inline void disable() {
	LPC_CGU->PLL1_CTRL |= (1U << 0);
}

inline void direct() {
	LPC_CGU->PLL1_CTRL |= (1U << 7);
}

inline bool is_locked() {
	return LPC_CGU->PLL1_STAT & (1U << 0);
}

} /* namespace pll1 */

} /* namespace cgu */

namespace ccu1 {

static_assert(offsetof(LPC_CCU1_Type, CLK_ADCHS_STAT) == 0xb04, "CLK_ADCHS_STAT offset wrong");

} /* namespace ccu1 */

namespace rgu {

enum class Reset {
	CORE = 0,
	PERIPH = 1,
	MASTER = 2,
	WWDT = 4,
	CREG = 5,
	BUS = 8,
	SCU = 9,
	M0_SUB = 12,
	M4_RST = 13,
	LCD = 16,
	USB0 = 17,
	USB1 = 18,
	DMA = 19,
	SDIO = 20,
	EMC = 21,
	ETHERNET = 22,
	FLASHA = 25,
	EEPROM = 27,
	GPIO = 28,
	FLASHB = 29,
	TIMER0 = 32,
	TIMER1 = 33,
	TIMER2 = 34,
	TIMER3 = 35,
	RITIMER = 36,
	SCT = 37,
	MOTOCONPWM = 38,
	QEI = 39,
	ADC0 = 40,
	ADC1 = 41,
	DAC = 42,
	UART0 = 44,
	UART1 = 45,
	UART2 = 46,
	UART3 = 47,
	I2C0 = 48,
	I2C1 = 49,
	SSP0 = 50,
	SSP1 = 51,
	I2S = 52,
	SPIFI = 53,
	CAN1 = 54,
	CAN0 = 55,
	M0APP = 56,
	SGPIO = 57,
	SPI = 58,
	ADCHS = 60,
};

enum class Status {
	NotActive = 0b00,
	ActivatedByRGUInput = 0b01,
	ActivatedBySoftware = 0b11,
};

inline void reset(const Reset reset) {
	LPC_RGU->RESET_CTRL[toUType(reset) >> 5] = (1U << (toUType(reset) & 0x1f));
}

inline void reset_mask(const uint64_t mask) {
	LPC_RGU->RESET_CTRL[0] = mask & 0xffffffffU;
	LPC_RGU->RESET_CTRL[1] = mask >> 32;
}

inline Status status(const Reset reset) {
	return static_cast<Status>(
		(LPC_RGU->RESET_STATUS[toUType(reset) >> 4] >> ((toUType(reset) & 0xf) * 2)) & 3
	);
}

inline bool active(const Reset reset) {
	return (LPC_RGU->RESET_ACTIVE_STATUS[toUType(reset) >> 5] >> (toUType(reset) & 0x1f)) & 1;
}

inline uint32_t external_status(const Reset reset) {
	return LPC_RGU->RESET_EXT_STAT[toUType(reset)];
}

inline uint64_t operator|(Reset r1, Reset r2) {
	return (1ULL << toUType(r1)) | (1ULL << toUType(r2));
}

inline uint64_t operator|(uint64_t m, Reset r) {
	return m | (1ULL << toUType(r));
}

static_assert(offsetof(LPC_RGU_Type, RESET_CTRL[0]) == 0x100, "RESET_CTRL[0] offset wrong");
static_assert(offsetof(LPC_RGU_Type, RESET_STATUS[0]) == 0x110, "RESET_STATUS[0] offset wrong");
static_assert(offsetof(LPC_RGU_Type, RESET_ACTIVE_STATUS[0]) == 0x150, "RESET_ACTIVE_STATUS[0] offset wrong");
static_assert(offsetof(LPC_RGU_Type, RESET_EXT_STAT[1]) == 0x404, "RESET_EXT_STAT[1] offset wrong");
static_assert(offsetof(LPC_RGU_Type, RESET_EXT_STAT[60]) == 0x4f0, "RESET_EXT_STAT[60] offset wrong");

} /* namespace rgu */

namespace scu {

struct SFS {
	uint32_t mode;
	uint32_t epd;
	uint32_t epun;
	uint32_t ehs;
	uint32_t ezi;
	uint32_t zif;

	constexpr operator uint32_t() const {
		return
			  ((mode & 7) << 0)
			| ((epd	& 1) << 3)
			| ((epun & 1) << 4)
			| ((ehs	& 1) << 5)
			| ((ezi	& 1) << 6)
			| ((zif	& 1) << 7)
			;
	}
};

static_assert(offsetof(LPC_SCU_Type, PINTSEL0) == 0xe00, "PINTSEL0 offset wrong");

} /* namespace scu */

namespace sgpio {

static_assert(offsetof(LPC_SGPIO_Type, MASK_A) == 0x0200, "SGPIO MASK_A offset wrong");
static_assert(offsetof(LPC_SGPIO_Type, GPIO_OUTREG) == 0x0214, "SGPIO GPIO_OUTREG offset wrong");
static_assert(offsetof(LPC_SGPIO_Type, CTRL_DISABLE) == 0x0220, "SGPIO CTRL_DISABLE offset wrong");
static_assert(offsetof(LPC_SGPIO_Type, CLR_EN_0) == 0x0f00, "SGPIO CLR_EN_0 offset wrong");
static_assert(offsetof(LPC_SGPIO_Type, CLR_EN_1) == 0x0f20, "SGPIO CLR_EN_1 offset wrong");
static_assert(offsetof(LPC_SGPIO_Type, CLR_EN_2) == 0x0f40, "SGPIO CLR_EN_2 offset wrong");
static_assert(offsetof(LPC_SGPIO_Type, CLR_EN_3) == 0x0f60, "SGPIO CLR_EN_3 offset wrong");
static_assert(offsetof(LPC_SGPIO_Type, SET_STATUS_3) == 0x0f74, "SGPIO SET_STATUS_3 offset wrong");
static_assert(sizeof(LPC_SGPIO_Type) == 0x0f78, "SGPIO type size wrong");

} /* namespace sgpio */

namespace gpdma {

static_assert(offsetof(LPC_GPDMA_Type, SYNC) == 0x034, "GPDMA SYNC offset wrong");
static_assert(offsetof(LPC_GPDMA_Type, CH[0]) == 0x100, "GPDMA CH[0] offset wrong");
static_assert(offsetof(LPC_GPDMA_Type, CH[7]) == 0x1e0, "GPDMA CH[7] offset wrong");

} /* namespace gpdma */

namespace sdmmc {

static_assert(offsetof(LPC_SDMMC_Type, RESP0) == 0x030, "SDMMC RESP0 offset wrong");
static_assert(offsetof(LPC_SDMMC_Type, TCBCNT) == 0x05c, "SDMMC TCBCNT offset wrong");
static_assert(offsetof(LPC_SDMMC_Type, RST_N) == 0x078, "SDMMC RST_N offset wrong");
static_assert(offsetof(LPC_SDMMC_Type, BMOD) == 0x080, "SDMMC BMOD offset wrong");
static_assert(offsetof(LPC_SDMMC_Type, DATA) == 0x100, "SDMMC DATA offset wrong");

} /* namespace sdmmc */

namespace spifi {

struct CTRL {
	uint32_t timeout;
	uint32_t cshigh;
	uint32_t d_prftch_dis;
	uint32_t inten;
	uint32_t mode3;
	uint32_t prftch_dis;
	uint32_t dual;
	uint32_t rfclk;
	uint32_t fbclk;
	uint32_t dmaen;

	constexpr operator uint32_t() const {
		return
			  ((timeout & 0xffff) <<	0)
			| ((cshigh & 1) << 16)
			| ((d_prftch_dis & 1) << 21)
			| ((inten & 1) << 22)
			| ((mode3 & 1) << 23)
			| ((prftch_dis & 1) << 27)
			| ((dual & 1) << 28)
			| ((rfclk & 1) << 29)
			| ((fbclk & 1) << 30)
			| ((dmaen & 1) << 31)
			;
	}
};

static_assert(offsetof(LPC_SPIFI_Type, STAT) == 0x01c, "SPIFI STAT offset wrong");

} /* namespace spifi */

namespace timer {

static_assert(offsetof(LPC_TIMER_Type, MR[0]) == 0x018, "TIMER MR[0] offset wrong");
static_assert(offsetof(LPC_TIMER_Type, CCR) == 0x028, "TIMER CCR offset wrong");
static_assert(offsetof(LPC_TIMER_Type, EMR) == 0x03c, "TIMER EMR offset wrong");
static_assert(offsetof(LPC_TIMER_Type, CTCR) == 0x070, "TIMER CTCR offset wrong");

} /* namespace timer */

namespace rtc {

namespace interrupt {

inline void clear_all() {
	LPC_RTC->ILR = (1U << 1) | (1U << 0);
}

inline void enable_second_inc() {
	LPC_RTC->CIIR = (1U << 0);
}

} /* namespace */

#if HAL_USE_RTC
struct RTC : public RTCTime {
	constexpr RTC(
		uint32_t year,
		uint32_t month,
		uint32_t day,
		uint32_t hour,
		uint32_t minute,
		uint32_t second
	) : RTCTime {
			(year << 16) | (month << 8) | (day << 0),
			(hour << 16) | (minute << 8) | (second << 0)
		}
	{
	}

	constexpr RTC(
	) : RTCTime { 0, 0 }
	{
	}

	uint16_t year() const {
		return (tv_date >> 16) & 0xfff;
	}

	uint8_t month() const {
		return (tv_date >> 8) & 0x00f;
	}

	uint8_t day() const {
		return (tv_date >> 0) & 0x01f;
	}

	uint8_t hour() const {
		return (tv_time >> 16) & 0x01f;
	}

	uint8_t minute() const {
		return (tv_time >> 8) & 0x03f;
	}

	uint8_t second() const {
		return (tv_time >> 0) & 0x03f;
	}
};
#endif

static_assert(offsetof(LPC_RTC_Type, CCR) == 0x008, "RTC CCR offset wrong");
static_assert(offsetof(LPC_RTC_Type, ASEC) == 0x060, "RTC ASEC offset wrong");

} /* namespace rtc */

} /* namespace lpc43xx */

#endif/*__LPC43XX_CPP_H__*/
