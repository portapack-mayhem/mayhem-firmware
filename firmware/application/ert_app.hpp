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

#include "log_file.hpp"

#include "ert_packet.hpp"

#include "recent_entries.hpp"

#include <cstddef>
#include <string>

struct ERTRecentEntry {
	using Key = ert::ID;

	// TODO: Is this the right choice of invalid key value?
	static constexpr Key invalid_key = 0;

	ert::ID id { invalid_key };

	size_t received_count { 0 };

	ert::Consumption last_consumption;

	ERTRecentEntry(
		const Key& key
	) : id { key }
	{
	}

	Key key() const {
		return id;
	}

	void update(const ert::Packet& packet);
};

class ERTLogger {
public:
	void on_packet(const ert::Packet& packet);

private:
	LogFile log_file { "ert.txt" };
};

using ERTRecentEntries = RecentEntries<ert::Packet, ERTRecentEntry>;

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

	std::string title() const override { return "ERT"; };

private:
	ERTRecentEntries recent;
	ERTLogger logger;

	ERTRecentEntriesView recent_entries_view { recent };

	void on_packet(const ert::Packet& packet);
	void on_show_list();
};

} /* namespace ui */

#endif/*__ERT_APP_H__*/
