/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2020 Shao
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

#ifndef __UI_TV_H__
#define __UI_TV_H__

#include "ui.hpp"
#include "ui_widget.hpp"

#include "event_m0.hpp"

#include "message.hpp"

#include <cstdint>
#include <cstddef>

namespace ui {
namespace tv {

class TimeScopeView : public View {
public:
	TimeScopeView(const Rect parent_rect);
	
	void paint(Painter& painter) override;
	
	void on_audio_spectrum(const AudioSpectrum* spectrum);
	
private:
	static constexpr int cursor_band_height = 4;
	
	int16_t audio_spectrum[128] { 0 };
	
	/*Labels labels {
		{ { 6 * 8, 0 * 16 }, "Hz", Color::light_grey() }
	};*/
	/*
	NumberField field_frequency {
		{ 0 * 8, 0 * 16 },
		5,
		{ 0, 48000 },
		48000 / 240,
		' '
	};*/
	
	Waveform waveform {
		{ 0, 1 * 16 + cursor_band_height, 30 * 8, 2 * 16 },
		audio_spectrum,
		128,
		0,
		false,
		Color::white()
	};
};

class TVView : public Widget {
public:
	void on_show() override;
	void on_hide() override;

	void paint(Painter& painter) override;
	void on_channel_spectrum(const ChannelSpectrum& spectrum);
	void on_adjust_xcorr(uint8_t xcorr);
	//ui::Color video_buffer[13312];
        uint8_t video_buffer_int[13312+128] { 0 }; //128 is for the over length caused by x_correction
	uint32_t count=0;
        uint8_t x_correction=0;
private:
	void clear();
	
};

class TVWidget : public View {
public:
	std::function<void(int32_t offset)> on_select { };
	
	TVWidget(const bool cursor = false);

	TVWidget(const TVWidget&) = delete;
	TVWidget(TVWidget&&) = delete;
	TVWidget& operator=(const TVWidget&) = delete;
	TVWidget& operator=(TVWidget&&) = delete;

	void on_show() override;
	void on_hide() override;

	void set_parent_rect(const Rect new_parent_rect) override;
	
	void show_audio_spectrum_view(const bool show);

	void paint(Painter& painter) override;
	NumberField field_xcorr {
		{ 0 * 8, 0 * 16 },
		5,
		{ 0, 128 },
		1,
		' '
	};

private:
	void update_widgets_rect();
	
	const Rect audio_spectrum_view_rect { 0 * 8, 0 * 16, 30 * 8, 2 * 16 + 20 };
	static constexpr Dim audio_spectrum_height = 16 * 2 + 20;
	static constexpr Dim scale_height = 20;
	
	TVView tv_view { };

	ChannelSpectrumFIFO* channel_fifo { nullptr };
	AudioSpectrum* audio_spectrum_data { nullptr };
	bool audio_spectrum_update { false };
	
	std::unique_ptr<TimeScopeView> audio_spectrum_view { };
	
	int sampling_rate { 0 };
	int32_t cursor_position { 0 };
	ui::Rect tv_normal_rect { };
	ui::Rect tv_reduced_rect { };

	MessageHandlerRegistration message_handler_channel_spectrum_config {
		Message::ID::ChannelSpectrumConfig,
		[this](const Message* const p) {
			const auto message = *reinterpret_cast<const ChannelSpectrumConfigMessage*>(p);
			this->channel_fifo = message.fifo;
		}
	};
	MessageHandlerRegistration message_handler_audio_spectrum {
		Message::ID::AudioSpectrum,
		[this](const Message* const p) {
			const auto message = *reinterpret_cast<const AudioSpectrumMessage*>(p);
			this->audio_spectrum_data = message.data;
			this->audio_spectrum_update = true;
		}
	};
	MessageHandlerRegistration message_handler_frame_sync {
		Message::ID::DisplayFrameSync,
		[this](const Message* const) {
			if( this->channel_fifo ) {
				ChannelSpectrum channel_spectrum;
				while( channel_fifo->out(channel_spectrum) ) {
					this->on_channel_spectrum(channel_spectrum);
				}
			}
			if (this->audio_spectrum_update) {
				this->audio_spectrum_update = false;
				this->on_audio_spectrum();
			}
		}
	};

	void on_channel_spectrum(const ChannelSpectrum& spectrum);
	void on_audio_spectrum();
};

} /* namespace tv */
} /* namespace ui */

#endif/*__UI_TV_H__*/
