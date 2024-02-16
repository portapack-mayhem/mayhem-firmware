/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2023 Great Scott Gadgets
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

#ifndef __MAX2839_H__
#define __MAX2839_H__

#include "max283x.hpp"
#include "gpio.hpp"
#include "spi_arbiter.hpp"

#include <cstdint>
#include <array>

#include "dirty_registers.hpp"
#include "utility.hpp"

namespace max2839 {

using namespace max283x;

constexpr size_t reg_count = 32;

enum class Register : address_t {
    RXENABLE = 0,
    RXRF_1 = 1,
    RXRF_2 = 2,
    RXRF_LPF = 3,
    LPF = 4,
    LPF_VGA_1 = 5,
    LPF_VGA_2 = 6,
    RSSI_VGA = 7,
    RX_TOP_1 = 8,
    RX_TOP_2 = 9,
    TX_TOP_1 = 10,
    TEMP_SENSE = 11,
    HPFSM_1 = 12,
    HPFSM_2 = 13,
    HPFSM_3 = 14,
    HPFSM_4 = 15,
    SPI_EN = 16,
    SYN_FR_DIV_1 = 17,
    SYN_FR_DIV_2 = 18,
    SYN_INT_DIV = 19,
    SYN_CFG_1 = 20,
    SYN_CFG_2 = 21,
    VAS_CFG = 22,
    LO_MISC = 23,
    XTAL_CFG = 24,
    VCO_CFG = 25,
    LO_GEN = 26,
    PA_DRV = 27,
    PA_DAC = 28,
    TX_GAIN = 29,
    TX_LO_IQ = 30,
    TX_DC_CORR = 31,
};

struct RXENABLE_Type {
    reg_t RESERVED0 : 10;
    reg_t RESERVED1 : 6;
};

static_assert(sizeof(RXENABLE_Type) == sizeof(reg_t), "RXENABLE_Type wrong size");

struct RXRF_1_Type {
    reg_t LNAband : 1;
    reg_t RESERVED0 : 1;
    reg_t MIMOmode : 1;
    reg_t iqerr_trim : 5;
    reg_t RESERVED1 : 6;
};

static_assert(sizeof(RXRF_1_Type) == sizeof(reg_t), "RXRF_1_Type wrong size");

struct RXRF_2_Type {
    reg_t LNAgain_SPI_EN : 1;
    reg_t RESERVED0 : 1;
    reg_t RX_IQERR_SPI_EN : 1;
    reg_t RESERVED1 : 7;
    reg_t RESERVED2 : 6;
};

static_assert(sizeof(RXRF_2_Type) == sizeof(reg_t), "RXRF_2_Type wrong size");

struct RXRF_LPF_Type {
    reg_t RESERVED0 : 10;
    reg_t RESERVED1 : 6;
};

static_assert(sizeof(RXRF_LPF_Type) == sizeof(reg_t), "RXRF_LPF_Type wrong size");

struct LPF_Type {
    reg_t RESERVED0 : 2;
    reg_t dF : 2;
    reg_t RESERVED1 : 2;
    reg_t FT : 4;
    reg_t RESERVED2 : 6;
};

static_assert(sizeof(LPF_Type) == sizeof(reg_t), "LPF_Type wrong size");

struct LPF_VGA_1_Type {
    reg_t L : 2;
    reg_t VGA : 6;
    reg_t ModeCtrl : 2;
    reg_t RESERVED0 : 6;
};

static_assert(sizeof(LPF_VGA_1_Type) == sizeof(reg_t), "LPF_VGA_1_Type wrong size");

struct LPF_VGA_2_Type {
    reg_t L : 2;
    reg_t VGA : 6;
    reg_t BUFF_VCM : 2;
    reg_t RESERVED0 : 6;
};

static_assert(sizeof(LPF_VGA_2_Type) == sizeof(reg_t), "LPF_VGA_2_Type wrong size");

struct RSSI_VGA_Type {
    reg_t RESERVED0 : 1;
    reg_t RSSI_MUX : 1;
    reg_t RSSI_MODE : 1;
    reg_t RESERVED1 : 4;
    reg_t RXBB_OUT_SEL : 1;
    reg_t RESERVED2 : 1;
    reg_t RSSI_INPUT : 1;
    reg_t RESERVED3 : 6;
};

static_assert(sizeof(RSSI_VGA_Type) == sizeof(reg_t), "RSSI_VGA_Type wrong size");

struct RX_TOP_1_Type {
    reg_t RESERVED0 : 1;
    reg_t VGAgain_SPI_EN : 1;
    reg_t LPF_MODE_SEL : 1;
    reg_t RESERVED1 : 7;
    reg_t RESERVED2 : 6;
};

static_assert(sizeof(RX_TOP_1_Type) == sizeof(reg_t), "RX_TOP_1_Type wrong size");

struct RX_TOP_2_Type {
    reg_t ts_adc_trigger : 1;
    reg_t ts_en : 1;
    reg_t RESERVED0 : 1;
    reg_t DOUT_DRVH : 1;
    reg_t DOUT_CSB_SEL : 1;
    reg_t DOUT_SEL : 3;
    reg_t RESERVED1 : 2;
    reg_t RESERVED2 : 6;
};

static_assert(sizeof(RX_TOP_2_Type) == sizeof(reg_t), "RX_TOP_2_Type wrong size");

struct TX_TOP_1_Type {
    reg_t TXCAL_GAIN : 2;
    reg_t TXCAL_V2I_FILT : 3;
    reg_t RESERVED1 : 5;
    reg_t RESERVED2 : 6;
};

static_assert(sizeof(TX_TOP_1_Type) == sizeof(reg_t), "TX_TOP_1_Type wrong size");

struct TEMP_SENSE_Type {
    reg_t RESERVED0 : 10;
    reg_t RESERVED1 : 6;
};

static_assert(sizeof(TEMP_SENSE_Type) == sizeof(reg_t), "TEMP_SENSE_Type wrong size");

struct HPFSM_1_Type {
    reg_t HPC_10M : 2;
    reg_t HPC_10M_GAIN : 2;
    reg_t HPC_600k : 3;
    reg_t HPC_600k_GAIN : 3;
    reg_t RESERVED0 : 6;
};

static_assert(sizeof(HPFSM_1_Type) == sizeof(reg_t), "HPFSM_1_Type wrong size");

struct HPFSM_2_Type {
    reg_t HPC_100k : 2;
    reg_t HPC_100k_GAIN : 2;
    reg_t HPC_30k : 2;
    reg_t HPC_30k_GAIN : 2;
    reg_t HPC_1k : 2;
    reg_t RESERVED0 : 6;
};

static_assert(sizeof(HPFSM_2_Type) == sizeof(reg_t), "HPFSM_2_Type wrong size");

struct HPFSM_3_Type {
    reg_t HPC_1k_B7B6 : 2;
    reg_t HPC_DELAY : 2;
    reg_t HPC_STOP : 2;
    reg_t HPC_STOP_M2 : 2;
    reg_t HPC_RXGAIN_EN : 1;
    reg_t TXGATE_EN : 1;
    reg_t RESERVED0 : 6;
};

static_assert(sizeof(HPFSM_3_Type) == sizeof(reg_t), "HPFSM_3_Type wrong size");

struct HPFSM_4_Type {
    reg_t HPC_DIVH : 1;
    reg_t RESERVED0 : 5;
    reg_t HPC_SEQ_BYP : 1;
    reg_t RESERVED1 : 2;
    reg_t HPC_MODE : 1;
    reg_t RESERVED2 : 6;
};

static_assert(sizeof(HPFSM_4_Type) == sizeof(reg_t), "HPFSM_4_Type wrong size");

struct SPI_EN_Type {
    reg_t EN_SPI : 1;
    reg_t CAL_SPI : 1;
    reg_t RESERVED0 : 4;
    reg_t PADAC_SPI_EN : 1;
    reg_t PADAC_TX_EN : 1;
    reg_t RESERVED1 : 2;
    reg_t RESERVED2 : 6;
};

static_assert(sizeof(SPI_EN_Type) == sizeof(reg_t), "SPI_EN_Type wrong size");

struct SYN_FR_DIV_1_Type {
    reg_t SYN_FRDIV_9_0 : 10;
    reg_t RESERVED0 : 6;
};

static_assert(sizeof(SYN_FR_DIV_1_Type) == sizeof(reg_t), "SYN_FR_DIV_1_Type wrong size");

struct SYN_FR_DIV_2_Type {
    reg_t SYN_FRDIV_19_10 : 10;
    reg_t RESERVED0 : 6;
};

static_assert(sizeof(SYN_FR_DIV_2_Type) == sizeof(reg_t), "SYN_FR_DIV_2_Type wrong size");

struct SYN_INT_DIV_Type {
    reg_t SYN_INTDIV : 8;
    reg_t LOGEN_BSW : 2;
    reg_t RESERVED0 : 6;
};

static_assert(sizeof(SYN_INT_DIV_Type) == sizeof(reg_t), "SYN_INT_DIV_Type wrong size");

struct SYN_CFG_1_Type {
    reg_t RESERVED0 : 1;
    reg_t SYN_REF_DIV_RATIO : 2;
    reg_t RESERVED1 : 2;
    reg_t SYN_CLOCKOUT_DRIVE : 1;
    reg_t RESERVED2 : 4;
    reg_t RESERVED3 : 6;
};

static_assert(sizeof(SYN_CFG_1_Type) == sizeof(reg_t), "SYN_CFG_1_Type wrong size");

struct SYN_CFG_2_Type {
    reg_t RESERVED0 : 10;
    reg_t RESERVED1 : 6;
};

static_assert(sizeof(SYN_CFG_2_Type) == sizeof(reg_t), "SYN_CFG_2_Type wrong size");

struct VAS_CFG_Type {
    reg_t VAS_MODE : 1;
    reg_t VAS_RELOCK_SEL : 1;
    reg_t VAS_DIV : 3;
    reg_t VAS_DLY : 2;
    reg_t VAS_TRIG_EN : 1;
    reg_t RESERVED0 : 2;
    reg_t RESERVED1 : 6;
};

static_assert(sizeof(VAS_CFG_Type) == sizeof(reg_t), "VAS_CFG_Type wrong size");

struct LO_MISC_Type {
    reg_t VAS_SPI : 5;
    reg_t XTAL_BIAS_SEL : 2;
    reg_t RESERVED0 : 3;
    reg_t RESERVED1 : 6;
};

static_assert(sizeof(LO_MISC_Type) == sizeof(reg_t), "LO_MISC_Type wrong size");

struct XTAL_CFG_Type {
    reg_t XTAL_FTUNE : 7;
    reg_t RESERVED0 : 1;
    reg_t XTAL_CLKOUT_DIV : 1;
    reg_t XTAL_CORE_EN : 1;
    reg_t RESERVED1 : 6;
};

static_assert(sizeof(XTAL_CFG_Type) == sizeof(reg_t), "XTAL_CFG_Type wrong size");

struct VCO_CFG_Type {
    reg_t RESERVED0 : 10;
    reg_t RESERVED1 : 6;
};

static_assert(sizeof(VCO_CFG_Type) == sizeof(reg_t), "VCO_CFG_Type wrong size");

struct LO_GEN_Type {
    reg_t RESERVED0 : 3;
    reg_t LOGEN_2GM : 1;
    reg_t RESERVED1 : 2;
    reg_t VAS_TST : 4;
    reg_t RESERVED2 : 6;
};

static_assert(sizeof(LO_GEN_Type) == sizeof(reg_t), "LO_GEN_Type wrong size");

struct PA_DRV_Type {
    reg_t TXLO_IQ_SPI : 6;
    reg_t TXLO_IQ_SPI_EN : 1;
    reg_t TXVGA_GAIN_SPI_EN : 1;
    reg_t TX_DCCORR_SPI_EN : 1;
    reg_t RESERVED0 : 1;
    reg_t RESERVED1 : 6;
};

static_assert(sizeof(PA_DRV_Type) == sizeof(reg_t), "PA_DRV_Type wrong size");

struct PA_DAC_Type {
    reg_t PADAC_BIAS : 6;
    reg_t PADAC_DLY : 4;
    reg_t RESERVED0 : 6;
};

static_assert(sizeof(PA_DAC_Type) == sizeof(reg_t), "PA_DAC_Type wrong size");

struct TX_GAIN_Type {
    reg_t TXVGA_GAIN_SPI : 6;
    reg_t RESERVED0 : 4;
    reg_t RESERVED1 : 6;
};

static_assert(sizeof(TX_GAIN_Type) == sizeof(reg_t), "TX_GAIN_Type wrong size");

struct TX_LO_IQ_Type {
    reg_t TX_DCCORR_I : 6;
    reg_t RESERVED0 : 2;
    reg_t PADAC_IV : 1;
    reg_t PADAC_VMODE : 1;
    reg_t RESERVED1 : 6;
};

static_assert(sizeof(TX_LO_IQ_Type) == sizeof(reg_t), "TX_LO_IQ_Type wrong size");

struct TX_DC_CORR_Type {
    reg_t TX_DCCORR_Q : 6;
    reg_t RESERVED0 : 3;
    reg_t PADAC_DIVH : 1;
    reg_t RESERVED1 : 6;
};

static_assert(sizeof(TX_DC_CORR_Type) == sizeof(reg_t), "TX_DC_CORR_Type wrong size");

struct Register_Type {
    RXENABLE_Type rxenable; /*  0 */
    RXRF_1_Type rxrf_1;
    RXRF_2_Type rxrf_2;
    RXRF_LPF_Type rxrf_lpf_1;
    LPF_Type lpf; /*  4 */
    LPF_VGA_1_Type lpf_vga_1;
    LPF_VGA_2_Type lpf_vga_2;
    RSSI_VGA_Type rssi_vga;
    RX_TOP_1_Type rx_top_1; /*  8 */
    RX_TOP_2_Type rx_top_2;
    TX_TOP_1_Type tx_top_1;
    TEMP_SENSE_Type temp_sense;
    HPFSM_1_Type hpfsm_1; /* 12 */
    HPFSM_2_Type hpfsm_2;
    HPFSM_3_Type hpfsm_3;
    HPFSM_4_Type hpfsm_4;
    SPI_EN_Type spi_en; /* 16 */
    SYN_FR_DIV_1_Type syn_fr_div_1;
    SYN_FR_DIV_2_Type syn_fr_div_2;
    SYN_INT_DIV_Type syn_int_div;
    SYN_CFG_1_Type syn_cfg_1; /* 20 */
    SYN_CFG_2_Type syn_cfg_2;
    VAS_CFG_Type vas_cfg;
    LO_MISC_Type lo_misc;
    XTAL_CFG_Type xtal_cfg; /* 24 */
    VCO_CFG_Type vco_cfg;
    LO_GEN_Type lo_gen;
    PA_DRV_Type pa_drv;
    PA_DAC_Type pa_dac; /* 28 */
    TX_GAIN_Type tx_gain;
    TX_LO_IQ_Type tx_lo_iq;
    TX_DC_CORR_Type tx_dc_corr;
};

static_assert(sizeof(Register_Type) == reg_count * sizeof(reg_t), "Register_Type wrong size");

struct RegisterMap {
    constexpr RegisterMap(
        Register_Type values)
        : r(values) {
    }

