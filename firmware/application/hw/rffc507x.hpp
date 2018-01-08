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

#ifndef __RFFC507X_H__
#define __RFFC507X_H__

#include "rffc507x_spi.hpp"

#include <cstdint>
#include <array>

#include "dirty_registers.hpp"
#include "rf_path.hpp"

namespace rffc507x {

using reg_t = spi::reg_t;
using address_t = spi::address_t;

constexpr size_t reg_count = 31;

enum class Register : address_t {
	LF = 0x00,
	XO = 0x01,
	CAL_TIME = 0x02,
	VCO_CTRL = 0x03,
	CT_CAL1 = 0x04,
	CT_CAL2 = 0x05,
	PLL_CAL1 = 0x06,
	PLL_CAL2 = 0x07,
	VCO_AUTO = 0x08,
	PLL_CTRL = 0x09,
	PLL_BIAS = 0x0a,
	MIX_CONT = 0x0b,
	P1_FREQ1 = 0x0c,
	P1_FREQ2 = 0x0d,
	P1_FREQ3 = 0x0e,
	P2_FREQ1 = 0x0f,
	P2_FREQ2 = 0x10,
	P2_FREQ3 = 0x11,
	FN_CTRL = 0x12,
	EXT_MOD = 0x13,
	FMOD = 0x14,
	SDI_CTRL = 0x15,
	GPO = 0x16,
	T_VCO = 0x17,
	IQMOD1 = 0x18,
	IQMOD2 = 0x19,
	IQMOD3 = 0x1a,
	IQMOD4 = 0x1b,
	T_CTRL = 0x1c,
	DEV_CTRL = 0x1d,
	TEST = 0x1e,
	READBACK = 0x1f,
};

enum class Readback : uint8_t {
	DeviceID = 0b0000,
	TuningCalibration = 0b0001,
	TuningVoltage = 0b0010,
	StateMachine = 0b0011,
	VCOCountL = 0b0100,
	VCOCountH = 0b0101,
	DCOffsetCal = 0b0110,
	VCOMode = 0b0111,
};

struct LF_Type {
	reg_t pllcpl		: 3;
	reg_t p1cpdef		: 6;
	reg_t p2cpdef		: 6;
	reg_t lfact			: 1;
};

static_assert(sizeof(LF_Type) == sizeof(reg_t), "LF_Type type wrong size");

struct XO_Type {
	reg_t suwait		: 10;
	reg_t xocf			: 1;
	reg_t xoc			: 4;
	reg_t xoch			: 1;
};

static_assert(sizeof(XO_Type) == sizeof(reg_t), "XO_Type type wrong size");

struct CAL_TIME_Type {
	reg_t tkv2			: 4;
	reg_t tkv1			: 4;
	reg_t reserved0		: 2;
	reg_t tct			: 5;
	reg_t wait			: 1;
};

static_assert(sizeof(CAL_TIME_Type) == sizeof(reg_t), "CAL_TIME_Type type wrong size");

struct VCO_CTRL_Type {
	reg_t reserved0		: 1;
	reg_t icpup			: 2;
	reg_t refst			: 1;
	reg_t xoi3			: 1;
	reg_t xoi2			: 1;
	reg_t xoi1			: 1;
	reg_t kvpol			: 1;
	reg_t kvrng			: 1;
	reg_t kvavg			: 2;
	reg_t clkpl			: 1;
	reg_t ctpol			: 1;
	reg_t ctavg			: 2;
	reg_t xtvco			: 1;
};

static_assert(sizeof(VCO_CTRL_Type) == sizeof(reg_t), "VCO_CTRL_Type type wrong size");

struct CT_CAL1_Type {
	reg_t p1ctdef		: 7;
	reg_t p1ct			: 1;
	reg_t p1ctv			: 5;
	reg_t p1ctgain		: 3;
};

static_assert(sizeof(CT_CAL1_Type) == sizeof(reg_t), "CT_CAL1_Type type wrong size");

struct CT_CAL2_Type {
	reg_t p2ctdef		: 7;
	reg_t p2ct			: 1;
	reg_t p2ctv			: 5;
	reg_t p2ctgain		: 3;
};

static_assert(sizeof(CT_CAL2_Type) == sizeof(reg_t), "CT_CAL2_Type type wrong size");

struct PLL_CAL1_Type {
	reg_t reserved0		: 2;
	reg_t p1sgn			: 1;
	reg_t p1kvgain		: 3;
	reg_t p1dn			: 9;
	reg_t p1kv			: 1;
};

static_assert(sizeof(PLL_CAL1_Type) == sizeof(reg_t), "PLL_CAL1_Type type wrong size");

struct PLL_CAL2_Type {
	reg_t reserved0		: 2;
	reg_t p2sgn			: 1;
	reg_t p2kvgain		: 3;
	reg_t p2dn			: 9;
	reg_t p2kv			: 1;
};

static_assert(sizeof(PLL_CAL2_Type) == sizeof(reg_t), "PLL_CAL2_Type type wrong size");

struct VCO_AUTO_Type {
	reg_t reserved0		: 1;
	reg_t ctmin			: 7;
	reg_t ctmax			: 7;
	reg_t auto_			: 1;
};

static_assert(sizeof(VCO_AUTO_Type) == sizeof(reg_t), "VCO_AUTO_Type type wrong size");

struct PLL_CTRL_Type {
	reg_t plldy			: 2;
	reg_t aloi			: 1;
	reg_t relok			: 1;
	reg_t ldlev			: 1;
	reg_t lden			: 1;
	reg_t tvco			: 5;
	reg_t pllst			: 1;
	reg_t clkdiv		: 3;
	reg_t divby			: 1;
};

static_assert(sizeof(PLL_CTRL_Type) == sizeof(reg_t), "PLL_CTRL_Type type wrong size");

struct PLL_BIAS_Type {
	reg_t p2vcoi		: 3;
	reg_t p2loi			: 4;
	reg_t reserved0		: 1;
	reg_t p1vcoi		: 3;
	reg_t p1loi			: 4;
	reg_t reserved1		: 1;
};

static_assert(sizeof(PLL_BIAS_Type) == sizeof(reg_t), "PLL_BIAS_Type type wrong size");

struct MIX_CONT_Type {
	reg_t reserved0		: 9;
	reg_t p2mixidd		: 3;
	reg_t p1mixidd		: 3;
	reg_t fulld			: 1;
};

static_assert(sizeof(MIX_CONT_Type) == sizeof(reg_t), "MIX_CONT_Type type wrong size");

struct P1_FREQ1_Type {
	reg_t p1vcosel		: 2;
	reg_t p1presc		: 2;
	reg_t p1lodiv		: 3;
	reg_t p1n			: 9;
};

static_assert(sizeof(P1_FREQ1_Type) == sizeof(reg_t), "P1_FREQ1_Type type wrong size");

struct P1_FREQ2_Type {
	reg_t p1nmsb		: 16;
};

static_assert(sizeof(P1_FREQ2_Type) == sizeof(reg_t), "P1_FREQ2_Type type wrong size");

struct P1_FREQ3_Type {
	reg_t reserved0		: 8;
	reg_t p1nlsb		: 8;
};

static_assert(sizeof(P1_FREQ3_Type) == sizeof(reg_t), "P1_FREQ3_Type type wrong size");

struct P2_FREQ1_Type {
	reg_t p2vcosel		: 2;
	reg_t p2presc		: 2;
	reg_t p2lodiv		: 3;
	reg_t p2n			: 9;
};

static_assert(sizeof(P2_FREQ1_Type) == sizeof(reg_t), "P2_FREQ1_Type type wrong size");

struct P2_FREQ2_Type {
	reg_t p2nmsb		: 16;
};

static_assert(sizeof(P2_FREQ2_Type) == sizeof(reg_t), "P2_FREQ2_Type type wrong size");

struct P2_FREQ3_Type {
	reg_t reserved0		: 8;
	reg_t p2nlsb		: 8;
};

static_assert(sizeof(P2_FREQ3_Type) == sizeof(reg_t), "P2_FREQ3_Type type wrong size");

struct FN_CTRL_Type {
	reg_t reserved0		: 1;
	reg_t tzps			: 1;
	reg_t dmode			: 1;
	reg_t fm			: 1;
	reg_t dith			: 1;
	reg_t mode			: 1;
	reg_t phsalndly		: 2;
	reg_t phsalngain	: 3;
	reg_t phaln			: 1;
	reg_t sdm			: 2;
	reg_t dithr			: 1;
	reg_t fnz			: 1;
};

static_assert(sizeof(FN_CTRL_Type) == sizeof(reg_t), "FN_CTRL_Type type wrong size");

struct EXT_MOD_Type {
	reg_t reserved0		: 10;
	reg_t modstep		: 4;
	reg_t modsetup		: 2;
};

static_assert(sizeof(EXT_MOD_Type) == sizeof(reg_t), "EXT_MOD_Type type wrong size");

struct FMOD_Type {
	reg_t modulation	: 16;
};

static_assert(sizeof(FMOD_Type) == sizeof(reg_t), "FMOD_Type type wrong size");

struct SDI_CTRL_Type {
	reg_t reserved0		: 1;
	reg_t reset			: 1;
	reg_t reserved1		: 9;
	reg_t addr			: 1;
	reg_t fourwire		: 1;
	reg_t mode			: 1;
	reg_t enbl			: 1;
	reg_t sipin			: 1;
};

static_assert(sizeof(SDI_CTRL_Type) == sizeof(reg_t), "SDI_CTRL_Type type wrong size");

struct GPO_Type {
	reg_t lock			: 1;
	reg_t gate			: 1;
	reg_t p1gpo			: 7;
	reg_t p2gpo			: 7;
};

static_assert(sizeof(GPO_Type) == sizeof(reg_t), "GPO_Type type wrong size");

struct T_VCO_Type {
	reg_t reserved0		: 7;
	reg_t curve_vco3	: 3;
	reg_t curve_vco2	: 3;
	reg_t curve_vco1	: 3;
};

static_assert(sizeof(T_VCO_Type) == sizeof(reg_t), "T_VCO_Type type wrong size");

struct IQMOD1_Type {
	reg_t bufdc			: 2;
	reg_t divbias		: 1;
	reg_t calblk		: 1;
	reg_t calnul		: 1;
	reg_t calon			: 1;
	reg_t lobias		: 2;
	reg_t modbias		: 3;
	/* Also defined as ctrl : 5 */
	reg_t modiv			: 1;
	reg_t mod			: 1;
	reg_t txlo			: 1;
	reg_t bbgm			: 1;
	reg_t ctrl			: 1;
};

static_assert(sizeof(IQMOD1_Type) == sizeof(reg_t), "IQMOD1_Type type wrong size");

struct IQMOD2_Type {
	reg_t modbuf		: 2;
	reg_t mod			: 2;
	reg_t calatten		: 2;
	reg_t rctune		: 6;
	reg_t bbatten		: 4;
};

static_assert(sizeof(IQMOD2_Type) == sizeof(reg_t), "IQMOD2_Type type wrong size");

struct IQMOD3_Type {
	/* Documentation error */
	reg_t reserved0		: 3;
	reg_t dacen			: 1;
	reg_t bufdacq		: 6;
	reg_t bufdaci		: 6;
};

static_assert(sizeof(IQMOD3_Type) == sizeof(reg_t), "IQMOD3_Type type wrong size");

struct IQMOD4_Type {
	/* Documentation error */
	reg_t bufbias2		: 2;
	reg_t bufbias1		: 2;
	reg_t moddacq		: 6;
	reg_t moddaci		: 6;
};

static_assert(sizeof(IQMOD4_Type) == sizeof(reg_t), "IQMOD4_Type type wrong size");

struct T_CTRL_Type {
	reg_t reserved0		: 5;
	reg_t v_test		: 1;
	reg_t ldo_by		: 1;
	reg_t ext_filt		: 1;
	reg_t ref_sel		: 1;
	reg_t filt_ctrl		: 2;
	reg_t fc_en			: 1;
	reg_t tbl_sel		: 2;
	reg_t tc_en			: 2;
};

static_assert(sizeof(T_CTRL_Type) == sizeof(reg_t), "T_CTRL_Type type wrong size");

struct DEV_CTRL_Type {
	reg_t reserved0		: 1;
	reg_t bypas			: 1;
	reg_t ctclk			: 1;
	reg_t dac			: 1;
	reg_t cpd			: 1;
	reg_t cpu			: 1;
	reg_t rsmstopst		: 5;
	reg_t rsmst			: 1;
	reg_t readsel		: 4;
};

static_assert(sizeof(DEV_CTRL_Type) == sizeof(reg_t), "DEV_CTRL_Type type wrong size");

struct TEST_Type {
	reg_t lfsrd			: 1;
	reg_t rcbyp			: 1;
	reg_t rgbyp			: 1;
	reg_t lfsrt			: 1;
	reg_t lfsrgatetime	: 4;
	reg_t lfsrp			: 1;
	reg_t lfsr			: 1;
	reg_t tsel			: 2;
	reg_t tmux			: 3;
	reg_t ten			: 1;
};

static_assert(sizeof(TEST_Type) == sizeof(reg_t), "TEST_Type type wrong size");

struct READBACK_0000_Type {
	reg_t mrev_id		: 3;
	reg_t dev_id		: 13;
};

static_assert(sizeof(READBACK_0000_Type) == sizeof(reg_t), "READBACK_0000_Type type wrong size");

struct READBACK_0001_Type {
	reg_t reserved0		: 1;
	reg_t ctfail		: 1;
	reg_t cp_cal		: 6;
	reg_t ct_cal		: 7;
	reg_t lock			: 1;
};

static_assert(sizeof(READBACK_0001_Type) == sizeof(reg_t), "READBACK_0001_Type type wrong size");

struct READBACK_0010_Type {
	reg_t v1_cal		: 8;
	reg_t v0_cal		: 8;
};

static_assert(sizeof(READBACK_0010_Type) == sizeof(reg_t), "READBACK_0010_Type type wrong size");

struct READBACK_0011_Type {
	reg_t reserved0		: 9;
	reg_t f_errflag		: 2;
	reg_t rsm_state		: 5;
};

static_assert(sizeof(READBACK_0011_Type) == sizeof(reg_t), "READBACK_0011_Type type wrong size");

struct READBACK_0100_Type {
	reg_t vco_count_l	: 16;
};

static_assert(sizeof(READBACK_0100_Type) == sizeof(reg_t), "READBACK_0100_Type type wrong size");

struct READBACK_0101_Type {
	reg_t vco_count_h	: 16;
};

static_assert(sizeof(READBACK_0101_Type) == sizeof(reg_t), "READBACK_0101_Type type wrong size");

struct READBACK_0110_Type {
	reg_t reserved0		: 14;
	reg_t cal_fbq		: 1;
	reg_t cal_fbi		: 1;
};

static_assert(sizeof(READBACK_0110_Type) == sizeof(reg_t), "READBACK_0110_Type type wrong size");

struct READBACK_0111_Type {
	reg_t reserved0		: 11;
	reg_t vco_tc_curve	: 3;
	reg_t vco_sel		: 2;
};

static_assert(sizeof(READBACK_0111_Type) == sizeof(reg_t), "READBACK_0111_Type type wrong size");

struct Register_Type {
	LF_Type			lf;
	XO_Type			xo;
	CAL_TIME_Type	cal_time;
	VCO_CTRL_Type	vco_ctrl;
	CT_CAL1_Type	ct_cal1;
	CT_CAL2_Type	ct_cal2;
	PLL_CAL1_Type	pll_cal1;
	PLL_CAL2_Type	pll_cal2;
	VCO_AUTO_Type	vco_auto;
	PLL_CTRL_Type	pll_ctrl;
	PLL_BIAS_Type	pll_bias;
	MIX_CONT_Type	mix_cont;
	P1_FREQ1_Type	p1_freq1;
	P1_FREQ2_Type	p1_freq2;
	P1_FREQ3_Type	p1_freq3;
	P2_FREQ1_Type	p2_freq1;
	P2_FREQ2_Type	p2_freq2;
	P2_FREQ3_Type	p2_freq3;
	FN_CTRL_Type	fn_ctrl;
	EXT_MOD_Type	ext_mod;
	FMOD_Type		fmod;
	SDI_CTRL_Type	sdi_ctrl;
	GPO_Type		gpo;
	T_VCO_Type		t_vco;
	IQMOD1_Type		iqmod1;
	IQMOD2_Type		iqmod2;
	IQMOD3_Type		iqmod3;
	IQMOD4_Type		iqmod4;
	T_CTRL_Type		t_ctrl;
	DEV_CTRL_Type	dev_ctrl;
	TEST_Type		test;
};

static_assert(sizeof(Register_Type) == reg_count * sizeof(reg_t), "Register_Type wrong size");

struct RegisterMap {
	constexpr RegisterMap(
		Register_Type values
	) : r(values)
	{
	}

