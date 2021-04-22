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

#ifndef __RECENT_ENTRIES_H__
#define __RECENT_ENTRIES_H__

#include "ui_widget.hpp"
#include "ui_font_fixed_8x16.hpp"

#include <cstddef>
#include <cstdint>
#include <list>
#include <utility>
#include <functional>
#include <iterator>
#include <algorithm>

template<class Entry>
using RecentEntries = std::list<Entry>;

template<typename ContainerType, typename Key>
typename ContainerType::const_iterator find(const ContainerType& entries, const Key key) {
	return std::find_if(
		std::begin(entries), std::end(entries),
		[key](typename ContainerType::const_reference e) { return e.key() == key; }
	);
}

template<typename ContainerType>
static void truncate_entries(ContainerType& entries, const size_t entries_max = 64) {
	while(entries.size() > entries_max) {
		entries.pop_back();
	}
}

template<typename ContainerType, typename Key>
typename ContainerType::reference on_packet(ContainerType& entries, const Key key) {
	auto matching_recent = find(entries, key);
	if( matching_recent != std::end(entries) ) {
		// Found within. Move to front of list, increment counter.
		entries.push_front(*matching_recent);
		entries.erase(matching_recent);
	} else {
		entries.emplace_front(key);
		truncate_entries(entries);
	}

	return entries.front();
}

template<typename ContainerType>
static std::pair<typename ContainerType::const_iterator, typename ContainerType::const_iterator> range_around(
	const ContainerType& entries,
	typename ContainerType::const_iterator item,
	const size_t count
) {
	auto start = item;
	auto end = item;
	size_t i = 0;

	// Move start iterator toward first entry.
	while( (start != std::begin(entries)) && (i < count / 2) ) {
		std::advance(start, -1);
		i++;
	}

	// Move end iterator toward last entry.
	while( (end != std::end(entries)) && (i < count) ) {
		std::advance(end, 1);
		i++;
	}

	return { start, end };
}

namespace ui {

using RecentEntriesColumn = std::pair<std::string, size_t>;

class RecentEntriesColumns {
public:
	using ContainerType = std::vector<RecentEntriesColumn>;

	RecentEntriesColumns(
		const std::initializer_list<RecentEntriesColumn> columns
	);

	ContainerType::const_iterator begin() const { return std::begin(_columns); }
	ContainerType::const_iterator end() const { return std::end(_columns); }

private:
	const ContainerType _columns;
};

class RecentEntriesHeader : public Widget {
public:
	RecentEntriesHeader(
		const RecentEntriesColumns& columns
	);

	void paint(Painter& painter) override;

private:
	const RecentEntriesColumns& _columns;
};

template<class Entries>
class RecentEntriesTable : public Widget {
public:
	using Entry = typename Entries::value_type;

	std::function<void(const Entry& entry)> on_select { };

	RecentEntriesTable(
		Entries& recent
	) : recent { recent }
	{
		set_focusable(true);
	}

	void paint(Painter& painter) override {
		const auto r = screen_rect();
		const auto& s = style();

		Rect target_rect { r.location(), { r.width(), s.font.line_height() }};
		const size_t visible_item_count = r.height() / s.font.line_height();

		auto selected = find(recent, selected_key);
		if( selected == std::end(recent) ) {
			selected = std::begin(recent);
		}

		auto range = range_around(recent, selected, visible_item_count);

		for(auto p = range.first; p != range.second; p++) {
			const auto& entry = *p;
			const auto is_selected_key = (selected_key == entry.key());
			const auto item_style = (has_focus() && is_selected_key) ? s.invert() : s;
			draw(entry, target_rect, painter, item_style);
			target_rect += { 0, target_rect.height() };
		}

		painter.fill_rectangle(
			{ target_rect.left(), target_rect.top(), target_rect.width(), r.bottom() - target_rect.top() },
			style().background
		);
	}

	bool on_encoder(const EncoderEvent event) override {
		advance(event);
		return true;
	}

	bool on_key(const ui::KeyEvent event) override {
		if( event == ui::KeyEvent::Select ) {
			if( on_select ) {
				const auto selected = find(recent, selected_key);
				if( selected != std::end(recent) ) {
					on_select(*selected);
					return true;
				}
			}
		}
		else if( event == ui::KeyEvent::Up ) {
			if(selected_key == recent.front().key()){
				return false;
			}
			else {
				advance(-1);
				return true;
			}
		}
		else if( event == ui::KeyEvent::Down ) {
			if( selected_key == recent.back().key()) {
				return false;
			}
			else {
				advance(1);
				return true;
			}
		}
		return false;
	}

	void on_focus() override {
		advance(0);
	}

private:
	Entries& recent;
	
	using EntryKey = typename Entry::Key;
	EntryKey selected_key = Entry::invalid_key;

	void advance(const int32_t amount) {
		auto selected = find(recent, selected_key);
		if( selected == std::end(recent) ) {
			if( recent.empty() ) {
				selected_key = Entry::invalid_key;
			} else {
				selected_key = recent.front().key();
			}
		} else {
			if( amount < 0 ) {
				if( selected != std::begin(recent) ) {
					std::advance(selected, -1);
				}
			}
			if( amount > 0 ) {
				std::advance(selected, 1);
				if( selected == std::end(recent) ) {
					return;
				}
			}
			selected_key = selected->key();
		}

		set_dirty();
	}

	void draw(
		const Entry& entry,
		const Rect& target_rect,
		Painter& painter,
		const Style& style
	);
};

template<class Entries>
class RecentEntriesView : public View {
public:
	using Entry = typename Entries::value_type;

	std::function<void(const Entry& entry)> on_select { };

	RecentEntriesView(
		const RecentEntriesColumns& columns,
		Entries& recent
	) : _header { columns },
		_table { recent }
	{
		add_children({
			&_header,
			&_table,
		});

		_table.on_select = [this](const Entry& entry) { if( this->on_select ) { this->on_select(entry); } };
	}

	void set_parent_rect(const Rect new_parent_rect) override {
		constexpr Dim scale_height = 16;

		View::set_parent_rect(new_parent_rect);
		_header.set_parent_rect({ 0, 0, new_parent_rect.width(), scale_height });
		_table.set_parent_rect({
			0, scale_height,
			new_parent_rect.width(),
			new_parent_rect.height() - scale_height
		});
	}

	void paint(Painter&) override {
		// Children completely cover this View, do not paint.
		// TODO: What happens here shouldn't matter if I do proper damage detection!
	}

	void focus() override {
		_table.focus();
	}

private:
	RecentEntriesHeader _header;
	RecentEntriesTable<Entries> _table;
};

} /* namespace ui */

#endif/*__RECENT_ENTRIES_H__*/
