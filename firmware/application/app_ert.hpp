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

#include "ui_console.hpp"
#include "message.hpp"
#include "log_file.hpp"
#include "manchester.hpp"
#include "field_reader.hpp"

#include "lpc43xx_cpp.hpp"
using namespace lpc43xx;

#include <cstddef>
#include <string>
#include <bitset>

namespace ert {

struct BitRemap {
	size_t operator()(const size_t bit_index) const {
		return bit_index;
	}
};

using ID = uint32_t;
using Consumption = uint32_t;

class Packet {
public:
	Packet(
		const rtc::RTC& received_at,
		const ERTPacket::Type type,
		const std::bitset<1024>& payload,
		const size_t payload_length
	) : payload_ { payload },
		payload_length_ { payload_length },
		received_at_ { received_at },
		decoder_ { payload_, payload_length_ },
		reader_ { decoder_ },
		type_ { type }
	{
	}

	size_t length() const;
	
	bool is_valid() const;

	rtc::RTC received_at() const;

	ERTPacket::Type type() const;
	ID id() const;
	Consumption consumption() const;

	bool crc_ok() const;

private:
	using Reader = FieldReader<ManchesterDecoder, BitRemap>;

	const std::bitset<1024> payload_;
	const size_t payload_length_;
	const rtc::RTC received_at_;
	const ManchesterDecoder decoder_;
	const Reader reader_;
	const ERTPacket::Type type_;

	const ID invalid_id = 0;
	const Consumption invalid_consumption = 0;
};

} /* namespace ert */

class ERTModel {
public:
	ERTModel();

	bool on_packet(const ert::Packet& packet);

private:
	LogFile log_file;
};

namespace ui {

class ERTView : public Console {
public:
	void on_show() override;
	void on_hide() override;

private:
	ERTModel model;

	void on_packet(const ert::Packet& packet);
};

} /* namespace ui */

#endif/*__APP_ERT_H__*/
