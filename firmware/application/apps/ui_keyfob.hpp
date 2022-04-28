/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2017 Furrtek
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

#include "ui.hpp"
#include "ui_transmitter.hpp"
#include "transmitter_model.hpp"
#include "encoders.hpp"

using namespace encoders;

namespace ui {

class KeyfobView : public View {
public:
	KeyfobView(NavigationView& nav);
	~KeyfobView();
	
	void focus() override;
	
	std::string title() const override { return "Key fob TX (beta)"; };

private:
	NavigationView& nav_;
	
	// 1013210ns / bit
	static constexpr uint32_t subaru_samples_per_bit = (OOK_SAMPLERATE * 0.00101321);
	static constexpr uint32_t repeats = 4;
	
	uint8_t frame[10] { };
	
	void generate_payload(size_t& bitstream_length);
	size_t generate_frame();
	void start_tx();
	void on_tx_progress(const uint32_t progress, const bool done);
	void update_progress(const uint32_t progress);
	void on_make_change(size_t index);
	void on_command_change(uint32_t value);
	
	// DEBUG
	void update_symfields();
	
	uint8_t subaru_get_checksum();
	bool subaru_is_valid();
	uint16_t subaru_get_code();
	void subaru_set_code(const uint16_t code);
	int32_t subaru_get_command();
	void subaru_set_command(const uint32_t command);

	Labels labels {
		{ { 5 * 8, 1 * 16 }, "Make:", Color::light_grey() },
		{ { 2 * 8, 2 * 16 }, "Command:", Color::light_grey() },
		{ { 2 * 8, 4 * 16 }, "Payload:       #####", Color::light_grey() },
		{ { 2 * 8, 7 * 16 }, "Checksum is fixed just", Color::light_grey() },
		{ { 2 * 8, 8 * 16 }, "before transmission.", Color::light_grey() },
	};

	OptionsField options_make {
		{ 10 * 8, 1 * 16 },
		8,
		{
			{ "Subaru", 0 }
		}
	};
	
	OptionsField options_command {
		{ 10 * 8, 2 * 16 },
		6,
		{
			{ "Lock", 1 },
			{ "Unlock", 2 },
			{ "Trunk", 11 },
			{ "Panic", 10 }
		}
	};
	
	SymField field_payload_a {
		{ 2 * 8, 5 * 16 },
		10,
		SymField::SYMFIELD_HEX
	};
	SymField field_payload_b {
		{ 13 * 8, 5 * 16 },
		10,
		SymField::SYMFIELD_HEX
	};
	
	Text text_status {
		{ 2 * 8, 13 * 16, 128, 16 },
		"Ready"
	};
	
	ProgressBar progressbar {
		{ 2 * 8, 13 * 16 + 20, 208, 16 }
	};
	
	TransmitterView tx_view {
		16 * 16,
		0,
		15,
		true
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
