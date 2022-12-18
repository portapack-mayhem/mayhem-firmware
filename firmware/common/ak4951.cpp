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

void AK4951::microphone_enable() {
// map.r.digital_mic.DMIC = 0;
// update(Register::DigitalMic);
	
	const uint_fast8_t mgain = 0b0111;
	map.r.signal_select_1.MGAIN20 = mgain & 7;
	map.r.signal_select_1.PMMP = 1;
	map.r.signal_select_1.MPSEL = 1;	// MPWR2 pin
	map.r.signal_select_1.MGAIN3 = (mgain >> 3) & 1;
	update(Register::SignalSelect1);

	map.r.signal_select_2.INL = 0b01;	// Lch input signal = LIN2
	map.r.signal_select_2.INR = 0b01;	// Rch input signal = RIN2
	map.r.signal_select_2.MICL = 0;		// MPWR = 2.4V
	update(Register::SignalSelect2);

// map.r.r_ch_mic_gain_setting.MGR = 0x80;	// Microphone sensitivity correction = 0dB.
// update(Register::RchMicGainSetting);
/*
	map.r.timer_select.FRN = ?;
	map.r.timer_select.FRATT = ?;
	map.r.timer_select.ADRST = 0b??;
	update(Register::TimerSelect);

	map.r.alc_timer_select. = ?;
	update(Register::ALCTimerSelect);
	map.r.alc_mode_control_1. = ?;
	map.r.alc_mode_control_1.ALC = 1;
	update(Register::ALCModeControl1);

	map.r.alc_mode_control_2.REF = ?;
	update(Register::ALCModeControl2);
*/
// map.r.l_ch_input_volume_control.IV = 0xe1;
// update(Register::LchInputVolumeControl);
// map.r.r_ch_input_volume_control.IV = 0xe1;
// update(Register::RchInputVolumeControl);
/*
	map.r.auto_hpf_control.STG = 0b00;
	map.r.auto_hpf_control.SENC = 0b011;
	map.r.auto_hpf_control.AHPF = 0;
	update(Register::AutoHPFControl);
*/
	map.r.digital_filter_select_1.HPFAD = 1;	// HPF1 (after ADC) = on
	map.r.digital_filter_select_1.HPFC = 0b11;	// 2336.8 Hz @ fs=48k
	update(Register::DigitalFilterSelect1);
/*
	map.r.digital_filter_select_2.HPF = 0;
	map.r.digital_filter_select_2.LPF = 0;
	map.r.digital_filter_select_2.FIL3 = 0;
	map.r.digital_filter_select_2.EQ0 = 0;
	map.r.digital_filter_select_2.GN = 0b00;
	update(Register::DigitalFilterSelect2);

	map.r.digital_filter_select_3.EQ1 = 0;
	map.r.digital_filter_select_3.EQ2 = 0;
	map.r.digital_filter_select_3.EQ3 = 0;
	map.r.digital_filter_select_3.EQ4 = 0;
	map.r.digital_filter_select_3.EQ5 = 0;
	update(Register::DigitalFilterSelect3);
*/
	map.r.digital_filter_mode.PFSDO = 0;	// ADC (+ 1st order HPF) Output
	map.r.digital_filter_mode.ADCPF = 1;	// ADC Output (default)
	update(Register::DigitalFilterMode);

	// ... Set coefficients ...

	map.r.power_management_1.PMADL = 1;		// ADC Lch = Lch input signal
	map.r.power_management_1.PMADR = 1;		// ADC Rch = Rch input signal
	map.r.power_management_1.PMPFIL = 0;	// Programmable filter unused, routed around.
	update(Register::PowerManagement1);

	// 1059/fs, 22ms @ 48kHz
	chThdSleepMilliseconds(22);
}

void AK4951::microphone_disable() {
	map.r.power_management_1.PMADL = 0;
	map.r.power_management_1.PMADR = 0;
	map.r.power_management_1.PMPFIL = 0;
	update(Register::PowerManagement1);

	map.r.alc_mode_control_1.ALC = 0;
	update(Register::ALCModeControl1);
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
