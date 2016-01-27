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

template<class Packet, class Entry>
class RecentEntries {
public:
	using EntryType = Entry;
	using Key = typename Entry::Key;
	using ContainerType = std::list<Entry>;
	using const_reference = typename ContainerType::const_reference;
	using const_iterator = typename ContainerType::const_iterator;
	using RangeType = std::pair<const_iterator, const_iterator>;
	
	const Entry& on_packet(const Key key, const Packet& packet) {
		auto matching_recent = find(key);
		if( matching_recent != std::end(entries) ) {
			// Found within. Move to front of list, increment counter.
			entries.push_front(*matching_recent);
			entries.erase(matching_recent);
		} else {
			entries.emplace_front(key);
			truncate_entries();
		}

		auto& entry = entries.front();
		entry.update(packet);

		return entry;
	}

	const_reference front() const {
		return entries.front();
	}

	const_iterator find(const Key key) const {
		return std::find_if(
			std::begin(entries), std::end(entries),
			[key](const Entry& e) { return e.key() == key; }
		);
	}

	const_iterator begin() const {
		return entries.begin();
	}

	const_iterator end() const {
		return entries.end();
	}

	bool empty() const {
		return entries.empty();
	}

	RangeType range_around(
		const_iterator item, const size_t count
	) const {
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

private:
	ContainerType entries;
	const size_t entries_max = 64;

	void truncate_entries() {
		while(entries.size() > entries_max) {
			entries.pop_back();
		}
	}
};

namespace ui {

template<class Entries>
class RecentEntriesView : public View {
public:
	using Entry = typename Entries::EntryType;

	std::function<void(const Entry& entry)> on_select;

	RecentEntriesView(
		Entries& recent
	) : recent { recent }
	{
		flags.focusable = true;
	}

	void paint(Painter& painter) override {
		const auto r = screen_rect();
		const auto& s = style();

		Rect target_rect { r.pos, { r.width(), s.font.line_height() }};
		const size_t visible_item_count = r.height() / s.font.line_height();

		const Style style_header {
			.font = font::fixed_8x16,
			.background = Color::blue(),
			.foreground = Color::white(),
		};

		draw_header(target_rect, painter, style_header);
		target_rect.pos.y += target_rect.height();

		auto selected = recent.find(selected_key);
		if( selected == std::end(recent) ) {
			selected = std::begin(recent);
		}

		auto range = recent.range_around(selected, visible_item_count);

		for(auto p = range.first; p != range.second; p++) {
			const auto& entry = *p;
			const auto is_selected_key = (selected_key == entry.key());
			draw(entry, target_rect, painter, s, (has_focus() && is_selected_key));
			target_rect.pos.y += target_rect.height();
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
				const auto selected = recent.find(selected_key);
				if( selected != std::end(recent) ) {
					on_select(*selected);
					return true;
				}
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
		auto selected = recent.find(selected_key);
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

	void draw_header(
		const Rect& target_rect,
		Painter& painter,
		const Style& style
	);

	void draw(
		const Entry& entry,
		const Rect& target_rect,
		Painter& painter,
		const Style& style,
		const bool is_selected
	);
};

} /* namespace ui */

#endif/*__RECENT_ENTRIES_H__*/
