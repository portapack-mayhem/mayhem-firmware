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

/* Host render harness for the WSPR RX StationListView (ui_station_list.hpp),
 * compiled against a minimal ui::Widget/ui::Painter shim (ui_shim/) but
 * against the REAL widget code, the REAL StationTable model and the REAL
 * fixed_8x16 glyph data. Covers the review points:
 *   - model wired -> one report through the same path -> row appears N=1;
 *   - header labels align with the rendered data columns;
 *   - two-line row: callsign/N/locator on line 1, SNR/agreement/freq/age on
 *     line 2;
 *   - 6+ records: scrolling changes offset at constant total, the scrollbar
 *     strip (x 234..239) reflects offset_ on every repaint and is never
 *     wiped by row paints (rows are 234 px, strip drawn after rows);
 *   - focus / blur repaints keep the scrollbar valid;
 *   - session confirmation persists through silence past the old TTL while
 *     the age keeps growing and dimming; eviction above the selection keeps
 *     the selected row visible.
 */

#include "ui_station_list.hpp"

#include <doctest.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

using namespace ui::external_app::wspr_rx;
using stations::EmptyState;
using stations::StationTable;
using stations::WSPR_STATION_MAX;

namespace {

/* Widget geometry for the existing widget-level cases: the list lives at
 * app-local y88 and is constructed with the maximum 6 rows (192 px, 6 px
 * scrollbar gutter at x234). The real application viewport only has room for
 * 5 rows; that full-frame layout is covered by the geometry test at the end
 * of this file (wspr_layout). */
constexpr int SCREEN_W = 240;
constexpr int SCREEN_H = 320;
constexpr int LIST_Y = 88;
constexpr int LIST_H = 192;
constexpr int SB_X = 234;

/* Monotonic seconds; consecutive physical TX identities are a whole WSPR
 * slot period (352 block-rows) apart, like the model tests. */
constexpr uint32_t BR_STEP = 352;

struct Harness {
    StationTable table;
    ui::HostFramebuffer fb;
    ui::Painter painter;
    StationListView list{{0, LIST_Y, SCREEN_W, LIST_H}};

    Harness() {
        fb.reset(SCREEN_W, SCREEN_H);
        painter.target(&fb);
        list.set_model(&table);
        list.focus();
    }

    /* Incremental repaint: pixel state persists (the strip is drawn on
     * every paint), the draw log is cleared. */
    void paint() {
        painter.target(&fb);
        fb.strings.clear();
        fb.fills.clear();
        list.paint(painter);
        list.clear_dirty();
    }

    /* Full repaint from a clean framebuffer: blur() forces the widget's
     * full_repaint_ path so every visible row is (re)drawn, then focus()
     * restores the focused state and repaints again (full, inverted
     * selection idiom). The frame reflects the complete focused widget. */
    void paint_full() {
        fb.reset(SCREEN_W, SCREEN_H);
        painter.target(&fb);
        list.blur();
        list.paint(painter);
        list.focus();
        fb.strings.clear();
        fb.fills.clear();
        list.paint(painter);
        list.clear_dirty();
    }

    /* Foreground color used for row r's line 1 in the last paint. */
    uint16_t fg_of_row(const size_t r) const {
        const int y = LIST_Y + (int)(r * StationListView::row_height);
        for (const auto& s : fb.strings)
            if (s.p.y() == y)
                return s.style.foreground.v;
        return 0;
    }

    void report(const char* call, const char* grid, uint32_t now_s, uint32_t br, uint8_t agreement = 90, uint16_t freq_hz = 1400) {
        /* The coordinator's exact wiring: selection protection, report,
         * then notify_data_changed + set_now (the same path the view
         * integration uses on device). */
        table.report(call, grid, 30, freq_hz, -5, agreement, now_s, br, 40,
                     list.selected_callsign());
        list.notify_data_changed();
        list.set_now(now_s);
    }

    /* First line-1 draw at row r (y = LIST_Y + r*32), or empty string. */
    std::string line1_of_row(const size_t r) const {
        const int y = LIST_Y + (int)(r * StationListView::row_height);
        for (const auto& s : fb.strings)
            if (s.p.y() == y)
                return s.text;
        return {};
    }

