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

#ifndef __UI_STATION_LIST_H__
#define __UI_STATION_LIST_H__

/* WSPR RX station list, spec UI_SPEC_APPROVED_V1.md sections 4/5/6.
 *
 * One compact fixed-data StationListView widget (per spec section 5: a
 * two-line row does not fit MenuView's one-line MenuItemView, so this is a
 * dedicated leaf Widget, not a MenuView and not a framework change).
 *
 * Geometry (local WSPRRxView coordinates; the application viewport is
 * 240 x 304, not the raw 240 x 320 screen - SystemView reserves the top 16 px
 * for the status strip, so app-local y maps to physical y+16). The widget
 * derives its visible row count from its own parent rect height, capped at
 * 6 rows: the application gives it 5 rows ({0, 88, 240, 160}) so that the
 * header, candidate panel and LAST line also fit inside the 304-px viewport:
 *   - 5 fully visible rows of 32 px each (two 8x16 font lines per row);
 *   - 6 px scrollbar reserved at the right edge (x 234..239);
 *   - row line 1: marker '?'/' ' x0, callsign x8 (8 chars, chars 1..8),
 *     unique-REPORT count N (3 chars right-aligned, chars 9..11),
 *     locator GRID (4 chars, chars 13..16), audio frequency in Hz
 *     (4 digits right-aligned, chars 18..21);
 *   - row line 2: SNR and agreement (A%, a DISPLAY metric only) at chars
 *     0..13, age as its own string at char 24 with its own freshness colour.
 *     Worst case is 29 characters (232 px) left of the scrollbar.
 *   - the table header labels exactly those columns: CALL at char 1, N at
 *     char 11, GRID at char 13, FREQ at char 18. The visible-range summary
 *     sits on the same row in its own small right-hand widget: the header is
 *     the constant literal HEADER_TEXT (pinned to the same columns as
 *     format_header_line by a host test), and the summary is format_summary()
 *     drawn at x header_width.
 *
 * Focus / navigation contract for the coordinator:
 *   - the widget is focusable; the framework moves focus to it with the
 *     standard geometric search (Down from upper controls) or via touch;
 *   - while focused, Up/Down/encoder move the selection one record, stopping
 *     at the first/last record (no wrap); the encoder button (Select) and
 *     Right are consumed as no-ops, so they can never trigger CLEAR LIST or
 *     anything else;
 *   - Left is consumed by the widget and forwarded to the on_left callback.
 *     It must NOT be left unhandled: the list spans the full screen width,
 *     so the framework's geometric search has nothing left of it and the
 *     "stuck left" rule would move focus to the back widget (exit the app).
 *     The coordinator wires e.g.
 *         station_list.on_left = [this]() { button_clear_list.focus(); };
 *     (any upper control the navigation review prefers).
 *
 * Live updates: the coordinator calls notify_data_changed() after the
 * station table changed (new report, eviction, CLEAR LIST). The widget
 * never takes focus, never creates widgets or std::strings, and keeps the
 * selection by callsign identity: if the selected callsign is still visible
 * it stays selected in place; if it was evicted, the selection re-binds to
 * the record that took its index (or the last record). set_now() feeds the
 * monotonic clock used for the last-report age; call it (e.g. from the
 * per-second tick) or rows would show the age frozen at the value it had
 * when painted.
 *
 * Painting: only visible rows are painted, and inside a repaint only the
 * rows whose rendered content actually changed are redrawn (per-row render
 * cache). All formatting goes into fixed char buffers; no allocations.
 */

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_painter.hpp"
#include "ui_font_fixed_8x16.hpp"

/* The list consumes the fixed station table directly: its API
 * (count / visible_count / visible_record / empty_state) is the model
 * interface, no adapter layer in between. The table owns session order; the
 * widget owns selection, viewport and painting. */
#include "ui_station_table.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace ui::external_app::wspr_rx {

using stations::EmptyState;
using stations::StationRecord;
using stations::StationTable;

