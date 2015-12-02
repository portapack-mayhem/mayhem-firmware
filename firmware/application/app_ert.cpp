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

#include "app_ert.hpp"

#include "portapack.hpp"
using namespace portapack;

#include "manchester.hpp"

ERTModel::ERTModel() {
	receiver_model.set_baseband_configuration({
		.mode = 6,
		.sampling_rate = 4194304,
		.decimation_factor = 1,
	});
	receiver_model.set_baseband_bandwidth(1750000);

	log_file.open_for_append("ert.txt");
}

std::string ERTModel::on_packet(const ERTPacketMessage& message) {
	std::string entry;

	if( message.packet.preamble == 0x555516a3 ) {
		entry += "IDM ";
	}
	if( message.packet.preamble == 0x1f2a60 ) {
		entry += "SCM ";
	}

	const ManchesterDecoder decoder(message.packet.payload, message.packet.bits_received);

	const auto hex_formatted = format_manchester(decoder);
	entry += hex_formatted.data;
	entry += "/";
	entry += hex_formatted.errors;

	if( log_file.is_ready() ) {
		log_file.write_entry(entry);
	}

	return entry;
}

namespace ui {

void ERTView::on_show() {
	Console::on_show();

	auto& message_map = context().message_map();
	message_map.register_handler(Message::ID::ERTPacket,
		[this](Message* const p) {
			const auto message = static_cast<const ERTPacketMessage*>(p);
			this->log(this->model.on_packet(*message));
		}
	);
}

void ERTView::on_hide() {
	auto& message_map = context().message_map();
	message_map.unregister_handler(Message::ID::ERTPacket);

	Console::on_hide();
}

void ERTView::log(const std::string& s) {
	writeln(s);
}

} /* namespace ui */