    /* First line-2 draw at row r (the SNR/agreement/frequency segment). */
    std::string line2_of_row(const size_t r) const {
        const int y = LIST_Y + (int)(r * StationListView::row_height) + 16;
        for (const auto& s : fb.strings)
            if (s.p.x() == 0 && s.p.y() == y)
                return s.text;
        return {};
    }

    /* Age segment drawn at its own column (x = col_age * 8). */
    std::string age_of_row(const size_t r) const {
        const int x = (int)(StationListView::col_age * 8);
        const int y = LIST_Y + (int)(r * StationListView::row_height) + 16;
        for (const auto& s : fb.strings)
            if (s.p.x() == x && s.p.y() == y)
                return s.text;
        return {};
    }

    uint16_t age_fg_of_row(const size_t r) const {
        const int x = (int)(StationListView::col_age * 8);
        const int y = LIST_Y + (int)(r * StationListView::row_height) + 16;
        for (const auto& s : fb.strings)
            if (s.p.x() == x && s.p.y() == y)
                return s.style.foreground.v;
        return 0;
    }

    /* True when the callsign is currently rendered inside the viewport. */
    bool row_visible(const std::string& call) const {
        for (const auto& s : fb.strings)
            if (s.p.y() >= LIST_Y && s.p.y() < LIST_Y + LIST_H &&
                s.text.find(call) != std::string::npos)
                return true;
        return false;
    }

    /* Scrollbar strip: EVERY pixel of x234..239 inside the list band must
     * be track (dark_grey) or thumb (light_grey) - rows may never leave
     * stale content in the gutter. */
    bool scrollbar_painted() const {
        for (int y = LIST_Y; y < LIST_Y + LIST_H; y++)
            for (int x = SB_X; x < SCREEN_W; x++) {
                const uint16_t v = fb.at(x, y);
                if (v != ui::Color::dark_grey().v &&
                    v != ui::Color::light_grey().v)
                    return false;
            }
        return true;
    }

    /* Y of the first light_grey thumb pixel in the strip, -1 if none. */
    int thumb_top() const {
        for (int y = LIST_Y; y < LIST_Y + LIST_H; y++)
            if (fb.at(SB_X + 2, y) == ui::Color::light_grey().v)
                return y;
        return -1;
    }

    std::string summary() const {
        char buf[16];
        list.format_summary(buf);
        return std::string(buf);
    }
};

/* ---- Full-system-frame geometry harness (review P1) --------------------
 * The application is not the raw screen: SystemView keeps physical y0..16 and
 * gives the pushed view a 240x304 parent rect at physical y16. This harness
 * renders the COMPLETE 240x320 frame (system strip + application translated
 * by +16) using the shared wspr_layout coordinates, so the assertions below
 * cover integration, not just list-widget pixels.
 */
constexpr int APP_TOP = wspr_layout::app_origin_y;       // 16
constexpr int APP_W = wspr_layout::app_width;            // 240
constexpr int APP_H = wspr_layout::app_viewport_height;  // 304
constexpr int APP_CAND_Y = wspr_layout::cand_y;          // 272
constexpr int APP_LIST_Y = wspr_layout::list_y;          // 88
constexpr int APP_LIST_H =
    wspr_layout::list_height(wspr_layout::app_viewport_height);  // 128
constexpr int APP_LAST_Y =
    wspr_layout::last_y(wspr_layout::app_viewport_height);  // 288
constexpr const char* LAST_TEXT = "LAST 12:34 STK 2048";
constexpr const char* CAND_TEXT = "... 1493Hz 12s";

bool rect_inside(const ui::Rect r, const int w, const int h) {
    return r.left() >= 0 && r.top() >= 0 && r.right() <= w && r.bottom() <= h;
}

bool rects_overlap(const ui::Rect a, const ui::Rect b) {
    return a.left() < b.right() && b.left() < a.right() &&
           a.top() < b.bottom() && b.top() < a.bottom();
}

struct FullFrameHarness {
    StationTable table;
    ui::HostFramebuffer fb;
    ui::Painter painter;
    StationListView list{{0, (ui::Coord)APP_LIST_Y, (ui::Dim)APP_W,
                          (ui::Dim)APP_LIST_H}};

