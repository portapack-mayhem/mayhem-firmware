/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __UI_SPECTRUM_H__
#define __UI_SPECTRUM_H__

#include "ui.hpp"
#include "ui_widget.hpp"

#include "event_m0.hpp"

#include "message.hpp"

#include <cstdint>
#include <cstddef>

namespace ui {
namespace spectrum {

class AudioSpectrumView : public View {
public:
	AudioSpectrumView(const Rect parent_rect);
	
	void paint(Painter& painter) override;
	
	void on_audio_spectrum(const AudioSpectrum* spectrum);
	
private:
	static constexpr int cursor_band_height = 4;
	
	int16_t audio_spectrum[128] { 0 };
	
	Labels labels {
		{ { 6 * 8, 0 * 16 }, "Hz", Color::light_grey() }
	};
	
	NumberField field_frequency {
		{ 0 * 8, 0 * 16 },
		5,
		{ 0, 48000 },
		48000 / 240,
		' '
	};
	
	Waveform waveform {
		{ 0, 1 * 16 + cursor_band_height, 30 * 8, 2 * 16 },
		audio_spectrum,
		128,
		0,
		false,
		Color::white()
	};
};

class FrequencyScale : public Widget {
public:
	std::function<void(int32_t offset)> on_select { };
	
	void on_show() override;
	void on_focus() override;
	void on_blur() override;
	
	bool on_encoder(const EncoderEvent delta) override;
	bool on_key(const KeyEvent key) override;

	void set_spectrum_sampling_rate(const int new_sampling_rate);
	void set_channel_filter(const int pass_frequency, const int stop_frequency);

	void paint(Painter& painter) override;

private:
	static constexpr int filter_band_height = 4;

	void on_tick_second();
	
	bool _blink { false };
	int32_t cursor_position { 0 };
	SignalToken signal_token_tick_second { };
	int spectrum_sampling_rate { 0 };
	const int spectrum_bins = std::tuple_size<decltype(ChannelSpectrum::db)>::value;
	int channel_filter_pass_frequency { 0 };
	int channel_filter_stop_frequency { 0 };

	void clear();
	void clear_background(Painter& painter, const Rect r);

	void draw_frequency_ticks(Painter& painter, const Rect r);
	void draw_filter_ranges(Painter& painter, const Rect r);
};

class WaterfallView : public Widget {
public:
	void on_show() override;
	void on_hide() override;

	void paint(Painter& painter) override;

	void on_channel_spectrum(const ChannelSpectrum& spectrum);

private:
	void clear();
};

class WaterfallWidget : public View {
public:
	std::function<void(int32_t offset)> on_select { };
	
	WaterfallWidget(const bool cursor = false);

	WaterfallWidget(const WaterfallWidget&) = delete;
	WaterfallWidget(WaterfallWidget&&) = delete;
	WaterfallWidget& operator=(const WaterfallWidget&) = delete;
	WaterfallWidget& operator=(WaterfallWidget&&) = delete;

	void on_show() override;
	void on_hide() override;

	void set_parent_rect(const Rect new_parent_rect) override;
	
	void show_audio_spectrum_view(const bool show);

	void paint(Painter& painter) override;

private:
	void update_widgets_rect();
	
	const Rect audio_spectrum_view_rect { 0 * 8, 0 * 16, 30 * 8, 2 * 16 + 20 };
	static constexpr Dim audio_spectrum_height = 16 * 2 + 20;
	static constexpr Dim scale_height = 20;
	
	WaterfallView waterfall_view { };
	FrequencyScale frequency_scale { };
	
	ChannelSpectrumFIFO* channel_fifo { nullptr };
	AudioSpectrum* audio_spectrum_data { nullptr };
	bool audio_spectrum_update { false };
	
	std::unique_ptr<AudioSpectrumView> audio_spectrum_view { };
	
	int sampling_rate { 0 };
	int32_t cursor_position { 0 };
	ui::Rect waterfall_normal_rect { };
	ui::Rect waterfall_reduced_rect { };

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

} /* namespace spectrum */
} /* namespace ui */

#endif/*__UI_SPECTRUM_H__*/
