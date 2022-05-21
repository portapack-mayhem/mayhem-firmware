/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
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

#ifndef __UI_MICTX_H__
#define __UI_MICTX_H__

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "transmitter_model.hpp"
#include "tone_key.hpp"
#include "message.hpp"
#include "receiver_model.hpp"

namespace ui {

class MicTXView : public View {
public:
	MicTXView(NavigationView& nav);
	~MicTXView();

	MicTXView(const MicTXView&) = delete;
	MicTXView(MicTXView&&) = delete;
	MicTXView& operator=(const MicTXView&) = delete;
	MicTXView& operator=(MicTXView&&) = delete;
	
	void focus() override;
	
	// PTT: Enable through KeyEvent (only works with presses), disable by polling :(
	// This is the old "RIGHT BUTTON" method.
	/*
	bool on_key(const KeyEvent key) {
		if ((key == KeyEvent::Right) && (!va_enabled) && ptt_enabled) {
			set_tx(true);
			return true;
		} else
			return false;
	};
	*/

	std::string title() const override { return "Microphone"; };

private:
	static constexpr uint32_t sampling_rate = 1536000U;
	static constexpr uint32_t lcd_frame_duration = (256 * 1000UL) / 60;	// 1 frame @ 60fps in ms .8 fixed point  /60
	
	void update_vumeter();
	void do_timing();
	void set_tx(bool enable);
//	void on_tuning_frequency_changed(rf::Frequency f);
	void on_tx_progress(const bool done);
	void configure_baseband();

	void rxaudio(bool is_on);
	void on_headphone_volume_changed(int32_t v);

	void set_ptt_visibility(bool v);
	
	bool transmitting { false };
	bool va_enabled { false };
	bool ptt_enabled { true };
	bool rogerbeep_enabled { false };
	bool rx_enabled { false };
	uint32_t tone_key_index { };
	float mic_gain { 1.0 };
    uint8_t  ak4951_alc_GUI_selected { 0 }; 
	uint32_t audio_level { 0 };
	uint32_t va_level { };
	uint32_t attack_ms { };
	uint32_t decay_ms { };
	uint32_t attack_timer { 0 };
	uint32_t decay_timer { 0 };
	int32_t tx_gain { 47 };
    bool rf_amp { false };
	int32_t rx_lna { 32 };
	int32_t rx_vga { 32 };
	bool rx_amp { false };
	rf::Frequency tx_frequency { 0 };
	rf::Frequency rx_frequency { 0 };
	int32_t focused_ui { 2 };
	bool button_touch { false };

	//AM TX Stuff
	bool enable_am { false };
	bool enable_dsb { false };
	bool enable_usb { false };
	bool enable_lsb { false };

	
	Labels labels_WM8731 {		
		{ { 3 * 8, 1 * 8 }, "MIC-GAIN:", Color::light_grey() },
		{ { 3 * 8, 3 * 8 }, "F:", Color::light_grey() },
		{ { 15 * 8, 3 * 8 }, "BW:   FM kHz", Color::light_grey() },
		{ { 3 * 8, 5 * 8 }, "GAIN:", Color::light_grey() },
		{ {11 * 8, 5 * 8 }, "Amp:", Color::light_grey() },
		{ { 18 * 8, (5 * 8) }, "Mode:", Color::light_grey() },
		{ { 3 * 8, 8 * 8 }, "TX Activation:", Color::light_grey() },
		{ { 4 * 8, 10 * 8 }, "LVL:", Color::light_grey() },
		{ {12 * 8, 10 * 8 }, "ATT:", Color::light_grey() },
		{ {20 * 8, 10 * 8 }, "DEC:", Color::light_grey() },
		{ { 4 * 8, ( 13 * 8 ) - 2 }, "TONE KEY:", Color::light_grey() },
		{ { 7 * 8, 23 * 8 }, "VOL:", Color::light_grey() },
		{ {15 * 8, 23 * 8 }, "FM RXBW:", Color::light_grey() },
		{ {17 * 8, 25 * 8 }, "SQ:", Color::light_grey() },
		{ { 5 * 8, 25 * 8 }, "F:", Color::light_grey() },
		{ { 5 * 8, 27 * 8 }, "LNA:", Color::light_grey()},
		{ {12 * 8, 27 * 8 }, "VGA:", Color::light_grey()},
		{ {19 * 8, 27 * 8 }, "AMP:", Color::light_grey()}
	};
	Labels labels_AK4951 {
		{ { 3 * 8, 1 * 8 }, "MIC-GAIN:", Color::light_grey() },
		{ { 17 * 8, 1 * 8 }, "ALC", Color::light_grey() },
		{ { 3 * 8, 3 * 8 }, "F:", Color::light_grey() },
		{ { 15 * 8, 3 * 8 }, "BW:   FM kHz", Color::light_grey() },
		{ { 3 * 8, 5 * 8 }, "GAIN:", Color::light_grey() },
		{ {11 * 8, 5 * 8 }, "Amp:", Color::light_grey() },
		{ { 18 * 8, (5 * 8) }, "Mode:", Color::light_grey() },
		{ { 3 * 8, 8 * 8 }, "TX Activation:", Color::light_grey() },
		{ { 4 * 8, 10 * 8 }, "LVL:", Color::light_grey() },
		{ {12 * 8, 10 * 8 }, "ATT:", Color::light_grey() },
		{ {20 * 8, 10 * 8 }, "DEC:", Color::light_grey() },
		{ { 4 * 8, ( 13 * 8 ) - 2 }, "TONE KEY:", Color::light_grey() },
		{ { 7 * 8, 23 * 8 }, "VOL:", Color::light_grey() },
		{ {15 * 8, 23 * 8 }, "FM RXBW:", Color::light_grey() },
		{ {17 * 8, 25 * 8 }, "SQ:", Color::light_grey() },
		{ { 5 * 8, 25 * 8 }, "F:", Color::light_grey() },
		{ { 5 * 8, 27 * 8 }, "LNA:", Color::light_grey()},
		{ {12 * 8, 27 * 8 }, "VGA:", Color::light_grey()},
		{ {19 * 8, 27 * 8 }, "AMP:", Color::light_grey()}
	};

