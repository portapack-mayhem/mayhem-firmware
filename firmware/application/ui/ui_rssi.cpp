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

#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))
#define abs(x) ((x)>0?(x):-(x))

namespace ui {

    void RSSI::paint(Painter& painter) {
        const auto r = screen_rect();

        constexpr int rssi_sample_range = 256;
        //constexpr float rssi_voltage_min = 0.4;
        constexpr float rssi_voltage_max = 2.2;
        constexpr float adc_voltage_max = 3.3;
        //constexpr int raw_min = rssi_sample_range * rssi_voltage_min / adc_voltage_max;
        constexpr int raw_min = 0 ;
        constexpr int raw_max = rssi_sample_range * rssi_voltage_max / adc_voltage_max;
        constexpr int raw_delta = raw_max - raw_min;


        if( !vertical_rssi_enabled )
        {
            // horizontal left to right level meters
            const range_t<int> x_avg_range { 0, r.width() - 1 };
            const auto x_avg = x_avg_range.clip((avg_ - raw_min) * r.width() / raw_delta);
            const range_t<int> x_min_range { 0, x_avg };
            const auto x_min = x_min_range.clip((min_ - raw_min) * r.width() / raw_delta);
            const range_t<int> x_max_range { x_avg + 1, r.width() - 1 };
            const auto x_max = x_max_range.clip((max_ - raw_min) * r.width() / raw_delta);
            const auto peak =  x_max_range.clip((peak_ - raw_min) * r.width() / raw_delta);

            // x_min
            const Rect r0 { r.left(), r.top(), x_min, r.height() };
            painter.fill_rectangle(
                    r0,
                    Color::blue()
                    );

            // x_avg
            const Rect r1 { r.left() + x_min, r.top(), x_avg - x_min, r.height() };
            painter.fill_rectangle(
                    r1,
                    Color::red()
                    );

            // x_avg middle marker
            const Rect r2 { r.left() + x_avg, r.top(), 1, r.height() };
            painter.fill_rectangle(
                    r2,
                    Color::white()
                    );

            // x_max
            const Rect r3 { r.left() + x_avg + 1, r.top(), x_max - (x_avg + 1), r.height() };
            painter.fill_rectangle(
                    r3,
                    Color::red()
                    );

            // filling last part in black
            const Rect r4 { r.left() + x_max, r.top(), r.width() - x_max, r.height() };
            painter.fill_rectangle(
                    r4,
                    Color::black()
                    );

            // show green peak value 
            if( peak_enabled )
            {
                const Rect r5 { r.left(), r.bottom() + peak , r.width() , 1 };
                painter.fill_rectangle(
                        r5,
                        Color::green()
                        );
            }
        }
        else
        {
            // vertical bottom to top level meters
            const range_t<int> y_avg_range { 0, r.height() - 1 };
            const auto y_avg = y_avg_range.clip((avg_ - raw_min) * r.height() / raw_delta);
            const range_t<int> y_min_range { 0, y_avg };
            const auto y_min = y_min_range.clip((min_ - raw_min) * r.height() / raw_delta);
            const range_t<int> y_max_range { y_avg + 1, r.height() - 1 };
            const auto y_max = y_max_range.clip((max_ - raw_min) * r.height() / raw_delta);
            const range_t<int> peak_range { 0, r.height() - 1 };
            const auto peak =  peak_range.clip((peak_ - raw_min) * r.height() / raw_delta);

            // y_min
            const Rect r0 { r.left(), r.bottom() - y_min, r.width() , y_min };
            painter.fill_rectangle(
                    r0,
                    Color::blue()
                    );

            // y_avg
            const Rect r1 { r.left(), r.bottom() - y_avg , r.width() , y_avg - y_min };
            painter.fill_rectangle(
                    r1,
                    Color::red()
                    );

            // y_avg middle marker
            const Rect r2 { r.left(), r.bottom() - y_avg , r.width() , 1 };
            painter.fill_rectangle(
                    r2,
                    Color::white()
                    );

            // y_max
            const Rect r3 {  r.left(), r.bottom() - y_max , r.width() , y_max - y_avg };
            //const Rect r3 {  r.left(), r.bottom() - y_max , r.width() , y_max - y_avg - 1 };
            painter.fill_rectangle(
                    r3,
                    Color::red()
                    );

            // fill last part of level in black
            const Rect r4 { r.left(), r.top() , r.width() , r.height() - y_max };
            painter.fill_rectangle(
                    r4,
                    Color::black()
                    );

            // show green peak value if enabled
            if( peak_enabled )
            {
                const Rect r5 { r.left(), r.bottom() - peak , r.width() , 1 };
                painter.fill_rectangle(
                        r5,
                        Color::green()
                        );
            }
        }
        if (pitch_rssi_enabled) {
            baseband::set_pitch_rssi((avg_ - raw_min) * 2000 / raw_delta, true);
        }
    }

    int32_t RSSI::get_min()
    { 
        return min_ ;
    }

