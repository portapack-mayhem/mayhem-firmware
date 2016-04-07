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