	VuMeter vumeter {
		{ 0 * 8, 1 * 8, 2 * 8, 33 * 8 },
		12,
		true
	};

	
	OptionsField options_gain {
		{ 12 * 8, 1 * 8 },
		4,
		{
			{ "x0.5", 5 },
			{ "x1.0", 10 },
			{ "x1.5", 15 },
			{ "x2.0", 20 }
		}
	};
	
	OptionsField options_ak4951_alc_mode {
		{ 20 * 8, 1 * 8 },				// Coordinates are: int:x (px), int:y (px)				
		11,
		{
			{ " OFF-20kHz", 0 },	// Nothing changed from ORIGINAL,keeping ALL programm. AK4951 Dig. block->OFF)
			{ "+12dB-6kHz", 1 },	// ALC-> on, (+12dB's) Auto Vol max + Wind Noise cancel + LPF 6kHz + Pre-amp Mic (+21dB=original)
			{ "+09dB-6kHz", 2 },	// ALC-> on, (+09dB's) Auto Vol max + Wind Noise cancel + LPF 6kHz + Pre-amp Mic (+21dB=original)
			{ "+06dB-6kHz", 3 },	// ALC-> on, (+06dB's) Auto Vol max + Wind Noise cancel + LPF 6kHz + Pre-amp Mic (+21dB=original)
			{ "+03dB-2kHz", 4 },	// ALC-> on, (+03dB's) Auto Vol max + Wind Noise cancel + LPF 3,5k + Pre-amp Mic (+21dB=original)+ EQ boosting ~<2kHz (f0~1k1,fb:1,7K, k=1,8)
			{ "+03dB-4kHz", 5 },	// ALC-> on, (+03dB's) Auto Vol max + Wind Noise cancel + LPF 4kHz + Pre-amp Mic (+21dB=original)+ EQ boosting ~<3kHz (f0~1k4,fb~2,4k, k=1,8)
			{ "+03dB-6kHz", 6 },	// ALC-> on, (+03dB's) Auto Vol max + Wind Noise cancel + LPF 6kHz + Pre-amp Mic (+21dB=original)
			{ "+00dB-6kHz", 7 },	// ALC-> on, (+00dB's) Auto Vol max + Wind Noise cancel + LPF 6kHz + Pre-amp Mic (+21dB=original)
			{ "-03dB-6kHz", 8 },	// ALC-> on, (-03dB's) Auto Vol max + Wind Noise cancel + LPF 6kHz + Pre-amp Mic (+21dB=original)
			{ "-06dB-6kHz", 9 },	// ALC-> on, (-06dB's) Auto Vol max + Wind Noise cancel + LPF 6kHz + Pre-amp Mic (+21dB=original)
			{ "-09dB-6kHz", 10 },	// ALC-> on, (-09dB's) Auto Vol max + Wind Noise cancel + LPF 6kHz - Pre-amp MIC -3dB (18dB's) 
			{ "-12dB-6kHz", 11 },	// ALC-> on, (-12dB's) Auto Vol max + Wind Noise cancel + LPF 6kHz - Pre-amp MIC -6dB (15dB's) 	
		}
	};