/* Shared, host-compilable application geometry. The firmware view
 * (ui_wspr_rx.hpp) and the host preview/geometry tests all consume these
 * constants, so the preview cannot drift from the real layout.
 *
 * Physical screen: 240 x 320. SystemView reserves y 0..16 for its status
 * strip and gives the pushed application a 240 x 304 parent rect at physical
 * y16 (ui_navigation.cpp: SystemView ctor + NavigationView::update_view).
 * Therefore app-local y 0..304 maps to physical y 16..320, and the old
 * "screen_height - 16 == 304" LAST line started at physical y320 and was
 * clipped. LAST is bottom-anchored above app-local y 288 so it also stays
 * clear of the 16-px info bar band if that bar is visible.
 *
 * At the real 304-px viewport this yields:
 *   header  y 72..88   (column labels + visible-range summary, v1.4 form)
 *   list    y 88..248   (5 rows x 32 px; row count derived from the viewport)
 *   cand    y 272..288  (candidate panel line, directly above LAST)
 *   LAST    y 288..304  (the last line of the viewport, physical 304..320)
 */
namespace wspr_layout {

constexpr Dim app_width = 240;
constexpr Dim physical_height = 320;
constexpr Dim system_status_height = 16;
constexpr Dim app_viewport_height = physical_height - system_status_height;  // 304
constexpr Dim app_origin_y = system_status_height;                           // physical y of local 0

/* Upper control bands (radio row, settings, buttons, STATUS line). They are
 * unchanged by the geometry repair but are part of the contained-child set. */
constexpr Dim radio_y = 0;
constexpr Dim radio_height = 16;
constexpr Dim settings_y = 16;
constexpr Dim settings_height = 16;
constexpr Dim buttons_y = 32;
constexpr Dim buttons_height = 24;
constexpr Dim status_y = 56;
constexpr Dim status_height = 16;

constexpr Dim header_y = 72;
constexpr Dim header_height = 16;
/* Header literal occupies 22 chars (" CALL           N GRID", GRID ends at
 * char 21); one blank char separates it from the visible-range summary
 * widget, which takes the remaining 7 chars. Seven is enough for every
 * reachable summary: 5 rows over a 12-station table tops out at "8-12/12". */
constexpr Dim header_width = 22 * 8;  // the 22-char literal
/* One blank character separates the literal from the summary widget. */
constexpr Dim summary_x = (22 + 1) * 8;
constexpr Dim summary_width = app_width - summary_x;  // 56 px = 7 chars
static_assert(summary_width >= 7 * 8, "summary needs 7 chars (\"8-12/12\")");
constexpr Dim list_y = 88;
constexpr Dim row_height = 32;
constexpr size_t max_list_rows = 6;
constexpr Dim last_height = 16;
/* Candidate panel: one compact 16 px line directly above LAST
 * (app-local 272..288). The station list keeps 5 rows at y 88..248. */
constexpr Dim cand_y = 272;
constexpr Dim cand_height = 16;
/* LAST occupies the very last visible line of the application viewport
 * (user feedback: it sat 16 px too high with a dead band below it).
 * app-local 288..304 = physical 304..320, the bottom of the screen. */
constexpr Dim last_y(Dim viewport_h) {
    return (Dim)(viewport_h - last_height);
}

/* The candidate panel occupies the band directly above LAST. */
constexpr Dim cand_y_at(Dim viewport_h) {
    return (Dim)(last_y(viewport_h) - cand_height);
}

constexpr size_t list_rows(Dim viewport_h) {
    const int available = (int)cand_y_at(viewport_h) - (int)list_y;
    if (available < (int)row_height)
        return 1;
    const size_t rows = (size_t)(available / (int)row_height);
    return rows > max_list_rows ? max_list_rows : rows;
}

constexpr Dim list_height(Dim viewport_h) {
    return (Dim)(list_rows(viewport_h) * row_height);
}

static_assert(app_viewport_height == 304,
              "SystemView status strip is 16 px on a 320-px screen");
static_assert(last_y(app_viewport_height) + last_height == app_viewport_height,
              "LAST must end exactly at the bottom of the viewport");
static_assert(cand_y_at(app_viewport_height) == cand_y,
              "the candidate panel sits in the band directly above LAST");
static_assert(cand_y >= list_y + list_height(app_viewport_height),
              "candidate panel must not overlap the list");
static_assert(list_rows(app_viewport_height) == 5,
              "the 304-px viewport holds 5 station rows (v1.4 form restored)");

}  // namespace wspr_layout

