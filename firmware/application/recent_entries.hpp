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

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <list>
#include <utility>

template <class Entry>
using RecentEntries = std::list<Entry>;

template <typename ContainerType, typename Key>
typename ContainerType::const_iterator find(const ContainerType& entries, const Key key) {
    return std::find_if(
        std::begin(entries), std::end(entries),
        [key](typename ContainerType::const_reference e) { return e.key() == key; });
}

template <typename ContainerType, typename Key>
typename ContainerType::iterator find(ContainerType& entries, const Key key) {
    return std::find_if(
        std::begin(entries), std::end(entries),
        [key](typename ContainerType::const_reference e) { return e.key() == key; });
}

template <typename ContainerType>
static void truncate_entries(ContainerType& entries, const size_t entries_max = 64) {
    while (entries.size() > entries_max) {
        entries.pop_back();
    }
}

template <typename ContainerType, typename Key>
typename ContainerType::reference on_packet(ContainerType& entries, const Key key) {
    auto matching_recent = find(entries, key);
    if (matching_recent != std::end(entries)) {
        // Found within. Move to front of list, increment counter.
        entries.push_front(*matching_recent);
        entries.erase(matching_recent);
    } else {
        entries.emplace_front(key);
        truncate_entries(entries);
    }

    return entries.front();
}

template <typename ContainerType>
static std::pair<typename ContainerType::const_iterator, typename ContainerType::const_iterator> range_around(
    const ContainerType& entries,
    typename ContainerType::const_iterator item,
    const size_t count) {
    auto start = item;
    auto end = item;
    size_t i = 0;

    // Move start iterator toward first entry.
    while ((start != std::begin(entries)) && (i < count / 2)) {
        std::advance(start, -1);
        i++;
    }

    // Move end iterator toward last entry.
    while ((end != std::end(entries)) && (i < count)) {
        std::advance(end, 1);
        i++;
    }

    return {start, end};
}

template <typename ContainerType, typename KeySelector, typename SortOrder>
void sortEntriesBy(ContainerType& entries, KeySelector keySelector, SortOrder ascending) {
    entries.sort([keySelector, ascending](const auto& a, const auto& b) {
        return ascending ? keySelector(a) < keySelector(b) : keySelector(a) > keySelector(b);
    });
}

template <typename ContainerType, typename KeySelector>
void resetFilteredEntries(ContainerType& entries, KeySelector keySelector) {
    // Clear the filteredEntries container
    auto it = entries.begin();
    while (it != entries.end()) {
        if (keySelector(*it)) {
            entries.erase(it);  // Add a new entry to filteredEntries
        }
        ++it;  // Move to the next element, outside of the if block
    }
}

template <typename ContainerType, typename MemberPtr, typename KeyValue>
void setAllMembersToValue(ContainerType& entries, MemberPtr memberPtr, const KeyValue& keyValue) {
    for (auto& entry : entries) {
        // Check if the member specified by memberPtr is equal to keyValue
        if (entry.*memberPtr != keyValue) {
            // Update the member with keyValue
            entry.*memberPtr = keyValue;
        }
    }
}

namespace ui {

using RecentEntriesColumn = std::pair<std::string, size_t>;

class RecentEntriesColumns {
   public:
    using ContainerType = std::vector<RecentEntriesColumn>;

    RecentEntriesColumns(std::initializer_list<RecentEntriesColumn> columns);

    ContainerType::iterator begin() { return _columns.begin(); }
    ContainerType::iterator end() { return _columns.end(); }
    ContainerType::const_iterator begin() const { return _columns.begin(); }
    ContainerType::const_iterator end() const { return _columns.end(); }

    void set(size_t index, const std::string& name, size_t width) {
        if (index < _columns.size()) {
            _columns[index] = {name, width};
        }
    }

    const RecentEntriesColumn& at(size_t index) const {
        return _columns.at(index);
    }

    size_t size() const { return _columns.size(); }

   private:
    ContainerType _columns;
};

class RecentEntriesHeader : public Widget {
   public:
    RecentEntriesHeader(
        RecentEntriesColumns& columns);

    void paint(Painter& painter) override;

   private:
    RecentEntriesColumns& _columns;
};

template <class Entries>
class RecentEntriesTable : public Widget {
   public:
    using Entry = typename Entries::value_type;

    std::function<void(const Entry& entry)> on_select{};

    RecentEntriesTable(
        Entries& recent,
        RecentEntriesColumns& columns)
        : recent{recent},
          columns{columns} {
    }

