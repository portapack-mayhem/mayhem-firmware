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

#ifndef __TPMS_APP_H__
#define __TPMS_APP_H__

#include "ui_console.hpp"

#include "field_reader.hpp"
#include "baseband_packet.hpp"
#include "manchester.hpp"
#include "log_file.hpp"

namespace tpms {

template<typename T>
class Optional {
public:
	constexpr Optional() : value_ { }, valid_ { false } { };
	constexpr Optional(const T& value) : value_ { value }, valid_ { true } { };
	constexpr Optional(T&& value) : value_ { std::move(value) }, valid_ { true } { };

	bool is_valid() const { return valid_; };
	T value() const { return value_; };

private:
	const T value_;
	const bool valid_;
};

class Reading {
public:
	constexpr Reading(
	) : type_ { 0 },
		id_ { 0 },
		value_1_ { 0 },
		value_2_ { 0 },
		value_3_ { 0 }
	{
	}
	
	constexpr Reading(
		uint32_t type,
		uint32_t id,
		uint16_t value_1
	) : type_ { type },
		id_ { id },
		value_1_ { value_1 },
		value_2_ { },
		value_3_ { }
	{
	}

	constexpr Reading(
		uint32_t type,
		uint32_t id,
		uint16_t value_1,
		uint16_t value_2,
		uint16_t value_3
	) : type_ { type },
		id_ { id },
		value_1_ { value_1 },
		value_2_ { value_2 },
		value_3_ { value_3 }
	{
	}

	uint32_t type() const {
		return type_;
	}

	uint32_t id() const {
		return id_;
	}

	uint16_t value_1() const {
		return value_1_;
	}

	Optional<uint16_t> value_2() const {
		return value_2_;
	}

	Optional<uint16_t> value_3() const {
		return value_3_;
	}

private:
	uint32_t type_;
	uint32_t id_;
	uint16_t value_1_;
	Optional<uint16_t> value_2_;
	Optional<uint16_t> value_3_;
};

class Packet {
public:
	constexpr Packet(
		const baseband::Packet& packet
	) : packet_ { packet },
		decoder_ { packet_, 1 },
		reader_ { decoder_ }
	{
	}

	Timestamp received_at() const;

	ManchesterFormatted symbols_formatted() const;

	Optional<Reading> reading() const;

private:
	using Reader = FieldReader<ManchesterDecoder, BitRemapNone>;

	const baseband::Packet packet_;
	const ManchesterDecoder decoder_;

	const Reader reader_;

	size_t crc_valid_length() const;
};

} /* namespace tpms */

class TPMSLogger {
public:
	void on_packet(const tpms::Packet& packet);

private:
	LogFile log_file { "tpms.txt" };
};

namespace ui {

class TPMSAppView : public View {
public:
	TPMSAppView();
	~TPMSAppView();

	void set_parent_rect(const Rect new_parent_rect) override;

private:
	TPMSLogger logger;

	Console console;

	void on_packet(const tpms::Packet& packet);
	void draw(const tpms::Packet& packet);
};

} /* namespace ui */

#endif/*__TPMS_APP_H__*/