	union {
		Register_Type r;
		std::array<reg_t, reg_count> w;
	};
};

static_assert(sizeof(RegisterMap) == reg_count * sizeof(reg_t), "RegisterMap type wrong size");
#if 0
struct ReadbackType {
	union {
		READBACK_0000_Type	readback_0000;
		READBACK_0001_Type	readback_0001;
		READBACK_0010_Type	readback_0010;
		READBACK_0011_Type	readback_0011;
		READBACK_0100_Type	readback_0100;
		READBACK_0101_Type	readback_0101;
		READBACK_0110_Type	readback_0110;
		READBACK_0111_Type	readback_0111;
		reg_t w;
	};
};
#endif
#if 0
/* Revision 1 devices (mrev_id = 001):
 * RFFC2071/2072/5071/5072, RFMD2080/2081
 */
constexpr RegisterMap default_revision_1 { std::array<reg_t, reg_count> {
  0xbefa, 0x4064, 0x9055, 0x2d02,
  0xb0bf, 0xb0bf, 0x0028, 0x0028,
  0xfc06, 0x8220, 0x0202, 0x4800,
  0x2324, 0x6276, 0x2700, 0x2f16,
  0x3b13, 0xb100, 0x2a80, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000,
  0x0283, 0xf00f, 0x0000, 0x000f,
  0x0000, 0x1000, 0x0001,
} };

/* Revision 2 devices (mrev_id = 010):
 * RFFC2071A/2072A/5071A/5072A
 */
constexpr RegisterMap default_revision_2 { std::array<reg_t, reg_count> {
  0xbefa, 0x4064, 0x9055, 0x2d02,
  0xacbf, 0xacbf, 0x0028, 0x0028,
  0xff00, 0x8220, 0x0202, 0x4800,
  0x1a94, 0xd89d, 0x8900, 0x1e84,
  0x89d8, 0x9d00, 0x2a80, 0x0000,
  0x0000, 0x0000, 0x0000, 0x4900,
  0x0281, 0xf00f, 0x0000, 0x0005,
  0xc840, 0x1000, 0x0005,
} };
#endif
constexpr RegisterMap default_hackrf_one { Register_Type {
	/* Started with recommended defaults for revision 1 devices
	 * (mrev_id = 001), RFFC2071/2072/5071/5072, RFMD2080/2081.
	 * Modified according to mixer programming guide.
	 */
	.lf = {		/* 0 */
		.pllcpl		= 0b010,
		.p1cpdef	= 0b011111,
		.p2cpdef	= 0b011111,
		.lfact		= 1,
	},
	.xo = {		/* 1 */
		.suwait		= 0b0001100100,
		.xocf		= 0b0,
		.xoc		= 0b1000,
		.xoch		= 0b0,
	},
	.cal_time = {	/* 2 */
		.tkv2		= 0b0101,
		.tkv1		= 0b0101,
		.reserved0	= 0b00,
		.tct		= 0b00100,
		.wait		= 0b1,
	},
	.vco_ctrl = {	/* 3 */
		.reserved0	= 0b0,
		.icpup		= 0b01,
		.refst		= 0b0,
		.xoi3		= 0b0,
		.xoi2		= 0b0,
		.xoi1		= 0b0,
		.kvpol		= 0b0,
		.kvrng		= 0b1,
		.kvavg		= 0b10,
		.clkpl		= 0b1,
		.ctpol		= 0b0,
		.ctavg		= 0b01,
		.xtvco		= 0b0,
	},
	.ct_cal1 = {	/* 4 */
		.p1ctdef	= 0b0111111,
		.p1ct		= 0b1,
		.p1ctv		= 12,			/* RFMD recommneded change: 16 -> 12 */
		.p1ctgain	= 0b101,
	},
	.ct_cal2 = {	/* 5 */
		.p2ctdef	= 0b0111111,
		.p2ct		= 0b1,
		.p2ctv		= 12,			/* RFMD recommneded change: 16 -> 12 */
		.p2ctgain	= 0b101,
	},
	.pll_cal1 = {	/* 6 */
		.reserved0	= 0b00,
		.p1sgn		= 0b0,
		.p1kvgain	= 0b101,
		.p1dn		= 0b000000000,
		.p1kv		= 0b0,
	},
	.pll_cal2 = {	/* 7 */
		.reserved0	= 0b00,
		.p2sgn		= 0b0,
		.p2kvgain	= 0b101,
		.p2dn		= 0b000000000,
		.p2kv		= 0b0,
	},
	.vco_auto = {	/* 8 */
		.reserved0	= 0b0,
		.ctmin		= 0,			/* RFMD recommended change:   3 ->   0 */
		.ctmax		= 127,			/* RFMD recommended change: 124 -> 127 */
		.auto_		= 0b1,
	},
	.pll_ctrl = {	/* 9 */
		.plldy		= 0b00,
		.aloi		= 0b0,
		.relok		= 0b0,
		.ldlev		= 0b0,
		.lden		= 0b1,
		.tvco		= 0b01000,
		.pllst		= 0b0,
		.clkdiv		= 0b000,
		.divby		= 0b1,
	},
	.pll_bias = {	/* 10 */
		.p2vcoi		= 0b010,
		.p2loi		= 0b0000,
		.reserved0	= 0b0,
		.p1vcoi		= 0b010,
		.p1loi		= 0b0000,
		.reserved1	= 0b0,
	},
	.mix_cont = {	/* 11 */
		.reserved0	= 0b000000000,
		.p2mixidd	= 4,
		.p1mixidd	= 4,
		.fulld		= 0b0,			/* Part on HackRF is half-duplex (single mixer) */
	},
	.p1_freq1 = {	/* 12 */
		.p1vcosel	= 0b00,			/* RFMD VCO bank 1 configuration from A series part */
		.p1presc	= 0b01,
		.p1lodiv	= 0b001,
		.p1n		= 0b000110101,
	},
	.p1_freq2 = {	/* 13 */
		.p1nmsb		= 0xd89d,		/* RFMD VCO bank 1 configuration from A series part */
	},
	.p1_freq3 = {	/* 14 */
		.reserved0	= 0b00000000,	/* RFMD VCO bank 1 configuration from A series part */
		.p1nlsb		= 0x89,
	},
	.p2_freq1 = {	/* 15 */
		.p2vcosel	= 0b00,			/* RFMD VCO bank 2 configuration from A series part */
		.p2presc	= 0b01,
		.p2lodiv	= 0b000,
		.p2n		= 0b000111101,
	},
	.p2_freq2 = {	/* 16 */
		.p2nmsb		= 0x89d8,		/* RFMD VCO bank 2 configuration from A series part */
	},
	.p2_freq3 = {	/* 17 */
		.reserved0	= 0b00000000,	/* RFMD VCO bank 2 configuration from A series part */
		.p2nlsb		= 0x9d,
	},
	.fn_ctrl = {	/* 18 */
		.reserved0	= 0b0,
		.tzps		= 0b0,
		.dmode		= 0b0,
		.fm			= 0b0,
		.dith		= 0b0,
		.mode		= 0b0,
		.phsalndly	= 0b10,
		.phsalngain	= 0b010,
		.phaln		= 0b1,
		.sdm		= 0b10,
		.dithr		= 0b0,
		.fnz		= 0b0,
	},
	.ext_mod = {	/* 19 */
		.reserved0	= 0b0000000000,
		.modstep	= 0b0000,
		.modsetup	= 0b00,
	},
	.fmod = {		/* 20 */
		.modulation	= 0x0000,
	},
	.sdi_ctrl = {	/* 21 */
		.reserved0	= 0b0,
		.reset		= 0b0,
		.reserved1	= 0b000000000,
		.addr		= 0b0,
		.fourwire	= 0b0,			/* Use three pin SPI mode */
		.mode		= 0b1,			/* Active PLL register bank 2, active mixer 2 */
		.enbl		= 0b0,			/* Part is initially disabled */
		.sipin		= 0b1,			/* Control MODE, ENBL from SPI bus */
	},
	.gpo = {		/* 22 */
		.lock		= 0b1,			/* Present LOCK signal on GPIO4/LD/DO, HackRF One test point P6 */
		.gate		= 0b1,			/* GPOs active even when part is disabled (ENBL=0) */
		.p1gpo		= 0b0000001,	/* Turn on GPO1 to turn *off* !ANT_BIAS (GPO numbering is one-based) */
		.p2gpo		= 0b0000001,	/* Turn on GPO1 to turn *off* !ANT_BIAS (GPO numbering is one-based) */
	},
	.t_vco = {	/* 23 */
		.reserved0	= 0b0000000,
		.curve_vco3	= 0b000,
		.curve_vco2	= 0b000,
		.curve_vco1	= 0b000,
	},
	.iqmod1 = {	/* 24 */
		.bufdc		= 0b11,
		.divbias	= 0b0,
		.calblk		= 0b0,
		.calnul		= 0b0,
		.calon		= 0b0,
		.lobias		= 0b10,
		.modbias	= 0b010,
		.modiv		= 0b0,
		.mod		= 0b0,
		.txlo		= 0b0,
		.bbgm		= 0b0,
		.ctrl		= 0b0,
	},
	.iqmod2 = {	/* 25 */
		.modbuf		= 0b11,
		.mod		= 0b11,
		.calatten	= 0b00,
		.rctune		= 0b000000,
		.bbatten	= 0b1111,
	},
	.iqmod3 = {	/* 26 */
		.reserved0	= 0b000,
		.dacen		= 0b0,
		.bufdacq	= 0b000000,
		.bufdaci	= 0b000000,
	},
	.iqmod4 = {	/* 27 */
		.bufbias2	= 0b11,
		.bufbias1	= 0b11,
		.moddacq	= 0b000000,
		.moddaci	= 0b000000,
	},
	.t_ctrl = {	/* 28 */
		.reserved0	= 0b00000,
		.v_test		= 0b0,
		.ldo_by		= 0b0,
		.ext_filt	= 0b0,
		.ref_sel	= 0b0,
		.filt_ctrl	= 0b00,
		.fc_en		= 0b0,
		.tbl_sel	= 0b00,
		.tc_en		= 0b00,
	},
	.dev_ctrl = {	/* 29 */
		.reserved0	= 0b0,
		.bypas		= 0b0,
		.ctclk		= 0b0,
		.dac		= 0b0,
		.cpd		= 0b0,
		.cpu		= 0b0,
		.rsmstopst	= 0b00000,
		.rsmst		= 0b0,
		.readsel	= 0b0001,
	},
	.test = {		/* 30 */
		.lfsrd			= 0b1,
		.rcbyp			= 0b0,
		.rgbyp			= 0b1,		/* RFMD recommended change: 0 -> 1 */
		.lfsrt			= 0b0,
		.lfsrgatetime	= 0b0000,
		.lfsrp			= 0b0,
		.lfsr			= 0b0,
		.tsel			= 0b00,
		.tmux			= 0b000,
		.ten			= 0b0,
	},
} };

class RFFC507x {
public:
	void init();
	void reset();

	void flush();

	void enable();
	void disable();

	void set_mixer_current(const uint8_t value);
	void set_frequency(const rf::Frequency lo_frequency);
	void set_gpo1(const bool new_value);
	
	reg_t read(const address_t reg_num);

private:
	spi::SPI _bus { };

	RegisterMap _map { default_hackrf_one };
	DirtyRegisters<Register, reg_count> _dirty { };

	void write(const address_t reg_num, const reg_t value);

	void write(const Register reg, const reg_t value);
	reg_t read(const Register reg);

	void flush_one(const Register reg);

	reg_t readback(const Readback readback);

	void init_for_best_performance();
};

} /* rffc507x */

#endif/*__RFFC507X_H__*/
