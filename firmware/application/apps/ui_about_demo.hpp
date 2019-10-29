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

#ifndef __UI_ABOUT_H__
#define __UI_ABOUT_H__

#include "ui_widget.hpp"
#include "ui_menu.hpp"
#include "ui_navigation.hpp"
#include "transmitter_model.hpp"
#include "baseband_api.hpp"

#include <cstdint>

namespace ui {

class AboutView : public View {
public:
	AboutView(NavigationView& nav);
	~AboutView();
	
	void on_show() override;
	void focus() override;

private:
	void ym_init();
	void update();
	void render_video();
	void render_audio();
	void draw_demoglyph(ui::Point p, char ch, ui::Color * pal);
	uint16_t debug_cnt = 0;
	
	typedef struct ymreg_t {
		uint8_t value;
		uint8_t cnt;
		uint16_t ptr;
		bool same;
	} ymreg_t;
	
	uint16_t headphone_vol = 5 << 2;
	
	ymreg_t ym_regs[14];
	uint16_t ym_frames;
	uint16_t ym_frame;
	uint8_t drum = 0;
	uint16_t ym_osc_cnt[3];
	uint32_t ym_rng = 1;
	uint16_t ym_noise_cnt;
	uint8_t ym_env_att, ym_env_hold, ym_env_alt, ym_env_holding, ym_env_vol;
	int8_t ym_env_step;
	uint16_t ym_env_cnt;
	uint8_t ym_osc_out[3];
	uint8_t ym_ch[3];
	uint8_t ym_out;
	uint16_t ym_sample_cnt = 0;
	
	int8_t ym_buffer[1024];

	uint8_t refresh_cnt;
	ui::Color paletteA[16];
	ui::Color paletteB[16];
	ui::Color * framebuffer;
	uint32_t phase = 0;
	uint8_t copperbars[5] = { 0 };
	uint8_t copperbuffer[72] = { 0 };
	
	uint8_t anim_state = 0;
	uint8_t credits_index = 0;
	uint16_t credits_timer = 0;
	
	int16_t ofx = -180, ofy = -24;

	const uint8_t char_map[64] = { 0xFF, 27, 46, 0xFF, 0xFF, 0xFF, 28, 45,
									58, 59, 0xFF, 43, 40, 57, 26, 42,
									29, 30, 31, 32, 33, 34, 35, 36,
									37, 38, 41, 0xFF, 0xFF, 0xFF, 0xFF, 44,
									0xFF, 0, 1, 2, 3, 4, 5, 6,
									7, 8, 9, 10, 11, 12, 13, 14,
									15, 16, 17, 18, 19, 20, 21, 22,
									23, 24, 25, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
	
	const uint8_t copperluma[16] = { 8,7,6,5,4,3,2,1,1,2,3,4,5,6,7,8 };
	const uint8_t coppercolor[5][3] = {	{ 255,0,0 },
										{ 0,255,0 },
										{ 0,0,255 },
										{ 255,0,255 },
										{ 255,255,0 } };
	
	typedef struct credits_t {
		char role[12];
		char name[12];
		bool change;
	} credits_t;
	
	//								  0123456789A		 0123456789A
	const credits_t credits[10] = { {"GURUS",			"J. BOONE", 		false},
									{"GURUS",			"M. OSSMANN", 		true},
									{"BUGS",			"FURRTEK", 			true},
									{"RDS WAVE",		"C. JACQUET", 		true},
									{"POCSAG RX",		"T. SAILER",		false},
									{"POCSAG RX",		"E. OENAL",			true},
									{"XYLOS DATA",		"CLX", 				true},
									{"GREETS TO",		"SIGMOUNTE", 		false},
									{"GREETS TO",		"WINDYOONA", 		true},
									{"THIS MUSIC",		"BIG ALEC", 		true}
									};

	Text text_title {
		{ 100, 32, 40, 16 },
		"About",
	};

	Text text_firmware {
		{ 0, 236, 240, 16 },
		"Version   " VERSION_STRING,
	};

	Text text_cpld_hackrf {
		{ 0, 252, 11*8, 16 },
		"HackRF CPLD",
	};

	Text text_cpld_hackrf_status {
		{ 240 - 3*8, 252, 3*8, 16 },
		"???"
	};

	Button button_ok {
		{ 72, 272, 96, 24 },
		"OK"
	};
	
	MessageHandlerRegistration message_handler_update {
		Message::ID::DisplayFrameSync,
		[this](const Message* const) {
			this->update();
		}
	};
	
	MessageHandlerRegistration message_handler_fifo_signal {
		Message::ID::FIFOSignal,
		[this](const Message* const p) {
			const auto message = static_cast<const FIFOSignalMessage*>(p);
			if (message->signaltype == 1) {
				this->render_audio();
				baseband::set_fifo_data(ym_buffer);
			}
		}
	};
	
};

} /* namespace ui */

#endif/*__UI_ABOUT_H__*/
