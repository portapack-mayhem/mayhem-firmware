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

#ifndef __APP_ERT_H__
#define __APP_ERT_H__

#include "receiver_model.hpp"
#include "ui_console.hpp"
#include "message.hpp"

#include <string>

class ERTModel {
public:
	ERTModel() {
		receiver_model.set_baseband_configuration({
			.mode = 6,
			.sampling_rate = 4194304,
			.decimation_factor = 1,
		});
		receiver_model.set_baseband_bandwidth(1750000);
	}

	std::string on_packet(const ERTPacketMessage& message) {
		std::string s;

		if( message.packet.preamble == 0x555516a3 ) {
			s += "IDM\n";
		}
		if( message.packet.preamble == 0x1f2a60 ) {
			s += "SCM\n";
		}

		const ManchesterDecoder decoder(message.packet.payload, message.packet.bits_received);

		const auto hex_formatted = format_manchester(decoder);
		s += hex_formatted.data;
		s += "\n";
		s += hex_formatted.errors;
		s += "\n";

		return s;
	}

private:
};

namespace ui {

class ERTView : public Console {
public:
	void on_show() override {
		Console::on_show();

		auto& message_map = context().message_map();
		message_map.register_handler(Message::ID::ERTPacket,
			[this](Message* const p) {
				const auto message = static_cast<const ERTPacketMessage*>(p);
				this->log(this->model.on_packet(*message));
			}
		);
	}

	void on_hide() override {
		auto& message_map = context().message_map();
		message_map.unregister_handler(Message::ID::ERTPacket);

		Console::on_hide();
	}

private:
	ERTModel model;

	void log(const std::string& s) {
		write(s);
	}
};

} /* namespace ui */

#endif/*__APP_ERT_H__*/