    int32_t RSSI::get_avg()
    { 
        return avg_ ;
    }

    int32_t RSSI::get_max()
    { 
        return max_ ;
    }

    int32_t RSSI::get_delta()
    { 
        return max_ - min_ ;
    }


    void RSSI::set_pitch_rssi(bool enabled) {
        pitch_rssi_enabled = enabled;
        if (!enabled) baseband::set_pitch_rssi(0, false);
    }

    void RSSI::set_vertical_rssi(bool enabled) {
        if( enabled )
            vertical_rssi_enabled = true ;
        else
            vertical_rssi_enabled = false ;
    }

    void RSSI::set_peak(bool enabled, size_t duration) {
        peak_enabled = enabled ;
        peak_duration = duration ;
    }

    void RSSI::on_statistics_update(const RSSIStatistics& statistics) {
        min_ = statistics.min;
        avg_ = statistics.accumulator / statistics.count;
        max_ = statistics.max;
        if( peak_enabled )
        {
            peak_duration_ = peak_duration_ + 100 ;
            if( max_ > peak_ )
            {
                peak_ = max_ ;
            }
            if( peak_duration_ > peak_duration )
            {
                peak_duration_ = 0 ;
                peak_ = max_ ;
            }
        }
        set_dirty();
    }


    void RSSIGraph::paint(Painter& painter) {
        const auto r = screen_rect();

        for ( int n = 2 ; (unsigned)n <= graph_list.size(); n++) {
            auto& entry = graph_list[graph_list.size()-n];
            auto& prev_entry = graph_list[graph_list.size()-(n-1)];

            // black
            const Point p0{ r.right() - n , r.top() };
            painter.draw_vline(
                    p0,
                    r.height(),
                    Color::black());

            // y_max
            int32_t top_y_val = max( entry.rssi_max , prev_entry.rssi_max );
            int32_t width_y = abs(  entry.rssi_max - prev_entry.rssi_max  );
            if( width_y == 0 )
                width_y = 1 ;
            const Point p1{ r.right() - n , r.bottom() - top_y_val };
            painter.draw_vline(
                    p1,
                    width_y,
                    Color::red());

            // y_avg    
            top_y_val = max( entry.rssi_avg , prev_entry.rssi_avg );
            width_y = abs(  entry.rssi_avg - prev_entry.rssi_avg  );
            if( width_y == 0 )
                width_y = 1 ;
            const Point p2{ r.right() - n , r.bottom() - top_y_val };
            painter.draw_vline(
                    p2,
                    width_y,
                    Color::white());

            // y_min
            top_y_val = max( entry.rssi_min , prev_entry.rssi_min );
            width_y = abs(  entry.rssi_min - prev_entry.rssi_min  );
            if( width_y == 0 )
                width_y = 1 ;
            const Point p3{ r.right() - n , r.bottom() - top_y_val };
            painter.draw_vline(
                    p3,
                    width_y,
                    Color::blue());

            // hack to display db
            top_y_val = max( entry.db , prev_entry.db );
            width_y = abs(  entry.db - prev_entry.db );
            if( width_y == 0 )
                width_y = 1 ;
            const Point p4{ r.right() - n , r.bottom() - top_y_val };
            painter.draw_vline(
                    p4,
                    width_y,
                    Color::green() );
        }
    }

    void RSSIGraph::add_values(int32_t rssi_min, int32_t rssi_avg, int32_t rssi_max, int32_t db )
    {
        const auto r = screen_rect();

        constexpr int rssi_sample_range = 256;
        //constexpr float rssi_voltage_min = 0.4;
        constexpr float rssi_voltage_max = 2.2;
        constexpr float adc_voltage_max = 3.3;
        //constexpr int raw_min = rssi_sample_range * rssi_voltage_min / adc_voltage_max;
        constexpr int raw_min = 0 ;
        constexpr int raw_max = rssi_sample_range * rssi_voltage_max / adc_voltage_max;
        constexpr int raw_delta = raw_max - raw_min;

        // vertical bottom to top level meters
        const range_t<int> y_avg_range { 0, r.height() - 1 };
        const auto y_avg = y_avg_range.clip((rssi_avg - raw_min) * r.height() / raw_delta);
        const range_t<int> y_min_range { 0, y_avg };
        const auto y_min = y_min_range.clip((rssi_min - raw_min) * r.height() / raw_delta);
        const range_t<int> y_max_range { y_avg + 1, r.height() - 1 };
        const auto y_max = y_max_range.clip((rssi_max - raw_min) * r.height() / raw_delta);
        const range_t<int> db_range { -100 , 20 };
        auto db_ = db_range.clip( db );
        db_ = db_ - 20 ;
        db_ = db_ * r.height() / 120 ;
        db_ = r.height() + db_ ;

        graph_list . push_back( { y_min, y_avg, y_max , db_ } );
        if( graph_list.size() >  (unsigned)r.width() )
        {
            graph_list.erase( graph_list.begin() );
        }
        set_dirty();
    }

} /* namespace ui */
