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
#include "ui_transmitter.hpp"
#include "rtc_time.hpp"
#include "tonesets.hpp"
#include "message.hpp"
#include "volume.hpp"
#include "audio.hpp"

#define NUOPTIX_TONE_LENGTH	((TONES_SAMPLERATE * 0.049) - 1)	// 49ms

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
	
	tx_modes tx_mode { IDLE };
	
	void on_tuning_frequency_changed(rf::Frequency f);
	void transmit(bool setup);
	void on_tx_progress(const uint32_t progress, const bool done);
	
	uint32_t timecode { 0 };
	
	Text text_timecode {
		{ 10 * 8, 2 * 16, 9 * 8, 16 },
		"Timecode:"
	};
	
	NumberField number_timecode {
		{ 13 * 8, 3 * 16 },
		4,
		{ 1, 9999 },
		1,
		'0'
	};
	
	Text text_mod {
		{ 10 * 8, 5 * 16, 6 * 8, 16 },
		"Mod: "
	};
	
	ProgressBar progressbar {
		{ 16, 14 * 16, 208, 16 }
	};
	
	/*Button button_impro {
		{ 64, 184, 112, 40 },
		"IMPROVISE"
	};*/
	
	TransmitterView tx_view {
		16 * 16,
		10000,
		15
	};
	
	MessageHandlerRegistration message_handler_tx_progress {
		Message::ID::TXProgress,
		[this](const Message* const p) {
			const auto message = *reinterpret_cast<const TXProgressMessage*>(p);
			this->on_tx_progress(message.progress, message.done);
		}
	};
};

} /* namespace ui */

#endif/*__UI_NUOPTIX_H__*/
