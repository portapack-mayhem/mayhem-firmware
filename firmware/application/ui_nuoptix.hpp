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

#ifndef __UI_NUOPTIX_H__
#define __UI_NUOPTIX_H__

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_font_fixed_8x16.hpp"
#include "baseband_api.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "time.hpp"
#include "message.hpp"
#include "volume.hpp"
#include "audio.hpp"

#define DTMF_DELTA_COEF (43.691)		// (65536*1024)/1536000
#define DTMF_C0			(uint32_t)(1209 * DTMF_DELTA_COEF)
#define DTMF_C1			(uint32_t)(1336 * DTMF_DELTA_COEF)
#define DTMF_C2			(uint32_t)(1477 * DTMF_DELTA_COEF)
#define DTMF_C3			(uint32_t)(1633 * DTMF_DELTA_COEF)
#define DTMF_R0			(uint32_t)(697 * DTMF_DELTA_COEF)
#define DTMF_R1			(uint32_t)(770 * DTMF_DELTA_COEF)
#define DTMF_R2			(uint32_t)(852 * DTMF_DELTA_COEF)
#define DTMF_R3			(uint32_t)(941 * DTMF_DELTA_COEF)

#define NUOPTIX_TONE_LENGTH 75264		// 1536000*0.049s

namespace ui {
	
class NuoptixView : public View {
public:
	NuoptixView(NavigationView& nav);
	~NuoptixView();

	void focus() override;
	
	std::string title() const override { return "Nuoptix sync"; };
	
private:
	enum tx_modes {
		IDLE = 0,
		NORMAL,
		IMPROVISE
	};
	
	tx_modes tx_mode = IDLE;
	
	// 0123456789ABCD#*
	const uint32_t dtmf_deltas[16][2] = {
		{ DTMF_C1, DTMF_R3 },
		{ DTMF_C0, DTMF_R0 },
		{ DTMF_C1, DTMF_R0 },
		{ DTMF_C2, DTMF_R0 },
		{ DTMF_C0, DTMF_R1 },
		{ DTMF_C1, DTMF_R1 },
		{ DTMF_C2, DTMF_R1 },
		{ DTMF_C0, DTMF_R2 },
		{ DTMF_C1, DTMF_R2 },
		{ DTMF_C2, DTMF_R2 },
		{ DTMF_C3, DTMF_R0 },
		{ DTMF_C3, DTMF_R1 },
		{ DTMF_C3, DTMF_R2 },
		{ DTMF_C3, DTMF_R3 },
		{ DTMF_C2, DTMF_R3 },
		{ DTMF_C0, DTMF_R3 }
	};
	
	void on_tuning_frequency_changed(rf::Frequency f);
	void transmit(bool setup);
	
	uint32_t timecode;
	rtc::RTC datetime;
	
	FrequencyField field_frequency {
		{ 1 * 8, 4 },
	};
	
	NumberField number_bw {
		{ 13 * 8, 4 },
		3,
		{1, 150},
		1,
		' '
	};
	
	Text text_kHz {
		{ 16 * 8, 4, 3 * 8, 16 },
		"kHz"
	};
	
	Text text_timecode {
		{ 10 * 8, 32, 9 * 8, 16 },
		"Timecode:"
	};
	
	NumberField number_timecode {
		{ 13 * 8, 48 },
		4,
		{ 1, 9999 },
		1,
		'0'
	};
	
	Text text_mod {
		{ 10 * 8, 80, 6 * 8, 16 },
		"Mod: "
	};
	
	ProgressBar pbar {
		{ 16, 236, 208, 16 }
	};
	
	Button button_tx {
		{ 64, 128, 112, 40 },
		"TX"
	};
	
	Button button_impro {
		{ 64, 184, 112, 40 },
		"IMPROVISE"
	};
	
	Button button_exit {
		{ 88, 270, 64, 32 },
		"Exit"
	};
	
	MessageHandlerRegistration message_handler_tx_done {
		Message::ID::TXDone,
		[this](const Message* const p) {
			const auto message = *reinterpret_cast<const TXDoneMessage*>(p);
			if (message.done)
				transmit(false);
			else
				pbar.set_value(message.progress);
		}
	};
};

} /* namespace ui */

#endif/*__UI_NUOPTIX_H__*/