class StationListView : public Widget {
   public:
    static constexpr Dim row_height = 32;
    /* Maximum visible rows. The actual count follows the widget's parent
     * rect height (the application viewport gives it 5 rows; the host
     * harness can still construct the full 6-row widget). */
    static constexpr size_t max_visible_rows = wspr_layout::max_list_rows;
    static constexpr Dim scrollbar_width = 6;
    /* WSPR callsigns are at most 6 characters in the protocol; 8 leaves room
     * for the rare compound call without wasting a whole 4-character cell
     * (user feedback: the old 12-char field was mostly empty). */
    static constexpr size_t max_callsign_chars = 8;

    /* Render columns, in characters of the fixed 8x16 font. Shared by the
     * row renderer and the header formatter so the headings can never drift
     * away from the data. */
    static constexpr size_t col_marker = 0;
    static constexpr size_t col_callsign = 1;    // 8 chars, 1..8
    static constexpr size_t col_n = 9;           // 3-wide right-aligned, 9..11
    static constexpr size_t col_grid = 13;       // 4 chars, 13..16
    static constexpr size_t col_freq = 18;       // 4 digits right-aligned, 18..21
    static constexpr size_t col_snr_value = 4;   // 3-wide right-aligned (line 2)
    static constexpr size_t col_agr_value = 10;  // "A" + 3 + '%' (line 2)
    static constexpr size_t col_age = 24;        // age string (line 2)

    /* Header row literal, character-for-character the same layout as
     * format_header_line() below (a host test pins this). Kept as a constant
     * so the device costs no per-tick formatting code. */
    static constexpr size_t header_chars = 22;
    static constexpr const char* HEADER_TEXT = " CALL      N GRID FREQ";

    /* Age freshness thresholds for the age colour policy (ui_policy.md):
     * below FRESH the age uses the row colour, up to STALE_AGE_S it is
     * dark_yellow, beyond that dark_grey. Confirmation itself never depends
     * on these - only the age text is dimmed. */
    static constexpr uint32_t FRESH_AGE_S = 12 * 60;
    static constexpr uint32_t STALE_AGE_S = 60 * 60;

    /* Header line for the columns rendered on row line 1. The caller draws
     * it above the list; a host test asserts it matches the row columns. */
    static void format_header_line(char (&buf)[24]) {
        std::memset(buf, ' ', 22);
        buf[22] = '\0';
        std::memcpy(buf + col_callsign, "CALL", 4);  // chars 1..4
        buf[col_n + 2] = 'N';                        // char 11 (right edge)
        std::memcpy(buf + col_grid, "GRID", 4);      // chars 13..16
        std::memcpy(buf + col_freq, "FREQ", 4);      // chars 18..21
    }
    /* Station table capacity lives in the table (12 per spec section 4);
     * only the scroll geometry needs the visible row count above. */

    StationListView(Rect parent_rect)
        : Widget{parent_rect} {
        set_focusable(true);
    }

    StationListView(const StationListView&) = delete;
    StationListView& operator=(const StationListView&) = delete;

    /* Number of whole 32-px rows inside this widget's own rect, capped at
     * max_visible_rows. Derived at call time from the parent rect assigned by
     * the owning view, so a shorter application viewport automatically shows
     * fewer rows instead of painting outside its parent. */
    size_t visible_rows() const {
        const Dim h = screen_rect().height();
        size_t rows = (h >= (Dim)row_height) ? (size_t)(h / row_height) : 1;
        if (rows > max_visible_rows)
            rows = max_visible_rows;
        return rows;
    }

    /* Model must outlive the widget. */
    void set_model(const StationTable* const model) {
        model_ = model;
        reset_view();
    }

    /* The coordinator passes this to StationTable::report() so eviction can
     * avoid the selected record. Null when nothing is selected. */
    const char* selected_callsign() const {
        return has_selection_ ? selected_callsign_ : nullptr;
    }

