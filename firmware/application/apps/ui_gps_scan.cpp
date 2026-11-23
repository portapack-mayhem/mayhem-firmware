/*
 * Copyright (C) 2026 zxkmm
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

#include "ui_gps_scan.hpp"
#include "baseband_api.hpp"
#include "portapack.hpp"
#include "portapack_persistent_memory.hpp"
#include "string_format.hpp"

#include <cstdio>
#include <cstring>

using namespace portapack;
using namespace gps_scan;

namespace {

static constexpr uint8_t kDoppKhzChoices[4] = {5, 10, 20, 35};
static constexpr const char* kDoppLabels[4] = {"+-5k", "+-10k", "+-20k", "+-35k"};
static constexpr const char* kDayNames[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

// Integer-only dBFS * 10 approximation for rms_x10 (full scale = 1280 => 0.0 dBFS).
int16_t rms_x10_to_dbfs_x10(uint16_t rms_x10) {
    if (rms_x10 <= 1) return -620;
    uint32_t v = rms_x10;
    int32_t msb = 0;
    while ((v >> (msb + 1)) != 0) msb++;
    const int32_t frac = (msb >= 4)
                             ? static_cast<int32_t>((v >> (msb - 4)) & 0x0FU)
                             : static_cast<int32_t>((v << (4 - msb)) & 0x0FU);
    const int32_t log2_diff_x16 = (msb * 16 + frac) - 165;
    return static_cast<int16_t>((log2_diff_x16 * 3853) / 1024);
}

const char* integ_label(Integration integ) {
    switch (integ) {
        case Integration::Ms1x1:
            return "1msx1";
        case Integration::Ms1x4:
            return "1msx4";
        case Integration::Ms2x4:
            return "2msx4";
        case Integration::Ms4x4:
            return "4msx4";
        default:
            return "2msx4";
    }
}

const char* status_tag(uint8_t st) {
    switch (static_cast<DetStatus>(st)) {
        case DetStatus::Lock:
            return "LOCK";
        case DetStatus::Weak:
            return "WEAK";
        default:
            return "NOIS";
    }
}

const char* track_state_tag(uint8_t st) {
    switch (static_cast<TrackState>(st)) {
        case TrackState::Reacq:
            return "ACQ ";
        case TrackState::PullIn:
            return "FLL ";
        case TrackState::WidePLL:
            return "WPLL";
        case TrackState::Locked:
            return "LOCK";
        case TrackState::Lost:
            return "LOST";
        default:
            return "IDLE";
    }
}

const char* nav_state_short(uint8_t st) {
    switch (static_cast<TrackState>(st)) {
        case TrackState::Reacq:
            return "ACQ";
        case TrackState::PullIn:
            return "FLL";
        case TrackState::WidePLL:
            return "WPL";
        case TrackState::Locked:
            return "LCK";
        case TrackState::Lost:
            return "LST";
        default:
            return "---";
    }
}

}  // namespace

namespace ui {

void GPSScanPanel::draw_cached(
    Painter& painter,
    Coord x,
    Coord y,
    size_t width_chars,
    const Style* style,
    const char* text,
    size_t slot) {
    if (slot >= kCacheSlots || !style) return;
    if (width_chars > 30) width_chars = 30;

    char padded[32];
    size_t len = std::strlen(text);
    if (len > width_chars) len = width_chars;
    std::memcpy(padded, text, len);
    while (len < width_chars) padded[len++] = ' ';
    padded[len] = '\0';

    auto& c = line_cache_[slot];
    if (c.style == style && std::strcmp(c.text, padded) == 0) {
        return;  // Unchanged: skip LCD write completely!
    }
    c.style = style;
    std::memcpy(c.text, padded, len + 1);
    painter.draw_string({x, y}, *style, padded);
}

void GPSScanPanel::paint(Painter& painter) {
    const auto r = screen_rect();

    // Full clear ONLY on view switch, first paint, or placeholder <-> live transition
    if (model_.view_mode != prev_view_mode_ ||
        (model_.view_mode == 1 && model_.has_track != prev_has_track_) ||
        (model_.view_mode == 2 && model_.has_nav != prev_has_nav_)) {
        invalidate_full();
        prev_view_mode_ = model_.view_mode;
        prev_has_track_ = model_.has_track;
        prev_has_nav_ = model_.has_nav;
    }

    if (needs_clear_) {
        painter.fill_rectangle_unrolled8(r, Theme::getInstance()->bg_darkest->background);
        needs_clear_ = false;
    }

    if (model_.view_mode == 1) {
        paint_tracking(painter, r);
    } else if (model_.view_mode == 2) {
        paint_fix(painter, r);
    } else if (model_.view_mode == 3) {
        paint_diagnostics(painter, r);
    } else {
        paint_list_and_plot(painter, r);
    }
}

void GPSScanPanel::paint_list_and_plot(Painter& painter, const Rect& r) {
    char buf[34];

    // Column header (slot 0)
    draw_cached(painter, r.left(), r.top(), 30, Theme::getInstance()->fg_cyan, "PRN  C/N0  DoppHz   Chip  Rat", 0);

    // Top 7 PRN rows (slots 1..7)
    static constexpr size_t kVisibleRows = 7;
    for (size_t i = 0; i < kVisibleRows; i++) {
        const Coord y = r.top() + 16 + static_cast<Coord>(i * 16);
        if (i >= model_.result_count) {
            const char* empty_msg = (i == 0 && model_.result_count == 0) ? "Press Scan, Track, or Test.." : "";
            draw_cached(painter, r.left(), y, 30, Theme::getInstance()->fg_medium, empty_msg, 1 + i);
            continue;
        }

        const uint8_t prn = model_.sorted_prns[i];
        const auto& res = model_.results[prn - 1];
        const bool is_sel = (prn == model_.selected_prn) || (model_.selected_prn == 0 && i == 0);

        const uint32_t chip_x10 = (static_cast<uint32_t>(res.code_phase) * 10230U + 1024U) >> 11;
        const int16_t cn0 = res.cn0_dbhz_x10;
        const int16_t cn0_abs = (cn0 < 0) ? -cn0 : cn0;

        std::snprintf(
            buf, sizeof(buf),
            "%cG%02u %s%2d.%1d %+5dHz %4lu.%1lu %2u.%1u",
            is_sel ? '>' : ' ',
            static_cast<unsigned>(res.prn),
            (cn0 < 0) ? "-" : "",
            static_cast<int>(cn0_abs / 10),
            static_cast<int>(cn0_abs % 10),
            static_cast<int>(res.doppler_hz),
            static_cast<unsigned long>(chip_x10 / 10),
            static_cast<unsigned long>(chip_x10 % 10),
            static_cast<unsigned>(res.ratio_x100 / 100),
            static_cast<unsigned>((res.ratio_x100 % 100) / 10));

        const Style* st = Theme::getInstance()->fg_medium;
        if (res.status == static_cast<uint8_t>(DetStatus::Lock)) {
            st = Theme::getInstance()->fg_green;
        } else if (res.status == static_cast<uint8_t>(DetStatus::Weak)) {
            st = Theme::getInstance()->fg_yellow;
        } else if (is_sel) {
            st = Theme::getInstance()->fg_light;
        }
        draw_cached(painter, r.left(), y, 30, st, buf, 1 + i);
    }

    // Correlation profile caption (slot 8)
    const Coord cap_y = r.top() + 130;
    if (model_.has_profile) {
        const auto& pr = model_.profile.result;
        std::snprintf(buf, sizeof(buf), "G%02u %s Z=%u.%u R=%u.%02u %+d",
                      static_cast<unsigned>(pr.prn),
                      status_tag(pr.status),
                      static_cast<unsigned>(pr.z_score_x10 / 10),
                      static_cast<unsigned>(pr.z_score_x10 % 10),
                      static_cast<unsigned>(pr.ratio_x100 / 100),
                      static_cast<unsigned>(pr.ratio_x100 % 100),
                      static_cast<int>(pr.doppler_hz));
        const Style* cap_st = Theme::getInstance()->fg_cyan;
        if (pr.status == static_cast<uint8_t>(DetStatus::Lock))
            cap_st = Theme::getInstance()->fg_green;
        else if (pr.status == static_cast<uint8_t>(DetStatus::Weak))
            cap_st = Theme::getInstance()->fg_yellow;
        draw_cached(painter, r.left(), cap_y, 30, cap_st, buf, 8);
    } else {
        draw_cached(painter, r.left(), cap_y, 30, Theme::getInstance()->fg_cyan, "Corr Peak (+-16 chips): --", 8);
    }

    // Correlation Spike Plot box (y = r.top() + 146 .. r.bottom())
    const Rect plot_box{r.left(), r.top() + 146, r.width(), static_cast<Dim>(r.height() - 146)};
    const Color bg = Theme::getInstance()->bg_darkest->background;

    if (!plot_chrome_drawn_) {
        painter.draw_rectangle(plot_box, Theme::getInstance()->fg_dark->foreground);
        plot_chrome_drawn_ = true;
    }

    if (!model_.has_profile) {
        draw_cached(painter, plot_box.left() + 24, plot_box.top() + 22, 24, Theme::getInstance()->fg_medium, "No correlation data yet", 9);
        return;
    }

    const Coord plot_x = plot_box.left() + 24;
    const Coord plot_y = plot_box.top() + 4;
    const Dim plot_h = plot_box.height() - 14;
    const Dim bar_w = 3;  // 64 * 3 = 192 px
    const Coord center_x = plot_x + 32 * bar_w + 1;

    Color bar_color = Theme::getInstance()->fg_medium->foreground;
    if (model_.profile.result.status == static_cast<uint8_t>(DetStatus::Lock)) {
        bar_color = Theme::getInstance()->fg_green->foreground;
    } else if (model_.profile.result.status == static_cast<uint8_t>(DetStatus::Weak)) {
        bar_color = Theme::getInstance()->fg_yellow->foreground;
    }

    // Per-column overwrite (top bg + bottom bar) -> zero full-box flicker!
    for (size_t i = 0; i < PROFILE_BINS; i++) {
        const uint8_t v = model_.profile.bins[i];
        Dim h = static_cast<Dim>((static_cast<uint32_t>(v) * plot_h) / 255U);
        if (h == 0 && v > 0) h = 1;
        if (h > plot_h) h = plot_h;
        const Coord bx = plot_x + static_cast<Coord>(i * bar_w);
        const Dim top_h = static_cast<Dim>(plot_h - h);
        if (top_h > 0) {
            painter.fill_rectangle({bx, plot_y, bar_w, top_h}, bg);
        }
        if (h > 0) {
            const Color c = (i == PROFILE_BINS / 2) ? Color::white() : bar_color;
            painter.fill_rectangle({bx, static_cast<Coord>(plot_y + top_h), bar_w - 1, h}, c);
            painter.fill_rectangle({static_cast<Coord>(bx + bar_w - 1), static_cast<Coord>(plot_y + top_h), 1, h}, bg);
        }
    }

    // Clear left/right margin strips before drawing 2nd-peak reference ticks
    painter.fill_rectangle({plot_box.left() + 4, plot_y, 16, plot_h}, bg);
    painter.fill_rectangle({plot_x + static_cast<Coord>(PROFILE_BINS * bar_w) + 4, plot_y, 16, plot_h}, bg);

    const Dim mean_h = static_cast<Dim>((static_cast<uint32_t>(model_.profile.mean_level) * plot_h) / 255U);
    const Coord mean_y = plot_y + static_cast<Coord>(plot_h - mean_h);
    painter.fill_rectangle({plot_x, mean_y, static_cast<Dim>(PROFILE_BINS * bar_w), 1}, Theme::getInstance()->fg_cyan->foreground);

    const Dim sec_h = static_cast<Dim>((static_cast<uint32_t>(model_.profile.second_level) * plot_h) / 255U);
    const Coord sec_y = plot_y + static_cast<Coord>(plot_h - sec_h);
    painter.fill_rectangle({plot_box.left() + 4, sec_y, 16, 1}, Theme::getInstance()->fg_yellow->foreground);
    painter.fill_rectangle({plot_x + static_cast<Coord>(PROFILE_BINS * bar_w) + 4, sec_y, 16, 1}, Theme::getInstance()->fg_yellow->foreground);

    const Style& small_style = *Theme::getInstance()->bg_darkest_small;
    painter.draw_string({plot_x, plot_box.bottom() - 9}, small_style, "-16c");
    painter.draw_string({center_x - 5, plot_box.bottom() - 9}, small_style, "0c");
    painter.draw_string({plot_x + static_cast<Coord>(PROFILE_BINS * bar_w) - 20, plot_box.bottom() - 9}, small_style, "+16c");
}

void GPSScanPanel::paint_tracking(Painter& painter, const Rect& r) {
    char buf[34];

    if (!model_.has_track) {
        draw_cached(painter, r.left(), r.top() + 16, 30, Theme::getInstance()->fg_cyan, "Stage 2: 1-Ch GPS Tracker", 0);
        draw_cached(painter, r.left(), r.top() + 40, 30, Theme::getInstance()->fg_light, "1. Scan to find a satellite", 1);
        draw_cached(painter, r.left(), r.top() + 56, 30, Theme::getInstance()->fg_light, "2. Select PRN (< > or P01..32)", 2);
        draw_cached(painter, r.left(), r.top() + 72, 30, Theme::getInstance()->fg_light, "3. Press Track (or Test)", 3);
        return;
    }

    const auto& tr = model_.track;
    const Color bg = Theme::getInstance()->bg_darkest->background;
    const Color grid = Theme::getInstance()->fg_dark->foreground;

    // Row 0 (slot 0): PRN, Lock State, C/N0, Duration, DMA Drops
    const int16_t cn0 = tr.cn0_dbhz_x10;
    const int16_t cn0_abs = (cn0 < 0) ? -cn0 : cn0;
    std::snprintf(buf, sizeof(buf), "G%02u [%s] %s%2d.%1ddB %3us D:%u%s",
                  static_cast<unsigned>(tr.prn),
                  track_state_tag(tr.state),
                  cn0 < 0 ? "-" : "",
                  static_cast<int>(cn0_abs / 10), static_cast<int>(cn0_abs % 10),
                  static_cast<unsigned>(tr.track_sec),
                  static_cast<unsigned>(tr.dma_drops),
                  tr.is_selftest ? "S" : "");
    const Style* st0 = Theme::getInstance()->fg_yellow;
    if (tr.state == static_cast<uint8_t>(TrackState::Locked))
        st0 = Theme::getInstance()->fg_green;
    else if (tr.state == static_cast<uint8_t>(TrackState::Lost))
        st0 = Theme::getInstance()->fg_red;
    draw_cached(painter, r.left(), r.top(), 30, st0, buf, 0);

    // Row 1 (slot 1): Fine Doppler (0.1 Hz), PLI, Bit Sync
    const int32_t d_abs = (tr.doppler_hz_x10 < 0) ? -tr.doppler_hz_x10 : tr.doppler_hz_x10;
    const int16_t pli_abs = (tr.pli_x100 < 0) ? -tr.pli_x100 : tr.pli_x100;
    std::snprintf(buf, sizeof(buf), "D:%c%ld.%1ldHz PLI:%c%d.%02d Bit:%c",
                  tr.doppler_hz_x10 < 0 ? '-' : '+',
                  static_cast<long>(d_abs / 10), static_cast<long>(d_abs % 10),
                  tr.pli_x100 < 0 ? '-' : '+',
                  static_cast<int>(pli_abs / 100), static_cast<int>(pli_abs % 100),
                  tr.bit_synced ? 'Y' : '-');
    draw_cached(painter, r.left(), r.top() + 16, 30, Theme::getInstance()->fg_light, buf, 1);

    // Row 2 (slot 2): Code phase & Early / Prompt / Late correlator magnitudes
    std::snprintf(buf, sizeof(buf), "C:%4lu.%1lu E:%u P:%u L:%u",
                  static_cast<unsigned long>(tr.code_chip_x100 / 100),
                  static_cast<unsigned long>((tr.code_chip_x100 % 100) / 10),
                  static_cast<unsigned>(tr.early_mag),
                  static_cast<unsigned>(tr.prompt_mag),
                  static_cast<unsigned>(tr.late_mag));
    draw_cached(painter, r.left(), r.top() + 32, 30, Theme::getInstance()->fg_cyan, buf, 2);

    // Middle Left: 96x96 Prompt I/Q BPSK Constellation Plot (incremental erase-old / draw-new!)
    const Rect iq_box{r.left() + 2, r.top() + 50, 96, 96};
    const Coord cx = iq_box.left() + 48;
    const Coord cy = iq_box.top() + 48;

    if (!iq_chrome_drawn_) {
        painter.draw_rectangle(iq_box, grid);
        painter.fill_rectangle({iq_box.left() + 2, cy, 92, 1}, grid);
        painter.fill_rectangle({cx, iq_box.top() + 2, 1, 92}, grid);
        const Style& small_st = *Theme::getInstance()->bg_darkest_small;
        painter.draw_string({iq_box.right() - 8, cy - 9}, small_st, "I");
        painter.draw_string({cx + 3, iq_box.top() + 2}, small_st, "Q");
        iq_chrome_drawn_ = true;
    }

    // Compute new 2x2 dot positions (clamped inside radius 39 so border & I/Q labels stay untouched)
    Point cur_iq[TRACK_IQ_HISTORY];
    for (size_t k = 0; k < TRACK_IQ_HISTORY; k++) {
        Coord px = cx + static_cast<Coord>((static_cast<int32_t>(tr.iq_hist[k].i) * 39) / 127);
        Coord py = cy - static_cast<Coord>((static_cast<int32_t>(tr.iq_hist[k].q) * 39) / 127);
        if (px < iq_box.left() + 4) px = iq_box.left() + 4;
        if (px > iq_box.right() - 6) px = iq_box.right() - 6;
        if (py < iq_box.top() + 10) py = iq_box.top() + 10;
        if (py > iq_box.bottom() - 6) py = iq_box.bottom() - 6;
        cur_iq[k] = {px, py};
    }

    // 1. Erase ONLY old 2x2 dots not covered by a new dot, restoring crosshair pixels underneath
    for (size_t i = 0; i < prev_iq_count_; i++) {
        const Point old_pt = prev_iq_[i];
        bool covered = false;
        for (size_t j = 0; j < TRACK_IQ_HISTORY; j++) {
            if (cur_iq[j].x() == old_pt.x() && cur_iq[j].y() == old_pt.y()) {
                covered = true;
                break;
            }
        }
        if (!covered) {
            for (Coord dy = 0; dy < 2; dy++) {
                const Coord py = old_pt.y() + dy;
                for (Coord dx = 0; dx < 2; dx++) {
                    const Coord px = old_pt.x() + dx;
                    const Color restore_c = (px == cx || py == cy) ? grid : bg;
                    display.draw_pixel({px, py}, restore_c);
                }
            }
        }
    }

    // 2. Draw new 2x2 dots (no prior blanking -> zero flicker!)
    for (size_t k = 0; k < TRACK_IQ_HISTORY; k++) {
        Color dot_c = grid;
        if (k >= TRACK_IQ_HISTORY - 8) {
            dot_c = (tr.state == static_cast<uint8_t>(TrackState::Locked))
                        ? Theme::getInstance()->fg_green->foreground
                        : Theme::getInstance()->fg_yellow->foreground;
        } else if (k >= TRACK_IQ_HISTORY - 18) {
            dot_c = Theme::getInstance()->fg_cyan->foreground;
        }
        painter.fill_rectangle({cur_iq[k].x(), cur_iq[k].y(), 2, 2}, dot_c);
        prev_iq_[k] = cur_iq[k];
    }
    prev_iq_count_ = TRACK_IQ_HISTORY;

    // Middle Right: LNAV Subframe Telemetry (x = 104..240, 17 chars wide, slots 3..7)
    const Coord rx = r.left() + 104;
    Coord ry = r.top() + 50;

    // SF 1 2 3 4 5 indicator (only redraw when sf_mask actually changes!)
    if (tr.sf_mask != prev_sf_mask_) {
        prev_sf_mask_ = tr.sf_mask;
        painter.draw_string({rx, ry}, *Theme::getInstance()->fg_light, "SF:");
        for (uint8_t s = 1; s <= 5; s++) {
            char d[2] = {static_cast<char>('0' + s), '\0'};
            const bool got = (tr.sf_mask & (1U << (s - 1))) != 0;
            const Style* sst = got ? Theme::getInstance()->fg_green : Theme::getInstance()->fg_dark;
            painter.draw_string({static_cast<Coord>(rx + 28 + (s - 1) * 16), ry}, *sst, d);
        }
    }
    ry += 16;

    // TOW seconds (slot 3)
    if (tr.frame_synced) {
        std::snprintf(buf, sizeof(buf), "TOW:%6lus", static_cast<unsigned long>(tr.tow_sec));
        draw_cached(painter, rx, ry, 17, Theme::getInstance()->fg_green, buf, 3);
    } else {
        draw_cached(painter, rx, ry, 17, Theme::getInstance()->fg_medium, "TOW:  syncing", 3);
    }
    ry += 16;

    // GPS Day-of-Week + HH:MM:SS (slot 4)
    if (tr.frame_synced) {
        const uint32_t day = (tr.tow_sec / 86400U) % 7U;
        const uint32_t rem = tr.tow_sec % 86400U;
        const uint32_t hh = rem / 3600U;
        const uint32_t mm = (rem % 3600U) / 60U;
        const uint32_t ss = rem % 60U;
        std::snprintf(buf, sizeof(buf), "%s %02lu:%02lu:%02lu",
                      kDayNames[day],
                      static_cast<unsigned long>(hh),
                      static_cast<unsigned long>(mm),
                      static_cast<unsigned long>(ss));
        draw_cached(painter, rx, ry, 17, Theme::getInstance()->fg_light, buf, 4);
    } else {
        draw_cached(painter, rx, ry, 17, Theme::getInstance()->fg_medium, "--- --:--:--", 4);
    }
    ry += 16;

    // GPS Week & SV Health (slot 5)
    if (tr.gps_week > 0) {
        std::snprintf(buf, sizeof(buf), "WK:%4u H:%s",
                      static_cast<unsigned>(tr.gps_week),
                      tr.sv_health == 0 ? "OK" : "BD");
        draw_cached(painter, rx, ry, 17, tr.sv_health == 0 ? Theme::getInstance()->fg_green : Theme::getInstance()->fg_red, buf, 5);
    } else {
        draw_cached(painter, rx, ry, 17, Theme::getInstance()->fg_medium, "WK:---- H:--", 5);
    }
    ry += 16;

    // Subframes OK / Err (slot 6)
    std::snprintf(buf, sizeof(buf), "Frm:%uok %uer",
                  static_cast<unsigned>(tr.frames_ok),
                  static_cast<unsigned>(tr.frames_err));
    draw_cached(painter, rx, ry, 17, tr.frames_ok > 0 ? Theme::getInstance()->fg_green : Theme::getInstance()->fg_light, buf, 6);
    ry += 16;

    // Last Subframe Parity & IODE (slot 7)
    std::snprintf(buf, sizeof(buf), "Par:%2u/10 I:%u",
                  static_cast<unsigned>(tr.last_parity_ok),
                  static_cast<unsigned>(tr.last_iode));
    draw_cached(painter, rx, ry, 17, Theme::getInstance()->fg_cyan, buf, 7);

    // Bottom: 50 bps Live NAV Bit Stream (slots 8, 9, 10)
    const Coord by = r.top() + 152;
    const char* tlm_str = (tr.frame_synced == 1) ? "TLM:8B+" : ((tr.frame_synced == 2) ? "TLM:74-" : "TLM:---");
    std::snprintf(buf, sizeof(buf), "NAV 50bps (%lub) %s",
                  static_cast<unsigned long>(tr.total_bits), tlm_str);
    draw_cached(painter, r.left(), by, 30, tr.frame_synced ? Theme::getInstance()->fg_green : Theme::getInstance()->fg_cyan, buf, 8);

    if (tr.total_bits > 0) {
        char line1[30];
        char line2[30];
        for (int b = 0; b < 28; b++) {
            line1[b] = ((tr.recent_bits >> (55 - b)) & 1ULL) ? '1' : '0';
            line2[b] = ((tr.recent_bits >> (27 - b)) & 1ULL) ? '1' : '0';
        }
        line1[28] = '\0';
        line2[28] = '\0';
        draw_cached(painter, r.left() + 4, by + 18, 28, Theme::getInstance()->fg_medium, line1, 9);
        draw_cached(painter, r.left() + 4, by + 34, 28, Theme::getInstance()->fg_light, line2, 10);
    } else {
        draw_cached(painter, r.left() + 4, by + 18, 28, Theme::getInstance()->fg_medium, "Waiting for 20ms bit sync...", 9);
        draw_cached(painter, r.left() + 4, by + 34, 28, Theme::getInstance()->fg_medium, "", 10);
    }
}

void GPSScanPanel::paint_fix(Painter& painter, const Rect& r) {
    char buf[34];

    if (!model_.has_nav) {
        draw_cached(painter, r.left(), r.top() + 8, 30, Theme::getInstance()->fg_cyan, "Stage 3: 5-Ch GPS 3D Fix", 0);
        draw_cached(painter, r.left(), r.top() + 32, 30, Theme::getInstance()->fg_light, "1. Scan to find >=4 SVs", 1);
        draw_cached(painter, r.left(), r.top() + 48, 30, Theme::getInstance()->fg_light, "2. Press Fix to track & solve", 2);
        draw_cached(painter, r.left(), r.top() + 64, 30, Theme::getInstance()->fg_light, "   (needs SF1-3 ~18-36s)", 3);
        draw_cached(painter, r.left(), r.top() + 88, 30, Theme::getInstance()->fg_yellow, "Indoors? Press Test for 5-SV", 4);
        draw_cached(painter, r.left(), r.top() + 104, 30, Theme::getInstance()->fg_yellow, "real-time 3D fix self-test!", 5);
        return;
    }

    const auto& nav = model_.nav_obs;
    const auto& fx = model_.fix;

    // Count ephemerides ready among currently tracked PRNs
    uint8_t eph_ready = 0;
    for (uint8_t i = 0; i < nav.num_ch && i < MAX_NAV_CH; i++) {
        for (const auto& ep : model_.ephem_cache) {
            if (ep.prn == nav.ch[i].prn && ep.is_complete()) {
                eph_ready++;
                break;
            }
        }
    }

    // Row 0 (slot 0): Fix status banner + exact M4 CPU microseconds (pack+track us / 1000us)
    const Style* st0 = Theme::getInstance()->fg_yellow;
    if (nav.phase == 1) {
        std::snprintf(buf, sizeof(buf), "NAV Re-Acq %u SVs...", static_cast<unsigned>(nav.num_ch));
    } else if (fx.valid) {
        std::snprintf(buf, sizeof(buf), "FIX3D %u/%u %us D:%u %u+%uu",
                      static_cast<unsigned>(fx.num_used),
                      static_cast<unsigned>(nav.num_ch),
                      static_cast<unsigned>(nav.track_sec),
                      static_cast<unsigned>(nav.dma_drops),
                      static_cast<unsigned>(nav.cpu_us_pack),
                      static_cast<unsigned>(nav.cpu_us_nav));
        st0 = Theme::getInstance()->fg_green;
    } else {
        std::snprintf(buf, sizeof(buf), "NAV %uc E:%u/4 D:%u %u+%uu",
                      static_cast<unsigned>(nav.num_ch),
                      static_cast<unsigned>(eph_ready),
                      static_cast<unsigned>(nav.dma_drops),
                      static_cast<unsigned>(nav.cpu_us_pack),
                      static_cast<unsigned>(nav.cpu_us_nav));
    }
    draw_cached(painter, r.left(), r.top(), 30, st0, buf, 0);

    // Row 1..4 (slots 1..4): LAT, LON, ALT+Res, HDOP+PDOP+WK
    if (fx.valid) {
        const int32_t lat_e5 = static_cast<int32_t>(fx.lat_deg * 100000.0 + (fx.lat_deg >= 0.0 ? 0.5 : -0.5));
        const int32_t lat_a = (lat_e5 < 0) ? -lat_e5 : lat_e5;
        std::snprintf(buf, sizeof(buf), "LAT: %c%2ld.%05ld deg",
                      lat_e5 < 0 ? '-' : '+',
                      static_cast<long>(lat_a / 100000), static_cast<long>(lat_a % 100000));
        draw_cached(painter, r.left(), r.top() + 16, 30, Theme::getInstance()->fg_green, buf, 1);

        const int32_t lon_e5 = static_cast<int32_t>(fx.lon_deg * 100000.0 + (fx.lon_deg >= 0.0 ? 0.5 : -0.5));
        const int32_t lon_a = (lon_e5 < 0) ? -lon_e5 : lon_e5;
        std::snprintf(buf, sizeof(buf), "LON:%c%3ld.%05ld deg",
                      lon_e5 < 0 ? '-' : '+',
                      static_cast<long>(lon_a / 100000), static_cast<long>(lon_a % 100000));
        draw_cached(painter, r.left(), r.top() + 32, 30, Theme::getInstance()->fg_green, buf, 2);

        const int32_t alt_x10 = static_cast<int32_t>(fx.alt_m * 10.0 + (fx.alt_m >= 0.0 ? 0.5 : -0.5));
        const int32_t alt_a = (alt_x10 < 0) ? -alt_x10 : alt_x10;
        const uint32_t res_x10 = static_cast<uint32_t>(fx.rms_res_m * 10.0 + 0.5);
        std::snprintf(buf, sizeof(buf), "ALT:%c%4ld.%1ldm Res:%lu.%1lum",
                      alt_x10 < 0 ? '-' : '+',
                      static_cast<long>(alt_a / 10), static_cast<long>(alt_a % 10),
                      static_cast<unsigned long>(res_x10 / 10), static_cast<unsigned long>(res_x10 % 10));
        draw_cached(painter, r.left(), r.top() + 48, 30, Theme::getInstance()->fg_light, buf, 3);

        const uint32_t hd_x10 = static_cast<uint32_t>(fx.hdop * 10.0 + 0.5);
        const uint32_t pd_x10 = static_cast<uint32_t>(fx.pdop * 10.0 + 0.5);
        std::snprintf(buf, sizeof(buf), "HDOP:%lu.%1lu PDOP:%lu.%1lu WK:%u",
                      static_cast<unsigned long>(hd_x10 / 10), static_cast<unsigned long>(hd_x10 % 10),
                      static_cast<unsigned long>(pd_x10 / 10), static_cast<unsigned long>(pd_x10 % 10),
                      static_cast<unsigned>(fx.gps_week));
        draw_cached(painter, r.left(), r.top() + 64, 30, Theme::getInstance()->fg_cyan, buf, 4);
    } else {
        static constexpr const char* kRejNames[6] = {"waiting", "<4 sync", "dt rng", "matrix", "no conv", "bad san"};
        const char* rstr = (fx.reject_reason < 6) ? kRejNames[fx.reject_reason] : "waiting";
        std::snprintf(buf, sizeof(buf), "LAT:  --.----- (%s)", rstr);
        draw_cached(painter, r.left(), r.top() + 16, 30, Theme::getInstance()->fg_medium, buf, 1);
        draw_cached(painter, r.left(), r.top() + 32, 30, Theme::getInstance()->fg_medium, "LON: ---.----- deg", 2);
        draw_cached(painter, r.left(), r.top() + 48, 30, Theme::getInstance()->fg_medium, "ALT:  ----.-m  Res: ---", 3);
        draw_cached(painter, r.left(), r.top() + 64, 30, Theme::getInstance()->fg_medium, "HDOP:--- PDOP:--- WK:----", 4);
    }

    // Row 5 (slot 5): GPS Time (from fix if valid, else from any frame-synced channel)
    uint32_t tow = fx.valid ? fx.gps_tow_sec : 0;
    bool has_tow = fx.valid;
    if (!has_tow) {
        for (uint8_t i = 0; i < nav.num_ch && i < MAX_NAV_CH; i++) {
            if (nav.ch[i].frame_synced) {
                tow = (nav.ch[i].sf_start_tow + (nav.ch[i].bit_in_sf * 20U) / 1000U) % 604800U;
                has_tow = true;
                break;
            }
        }
    }
    if (has_tow) {
        const uint32_t day = (tow / 86400U) % 7U;
        const uint32_t rem = tow % 86400U;
        std::snprintf(buf, sizeof(buf), "GPS:%s %02lu:%02lu:%02lu (%lus)",
                      kDayNames[day],
                      static_cast<unsigned long>(rem / 3600U),
                      static_cast<unsigned long>((rem % 3600U) / 60U),
                      static_cast<unsigned long>(rem % 60U),
                      static_cast<unsigned long>(tow));
        draw_cached(painter, r.left(), r.top() + 80, 30, Theme::getInstance()->fg_light, buf, 5);
    } else {
        draw_cached(painter, r.left(), r.top() + 80, 30, Theme::getInstance()->fg_medium, "GPS: syncing TLM/HOW...", 5);
    }

    // Row 6 (slot 6): Channel table header
    draw_cached(painter, r.left(), r.top() + 98, 30, Theme::getInstance()->fg_cyan, "CH PRN ST  C/N0  Dopp S EPH", 6);

    // Rows 7..11 (slots 7..11): 5 live tracking channels
    for (uint8_t i = 0; i < MAX_NAV_CH; i++) {
        const Coord y = r.top() + 114 + static_cast<Coord>(i * 16);
        if (i >= nav.num_ch) {
            draw_cached(painter, r.left(), y, 30, Theme::getInstance()->fg_medium, "", 7 + i);
            continue;
        }
        const auto& c = nav.ch[i];

        // Look up cached SF1-3 mask for this PRN
        uint8_t eph_mask = c.sf_mask & 0x07U;
        bool eph_ok = false;
        for (const auto& ep : model_.ephem_cache) {
            if (ep.prn == c.prn) {
                eph_mask |= (ep.sf_valid_mask & 0x07U);
                eph_ok = ep.is_complete();
                break;
            }
        }

        const int16_t cn0 = c.cn0_dbhz_x10;
        const int16_t cn0_a = (cn0 < 0) ? -cn0 : cn0;
        const int32_t d_hz = c.doppler_hz_x10 / 10;
        const char sync_ch = c.frame_synced ? 'F' : (c.bit_synced ? 'b' : '.');
        std::snprintf(buf, sizeof(buf), "%u G%02u %s %2d.%1d %+5ld %c[%c%c%c]",
                      static_cast<unsigned>(i),
                      static_cast<unsigned>(c.prn),
                      nav_state_short(c.state),
                      static_cast<int>(cn0_a / 10), static_cast<int>(cn0_a % 10),
                      static_cast<long>(d_hz),
                      sync_ch,
                      (eph_mask & 1U) ? '1' : '-',
                      (eph_mask & 2U) ? '2' : '-',
                      (eph_mask & 4U) ? '3' : '-');

        const Style* cst = Theme::getInstance()->fg_yellow;
        if (c.state == static_cast<uint8_t>(TrackState::Lost)) {
            cst = Theme::getInstance()->fg_red;
        } else if (c.state == static_cast<uint8_t>(TrackState::Locked)) {
            cst = eph_ok ? Theme::getInstance()->fg_green : Theme::getInstance()->fg_cyan;
        }
        draw_cached(painter, r.left(), y, 30, cst, buf, 7 + i);
    }
}

void GPSScanPanel::paint_diagnostics(Painter& painter, const Rect& r) {
    char buf[34];
    Coord y = r.top();
    size_t slot = 0;
    auto draw = [&](const Style* st, const char* s) {
        draw_cached(painter, r.left(), y, 30, st, s, slot++);
        y += 16;
    };

    const auto& st = model_.stats;

    draw(Theme::getInstance()->fg_cyan, "-- RF & HARDWARE --");

    const uint64_t f_hz = receiver_model.target_frequency();
    const uint32_t f_mhz = static_cast<uint32_t>(f_hz / 1'000'000ULL);
    const uint32_t f_rem = static_cast<uint32_t>((f_hz % 1'000'000ULL) / 100U);
    std::snprintf(buf, sizeof(buf), "F:%lu.%04luM Fs:2.048M",
                  static_cast<unsigned long>(f_mhz), static_cast<unsigned long>(f_rem));
    draw(Theme::getInstance()->fg_light, buf);

    std::snprintf(buf, sizeof(buf), "BW:2.5M ANT:%s AMP:%u L%u V%u",
                  portapack::get_antenna_bias() ? "ON " : "OFF",
                  receiver_model.rf_amp() ? 1U : 0U,
                  static_cast<unsigned>(receiver_model.lna()),
                  static_cast<unsigned>(receiver_model.vga()));
    draw(Theme::getInstance()->fg_light, buf);

    draw(Theme::getInstance()->fg_cyan, "-- ADC & SNAPSHOT --");

    const int16_t mi = st.mean_i_x10;
    const int16_t mq = st.mean_q_x10;
    std::snprintf(buf, sizeof(buf), "DC:%c%d.%d/%c%d.%d Pk:%u/%u",
                  mi < 0 ? '-' : '+', static_cast<int>((mi < 0 ? -mi : mi) / 10), static_cast<int>((mi < 0 ? -mi : mi) % 10),
                  mq < 0 ? '-' : '+', static_cast<int>((mq < 0 ? -mq : mq) / 10), static_cast<int>((mq < 0 ? -mq : mq) % 10),
                  static_cast<unsigned>(st.peak_abs_i), static_cast<unsigned>(st.peak_abs_q));
    draw(Theme::getInstance()->fg_light, buf);

    const uint16_t rms_avg = static_cast<uint16_t>((st.rms_i_x10 + st.rms_q_x10) / 2);
    const int16_t dbfs_x10 = rms_x10_to_dbfs_x10(rms_avg);
    const int16_t dbfs_abs = (dbfs_x10 < 0) ? -dbfs_x10 : dbfs_x10;
    std::snprintf(buf, sizeof(buf), "RMS:%u.%u/%u.%u (-%d.%ddBFS)",
                  static_cast<unsigned>(st.rms_i_x10 / 10), static_cast<unsigned>(st.rms_i_x10 % 10),
                  static_cast<unsigned>(st.rms_q_x10 / 10), static_cast<unsigned>(st.rms_q_x10 % 10),
                  static_cast<int>(dbfs_abs / 10), static_cast<int>(dbfs_abs % 10));
    draw(Theme::getInstance()->fg_light, buf);

    std::snprintf(buf, sizeof(buf), "Q2:%02u/%02u/%02u/%02u%% (16/34/34/16)",
                  static_cast<unsigned>(st.q2_pct[0]), static_cast<unsigned>(st.q2_pct[1]),
                  static_cast<unsigned>(st.q2_pct[2]), static_cast<unsigned>(st.q2_pct[3]));
    draw(Theme::getInstance()->fg_light, buf);

    std::snprintf(buf, sizeof(buf), "Clp:%u.%u%% Drp:%u Snap:%ums",
                  static_cast<unsigned>(st.clip_pm / 10), static_cast<unsigned>(st.clip_pm % 10),
                  static_cast<unsigned>(st.dma_drops), static_cast<unsigned>(st.snapshot_ms));
    draw(st.dma_drops > 0 ? Theme::getInstance()->fg_red : Theme::getInstance()->fg_green, buf);

    draw(Theme::getInstance()->fg_cyan, "-- SCAN & SELECTED PRN --");

    std::snprintf(buf, sizeof(buf), "#%lu %u.%us %lu IFFT Buf:%u",
                  static_cast<unsigned long>(model_.scan_id),
                  static_cast<unsigned>(model_.scan_ms / 1000),
                  static_cast<unsigned>((model_.scan_ms % 1000) / 100),
                  static_cast<unsigned long>(model_.ifft_count),
                  static_cast<unsigned>(st.dma_buffers));
    draw(Theme::getInstance()->fg_light, buf);

    if (model_.result_count > 0 && model_.selected_rank < model_.result_count) {
        const auto& res = model_.results[model_.sorted_prns[model_.selected_rank] - 1];
        const uint32_t chip_x10 = (static_cast<uint32_t>(res.code_phase) * 10230U + 1024U) >> 11;
        const int16_t cn0 = res.cn0_dbhz_x10;
        const int16_t cn0_abs = (cn0 < 0) ? -cn0 : cn0;

        std::snprintf(buf, sizeof(buf), "G%02u %s %s%d.%ddB %+dHz %lu.%u",
                      static_cast<unsigned>(res.prn),
                      status_tag(res.status),
                      cn0 < 0 ? "-" : "",
                      static_cast<int>(cn0_abs / 10), static_cast<int>(cn0_abs % 10),
                      static_cast<int>(res.doppler_hz),
                      static_cast<unsigned long>(chip_x10 / 10),
                      static_cast<unsigned>(chip_x10 % 10));
        const Style* rst = Theme::getInstance()->fg_light;
        if (res.status == static_cast<uint8_t>(DetStatus::Lock))
            rst = Theme::getInstance()->fg_green;
        else if (res.status == static_cast<uint8_t>(DetStatus::Weak))
            rst = Theme::getInstance()->fg_yellow;
        draw(rst, buf);

        std::snprintf(buf, sizeof(buf), "Pk:%lu 2nd:%lu R=%u.%02u Z=%u.%u",
                      static_cast<unsigned long>(res.peak_power),
                      static_cast<unsigned long>(res.second_peak),
                      static_cast<unsigned>(res.ratio_x100 / 100),
                      static_cast<unsigned>(res.ratio_x100 % 100),
                      static_cast<unsigned>(res.z_score_x10 / 10),
                      static_cast<unsigned>(res.z_score_x10 % 10));
        draw(Theme::getInstance()->fg_light, buf);
    } else {
        draw(Theme::getInstance()->fg_medium, "No PRN scanned yet");
        draw(Theme::getInstance()->fg_medium, "");
    }
}

GPSScanView::GPSScanView(NavigationView& nav)
    : nav_{nav} {
    baseband::run_image(portapack::spi_flash::image_tag_gps_scan);

    add_children({
        &field_frequency,
        &labels_rf,
        &field_rf_amp,
        &field_lna,
        &field_vga,
        &rssi,
        &button_bias,
        &button_integ,
        &button_dopp,
        &field_prn,
        &check_auto,
        &button_scan,
        &button_track,
        &button_fix,
        &button_test,
        &button_view,
        &button_prev,
        &button_next,
        &text_status,
        &progress_bar,
        &text_adc_strip,
        &panel,
    });

    // GPS L1 C/A defaults: 1575.42 MHz, 1 kHz step, LNA 40, VGA 32, AMP 0
    receiver_model.set_lna(40);
    receiver_model.set_vga(32);
    receiver_model.set_rf_amp(false);
    field_frequency.set_step(1000);
    field_prn.set_value(0);
    progress_bar.set_max(100);
    progress_bar.set_value(0);

    update_bias_button();
    update_view_button();
    update_action_buttons();

    button_bias.on_select = [this](Button&) {
        toggle_bias_tee();
    };

    button_integ.on_select = [this](Button&) {
        cycle_integ();
    };

    button_dopp.on_select = [this](Button&) {
        cycle_dopp();
    };

    check_auto.on_select = [this](Checkbox&, bool v) {
        auto_rescan_ = v;
    };

    button_scan.on_select = [this](Button&) {
        if (scanning_ || tracking_ || nav_fixing_) {
            scanning_ = false;
            tracking_ = false;
            nav_fixing_ = false;
            selftest_active_ = false;
            update_action_buttons();
            send_config(Mode::Idle);
        } else {
            scanning_ = true;
            tracking_ = false;
            nav_fixing_ = false;
            selftest_active_ = false;
            model_.result_count = 0;
            model_.selected_rank = 0;
            model_.results.fill({});
            model_.view_mode = 0;
            update_view_button();
            update_action_buttons();
            panel.set_dirty();
            send_config(auto_rescan_ ? Mode::ScanAuto : Mode::ScanSingle);
        }
    };

    button_track.on_select = [this](Button&) {
        if (tracking_) {
            tracking_ = false;
            selftest_active_ = false;
            update_action_buttons();
            send_config(Mode::Idle);
        } else {
            start_tracking_selected(false);
        }
    };

    button_fix.on_select = [this](Button&) {
        if (nav_fixing_) {
            nav_fixing_ = false;
            selftest_active_ = false;
            update_action_buttons();
            send_config(Mode::Idle);
        } else {
            start_nav_fix(false);
        }
    };

    button_test.on_select = [this](Button&) {
        if (model_.view_mode == 1) {
            start_tracking_selected(true);
        } else if (model_.view_mode == 2) {
            start_nav_fix(true);
        } else {
            scanning_ = true;
            tracking_ = false;
            nav_fixing_ = false;
            selftest_active_ = true;
            model_.result_count = 0;
            model_.selected_rank = 0;
            model_.results.fill({});
            update_action_buttons();
            panel.set_dirty();
            send_config(Mode::SelfTest);
        }
    };

    button_view.on_select = [this](Button&) {
        model_.view_mode = (model_.view_mode + 1U) % 4U;
        update_view_button();
        panel.set_dirty();
    };

    button_prev.on_select = [this](Button&) {
        select_rank(-1);
    };

    button_next.on_select = [this](Button&) {
        select_rank(+1);
    };

    receiver_model.enable();
    send_config(Mode::Idle);
}

GPSScanView::~GPSScanView() {
    send_config(Mode::Idle);
    receiver_model.disable();
    baseband::shutdown();
}

void GPSScanView::focus() {
    button_scan.focus();
}

void GPSScanView::toggle_bias_tee() {
    const bool next = !portapack::get_antenna_bias();
    portapack::set_antenna_bias(next);
    receiver_model.set_antenna_bias();
    update_bias_button();
    StatusRefreshMessage message{};
    EventDispatcher::send_message(message);
    if (model_.view_mode == 3) panel.set_dirty();
}

void GPSScanView::update_bias_button() {
    const bool on = portapack::get_antenna_bias();
    button_bias.set_text(on ? "ANT:ON" : "ANT:OFF");
    button_bias.set_style(on ? Theme::getInstance()->fg_green : Theme::getInstance()->fg_light);
}

void GPSScanView::update_view_button() {
    // Button label shows the NEXT view it will switch to
    static constexpr const char* kNextView[4] = {"Trk", "Fix", "Diag", "List"};
    button_view.set_text(kNextView[model_.view_mode % 4U]);
}

void GPSScanView::update_action_buttons() {
    button_scan.set_text(scanning_ ? "Stop" : "Scan");
    button_track.set_text(tracking_ ? "Stop" : "Trk");
    button_track.set_style(tracking_ ? Theme::getInstance()->fg_green : Theme::getInstance()->fg_light);
    button_fix.set_text(nav_fixing_ ? "Stop" : "Fix");
    button_fix.set_style(nav_fixing_ ? Theme::getInstance()->fg_green : Theme::getInstance()->fg_light);
}

void GPSScanView::cycle_integ() {
    const uint8_t next = (static_cast<uint8_t>(integ_) + 1U) & 3U;
    integ_ = static_cast<Integration>(next);
    button_integ.set_text(integ_label(integ_));
}

void GPSScanView::cycle_dopp() {
    dopp_idx_ = (dopp_idx_ + 1U) & 3U;
    button_dopp.set_text(kDoppLabels[dopp_idx_]);
}

void GPSScanView::send_config(Mode mode, uint8_t prn_override, int16_t dopp_hint) {
    Config cfg{};
    cfg.mode = mode;
    cfg.integration = integ_;
    cfg.doppler_khz = kDoppKhzChoices[dopp_idx_];
    cfg.prn_filter = (prn_override != 0) ? prn_override : static_cast<uint8_t>(field_prn.value());
    cfg.profile_prn = model_.selected_prn;
    cfg.track_dopp_hint_hz = dopp_hint;
    baseband::set_gps_scan_config(cfg);
}

void GPSScanView::start_tracking_selected(bool selftest) {
    scanning_ = false;
    tracking_ = true;
    nav_fixing_ = false;
    selftest_active_ = selftest;
    model_.view_mode = 1;  // Switch directly to Tracking view
    model_.has_track = false;
    update_view_button();
    update_action_buttons();

    uint8_t target_prn = static_cast<uint8_t>(field_prn.value());
    int16_t dopp_hint = 0;

    if (selftest) {
        target_prn = 1;
        dopp_hint = 1250;
    } else {
        if (target_prn == 0) {
            if (model_.selected_prn >= 1 && model_.selected_prn <= NUM_PRNS) {
                target_prn = model_.selected_prn;
            } else if (model_.result_count > 0) {
                target_prn = model_.sorted_prns[0];
            } else {
                target_prn = 1;
            }
        }
        if (target_prn >= 1 && target_prn <= NUM_PRNS && model_.results[target_prn - 1].prn == target_prn) {
            dopp_hint = model_.results[target_prn - 1].doppler_hz;
        }
    }

    char buf[34];
    std::snprintf(buf, sizeof(buf), "TRK G%02u %s...",
                  static_cast<unsigned>(target_prn),
                  selftest ? "SelfTest" : "Re-Acq");
    text_status.set_style(Theme::getInstance()->fg_yellow);
    text_status.set(buf);
    progress_bar.set_value(0);
    panel.set_dirty();

    send_config(selftest ? Mode::TrackSelfTest : Mode::Track, target_prn, dopp_hint);
}

void GPSScanView::start_nav_fix(bool selftest) {
    if (!selftest && model_.result_count < 4) {
        model_.view_mode = 2;
        update_view_button();
        text_status.set_style(Theme::getInstance()->fg_yellow);
        text_status.set("FIX: Scan >=4 SVs or tap Test");
        panel.set_dirty();
        return;
    }

    scanning_ = false;
    tracking_ = false;
    nav_fixing_ = true;
    selftest_active_ = selftest;
    nav_obs_div_ = 0;

    // Never mix synthetic self-test ephemerides with live sky ephemerides!
    if (model_.ephem_is_selftest != selftest) {
        model_.ephem_cache.fill({});
        model_.ephem_is_selftest = selftest;
    }

    model_.fix = {};
    model_.has_nav = true;
    model_.nav_obs = {};
    model_.nav_obs.is_selftest = selftest ? 1 : 0;
    model_.view_mode = 2;
    update_view_button();
    update_action_buttons();

    Config cfg{};
    cfg.mode = selftest ? Mode::NavFixSelfTest : Mode::NavFix;
    cfg.integration = integ_;
    cfg.doppler_khz = kDoppKhzChoices[dopp_idx_];

    if (!selftest) {
        uint8_t cnt = 0;
        for (size_t i = 0; i < model_.result_count && cnt < MAX_NAV_CH; i++) {
            const uint8_t prn = model_.sorted_prns[i];
            if (prn >= 1 && prn <= NUM_PRNS) {
                cfg.nav_targets[cnt].prn = prn;
                cfg.nav_targets[cnt].dopp_hint_hz = model_.results[prn - 1].doppler_hz;
                cnt++;
            }
        }
        cfg.nav_count = cnt;
        model_.nav_obs.num_ch = cnt;
        model_.nav_obs.phase = 1;
    } else {
        cfg.nav_count = MAX_NAV_CH;
        model_.nav_obs.num_ch = MAX_NAV_CH;
        model_.nav_obs.phase = 2;
    }

    text_status.set_style(Theme::getInstance()->fg_yellow);
    text_status.set(selftest ? "FIX SelfTest 5-SV (3D sim)..." : "FIX Re-Acq 5 SVs...");
    progress_bar.set_value(0);
    panel.set_dirty();

    baseband::set_gps_scan_config(cfg);
}

uint8_t GPSScanView::count_ready_ephemerides() const {
    uint8_t ready = 0;
    for (uint8_t i = 0; i < model_.nav_obs.num_ch && i < MAX_NAV_CH; i++) {
        const uint8_t prn = model_.nav_obs.ch[i].prn;
        for (const auto& ep : model_.ephem_cache) {
            if (ep.prn == prn && ep.is_complete()) {
                ready++;
                break;
            }
        }
    }
    return ready;
}

void GPSScanView::on_stats(const GPSScanStatsMessage& msg) {
    model_.stats = msg.stats;
    if (!tracking_ && !nav_fixing_) {
        model_.scan_ms = msg.scan_ms;
        model_.ifft_count = msg.ifft_count;
        model_.scan_id = msg.scan_id;

        char buf[34];
        switch (msg.scan_state) {
            case ScanState::Idle:
                if (scanning_) {
                    scanning_ = false;
                    update_action_buttons();
                }
                std::snprintf(buf, sizeof(buf), "IDLE  DMA:%u  Ready",
                              static_cast<unsigned>(msg.stats.dma_buffers));
                text_status.set_style(Theme::getInstance()->fg_light);
                break;

            case ScanState::Capturing:
                std::snprintf(buf, sizeof(buf), "CAP #%lu (%s)...",
                              static_cast<unsigned long>(msg.scan_id),
                              selftest_active_ ? "TEST" : integ_label(integ_));
                text_status.set_style(Theme::getInstance()->fg_yellow);
                progress_bar.set_value(0);
                break;

            case ScanState::Scanning:
                std::snprintf(buf, sizeof(buf), "%s G%02u %3u%% %u.%us Lk:%u",
                              selftest_active_ ? "TST" : "SCN",
                              static_cast<unsigned>(msg.current_prn),
                              static_cast<unsigned>(msg.progress_pct),
                              static_cast<unsigned>(msg.scan_ms / 1000),
                              static_cast<unsigned>((msg.scan_ms % 1000) / 100),
                              static_cast<unsigned>(msg.detected_count));
                text_status.set_style(Theme::getInstance()->fg_yellow);
                progress_bar.set_value(msg.progress_pct);
                break;

            case ScanState::Done:
                if (!auto_rescan_ || selftest_active_) {
                    scanning_ = false;
                    update_action_buttons();
                }
                std::snprintf(buf, sizeof(buf), "%s#%lu %u.%us Lk:%u %s",
                              selftest_active_ ? "TST" : "OK ",
                              static_cast<unsigned long>(msg.scan_id),
                              static_cast<unsigned>(msg.scan_ms / 1000),
                              static_cast<unsigned>((msg.scan_ms % 1000) / 100),
                              static_cast<unsigned>(msg.detected_count),
                              msg.stats.is_selftest ? "[SIM]" : "");
                text_status.set_style(msg.detected_count > 0 ? Theme::getInstance()->fg_green : Theme::getInstance()->fg_cyan);
                progress_bar.set_value(100);
                break;
        }
        text_status.set(buf);
    }

    refresh_adc_strip();
    if (model_.view_mode == 3) {
        panel.set_dirty();
    }
}

void GPSScanView::on_result(const GPSScanResultMessage& msg) {
    const auto& r = msg.result;
    if (r.prn < 1 || r.prn > NUM_PRNS) return;

    model_.scan_id = msg.scan_id;
    model_.results[r.prn - 1] = r;
    sort_results();
    if (model_.view_mode == 0 || model_.view_mode == 3) {
        panel.set_dirty();
    }
}

void GPSScanView::on_profile(const GPSScanProfileMessage& msg) {
    model_.profile = msg.profile;
    model_.has_profile = true;
    model_.selected_prn = msg.profile.result.prn;

    for (size_t i = 0; i < model_.result_count; i++) {
        if (model_.sorted_prns[i] == model_.selected_prn) {
            model_.selected_rank = i;
            break;
        }
    }
    if (model_.view_mode == 0 || model_.view_mode == 3) {
        panel.set_dirty();
    }
}

void GPSScanView::on_track(const GPSScanTrackMessage& msg) {
    model_.track = msg.status;
    model_.has_track = true;

    const auto& tr = msg.status;
    char buf[34];
    const int32_t d_abs = (tr.doppler_hz_x10 < 0) ? -tr.doppler_hz_x10 : tr.doppler_hz_x10;
    std::snprintf(buf, sizeof(buf), "TRK G%02u %s %c%ld.%1ldHz SF:%u",
                  static_cast<unsigned>(tr.prn),
                  track_state_tag(tr.state),
                  tr.doppler_hz_x10 < 0 ? '-' : '+',
                  static_cast<long>(d_abs / 10), static_cast<long>(d_abs % 10),
                  static_cast<unsigned>(tr.frames_ok));

    const Style* st = Theme::getInstance()->fg_yellow;
    if (tr.state == static_cast<uint8_t>(TrackState::Locked))
        st = Theme::getInstance()->fg_green;
    else if (tr.state == static_cast<uint8_t>(TrackState::Lost))
        st = Theme::getInstance()->fg_red;
    text_status.set_style(st);
    text_status.set(buf);

    // Progress bar doubles as PLI lock quality bar (0..100%) during tracking!
    int32_t pli_pct = tr.pli_x100;
    if (pli_pct < 0) pli_pct = 0;
    if (pli_pct > 100) pli_pct = 100;
    progress_bar.set_value(static_cast<uint32_t>(pli_pct));

    if (model_.view_mode == 1) {
        panel.set_dirty();
    }
}

void GPSScanView::on_subframe(const GPSScanSubframeMessage& msg) {
    const auto& sf = msg.subframe;
    if (sf.prn < 1 || sf.prn > NUM_PRNS || sf.sf_id < 1 || sf.sf_id > 3) return;

    // Find existing slot for PRN or allocate an empty / oldest slot
    size_t slot = GPSScanModel::kEphemCacheSize;
    for (size_t i = 0; i < GPSScanModel::kEphemCacheSize; i++) {
        if (model_.ephem_cache[i].prn == sf.prn) {
            slot = i;
            break;
        }
        if (slot == GPSScanModel::kEphemCacheSize && model_.ephem_cache[i].prn == 0) {
            slot = i;
        }
    }
    if (slot >= GPSScanModel::kEphemCacheSize) {
        slot = sf.prn % GPSScanModel::kEphemCacheSize;
    }

    model_.ephem_cache[slot].ingest_subframe(sf.prn, sf.sf_id, sf.words);
    if (model_.view_mode == 2) {
        panel.set_dirty();
    }
}

void GPSScanView::on_nav_obs(const GPSScanNavObsMessage& msg) {
    model_.nav_obs = msg.obs;
    model_.has_nav = true;

    const auto& nav = msg.obs;
    uint8_t locked_cnt = 0;
    for (uint8_t i = 0; i < nav.num_ch && i < MAX_NAV_CH; i++) {
        if (nav.ch[i].state == static_cast<uint8_t>(TrackState::Locked) ||
            nav.ch[i].state == static_cast<uint8_t>(TrackState::WidePLL)) {
            locked_cnt++;
        }
    }

    const uint8_t eph_ready = count_ready_ephemerides();

    // Solve 3D WGS-84 PVT at 1 Hz (every 2nd 2 Hz observation packet) once >=4 ephemerides are complete
    nav_obs_div_++;
    if (nav.phase == 2 && eph_ready >= 4 && (nav_obs_div_ & 1U) == 0U) {
        const PvtFix cand = solve_pvt(nav, model_.ephem_cache.data(), GPSScanModel::kEphemCacheSize);
        if (cand.valid) {
            model_.fix = cand;
        } else if (!model_.fix.valid) {
            model_.fix.reject_reason = cand.reject_reason;
        }
    }

    char buf[34];
    if (nav.phase == 1) {
        std::snprintf(buf, sizeof(buf), "FIX Re-Acq %u SVs...", static_cast<unsigned>(nav.num_ch));
        text_status.set_style(Theme::getInstance()->fg_yellow);
        progress_bar.set_value(10);
    } else if (model_.fix.valid) {
        const uint32_t hd_x10 = static_cast<uint32_t>(model_.fix.hdop * 10.0 + 0.5);
        std::snprintf(buf, sizeof(buf), "3D FIX %uSV HDOP:%lu.%1lu D:%u",
                      static_cast<unsigned>(model_.fix.num_used),
                      static_cast<unsigned long>(hd_x10 / 10),
                      static_cast<unsigned long>(hd_x10 % 10),
                      static_cast<unsigned>(nav.dma_drops));
        text_status.set_style(Theme::getInstance()->fg_green);
        progress_bar.set_value(100);
    } else {
        std::snprintf(buf, sizeof(buf), "FIX Lck:%u/%u Eph:%u/4 %us",
                      static_cast<unsigned>(locked_cnt),
                      static_cast<unsigned>(nav.num_ch),
                      static_cast<unsigned>(eph_ready),
                      static_cast<unsigned>(nav.track_sec));
        text_status.set_style(Theme::getInstance()->fg_yellow);
        // Progress bar shows ephemeris readiness (0..100%)
        uint32_t sf_total = 0;
        for (uint8_t i = 0; i < nav.num_ch && i < MAX_NAV_CH; i++) {
            uint8_t m = nav.ch[i].sf_mask & 0x07U;
            for (const auto& ep : model_.ephem_cache) {
                if (ep.prn == nav.ch[i].prn) {
                    m |= (ep.sf_valid_mask & 0x07U);
                    break;
                }
            }
            sf_total += ((m & 1U) ? 1U : 0U) + ((m & 2U) ? 1U : 0U) + ((m & 4U) ? 1U : 0U);
        }
        uint32_t pct = (sf_total * 100U) / 12U;
        if (pct > 99U) pct = 99U;
        progress_bar.set_value(pct);
    }
    text_status.set(buf);

    if (model_.view_mode == 2) {
        panel.set_dirty();
    }
}

void GPSScanView::sort_results() {
    model_.result_count = 0;
    for (uint8_t p = 1; p <= NUM_PRNS; p++) {
        if (model_.results[p - 1].prn == p) {
            model_.sorted_prns[model_.result_count++] = p;
        }
    }

    for (size_t i = 1; i < model_.result_count; i++) {
        const uint8_t key = model_.sorted_prns[i];
        const auto& rk = model_.results[key - 1];
        size_t j = i;
        while (j > 0) {
            const auto& rprev = model_.results[model_.sorted_prns[j - 1] - 1];
            const bool better =
                (rk.status > rprev.status) ||
                (rk.status == rprev.status && rk.z_score_x10 > rprev.z_score_x10) ||
                (rk.status == rprev.status && rk.z_score_x10 == rprev.z_score_x10 && rk.ratio_x100 > rprev.ratio_x100);
            if (!better) break;
            model_.sorted_prns[j] = model_.sorted_prns[j - 1];
            j--;
        }
        model_.sorted_prns[j] = key;
    }

    if (model_.selected_rank >= model_.result_count) {
        model_.selected_rank = 0;
    }
}

void GPSScanView::refresh_adc_strip() {
    const auto& st = model_.stats;
    const uint16_t rms_avg = static_cast<uint16_t>((st.rms_i_x10 + st.rms_q_x10) / 2);
    const int16_t dbfs_x10 = rms_x10_to_dbfs_x10(rms_avg);
    const int16_t dbfs_abs = (dbfs_x10 < 0) ? -dbfs_x10 : dbfs_x10;

    char buf[34];
    std::snprintf(
        buf, sizeof(buf),
        "RMS:%2u/%2u -%2d.%1ddB C:%u.%u D:%u",
        static_cast<unsigned>(st.rms_i_x10 / 10),
        static_cast<unsigned>(st.rms_q_x10 / 10),
        static_cast<int>(dbfs_abs / 10),
        static_cast<int>(dbfs_abs % 10),
        static_cast<unsigned>(st.clip_pm / 10),
        static_cast<unsigned>(st.clip_pm % 10),
        static_cast<unsigned>(st.dma_drops));

    if (st.dma_drops > 0 || rms_avg < 20 || st.clip_pm >= 100) {
        text_adc_strip.set_style(Theme::getInstance()->fg_red);
    } else if (rms_avg < 80 || rms_avg > 750 || st.clip_pm >= 30) {
        text_adc_strip.set_style(Theme::getInstance()->fg_yellow);
    } else {
        text_adc_strip.set_style(Theme::getInstance()->fg_green);
    }
    text_adc_strip.set(buf);
}

void GPSScanView::select_rank(int delta) {
    if (model_.result_count == 0) return;
    int next = static_cast<int>(model_.selected_rank) + delta;
    if (next < 0) next = static_cast<int>(model_.result_count) - 1;
    if (next >= static_cast<int>(model_.result_count)) next = 0;
    model_.selected_rank = static_cast<size_t>(next);
    model_.selected_prn = model_.sorted_prns[model_.selected_rank];
    panel.set_dirty();
    request_selected_profile();
}

void GPSScanView::request_selected_profile() {
    if (!scanning_ && !tracking_ && !nav_fixing_ && model_.selected_prn >= 1 && model_.selected_prn <= NUM_PRNS) {
        send_config(Mode::Idle);
    }
}

} /* namespace ui */
