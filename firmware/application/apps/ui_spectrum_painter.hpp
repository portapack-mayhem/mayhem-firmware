/*
 * Copyright (C) 2023 Bernd Herzog
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

#pragma once

#include "ui.hpp"
#include "ui_widget.hpp"

#include "ui_navigation.hpp"
#include "ui_tabview.hpp"
#include "ui_transmitter.hpp"
#include "baseband_api.hpp"

#include "portapack.hpp"
#include "message.hpp"

namespace ui {

class SpectrumInputImageView : public View {
public:
	SpectrumInputImageView(NavigationView& nav);
	~SpectrumInputImageView();

	void focus() override;
	void paint(Painter&) override;

	//std::string get_filename();
	bool drawBMP_scaled(const ui::Rect r, const std::string file);
	uint16_t get_width();
	uint16_t get_height();
	std::vector<uint8_t> get_line(uint16_t);

	std::function<void()> on_input_avaliable { };

private:
	bool painted {false};
	std::string file {""};
	uint16_t width {0};
	uint16_t height {0};
	uint8_t type {0};
	uint32_t data_start {0};
	
	Button button_load_image {
		{ 0 * 8, 17 * 8, 18 * 8, 28 },
		"Load Image ..."
	};
};

class SpectrumInputTextView : public View {
public:
	SpectrumInputTextView(NavigationView& nav);
	~SpectrumInputTextView();

	void focus() override;
	
private:
	Button button_start {
		{ 0 * 8, 11 * 8, 11 * 8, 28 },
		"s2"
	};
};

class SpectrumPainterView : public View {
public:
	SpectrumPainterView(NavigationView& nav);
	~SpectrumPainterView();

	SpectrumPainterView(const SpectrumPainterView&) = delete;
	SpectrumPainterView(SpectrumPainterView&&) = delete;
	SpectrumPainterView& operator=(const SpectrumPainterView&) = delete;
	SpectrumPainterView& operator=(SpectrumPainterView&&) = delete;

	void focus() override;
	
	std::string title() const override { return "Spec.Painter"; };

private:
	NavigationView& nav_;
	bool tx_active { false };
	uint16_t tx_current_line { 0 };
	uint16_t tx_current_max_lines { 0 };

	SpectrumInputImageView input_image { nav_ };
	SpectrumInputTextView input_text { nav_ };
	
	std::array<View*, 2> input_views { { &input_image, &input_text } };

	TabView tab_view {
		{ "Image", Color::white(), input_views[0] },
		{ "Text", Color::white(), input_views[1] }
	};

	ProgressBar progressbar {
		{ 16, 25 * 8, 208, 16 }
	};
	
	TransmitterView tx_view {
		16 * 16,
		10000,
		12
	};

	SpectrumPainterFIFO* fifo { nullptr };

	void start_tx();
	void frame_sync();

	// TODO Add Message + hander for initial setup: M4 will send two buffer pointer
	MessageHandlerRegistration message_handler_fifo_signal {
		Message::ID::SpectrumPainterBufferResponseConfigure,
		[this](const Message* const p) {
			const auto message = static_cast<const SpectrumPainterBufferConfigureResponseMessage*>(p);
			this->fifo = message->fifo;
			this->start_tx();

			// if (message->signal == RequestSignalMessage::Signal::FillRequest) {
			// 	this->OnRequestSignal();
			// }
		}
	};

	MessageHandlerRegistration message_handler_sample {
		Message::ID::DisplayFrameSync,
		[this](const Message* const) {
			this->frame_sync();
		}
	};
};

}