    FullFrameHarness() {
        fb.reset(APP_W, wspr_layout::physical_height);
        painter.target(&fb);
        list.set_model(&table);
        list.focus();
    }

    void report(const char* call, const char* grid, uint32_t now_s, uint32_t br) {
        table.report(call, grid, 30, 1400, -5, 90, now_s, br, 40,
                     list.selected_callsign());
        list.notify_data_changed();
        list.set_now(now_s);
    }

    /* Complete system frame: status strip at physical y0..16, then the
     * application drawn at its real origin (painter translated by +16). */
    void render() {
        fb.reset(APP_W, wspr_layout::physical_height);
        painter.target(&fb);

        /* SystemView status strip. */
        painter.set_origin({0, 0});
        painter.fill_rectangle({0, 0, APP_W, APP_TOP},
                               ui::Color::darker_grey());
        painter.draw_string({2, 0},
                            {ui::font::fixed_8x16, ui::Color::darker_grey(),
                             ui::Color::light_grey()},
                            "< WSPR RX");

        /* Application at physical y+16, in app-local coordinates. */
        painter.set_origin({0, APP_TOP});
        painter.fill_rectangle({0, 0, APP_W, APP_H}, ui::Color::black());

        /* Header row (v1.4 form): the constant column-label literal and
         * the visible-range summary are two widgets on the same row, like
         * the device draws them. */
        {
            char header[24];
            StationListView::format_header_line(header);
            painter.draw_string({0, wspr_layout::header_y},
                                {ui::font::fixed_8x16, ui::Color::black(),
                                 ui::Color::light_grey()},
                                header);
            char summary[16];
            summary[0] = '\0';
            list.format_summary(summary);
            painter.draw_string({wspr_layout::summary_x,
                                 wspr_layout::header_y},
                                {ui::font::fixed_8x16, ui::Color::black(),
                                 ui::Color::light_grey()},
                                summary);
        }

        /* Candidate panel line , in the old summary band
         * directly above LAST, exactly where the device Text widget sits. */
        painter.draw_string({0, (ui::Coord)APP_CAND_Y},
                            {ui::font::fixed_8x16, ui::Color::black(),
                             ui::Color::dark_yellow()},
                            CAND_TEXT);

        /* Force a complete list repaint (blur primes the full_repaint_ path,
         * focus restores the inverted selection idiom). */
        list.blur();
        list.paint(painter);
        list.focus();
        list.paint(painter);

        painter.draw_string({0, (ui::Coord)APP_LAST_Y},
                            {ui::font::fixed_8x16, ui::Color::black(),
                             ui::Color::dark_grey()},
                            LAST_TEXT);
    }

    /* Non-background pixels in a physical screen band. */
    int painted_pixels(const int y0, const int y1) const {
        int n = 0;
        for (int y = y0; y < y1; y++)
            for (int x = 0; x < APP_W; x++)
                if (fb.at(x, y) != ui::Color::black().v)
                    n++;
        return n;
    }

    int pixels_of(const int y0, const int y1, const ui::Color c) const {
        int n = 0;
        for (int y = y0; y < y1; y++)
            for (int x = 0; x < APP_W; x++)
                if (fb.at(x, y) == c.v)
                    n++;
        return n;
    }

    const ui::HostFramebuffer::StringDraw* find_text(const int physical_y,
                                                     const char* text) const {
        for (const auto& s : fb.strings)
            if (s.p.y() == physical_y && s.text == text)
                return &s;
        return nullptr;
    }
};

}  // namespace

