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
#include "ui_receiver.hpp"
#include "ui_rssi.hpp"
#include "ui_channel.hpp"
#include "app_settings.hpp"
#include "event_m0.hpp"

#include "log_file.hpp"

#include "recent_entries.hpp"

#include "tpms_packet.hpp"

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

	Optional<Pressure> last_pressure { };
	Optional<Temperature> last_temperature { };
	Optional<tpms::Flags> last_flags { };

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

using TPMSRecentEntries = RecentEntries<TPMSRecentEntry>;

class TPMSLogger {
public:
	Optional<File::Error> append(const std::filesystem::path& filename) {
		return log_file.append(filename);
	}
	
	void on_packet(const tpms::Packet& packet, const uint32_t target_frequency);

private:
	LogFile log_file { };
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

	std::string title() const override { return "TPMS Cars RX"; };

private:
	static constexpr uint32_t initial_target_frequency = 315000000;
	static constexpr uint32_t sampling_rate = 2457600;
	static constexpr uint32_t baseband_bandwidth = 1750000;

	// app save settings
	std::app_settings 		settings { }; 		
	std::app_settings::AppSettings 	app_settings { };

	MessageHandlerRegistration message_handler_packet {
		Message::ID::TPMSPacket,
		[this](Message* const p) {
			const auto message = static_cast<const TPMSPacketMessage*>(p);
			const tpms::Packet packet { message->packet, message->signal_type };
			this->on_packet(packet);
		}
	};

	static constexpr ui::Dim header_height = 1 * 16;

	ui::Rect view_normal_rect { };

	RSSI rssi {
		{ 21 * 8, 0, 6 * 8, 4 },
	};

	Channel channel {
		{ 21 * 8, 5, 6 * 8, 4 },
	};

	OptionsField options_band {
		{ 0 * 8, 0 * 16 },
		3,
		{
			{ "315", 315000000 },
			{ "433", 433920000 },
		}
	};

	OptionsField options_type {
		{ 5 * 8, 0 * 16 },
		3,
		{
			{ "kPa", 0 },
			{ "PSI", 1 }
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

	TPMSRecentEntries recent { };
	std::unique_ptr<TPMSLogger> logger { };

	const RecentEntriesColumns columns_kpa { {
		{ "Tp", 2 },
		{ "ID", 8 },
		{ "kPa", 3 },
		{ "C", 3 },
		{ "Cnt", 3 },
		{ "Fl", 2 },
	} };
	TPMSRecentEntriesView recent_entries_view_kpa { columns_kpa, recent };

	const RecentEntriesColumns columns_psi { {
		{ "Tp", 2 },
		{ "ID", 8 },
		{ "PSI", 3 },
		{ "C", 3 },
		{ "Cnt", 3 },
		{ "Fl", 2 },
	} };
	TPMSRecentEntriesView recent_entries_view_psi { columns_psi, recent };

	uint32_t target_frequency_ = initial_target_frequency;

	void on_packet(const tpms::Packet& packet);
	void on_show_list();
	void update_type();

	void on_band_changed(const uint32_t new_band_frequency);

	uint32_t target_frequency() const;
	void set_target_frequency(const uint32_t new_value);

	uint32_t tuning_frequency() const;
};

} /* namespace ui */

#endif/*__TPMS_APP_H__*/
