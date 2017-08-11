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

#include "wm8731.hpp"
#include "utility.hpp"

namespace wolfson {
namespace wm8731 {

void WM8731::configure_interface_i2s_slave() {
	write(DigitalAudioInterfaceFormat {
		.format = 2,
		.iwl = 0,
		.lrp = 0,
		.lrswap = 0,
		.ms = 0,
		.bclkinv = 0,
		.reserved0 = 0,
	});
}

void WM8731::configure_interface_i2s_master() {
	write(DigitalAudioInterfaceFormat {
		.format = 2,
		.iwl = 0,
		.lrp = 0,
		.lrswap = 0,
		.ms = 1,
		.bclkinv = 0,
		.reserved0 = 0,
	});
}

void WM8731::init() {
	reset();

	write(PowerDownControl {
		.lineinpd = 1,	
		.micpd = 0,
		.adcpd = 0,
		.dacpd = 0,
		.outpd = 0,
		.oscpd = 1,
		.clkoutpd = 1,
		.poweroff = 0,
		.reserved0 = 0,
	});

	// write(SamplingControl {
	// 	.usb_normal = 0,
	// 	.bosr = 0,
	// 	.sr = 0,
	// 	.clkidiv2 = 0,
	// 	.clkodiv2 = 0,
	// 	.reserved0 = 0,
	// });

	configure_interface_i2s_slave();

	write(DigitalAudioPathControl {
		.adchpd = 0,
		.deemp = 0,
		.dacmu = 0,
		.hpor = 0,
		.reserved0 = 0,
	});

	write(AnalogAudioPathControl {
		.micboost = 1,	// Enable 20dB boost
		.mutemic = 0,	// Disable mute (unmute)
		.insel = 1,		// Microphone input to ADC
		.bypass = 0,
		.dacsel = 1,
		.sidetone = 0,
		.sideatt = 0,
		.reserved0 = 0,
	});

	write(ActiveControl {
		.active = 1,
		.reserved0 = 0,
	});

	//set_line_in_volume(0.0_dB);
	headphone_mute();
}

bool WM8731::detected() {
	return reset();
}

bool WM8731::reset() {
	return write(0x0f, 0);
}

bool WM8731::write(const Register reg) {
	return write(toUType(reg), map.w[toUType(reg)]);
}

bool WM8731::write(const address_t reg_address, const reg_t value) {
	const uint16_t word = (reg_address << 9) | value;
	const std::array<uint8_t, 2> values {
		static_cast<uint8_t>(word >> 8),
		static_cast<uint8_t>(word & 0xff),
	};
	return bus.transmit(bus_address, values.data(), values.size());
}

uint32_t WM8731::reg_read(const size_t reg_address) {
	return map.w[reg_address];
}

void WM8731::write(const LeftLineIn value) {
	map.r.left_line_in = value;
	write(Register::LeftLineIn);
}

void WM8731::write(const RightLineIn value) {
	map.r.right_line_in = value;
	write(Register::RightLineIn);
}

void WM8731::write(const LeftHeadphoneOut value) {
	map.r.left_headphone_out = value;
	write(Register::LeftHeadphoneOut);
}

void WM8731::write(const RightHeadphoneOut value) {
	map.r.right_headphone_out = value;
	write(Register::RightHeadphoneOut);
}

void WM8731::write(const AnalogAudioPathControl value) {
	map.r.analog_audio_path_control = value;
	write(Register::AnalogAudioPathControl);
}

void WM8731::write(const DigitalAudioPathControl value) {
	map.r.digital_audio_path_control = value;
	write(Register::DigitalAudioPathControl);
}

void WM8731::write(const PowerDownControl value) {
	map.r.power_down_control = value;
	write(Register::PowerDownControl);
}

void WM8731::write(const DigitalAudioInterfaceFormat value) {
	map.r.digital_audio_interface_format = value;
	write(Register::DigitalAudioInterfaceFormat);
}

void WM8731::write(const SamplingControl value) {
	map.r.sampling_control = value;
	write(Register::SamplingControl);
}

void WM8731::write(const ActiveControl value) {
	map.r.active_control = value;
	write(Register::ActiveControl);
}

} /* namespace wm8731 */
} /* namespace wolfson */
