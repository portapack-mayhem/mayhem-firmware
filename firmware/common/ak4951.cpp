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

#include "ak4951.hpp"

#include "portapack_io.hpp"
using namespace portapack;

#include <ch.h>

namespace asahi_kasei {
namespace ak4951 {

void AK4951::configure_digital_interface_i2s() {
	// Configure for external slave mode.
	map.r.mode_control_1.DIF = 0b11;	// I2S compatible
	map.r.mode_control_1.BCKO = 0;		// BICK = 32fs
	update(Register::ModeControl1);

	map.r.mode_control_2.CM = 0b00;		// MCKI = 256fs
	map.r.mode_control_2.FS = 0b1011;	// fs = 48kHz
	update(Register::ModeControl2);
}

void AK4951::configure_digital_interface_external_slave() {
	map.r.power_management_2.MS = 0;	// Slave mode
	map.r.power_management_2.PMPLL = 0;	// EXT mode
	update(Register::PowerManagement2);
}

void AK4951::configure_digital_interface_external_master() {
	map.r.power_management_2.MS = 1;	// Master mode
	map.r.power_management_2.PMPLL = 0;	// EXT mode
	update(Register::PowerManagement2);
}

void AK4951::init() {
	reset();

	// Write dummy address to "release" the reset.
	write(0x00, 0x00);

	configure_digital_interface_i2s();
	configure_digital_interface_external_slave();

	map.r.power_management_1.PMVCM = 1;
	update(Register::PowerManagement1);

	// Headphone output is hi-Z when not active, reduces crosstalk from speaker output.
	map.r.beep_control.HPZ = 1;
	update(Register::BeepControl);

	// Pause for VCOM and REGFIL pins to stabilize.
	chThdSleepMilliseconds(2);

	headphone_mute();

	// SPK-Amp gain setting: SPKG1-0 bits = “00” → “01”
	map.r.signal_select_2.SPKG = 0b01;
	update(Register::SignalSelect2);

	map.r.signal_select_3.MONO = 0b00;
	update(Register::SignalSelect3);

	map.r.digital_filter_mode.PFSDO = 0;	// ADC bypass digital filter block.
	map.r.digital_filter_mode.ADCPF = 1;	// ADC output
	map.r.digital_filter_mode.PFDAC = 0b00;	// SDTI
	update(Register::DigitalFilterMode);

	// Set up FRN, FRATT and ADRST1-0 bits (Addr = 09H)
	// map.r.timer_select.FRN = 0;
	// map.r.timer_select.FRATT = 0;
	// map.r.timer_select.ADRST = 0b00;
	// update(Register::TimerSelect);

	// Set up ALC mode (Addr = 0AH, 0BH)
	// map.r.alc_timer_select. = ;
	// update(Register::ALCTimerSelect);
	// map.r.alc_mode_control_1. = ;
	// update(Register::ALCModeControl1);

	// Set up REF value of ALC (Addr = 0CH)
	// map.r.alc_mode_control_2. = ;
	// update(Register::ALCModeControl2);

	// Set up IVOL value of ALC operation start (Addr = 0DH)
	// map.r.l_ch_input_volume_control. = ;
	// update(Register::LchInputVolumeControl);
	// map.r.r_ch_input_volume_control. = ;
	// update(Register::RchInputVolumeControl);

	// Set up the output digital volume. (Addr = 13H)
	// set_headphone_volume(...);

	// Set up Programmable Filter Path: PFDAC1-0 bits=“01”, PFSDO=ADCPF bits=“0” (Addr = 1DH)
	// map.r.digital_filter_mode.PFDAC = 0b01;
	// update(Register::DigitalFilterMode);
}

bool AK4951::detected() {
	return reset();
}

bool AK4951::reset() {
	io.audio_reset_state(true);

	// PDN# pulse must be >200ns
	chThdSleepMicroseconds(10);

	io.audio_reset_state(false);

	return true;
}

void AK4951::set_digtal_volume_control(const reg_t value) {
	map.r.l_ch_digital_volume_control.DV = value;
	update(Register::LchDigitalVolumeControl);
}

void AK4951::set_headphone_volume(const volume_t volume) {
	const auto normalized = headphone_gain_range().normalize(volume);
	auto n = normalized.centibel() / 5;
	set_digtal_volume_control(0xcb - n);
}

void AK4951::headphone_mute() {
	set_digtal_volume_control(0xff);
}

void AK4951::set_dac_power(const bool enable) {
	map.r.power_management_1.PMDAC = enable;
	update(Register::PowerManagement1);
}

void AK4951::set_headphone_power(const bool enable) {
	map.r.power_management_2.PMHPL = map.r.power_management_2.PMHPR = enable;
	update(Register::PowerManagement2);
}

void AK4951::set_speaker_power(const bool enable) {
	map.r.power_management_2.PMSL = enable;
	update(Register::PowerManagement2);
}

void AK4951::select_line_out(const LineOutSelect value) {
	map.r.power_management_2.LOSEL = (value == LineOutSelect::Line) ? 1 : 0;
	update(Register::PowerManagement2);
}

void AK4951::headphone_enable() {
	set_dac_power(true);
	set_headphone_power(true);

	// Wait for headphone amplifier charge pump power-up.
	chThdSleepMilliseconds(35);
}

void AK4951::headphone_disable() {
	set_headphone_power(false);
	set_dac_power(false);
}

void AK4951::speaker_enable() {
	// Set up the path of DAC → SPK-Amp: DACS bit = “0” → “1”
	map.r.signal_select_1.DACS = 1;
	update(Register::SignalSelect1);

	// Enter Speaker-Amp Output Mode: LOSEL bit = “0”
	select_line_out(LineOutSelect::Speaker);

	// Power up DAC, Programmable Filter and Speaker-Amp: PMDAC=PMPFIL=PMSL bits=“0”→“1”
	set_dac_power(true);
	// map.r.power_management_1.PMPFIL = 1;
	// update(Register::PowerManagement1);
	set_speaker_power(true);

	// Time from PMSL=1 to SLPSN=1.
	chThdSleepMilliseconds(1);

	// Exit the power-save mode of Speaker-Amp: SLPSN bit = “0” → “1”
	map.r.signal_select_1.SLPSN = 1;
	update(Register::SignalSelect1);
}

void AK4951::speaker_disable() {
	// Enter Speaker-Amp Power Save Mode: SLPSN bit = “1” → “0”
	map.r.signal_select_1.SLPSN = 0;
	update(Register::SignalSelect1);

	// Disable the path of DAC → SPK-Amp: DACS bit = “1” → “0”
	map.r.signal_select_1.DACS = 0;
	update(Register::SignalSelect1);

	// Power down DAC, Programmable Filter and speaker: PMDAC=PMPFIL=PMSL bits= “1”→“0”
	set_dac_power(false);
	// map.r.power_management_1.PMPFIL = 0;
	// update(Register::PowerManagement1);
	set_speaker_power(false);
}

void AK4951::microphone_enable(int8_t alc_mode) {
// alc_mode =0 = (OFF =same as original code = NOT using AK4951 Programmable digital filter block), 
// alc_mode >1 (with  DIGITAL FILTER BLOCK , example :  1:(+12dB) , 2:(+9dB)", 3:(+6dB), ...) 

// map.r.digital_mic.DMIC = 0;		// originally commented code 
// update(Register::DigitalMic);	// originally commented code 

uint_fast8_t mgain =0b0111;   			// Pre-amp mic (Original code,  =0b0111 (+21dB's=7x3dBs),(Max is NOT 0b1111!, it is 0b1010=+30dBs=10x3dBs) 

map.r.signal_select_2.INL = 0b01;		// Lch input signal = LIN2 , our ext. MONO MIC is connected here LIN2 in Portapack.
map.r.signal_select_2.INR = 0b01;		// Rch input signal = RIN2 , Not used ,not connected ,but no problem. 
map.r.signal_select_2.MICL = 0;			// MPWR = 2.4V  (it has two possible settings , 2.4V or 2.0V) , (majority smarthphones around 2V , range 1V-5V)
update(Register::SignalSelect2);

// ------Common code part, =  original setting  conditions, it is fine for all user-GUI alc_modes: OFF , and ALC modes .*/
map.r.digital_filter_select_1.HPFAD = 1;	// HPF1 ON (after ADC);page 40 datasheet, HPFAD bit controls the ON/OFF of the HPF1 (HPF ON is recommended).
map.r.digital_filter_select_1.HPFC = 0b11;	// HPF Cut off frequency of high pass filter from 236.8 Hz @fs=48k  ("00":3.7Hz, "01":14,8Hz, "10":118,4Hz) 
update(Register::DigitalFilterSelect1);

// map.r.r_ch_mic_gain_setting.MGR = 0x80;	// Microphone sensitivity correction = 0dB.,  (not used by now , original code cond.)
// update(Register::RchMicGainSetting);		// (those two lines , not activated, same as original)	

// pre-load 4 byes LPF coefficicients (.lpf_coefficient_0,1,2,3), FSA 14..0, FSB 14..0 , (fcut initial 6kHz, fs 48Khz). 	
// it will be default pre-loading coeff. for al  ALC modes,  LPF bit is activated down, for all ALC digital modes.
map.r.lpf_coefficient_0.l = 0x5F;		// Pre-loading here LPF 6kHz, 1st Order from digital Block , Fc=6000 Hz,  fs = 48khz
map.r.lpf_coefficient_1.h = 0x09;		// LPF bit is activated down, for all ALC digital modes.
map.r.lpf_coefficient_2.l = 0xBF;		// Writting reg to AK4951,  with "update", following instructions.
map.r.lpf_coefficient_3.h = 0x32;

update(Register::LPFCoefficient0);	// Writing  pre-loaded  4 bytes LPF  CoefFiecients 14 bits (FSA13..0,  FSB13..0
update(Register::LPFCoefficient1);	// In this case , LPF 6KHz , when we activate the LPF block.
update(Register::LPFCoefficient2);
update(Register::LPFCoefficient3);

// Reset , setting OFF all 5 x Digital Equalizer filters 
map.r.digital_filter_select_3.EQ1 = 0;	  // EQ1 Coeffic Setting , (0: Disable-default, audio data passes  EQ1 block by 0dB gain). When EQ1="1”, the settings of E1A15-0, E1B15-0 and E1C15-0 bits are enabled
map.r.digital_filter_select_3.EQ2 = 0;	  // EQ2 Coeffic Setting , (0: Disable-default, audio data passes  EQ2 block by 0dB gain). When EQ2="1”, the settings of E2A15-0, E2B15-0 and E2C15-0 bits are enabled
map.r.digital_filter_select_3.EQ3 = 0;    // EQ3 Coeffic Setting , (0: Disable-default, audio data passes  EQ3 block by 0dB gain). When EQ3="1”, the settings of E3A15-0, E3B15-0 and E3C15-0 bits are enabled
map.r.digital_filter_select_3.EQ4 = 0;    // EQ4 Coeffic Setting , (0: Disable-default, audio data passes  EQ4 block by 0dB gain). When EQ4="1”, the settings of E4A15-0, E4B15-0 and E4C15-0 bits are enabled
map.r.digital_filter_select_3.EQ5 = 0;    // EQ5 Coeffic Setting , (0: Disable-default, audio data passes  EQ5 block by 0dB gain). When EQ5="1”, the settings of E5A15-0, E5B15-0 and E5C15-0 bits are enabled
update(Register::DigitalFilterSelect3);		// A,B,C EQ1 Coefficients are already pre-loaded in ak4951.hpp


  if (alc_mode==0) { 		// Programmable Digital Filter OFF,  same as original condition., no Digital ALC, nor Wind Noise Filter, LPF , EQ 

		map.r.digital_filter_select_2.LPF = 0;	  // LPF-Block, Coeffic Setting Enable (OFF-Default), When LPF bit is “0”, audio data passes the LPF block by 0dB gain. 
		update(Register::DigitalFilterSelect2);

	  	// Pre-loading AUDIO PATH with all DIGITAL BLOCK by pased, see, audio path block diagramm AK4951 datasheet + Table  Playback mode  -Recording mode. 
		// Digital filter block PATH is BY PASSED (we can swith off DIG. BLOCK power , PMPFIL=0) .The Path in Recording Mode 2 & Playback Mode 2  (NO DIG FILTER BLOCK AT ALL, not for MIC recording, nor for Playback)
		map.r.digital_filter_mode.ADCPF = 1;	// ADCPF bit swith ("0" Mic  after ADC Output connected (recording mode) to the DIGITAL FILTER  BLOCK. ("1" Playback mode)  
		map.r.digital_filter_mode.PFSDO = 0;	// ADC bit switch ("0" : 1st order HPF) connectedto the  Output. By bass DIGITAL block .
    	map.r.digital_filter_mode.PFDAC = 0b00;	// (Input selector for DAC (not used in MIC), SDTI= Audio Serial Data Input Pin)
		update(Register::DigitalFilterMode);		// Writing the Audio Path :  NO DIGITAL BLOCK or DIG BLOCK FOR  MIC ,   Audio mode path : Playback mode /-Recording mode.

		map.r.power_management_1.PMADL  = 1;	// ADC Lch = Lch input signal. Mic Amp Lch and ADC Lch Power Management
		map.r.power_management_1.PMADR  = 1;	// ADC Rch = Rch input signal. Mic Amp Rch and ADC Rch Power Management
		map.r.power_management_1.PMPFIL = 0;	// Pre-loading , Programmable Dig. filter OFF ,filter unused, routed around.(original value = 0 ) 
		update(Register::PowerManagement1);		// Activating the Power management of the used blocks . (Mic ADC always + Dig Block filter , when used )

		// 1059/fs, 22ms @ 48kHz
		chThdSleepMilliseconds(22);

  } else {    // ( alc_mode !=0) 

	switch(alc_mode) { 	// Pre-loading register values depending on user-GUI selection (they will be sended below, with "update(Register_name::xxx )".
	 
		case 1: 	// ALC-> on, (+12dB's) Auto Vol max + Wind Noise cancel + LPF 6kHz + Pre-amp Mic (+21dB=original)
			map.r.alc_mode_control_2.REF = 0xC0;    	// REF7-0 bits,max gain at ALC recovery operation,(FFH +36dBs , D0H +18dBs, A0H 0dBs, C0H=+12dBs) 
			map.r.l_ch_input_volume_control.IV = 0xC0; 	// Left, Input Digital Volume Setting, (FFH +36dBs , D0H +18dBs, A0H 0dBs, 70H=-18dBs)
			map.r.r_ch_input_volume_control.IV = 0xC0; 	// Right Input Dig Vol Setting, same comment as above , The value of IVOL should be <= than REF’s

			// Already Pre-loaded, "map.r.lpf_coefficient", 6Khz - LPF 1st Order from digital Block,Fc=6000Hz,fs = 48khz
			// LPF bit is activated down, for all ALC digital modes.
			break;

		case 2: 	// ALC-> on, (+09dB's) Auto Vol max + Wind Noise cancel + LPF 6kHz + Pre-amp Mic (+21dB=original)
			map.r.alc_mode_control_2.REF = 0xB8;    	// REF7-0 bits,max gain at ALC recoveryoperation,(FFH +36dBs , D0H +18dBs, A0H 0dBs, B8H= +9dBs) 
			map.r.l_ch_input_volume_control.IV = 0xB8; 	// Left, Input Digital Volume Setting, (FFH +36dBs , D0H +18dBs, A0H 0dBs, 70H=-18dBs)
			map.r.r_ch_input_volume_control.IV = 0xB8; 	// Right Input Dig Vol Setting, same comment as above , The value of IVOL should be <= than REF’s

			// Already Pre-loaded, "map.r.lpf_coefficient", 6Khz - LPF 1st Order from digital Block,Fc=6000Hz,fs = 48khz
			// LPF bit is activated down, for all ALC digital modes.
			break;

	    case 3: 	// ALC-> on, (+06dB's) Auto Vol max + Wind Noise cancel + LPF 6kHz + Pre-amp Mic (+21dB=original)
			map.r.alc_mode_control_2.REF = 0xB0;    	// 0xB8 , REF7-0 bits,max gain at ALC recoveryoperation,(FFH +36dBs , D0H +18dBs, A0H 0dBs, B0H= +6dBs) 
			map.r.l_ch_input_volume_control.IV = 0xB0; 	// Left, Input Digital Volume Setting, (FFH +36dBs , D0H +18dBs, A0H 0dBs, 70H=-18dBs)
			map.r.r_ch_input_volume_control.IV = 0xB0; 	// Right Input Dig Vol Setting, same comment as above , Then value of IVOL should be <= than REF’s
		
			// Already Pre-loaded, "map.r.lpf_coefficient", 6Khz - LPF 1st Order from digital Block,Fc=6000Hz,fs = 48khz
			// LPF bit is activated down, for all ALC digital modes.
			break;

	    case 4: 	// ALC-> on, (+03dB's) Auto Vol max + Wind Noise cancel + Pre-amp Mic (+21dB=original)
					// + EQ boosting ~<2kHz (f0:1,1k, fb:1,7K, k=1,8)   && + LPF 3,5k 
			map.r.alc_mode_control_2.REF = 0xA8;    	// 0xA8 , REF7-0 bits,max gain at ALC recoveryoperation,(FFH +36dBs , D0H +18dBs, A0H 0dBs, A8H= +3dBs) 
			map.r.l_ch_input_volume_control.IV = 0xA8; 	// Left, Input Digital Volume Setting, (FFH +36dBs , D0H +18dBs, A0H 0dBs, 70H=-18dBs)
			map.r.r_ch_input_volume_control.IV = 0xA8; 	// Right Input Dig Vol Setting, same comment as above , Then value of IVOL should be <= than REF’s

			//The EQn (n=1, 2, 3, 4 or 5) coefficient must be set when EQn bit = “0” or PMPFIL bit = “0”.
			map.r.digital_filter_select_3.EQ1 = 1;	    // EQ1 Coeffic Setting , (0: Disable-default, audio data passes  EQ1 block by 0dB gain). When EQ1="1”, the settings of E1A15-0, E1B15-0 and E1C15-0 bits are enabled
			update(Register::DigitalFilterSelect3);		// A,B,C EQ1 Coefficients are already pre-loaded in ak4951.hpp

			map.r.lpf_coefficient_0.l = 0x0D;		// Pre-loading here LPF 3,5k  ,  1st Order from digital Block , Fc=3.500 Hz,  fs = 48khz
		   	map.r.lpf_coefficient_1.h = 0x06;		// LPF bit is activated down, for all ALC digital modes.
       		map.r.lpf_coefficient_2.l = 0x1A;		// Writting reg to AK4951 , down with update....
	   		map.r.lpf_coefficient_3.h = 0x2C;
   			// LPF bit is activated down, for all ALC digital modes.			
 			break;

	  	case 5: 	// ALC-> on, (+03dB's) Auto Vol max + Wind Noise cancel + Pre-amp Mic (+21dB=original)
		  			// + EQ boosting ~<3kHz (f0~1k4,fb~2,4k,k=1,8)   && LPF 4kHz 
			map.r.alc_mode_control_2.REF = 0xA8;    	// 0xA0 , REF7-0 bits,max gain at ALC recoveryoperation,(FFH +36dBs , D0H +18dBs, A0H 0dBs, A8H= +3dBs) 
			map.r.l_ch_input_volume_control.IV = 0xA8; 	// Left, Input Digital Volume Setting, (FFH +36dBs , D0H +18dBs, A0H 0dBs, 70H=-18dBs)
			map.r.r_ch_input_volume_control.IV = 0xA8; 	// Right Input Dig Vol Setting, same comment as above , Then value of IVOL should be <= than REF’s
		
			map.r.digital_filter_select_3.EQ2 = 1;	  // EQ2 Coeffic Setting , (0: Disable-default, audio data passes  EQ2 block by 0dB gain). When EQ2="1”, the settings of E2A15-0, E2B15-0 and E2C15-0 bits are enabled
			update(Register::DigitalFilterSelect3);

			map.r.lpf_coefficient_0.l = 0xC3;		// Pre-loading here LPF 4k  ,  1st Order from digital Block , Fc=4000 Hz,  fs = 48khz
	   		map.r.lpf_coefficient_1.h = 0x06;		// LPF bit is activated down, for all ALC digital modes.
       		map.r.lpf_coefficient_2.l = 0x86;		// Writting reg to AK4951 , down with update....
	   		map.r.lpf_coefficient_3.h = 0x2D;
   			// LPF bit is activated down, for all ALC digital modes.
			break;

		 case 6: 	// ALC-> on, (+03dB's) Auto Vol max + Wind Noise cancel + LPF 6kHz + Pre-amp Mic (+21dB=original)
			map.r.alc_mode_control_2.REF = 0xA8;    	// REF7-0 bits,max gain at ALC recoveryoperation,(FFH +36dBs , D0H +18dBs, A0H 0dBs, A0H= 0dBs) 
			map.r.l_ch_input_volume_control.IV = 0xA8; 	// Left, Input Digital Volume Setting, (FFH +36dBs , D0H +18dBs, A0H 0dBs, 70H=-18dBs)
			map.r.r_ch_input_volume_control.IV = 0xA8; 	// Right Input Dig Vol Setting, same comment as above , Then value of IVOL should be <= than REF’s

			// Already Pre-loaded, "map.r.lpf_coefficient", 6Khz - LPF 1st Order from digital Block,Fc=6000Hz,fs = 48khz
			// LPF bit is activated down, for all ALC digital modes.
			break;

		 case 7: 	// ALC-> on, (+00dB's) Auto Vol max + Wind Noise cancel + LPF 6kHz + Pre-amp Mic (+21dB=original)
			map.r.alc_mode_control_2.REF = 0xA0;    	// REF7-0 bits,max gain at ALC recoveryoperation,(FFH +36dBs , D0H +18dBs, A0H 0dBs, A0H= 0dBs) 
			map.r.l_ch_input_volume_control.IV = 0xA0; 	// Left, Input Digital Volume Setting, (FFH +36dBs , D0H +18dBs, A0H 0dBs, 70H=-18dBs)
			map.r.r_ch_input_volume_control.IV = 0xA0; 	// Right Input Dig Vol Setting, same comment as above , Then value of IVOL should be <= than REF’s

			// Already Pre-loaded, "map.r.lpf_coefficient", 6Khz - LPF 1st Order from digital Block,Fc=6000Hz,fs = 48khz
			// LPF bit is activated down, for all ALC digital modes.
			break;

		case 8:		// ALC-> on, (-03dB's) Auto Vol max + Wind Noise cancel + LPF 6kHz + Pre-amp Mic (+21dB=original)
			map.r.alc_mode_control_2.REF = 0x98;    	//REF7-0 bits,max gain at ALC recovery operation,(FFH +36dBs , D0H +18dBs, A0H 0dBs, 98H=-03dBs) 
			map.r.l_ch_input_volume_control.IV = 0x98; 	// Left, Input Digital Volume Setting, (FFH +36dBs , D0H +18dBs, A0H 0dBs, 70H=-18dBs)
			map.r.r_ch_input_volume_control.IV = 0x98; 	// Right Input Dig Vol Setting, same comment as above , Then value of IVOL should be <= than REF’s
	
			// Already Pre-loaded, "map.r.lpf_coefficient", 6Khz - LPF 1st Order from digital Block,Fc=6000Hz,fs = 48khz
			// LPF bit is activated down, for all ALC digital modes.
			break;

		case 9: 	// ALC-> on, (-06dB's) Auto Vol max + Wind Noise cancel + LPF 6kHz + Pre-amp Mic (+21dB=original)
			map.r.alc_mode_control_2.REF = 0x90;    	// REF7-0 bits,max gain at ALC recovery operation,(FFH +36dBs , D0H +18dBs, A0H 0dBs, 90H=-06dBs) 
			map.r.l_ch_input_volume_control.IV = 0x90; 	// Left, Input Digital Volume Setting, (FFH +36dBs , D0H +18dBs, A0H 0dBs, 70H=-18dBs)
			map.r.r_ch_input_volume_control.IV = 0x90; 	// Right Input Dig Vol Setting, same comment as above , Then value of IVOL should be <= than REF’s

			// Already Pre-loaded, "map.r.lpf_coefficient", 6Khz - LPF 1st Order from digital Block,Fc=6000Hz,fs = 48khz
			// LPF bit is activated down, for all ALC digital modes.
			break;

		case 10: 	// ALC-> on, (-09dB's) Auto Vol max + Wind Noise cancel + LPF 6kHz - Pre-amp MIC -3dB (18dB's) 
			// Reduce also Pre-amp Mic -3dB's (+18dB's)		
			mgain = 0b0110;		// Pre-amp mic Mic Gain Pre-amp (+18dB), Original=0b0111 (+21dB's =7x3dBs),
	
			map.r.alc_mode_control_2.REF = 0x88;    	// REF7-0 bits,max gain at ALC recovery operation,(FFH +36dBs , D0H +18dBs, A0H 0dBs, 88H=-09dBs) 
			map.r.l_ch_input_volume_control.IV = 0x88; 	// Left, Input Digital Volume Setting, (FFH +36dBs , D0H +18dBs, A0H 0dBs, 70H=-18dBs)
			map.r.r_ch_input_volume_control.IV = 0x88; 	// Right Input Dig Vol Setting, same comment as above , Then value of IVOL should be <= than REF’s
 	
			// Already Pre-loaded, "map.r.lpf_coefficient", 6Khz - LPF 1st Order from digital Block,Fc=6000Hz,fs = 48khz
			// LPF bit is activated down, for all ALC digital modes.
			break;

		case 11: // ALC-> on, (-12dB's) Auto Vol max + Wind Noise cancel + LPF 6kHz - Pre-amp MIC -6dB (15dB's) 	
			// Reduce also Pre-amp Mic -6dB's (+15dB's)  	
			mgain = 0b0101;		// Pre-amp mic Mic Gain Pre-amp (+15dB), (Original=0b0111 (+21dB's= 7x3dBs),

			map.r.alc_mode_control_2.REF = 0x80;    	// REF7-0 bits,max gain at ALC recovery operation,(FFH +36dBs , D0H +18dBs, A0H 0dBs, 80H=-12dBs) 
			map.r.l_ch_input_volume_control.IV = 0x80; 	// Left, Input Digital Volume Setting, (FFH +36dBs , D0H +18dBs, A0H 0dBs, 70H=-18dBs)
			map.r.r_ch_input_volume_control.IV = 0x80; 	// Right Input Dig Vol Setting, same comment as above , Then value of IVOL should be <= than REF’s

			// Already Pre-loaded, "map.r.lpf_coefficient", 6Khz - LPF 1st Order from digital Block,Fc=6000Hz,fs = 48khz
			// LPF bit is activated down, for all ALC digital modes.
			break;
	}

	//-------------------------------DIGITAL  ALC  (Automatic Level Control ) --- --------
	map.r.alc_mode_control_1.ALC = 0;  // LMTH2-0, WTM1-0, RGAIN2-0, REF7-0, RFST1-0, EQFC1-0, FRATT, FRN and ALCEQN bits (needs to be set up with  ALC disable = 0) 
	update(Register::ALCModeControl1);

	map.r.timer_select.FRN = 0;      	// (FRN= 0  Fast Recovery mode , enable ) 
	map.r.timer_select.FRATT = 0;    	// Fast Recovery Ref. Volume Atten. Amount -0,00106dB's, timing 4/fs (default)
	map.r.timer_select.ADRST = 0b00; 	// initial offset ADC cycles , 22ms @fs=48Khz.
	update(Register::TimerSelect);

	map.r.alc_timer_select.RFST = 0b00;   // RFST1-0: ALC Fast Recovery Speed Default: “00” (0.0032dB)
	map.r.alc_timer_select.WTM = 0b00;    // ALC Recovery Operation Waiting Period 128/fs = 2,7 mseg (min=default)
	map.r.alc_timer_select.EQFC = 0b10;   // Selecting default, fs 48Khz , ALCEQ: First order zero pole high pass filter fc2=100Hz, fc1=150Hz
	map.r.alc_timer_select.IVTM = 0;      // IVTM bit set the vol transition time ,236/fs = 4,9msecs (min) (default was 19,7msegs.)
	update(Register::ALCTimerSelect);

	map.r.alc_mode_control_1.LMTH10 = 0b11;  // ALC Limiter Detec Level/ Recovery Counter Reset; lower 2 bits (Ob111=-8,4dbs), (default 0b000=-2,5dBs)
	map.r.alc_mode_control_1.RGAIN  = 0b000; // ALC Recovery Gain Step, max step , max speed. Default: “000” (0.00424dB)
	map.r.alc_mode_control_1.ALC = 1;		 // ALC Enable .  (we are now, NOT in MANUAL volume mode, only becomes manual when (ALC=“0” while ADCPF=“1”. )
	map.r.alc_mode_control_1.LMTH2 = 1;      // ALC Limiter Detection Level/ Recovery Counter Reset Level,Upper bit,default 0b000      
	map.r.alc_mode_control_1.ALCEQN = 1;     // ALC EQ Off =1 not used by now,   0: ALC EQ On (default) 
	update(Register::ALCModeControl1);

	//   map.r.alc_mode_control_2.REF = 0x??;   	// Pre-loaded in top part. Maximum gain at ALC recovery operation,.(FFH +36dBs , D0H +18dBs, A0H 0dBs, 70H=-18dBs) 
	update(Register::ALCModeControl2);

	//   map.r.l_ch_input_volume_control.IV = 0x??; // Pre-loaded in top part. Left, Input Digital Volume Setting,  (FFH +36dBs , D0H +18dBs, A0H 0dBs, 70H=-18dBs)
    update(Register::LchInputVolumeControl);
	
	//   map.r.r_ch_input_volume_control.IV = 0x??; // Pre-loaded in top part. Right,Input Digital Volume Setting, (FFH +36dBs , D0H +18dBs, A0H 0dBs, 70H=-18dBs) 
    update(Register::RchInputVolumeControl);
	
	
	//---------------Switch ON, Digital Automatic Wind Noise Filter reduction  -------------------     
	// Difficult to realise that Dynamic HPF Wind noise filter benefit, maybe because we have another fixed HPF 236.8 Hz .
	// Anyway ,  we propose to  activate it , with default  setting conditions.
	map.r.power_management_1.PMPFIL = 0;	// (*1) To programm SENC, STG , we need PMPFIL = 0 . (but this disconnect Digital block power supply.
	update(Register::PowerManagement1);		//  Updated PMPFIL to 0 . (*1)

	map.r.auto_hpf_control.STG = 0b00;		//  (00=LOW ATTENUATION Level), lets put 11 (HIGH ATTENUATION Level) (default 00)
	map.r.auto_hpf_control.SENC = 0b011;	//  (000=LOW sensitivity detection)… 111((MAX sensitivity detection) (default 011)
	map.r.auto_hpf_control.AHPF = 1;    	//  Autom. Wind noise filter ON  (AHPF bit=“1”).It atten. wind noise when detecting ,and adjusts the atten. level dynamically.
	update(Register::AutoHPFControl);       

	//  We are in Digital Block ON , (Wind Noise Filter+ALC+LPF+EQ),==>  needs at the end , PMPFIL=1 , Program. Dig.filter ON   
	//  map.r.power_management_1.PMPFIL = 1;	// that instruction is at the end , we can skp pre-loading Programmable Dig. filter ON (*1)
	//---------------------------------------------------------------------
 	    
	// Writing AUDIO PATH diagramm, Changing  Audio mode path : Playback mode1 /-Recording mode2. (Figure 37 AK4951 datasheet, Table 27. Recording Playback Mode)
	// When changing those modes, PMPFIL bit must be “0”, it is OK (*1)
	map.r.digital_filter_mode.ADCPF = 1;	// ADCPF bit swith ("0" Mic  after ADC Output connected (recording mode) to the DIGITAL FILTER  BLOCK. ("1" Playback mode)  
	map.r.digital_filter_mode.PFSDO = 1;	// ADC (+ 1st order HPF) Output
    map.r.digital_filter_mode.PFDAC = 0b00;	// (Input selector for DAC (not used in MIC), SDTI= Audio Serial Data Input Pin)
	update(Register::DigitalFilterMode);	// Writing the Audio Path :  NO DIGITAL BLOCK or DIG BLOCK FOR  MIC ,   Audio mode path : Playback mode /-Recording mode.

	// The EQn (n=1, 2, 3, 4 or 5) coefficient must be set when EQn bit = “0” or PMPFIL bit = “0”., but we are already (*1) 
	// map.r.power_management_1.PMPFIL = 0;		// In the previous Wind Noise Filter , we already set up PPFIL = 0
	// update(Register::PowerManagement1);		// Activating the Power management of the used blocks . (Mic ADC always + Dig Block filter , when used )

	// ... Set EQ & LPF  coefficients ---------------------------------

	// writting to the IC ak4951 reg. settings defined in Ak4951.hpp ,  the 30 bytes ,  EQ coefficient =  5 (EQ1,2,3,4,5) x 3 (A,B,C coefficients) x 2 bytes (16 bits)
    update(Register::E1Coefficient0);	// we could pre-load here,ex ,"map.r.e1_coefficient_0.l = 0x50;" , EQ1 Coefficient A  : A7...A0, but already done in ak4951.hpp
    update(Register::E1Coefficient1);	// we could pre-load here,ex ,"map.r.e1_coefficient_1.h = 0xFE;" , EQ1 Coefficient A  : A15..A8,  "   "
    update(Register::E1Coefficient2);	// we could pre-load here,ex ,"map.r.e1_coefficient_2.l = 0x29;" , EQ1 Coefficient B  : B7...B0,  "   "   
    update(Register::E1Coefficient3);	// we could pre-load here,ex ,"map.r.e1_coefficient_3.h = 0xC5;" , EQ1 Coefficient B  : B15..B8,  "   "
    update(Register::E1Coefficient4);	// we could pre-load here,ex ,"map.r.e1_coefficient_4.l = 0xA0;" , EQ1 Coefficient C  : C7...C0,  "   "
    update(Register::E1Coefficient5);	// we could pre-load here,ex ,"map.r.e1_coefficient_5.h = 0x1C;" , EQ1 Coefficient C  : C15..C8,  "   "
		
	update(Register::E2Coefficient0);	// writing  pre-loaded EQ2 coefficcients
    update(Register::E2Coefficient1);	
    update(Register::E2Coefficient2);
    update(Register::E2Coefficient3);
    update(Register::E2Coefficient4);
    update(Register::E2Coefficient5);

	// Already pre-loaded LPF coefficients to 6k, 3,5k or 4k  ,(LPF 6Khz all digital alc modes top , except when  3k5 , 4k) 
	update(Register::LPFCoefficient0);	// Writing  pre-loaded  4 bytes  LPF  CoefFiecients 14 bits (FSA13..0,  FSB13..0
	update(Register::LPFCoefficient1);
	update(Register::LPFCoefficient2);
	update(Register::LPFCoefficient3);

    // Activating LPF block , (and re-configuring the rest of bits of the same register)
	map.r.digital_filter_select_2.HPF = 0;    // HPF2-Block, Coeffic Setting Enable (OFF-Default), When HPF bit is “0”, audio data passes the HPF2 block by is 0dB gain. 
	map.r.digital_filter_select_2.LPF = 1;	  // LPF-Block, Coeffic Setting Enable (OFF-Default), When LPF bit is “0”, audio data passes the LPF block by 0dB gain. 	
	map.r.digital_filter_select_2.FIL3 = 0;	  // Stereo_Emphasis_Filter-Block,(OFF-Default)  Coefficient Setting Enable , OFF , Disable.	
	map.r.digital_filter_select_2.EQ0 = 0;    // Gain Compensation-Block, (OFF-Default) Coeffic Setting Enable, When EQ0 bit = “0” audio data passes the EQ0 block by 0dB gain. 
	map.r.digital_filter_select_2.GN = 0b00;  // Gain Setting of the Gain Compensation Block Default: “00”-Default (0dB)	
	update(Register::DigitalFilterSelect2);

		// Acitivating digital block , power supply 
	map.r.power_management_1.PMADL  = 1;	// ADC Lch = Lch input signal. Mic Amp Lch and ADC Lch Power Management
	map.r.power_management_1.PMADR  = 1;	// ADC Rch = Rch input signal. Mic Amp Rch and ADC Rch Power Management
	map.r.power_management_1.PMPFIL = 1;	// Pre-loaded in top part.  Orig value=0, Programmable Digital filter unused (not power up), routed around. 
	update(Register::PowerManagement1);		// Activating the Power management of the used blocks . (Mic ADC always + Dig Block filter , when used )

	// 1059/fs, 22ms @ 48kHz
	chThdSleepMilliseconds(22);

  } 

  // Common  part  for all alc_mode , --------------------------
  // const uint_fast8_t mgain = 0b0111;		// Already pre-loaded , in above switch case . 
  map.r.signal_select_1.MGAIN20 = mgain & 7;  // writing 3 lower bits of mgain , (pre-amp mic gain).  
  map.r.signal_select_1.PMMP = 1;				// Activating DC Mic Power supply through 2kohms res., similar majority  smartphones  headphone+mic jack, "plug-in-power"
  map.r.signal_select_1.MPSEL = 1;			// MPWR2 pin ,selecting output voltage to  MPWR2 pin, that we are using in portapack ext. MIC)
  map.r.signal_select_1.MGAIN3 = (mgain >> 3) & 1; // writing 4th upper bit of mgain  (pre-amp mic gain).
  update(Register::SignalSelect1);
	
}



void AK4951::microphone_disable() {
	map.r.power_management_1.PMADL = 0;		// original code  , disable Power managem.Mic  ADC L
	map.r.power_management_1.PMADR = 0;		// original code  , disable Power managem.Mic  ADC R
	map.r.power_management_1.PMPFIL = 0;	// original code  , disable Power managem. all Programmable Dig. block  
	update(Register::PowerManagement1);

	map.r.alc_mode_control_1.ALC = 0;		// original code  , Restore , disable ALC block.
	update(Register::ALCModeControl1);

	map.r.auto_hpf_control.AHPF = 0;    	//----------- new code addition , Restore disable Wind noise filter OFF (AHPF bit=“0”). 
	update(Register::AutoHPFControl); 

	//Restore original AUDIO PATH , condition,  (Digital filter block PATH is BY PASSED)  (we can also swith off DIG. BLOCK power , PMPFIL=0)
	// The Path in Recording Mode 2 & Playback Mode 2 , (NO DIG FILTER BLOCK AT ALL, not for MIC recording, nor for Playback)
	map.r.digital_filter_mode.ADCPF = 1;	// new code addition , ADCPF bit swith ("0" Mic  after ADC Output connected (recording mode) to the DIGITAL FILTER  BLOCK. ("1" Playback mode)  
	map.r.digital_filter_mode.PFSDO = 0;	// new code addition , ADC bit switch ("0" : 1st order HPF) connectedto the  Output. By bass DIGITAL block .
   	map.r.digital_filter_mode.PFDAC = 0b00;	// new code addition , (Input selector for DAC (not used in MIC), SDTI= Audio Serial Data Input Pin)
	update(Register::DigitalFilterMode);	// Writing the Audio Path :  NO DIGITAL BLOCK or DIG BLOCK FOR  MIC ,   Audio mode path : Playback mode /-Recording mode.
	
	// Restore original condition , LPF , OFF . same as when not using DIGITAL Programmable block
	map.r.digital_filter_select_2.LPF = 0;	  // LPF-Block, Coeffic Setting Enable (OFF-Default), When LPF bit is “0”, audio data passes the LPF block by 0dB gain. 
	update(Register::DigitalFilterSelect2);

	map.r.lpf_coefficient_0.l = 0x00;		// Pre-loading here LPF 6k  ,  1st Order from digital Block , Fc=6000 Hz,  fs = 48khz
	map.r.lpf_coefficient_1.h = 0x00;		// LPF bit is activated down, for all ALC digital modes.
	map.r.lpf_coefficient_2.l = 0x00;		// Writting reg to AK4951 , down with update....
	map.r.lpf_coefficient_3.h = 0x00;

	update(Register::LPFCoefficient0);		// Writing  pre-loaded  4 bytes  LPF  CoefFiecients 14 bits (FSA13..0,  FSB13..0
	update(Register::LPFCoefficient1);
	update(Register::LPFCoefficient2);
	update(Register::LPFCoefficient3);

	// Switch off all EQ 1,2,3,4,5
	map.r.digital_filter_select_3.EQ1 = 0;	  // EQ1 Coeffic Setting , (0: Disable-default, audio data passes  EQ1 block by 0dB gain). When EQ1="1”, the settings of E1A15-0, E1B15-0 and E1C15-0 bits are enabled
	map.r.digital_filter_select_3.EQ2 = 0;	  // EQ2 Coeffic Setting , (0: Disable-default, audio data passes  EQ2 block by 0dB gain). When EQ2="1”, the settings of E2A15-0, E2B15-0 and E2C15-0 bits are enabled
	map.r.digital_filter_select_3.EQ3 = 0;    // EQ3 Coeffic Setting , (0: Disable-default, audio data passes  EQ3 block by 0dB gain). When EQ3="1”, the settings of E3A15-0, E3B15-0 and E3C15-0 bits are enabled
	map.r.digital_filter_select_3.EQ4 = 0;    // EQ4 Coeffic Setting , (0: Disable-default, audio data passes  EQ4 block by 0dB gain). When EQ4="1”, the settings of E4A15-0, E4B15-0 and E4C15-0 bits are enabled
	map.r.digital_filter_select_3.EQ5 = 0;    // EQ5 Coeffic Setting , (0: Disable-default, audio data passes  EQ5 block by 0dB gain). When EQ5="1”, the settings of E5A15-0, E5B15-0 and E5C15-0 bits are enabled
	update(Register::DigitalFilterSelect3);

}

reg_t AK4951::read(const address_t reg_address) {
	const std::array<uint8_t, 1> tx { reg_address };
	std::array<uint8_t, 1> rx { 0x00 };
	bus.transmit(bus_address, tx.data(), tx.size());
	bus.receive(bus_address, rx.data(), rx.size());
	return rx[0];
}

void AK4951::update(const Register reg) {
	write(toUType(reg), map.w[toUType(reg)]);
}

void AK4951::write(const address_t reg_address, const reg_t value) {
	const std::array<uint8_t, 2> tx { reg_address, value };
	bus.transmit(bus_address, tx.data(), tx.size());
}

} /* namespace ak4951 */
} /* namespace asahi_kasei */