    union {
        Register_Type r;
        std::array<reg_t, reg_count> w;
    };
};

static_assert(sizeof(RegisterMap) == reg_count * sizeof(reg_t), "RegisterMap type wrong size");

constexpr RegisterMap initial_register_values{Register_Type{
    /* settings recommended by MAX2839 data sheet */
    .rxenable = {
        /* 0 */
        .RESERVED0 = 0,
        .RESERVED1 = 0,
    },
    .rxrf_1 = {
        /* 1 */
        .LNAband = 0,
        .RESERVED0 = 0,
        .MIMOmode = 1,
        .iqerr_trim = 0b000001,
        .RESERVED1 = 0,
    },
    .rxrf_2 = {
        /* 2 */
        .LNAgain_SPI_EN = 0,
        .RESERVED0 = 0,
        .RX_IQERR_SPI_EN = 0,
        .RESERVED1 = 0b0010000,
        .RESERVED2 = 0,
    },
    .rxrf_lpf_1 = {
        /* 3 */
        .RESERVED0 = 0b0110111001,
        .RESERVED1 = 0,
    },
    .lpf = {
        /* 4 */
        .RESERVED0 = 0b10,
        .dF = 0b01,
        .RESERVED1 = 0b10,
        .FT = 0b1111,
        .RESERVED2 = 0,
    },
    .lpf_vga_1 = {
        /* 5 */
        .L = 0b00,
        .VGA = 0b000000,
        .ModeCtrl = 0b01,
        .RESERVED0 = 0,
    },
    .lpf_vga_2 = {
        /* 6 */
        .L = 0b00,
        .VGA = 0b000000,
        .BUFF_VCM = 0b00,
        .RESERVED0 = 0,
    },
    .rssi_vga = {
        /* 7 */
        .RESERVED0 = 0,
        .RSSI_MUX = 0,
        .RSSI_MODE = 0,
        .RESERVED1 = 0b0001,
        .RXBB_OUT_SEL = 0,
        .RESERVED2 = 0,
        .RSSI_INPUT = 1,
        .RESERVED3 = 0,
    },
    .rx_top_1 = {
        /* 8 */
        .RESERVED0 = 0,
        .VGAgain_SPI_EN = 0,
        .LPF_MODE_SEL = 0,
        .RESERVED1 = 0b1000100,
        .RESERVED2 = 0,
    },
    .rx_top_2 = {
        /* 9 */
        .ts_adc_trigger = 0,
        .ts_en = 0,
        .RESERVED0 = 0,
        .DOUT_DRVH = 1,
        .DOUT_CSB_SEL = 1,
        .DOUT_SEL = 0b000,
        .RESERVED1 = 0b00,
        .RESERVED2 = 0,
    },
    .tx_top_1 = {
        /* 10 */
        .TXCAL_GAIN = 0b00,
        .TXCAL_V2I_FILT = 0b011,
        .RESERVED1 = 0b00000,
        .RESERVED2 = 0,
    },
    .temp_sense = {
        /* 11 */
        .RESERVED0 = 0b0000000100,
        .RESERVED1 = 0,
    },
    .hpfsm_1 = {
        /* 12 */
        .HPC_10M = 0b11,
        .HPC_10M_GAIN = 0b11,
        .HPC_600k = 0b100,
        .HPC_600k_GAIN = 0b100,
        .RESERVED0 = 0,
    },
    .hpfsm_2 = {
        /* 13 */
        .HPC_100k = 0b00,
        .HPC_100k_GAIN = 0b00,
        .HPC_30k = 0b01,
        .HPC_30k_GAIN = 0b01,
        .HPC_1k = 0b01,
        .RESERVED0 = 0,
    },
    .hpfsm_3 = {
        /* 14 */
        .HPC_1k_B7B6 = 0b01,
        .HPC_DELAY = 0b01,
        .HPC_STOP = 0b00,
        .HPC_STOP_M2 = 0b11,
        .HPC_RXGAIN_EN = 1,
        .TXGATE_EN = 1,
        .RESERVED0 = 0,
    },
    .hpfsm_4 = {
        /* 15 */
        .HPC_DIVH = 1,
        .RESERVED0 = 0b00000,
        .HPC_SEQ_BYP = 0,
        .RESERVED1 = 0b00,
        .HPC_MODE = 1,
        .RESERVED2 = 0,
    },
    .spi_en = {
        /* 16 */
        .EN_SPI = 0,
        .CAL_SPI = 0,
        .RESERVED0 = 0b0111,
        .PADAC_SPI_EN = 0,
        .PADAC_TX_EN = 0,
        .RESERVED1 = 0b00,
        .RESERVED2 = 0,
    },
    .syn_fr_div_1 = {
        /* 17 */
        .SYN_FRDIV_9_0 = 0b0101010101,
        .RESERVED0 = 0,
    },
    .syn_fr_div_2 = {
        /* 18 */
        .SYN_FRDIV_19_10 = 0b0101010101,
        .RESERVED0 = 0,
    },
    .syn_int_div = {
        /* 19 */
        .SYN_INTDIV = 0b01010011,
        .LOGEN_BSW = 0b01,
        .RESERVED0 = 0,
    },
    .syn_cfg_1 = {
        /* 20 */
        .RESERVED0 = 1,
        .SYN_REF_DIV_RATIO = 0b00,
        .RESERVED1 = 0b01,
        .SYN_CLOCKOUT_DRIVE = 0,
        .RESERVED2 = 0b1001,
        .RESERVED3 = 0,
    },
    .syn_cfg_2 = {
        /* 21 */
        .RESERVED0 = 0b0000101101,
        .RESERVED1 = 0,
    },
    .vas_cfg = {
        /* 22 */
        .VAS_MODE = 1,
        .VAS_RELOCK_SEL = 0,
        .VAS_DIV = 0b010,
        .VAS_DLY = 0b01,
        .VAS_TRIG_EN = 1,
        .RESERVED0 = 0b01,
        .RESERVED1 = 0,
    },
    .lo_misc = {
        /* 23 */
        .VAS_SPI = 0b01111,
        .XTAL_BIAS_SEL = 0b10,
        .RESERVED0 = 0b100,
        .RESERVED1 = 0,
    },
    .xtal_cfg = {
        /* 24 */
        .XTAL_FTUNE = 0b0000000,
        .RESERVED0 = 1,
        .XTAL_CLKOUT_DIV = 1,
        .XTAL_CORE_EN = 0,
        .RESERVED1 = 0,
    },
    .vco_cfg = {
        /* 25 */
        .RESERVED0 = 0b0000000000,
        .RESERVED1 = 0,
    },
    .lo_gen = {
        /* 26 */
        .RESERVED0 = 0b000,
        .LOGEN_2GM = 0,
        .RESERVED1 = 0b00,
        .VAS_TST = 0b1111,
        .RESERVED2 = 0,
    },
    .pa_drv = {
        /* 27 */
        .TXLO_IQ_SPI = 0b000000,
        .TXLO_IQ_SPI_EN = 0,
        .TXVGA_GAIN_SPI_EN = 0,
        .TX_DCCORR_SPI_EN = 0,
        .RESERVED0 = 1,
        .RESERVED1 = 0,
    },
    .pa_dac = {
        /* 28 */
        .PADAC_BIAS = 0b000000,
        .PADAC_DLY = 0b0011,
        .RESERVED0 = 0,
    },
    .tx_gain = {
        /* 29 */
        .TXVGA_GAIN_SPI = 0b111111,
        .RESERVED0 = 0b0000,
        .RESERVED1 = 0,
    },
    .tx_lo_iq = {
        /* 30 */
        .TX_DCCORR_I = 0b000000,
        .RESERVED0 = 0b00,
        .PADAC_IV = 1,
        .PADAC_VMODE = 1,
        .RESERVED1 = 0,
    },
    .tx_dc_corr = {
        /* 31 */
        .TX_DCCORR_Q = 0b000000,
        .RESERVED0 = 0b101,
        .PADAC_DIVH = 1,
        .RESERVED1 = 0,
    },
}};

class MAX2839 : public MAX283x {
   public:
    constexpr MAX2839(
        spi::arbiter::Target& target)
        : _target(target) {
    }

    void init() override;
    void set_mode(const Mode mode) override;

    void set_tx_vga_gain(const int_fast8_t db) override;
    void set_lna_gain(const int_fast8_t db) override;
    void set_vga_gain(const int_fast8_t db) override;
    void set_lpf_rf_bandwidth_rx(const uint32_t bandwidth_minimum) override;
    void set_lpf_rf_bandwidth_tx(const uint32_t bandwidth_minimum) override;
    bool set_frequency(const rf::Frequency lo_frequency) override;
    void set_rx_lo_iq_calibration(const size_t v) override;
    void set_tx_LO_iq_phase_calibration(const size_t v) override;
    void set_rx_buff_vcm(const size_t v) override;

    int8_t temp_sense() override;

    reg_t read(const address_t reg_num) override;
    void write(const address_t reg_num, const reg_t value) override;

   private:
    spi::arbiter::Target& _target;

    RegisterMap _map{initial_register_values};
    DirtyRegisters<Register, reg_count> _dirty{};

    void flush_one(const Register reg);

    void write(const Register reg, const reg_t value);
    reg_t read(const Register reg);

    void flush();

    void configure_rx_gain();
};

}  // namespace max2839

#endif /*__MAX2839_H__*/