TEST_CASE("station list view: model wiring renders reports") {
    SUBCASE("empty model: no-reports message") {
        Harness h;
        CHECK(h.table.empty_state() == EmptyState::NoReports);
        h.paint_full();
        CHECK(h.fb.strings.size() > 0);
        CHECK(h.line1_of_row(0) == "No reports yet");
        CHECK(h.scrollbar_painted());  // strip drawn even when empty
    }

    SUBCASE("one report through the same path: row appears with N=1") {
        Harness h;
        h.report("A1AAA", "JN49", 10, 1000);
        REQUIRE(h.table.count() == 1);
        CHECK(h.table.find("A1AAA")->n_reports == 1);
        h.paint_full();
        const std::string line1 = h.line1_of_row(0);
        CHECK(line1[0] == '?');  // unconfirmed marker
        CHECK(line1.substr(StationListView::col_callsign, 5) == "A1AAA");
        CHECK(line1.substr(StationListView::col_n, 3) == "  1");  // N right-aligned
        CHECK(line1.substr(StationListView::col_grid, 4) == "JN49");
        // Frequency now sits on line 1 (user feedback: use the empty space
        // right of GRID); line 2 keeps SNR + agreement, age is separate.
        CHECK(line1.substr(StationListView::col_freq, 4) == "1400");
        CHECK(h.line2_of_row(0).substr(0, 14) == "SNR  -5  A 90%");
        CHECK(h.age_of_row(0) == "0s");
        CHECK(h.scrollbar_painted());
        CHECK(h.summary() == "1-1/1");
    }
}

TEST_CASE("station list view: header aligns with the rendered data columns") {
    char hdr[24];
    StationListView::format_header_line(hdr);
    CHECK(std::string(hdr + StationListView::col_callsign, 4) == "CALL");
    CHECK(hdr[StationListView::col_n + 2] == 'N');  // right edge of the N field
    CHECK(std::string(hdr + StationListView::col_grid, 4) == "GRID");
    CHECK(std::string(hdr + StationListView::col_freq, 4) == "FREQ");
    // The device draws the constant literal; it must match the formatter.
    CHECK(std::string(hdr) == StationListView::HEADER_TEXT);
    // The page summary is NOT in the header row any more.
    CHECK(std::string(hdr).find('/') == std::string::npos);

    // The data really lands on those columns.
    Harness h;
    h.report("R3DNC", "KO96", 10, 1000, 90, 1447);
    h.paint_full();
    const std::string line1 = h.line1_of_row(0);
    CHECK(line1.substr(StationListView::col_callsign, 5) == "R3DNC");
    CHECK(line1.substr(StationListView::col_n, 3) == "  1");
    CHECK(line1.substr(StationListView::col_grid, 4) == "KO96");
    CHECK(line1.substr(StationListView::col_freq, 4) == "1447");
    CHECK(h.summary() == "1-1/1");
}

TEST_CASE("station list view: 8-char callsign field, longer calls truncate") {
    Harness h;
    // The longest realistic WSPR call fits the 8-char field.
    h.report("ZZ9XYZW1", "AA00", 0, 1000, 100);
    h.paint_full();
    std::string line1 = h.line1_of_row(0);
    CHECK(line1.substr(StationListView::col_callsign,
                       StationListView::max_callsign_chars) == "ZZ9XYZW1");
    CHECK(line1.substr(StationListView::col_n, 3) == "  1");
    CHECK(line1.substr(StationListView::col_grid, 4) == "AA00");
    CHECK(line1.substr(StationListView::col_freq, 4) == "1400");
    CHECK(h.line2_of_row(0).substr(0, 14) == "SNR  -5  A100%");

    // Anything longer is clipped to the field width, never into N/GRID.
    Harness h2;
    h2.report("ZZ9XYZW1AWQ0", "AA00", 0, 1000, 100);
    h2.paint_full();
    line1 = h2.line1_of_row(0);
    CHECK(line1.substr(StationListView::col_callsign,
                       StationListView::max_callsign_chars) == "ZZ9XYZW1");
    CHECK(line1.substr(StationListView::col_n, 3) == "  1");
    CHECK(line1.substr(StationListView::col_grid, 4) == "AA00");
}

