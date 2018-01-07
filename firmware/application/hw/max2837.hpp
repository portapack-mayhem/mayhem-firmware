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

#ifndef __MAX2837_H__
#define __MAX2837_H__

#include "gpio.hpp"
#include "spi_arbiter.hpp"

#include <cstdint>
#include <array>

#include "dirty_registers.hpp"
#include "rf_path.hpp"
#include "utility.hpp"

namespace max2837 {

enum class Mode {
	Mask_Enable = 0b001,
	Mask_RxEnable = 0b010,
	Mask_TxEnable = 0b100,

	Shutdown = 0b000,
	Standby = Mask_Enable,
	Receive = Mask_Enable | Mask_RxEnable,
	Transmit = Mask_Enable | Mask_TxEnable,
};

/*************************************************************************/

namespace lo {

constexpr std::array<rf::FrequencyRange, 4> band { {
	{ 2300000000, 2400000000 },
	{ 2400000000, 2500000000 },
	{ 2500000000, 2600000000 },
	{ 2600000000, 2700000000 },
} };

} /* namespace lo */

/*************************************************************************/

namespace lna {

constexpr range_t<int8_t> gain_db_range { 0, 40 };
constexpr int8_t gain_db_step = 8;

constexpr std::array<rf::FrequencyRange, 2> band { {
	{ 2300000000, 2500000000 },
	{ 2500000000, 2700000000 },
} };

} /* namespace lna */

/*************************************************************************/

namespace vga {

constexpr range_t<int8_t> gain_db_range { 0, 62 };
constexpr int8_t gain_db_step = 2;

} /* namespace vga */

/*************************************************************************/

namespace tx {

constexpr range_t<int8_t> gain_db_range { 0, 47 };
constexpr int8_t gain_db_step = 1;
}

/*************************************************************************/

namespace filter {

constexpr std::array<uint32_t, 16> bandwidths {
	/* Assumption: these values are in ascending order */
	 1750000,
	 2500000,	/* Some documentation says 2.25MHz */
	 3500000,
	 5000000,
	 5500000,
	 6000000,
	 7000000,
	 8000000,
	 9000000,
	10000000,
	12000000,
	14000000,
	15000000,
	20000000,
	24000000,
	28000000,
};

constexpr auto bandwidth_minimum = bandwidths[0];
constexpr auto bandwidth_maximum = bandwidths[bandwidths.size() - 1];

} /* namespace filter */

/*************************************************************************/

using reg_t = uint16_t;
using address_t = uint8_t;

constexpr size_t reg_count = 32;

enum class Register : address_t {
	RXRF_1			= 0,
	RXRF_2			= 1,
	LPF_1			= 2,
	LPF_2			= 3,
	LPF_3_VGA_1		= 4,
	VGA_2			= 5,
	VGA_3_RX_TOP	= 6,
	TEMP_SENSE		= 7,
	RX_TOP_RX_BIAS	= 8,
	RX_TOP			= 9,
	TX_TOP_1		= 10,
	TX_TOP_2		= 11,
	HPFSM_1			= 12,
	HPFSM_2			= 13,
	HPFSM_3			= 14,
	HPFSM_4			= 15,
	SPI_EN			= 16,
	SYN_FR_DIV_1	= 17,
	SYN_FR_DIV_2	= 18,
	SYN_INT_DIV		= 19,
	SYN_CFG_1		= 20,
	SYN_CFG_2		= 21,
	VAS_CFG			= 22,
	LO_MISC			= 23,
	XTAL_CFG		= 24,
	VCO_CFG			= 25,
	LO_GEN			= 26,
	PA_DRV_PA_DAC	= 27,
	PA_DAC			= 28,
	TX_GAIN			= 29,
	TX_LO_IQ		= 30,
	TX_DC_CORR		= 31,
};

struct RXRF_1_Type {
	reg_t LNA_EN	: 1;
	reg_t Mixer_EN	: 1;
	reg_t RxLO_EN	: 1;
	reg_t Lbias		: 2;
	reg_t Mbias		: 2;
	reg_t buf		: 2;
	reg_t LNAband	: 1;
	reg_t RESERVED0	: 6;
};

static_assert(sizeof(RXRF_1_Type) == sizeof(reg_t), "RXRF_1_Type wrong size");

struct RXRF_2_Type {
	reg_t LNAtune		: 1;
	reg_t LNAde_Q		: 1;
	reg_t L				: 3;
	reg_t iqerr_trim	: 5;
	reg_t RESERVED0		: 6;
};

static_assert(sizeof(RXRF_2_Type) == sizeof(reg_t), "RXRF_2_Type wrong size");

struct LPF_1_Type {
	reg_t LPF_EN	: 1;
	reg_t TxBB_EN	: 1;
	reg_t ModeCtrl	: 2;
	reg_t FT		: 4;
	reg_t dF		: 2;
	reg_t RESERVED0 : 6;
};

static_assert(sizeof(LPF_1_Type) == sizeof(reg_t), "LPF_1_Type wrong size");

struct LPF_2_Type {
	reg_t PT_SPI	: 4;
	reg_t Bqd		: 3;
	reg_t TxRPCM	: 3;
	reg_t RESERVED0	: 6;
};

static_assert(sizeof(LPF_2_Type) == sizeof(reg_t), "LPF_2_Type wrong size");

struct LPF_3_VGA_1_Type {
	reg_t RP			: 2;
	reg_t TxBuff		: 2;
	reg_t VGA_EN		: 1;
	reg_t VGAMUX_enable	: 1;
	reg_t BUFF_Curr		: 2;
	reg_t BUFF_VCM		: 2;
	reg_t RESERVED0		: 6;
};

static_assert(sizeof(LPF_3_VGA_1_Type) == sizeof(reg_t), "LPF_3_VGA_1_Type wrong size");

struct VGA_2_Type {
	reg_t VGA			: 5;
	reg_t sel_In1_In2	: 1;
	reg_t turbo15n20	: 1;
	reg_t VGA_Curr		: 2;
	reg_t fuse_arm		: 1;
	reg_t RESERVED0		: 6;
};

static_assert(sizeof(VGA_2_Type) == sizeof(reg_t), "VGA_2_Type wrong size");

struct VGA_3_RX_TOP_Type {
	reg_t RESERVED0				: 6;
	reg_t RSSI_EN_SPIenables	: 1;
	reg_t RSSI_MUX				: 1;
	reg_t RSSI_MODE				: 1;
	reg_t LPF_MODE_SEL			: 1;
	reg_t RESERVED1				: 6;
};

static_assert(sizeof(VGA_3_RX_TOP_Type) == sizeof(reg_t), "VGA_3_RX_TOP_Type wrong size");

struct TEMP_SENSE_Type {
	reg_t ts_adc							: 5;
	reg_t RESERVED0							: 1;
	reg_t PLL_test_output					: 1;
	reg_t VAS_test_output					: 1;
	reg_t HPFSM_test_output					: 1;
	reg_t LOGEN_trim_divider_test_output	: 1;
	reg_t RESERVED1							: 6;
};

static_assert(sizeof(TEMP_SENSE_Type) == sizeof(reg_t), "TEMP_SENSE_Type wrong size");

struct RX_TOP_RX_BIAS_Type {
	reg_t LNAgain_SPI_EN	: 1;
	reg_t VGAgain_SPI_EN	: 1;
	reg_t EN_Bias_Trim		: 1;
	reg_t BIAS_TRIM_SPI		: 5;
	reg_t BIAS_TRIM_CNTRL	: 1;
	reg_t RX_IQERR_SPI_EN	: 1;
	reg_t RESERVED0			: 6;
};

static_assert(sizeof(RX_TOP_RX_BIAS_Type) == sizeof(reg_t), "RX_TOP_RX_BIAS_Type wrong size");

struct RX_TOP_Type {
	reg_t ts_adc_trigger	: 1;
	reg_t ts_en				: 1;
	reg_t LPFtrim_SPI_EN	: 1;
	reg_t DOUT_DRVH			: 1;
	reg_t DOUT_PU			: 1;
	reg_t DOUT_SEL			: 3;
	reg_t fuse_th			: 1;
	reg_t fuse_burn_gkt		: 1;
	reg_t RESERVED0			: 6;
};

static_assert(sizeof(RX_TOP_Type) == sizeof(reg_t), "RX_TOP_Type wrong size");

struct TX_TOP_1_Type {
	reg_t RESERVED0			: 1;
	reg_t TXCAL_GAIN		: 2;
	reg_t TXCAL_V2I_FILT	: 3;
	reg_t TX_BIAS_ADJ		: 2;
	reg_t RESERVED1			: 2;
	reg_t RESERVED2			: 6;
};

static_assert(sizeof(TX_TOP_1_Type) == sizeof(reg_t), "TX_TOP_1_Type wrong size");

struct TX_TOP_2_Type {
	reg_t AMD_SPI_EN		: 1;
	reg_t TXMXR_V2I_GAIN	: 4;
	reg_t RESERVED0			: 5;
	reg_t RESERVED1			: 6;
};

static_assert(sizeof(TX_TOP_2_Type) == sizeof(reg_t), "TX_TOP_2_Type wrong size");

struct HPFSM_1_Type {
	reg_t HPC_10M		: 2;
	reg_t HPC_10M_GAIN	: 2;
	reg_t HPC_600k		: 3;
	reg_t HPC_600k_GAIN	: 3;
	reg_t RESERVED0		: 6;
};

static_assert(sizeof(HPFSM_1_Type) == sizeof(reg_t), "HPFSM_1_Type wrong size");

struct HPFSM_2_Type {
	reg_t HPC_100k		: 2;
	reg_t HPC_100k_GAIN	: 2;
	reg_t HPC_30k		: 2;
	reg_t HPC_30k_GAIN	: 2;
	reg_t HPC_1k		: 2;
	reg_t RESERVED0		: 6;
};

static_assert(sizeof(HPFSM_2_Type) == sizeof(reg_t), "HPFSM_2_Type wrong size");

struct HPFSM_3_Type {
	reg_t HPC_1k_GAIN	: 2;
	reg_t HPC_DELAY		: 2;
	reg_t HPC_STOP		: 2;
	reg_t HPC_STOP_M2	: 2;
	reg_t HPC_RXGAIN_EN	: 1;
	reg_t HPC_MODE		: 1;
	reg_t RESERVED0		: 6;
};

static_assert(sizeof(HPFSM_3_Type) == sizeof(reg_t), "HPFSM_3_Type wrong size");

struct HPFSM_4_Type {
	reg_t HPC_DIVH		: 1;
	reg_t HPC_TST		: 5;
	reg_t HPC_SEQ_BYP	: 1;
	reg_t DOUT_CSB_SEL	: 1;
	reg_t RESERVED0		: 2;
	reg_t RESERVED1		: 6;
};

static_assert(sizeof(HPFSM_4_Type) == sizeof(reg_t), "HPFSM_4_Type wrong size");

struct SPI_EN_Type {
	reg_t EN_SPI		: 1;
	reg_t CAL_SPI		: 1;
	reg_t LOGEN_SPI_EN	: 1;
	reg_t SYN_SPI_EN	: 1;
	reg_t VAS_SPI_EN	: 1;
	reg_t PADRV_SPI_EN	: 1;
	reg_t PADAC_SPI_EN 	: 1;
	reg_t PADAC_TX_EN	: 1;
	reg_t TXMX_SPI_EN	: 1;
	reg_t TXLO_SPI_EN	: 1;
	reg_t RESERVED0		: 6;
};

static_assert(sizeof(SPI_EN_Type) == sizeof(reg_t), "SPI_EN_Type wrong size");

struct SYN_FR_DIV_1_Type {
	reg_t SYN_FRDIV_9_0	: 10;
	reg_t RESERVED0		: 6;
};

static_assert(sizeof(SYN_FR_DIV_1_Type) == sizeof(reg_t), "SYN_FR_DIV_1_Type wrong size");

struct SYN_FR_DIV_2_Type {
	reg_t SYN_FRDIV_19_10	: 10;
	reg_t RESERVED0			: 6;
};

static_assert(sizeof(SYN_FR_DIV_2_Type) == sizeof(reg_t), "SYN_FR_DIV_2_Type wrong size");

struct SYN_INT_DIV_Type {
	reg_t SYN_INTDIV	: 8;
	reg_t LOGEN_BSW		: 2;
	reg_t RESERVED0		: 6;
};

static_assert(sizeof(SYN_INT_DIV_Type) == sizeof(reg_t), "SYN_INT_DIV_Type wrong size");

struct SYN_CFG_1_Type {
	reg_t SYN_MODE_FR_EN		: 1;
	reg_t SYN_REF_DIV_RATIO		: 2;
	reg_t SYN_CP_CURRENT		: 2;
	reg_t SYN_CLOCKOUT_DRIVE	: 1;
	reg_t SYN_TURBO_EN			: 1;
	reg_t CP_TRM_SET			: 1;
	reg_t CP_TRM_CODE			: 2;
	reg_t RESERVED0				: 6;
};

static_assert(sizeof(SYN_CFG_1_Type) == sizeof(reg_t), "SYN_CFG_1_Type wrong size");

struct SYN_CFG_2_Type {
	reg_t SYN_CP_COMMON_MODE_EN			: 1;
	reg_t SYN_PRESCALER_BIAS_BOOST		: 1;
	reg_t SYN_CP_BETA_CURRENT_COMP_EN	: 1;
	reg_t SYN_SD_CLK_SEL				: 1;
	reg_t SYN_CP_PW_ADJ					: 1;
	reg_t SYN_CP_LIN_CURRENT_SEL		: 2;
	reg_t SYN_TEST_OUT_SEL				: 3;
	reg_t RESERVED0						: 6;
};

static_assert(sizeof(SYN_CFG_2_Type) == sizeof(reg_t), "SYN_CFG_2_Type wrong size");

struct VAS_CFG_Type {
	reg_t VAS_MODE			: 1;
	reg_t VAS_RELOCK_SEL	: 1;
	reg_t VAS_DIV			: 3;
	reg_t VAS_DLY			: 2;
	reg_t VAS_TRIG_EN		: 1;
	reg_t VAS_ADE			: 1;
	reg_t VAS_ADL_SPI		: 1;
	reg_t RESERVED0			: 6;
};

static_assert(sizeof(VAS_CFG_Type) == sizeof(reg_t), "VAS_CFG_Type wrong size");

struct LO_MISC_Type {
	reg_t VAS_SPI			: 5;
	reg_t XTAL_BIAS_SEL		: 2;
	reg_t XTAL_E2C_BIAS_SEL	: 1;
	reg_t VAS_SE			: 1;
	reg_t VCO_SPI_EN		: 1;
	reg_t RESERVED0			: 6;
};

static_assert(sizeof(LO_MISC_Type) == sizeof(reg_t), "LO_MISC_Type wrong size");

struct XTAL_CFG_Type {
	reg_t XTAL_FTUNE		: 7;
	reg_t XTAL_CLKOUT_EN	: 1;
	reg_t XTAL_CLKOUT_DIV	: 1;
	reg_t XTAL_CORE_EN		: 1;
	reg_t RESERVED0			: 6;
};

static_assert(sizeof(XTAL_CFG_Type) == sizeof(reg_t), "XTAL_CFG_Type wrong size");

struct VCO_CFG_Type {
	reg_t VCO_BIAS_SPI_EN	: 1;
	reg_t VCO_BIAS_SPI		: 4;
	reg_t VCO_CMEN			: 1;
	reg_t VCO_PDET_TST		: 2;
	reg_t VCO_BUF_BIASH		: 2;
	reg_t RESERVED0			: 6;
};

static_assert(sizeof(VCO_CFG_Type) == sizeof(reg_t), "VCO_CFG_Type wrong size");

struct LO_GEN_Type {
	reg_t LOGEN_BIASH1	: 2;
	reg_t LOGEN_BIASH2	: 1;
	reg_t LOGEN_2GM		: 1;
	reg_t LOGEN_TRIM1	: 1;
	reg_t LOGEN_TRIM2	: 1;
	reg_t VAS_TST		: 4;
	reg_t RESERVED0		: 6;
};

static_assert(sizeof(LO_GEN_Type) == sizeof(reg_t), "LO_GEN_Type wrong size");

struct PA_DRV_PA_DAC_Type {
	reg_t PADRV_BIAS			: 3;
	reg_t PADRV_DOWN_SPI_EN		: 1;
	reg_t PADRV_DOWN_SPI_SEL	: 1;
	reg_t PADAC_IV				: 1;
	reg_t PADAC_VMODE			: 1;
	reg_t PADAC_DIVH			: 1;
	reg_t TXGATE_EN				: 1;
	reg_t TX_DCCORR_EN			: 1;
	reg_t RESERVED0				: 6;
};

static_assert(sizeof(PA_DRV_PA_DAC_Type) == sizeof(reg_t), "PA_DRV_PA_DAC_Type wrong size");

struct PA_DAC_Type {
	reg_t PADAC_BIAS	: 6;
	reg_t PADAC_DLY		: 4;
	reg_t RESERVED0		: 6;
};

static_assert(sizeof(PA_DAC_Type) == sizeof(reg_t), "PA_DAC_Type wrong size");

struct TX_GAIN_Type {
	reg_t TXVGA_GAIN_SPI_EN		: 1;
	reg_t TXVGA_GAIN_MSB_SPI_EN	: 1;
	reg_t TX_DCCORR_SPI_EN		: 1;
	reg_t FUSE_ARM				: 1;
	reg_t TXVGA_GAIN_SPI		: 6;
	reg_t RESERVED0				: 6;
};

static_assert(sizeof(TX_GAIN_Type) == sizeof(reg_t), "TX_GAIN_Type wrong size");

struct TX_LO_IQ_Type {
	reg_t TXLO_IQ_SPI		: 5;
	reg_t TXLO_IQ_SPI_EN	: 1;
	reg_t TXLO_BUFF			: 2;
	reg_t FUSE_GKT			: 1;
	reg_t FUSE_RTH			: 1;
	reg_t RESERVED0			: 6;
};

static_assert(sizeof(TX_LO_IQ_Type) == sizeof(reg_t), "TX_LO_IQ_Type wrong size");

struct TX_DC_CORR_Type {
	reg_t TX_DCCORR_I	: 5;
	reg_t TX_DCCORR_Q	: 5;
	reg_t RESERVED0		: 6;
};

static_assert(sizeof(TX_DC_CORR_Type) == sizeof(reg_t), "TX_DC_CORR_Type wrong size");

struct Register_Type {
	RXRF_1_Type			rxrf_1;			/*  0 */
	RXRF_2_Type			rxrf_2;
	LPF_1_Type			lpf_1;
	LPF_2_Type			lpf_2;
	LPF_3_VGA_1_Type	lpf_3_vga_1;	/*  4 */
	VGA_2_Type			vga_2;
	VGA_3_RX_TOP_Type	vga_3_rx_top;
	TEMP_SENSE_Type		temp_sense;
	RX_TOP_RX_BIAS_Type	rx_top_rx_bias;	/*  8 */
	RX_TOP_Type			rx_top;
	TX_TOP_1_Type		tx_top_1;
	TX_TOP_2_Type		tx_top_2;
	HPFSM_1_Type		hpfsm_1;		/* 12 */
	HPFSM_2_Type		hpfsm_2;
	HPFSM_3_Type		hpfsm_3;
	HPFSM_4_Type		hpfsm_4;
	SPI_EN_Type			spi_en;			/* 16 */
	SYN_FR_DIV_1_Type	syn_fr_div_1;
	SYN_FR_DIV_2_Type	syn_fr_div_2;
	SYN_INT_DIV_Type	syn_int_div;
	SYN_CFG_1_Type		syn_cfg_1;		/* 20 */
	SYN_CFG_2_Type		syn_cfg_2;
	VAS_CFG_Type		vas_cfg;
	LO_MISC_Type		lo_misc;
	XTAL_CFG_Type		xtal_cfg;		/* 24 */
	VCO_CFG_Type		vco_cfg;
	LO_GEN_Type			lo_gen;
	PA_DRV_PA_DAC_Type	pa_drv_pa_dac;
	PA_DAC_Type			pa_dac;			/* 28 */
	TX_GAIN_Type		tx_gain;
	TX_LO_IQ_Type		tx_lo_iq;
	TX_DC_CORR_Type		tx_dc_corr;
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

constexpr RegisterMap initial_register_values { Register_Type {
	/* Best effort to reconcile default values specified in three
	 * different places in the MAX2837 documentation.
	 */
	.rxrf_1 = { /* 0 */
		.LNA_EN		= 0,
		.Mixer_EN	= 0,
		.RxLO_EN	= 0,
		.Lbias		= 0b10,
		.Mbias		= 0b10,
		.buf		= 0b10,
		.LNAband	= 0,
		.RESERVED0	= 0,
	},
	.rxrf_2 = {	/* 1 */
		.LNAtune	= 0,
		.LNAde_Q	= 1,
		.L			= 0b000,
		.iqerr_trim	= 0b00000,
		.RESERVED0	= 0,
	},
	.lpf_1 = {	/* 2 */
		.LPF_EN		= 0,
		.TxBB_EN	= 0,
		.ModeCtrl	= 0b01,
		.FT			= 0b1111,
		.dF			= 0b01,
		.RESERVED0	= 0,
	},
	.lpf_2 = { /* 3 */
		.PT_SPI		= 0b1001,
		.Bqd		= 0b011,
		.TxRPCM		= 0b011,
		.RESERVED0	= 0,
	},
	.lpf_3_vga_1 = { /* 4 */
		.RP				= 0b10,
		.TxBuff			= 0b10,
		.VGA_EN			= 0,
		.VGAMUX_enable	= 0,
		.BUFF_Curr		= 0b00,
		.BUFF_VCM		= 0b00,
		.RESERVED0		= 0,
	},
	.vga_2 = { /* 5 */
		.VGA			= 0b00000,
		.sel_In1_In2	= 0,
		.turbo15n20		= 0,
		.VGA_Curr		= 0b01,
		.fuse_arm		= 0,
		.RESERVED0		= 0,
	},
	.vga_3_rx_top = { /* 6 */
		.RESERVED0			= 0b000110,
		.RSSI_EN_SPIenables	= 0,
		.RSSI_MUX			= 0,
		.RSSI_MODE			= 0,
		.LPF_MODE_SEL		= 0,
		.RESERVED1			= 0,
	},
	.temp_sense = { /* 7 */
		.ts_adc				= 0b00000,
		.RESERVED0			= 0,
		.PLL_test_output	= 0,
		.VAS_test_output	= 0,
		.HPFSM_test_output	= 0,
		.LOGEN_trim_divider_test_output = 0,
		.RESERVED1			= 0,
	},
	.rx_top_rx_bias = { /* 8 */
		.LNAgain_SPI_EN		= 0,
		.VGAgain_SPI_EN		= 0,
		.EN_Bias_Trim		= 0,
		.BIAS_TRIM_SPI		= 0b10000,
		.BIAS_TRIM_CNTRL	= 0,
		.RX_IQERR_SPI_EN	= 0,
		.RESERVED0			= 0,
	},
	.rx_top = { /* 9 */
		.ts_adc_trigger	= 0,
		.ts_en			= 0,
		.LPFtrim_SPI_EN	= 0,
		.DOUT_DRVH		= 1,	/* Documentation mismatch */
		.DOUT_PU		= 1,
		.DOUT_SEL		= 0b000,
		.fuse_th		= 0,
		.fuse_burn_gkt	= 0,
		.RESERVED0		= 0,
	},
	.tx_top_1 = { /* 10 */
		.RESERVED0		= 0,
		.TXCAL_GAIN		= 0b00,
		.TXCAL_V2I_FILT = 0b011,
		.TX_BIAS_ADJ	= 0b01,
		.RESERVED1		= 0b00,
		.RESERVED2		= 0,
	},
	.tx_top_2 = { /* 11 */
		.AMD_SPI_EN		= 0,
		.TXMXR_V2I_GAIN	= 0b1011,
		.RESERVED0		= 0b00000,
		.RESERVED1		= 0,
	},
	.hpfsm_1 = { /* 12 */
		.HPC_10M		= 0b11,
		.HPC_10M_GAIN	= 0b11,
		.HPC_600k		= 0b100,
		.HPC_600k_GAIN	= 0b100,
		.RESERVED0		= 0,
	},
	.hpfsm_2 = { /* 13 */
		.HPC_100k		= 0b00,
		.HPC_100k_GAIN	= 0b00,
		.HPC_30k		= 0b01,
		.HPC_30k_GAIN	= 0b01,
		.HPC_1k			= 0b01,
		.RESERVED0		= 0,
	},
	.hpfsm_3 = { /* 14 */
		.HPC_1k_GAIN	= 0b01,
		.HPC_DELAY		= 0b01,
		.HPC_STOP		= 0b00,
		.HPC_STOP_M2	= 0b11,
		.HPC_RXGAIN_EN	= 1,
		.HPC_MODE		= 0,
		.RESERVED0		= 0,
	},
	.hpfsm_4 = { /* 15 */
		.HPC_DIVH		= 1,
		.HPC_TST		= 0b00000,
		.HPC_SEQ_BYP	= 0,
		.DOUT_CSB_SEL	= 1,	/* Documentation mismatch */
		.RESERVED0		= 0b00,
		.RESERVED1		= 0,
	},
	.spi_en = { /* 16 */
		.EN_SPI			= 0,
		.CAL_SPI		= 0,
		.LOGEN_SPI_EN	= 1,
		.SYN_SPI_EN		= 1,
		.VAS_SPI_EN		= 1,
		.PADRV_SPI_EN	= 0,
		.PADAC_SPI_EN	= 0,
		.PADAC_TX_EN	= 0,
		.TXMX_SPI_EN	= 0,
		.TXLO_SPI_EN	= 0,
		.RESERVED0		= 0,
	},
	.syn_fr_div_1 = { /* 17 */
		.SYN_FRDIV_9_0	= 0b0101010101,
		.RESERVED0		= 0,
	},
	.syn_fr_div_2 = { /* 18 */
		.SYN_FRDIV_19_10	= 0b0101010101,
		.RESERVED0			= 0,
	},
	.syn_int_div = { /* 19 */
		.SYN_INTDIV	= 0b01010011,
		.LOGEN_BSW	= 0b01,
		.RESERVED0	= 0,
	},
	.syn_cfg_1 = { /* 20 */
		.SYN_MODE_FR_EN		= 1,
		.SYN_REF_DIV_RATIO	= 0b00,
		.SYN_CP_CURRENT		= 0b00,
		.SYN_CLOCKOUT_DRIVE	= 0,
		.SYN_TURBO_EN		= 1,
		.CP_TRM_SET			= 0,
		.CP_TRM_CODE		= 0b10,
		.RESERVED0			= 0,
	},
	.syn_cfg_2 = { /* 21 */
		.SYN_CP_COMMON_MODE_EN		= 1,
		.SYN_PRESCALER_BIAS_BOOST	= 0,
		.SYN_CP_BETA_CURRENT_COMP_EN = 1,
		.SYN_SD_CLK_SEL				= 1,
		.SYN_CP_PW_ADJ				= 0,
		.SYN_CP_LIN_CURRENT_SEL		= 0b01,
		.SYN_TEST_OUT_SEL			= 0b000,
		.RESERVED0					= 0,
	},
	.vas_cfg = { /* 22 */
		.VAS_MODE		= 1,
		.VAS_RELOCK_SEL	= 0,
		.VAS_DIV		= 0b010,
		.VAS_DLY		= 0b01,
		.VAS_TRIG_EN	= 1,
		.VAS_ADE		= 1,
		.VAS_ADL_SPI	= 0,
		.RESERVED0		= 0,
	},
	.lo_misc = { /* 23 */
		.VAS_SPI			= 0b01111,
		.XTAL_BIAS_SEL		= 0b10,
		.XTAL_E2C_BIAS_SEL	= 0,
		.VAS_SE				= 0,
		.VCO_SPI_EN			= 1,
		.RESERVED0			= 0,
	},
	.xtal_cfg = { /* 24 */
		.XTAL_FTUNE			= 0b0000000,
		.XTAL_CLKOUT_EN		= 0,	/* 1->0 to "turn off" CLKOUT pin. Doesn't seem to work though... */
		.XTAL_CLKOUT_DIV	= 1,
		.XTAL_CORE_EN		= 0,
		.RESERVED0			= 0,
	},
	.vco_cfg = { /* 25 */
		.VCO_BIAS_SPI_EN	= 0,
		.VCO_BIAS_SPI		= 0b0000,
		.VCO_CMEN			= 0,
		.VCO_PDET_TST		= 0b00,
		.VCO_BUF_BIASH		= 0b01,
		.RESERVED0			= 0,
	},
	.lo_gen = { /* 26 */
		.LOGEN_BIASH1	= 0b10,
		.LOGEN_BIASH2	= 0,
		.LOGEN_2GM		= 1,
		.LOGEN_TRIM1	= 0,
		.LOGEN_TRIM2	= 0,
		.VAS_TST		= 0b1111,
		.RESERVED0		= 0,
	},
	.pa_drv_pa_dac = { /* 27 */
		.PADRV_BIAS			= 0b011,
		.PADRV_DOWN_SPI_EN	= 0,
		.PADRV_DOWN_SPI_SEL	= 0,	/* Documentation mismatch */
		.PADAC_IV			= 1,
		.PADAC_VMODE		= 1,
		.PADAC_DIVH			= 1,
		.TXGATE_EN			= 1,
		.TX_DCCORR_EN		= 1,
		.RESERVED0			= 0,
	},
	.pa_dac = { /* 28 */
		.PADAC_BIAS	= 0b000000,
		.PADAC_DLY	= 0b0011,
		.RESERVED0	= 0,
	},
	.tx_gain = { /* 29 */
		.TXVGA_GAIN_SPI_EN		= 0,
		.TXVGA_GAIN_MSB_SPI_EN	= 0,
		.TX_DCCORR_SPI_EN		= 0,
		.FUSE_ARM				= 0,
		.TXVGA_GAIN_SPI			= 0b111111,
		.RESERVED0				= 0,
	},
	.tx_lo_iq = { /* 30 */
		.TXLO_IQ_SPI	= 0b00000,
		.TXLO_IQ_SPI_EN	= 0,
		.TXLO_BUFF		= 0b10,
		.FUSE_GKT		= 0,
		.FUSE_RTH		= 0,
		.RESERVED0		= 0,
	},
	.tx_dc_corr = { /* 31 */
		.TX_DCCORR_I	= 0b00000,
		.TX_DCCORR_Q	= 0b00000,
		.RESERVED0		= 0,
	},
} };

class MAX2837 {
public:
	constexpr MAX2837(
		spi::arbiter::Target& target
	) : _target(target)
	{
	}