    /* Call after any station-table change. Never touches focus and never
     * auto-selects: an unselected list stays unselected when records arrive. */
    void notify_data_changed() {
        if (!model_)
            return;

        const size_t total = model_->visible_count();
        if (total == 0) {
            /* Nothing visible (clear): drop the index, keep the callsign so
             * the identity can re-bind when it reappears. */
            offset_ = 0;
            has_selection_ = false;
            selected_index_ = 0;
        } else {
            bool rebound = false;
            if (selected_callsign_[0] != '\0') {
                for (size_t i = 0; i < total; i++) {
                    const auto rec = model_->visible_record(i);
                    if (rec && std::strncmp(rec->callsign, selected_callsign_,
                                            sizeof(selected_callsign_)) == 0) {
                        selected_index_ = i;
                        rebound = true;
                        break;
                    }
                }
            }
            if (!rebound) {
                /* Selected callsign not visible (evicted). Clamp to the
                 * nearest surviving record only if something had been
                 * selected before; a fresh list stays unselected. */
                if (has_selection_) {
                    if (selected_index_ >= total)
                        selected_index_ = total - 1;
                    const auto rec = model_->visible_record(selected_index_);
                    if (rec)
                        copy_callsign(rec->callsign);
                } else {
                    selected_index_ = 0;
                }
            }
            has_selection_ = rebound || has_selection_;

            const size_t max_offset =
                (total > visible_rows()) ? (total - visible_rows()) : 0;
            if (offset_ > max_offset)
                offset_ = max_offset;
            /* Keep the selection inside the viewport: an eviction ABOVE the
             * selection shifts it one index up, so a selection that sat on
             * the top visible row would silently move out of the window.
             * Mirror select()'s visibility rule after any re-binding. */
            if (has_selection_) {
                if (selected_index_ < offset_)
                    offset_ = selected_index_;
                else if (selected_index_ >= offset_ + visible_rows())
                    offset_ = selected_index_ - visible_rows() + 1;
            }
        }
        set_dirty();
    }

    /* Monotonic seconds now (chTimeNow() / CH_FREQUENCY), for the
     * last-report age rendered on row line 2. Confirmation is a session
     * property and is NOT time-dependent, so feeding the clock never changes
     * visibility or the confirmed marker; only the age text and its colour
     * advance. */
    void set_now(const uint32_t now_s) {
        if (now_s != now_s_) {
            now_s_ = now_s;
            set_dirty();
        }
    }

    /* Summary line contract for the coordinator's line BELOW the list:
     * "1-6/12", "7-12/12", ... over all visible records. Empty string when
     * the list is empty. Not part of the header row. */
    void format_summary(char (&buf)[16]) const {
        buf[0] = '\0';
        if (!model_)
            return;
        const size_t total = model_->visible_count();
        if (total == 0)
            return;
        const size_t first = offset_ + 1;
        const size_t last = (offset_ + visible_rows() < total)
                                ? (offset_ + visible_rows())
                                : total;
        std::snprintf(buf, sizeof(buf), "%u-%u/%u",
                      (unsigned)first, (unsigned)last, (unsigned)total);
    }

    /* Left exits the list upward; see the header comment. The coordinator
     * must wire this to an upper control (button, field). */
    std::function<void()> on_left{};

    void paint(Painter& painter) override;

    /* Keys: Up/Down mirror the encoder (stop at the ends, consume);
     * Left hands control to on_left; Select and Right are consumed no-ops
     * so the encoder button can never trigger CLEAR LIST / RX RESTART. */
    bool on_key(const KeyEvent event) override {
        switch (event) {
            case KeyEvent::Up:
                return on_encoder(-1);
            case KeyEvent::Down:
                return on_encoder(1);
            case KeyEvent::Left:
                if (on_left)
                    on_left();
                return true;
            case KeyEvent::Right:
            case KeyEvent::Select:
                return true;
            default:
                return false;
        }
    }

    bool on_encoder(const EncoderEvent event) override {
        if (!model_ || model_->visible_count() == 0)
            return true;
        if (!has_selection_) {
            select(0);
            return true;
        }
        int32_t target = (int32_t)selected_index_ + event;
        if (target < 0)
            target = 0;
        if (target >= (int32_t)model_->visible_count())
            target = (int32_t)model_->visible_count() - 1;
        if ((size_t)target != selected_index_)
            select((size_t)target);
        return true;
    }

    /* Touch selects the touched row and moves focus into the list. */
    bool on_touch(const TouchEvent event) override {
        if (event.type != TouchEvent::Type::Start &&
            event.type != TouchEvent::Type::End)
            return false;

        const auto rect = screen_rect();
        if (!rect.contains(event.point))
            return false;

        const int32_t row = (event.point.y() - rect.top()) / row_height;
        if (model_ && row >= 0 && row < (int32_t)visible_rows() &&
            (offset_ + (size_t)row) < model_->visible_count()) {
            select(offset_ + (size_t)row);
            focus();
        }
        return true;
    }

