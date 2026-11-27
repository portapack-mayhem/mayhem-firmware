/*
 * Copyright (C) 2026 Ivan
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

/* WSPR uses the standard RecentEntries header and table. Its adapter
 * returns focus to the controls on Left. */
#ifndef __UI_STATION_LIST_H__
#define __UI_STATION_LIST_H__

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_painter.hpp"
#include "ui_font_fixed_8x16.hpp"

/* The standard recent-entries Header + Table (coordinator-owned) and the one
 * station model (T02). The table draws StationTable::entries() directly. */
#include "recent_entries.hpp"
#include "ui_station_table.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <string_view>

namespace ui::external_app::wspr_rx {

using stations::StationRecord;
using stations::StationTable;

/* Shared, host-compilable application geometry. The firmware view
 * (ui_wspr_rx.hpp) and the host preview/tests consume these constants, so the
 * preview cannot drift from the real layout.
 *
 * Physical screen: 240 x 320. SystemView reserves physical y0..16 for its
 * status strip and gives the pushed application a 240 x 304 parent rect at
 * physical y16 (NavigationView::update_view). Therefore app-local y 0..304
 * maps to physical y 16..320, and NOTHING may paint below app-local 304.
 *
 * The fixed constants describe the verified 304-px viewport; the *_at()
 * helpers derive the same bands from an actual parent height so a shorter
 * host harness cannot paint outside its parent.
 */
namespace wspr_layout {

constexpr Dim app_width = 240;
constexpr Dim physical_height = 320;
constexpr Dim system_status_height = 16;
constexpr Dim app_viewport_height = physical_height - system_status_height;  // 304
constexpr Dim app_origin_y = system_status_height;                           // physical y of local 0

/* Upper control bands. */
constexpr Dim radio_y = 0;
constexpr Dim radio_height = 16;
constexpr Dim settings_y = 16;
constexpr Dim settings_height = 16;
constexpr Dim buttons_y = 32;
constexpr Dim buttons_height = 24;
constexpr Dim status_y = 56;
constexpr Dim status_height = 16;

/* Standard header (CALL / N / GRID column labels). */
constexpr Dim header_y = 72;
constexpr Dim header_height = 16;

constexpr Dim list_y = 88;
constexpr Dim row_height = 16;
constexpr Dim list_height = 200;
constexpr size_t list_rows = list_height / row_height;

/* Last accepted REPORT line: the bottom line of the viewport. */
constexpr Dim last_y = 288;
constexpr Dim last_height = 16;

/* Viewport-relative derivations (same bands for the 304-px app viewport). */
constexpr Dim last_y_at(Dim viewport_h) {
    return (Dim)(viewport_h - last_height);
}
constexpr Dim list_height_at(Dim viewport_h) {
    return (Dim)(last_y_at(viewport_h) - list_y);
}

static_assert(app_viewport_height == 304,
              "SystemView status strip is 16 px on a 320-px screen");
static_assert(radio_y + radio_height == settings_y, "radio row 0..16");
static_assert(settings_y + settings_height == buttons_y, "settings row 16..32");
static_assert(buttons_y + buttons_height == status_y, "buttons 32..56");
static_assert(status_y + status_height == header_y, "status 56..72 meets header at 72");
static_assert(header_y + header_height == list_y, "header 72..88 meets list at 88");
static_assert(list_y + list_height == last_y);
static_assert(last_y + last_height == app_viewport_height);
static_assert(list_rows == 12);
static_assert(list_height_at(app_viewport_height) == list_height,
              "viewport-relative list height matches the constant");

}  // namespace wspr_layout

/* Bounded callsign field printed by the standard table. The CALL column is 9
 * characters wide; the marker occupies the first one and the callsign the
 * remaining 8 (8 characters is the widest protocol call; longer compound
 * calls are truncated in the row). */
constexpr size_t wspr_row_marker_col = 0;
constexpr size_t wspr_row_call_col = 1;
constexpr size_t wspr_row_call_chars = 8;
constexpr size_t wspr_row_n_col = 10;  // header x = 9*8 + 8 = 80 px = char 10
constexpr size_t wspr_row_n_chars = 3;
constexpr size_t wspr_row_grid_col = 14;  // header x = 80 + 3*8 + 8 = 112 px
constexpr size_t wspr_row_grid_chars = 4;
constexpr size_t wspr_row_pwr_col = 19;
constexpr size_t wspr_row_snr_col = 24;
constexpr size_t wspr_row_level_chars = 4;
constexpr size_t wspr_row_chars = wspr_row_snr_col + wspr_row_level_chars;

inline ui::RecentEntriesColumns station_columns() {
    return {{"CALL", 9}, {"N", 3}, {"GRID", 4}, {"PWR", 4}, {"SNR", 4}};
}
static_assert(wspr_row_chars <= wspr_layout::app_width / 8);
static_assert(wspr_row_call_col + wspr_row_call_chars == 9,
              "CALL column is 9 chars: marker + 8-char callsign");
static_assert(wspr_row_n_col == 10 && wspr_row_n_col + wspr_row_n_chars == 13,
              "N column starts at char 10 (x80) and is 3 chars wide");
static_assert(wspr_row_grid_col == 14,
              "GRID column starts at char 14 (x112), one blank after N");

class WsprStationTable : public ui::RecentEntriesTable<StationTable::Entries> {
   public:
    using Base = ui::RecentEntriesTable<StationTable::Entries>;
    using Base::Base;

