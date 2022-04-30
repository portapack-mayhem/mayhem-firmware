/*
 * Copyright (C) 2017 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __AK4951_H__
#define __AK4951_H__

#include <cstdint>
#include <array>

#include "utility.hpp"

#include "i2c_pp.hpp"

#include "audio.hpp"

namespace asahi_kasei {
namespace ak4951 {

using address_t = uint8_t;
using reg_t = uint8_t;

constexpr size_t reg_count = 0x50;

enum class Register : address_t {
	PowerManagement1 = 0x00,
	PowerManagement2 = 0x01,
	SignalSelect1 = 0x02,
	SignalSelect2 = 0x03,
	SignalSelect3 = 0x04,
	ModeControl1 = 0x05,
	ModeControl2 = 0x06,
	ModeControl3 = 0x07,
	DigitalMic = 0x08,
	TimerSelect = 0x09,
	ALCTimerSelect = 0x0a,
	ALCModeControl1 = 0x0b,
	ALCModeControl2 = 0x0c,
	LchInputVolumeControl = 0x0d,
	RchInputVolumeControl = 0x0e,
	ALCVolume = 0x0f,
	_Reserved_0x10 = 0x10,
	RchMicGainSetting = 0x11,
	BeepControl = 0x12,
	LchDigitalVolumeControl = 0x13,
	RchDigitalVolumeControl = 0x14,
	EQCommonGainSelect = 0x15,
	EQ2CommonGainSetting = 0x16,
	EQ3CommonGainSetting = 0x17,
	EQ4CommonGainSetting = 0x18,
	EQ5CommonGainSetting = 0x19,
	AutoHPFControl = 0x1a,
	DigitalFilterSelect1 = 0x1b,
	DigitalFilterSelect2 = 0x1c,
	DigitalFilterMode = 0x1d,
	HPF2Coefficient0 = 0x1e,
	HPF2Coefficient1 = 0x1f,
	HPF2Coefficient2 = 0x20,
	HPF2Coefficient3 = 0x21,
	LPFCoefficient0 = 0x22,
	LPFCoefficient1 = 0x23,
	LPFCoefficient2 = 0x24,
	LPFCoefficient3 = 0x25,
	FIL3Coefficient0 = 0x26,
	FIL3Coefficient1 = 0x27,
	FIL3Coefficient2 = 0x28,
	FIL3Coefficient3 = 0x29,
	EQCoefficient0 = 0x2a,
	EQCoefficient1 = 0x2b,
	EQCoefficient2 = 0x2c,
	EQCoefficient3 = 0x2d,
	EQCoefficient4 = 0x2e,
	EQCoefficient5 = 0x2f,
	DigitalFilterSelect3 = 0x30,
	DeviceInformation = 0x31,
	E1Coefficient0 = 0x32,
	E1Coefficient1 = 0x33,
	E1Coefficient2 = 0x34,
	E1Coefficient3 = 0x35,
	E1Coefficient4 = 0x36,
	E1Coefficient5 = 0x37,
	E2Coefficient0 = 0x38,
	E2Coefficient1 = 0x39,
	E2Coefficient2 = 0x3a,
	E2Coefficient3 = 0x3b,
	E2Coefficient4 = 0x3c,
	E2Coefficient5 = 0x3d,
	E3Coefficient0 = 0x3e,
	E3Coefficient1 = 0x3f,
	E3Coefficient2 = 0x40,
	E3Coefficient3 = 0x41,
	E3Coefficient4 = 0x42,
	E3Coefficient5 = 0x43,
	E4Coefficient0 = 0x44,
	E4Coefficient1 = 0x45,
	E4Coefficient2 = 0x46,
	E4Coefficient3 = 0x47,
	E4Coefficient4 = 0x48,
	E4Coefficient5 = 0x49,
	E5Coefficient0 = 0x4a,
	E5Coefficient1 = 0x4b,
	E5Coefficient2 = 0x4c,
	E5Coefficient3 = 0x4d,
	E5Coefficient4 = 0x4e,
	E5Coefficient5 = 0x4f,
	_count,
};

static_assert(toUType(Register::_count) == reg_count, "Register::_count != reg_count");

struct PowerManagement1 {
	reg_t PMADL     : 1;
	reg_t PMADR     : 1;
	reg_t PMDAC     : 1;
	reg_t reserved0 : 2;
	reg_t PMBP      : 1;
	reg_t PMVCM     : 1;
	reg_t PMPFIL    : 1;
};

static_assert(sizeof(PowerManagement1) == sizeof(reg_t), "wrong size `struct");

struct PowerManagement2 {
	reg_t LOSEL     : 1;
	reg_t PMSL      : 1;
	reg_t PMPLL     : 1;
	reg_t MS        : 1;
	reg_t PMHPL     : 1;
	reg_t PMHPR     : 1;
	reg_t reserved0 : 1;
	reg_t PMOSC     : 1;
};

static_assert(sizeof(PowerManagement2) == sizeof(reg_t), "wrong size struct");

struct SignalSelect1 {
	reg_t MGAIN20   : 3;
	reg_t PMMP      : 1;
	reg_t MPSEL     : 1;
	reg_t DACS      : 1;
	reg_t MGAIN3    : 1;
	reg_t SLPSN     : 1;
};

static_assert(sizeof(SignalSelect1) == sizeof(reg_t), "wrong size struct");

struct SignalSelect2 {
	reg_t INR       : 2;
	reg_t INL       : 2;
	reg_t MICL      : 1;
	reg_t reserved0 : 1;
	reg_t SPKG      : 2;
};

static_assert(sizeof(SignalSelect2) == sizeof(reg_t), "wrong size struct");

struct SignalSelect3 {
	reg_t MONO      : 2;
	reg_t PTS       : 2;
	reg_t reserved0 : 1;
	reg_t DACL      : 1;
	reg_t LVCM      : 2;
};

static_assert(sizeof(SignalSelect3) == sizeof(reg_t), "wrong size struct");

struct ModeControl1 {
	reg_t DIF       : 2;
	reg_t CKOFF     : 1;
	reg_t BCKO      : 1;
	reg_t PLL       : 4;
};

static_assert(sizeof(ModeControl1) == sizeof(reg_t), "wrong size struct");

struct ModeControl2 {
	reg_t FS        : 4;
	reg_t reserved0 : 2;
	reg_t CM        : 2;
};

static_assert(sizeof(ModeControl2) == sizeof(reg_t), "wrong size struct");

struct ModeControl3 {
	reg_t reserved0 : 2;
	reg_t IVOLC     : 1;
	reg_t reserved1 : 1;
	reg_t DVOLC     : 1;
	reg_t SMUTE     : 1;
	reg_t THDET     : 1;
	reg_t TSDSEL    : 1;
};

static_assert(sizeof(ModeControl3) == sizeof(reg_t), "wrong size struct");

struct DigitalMIC {
	reg_t DMIC      : 1;
	reg_t DCLKP     : 1;
	reg_t reserved0 : 1;
	reg_t DCLKE     : 1;
	reg_t PMDML     : 1;
	reg_t PMDMR     : 1;
	reg_t reserved1 : 1;
	reg_t READ      : 1;
};

static_assert(sizeof(DigitalMIC) == sizeof(reg_t), "wrong size struct");

struct TimerSelect {
	reg_t DVTM      : 1;
	reg_t MOFF      : 1;
	reg_t reserved0 : 2;
	reg_t FRN       : 1;
	reg_t FRATT     : 1;
	reg_t ADRST     : 2;
};

static_assert(sizeof(TimerSelect) == sizeof(reg_t), "wrong size struct");

struct ALCTimerSelect {
	reg_t RFST      : 2;
	reg_t WTM       : 2;
	reg_t EQFC      : 2;
	reg_t IVTM      : 1;
	reg_t reserved0 : 1;
};

static_assert(sizeof(ALCTimerSelect) == sizeof(reg_t), "wrong size struct");

struct ALCModeControl1 {
	reg_t LMTH10    : 2;
	reg_t RGAIN     : 3;
	reg_t ALC       : 1;
	reg_t LMTH2     : 1;
	reg_t ALCEQN    : 1;
};

static_assert(sizeof(ALCModeControl1) == sizeof(reg_t), "wrong size struct");

struct ALCModeControl2 {
	reg_t REF       : 8;
};

static_assert(sizeof(ALCModeControl2) == sizeof(reg_t), "wrong size struct");

struct InputVolumeControl {
	reg_t IV        : 8;
};

static_assert(sizeof(InputVolumeControl) == sizeof(reg_t), "wrong size struct");

using LchInputVolumeControl = InputVolumeControl;
using RchInputVolumeControl = InputVolumeControl;

struct ALCVolume {
	reg_t VOL       : 8;
};

static_assert(sizeof(ALCVolume) == sizeof(reg_t), "wrong size struct");

struct RchMICGainSetting {
	reg_t MGR       : 8;
};

static_assert(sizeof(RchMICGainSetting) == sizeof(reg_t), "wrong size struct");

struct BeepControl {
	reg_t BPLVL     : 4;
	reg_t BEEPH     : 1;
	reg_t BEEPS     : 1;
	reg_t BPVCM     : 1;
	reg_t HPZ       : 1;
};

static_assert(sizeof(BeepControl) == sizeof(reg_t), "wrong size struct");

struct DigitalVolumeControl {
	reg_t DV        : 8;
};

static_assert(sizeof(DigitalVolumeControl) == sizeof(reg_t), "wrong size struct");

using LchDigitalVolumeControl = DigitalVolumeControl;
using RchDigitalVolumeControl = DigitalVolumeControl;

struct EQCommonGainSelect {
	reg_t reserved0 : 1;
	reg_t EQC2      : 1;
	reg_t EQC3      : 1;
	reg_t EQC4      : 1;
	reg_t EQC5      : 1;
	reg_t reserved1 : 3;
};

static_assert(sizeof(EQCommonGainSelect) == sizeof(reg_t), "wrong size struct");

struct EQCommonGainSetting {
	reg_t EQnT      : 2;
	reg_t EQnG      : 6;
};

static_assert(sizeof(EQCommonGainSetting) == sizeof(reg_t), "wrong size struct");

using EQ2CommonGainSetting = EQCommonGainSetting;
using EQ3CommonGainSetting = EQCommonGainSetting;
using EQ4CommonGainSetting = EQCommonGainSetting;
using EQ5CommonGainSetting = EQCommonGainSetting;

struct AutoHPFControl {
	reg_t STG       : 2;
	reg_t SENC      : 3;
	reg_t AHPF      : 1;
	reg_t reserved0 : 2;
};

static_assert(sizeof(AutoHPFControl) == sizeof(reg_t), "wrong size struct");

struct DigitalFilterSelect1 {
	reg_t HPFAD     : 1;
	reg_t HPFC      : 2;
	reg_t reserved0 : 5;
};

static_assert(sizeof(DigitalFilterSelect1) == sizeof(reg_t), "wrong size struct");

struct DigitalFilterSelect2 {
	reg_t HPF       : 1;
	reg_t LPF       : 1;
	reg_t reserved0 : 2;
	reg_t FIL3      : 1;
	reg_t EQ0       : 1;
	reg_t GN        : 2;
};

static_assert(sizeof(DigitalFilterSelect2) == sizeof(reg_t), "wrong size struct");

struct DigitalFilterMode {
	reg_t PFSDO     : 1;
	reg_t ADCPF     : 1;
	reg_t PFDAC     : 2;
	reg_t PFVOL     : 2;
	reg_t reserved0 : 2;
};

static_assert(sizeof(DigitalFilterMode) == sizeof(reg_t), "wrong size struct");

struct Coefficient14L {
	reg_t l         : 8;
};

struct Coefficient14H {
	reg_t h         : 6;
	reg_t reserved0 : 2;
};

static_assert(sizeof(Coefficient14L) == sizeof(reg_t), "wrong size struct");
static_assert(sizeof(Coefficient14H) == sizeof(reg_t), "wrong size struct");

using Coefficient16L = Coefficient14L;

struct Coefficient16H {
	reg_t h         : 8;
};

static_assert(sizeof(Coefficient16H) == sizeof(reg_t), "wrong size struct");

using HPF2Coefficient0 = Coefficient14L;
using HPF2Coefficient1 = Coefficient14H;
using HPF2Coefficient2 = Coefficient14L;
using HPF2Coefficient3 = Coefficient14H;

using LPFCoefficient0 = Coefficient14L;
using LPFCoefficient1 = Coefficient14H;
using LPFCoefficient2 = Coefficient14L;
using LPFCoefficient3 = Coefficient14H;

using FIL3Coefficient0 = Coefficient14L;

struct FIL3Coefficient1 {
	reg_t h : 6;
	reg_t reserved0 : 1;
	reg_t s : 1;
};

static_assert(sizeof(FIL3Coefficient1) == sizeof(reg_t), "wrong size struct");

using FIL3Coefficient2 = Coefficient14L;
using FIL3Coefficient3 = Coefficient14H;

using EQCoefficient0 = Coefficient16L;
using EQCoefficient1 = Coefficient16H;
using EQCoefficient2 = Coefficient14L;
using EQCoefficient3 = Coefficient14H;
using EQCoefficient4 = Coefficient16L;
using EQCoefficient5 = Coefficient16H;

struct DigitalFilterSelect3 {
	reg_t EQ1       : 1;
	reg_t EQ2       : 1;
	reg_t EQ3       : 1;
	reg_t EQ4       : 1;
	reg_t EQ5       : 1;
	reg_t reserved0 : 3;
};

static_assert(sizeof(DigitalFilterSelect3) == sizeof(reg_t), "wrong size struct");

struct DeviceInformation {
	reg_t DVN       : 4;
	reg_t REV       : 4;
};

static_assert(sizeof(DeviceInformation) == sizeof(reg_t), "wrong size struct");

using E1Coefficient0 = Coefficient16L;
using E1Coefficient1 = Coefficient16H;
using E1Coefficient2 = Coefficient16L;
using E1Coefficient3 = Coefficient16H;
using E1Coefficient4 = Coefficient16L;
using E1Coefficient5 = Coefficient16H;

using E2Coefficient0 = Coefficient16L;
using E2Coefficient1 = Coefficient16H;
using E2Coefficient2 = Coefficient16L;
using E2Coefficient3 = Coefficient16H;
using E2Coefficient4 = Coefficient16L;
using E2Coefficient5 = Coefficient16H;

using E3Coefficient0 = Coefficient16L;
using E3Coefficient1 = Coefficient16H;
using E3Coefficient2 = Coefficient16L;
using E3Coefficient3 = Coefficient16H;
using E3Coefficient4 = Coefficient16L;
using E3Coefficient5 = Coefficient16H;

using E4Coefficient0 = Coefficient16L;
using E4Coefficient1 = Coefficient16H;
using E4Coefficient2 = Coefficient16L;
using E4Coefficient3 = Coefficient16H;
using E4Coefficient4 = Coefficient16L;
using E4Coefficient5 = Coefficient16H;

using E5Coefficient0 = Coefficient16L;
using E5Coefficient1 = Coefficient16H;
using E5Coefficient2 = Coefficient16L;
using E5Coefficient3 = Coefficient16H;
using E5Coefficient4 = Coefficient16L;
using E5Coefficient5 = Coefficient16H;

struct Register_Type {
	PowerManagement1		power_management_1;
	PowerManagement2		power_management_2;
	SignalSelect1			signal_select_1;
	SignalSelect2			signal_select_2;
	SignalSelect3			signal_select_3;
	ModeControl1			mode_control_1;
	ModeControl2			mode_control_2;
	ModeControl3			mode_control_3;
	DigitalMIC				digital_mic;
	TimerSelect				timer_select;
	ALCTimerSelect			alc_timer_select;
	ALCModeControl1			alc_mode_control_1;
	ALCModeControl2			alc_mode_control_2;
	LchInputVolumeControl	l_ch_input_volume_control;
	RchInputVolumeControl	r_ch_input_volume_control;
	ALCVolume				alc_volume;
	reg_t					_reserved_0x10;
	RchMICGainSetting		r_ch_mic_gain_setting;
	BeepControl				beep_control;
	LchDigitalVolumeControl	l_ch_digital_volume_control;
	RchDigitalVolumeControl	r_ch_digital_volume_control;
	EQCommonGainSelect		eq_common_gain_select;
	EQ2CommonGainSetting	eq2_common_gain_setting;
	EQ3CommonGainSetting	eq3_common_gain_setting;
	EQ4CommonGainSetting	eq4_common_gain_setting;
	EQ5CommonGainSetting	eq5_common_gain_setting;
	AutoHPFControl			auto_hpf_control;
	DigitalFilterSelect1	digital_filter_select_1;
	DigitalFilterSelect2	digital_filter_select_2;
	DigitalFilterMode		digital_filter_mode;
	HPF2Coefficient0		hpf_2_coefficient_0;
	HPF2Coefficient1		hpf_2_coefficient_1;
	HPF2Coefficient2		hpf_2_coefficient_2;
	HPF2Coefficient3		hpf_2_coefficient_3;
	LPFCoefficient0			lpf_coefficient_0;
	LPFCoefficient1			lpf_coefficient_1;
	LPFCoefficient2			lpf_coefficient_2;
	LPFCoefficient3			lpf_coefficient_3;
	FIL3Coefficient0		fil_3_coefficient_0;
	FIL3Coefficient1		fil_3_coefficient_1;
	FIL3Coefficient2		fil_3_coefficient_2;
	FIL3Coefficient3		fil_3_coefficient_3;
	EQCoefficient0			eq_coefficient_0;
	EQCoefficient1			eq_coefficient_1;
	EQCoefficient2			eq_coefficient_2;
	EQCoefficient3			eq_coefficient_3;
	EQCoefficient4			eq_coefficient_4;
	EQCoefficient5			eq_coefficient_5;
	DigitalFilterSelect3	digital_filter_select_3;
	DeviceInformation		device_information;
	E1Coefficient0			e1_coefficient_0;
	E1Coefficient1			e1_coefficient_1;
	E1Coefficient2			e1_coefficient_2;
	E1Coefficient3			e1_coefficient_3;
	E1Coefficient4			e1_coefficient_4;
	E1Coefficient5			e1_coefficient_5;
	E2Coefficient0			e2_coefficient_0;
	E2Coefficient1			e2_coefficient_1;
	E2Coefficient2			e2_coefficient_2;
	E2Coefficient3			e2_coefficient_3;
	E2Coefficient4			e2_coefficient_4;
	E2Coefficient5			e2_coefficient_5;
	E3Coefficient0			e3_coefficient_0;
	E3Coefficient1			e3_coefficient_1;
	E3Coefficient2			e3_coefficient_2;
	E3Coefficient3			e3_coefficient_3;
	E3Coefficient4			e3_coefficient_4;
	E3Coefficient5			e3_coefficient_5;
	E4Coefficient0			e4_coefficient_0;
	E4Coefficient1			e4_coefficient_1;
	E4Coefficient2			e4_coefficient_2;
	E4Coefficient3			e4_coefficient_3;
	E4Coefficient4			e4_coefficient_4;
	E4Coefficient5			e4_coefficient_5;
	E5Coefficient0			e5_coefficient_0;
	E5Coefficient1			e5_coefficient_1;
	E5Coefficient2			e5_coefficient_2;
	E5Coefficient3			e5_coefficient_3;
	E5Coefficient4			e5_coefficient_4;
	E5Coefficient5			e5_coefficient_5;
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

constexpr RegisterMap default_after_reset { Register_Type {
	.power_management_1 = {
		.PMADL = 0,
		.PMADR = 0,
		.PMDAC = 0,
		.reserved0 = 0,
		.PMBP = 0,
		.PMVCM = 0,
		.PMPFIL = 0,
	},
	.power_management_2 = {
		.LOSEL = 0,
		.PMSL = 0,
		.PMPLL = 0,
		.MS = 0,
		.PMHPL = 0,
		.PMHPR = 0,
		.reserved0 = 0,
		.PMOSC = 0,
	},
	.signal_select_1 = {
		.MGAIN20 = 0b110,
		.PMMP = 0,
		.MPSEL = 0,
		.DACS = 0,
		.MGAIN3 = 0,
		.SLPSN = 0,
	},
	.signal_select_2 = {
		.INR = 0b00,
		.INL = 0b00,
		.MICL = 0,
		.reserved0 = 0,
		.SPKG = 0b00,
	},
	.signal_select_3 = {
		.MONO = 0b00,
		.PTS = 0b01,
		.reserved0 = 0,
		.DACL = 0,
		.LVCM = 0b01,
	},
	.mode_control_1 = {
		.DIF = 0b10,
		.CKOFF = 0,
		.BCKO = 0,
		.PLL = 0b0101,
	},
	.mode_control_2 = {
		.FS = 0b1011,
		.reserved0 = 0,
		.CM = 0b00,
	},
	.mode_control_3 = {
		.reserved0 = 0,
		.IVOLC = 1,
		.reserved1 = 0,
		.DVOLC = 1,
		.SMUTE = 0,
		.THDET = 0,
		.TSDSEL = 0,
	},
	.digital_mic = {
		.DMIC = 0,
		.DCLKP = 0,
		.reserved0 = 0,
		.DCLKE = 0,
		.PMDML = 0,
		.PMDMR = 1,
		.reserved1 = 0,
		.READ = 0,
	},
	.timer_select = {
		.DVTM = 0,
		.MOFF = 0,
		.reserved0 = 0,
		.FRN = 0,
		.FRATT = 0,
		.ADRST = 0b00,
	},
	.alc_timer_select = {
		.RFST = 0b00,
		.WTM = 0b00,
		.EQFC = 0b10,
		.IVTM = 1,
		.reserved0 = 0,
	},
	.alc_mode_control_1 = {
		.LMTH10 = 0b00,
		.RGAIN = 0b000,
		.ALC = 0,
		.LMTH2 = 0,
		.ALCEQN = 0,
	},
	.alc_mode_control_2 = {
		.REF = 0xe1,
	},
	.l_ch_input_volume_control = {
		.IV = 0xe1,
	},
	.r_ch_input_volume_control = {
		.IV = 0xe1,
	},
	.alc_volume = {
		.VOL = 0x00,	// Read-only.
	},
	._reserved_0x10 = 0x80,
	.r_ch_mic_gain_setting = {
		.MGR = 0x80,
	},
	.beep_control = {
		.BPLVL = 0b0000,
		.BEEPH = 0,
		.BEEPS = 0,
		.BPVCM = 0,
		.HPZ = 0,
	},
	.l_ch_digital_volume_control = {
		.DV = 0x18,
	},
	.r_ch_digital_volume_control = {
		.DV = 0x18,
	},
	.eq_common_gain_select = {
		.reserved0 = 0,
		.EQC2 = 0,
		.EQC3 = 0,
		.EQC4 = 0,
		.EQC5 = 0,
		.reserved1 = 0,
	},
	.eq2_common_gain_setting = {
		.EQnT = 0b00,
		.EQnG = 0b000000,
	},
	.eq3_common_gain_setting = {
		.EQnT = 0b00,
		.EQnG = 0b000000,
	},
	.eq4_common_gain_setting = {
		.EQnT = 0b00,
		.EQnG = 0b000000,
	},
	.eq5_common_gain_setting = {
		.EQnT = 0b00,
		.EQnG = 0b000000,
	},
	.auto_hpf_control = {
		.STG = 0b00,
		.SENC = 0b011,
		.AHPF = 0,
		.reserved0 = 0,
	},
	.digital_filter_select_1 = {
		.HPFAD = 1,
		.HPFC = 0b00,
		.reserved0 = 0,
	},
	.digital_filter_select_2 = {
		.HPF = 0,
		.LPF = 0,
		.reserved0 = 0,
		.FIL3 = 0,
		.EQ0 = 0,
		.GN = 0b00,
	},
	.digital_filter_mode = {
		.PFSDO = 1,
		.ADCPF = 1,
		.PFDAC = 0b00,
		.PFVOL = 0b00,
		.reserved0 = 0,
	},
	
	.hpf_2_coefficient_0 = { .l = 0xb0 },
	.hpf_2_coefficient_1 = { .h = 0x1f, .reserved0 = 0 },
	.hpf_2_coefficient_2 = { .l = 0x9f },
	.hpf_2_coefficient_3 = { .h = 0x20, .reserved0 = 0 },
	
	.lpf_coefficient_0 = { .l = 0x00 },
	.lpf_coefficient_1 = { .h = 0x00, .reserved0 = 0 },
	.lpf_coefficient_2 = { .l = 0x00 },
	.lpf_coefficient_3 = { .h = 0x00, .reserved0 = 0 },
	
	.fil_3_coefficient_0 = { .l = 0x00 },
	.fil_3_coefficient_1 = { .h = 0x00, .reserved0 = 0, .s = 0 },
	.fil_3_coefficient_2 = { .l = 0x00 },
	.fil_3_coefficient_3 = { .h = 0x00, .reserved0 = 0 },
	
	.eq_coefficient_0 = { .l = 0x00 },
	.eq_coefficient_1 = { .h = 0x00 },
	.eq_coefficient_2 = { .l = 0x00 },
	.eq_coefficient_3 = { .h = 0x00, .reserved0 = 0 },
	.eq_coefficient_4 = { .l = 0x00 },
	.eq_coefficient_5 = { .h = 0x00 },

	.digital_filter_select_3 = {
		.EQ1 = 0,
		.EQ2 = 0,
		.EQ3 = 0,
		.EQ4 = 0,
		.EQ5 = 0,
		.reserved0 = 0,
	},
	.device_information = {
		.DVN = 0b0001,
		.REV = 0b1100,
	},
	
	// just pre-loading into memory, 30 bytes =  EQ 1,2,3,4,5  x A,B,C (2 x bytes) coefficients, but it will be written from ak4951.cpp 
	.e1_coefficient_0 = { .l = 0xCA },		//EQ1 Coefficient A  : A7...A0,		BW : 300Hz - 1700Hz  (fo = 1150Hz , fb= 1700Hz) , k=1,8 peaking 
	.e1_coefficient_1 = { .h = 0x05 },		//EQ1 Coefficient A  : A15..A8
	.e1_coefficient_2 = { .l = 0xEB },		//EQ1 Coefficient B  : B7...B0
	.e1_coefficient_3 = { .h = 0x38 },		//EQ1 Coefficient B  : B15...B8
	.e1_coefficient_4 = { .l = 0x6F },		//EQ1 Coefficient C  : C7...C0
	.e1_coefficient_5 = { .h = 0xE6 },		//EQ1 Coefficient C  : C15..C8
	
	.e2_coefficient_0 = { .l = 0x05 },		//EQ2 Coefficient A  : A7...A0,		BW : 250Hz - 2700Hz  (fo = 1475Hz , fb= 2450Hz) , k=1,8 peaking  
	.e2_coefficient_1 = { .h = 0x08 },		//EQ2 Coefficient A  : A15..A8
	.e2_coefficient_2 = { .l = 0x11 },		//EQ2 Coefficient B  : B7...B0
	.e2_coefficient_3 = { .h = 0x36 },		//EQ2 Coefficient B  : B15...B8
	.e2_coefficient_4 = { .l = 0xE9 },		//EQ2 Coefficient C  : C7...C0
	.e2_coefficient_5 = { .h = 0xE8 },		//EQ2 Coefficient C  : C15..C8
	
	.e3_coefficient_0 = { .l = 0x00 },		//EQ3 Coefficient A  : A7...A0,		not used  currently 
	.e3_coefficient_1 = { .h = 0x00 },		//EQ3 Coefficient A  : A15..A8
	.e3_coefficient_2 = { .l = 0x00 },		//EQ3 Coefficient B  : B7...B0
	.e3_coefficient_3 = { .h = 0x00 },		//EQ3 Coefficient B  : B15...B8
	.e3_coefficient_4 = { .l = 0x00 },		//EQ3 Coefficient C  : C7...C0
	.e3_coefficient_5 = { .h = 0x00 },		//EQ3 Coefficient C  : C15..C8
	
	.e4_coefficient_0 = { .l = 0x00 },		//EQ4 Coefficient A  : A7...A0,		not used  currently
	.e4_coefficient_1 = { .h = 0x00 },		//EQ4 Coefficient A  : A15..A8
	.e4_coefficient_2 = { .l = 0x00 },		//EQ4 Coefficient B  : B7...B0
	.e4_coefficient_3 = { .h = 0x00 },		//EQ4 Coefficient B  : B15...B8
	.e4_coefficient_4 = { .l = 0x00 },		//EQ4 Coefficient C  : C7...C0
	.e4_coefficient_5 = { .h = 0x00 },		//EQ4 Coefficient C  : C15..C8

	.e5_coefficient_0 = { .l = 0x00 },		//EQ5 Coefficient A  : A7...A0,		not used currently
	.e5_coefficient_1 = { .h = 0x00 },		//EQ5 Coefficient A  : A15..A8
	.e5_coefficient_2 = { .l = 0x00 },		//EQ5 Coefficient B  : B7...B0
	.e5_coefficient_3 = { .h = 0x00 },		//EQ5 Coefficient B  : B15...B8
	.e5_coefficient_4 = { .l = 0x00 },		//EQ5 Coefficient C  : C7...C0
	.e5_coefficient_5 = { .h = 0x00 },		//EQ5 Coefficient C  : C15..C8
} };

class AK4951 : public audio::Codec  {
public:
	constexpr AK4951(
		I2C& bus,
		const I2C::address_t bus_address
	) : bus(bus),
		bus_address(bus_address)
	{
	}

	std::string name() const override {
		return "AK4951";
	}

	bool detected();
	
	void init() override;
	bool reset() override;

	volume_range_t headphone_gain_range() const override {
		return { -89.5_dB, 12.0_dB }; 
	}

	void headphone_enable() override;
	void headphone_disable() override;

	void speaker_enable();
	void speaker_disable();

	void set_headphone_volume(const volume_t volume) override;
	void headphone_mute();

	void microphone_enable(int8_t alc_mode); 	// added user GUI parameter , to set up AK4951 ALC mode.
	void microphone_disable();

	size_t reg_count() const override {
		return asahi_kasei::ak4951::reg_count;
	}

	size_t reg_bits() const override {
		return 8;
	}

	uint32_t reg_read(const size_t reg_address) override {
		return read(reg_address);
	}

private:
	I2C& bus;
	const I2C::address_t bus_address;
	RegisterMap map { default_after_reset };

	enum class LineOutSelect {
		Speaker,
		Line,
	};

	void configure_digital_interface_i2s();
	void configure_digital_interface_external_slave();
	void configure_digital_interface_external_master();
	void set_digtal_volume_control(const reg_t value);
	void set_dac_power(const bool enable);
	void set_headphone_power(const bool enable);
	void set_speaker_power(const bool enable);
	void select_line_out(const LineOutSelect value);

	reg_t read(const address_t reg_address);
	void update(const Register reg);
	void write(const address_t reg_address, const reg_t value);
};

} /* namespace ak4951 */
} /* namespace asahi_kasei */

#endif/*__AK4951_H__*/