    void on_focus() override {
        full_repaint_ = true;
        set_dirty();
    }

    void on_blur() override {
        full_repaint_ = true;
        set_dirty();
    }

   private:
    static constexpr size_t line_chars = 29;  // 232 px usable, 6 px scrollbar

    struct RowRender {
        char line1[line_chars + 1]{};
        char line2[line_chars + 1]{};
        char age[10]{};  // drawn separately at col_age (own freshness colour)
        Color foreground{};
        Color background{};
        Color age_foreground{};
        bool valid{false};
    };

    void copy_callsign(const char* callsign) {
        std::strncpy(selected_callsign_, callsign, sizeof(selected_callsign_) - 1);
        selected_callsign_[sizeof(selected_callsign_) - 1] = '\0';
    }

    void reset_view() {
        offset_ = 0;
        has_selection_ = false;
        selected_index_ = 0;
        selected_callsign_[0] = '\0';
        full_repaint_ = true;
        set_dirty();
    }

    /* Move selection to index and keep it fully visible; viewport shifts by
     * one record at the edges. Stops at the ends (caller clamps, no wrap). */
    void select(const size_t index) {
        if (!model_)
            return;
        const size_t total = model_->visible_count();
        if (total == 0)
            return;
        selected_index_ = (index < total) ? index : total - 1;
        const auto rec = model_->visible_record(selected_index_);
        if (rec)
            copy_callsign(rec->callsign);
        has_selection_ = true;

        if (selected_index_ < offset_)
            offset_ = selected_index_;
        else if (selected_index_ >= offset_ + visible_rows())
            offset_ = selected_index_ - visible_rows() + 1;
        set_dirty();
    }

    /* Fixed-buffer formatting helpers; no std::string, no allocation. */
    static void format_padded_right(char* out, size_t width, const char* value) {
        const size_t len = std::strlen(value);
        if (len >= width) {
            std::memcpy(out, value + (len - width), width);
            return;
        }
        for (size_t i = 0; i < width - len; i++)
            out[i] = ' ';
        std::memcpy(out + (width - len), value, len);
    }

    static void format_age(const uint32_t age_s, char (&out)[10]) {
        if (age_s < 3600u)
            std::snprintf(out, sizeof(out), "%us", (unsigned)age_s);
        else if (age_s < 86400u)
            std::snprintf(out, sizeof(out), "%um", (unsigned)(age_s / 60u));
        else
            std::snprintf(out, sizeof(out), "%uh", (unsigned)(age_s / 3600u));
    }

    /* Age colour policy (ui_policy): a fresh age uses the row colour;
     * between FRESH_AGE_S and STALE_AGE_S it is
     * dark yellow; older it is dark grey. This dims only the age text - the
     * confirmation marker and colour never depend on age. */
    static Color age_color(const uint32_t age_s, const Color base) {
        if (age_s < FRESH_AGE_S)
            return base;
        if (age_s < STALE_AGE_S)
            return Color::dark_yellow();
        return Color::dark_grey();
    }