    void paint(Painter& painter) override {
        const auto r = screen_rect();
        const auto& s = style();

        Rect target_rect{r.location(), {r.width(), s.font.line_height()}};
        const size_t visible_item_count = r.height() / s.font.line_height();

        set_focusable(!recent.empty());

        auto selected = find(recent, selected_key);
        if (selected == std::end(recent)) {
            selected = std::begin(recent);
        }

        auto range = range_around(recent, selected, visible_item_count);

        for (auto p = range.first; p != range.second; p++) {
            const auto& entry = *p;
            const auto is_selected_key = (selected_key == entry.key());
            const auto item_style = (has_focus() && is_selected_key) ? s.invert() : s;
            draw(entry, target_rect, painter, item_style, columns);
            target_rect += {0, target_rect.height()};
        }

        painter.fill_rectangle(
            {target_rect.left(), target_rect.top(), target_rect.width(), r.bottom() - target_rect.top()},
            style().background);
    }

    bool on_encoder(const EncoderEvent event) override {
        advance(event);
        return true;
    }

    bool on_key(const ui::KeyEvent event) override {
        if (event == ui::KeyEvent::Select) {
            if (on_select) {
                const auto selected = find(recent, selected_key);
                if (selected != std::end(recent)) {
                    on_select(*selected);
                    return true;
                }
            }
        } else if (event == ui::KeyEvent::Up) {
            if (selected_key == recent.front().key()) {
                return false;
            } else {
                advance(-1);
                return true;
            }
        } else if (event == ui::KeyEvent::Down) {
            if (selected_key == recent.back().key()) {
                return false;
            } else {
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
    RecentEntriesColumns& columns;
    using EntryKey = typename Entry::Key;
    EntryKey selected_key = Entry::invalid_key;

    void advance(const int32_t amount) {
        auto selected = find(recent, selected_key);
        if (selected == std::end(recent)) {
            if (recent.empty()) {
                selected_key = Entry::invalid_key;
            } else {
                selected_key = recent.front().key();
            }
        } else {
            if (amount < 0) {
                if (selected != std::begin(recent)) {
                    std::advance(selected, -1);
                }
            }
            if (amount > 0) {
                std::advance(selected, 1);
                if (selected == std::end(recent)) {
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
        const Style& style,
        RecentEntriesColumns& columns);
};

template <class Entries>
class RecentEntriesView : public View {
   public:
    using Entry = typename Entries::value_type;

    std::function<void(const Entry& entry)> on_select{};

    RecentEntriesView(
        RecentEntriesColumns& columns,
        Entries& recent)
        : _header{columns},
          _table{recent, columns} {
        // Re calculate the widths if we got any zero-width columns (max 1). That means 'fill all the remaining space'. Only 1 col can have that
        uint16_t total_width = 0;
        uint8_t zero_width_count = 0;
        for (const auto& column : columns) {
            if (column.second == 0) {
                zero_width_count++;
            } else {
                total_width += column.second;  // char count
            }
        }
        total_width += (columns.size() - 1);  // spaces between columns
        if (zero_width_count >= 1) {          // we do only for the first, other 0-s will be ignored
            const auto screen_width_chars = screen_width / UI_POS_DEFAULT_WIDTH;
            const auto remaining_width = screen_width_chars > total_width ? screen_width_chars - total_width : 0;
            uint8_t i = 0;
            for (const auto& column : columns) {
                if (column.second == 0) {
                    columns.set(i, column.first, remaining_width);
                    break;  // only the first 0-width column gets the remaining space
                }
                i++;
            }
        }

        add_children({
            &_header,
            &_table,
        });

        _table.on_select = [this](const Entry& entry) { if( this->on_select ) { this->on_select(entry); } };
    }

    void set_parent_rect(const Rect new_parent_rect) override {
        constexpr Dim scale_height = 16;

        View::set_parent_rect(new_parent_rect);
        _header.set_parent_rect({0, 0, new_parent_rect.width(), scale_height});
        _table.set_parent_rect({0, scale_height,
                                new_parent_rect.width(),
                                new_parent_rect.height() - scale_height});
    }

    void paint(Painter&) override {
        // Children completely cover this View, do not paint.
        // TODO: What happens here shouldn't matter if I do proper damage detection!
    }

    void focus() override {
        _table.focus();
    }

    void set_table(Entries& new_table) {
        _table = new_table;
    }

   private:
    RecentEntriesHeader _header;
    RecentEntriesTable<Entries> _table;
};

} /* namespace ui */

#endif /*__RECENT_ENTRIES_H__*/
