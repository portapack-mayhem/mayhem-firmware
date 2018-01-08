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

#include <ch.h>

#include "demofont.hpp"
#include "ymdata.hpp"

#include "cpld_update.hpp"
#include "portapack.hpp"
#include "audio.hpp"
#include "event_m0.hpp"

#include "ui_about.hpp"
#include "touch.hpp"
#include "sine_table.hpp"

#include "portapack_shared_memory.hpp"
#include "portapack_persistent_memory.hpp"
#include "lpc43xx_cpp.hpp"

#include <math.h>
#include <cstring>

using namespace lpc43xx;
using namespace portapack;

namespace ui {
	
void AboutView::on_show() {
	transmitter_model.set_tuning_frequency(1337000000);		// TODO: Change
	transmitter_model.set_baseband_configuration({
		.mode = 0,
		.sampling_rate = 1536000,
		.decimation_factor = 1,
	});
	transmitter_model.set_rf_amp(true);
	transmitter_model.set_lna(40);
	transmitter_model.set_vga(40);
	transmitter_model.set_baseband_bandwidth(1750000);
	transmitter_model.enable();
	
	baseband::set_audiotx_data(32, 50, false, 0);
	
	//audio::headphone::set_volume(volume_t::decibel(0 - 99) + audio::headphone::volume_range().max);
}

void AboutView::render_video() {
	uint8_t p, r, luma, chroma, cy;
	ui::Color cc;
	char ch;
	float s;
	
	// Send framebuffer to LCD. Gotta go fast !
	display.render_box({30, 112}, {180, 72}, framebuffer);

	// Clear framebuffer to black
	memset(framebuffer, 0, 180 * 72 * sizeof(ui::Color));

	// Drum hit palette animation
	if (drum > 1) drum--;

	// Render copper bars from Y buffer
	for (p = 0; p < 72; p++) {
		luma = copperbuffer[p] & 0x0F;		// 0 is transparent
		if (luma) {
			chroma = copperbuffer[p]>>4;
			cc = ui::Color(std::min((coppercolor[chroma][0]/luma)*drum,255), std::min((coppercolor[chroma][1]/luma)*drum,255), std::min((coppercolor[chroma][2]/luma)*drum,255));
			for (r = 0; r < 180; r++)
				framebuffer[(p*180)+r] = cc;
		}
	}
	
	// Scroll in/out state machine
	if (anim_state == 0) {
		// Scroll in
		if (ofy < 8) {
			ofy++;
			anim_state = 0;
		} else {
			anim_state = 1;
		}
		if (ofx < (int16_t)(180 - (strlen(credits[credits_index].name) * 16) - 8)) {
			ofx += 8;
			anim_state = 0;
		}
	} else if (anim_state == 1) {
		// Just wait
		if (credits_timer == (30*3)) {
			credits_timer = 0;
			anim_state = 2;
		} else {
			credits_timer++;
		}
	} else {
		// Scroll out
		if (credits[credits_index].change == true) {
			if (ofy > -24) {
				ofy--;
				anim_state = 2;
			} else {
				anim_state = 0;
			}
		} else {
			anim_state = 0;
		}
		if (ofx < 180) {
			ofx += 8;
			anim_state = 2;
		}
	
		// Switch to next text
		if (anim_state == 0) {
			if (credits_index == 9)
				credits_index = 0;
			else
				credits_index++;
			ofx = -(strlen(credits[credits_index].name) * 16) - 16;
		}
	}
	
	// Sine text ("role")
	p = 0;
	while ((ch = credits[credits_index].role[p])) {
		draw_demoglyph({(ui::Coord)(8+(p*16)), (ui::Coord)(ofy+(sine_table_f32[((p*16)+(phase>>5))&0xFF] * 8))}, ch, paletteA);
		p++;
	}
		
	// Scroll text (name)
	p = 0;
	while ((ch = credits[credits_index].name[p])) {
		draw_demoglyph({(ui::Coord)(ofx+(p*16)), 56}, ch, paletteB);
		p++;
	}
	
	// Clear bars Y buffer
	memset(copperbuffer, 0, 72);
		
	// Render bars to Y buffer
	for (p = 0; p < 5; p++) {
		cy = copperbars[p];
		for (r = 0; r < 16; r++)
			copperbuffer[cy+r] = copperluma[r] + (p<<4);
	}
	
	// Animate bars positions
	for (p = 0; p < 5; p++) {
	  	s = sine_table_f32[((p*32)+(phase/24))&0xFF];
		s += sine_table_f32[((p*16)+(phase/35))&0xFF];
		copperbars[p] = 28+(uint8_t)(s * 14);
	}
	
	phase += 128;
}

void AboutView::draw_demoglyph(ui::Point p, char ch, ui::Color * pal) {
	uint8_t x, y, c, cl, cr;
	uint16_t che;
	int16_t lbx, il;

	// Map ASCII to font bitmap
	if ((ch >= 32) || (ch < 96))
		che = char_map[ch-32];
	else
		che = 0xFF;
		
	if (che < 0xFF) {
		che = (che * 128) + 48;		// Start in bitmap

		il = (180 * p.y) + p.x;		// Start il framebuffer
		
		for (y = 0; y < 16; y++) {
			if (p.y + y >= 72) break;					// Over bottom of framebuffer, abort
			if (p.y + y >= 0) {
				for (x = 0; x < 8; x++) {
					c = demofont_bin[x+(y*8)+che];		// Split byte in 2 4BPP pixels
					cl = c >> 4;
					cr = c & 0x0F;
					lbx = p.x + (x*2);
					if (cl && (lbx < 180) && (lbx >= 0)) framebuffer[il] = pal[cl];
					lbx++;
					il++;
					if (cr && (lbx < 180) && (lbx >= 0)) framebuffer[il] = pal[cr];
					il++;
				}
				il += 180-16;
			} else {
				il += 180;
			}
		}
	}
}

void AboutView::render_audio() {
	uint8_t i, ymdata;
	uint16_t ym_render_cnt;
	
	// This is heavily inspired by MAME's ay8910.cpp and the YM2149's datasheet
	
	// Render 1024 music samples
	for (ym_render_cnt = 0; ym_render_cnt < 1024; ym_render_cnt++) {

		// Update registers at 48000/960 = 50Hz
		if (ym_sample_cnt == 0) {
			// "Decompress" on the fly and update YM registers
			for (i = 0; i < 14; i++) {
				if (!ym_regs[i].cnt) {
					// New run
					ymdata = ymdata_bin[ym_regs[i].ptr++];
					ym_regs[i].cnt = ymdata & 0x7F;
					if (ymdata & 0x80) {
						ym_regs[i].same = true;
						ym_regs[i].value = ymdata_bin[ym_regs[i].ptr++];
					} else {
						ym_regs[i].same = false;
					}
					// Detect drum on channel B
					if (i == 3)
						if (ym_regs[3].value > 2) drum = 4;
				}
				if (ym_regs[i].same == false) {
					ym_regs[i].value = ymdata_bin[ym_regs[i].ptr++];
					if (i == 13) {
						// Update envelope attributes
						ym_env_att = (ym_regs[13].value & 4) ? 0x1F : 0x00;
						if (!(ym_regs[13].value & 8)) {
							ym_env_hold = 1;
							ym_env_alt = ym_env_att;
						} else {
							ym_env_hold = ym_regs[13].value & 1;
							ym_env_alt = ym_regs[13].value & 2;
						}
						// Reset envelope counter
						ym_env_step = 0x1F;
						ym_env_holding = 0;
						ym_env_vol = (ym_env_step ^ ym_env_att);		
					}
				}
				ym_regs[i].cnt--;
			}
			ym_frame++;
		}
		
		// Square wave oscillators
		// 2457600/16/48000 = 3.2, but 4 sounds better than 3...
		for (i = 0; i < 3; i++) {
			ym_osc_cnt[i] += 4;
			if (ym_osc_cnt[i] >= (ym_regs[i*2].value | ((ym_regs[(i*2)+1].value & 0x0f) << 8))) {
				ym_osc_cnt[i] = 0;
				ym_osc_out[i] ^= 1;
			}
		}
		
		// Noise generator
		ym_noise_cnt += 4;
		if (ym_noise_cnt >= ((ym_regs[6].value & 0x1F) * 2)) {
			ym_noise_cnt = 0;
			ym_rng ^= (((ym_rng & 1) ^ ((ym_rng >> 3) & 1)) << 17);
			ym_rng >>= 1;
		}
		
		// Mix tones and noise
		for (i = 0; i < 3; i++)
			ym_ch[i] = (ym_osc_out[i] | ((ym_regs[7].value >> i) & 1)) & ((ym_rng & 1) | ((ym_regs[7].value >> (i + 3)) & 1));
			
		// Envelope generator
		if (!ym_env_holding) {
			ym_env_cnt += 8;
			if (ym_env_cnt >= (ym_regs[11].value | (ym_regs[12].value<<8))) {
				ym_env_cnt = 0;
				ym_env_step--;
				if (ym_env_step < 0) {
					if (ym_env_hold) {
						if (ym_env_alt)
							ym_env_att ^= 0x1F;
						ym_env_holding = 1;
						ym_env_step = 0;
					} else {
						if (ym_env_alt && (ym_env_step & 0x20))
							ym_env_att ^= 0x1F;
						ym_env_step &= 0x1F;
					}
				}
			}
		}
		ym_env_vol = (ym_env_step ^ ym_env_att);

		ym_out = 0;
		for (i = 0; i < 3; i++) {
			if (ym_regs[i + 8].value & 0x10) {
				// Envelope mode
				ym_out += (ym_ch[i] ? ym_env_vol : 0);
			} else {
				// Fixed mode
				ym_out += (ym_ch[i] ? (ym_regs[i + 8].value & 0x0F) : 0);
			}
		}
	
		ym_buffer[ym_render_cnt] = (ym_out * 2) - 45;

		if (ym_sample_cnt < 960) {
			ym_sample_cnt++;
		} else {
			ym_sample_cnt = 0;
		}
		
		// Loop
		if (ym_frame == ym_frames) ym_init();
	}
}

void AboutView::update() {
	if (framebuffer) {
		// Update 1 out of 2 frames, 60Hz is very laggy
		if (refresh_cnt & 1) render_video();
		refresh_cnt++;
	}
	
	// Slowly increase volume to avoid jumpscare
	if (headphone_vol < (70 << 2)) {
		audio::headphone::set_volume(volume_t::decibel((headphone_vol/4) - 99) + audio::headphone::volume_range().max);
		headphone_vol++;
	}
}

void AboutView::ym_init() {
	uint8_t reg;
	
	for (reg = 0; reg < 14; reg++) {
		ym_regs[reg].cnt = 0;
		// Pick up start pointers for each YM registers RLE blocks
		ym_regs[reg].ptr = ((uint16_t)(ymdata_bin[(reg*2)+3])<<8) + ymdata_bin[(reg*2)+2];
		ym_regs[reg].same = false;	// Useless ?
		ym_regs[reg].value = 0;		// Useless ?
	}
	
	ym_frame = 0;
}

AboutView::AboutView(
	NavigationView& nav
)
{
	uint8_t p, c;
	
	baseband::run_image(portapack::spi_flash::image_tag_audio_tx);
	
	add_children({ {
		&text_title,
		&text_firmware,
		&text_cpld_hackrf,
		&text_cpld_hackrf_status,
		&button_ok,
	} });
	
	if( cpld_hackrf_verify_eeprom() ) {
		text_cpld_hackrf_status.set(" OK");
	} else {
		text_cpld_hackrf_status.set("BAD");
	}
	
	// Politely ask for about 26kB
	framebuffer = (ui::Color *)chHeapAlloc(0x0, 180 * 72 * sizeof(ui::Color));
	
	if (framebuffer) {
		memset(framebuffer, 0, 180 * 72 * sizeof(ui::Color));
		
		// Copy original font palette
		c = 0;
		for (p = 0; p < 48; p+=3)
			paletteA[c++] = ui::Color(demofont_bin[p], demofont_bin[p+1], demofont_bin[p+2]);
		
		// Increase red in another one
		c = 0;
		for (p = 0; p < 48; p+=3)
			paletteB[c++] = ui::Color(std::min(demofont_bin[p]+64, 255), demofont_bin[p+1], demofont_bin[p+2]);
	}

	// Init YM synth
	ym_frames = ((uint16_t)(ymdata_bin[1])<<8) + ymdata_bin[0];
	ym_init();
	

	button_ok.on_select = [this,&nav](Button&){
		if (framebuffer) chHeapFree(framebuffer);	// Do NOT forget this
		nav.pop();
	};
}

AboutView::~AboutView() {
	transmitter_model.disable();
	baseband::shutdown();
}

void AboutView::focus() {
	button_ok.focus();
}

} /* namespace ui */