    /* Renders one record into the two-line buffers. Everything is bounded:
     * callsign at most 12 chars, grid 4, N saturated at 999 by the table
     * (WSPR_N_MAX), SNR clamped into [-99, 99], frequency clamped to
     * [0, 9999] so the %4d field is fixed width, agreement 0..100.
     *
     * Line 1 (columns from the shared constants):
     *   marker @0, callsign @1 (8), N @9..11 (right), GRID @13..16,
     *   FREQ @18..21 (right).
     * Line 2:
     *   "SNR <3>  A<3>%" at chars 0..13, age at char 24.
     *
     * confirmed_eff is the session confirmation state (no TTL applied).
     */
    void render_row(const StationRecord& rec, const bool confirmed_eff, RowRender& out) const {
        char buf[16];

        std::memset(out.line1, ' ', line_chars);
        out.line1[line_chars] = '\0';

        out.line1[col_marker] = confirmed_eff ? ' ' : '?';
        char call[max_callsign_chars + 1];
        std::strncpy(call, rec.callsign, max_callsign_chars);
        call[max_callsign_chars] = '\0';
        std::memcpy(out.line1 + col_callsign, call, std::strlen(call));

        std::snprintf(buf, sizeof(buf), "%u", (unsigned)rec.n_reports);
        format_padded_right(out.line1 + col_n, 3, buf);  // chars 10..12

        char grid[5];
        std::strncpy(grid, rec.grid, sizeof(grid) - 1);
        grid[sizeof(grid) - 1] = '\0';
        std::memcpy(out.line1 + col_grid, grid, std::strlen(grid));

        int snr = rec.snr_db;
        if (snr < -99)
            snr = -99;
        if (snr > 99)
            snr = 99;
        int freq = rec.freq_hz;
        if (freq < 0)
            freq = 0;
        if (freq > 9999)
            freq = 9999;
        std::snprintf(buf, sizeof(buf), "%d", freq);
        format_padded_right(out.line1 + col_freq, 4, buf);  // chars 19..22

        char tmp[24];
        std::snprintf(tmp, sizeof(tmp), "SNR %3d  A%3u%%", snr,
                      (unsigned)rec.agreement_pct);
        std::memset(out.line2, ' ', line_chars);
        out.line2[line_chars] = '\0';
        std::memcpy(out.line2, tmp, std::strlen(tmp));

        const uint32_t age_s =
            (now_s_ >= rec.last_report_s) ? (now_s_ - rec.last_report_s) : 0;
        format_age(age_s, out.age);
    }

    void paint_row(Painter& painter, const Rect row_rect, const RowRender& render) {
        painter.fill_rectangle(row_rect, render.background);
        const Point p0 = row_rect.location();
        painter.draw_string(p0, {font::fixed_8x16, render.background, render.foreground},
                            render.line1);
        painter.draw_string({p0.x(), (Coord)(p0.y() + 16)},
                            {font::fixed_8x16, render.background, render.foreground},
                            render.line2);
        /* Age is its own draw so it can carry a freshness colour without
         * changing the rest of line 2. Background matches the row so the
         * glyph cells blend in. */
        painter.draw_string({(Coord)(p0.x() + (Coord)(col_age * 8)),
                             (Coord)(p0.y() + 16)},
                            {font::fixed_8x16, render.background, render.age_foreground},
                            render.age);
    }

    /* Draws the full 6 px scrollbar strip. Called on EVERY paint (after the
     * rows): scrolling changes offset_ but not total, and only a full
     * repaint clears the whole area, so a change-gated draw can never keep
     * the thumb in sync - unconditional is simpler and cheap (6x160 px). */
    void paint_scrollbar(Painter& painter, const Rect rect, const size_t total) {
        const Rect track{rect.left() + rect.width() - (Coord)scrollbar_width,
                         rect.top(), (Dim)scrollbar_width, rect.height()};
        painter.fill_rectangle(track, Color::dark_grey());

        const Dim track_h = track.height();
        if (total == 0) {
            return;
        }
        if (total <= visible_rows()) {
            painter.fill_rectangle(track, Color::light_grey());
            return;
        }
        Dim top = (Dim)((offset_ * track_h) / total);
        Dim bottom = (Dim)(((offset_ + visible_rows()) * track_h) / total);
        if (bottom > track_h)
            bottom = track_h;
        if (bottom - top < 4)
            bottom = top + 4;
        if (bottom > track_h)
            bottom = track_h;
        painter.fill_rectangle({track.location().x(),
                                (Coord)(track.top() + top),
                                (Dim)scrollbar_width,
                                (Dim)(bottom - top)},
                               Color::light_grey());
    }

    const StationTable* model_{nullptr};
    size_t offset_{0};  // index of the topmost visible record
    size_t selected_index_{0};
    char selected_callsign_[13];
    bool has_selection_{false};
    uint32_t now_s_{0};
    bool full_repaint_{true};
    /* No scrollbar cache: the strip is drawn on every paint (see
     * paint_scrollbar), so no total/offset invalidation can go stale. */
    RowRender cache_[max_visible_rows]{};
};