	FrequencyField field_frequency {
		{ 5 * 8, 3 * 8 },
	};
	NumberField field_bw {
		{ 18 * 8, 3 * 8 },
		3,
		{ 0, 150 },
		1,
		' '
	};
	
	NumberField field_rfgain {
		{ 8 * 8, 5 * 8 },
		2,
		{ 0, 47 },
		1,
		' '
	};
	NumberField field_rfamp {
		{ 15 * 8, 5 * 8 },
		2,
		{ 0, 14 },
		14,
		' '
	};
	
	OptionsField options_mode {
		{ 24 * 8, 5 * 8 },
		3,
		{
			{ "FM", 0 },
			{ "AM", 1 },
			{ "USB", 2 },
			{ "LSB", 3 },
			{ "DSB", 4 }
		}
	};
	/*
	Checkbox check_va {
		{ 3 * 8, (10 * 8) - 4 },
		7,
		"Voice activation",
		false
	};
	*/

	OptionsField field_va {
		{ 17 * 8, 8 * 8 },
		3,
		{
			{" OFF", 0},
			{" PTT", 1},
			{"AUTO", 2}
		}
	};

	NumberField field_va_level {
		{ 8 * 8, 10 * 8 },
		3,
		{ 0, 255 },
		2,
		' '
	};
	NumberField field_va_attack {
		{ 16 * 8, 10 * 8 },
		3,
		{ 0, 999 },
		20,
		' '
	};
	NumberField field_va_decay {
		{ 24 * 8, 10 * 8 },
		4,
		{ 0, 9999 },
		100,
		' '
	};
	
	OptionsField options_tone_key {
		{ 10 * 8, ( 15 * 8 ) - 2 },
		23,
		{ }
	};

	Checkbox check_rogerbeep {
		{ 3 * 8, ( 16 * 8 ) + 4 },
		10,
		"Roger beep",
		false
	};

	Checkbox check_rxactive {
		{ 3 * 8, ( 21 * 8 ) - 4 },
		8,
		"RX audio listening",
		false
	};

	NumberField field_volume {
		{ 11* 8, 23 * 8 },
		2,
		{ 0, 99 },
		1,
		' ',
	};
	
	OptionsField field_rxbw {
	       { 23* 8, 23 * 8},
	       3,
	       {
	       		{"8k5", 0},
	       		{"11k", 1},
	       		{"16k", 2}
	       	}
	};
	
	NumberField field_squelch {
		{ 20 * 8, 25 * 8 },
		2,
		{ 0, 99 },
		1,
		' ',
	};

	FrequencyField field_rxfrequency {
		{ 7 * 8, 25 * 8 },
	};

	NumberField field_rxlna {
		{ 9 * 8, 27 * 8 },
		2,
		{ 0, 40 },
		8,
		' ',
	};

	NumberField field_rxvga {
		{ 16 * 8, 27 * 8 },
		2,
		{ 0, 62 },
		2,
		' ',
	};

	NumberField field_rxamp {
		{ 23 * 8, 27 * 8 },
		1,
		{ 0, 1 },
		1,
		' ',
	};

	Button tx_button {
		{ 10 * 8, 30 * 8, 10 * 8, 5 * 8 },
		"TX",
		true
	};


	MessageHandlerRegistration message_handler_lcd_sync {
		Message::ID::DisplayFrameSync,
		[this](const Message* const) {
			this->do_timing();
			this->update_vumeter();
		}
	};
	
	MessageHandlerRegistration message_handler_audio_level {
		Message::ID::AudioLevelReport,
		[this](const Message* const p) {
			const auto message = static_cast<const AudioLevelReportMessage*>(p);
			this->audio_level = message->value;
		}
	};
	
	MessageHandlerRegistration message_handler_tx_progress {
		Message::ID::TXProgress,
		[this](const Message* const p) {
			const auto message = *reinterpret_cast<const TXProgressMessage*>(p);
			this->on_tx_progress(message.done);
		}
	};
};

} /* namespace ui */

#endif/*__UI_MICTX_H__*/
