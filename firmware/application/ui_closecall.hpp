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

#include "receiver_model.hpp"

#include "spectrum_color_lut.hpp"

#include "ui_receiver.hpp"
#include "ui_font_fixed_8x16.hpp"

namespace ui {

#define CC_SLICE_WIDTH	2500000		// Radio bandwidth
#define CC_BIN_NB		256			// Total power bins (skip 4 at center, 2*6 on sides)
#define CC_BIN_NB_NO_DC	(CC_BIN_NB - 16)
#define CC_BIN_WIDTH	(CC_SLICE_WIDTH / CC_BIN_NB)

class CloseCallView : public View {
public:
	CloseCallView(NavigationView& nav);
	~CloseCallView();
	
	CloseCallView(const CloseCallView&) = delete;
	CloseCallView(CloseCallView&&) = delete;
	CloseCallView& operator=(const CloseCallView&) = delete;
	CloseCallView& operator=(CloseCallView&&) = delete;
	
	void on_show() override;
	void on_hide() override;
	void focus() override;
	std::string title() const override { return "Close Call"; };

private:
	const Style style_grey {		// For labels and lost signal
		.font = font::fixed_8x16,
		.background = Color::black(),
		.foreground = Color::grey(),
	};
	
	const Style style_locked {
		.font = font::fixed_8x16,
		.background = Color::black(),
		.foreground = Color::green(),
	};
	
	rf::Frequency f_min { 0 }, f_max { 0 };
	Coord last_pos { 0 };
	ChannelSpectrumFIFO* fifo { nullptr };
	uint8_t detect_counter { 0 }, release_counter { 0 };
	uint8_t slice_trim { 0 };
	uint32_t mean { 0 };
	uint32_t min_threshold { 80 };	// Todo: Put this in persistent / settings
	rf::Frequency slice_start { 0 };
	rf::Frequency slice_frequency { 0 };
	uint8_t slices_max { 0 };
	uint8_t slices_counter { 0 };
	int16_t last_channel { 0 };
	uint32_t weight { 0 };
	uint64_t frequency_acc { 0 };
	rf::Frequency scan_span { 0 }, resolved_frequency { 0 };
	uint16_t locked_imax { 0 };
	uint8_t slicemax_pow[32];		// Todo: Cap max slices !
	int16_t slicemax_idx[32];
	uint8_t scan_counter { 0 };
	SignalToken signal_token_tick_second { };
	bool ignore { true };
	bool slicing { false };
	bool locked { false };
	
	void on_channel_spectrum(const ChannelSpectrum& spectrum);
	void on_range_changed();
	void do_detection();
	void on_lna_changed(int32_t v_db);
	void on_vga_changed(int32_t v_db);
	void on_tick_second();
	
	/* |012345678901234567890123456789|
	 * | Min:      Max:       LNA VGA |
	 * | 0000.0000 0000.0000  00  00  |
	 * | Threshold: 000               |
	 * | Slices: 00        Rate: 00Hz |
	 * |
	 * */
	
	Labels labels {
		{ { 1 * 8, 0 }, "Min:      Max:       LNA VGA", Color::light_grey() },
		{ { 1 * 8, 4 * 8 }, "Threshold:", Color::light_grey() },
		{ { 1 * 8, 6 * 8 }, "Slices:           Rate:   Hz", Color::light_grey() }
	};
	
	NumberField field_threshold {
		{ 12 * 8, 2 * 16 },
		3,
		{ 5, 250 },
		5,
		' '
	};
	 
	FrequencyField field_frequency_min {
		{ 1 * 8, 1 * 16 },
	};
	FrequencyField field_frequency_max {
		{ 11 * 8, 1 * 16 },
	};
	
	LNAGainField field_lna {
		{ 22 * 8, 1 * 16 }
	};
	VGAGainField field_vga {
		{ 26 * 8, 1 * 16 }
	};
	
	Text text_slices {
		{ 9 * 8, 3 * 16, 2 * 8, 16 },
		"--"
	};
	Text text_rate {
		{ 24 * 8, 3 * 16, 2 * 8, 16 },
		"--"
	};
	
	Text text_infos {
		{ 1 * 8, 6 * 16, 28 * 8, 16 },
		"..."
	};
	
	BigFrequency big_display {
		{ 4, 8 * 16, 28 * 8, 32 },
		0
	};
	
	Text text_mhz {
		{ 26 * 8, 12 * 16 - 4, 3 * 8, 16 },
		"MHz"
	};
	
	Text text_precision {
		{ 1 * 8, 13 * 16, 28 * 8, 16 },
		"..."
	};
	
	Text text_debug {
		{ 1 * 8, 14 * 16, 28 * 8, 16 },
		"DEBUG: -"
	};
	
	Button button_exit {
		{ 92, 264, 56, 32 },
		"Exit"
	};
	
	MessageHandlerRegistration message_handler_spectrum_config {
		Message::ID::ChannelSpectrumConfig,
		[this](const Message* const p) {
			const auto message = *reinterpret_cast<const ChannelSpectrumConfigMessage*>(p);
			this->fifo = message.fifo;
		}
	};
	MessageHandlerRegistration message_handler_frame_sync {
		Message::ID::DisplayFrameSync,
		[this](const Message* const) {
			if( this->fifo ) {
				ChannelSpectrum channel_spectrum;
				while( fifo->out(channel_spectrum) ) {
					this->on_channel_spectrum(channel_spectrum);
				}
			}
		}
	};
};

} /* namespace ui */
