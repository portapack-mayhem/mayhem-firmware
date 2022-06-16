/*
 * Copyright (C) 2016 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
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

#ifndef __GPS_SIM_APP_HPP__
#define __GPS_SIM_APP_HPP__
#include "io_file.hpp"

#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "stream_reader.hpp"
#include "ui_spectrum.hpp"

#include <string>
#include <memory>

namespace ui {

class GpsSimAppView : public View {
public:
	GpsSimAppView(NavigationView& nav);
	~GpsSimAppView();

	void on_hide() override;
	void set_parent_rect(const Rect new_parent_rect) override;
	void focus() override;

	std::string title() const override { return "GPS Sim TX"; };
	
private:
	NavigationView& nav_;
	
	static constexpr ui::Dim header_height = 3 * 16;
	
	uint32_t sample_rate = 0;
	int32_t tx_gain { 47 };
	bool rf_amp { true }; // aux private var to store temporal, same as Replay App rf_amp user selection.
	static constexpr uint32_t baseband_bandwidth = 3000000; //filter bandwidth
	const size_t read_size { 16384 };
	const size_t buffer_count { 3 };

	void on_file_changed(std::filesystem::path new_file_path);
	void on_target_frequency_changed(rf::Frequency f);
	void on_tx_progress(const uint32_t progress);
	
	void set_target_frequency(const rf::Frequency new_value);
	rf::Frequency target_frequency() const;

	void toggle();
	void start();
	void stop(const bool do_loop);
	bool is_active() const;
	void handle_stream_reader_done(const Error error);
	void handle_error(const Error error);

	std::filesystem::path file_path { };

	// handle io exchange
	std::unique_ptr<stream::IoExchange> io_exchange{};
	MessageHandlerRegistration io_exchange_handler_registration{
		Message::ID::IoExchangeConfig,
		[this](const Message *const message)
		{
			if (io_exchange)
				io_exchange.reset();

			const auto *const msg = reinterpret_cast<const IoExchangeMessage *>(message);
			io_exchange = std::make_unique<stream::IoExchange>(msg->config);
		}};

	std::unique_ptr<stream::StreamReader> stream_reader{};

	Labels labels {
		{ { 10 * 8, 2 * 16 }, "GAIN   A:", Color::light_grey() }
	};
	
	Button button_open {
		{ 0 * 8, 0 * 16, 10 * 8, 2 * 16 },
		"Open file"
	};
	
	Text text_filename {
		{ 11 * 8, 0 * 16, 12 * 8, 16 },
		"-"
	};
	Text text_sample_rate {
		{ 24 * 8, 0 * 16, 6 * 8, 16 },
		"-"
	};
	
	Text text_duration {
		{ 11 * 8, 1 * 16, 6 * 8, 16 },
		"-"
	};
	ProgressBar progressbar {
		{ 18 * 8, 1 * 16, 12 * 8, 16 }
	};
	
	FrequencyField field_frequency {
		{ 0 * 8, 2 * 16 },
	};
	NumberField field_rfgain {
		{ 14 * 8, 2 * 16 },
		2,
		{ 0, 47 },
		1,
		' '	
	};
	NumberField field_rfamp {     // previously we were using "RFAmpField field_rf_amp" but that is general Receiver amp setting.
		{ 19 * 8, 2 * 16 },
		2,
		{ 0, 14 },                // this time we will display GUI , 0 or 14 dBs same as Mic and Replay App
		14,
		' '
	};
	Checkbox check_loop {
		{ 21 * 8, 2 * 16 },
		4,
		"Loop",
		true
	};
	ImageButton button_play {
		{ 28 * 8, 2 * 16, 2 * 8, 1 * 16 },
		&bitmap_play,
		Color::green(),
		Color::black()
	};

	spectrum::WaterfallWidget waterfall { };

	MessageHandlerRegistration message_handler_stream_reader_error{
		Message::ID::StreamReaderDone,
		[this](const Message *const p) {
			const auto message = *reinterpret_cast<const StreamReaderDoneMessage *>(p);
			this->handle_stream_reader_done(message.error.code);
		}};

	MessageHandlerRegistration message_handler_tx_progress{
		Message::ID::TXProgress,
		[this](const Message *const p) {
			const auto message = *reinterpret_cast<const TXProgressMessage *>(p);
			this->on_tx_progress(message.progress);
		}};
};

} /* namespace ui */

#endif/*__GPS_SIM_APP_HPP__*/
