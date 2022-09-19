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

#ifndef __UI_SSTVTX_H__
#define __UI_SSTVTX_H__

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_receiver.hpp"
#include "ui_navigation.hpp"
#include "ui_font_fixed_8x16.hpp"
#include "baseband_api.hpp"
#include "ui_transmitter.hpp"
#include "message.hpp"
#include "sstv.hpp"
#include "file.hpp"
#include "bmp.hpp"
#include "app_settings.hpp"

using namespace sstv;

namespace ui {

class SSTVTXView : public View {
public:
	SSTVTXView(NavigationView& nav);
	~SSTVTXView();
	
	SSTVTXView(const SSTVTXView&) = delete;
	SSTVTXView(SSTVTXView&&) = delete;
	SSTVTXView& operator=(const SSTVTXView&) = delete;
	SSTVTXView& operator=(SSTVTXView&&) = delete;

	void focus() override;
	void paint(Painter&) override;
	
	std::string title() const override { return "SSTV TX (beta)"; };
	
private:
	NavigationView& nav_;
	
	sstv_scanline scanline_buffer { };
	// app save settings
	std::app_settings 		settings { }; 		
	std::app_settings::AppSettings 	app_settings { };	

	bool file_error { false };
	File bmp_file { };
	bmp_header_t bmp_header { };
	std::vector<std::filesystem::path> bitmaps { };
	uint32_t scanline_counter { 0 };
	uint8_t pixels_buffer[320 * 3];		// 320 pixels @ 24bpp
	const sstv_mode * tx_sstv_mode { };
	
	uint8_t component_map[3] { };

	void read_boundary(uint8_t * buffer, uint32_t position, uint32_t length);
	void on_bitmap_changed(const size_t index);
	void on_mode_changed(const size_t index);
	void on_tuning_frequency_changed(rf::Frequency f);
	void start_tx();
	void prepare_scanline();
	
	Labels labels {
		{ { 1 * 8, 1 * 8 }, "File:", Color::light_grey() },
		{ { 1 * 8, 3 * 8 }, "Mode:", Color::light_grey() }
	};
	
	OptionsField options_bitmaps {
		{ 6 * 8, 1 * 8 },
		16,
		{ }
	};
	OptionsField options_modes {
		{ 6 * 8, 3 * 8 },
		16,
		{ }
	};
	
	ProgressBar progressbar {
		{ 16, 25 * 8, 208, 16 }
	};
	
	TransmitterView tx_view {
		16 * 16,
		10000,
		12
	};
	
	MessageHandlerRegistration message_handler_fifo_signal {
		Message::ID::RequestSignal,
		[this](const Message* const p) {
			const auto message = static_cast<const RequestSignalMessage*>(p);
			if (message->signal == RequestSignalMessage::Signal::FillRequest) {
				this->prepare_scanline();
			}
		}
	};
};

} /* namespace ui */

#endif/*__UI_SSTVTX_H__*/