	void init();
	void set_mode(const Mode mode);

	void set_tx_vga_gain(const int_fast8_t db);
	void set_lna_gain(const int_fast8_t db);
	void set_vga_gain(const int_fast8_t db);
	void set_lpf_rf_bandwidth(const uint32_t bandwidth_minimum);
#if 0
	void rx_cal() {
		_map.r.spi_en.EN_SPI = 1;
		_map.r.spi_en.CAL_SPI = 1;
		flush_one(Register::SPI_EN);

		_map.r.vga_3_rx_top.LPF_MODE_SEL = 1;
		flush_one(Register::VGA_3_RX_TOP);

		_map.r.lpf_1.ModeCtrl = 0b00;
		flush_one(Register::LPF_1);

		_map.r.lo_gen.LOGEN_2GM = 1;
		flush_one(Register::LO_GEN);

		chThdSleepMilliseconds(100);

		_map.r.spi_en.CAL_SPI = 0;
		flush_one(Register::SPI_EN);

		_map.r.vga_3_rx_top.LPF_MODE_SEL = 0;
		flush_one(Register::VGA_3_RX_TOP);

		_map.r.lpf_1.ModeCtrl = 0b01;
		flush_one(Register::LPF_1);

		_map.r.lo_gen.LOGEN_2GM = 0;
		flush_one(Register::LO_GEN);
	}

	void test_rx_offset(const size_t n) {
		_map.r.hpfsm_4.HPC_TST = n;
		_dirty[Register::HPFSM_4] = 1;
		/*
		_map.r.hpfsm_3.HPC_STOP = n;
		_dirty[Register::HPFSM_3] = 1;
		*/
		flush();
	}
#endif

	bool set_frequency(const rf::Frequency lo_frequency);

	void set_rx_lo_iq_calibration(const size_t v);
	void set_rx_bias_trim(const size_t v);
	void set_vco_bias(const size_t v);
	void set_rx_buff_vcm(const size_t v);

	reg_t temp_sense();

	reg_t read(const address_t reg_num);

private:
	spi::arbiter::Target& _target;

	RegisterMap _map { initial_register_values };
	DirtyRegisters<Register, reg_count> _dirty { };

	void flush_one(const Register reg);

	void write(const address_t reg_num, const reg_t value);

	void write(const Register reg, const reg_t value);
	reg_t read(const Register reg);

	void flush();
};

}

#endif/*__MAX2837_H__*/