TEST_CASE("station list view: scrolling at constant total keeps scrollbar") {
    Harness h;
    // 7 records with 6 visible rows: the viewport must scroll by exactly one.
    for (unsigned i = 0; i < 7; i++) {
        char call[13];
        std::snprintf(call, sizeof(call), "C3CC%u", i);
        h.report(call, "JO59", i * 10 + 1, 500 + i * BR_STEP);
    }
    REQUIRE(h.table.count() == 7);
    CHECK(h.table.visible_count() == 7);

    h.paint_full();
    CHECK(h.list.has_focus());
    const int thumb_top_view0 = h.thumb_top();
    REQUIRE(thumb_top_view0 >= LIST_Y);
    CHECK(h.scrollbar_painted());
    CHECK(h.summary() == "1-6/7");

    /* Encoder down to the 7th record: offset changes, total does not.
     * The first press selects record 0 (the list starts unselected). */
    for (unsigned i = 0; i < 7; i++)
        h.list.on_encoder(1);
    h.paint();
    CHECK(h.summary() == "2-7/7");
    CHECK(h.scrollbar_painted());
    const int thumb_top_view1 = h.thumb_top();
    CHECK(thumb_top_view1 > thumb_top_view0);  // thumb followed offset_
    // Row 0 now shows C3CC1, not C3CC0 (full frame view).
    h.paint_full();
    CHECK(h.line1_of_row(0).find("C3CC1") != std::string::npos);

    /* Back to the top (selection 0 pulls the viewport with it): strip
     * valid, thumb back at the top. */
    for (unsigned i = 0; i < 6; i++)
        h.list.on_encoder(-1);
    h.paint();
    CHECK(h.scrollbar_painted());
    CHECK(h.thumb_top() == thumb_top_view0);
    CHECK(h.summary() == "1-6/7");
}

TEST_CASE("station list view: focus and blur repaints keep the scrollbar") {
    Harness h;
    for (unsigned i = 0; i < 7; i++) {
        char call[13];
        std::snprintf(call, sizeof(call), "D4DD%u", i);
        h.report(call, "JO59", i * 10 + 1, 700 + i * BR_STEP);
    }
    REQUIRE(h.table.visible_count() == 7);
    h.list.on_encoder(1);  // select index 0 (first press of an idle list)
    h.list.on_encoder(1);  // index 1
    h.paint_full();
    REQUIRE(h.scrollbar_painted());
    const int thumb_focused = h.thumb_top();

    SUBCASE("blur repaint: strip survives the full repaint") {
        h.list.blur();
        h.paint();  // full_repaint_ path, clean-frame assertions follow
        CHECK(h.scrollbar_painted());
        CHECK(h.thumb_top() == thumb_focused);
        h.paint_full();
        CHECK(h.scrollbar_painted());
        CHECK(h.summary() == "1-6/7");
    }

    SUBCASE("re-focus repaint: strip still valid, selection inverted") {
        h.list.blur();
        h.paint();
        h.list.focus();
        h.paint();
        CHECK(h.scrollbar_painted());
        CHECK(h.thumb_top() == thumb_focused);
        // Focused selection is inverted: the record color (grey, the row
        // is unconfirmed) becomes the row background, text dark.
        CHECK(h.fb.at(8, LIST_Y + StationListView::row_height + 2) ==
              ui::Color::light_grey().v);
    }
}

TEST_CASE("station list view: confirmation persists through silence") {
    Harness h;
    /* Confirm A1AAA: two independent TX with the same payload. */
    h.report("A1AAA", "JN49", 0, 4000);
    h.report("A1AAA", "JN49", 120, 4000 + BR_STEP);
    REQUIRE(h.table.record(0)->confirmed);
    CHECK(h.table.find("A1AAA")->n_reports == 2);

    h.paint_full();
    CHECK(h.line1_of_row(0)[0] == ' ');  // confirmed: no '?'
    CHECK(h.fg_of_row(0) == ui::Theme::getInstance()->fg_green->foreground.v);
    CHECK(h.age_of_row(0) == "0s");
    CHECK(h.age_fg_of_row(0) == ui::Color::green().v);  // fresh: row colour

    /* Far past the old 12-minute TTL, with NO new report: the row stays
     * confirmed (green, no '?'), only the age advanced and dimmed. This is
     * the core requirement - silence must not un-confirm a payload. */
    h.list.set_now((12u * 60u) + 121);
    h.paint_full();
    CHECK(h.line1_of_row(0)[0] == ' ');
    CHECK(h.fg_of_row(0) == ui::Theme::getInstance()->fg_green->foreground.v);
    CHECK(h.age_of_row(0) == "721s");                         // 841 - 120 (last report)
    CHECK(h.age_fg_of_row(0) == ui::Color::dark_yellow().v);  // 12..60 min
    CHECK(h.table.count() == 1);
    CHECK(h.table.find("A1AAA")->n_reports == 2);
    CHECK(h.table.visible_count() == 1);
    CHECK(h.table.is_confirmed(*h.table.record(0)));
    CHECK(h.table.effective_confirmed(*h.table.record(0)));
    CHECK(h.summary() == "1-1/1");

    /* An hour plus: still confirmed, age dimmed to dark grey. */
    h.list.set_now(3600 + 121);  // age 3601 s -> "60m"
    h.paint_full();
    CHECK(h.line1_of_row(0)[0] == ' ');
    CHECK(h.fg_of_row(0) == ui::Theme::getInstance()->fg_green->foreground.v);
    CHECK(h.age_of_row(0) == "60m");
    CHECK(h.age_fg_of_row(0) == ui::Color::dark_grey().v);
    CHECK(h.table.count() == 1);
    CHECK(h.table.find("A1AAA")->n_reports == 2);
}

