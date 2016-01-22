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
	T value_;
	bool valid_;
};

class TransponderID {
public:
	constexpr TransponderID(
	) : id_ { 0 }
	{
	}

	constexpr TransponderID(
		const uint32_t id
	) : id_ { id }
	{
	}

	uint32_t value() const {
		return id_;
	}

private:
	uint32_t id_;
};

class Pressure {
public:
	constexpr Pressure(
	) : kpa_ { 0 }
	{
	}

	constexpr Pressure(
		const int32_t kilopascal
	) : kpa_ { static_cast<int16_t>(kilopascal) }
	{
	}

	int16_t kilopascal() const {
		return kpa_;
	}

	int16_t psi() const {
		return static_cast<int32_t>(kpa_) * 1000 / 6895;
	}

private:
	int16_t kpa_;
};

class Temperature {
public:
	constexpr Temperature(
	) : c_ { 0 }
	{
	}

	constexpr Temperature(
		const int32_t celsius
	) : c_ { static_cast<int16_t>(celsius) }
	{
	}

	int16_t celsius() const {
		return c_;
	}

	int16_t fahrenheit() const {
		return (c_ * 9 / 5) + 32;
	}

private:
	int16_t c_;
};

class Reading {
public:
	enum Type {
		None = 0,
		FLM_64 = 1,
		FLM_72 = 2,
		FLM_80 = 3,
	};

	constexpr Reading(
	) : type_ { Type::None }
	{
	}
	
	constexpr Reading(
		Type type,
		TransponderID id
	) : type_ { type },
		id_ { id }
	{
	}
	
	constexpr Reading(
		Type type,
		TransponderID id,
		Optional<Pressure> pressure = { },
		Optional<Temperature> temperature = { }
	) : type_ { type },
		id_ { id },
		pressure_ { pressure },
		temperature_ { temperature }
	{
	}

	Type type() const {
		return type_;
	}

	TransponderID id() const {
		return id_;
	}

	Optional<Pressure> pressure() const {
		return pressure_;
	}

	Optional<Temperature> temperature() const {
		return temperature_;
	}

private:
	Type type_ { Type::None };
	TransponderID id_ { 0 };
	Optional<Pressure> pressure_ { };
	Optional<Temperature> temperature_ { };
};

class Packet {
public:
	constexpr Packet(
		const baseband::Packet& packet
	) : packet_ { packet },
		decoder_ { packet_, 0 },
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