    /* Left is consumed here so the framework never runs its geometric focus
     * search (nothing is left of a full-width list; the "stuck left" rule
     * would focus the Back widget and exit the app). Up/Down/encoder/Select
     * stay the standard table's. */
    std::function<void()> on_left{};

    bool on_key(const ui::KeyEvent event) override {
        if (event == ui::KeyEvent::Left) {
            if (on_left)
                on_left();
            return true;
        }
        return Base::on_key(event);
    }


};

}  // namespace ui::external_app::wspr_rx

namespace ui {

/* One-line standard row renderer for the WSPR station table. Fixed stack
 * buffer, no std::string, no allocation; the style passed by the table is
 * used unchanged (focused selection stays inverted). The line is padded with
 * spaces to the full row width so an inverted selection covers the whole row
 * (the same idiom the stock AIS/subcar renderers use). */
template <>
inline void RecentEntriesTable<ui::external_app::wspr_rx::StationTable::Entries>::draw(
    const Entry& entry,
    const Rect& target_rect,
    Painter& painter,
    const Style& style,
    RecentEntriesColumns&) {
    using namespace ui::external_app::wspr_rx;

    /* 240 px / 8 = 30 chars is the widest this viewport allows; the buffer is
     * one larger for the NUL. */
    char line[31];
    size_t width_chars = (size_t)(target_rect.width() / 8);
    if (width_chars > sizeof(line) - 1)
        width_chars = sizeof(line) - 1;
    if (width_chars < wspr_row_chars)
        width_chars = wspr_row_chars;
    std::memset(line, ' ', width_chars);
    line[width_chars] = '\0';

    /* Early single-slot reports use '?', ordinary first reports use '*'.
     * An independent matching repeat clears the marker. */
    line[wspr_row_marker_col] = entry.marker();

    /* Callsign, bounded to the 8 chars left of the 9-char CALL column. */
    for (size_t i = 0; i < wspr_row_call_chars && entry.callsign[i] != '\0'; i++)
        line[wspr_row_call_col + i] = entry.callsign[i];

    /* N right-aligned in chars 10..12 (N saturates at 999 in the model). */
    char num[8];
    std::snprintf(num, sizeof(num), "%u", (unsigned)entry.n_reports);
    const size_t num_len = std::strlen(num);
    const size_t shown = (num_len > wspr_row_n_chars) ? wspr_row_n_chars : num_len;
    std::memcpy(line + wspr_row_n_col + (wspr_row_n_chars - shown),
                num + (num_len - shown), shown);

    /* GRID at chars 14..17. */
    for (size_t i = 0; i < wspr_row_grid_chars && entry.grid[i] != '\0'; i++)
        line[wspr_row_grid_col + i] = entry.grid[i];

    std::snprintf(num, sizeof(num), "%4d", static_cast<int>(entry.dbm));
    std::memcpy(line + wspr_row_pwr_col, num, wspr_row_level_chars);
    std::snprintf(num, sizeof(num), "%4d", static_cast<int>(entry.snr_db));
    std::memcpy(line + wspr_row_snr_col, num, wspr_row_level_chars);

    painter.draw_string(
        target_rect.location(),
        style,
        std::string_view{line, width_chars});
}

}  // namespace ui

#endif /*__UI_STATION_LIST_H__*/
