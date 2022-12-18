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

#ifndef __ERT_APP_H__
#define __ERT_APP_H__

#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "ui_rssi.hpp"
#include "ui_channel.hpp"

#include "event_m0.hpp"
#include "app_settings.hpp"
#include "log_file.hpp"

#include "ert_packet.hpp"

#include "recent_entries.hpp"

#include <cstddef>
#include <string>

struct ERTKey {
	ert::ID id;
	ert::CommodityType commodity_type;

	constexpr ERTKey(
		ert::ID id = ert::invalid_id,
		ert::CommodityType commodity_type = ert::invalid_commodity_type
	) : id { id  },
		commodity_type { commodity_type }
	{
	}

	ERTKey( const ERTKey& other ) = default;

	ERTKey& operator=(const ERTKey& other) {
		id = other.id;
		commodity_type = other.commodity_type;
		return *this;
	}

	bool operator==(const ERTKey& other) const {
		return (id == other.id) && (commodity_type == other.commodity_type);
	}
};

struct ERTRecentEntry {
	using Key = ERTKey;

	// TODO: Is this the right choice of invalid key value?
	static const Key invalid_key;

	ert::ID id { ert::invalid_id };
	ert::CommodityType commodity_type { ert::invalid_commodity_type };

	size_t received_count { 0 };

	ert::Consumption last_consumption { };

	ERTRecentEntry(
		const Key& key
	) : id { key.id },
		commodity_type { key.commodity_type }
	{
	}

	Key key() const {
		return { id, commodity_type };
	}

	void update(const ert::Packet& packet);
};

class ERTLogger {
public:
	Optional<File::Error> append(const std::filesystem::path& filename) {
		return log_file.append(filename);
	}
	
	void on_packet(const ert::Packet& packet);

private:
	LogFile log_file { };
};

using ERTRecentEntries = RecentEntries<ERTRecentEntry>;

namespace ui {

using ERTRecentEntriesView = RecentEntriesView<ERTRecentEntries>;

class ERTAppView : public View {
public:
	static constexpr uint32_t initial_target_frequency = 911600000;
	static constexpr uint32_t sampling_rate = 4194304;
	static constexpr uint32_t baseband_bandwidth = 2500000;

	ERTAppView(NavigationView& nav);
	~ERTAppView();

	void set_parent_rect(const Rect new_parent_rect) override;

	// Prevent painting of region covered entirely by a child.
	// TODO: Add flag to View that specifies view does not need to be cleared before painting.
	void paint(Painter&) override { };

	void focus() override;

	std::string title() const override { return "ERT Meter RX"; };

private:
	ERTRecentEntries recent { };
	std::unique_ptr<ERTLogger> logger { };

	// app save settings
	std::app_settings 		settings { }; 		
	std::app_settings::AppSettings 	app_settings { };


	const RecentEntriesColumns columns { {
		{ "ID", 10 },
		{ "Tp", 2 },
		{ "Consumpt", 10 },
		{ "Cnt", 3 },
	} };
	ERTRecentEntriesView recent_entries_view { columns, recent };

	static constexpr auto header_height = 1 * 16;

	RFAmpField field_rf_amp {
		{ 13 * 8, 0 * 16 }
	};

	LNAGainField field_lna {
		{ 15 * 8, 0 * 16 }
	};

	VGAGainField field_vga {
		{ 18 * 8, 0 * 16 }
	};

	RSSI rssi {
		{ 21 * 8, 0, 6 * 8, 4 },
	};

	MessageHandlerRegistration message_handler_packet {
		Message::ID::ERTPacket,
		[this](Message* const p) {
			const auto message = static_cast<const ERTPacketMessage*>(p);
			const ert::Packet packet { message->type, message->packet };
			this->on_packet(packet);
		}
	};

	void on_packet(const ert::Packet& packet);
	void on_show_list();
};

} /* namespace ui */

#endif/*__ERT_APP_H__*/