TEST_CASE("station list view: every accepted report is visible") {
    Harness h;
    /* Low agreement (1%, 0%) and a confirmed record coexist; nothing hides. */
    h.report("E5EEE", "JN49", 0, 9000, 1);
    h.report("F6FFF", "JO59", 10, 9000 + BR_STEP, 0);
    h.report("E5EEE", "JN49", 120, 9000 + 2 * BR_STEP, 1);
    REQUIRE(h.table.record(0)->confirmed);
    CHECK(h.table.count() == 2);
    CHECK(h.table.visible_count() == 2);
    CHECK(h.table.empty_state() == EmptyState::Reports);

    h.paint_full();
    CHECK(h.row_visible("E5EEE"));
    CHECK(h.row_visible("F6FFF"));
    CHECK(h.line1_of_row(0)[0] == ' ');  // confirmed marker
    CHECK(h.line1_of_row(1)[0] == '?');  // unconfirmed marker
    CHECK(h.summary() == "1-2/2");
}

TEST_CASE("station list view: eviction above the selection keeps it visible") {
    char summary[16];

    SUBCASE("viewport at the bottom, first record evicted") {
        Harness h;
        for (unsigned i = 0; i < 12; i++) {
            char call[13];
            std::snprintf(call, sizeof(call), "G7GG%02u", i);
            h.report(call, "JN58", i + 1, 800 + i * BR_STEP);
        }
        REQUIRE(h.table.count() == WSPR_STATION_MAX);
        /* Encoder to the last record puts the viewport at the bottom
         * (offset 7). The first press selects record 0. */
        for (unsigned i = 0; i < 12; i++)
            h.list.on_encoder(1);
        h.paint_full();
        CHECK(h.summary() == "7-12/12");
        CHECK(h.row_visible("G7GG11"));

        /* The 13th station evicts G7GG00 (LRU, ABOVE the selection) while
         * the viewport sits at the bottom. The selected row must stay
         * visible. */
        h.report("H8HH00", "JN58", 200, 800 + 12 * BR_STEP);
        REQUIRE(h.table.count() == 12);
        CHECK(h.table.find("G7GG00") == nullptr);
        h.paint_full();
        CHECK(h.row_visible("G7GG11"));
        CHECK(h.summary() == "7-12/12");
        CHECK(h.scrollbar_painted());
    }

    SUBCASE("selection on the top visible row, record above evicted") {
        Harness h;
        for (unsigned i = 0; i < 12; i++) {
            char call[13];
            std::snprintf(call, sizeof(call), "K9KK%02u", i);
            h.report(call, "JN58", i + 1, 800 + i * BR_STEP);
        }
        /* Move to index 7 (offset 3), then back to index 3: viewport offset
         * stays 3, selection on the TOP visible row. The first press
         * selects record 0. */
        for (unsigned i = 0; i < 8; i++)
            h.list.on_encoder(1);
        for (unsigned i = 0; i < 4; i++)
            h.list.on_encoder(-1);
        h.paint_full();
        CHECK(h.summary() == "3-8/12");
        REQUIRE(h.row_visible("K9KK05"));

        /* K9KK00 is the LRU and sits ABOVE the viewport: evicting it moves
         * the selection to index 4; the offset must follow so the selected
         * row does not jump out of the window. */
        h.report("L0LL00", "JN58", 200, 800 + 12 * BR_STEP);
        CHECK(h.table.find("K9KK00") == nullptr);
        h.paint_full();
        CHECK(h.row_visible("K9KK05"));
        h.list.format_summary(summary);
        CHECK(std::string(summary) == "3-8/12");
        CHECK(h.scrollbar_painted());
    }
}