inline void StationListView::paint(Painter& painter) {
    const auto rect = screen_rect();
    const auto base_style = style();
    const Color normal_bg = base_style.background;
    const Color confirmed_fg = Theme::getInstance()->fg_green->foreground;
    const Color unconfirmed_fg = Color::light_grey();

    const size_t total = model_ ? model_->visible_count() : 0;

    if (full_repaint_) {
        painter.fill_rectangle(rect, normal_bg);
        for (auto& row : cache_)
            row.valid = false;
        full_repaint_ = false;
    }

    if (total == 0) {
        /* Empty state, rendered inside the list area. There is no filter,
         * so the only empty state is an empty table. */
        RowRender render;
        render.foreground = unconfirmed_fg;
        render.background = normal_bg;
        render.age_foreground = unconfirmed_fg;
        render.valid = true;
        std::snprintf(render.line1, sizeof(render.line1), "%s",
                      "No reports yet");
        render.line2[0] = '\0';
        render.age[0] = '\0';

        const Rect row_rect{rect.location(),
                            {(Dim)(rect.width() - scrollbar_width), row_height}};
        if (!cache_[0].valid || std::strcmp(cache_[0].line1, render.line1) != 0 ||
            cache_[0].foreground.v != render.foreground.v ||
            cache_[0].background.v != render.background.v) {
            paint_row(painter, row_rect, render);
            cache_[0] = render;
        }
        /* Rows below the message may still hold pixels from a previously
         * filled list: clear them once (never over the scrollbar strip). */
        for (size_t r = 1; r < visible_rows(); r++) {
            if (cache_[r].valid) {
                painter.fill_rectangle(
                    {rect.location().x(),
                     (Coord)(rect.top() + (int)(r * row_height)),
                     (Dim)(rect.width() - scrollbar_width), row_height},
                    normal_bg);
                cache_[r].valid = false;
            }
        }
        paint_scrollbar(painter, rect, total);
        return;
    }

    /* Rows occupy the list width LEFT of the 6 px scrollbar gutter; the
     * scrollbar is drawn AFTER the rows on every paint so no row fill can
     * wipe it and the thumb always matches offset_. */
    for (size_t r = 0; r < visible_rows(); r++) {
        const size_t index = offset_ + r;
        const Rect row_rect{rect.location().x(),
                            (Coord)(rect.top() + (int)(r * row_height)),
                            (Dim)(rect.width() - scrollbar_width), row_height};

        if (index >= total) {
            /* Blank row below the last record: clear it once. */
            if (cache_[r].valid) {
                painter.fill_rectangle(row_rect, normal_bg);
                cache_[r].valid = false;
            }
            continue;
        }

        const auto rec = model_->visible_record(index);
        if (!rec)
            continue;

        RowRender render;
        const bool confirmed_eff = model_->effective_confirmed(*rec);
        render_row(*rec, confirmed_eff, render);

        const bool selected = has_selection_ && (index == selected_index_);
        render.foreground = confirmed_eff ? confirmed_fg : unconfirmed_fg;
        if (selected && has_focus()) {
            /* Focused selection: inverted row, MenuView idiom - the record
             * color becomes the background, text in the base background. */
            render.background = render.foreground;
            render.foreground = normal_bg;
        } else if (selected) {
            /* Blurred selection: keep the record colors, dim background. */
            render.background = Color::darker_grey();
        } else {
            render.background = normal_bg;
        }
        /* Age colour: freshness dimming on unselected rows; a selected row
         * uses its text colour so the dimming cannot hurt readability. */
        render.age_foreground =
            age_color(model_->age_s(*rec, now_s_), render.foreground);
        if (selected)
            render.age_foreground = render.foreground;

        const bool changed =
            !cache_[r].valid ||
            std::strcmp(cache_[r].line1, render.line1) != 0 ||
            std::strcmp(cache_[r].line2, render.line2) != 0 ||
            std::strcmp(cache_[r].age, render.age) != 0 ||
            cache_[r].foreground.v != render.foreground.v ||
            cache_[r].background.v != render.background.v ||
            cache_[r].age_foreground.v != render.age_foreground.v;
        if (changed) {
            paint_row(painter, row_rect, render);
            cache_[r] = render;
            cache_[r].valid = true;
        }
    }

    paint_scrollbar(painter, rect, total);
}

} /* namespace ui::external_app::wspr_rx */

#endif /*__UI_STATION_LIST_H__*/
