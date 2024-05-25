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

#include "ui_rssi.hpp"

#include "baseband_api.hpp"
#include "utility.hpp"

#include <algorithm>

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))
#define abs(x) ((x) > 0 ? (x) : -(x))

// flag value for graph min
// If it does not change, then all graph min values are is zero
#define GRAPH_MIN_ALL_ZERO_FLAG 666

namespace ui {

RSSI::RSSI(
    Rect parent_rect,
    bool instant_exec)
    : Widget{parent_rect},
      instant_exec_{instant_exec} {
}

void RSSI::paint(Painter& painter) {
    const auto r = screen_rect();

    constexpr int rssi_sample_range = 256;
    // constexpr float rssi_voltage_min = 0.4;
    constexpr float rssi_voltage_max = 2.2;
    constexpr float adc_voltage_max = 3.3;
    // constexpr int raw_min = rssi_sample_range * rssi_voltage_min / adc_voltage_max;
    constexpr int raw_min = 0;
    constexpr int raw_max = rssi_sample_range * rssi_voltage_max / adc_voltage_max;
    constexpr int raw_delta = raw_max - raw_min;

    if (!vertical_rssi_enabled) {
        // horizontal left to right level meters
        const range_t<int> x_avg_range{0, r.width() - 1};
        const int16_t x_avg = x_avg_range.clip((avg_ - raw_min) * r.width() / raw_delta);
        const range_t<int> x_min_range{0, x_avg};
        const int16_t x_min = x_min_range.clip((min_ - raw_min) * r.width() / raw_delta);
        const range_t<int> x_max_range{x_avg + 1, r.width() - 1};
        const int16_t x_max = x_max_range.clip((max_ - raw_min) * r.width() / raw_delta);
        const int16_t peak = x_max_range.clip((peak_ - raw_min) * r.width() / raw_delta);

        // x_min
        const Rect r0{r.left(), r.top(), x_min, r.height()};
        painter.fill_rectangle(
            r0,
            Theme::getInstance()->fg_blue->foreground);

        // x_avg
        const Rect r1{r.left() + x_min, r.top(), x_avg - x_min, r.height()};
        painter.fill_rectangle(
            r1,
            Theme::getInstance()->fg_red->foreground);

        // x_avg middle marker
        const Rect r2{r.left() + x_avg, r.top(), 1, r.height()};
        painter.fill_rectangle(
            r2,
            Color::white());

        // x_max
        const Rect r3{r.left() + x_avg + 1, r.top(), x_max - (x_avg + 1), r.height()};
        painter.fill_rectangle(
            r3,
            Theme::getInstance()->fg_red->foreground);

        // filling last part in black
        const Rect r4{r.left() + x_max, r.top(), r.width() - x_max, r.height()};
        painter.fill_rectangle(
            r4,
            Theme::getInstance()->bg_darkest->background);

        // show green peak value
        if (peak_enabled) {
            const Rect r5{r.left() + peak - 3, r.top(), 3, r.height()};
            painter.fill_rectangle(
                r5,
                Theme::getInstance()->fg_green->foreground);
        }
    } else {
        // vertical bottom to top level meters
        const range_t<int> y_avg_range{0, r.height() - 1};
        const int16_t y_avg = y_avg_range.clip((avg_ - raw_min) * r.height() / raw_delta);
        const range_t<int> y_min_range{0, y_avg};
        const int16_t y_min = y_min_range.clip((min_ - raw_min) * r.height() / raw_delta);
        const range_t<int> y_max_range{y_avg + 1, r.height() - 1};
        const int16_t y_max = y_max_range.clip((max_ - raw_min) * r.height() / raw_delta);
        const range_t<int> peak_range{0, r.height() - 1};
        const int16_t peak = peak_range.clip((peak_ - raw_min) * r.height() / raw_delta);

        // y_min
        const Rect r0{r.left(), r.bottom() - y_min, r.width(), y_min};
        painter.fill_rectangle(
            r0,
            Color::blue());

        // y_avg
        const Rect r1{r.left(), r.bottom() - y_avg, r.width(), y_avg - y_min};
        painter.fill_rectangle(
            r1,
            Color::red());

        // y_avg middle marker
        const Rect r2{r.left(), r.bottom() - y_avg, r.width(), 1};
        painter.fill_rectangle(
            r2,
            Color::white());

        // y_max
        const Rect r3{r.left(), r.bottom() - y_max, r.width(), y_max - y_avg};
        // const Rect r3 {  r.left(), r.bottom() - y_max , r.width() , y_max - y_avg - 1 };
        painter.fill_rectangle(
            r3,
            Color::red());

        // fill last part of level in black
        const Rect r4{r.left(), r.top(), r.width(), r.height() - y_max};
        painter.fill_rectangle(
            r4,
            Color::black());

        // show green peak value if enabled
        if (peak_enabled) {
            const Rect r5{r.left(), r.bottom() - peak - 3, r.width(), 3};
            painter.fill_rectangle(
                r5,
                Color::green());
        }
    }
    if (pitch_rssi_enabled) {
        baseband::set_pitch_rssi((avg_ - raw_min) * 2000 / raw_delta, true);
    }
    if (has_focus() || highlighted()) {
        const Rect r6{r.left(), r.top(), r.width(), r.height()};
        painter.draw_rectangle(
            r6,
            Color::white());
    }
}

uint8_t RSSI::get_min() {
    return min_;
}

uint8_t RSSI::get_avg() {
    return avg_;
}

uint8_t RSSI::get_max() {
    return max_;
}

uint8_t RSSI::get_delta() {
    return max_ - min_;
}

void RSSI::set_pitch_rssi(bool enabled) {
    pitch_rssi_enabled = enabled;
    if (!enabled) baseband::set_pitch_rssi(0, false);
}

void RSSI::set_vertical_rssi(bool enabled) {
    if (enabled)
        vertical_rssi_enabled = true;
    else
        vertical_rssi_enabled = false;
}

void RSSI::set_peak(bool enabled, size_t duration) {
    peak_enabled = enabled;
    peak_duration = duration;
}

void RSSI::on_statistics_update(const RSSIStatistics& statistics) {
    min_ = statistics.min;
    avg_ = statistics.accumulator / statistics.count;
    max_ = statistics.max;
    if (peak_enabled) {
        peak_duration_ = peak_duration_ + 100;
        if (max_ > peak_) {
            peak_ = max_;
        }
        if (peak_duration_ > peak_duration) {
            peak_duration_ = 0;
            peak_ = max_;
        }
    }
    set_dirty();
}

uint8_t RSSIGraph::get_graph_min() {
    return graph_min_;
}

uint8_t RSSIGraph::get_graph_avg() {
    return graph_avg_;
}

uint8_t RSSIGraph::get_graph_max() {
    return graph_max_;
}

uint8_t RSSIGraph::get_graph_delta() {
    return graph_max_ - graph_min_;
}

void RSSIGraph::paint(Painter& painter) {
    const auto r = screen_rect();

    if (graph_list.size() == 0)
        return;

    int xpos = 0, prev_xpos = r.width();
    RSSIGraph_entry& prev_entry = graph_list[0];

    graph_min_ = GRAPH_MIN_ALL_ZERO_FLAG;  // if it stays at that value the whole graphlist min are zero
    graph_max_ = 0;
    int avg_sum = 0;
    for (int n = 1; (unsigned)n <= graph_list.size(); n++) {
        xpos = (r.width() * (graph_list.size() - n)) / graph_list.size();
        int size = abs(xpos - prev_xpos);
        RSSIGraph_entry& entry = graph_list[n - 1];

        // stats
        if (entry.rssi_min != 0 && entry.rssi_min < graph_min_) {
            graph_min_ = entry.rssi_min;
        }
        if (entry.rssi_max > graph_max_) {
            graph_max_ = entry.rssi_max;
        }
        avg_sum += entry.rssi_avg;

        // black
        const Rect r0{r.right() - prev_xpos, r.top(), size, r.height()};
        painter.fill_rectangle(
            r0,
            Color::black());

        // y_max
        int top_y_val = max(entry.rssi_max, prev_entry.rssi_max);
        int width_y = abs(entry.rssi_max - prev_entry.rssi_max);
        if (width_y == 0)
            width_y = 1;
        const Point p1v{r.right() - prev_xpos, r.bottom() - top_y_val};
        painter.draw_vline(
            p1v,
            width_y,
            Color::red());
        const Point p1h{r.right() - prev_xpos, r.bottom() - entry.rssi_max};
        painter.draw_hline(
            p1h,
            size,
            Color::red());

        // y_avg
        top_y_val = max(entry.rssi_avg, prev_entry.rssi_avg);
        width_y = abs(entry.rssi_avg - prev_entry.rssi_avg);
        if (width_y == 0)
            width_y = 1;
        const Point p2v{r.right() - prev_xpos, r.bottom() - top_y_val};
        painter.draw_vline(
            p2v,
            width_y,
            Color::white());
        const Point p2h{r.right() - prev_xpos, r.bottom() - entry.rssi_avg};
        painter.draw_hline(
            p2h,
            size,
            Color::white());

        // y_min
        top_y_val = max(entry.rssi_min, prev_entry.rssi_min);
        width_y = abs(entry.rssi_min - prev_entry.rssi_min);
        if (width_y == 0)
            width_y = 1;
        const Point p3v{r.right() - prev_xpos, r.bottom() - top_y_val};
        painter.draw_vline(
            p3v,
            width_y,
            Color::blue());
        const Point p3h{r.right() - prev_xpos, r.bottom() - entry.rssi_min};
        painter.draw_hline(
            p3h,
            size,
            Color::blue());

        // hack to display db
        top_y_val = max(entry.db, prev_entry.db);
        width_y = abs(entry.db - prev_entry.db);
        if (width_y == 0)
            width_y = 1;
        const Point p4v{r.right() - prev_xpos, r.bottom() - top_y_val};
        painter.draw_vline(
            p4v,
            width_y,
            Color::green());
        const Point p4h{r.right() - prev_xpos, r.bottom() - entry.db};
        painter.draw_hline(
            p4h,
            size,
            Color::green());

        prev_entry = entry;
        prev_xpos = xpos;
    }
    graph_avg_ = (avg_sum / graph_list.size());
    // hack to only set to 0 if all graphlist min values are 0
    if (graph_min_ == GRAPH_MIN_ALL_ZERO_FLAG)
        graph_min_ = 0;
}

/*void RSSIGraph::paintOld(Painter& painter) {
        const auto r = screen_rect();
        int16_t size = r.width() /  nb_columns ;
        int16_t top_y_val = 0 ;
        int16_t width_y = 0 ;

        if( size < 1 )
            size = 1 ;

       int xpos = r.right() ;
        for ( int n = 2 ; (unsigned)n <= graph_list.size(); n++) {
            auto& entry = graph_list[graph_list.size()-n];
            auto& prev_entry = graph_list[graph_list.size()-(n-1)];

            // black
            const Rect r0{ xpos - size , r.top() , size , r.height() };
            painter.fill_rectangle(
                r0,
                Color::black());

            // y_max
            top_y_val = max( entry.rssi_max , prev_entry.rssi_max );
            width_y = abs(  entry.rssi_max - prev_entry.rssi_max  );
            if( width_y == 0 )
                width_y = 1 ;
            const Rect r1{ xpos - size , r.bottom() - top_y_val , size , width_y };
            painter.fill_rectangle(
                    r1,
                    Color::red());

            // y_avg
            top_y_val = max( entry.rssi_avg , prev_entry.rssi_avg );
            width_y = abs(  entry.rssi_avg - prev_entry.rssi_avg  );
            if( width_y == 0 )
                width_y = 1 ;
            const Rect r2{ xpos - size , r.bottom() - top_y_val , size , width_y };
            painter.fill_rectangle(
                    r2,
                    Color::white());

            // y_min
            top_y_val = max( entry.rssi_min , prev_entry.rssi_min );
            width_y = abs(  entry.rssi_min - prev_entry.rssi_min  );
            if( width_y == 0 )
                width_y = 1 ;
            const Rect r3{ xpos - size , r.bottom() - top_y_val , size , width_y };
            painter.fill_rectangle(
                    r3,
                    Color::blue());

            // hack to display db
            top_y_val = max( entry.db , prev_entry.db );
            width_y = abs(  entry.db - prev_entry.db );
            if( width_y == 0 )
                width_y = 1 ;
            const Rect r4{ xpos - size , r.bottom() - top_y_val , size , width_y };
            painter.fill_rectangle(
                    r4,
                    Color::green());
            xpos = xpos - size ;
        }
        if( xpos > r.left() )
        {
            const Rect r5{ r.left() , r.top() , xpos - r.left() , r.height() };
            painter.fill_rectangle(
                    r5,
                    Color::black());
        }
    }*/

void RSSIGraph::add_values(int16_t rssi_min, int16_t rssi_avg, int16_t rssi_max, int16_t db) {
    const auto r = screen_rect();

    constexpr int rssi_sample_range = 256;
    // constexpr float rssi_voltage_min = 0.4;
    constexpr float rssi_voltage_max = 2.2;
    constexpr float adc_voltage_max = 3.3;
    // constexpr int raw_min = rssi_sample_range * rssi_voltage_min / adc_voltage_max;
    constexpr int raw_min = 0;
    constexpr int raw_max = rssi_sample_range * rssi_voltage_max / adc_voltage_max;
    constexpr int raw_delta = raw_max - raw_min;

    // vertical bottom to top level meters
    const range_t<int> y_avg_range{0, r.height() - 1};
    const int16_t y_avg = y_avg_range.clip((rssi_avg - raw_min) * r.height() / raw_delta);
    const range_t<int> y_min_range{0, y_avg};
    const int16_t y_min = y_min_range.clip((rssi_min - raw_min) * r.height() / raw_delta);
    const range_t<int> y_max_range{y_avg + 1, r.height() - 1};
    const int16_t y_max = y_max_range.clip((rssi_max - raw_min) * r.height() / raw_delta);
    const range_t<int> db_range{-80, 10};
    int16_t db_ = db_range.clip(db);
    db_ = db_ - 10;
    db_ = db_ * r.height() / 90;
    db_ = r.height() + db_;

    graph_list.push_back({y_min, y_avg, y_max, db_});
    while (graph_list.size() > nb_columns) {
        graph_list.erase(graph_list.begin());
    }
    set_dirty();
}

void RSSIGraph::set_nb_columns(int16_t nb) {
    nb_columns = nb;
    while (graph_list.size() > nb_columns) {
        graph_list.erase(graph_list.begin());
    }
}

void RSSIGraph::on_hide() {
    nb_columns_before_hide = nb_columns;
    nb_columns = 1;
    while (graph_list.size() > nb_columns) {
        graph_list.erase(graph_list.begin());
    }
}

void RSSIGraph::on_show() {
    nb_columns = nb_columns_before_hide;
}

void RSSI::on_focus() {
    if (on_highlight)
        on_highlight(*this);
}

bool RSSI::on_key(const KeyEvent key) {
    if (key == KeyEvent::Select) {
        if (on_select) {
            on_select(*this);
            return true;
        }
    } else {
        if (on_dir) {
            return on_dir(*this, key);
        }
    }
    return false;
}

bool RSSI::on_touch(const TouchEvent event) {
    switch (event.type) {
        case TouchEvent::Type::Start:
            set_highlighted(true);
            set_dirty();
            if (on_touch_press) {
                on_touch_press(*this);
            }
            if (on_select && instant_exec_) {
                on_select(*this);
            }
            return true;
        case TouchEvent::Type::End:
            set_highlighted(false);
            set_dirty();
            if (on_touch_release) {
                on_touch_release(*this);
            }
            if (on_select && !instant_exec_) {
                on_select(*this);
            }
            return true;
        default:
            return false;
    }
}
} /* namespace ui */