/* Review P1: the LAST line used to be placed at app-local y304, i.e. physical
 * y320 under SystemView's 16-px status strip, so it was clipped. These checks
 * assert the REAL 240x304 application viewport geometry and that every child
 * (header, list, summary, LAST) fits and renders inside a COMPLETE 240x320
 * system frame, not just inside the list widget. */
TEST_CASE("station list view: full system frame geometry and LAST visibility") {
    const ui::Rect app_viewport{0, 0, APP_W, APP_H};
    const ui::Rect header{0, wspr_layout::header_y, APP_W,
                          wspr_layout::header_height};
    const ui::Rect list{0, APP_LIST_Y, APP_W, APP_LIST_H};
    const ui::Rect last{0, APP_LAST_Y, APP_W, wspr_layout::last_height};

    SUBCASE("every child rect is inside the 240x304 application viewport") {
        CHECK(APP_H == 304);
        CHECK(rect_inside(app_viewport, APP_W, APP_H));
        /* Upper control bands: radio row, settings, buttons, STATUS line. */
        CHECK(rect_inside({0, wspr_layout::radio_y, APP_W,
                           wspr_layout::radio_height},
                          APP_W, APP_H));
        CHECK(rect_inside({0, wspr_layout::settings_y, APP_W,
                           wspr_layout::settings_height},
                          APP_W, APP_H));
        CHECK(rect_inside({0, wspr_layout::buttons_y, APP_W,
                           wspr_layout::buttons_height},
                          APP_W, APP_H));
        CHECK(rect_inside({0, wspr_layout::status_y, APP_W,
                           wspr_layout::status_height},
                          APP_W, APP_H));
        /* Candidate panel line in the band directly above LAST. */
        const ui::Rect cand{0, APP_CAND_Y, APP_W, wspr_layout::cand_height};
        CHECK(rect_inside(cand, APP_W, APP_H));
        CHECK(cand.top() == last.top() - wspr_layout::cand_height);
        CHECK(rect_inside(header, APP_W, APP_H));
        CHECK(rect_inside(list, APP_W, APP_H));
        CHECK(rect_inside(last, APP_W, APP_H));
        /* The old placement (local y304, height 16) is NOT contained. */
        CHECK_FALSE(rect_inside({0, 304, APP_W, 16}, APP_W, APP_H));
        /* LAST is the very last line: it ends exactly at the viewport
         * bottom (user feedback: it used to sit 16 px too high). */
        CHECK(last.bottom() == APP_H);
        CHECK(APP_TOP + last.bottom() == 320);
    }

    SUBCASE("list, candidate panel and LAST do not overlap each other") {
        const ui::Rect cand{0, APP_CAND_Y, APP_W, wspr_layout::cand_height};
        CHECK_FALSE(rects_overlap(cand, list));
        CHECK_FALSE(rects_overlap(cand, last));
        CHECK(cand.bottom() == last.top());
        CHECK_FALSE(rects_overlap(list, last));
        CHECK(list.bottom() <= cand.top());
        /* The list holds 5 rows again (v1.4 form restored): the candidate
         * panel took the freed summary band, scrolling/scrollbar unchanged. */
        CHECK(wspr_layout::list_rows(wspr_layout::app_viewport_height) == 5);
        CHECK(APP_LIST_H == 5 * StationListView::row_height);
    }

    SUBCASE("LAST text is drawn and visible inside the full system frame") {
        FullFrameHarness h;
        for (unsigned i = 0; i < 12; i++) {
            char call[13];
            std::snprintf(call, sizeof(call), "M5MM%02u", i);
            h.report(call, "JO59", i + 1, 500 + i * BR_STEP);
        }
        h.render();

        /* Complete frame, not a 240x304 app pasted at origin 0. */
        CHECK(h.fb.width == APP_W);
        CHECK(h.fb.height == 320);

        /* System strip is painted at y0..16; the app starts at y16. */
        CHECK(h.fb.at(0, 0) == ui::Color::darker_grey().v);
        CHECK(h.pixels_of(0, APP_TOP, ui::Color::darker_grey()) > 0);
        CHECK(h.fb.at(0, APP_TOP) == ui::Color::black().v);

        /* LAST was drawn at its ACTUAL physical origin (16 + 288 = 304),
         * the bottom line of the screen, and its glyph pixels are on screen. */
        const auto* draw = h.find_text(APP_TOP + APP_LAST_Y, LAST_TEXT);
        REQUIRE(draw != nullptr);
        CHECK(draw->p.y() == 304);
        CHECK(draw->p.y() + 16 <= 320);
        const int last_pixels =
            h.pixels_of(APP_TOP + APP_LAST_Y, APP_TOP + APP_LAST_Y + 16,
                        ui::Color::dark_grey());
        CHECK(last_pixels > 30);
        /* Nothing of LAST leaks into the system strip or past the screen. */
        CHECK(h.pixels_of(0, APP_TOP, ui::Color::dark_grey()) == 0);

        /* The candidate panel line is drawn at its real physical origin
         * (16 + 272 = 288), directly above LAST, never in the strip. */
        const auto* cand = h.find_text(APP_TOP + APP_CAND_Y, CAND_TEXT);
        REQUIRE(cand != nullptr);
        CHECK(cand->p.y() == 288);
        CHECK(cand->p.y() + 16 <= APP_TOP + APP_H);
        const int cand_pixels =
            h.pixels_of(APP_TOP + APP_CAND_Y, APP_TOP + APP_CAND_Y + 16,
                        ui::Color::dark_yellow());
        CHECK(cand_pixels > 30);
        CHECK(h.pixels_of(0, APP_TOP, ui::Color::dark_yellow()) == 0);

        /* The last visible list row (index 4 of 5) is fully inside the app
         * viewport too: physical y 16+88+4*32 = 232, two text lines. */
        const int row4_y = APP_TOP + APP_LIST_Y + 4 * StationListView::row_height;
        bool row4_visible = false;
        for (const auto& s : h.fb.strings)
            if (s.p.y() == row4_y && s.text.find("M5MM04") != std::string::npos)
                row4_visible = true;
        CHECK(row4_visible);
        CHECK(row4_y + 16 <= APP_TOP + APP_H);

        /* The header row carries the column labels AND the visible-range
         * summary (v1.4 form): the label literal at x0 and the summary
         * widget at x header_width, both on physical y88. */
        char hdr[24];
        StationListView::format_header_line(hdr);
        REQUIRE(h.find_text(APP_TOP + wspr_layout::header_y, hdr) != nullptr);
        REQUIRE(h.find_text(APP_TOP + wspr_layout::header_y, "1-5/12") != nullptr);
        CHECK(APP_TOP + wspr_layout::header_y == 88);

        /* Sensitivity: the OLD placement (app-local 304 = physical 320)
         * paints ZERO pixels when the app is at its real origin. A
         * widget-only test at origin 0 would have missed this. */
        ui::HostFramebuffer off;
        off.reset(APP_W, wspr_layout::physical_height);
        ui::Painter p;
        p.target(&off);
        p.set_origin({0, APP_TOP});
        p.draw_string({0, 304},
                      {ui::font::fixed_8x16, ui::Color::black(),
                       ui::Color::dark_grey()},
                      LAST_TEXT);
        CHECK(p.origin().y() == 16);
        int off_pixels = 0;
        for (int y = 0; y < off.height; y++)
            for (int x = 0; x < off.width; x++)
                if (off.at(x, y) != ui::Color::black().v)
                    off_pixels++;
        CHECK(off_pixels == 0);  // clipped: the old bug
    }
}
