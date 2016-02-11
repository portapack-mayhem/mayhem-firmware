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

#include "ui_widget.hpp"
#include "ui_navigation.hpp"

#include "field_reader.hpp"
#include "baseband_packet.hpp"
#include "manchester.hpp"
#include "log_file.hpp"

#include "recent_entries.hpp"

#include "optional.hpp"

#include "units.hpp"
using units::Temperature;
using units::Pressure;

namespace tpms {

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

	constexpr uint32_t value() const {
		return id_;
	}

private:
	uint32_t id_;
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

namespace std {

constexpr bool operator==(const tpms::TransponderID& lhs, const tpms::TransponderID& rhs) {
	return (lhs.value() == rhs.value());
}

} /* namespace std */

struct TPMSRecentEntry {
	using Key = std::pair<tpms::Reading::Type, tpms::TransponderID>;

	static const Key invalid_key;

	tpms::Reading::Type type { invalid_key.first };
	tpms::TransponderID id { invalid_key.second };

	size_t received_count { 0 };

	Optional<Pressure> last_pressure;
	Optional<Temperature> last_temperature;

	TPMSRecentEntry(
		const Key& key
	) : type { key.first },
		id { key.second }
	{
	}

	Key key() const {
		return { type, id };
	}

	void update(const tpms::Reading& reading);
};

using TPMSRecentEntries = RecentEntries<tpms::Reading, TPMSRecentEntry>;

class TPMSLogger {
public:
	TPMSLogger(const std::string& file_path);
	
	void on_packet(const tpms::Packet& packet, const uint32_t target_frequency);

private:
	LogFile log_file;
};

namespace ui {

using TPMSRecentEntriesView = RecentEntriesView<TPMSRecentEntries>;

class TPMSAppView : public View {
public:
	TPMSAppView(NavigationView& nav);
	~TPMSAppView();

	void set_parent_rect(const Rect new_parent_rect) override;

	// Prevent painting of region covered entirely by a child.
	// TODO: Add flag to View that specifies view does not need to be cleared before painting.
	void paint(Painter&) override { };

	void focus() override;

	std::string title() const override { return "TPMS"; };

private:
	static constexpr uint32_t initial_target_frequency = 315000000;
	static constexpr uint32_t sampling_rate = 2457600;
	static constexpr uint32_t baseband_bandwidth = 1750000;

	TPMSRecentEntries recent;
	std::unique_ptr<TPMSLogger> logger;

	TPMSRecentEntriesView recent_entries_view { recent };

	void on_packet(const tpms::Packet& packet);
	void on_show_list();

	uint32_t target_frequency() const;
	uint32_t tuning_frequency() const;
};

} /* namespace ui */

#endif/*__TPMS_APP_H__*/
