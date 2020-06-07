/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2018 Furrtek
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

#include "ui_receiver.hpp"
#include "ui_font_fixed_8x16.hpp"
#include "freqman.hpp"

namespace ui {

class ScannerThread {
public:
	ScannerThread(std::vector<rf::Frequency> frequency_list);
	~ScannerThread();
	
	void set_scanning(const bool v);

	ScannerThread(const ScannerThread&) = delete;
	ScannerThread(ScannerThread&&) = delete;
	ScannerThread& operator=(const ScannerThread&) = delete;
	ScannerThread& operator=(ScannerThread&&) = delete;

private:
	std::vector<rf::Frequency> frequency_list_ { };
	Thread* thread { nullptr };
	
	bool _scanning { true };

	static msg_t static_fn(void* arg);
	
	void run();
};

class ScannerView : public View {
public:
	ScannerView(NavigationView&);
	~ScannerView();
	
	void focus() override;
	
	std::string title() const override { return "Scanner"; };

private:
	void on_statistics_update(const ChannelStatistics& statistics);
	void on_headphone_volume_changed(int32_t v);
	void handle_retune(uint32_t i);
	
	std::vector<rf::Frequency> frequency_list { };
	std::vector<string> description_list { };
	int32_t trigger { 0 };
	int32_t squelch { 0 };
	uint32_t timer { 0 };
	uint32_t wait { 0 };
	freqman_db database { };
	
	Labels labels {
		{ { 0 * 8, 0 * 16 }, "LNA:    TRIGGER:  /99   VOL:", Color::light_grey() },
		{ { 0 * 8, 1 * 16 }, "VGA:    SQUELCH:  /99   AMP:", Color::light_grey() },
		{ { 0 * 8, 2 * 16 }, " BW:       WAIT:", Color::light_grey() },
	};
	
	LNAGainField field_lna {
		{ 4 * 8, 0 * 16 }
	};

	VGAGainField field_vga {
		{ 4 * 8, 1 * 16 }
	};
	
	RFAmpField field_rf_amp {
		{ 28 * 8, 1 * 16 }
	};
	
	NumberField field_volume {
		{ 28 * 8, 0 * 16 },
		2,
		{ 0, 99 },
		1,
		' ',
	};

	OptionsField field_bw {
		{ 4 * 8, 2 * 16 },
		3,
		{
			{ "8k5", 0 },
			{ "11k", 0 },
			{ "16k", 0 },
		}
	};

	NumberField field_trigger {
		{ 16 * 8, 0 * 16 },
		2,
		{ 0, 99 },
		1,
		' ',
	};
	
	NumberField field_squelch {
		{ 16 * 8, 1 * 16 },
		2,
		{ 0, 99 },
		1,
		' ',
	};

	NumberField field_wait {
		{ 16 * 8, 2 * 16 },
		2,
		{ 0, 99 },
		1,
		' ',
	};

	Text text_cycle {
		{ 0, 5 * 16, 240, 16 },
		"--/--"
	};
	Text desc_cycle {
		{0, 6 * 16, 240, 16 },
		" "
	};
	
	std::unique_ptr<ScannerThread> scan_thread { };
	
	MessageHandlerRegistration message_handler_retune {
		Message::ID::Retune,
		[this](const Message* const p) {
			const auto message = *reinterpret_cast<const RetuneMessage*>(p);
			this->handle_retune(message.range);
		}
	};
	
	MessageHandlerRegistration message_handler_stats {
		Message::ID::ChannelStatistics,
		[this](const Message* const p) {
			this->on_statistics_update(static_cast<const ChannelStatisticsMessage*>(p)->statistics);
		}
	};
};

} /* namespace ui */
