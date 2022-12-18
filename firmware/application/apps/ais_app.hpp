/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __AIS_APP_H__
#define __AIS_APP_H__

#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "ui_rssi.hpp"
#include "ui_channel.hpp"

#include "ui_geomap.hpp"

#include "event_m0.hpp"

#include "log_file.hpp"
#include "app_settings.hpp"
#include "ais_packet.hpp"

#include "lpc43xx_cpp.hpp"
using namespace lpc43xx;

#include <cstdint>
#include <cstddef>
#include <string>
#include <list>
#include <utility>

#include <iterator>

#include "recent_entries.hpp"

struct AISPosition {
	rtc::RTC timestamp { };
	ais::Latitude latitude { };
	ais::Longitude longitude { };
	ais::RateOfTurn rate_of_turn { -128 };
	ais::SpeedOverGround speed_over_ground { 1023 };
	ais::CourseOverGround course_over_ground { 3600 };
	ais::TrueHeading true_heading { 511 };
};

struct AISRecentEntry {
	using Key = ais::MMSI;

	static constexpr Key invalid_key = 0xffffffff;

	ais::MMSI mmsi;
	std::string name;
	std::string call_sign;
	std::string destination;
	AISPosition last_position;
	size_t received_count;
	int8_t navigational_status;

	AISRecentEntry(
	) : AISRecentEntry { 0 }
	{
	}

	AISRecentEntry(
		const ais::MMSI& mmsi
	) : mmsi { mmsi },
		name { },
		call_sign { },
		destination { },
		last_position { },
		received_count { 0 },
		navigational_status { -1 }
	{
	}

	Key key() const {
		return mmsi;
	}

	void update(const ais::Packet& packet);
};

using AISRecentEntries = RecentEntries<AISRecentEntry>;

class AISLogger {
public:
	Optional<File::Error> append(const std::filesystem::path& filename) {
		return log_file.append(filename);
	}
	
	void on_packet(const ais::Packet& packet);

private:
	LogFile log_file { };
};

namespace ui {

using AISRecentEntriesView = RecentEntriesView<AISRecentEntries>;

class AISRecentEntryDetailView : public View {
public:
	std::function<void(void)> on_close { };

	AISRecentEntryDetailView(NavigationView& nav);

	void set_entry(const AISRecentEntry& new_entry);
	const AISRecentEntry& entry() const { return entry_; };

	void update_position();
	void focus() override;
	void paint(Painter&) override;

    AISRecentEntryDetailView(const AISRecentEntryDetailView&Entry);
    AISRecentEntryDetailView &operator=(const AISRecentEntryDetailView&Entry);

private:
	AISRecentEntry entry_ { };

	Button button_done {
		{ 125, 224, 96, 24 },
		"Done"
	};
	Button button_see_map {
		{ 19, 224, 96, 24 },
		"See on map"
	};
	GeoMapView* geomap_view { nullptr };
	bool send_updates { false };

	Rect draw_field(
		Painter& painter,
		const Rect& draw_rect,
		const Style& style,
		const std::string& label,
		const std::string& value
	);
};

class AISAppView : public View {
public:
	AISAppView(NavigationView& nav);
	~AISAppView();

	void set_parent_rect(const Rect new_parent_rect) override;

	// Prevent painting of region covered entirely by a child.
	// TODO: Add flag to View that specifies view does not need to be cleared before painting.
	void paint(Painter&) override { };

	void focus() override;

	std::string title() const override { return "AIS Boats RX"; };

private:
	static constexpr uint32_t initial_target_frequency = 162025000;
	static constexpr uint32_t sampling_rate = 2457600;
	static constexpr uint32_t baseband_bandwidth = 1750000;


	// app save settings
	std::app_settings 		settings { }; 		
	std::app_settings::AppSettings 	app_settings { };

	NavigationView& nav_;

	AISRecentEntries recent { };
	std::unique_ptr<AISLogger> logger { };

	const RecentEntriesColumns columns { {
		{ "MMSI", 9 },
		{ "Name/Call", 20 },
	} };
	AISRecentEntriesView recent_entries_view { columns, recent };
	AISRecentEntryDetailView recent_entry_detail_view { nav_ };

	static constexpr auto header_height = 1 * 16;

	Text label_channel {
		{ 0 * 8, 0 * 16, 2 * 8, 1 * 16 },
		"Ch"
	};

	OptionsField options_channel {
		{ 3 * 8, 0 * 16 },
		3,
		{
			{ "87B", 161975000 },
			{ "88B", 162025000 },
		}
	};

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

	Channel channel {
		{ 21 * 8, 5, 6 * 8, 4 },
	};

	MessageHandlerRegistration message_handler_packet {
		Message::ID::AISPacket,
		[this](Message* const p) {
			const auto message = static_cast<const AISPacketMessage*>(p);
			const ais::Packet packet { message->packet };
			if( packet.is_valid() ) {
				this->on_packet(packet);
			}
		}
	};

	uint32_t target_frequency_ = initial_target_frequency;

	void on_packet(const ais::Packet& packet);
	void on_show_list();
	void on_show_detail(const AISRecentEntry& entry);

	void on_frequency_changed(const uint32_t new_target_frequency);

	uint32_t target_frequency() const;
	void set_target_frequency(const uint32_t new_value);

	uint32_t tuning_frequency() const;
};

} /* namespace ui */

#endif/*__AIS_APP_H__*/
