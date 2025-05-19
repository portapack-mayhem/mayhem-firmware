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

#ifndef __UI_RSSI_H__
#define __UI_RSSI_H__

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_painter.hpp"
#include "event_m0.hpp"
#include "message.hpp"
#include <cstdint>

namespace ui {

class RSSI : public Widget {
   public:
    std::function<void(RSSI&)> on_select{};
    std::function<void(RSSI&)> on_touch_release{};  // Executed when releasing touch, after on_select.
    std::function<void(RSSI&)> on_touch_press{};    // Executed when touching, before on_select.
    std::function<bool(RSSI&, KeyEvent)> on_dir{};
    std::function<void(RSSI&)> on_highlight{};

    RSSI(Rect parent_rect, bool instant_exec);  // instant_exec: Execute on_select when you touching instead of releasing
    RSSI(
        Rect parent_rect)
        : RSSI{parent_rect, false} {
    }

    RSSI()
        : RSSI{{}, {}} {
    }

    // get last used/received min/avg/max/delta
    uint8_t get_min();
    uint8_t get_avg();
    uint8_t get_max();
    uint8_t get_delta();
    void set_vertical_rssi(bool enabled);
    void set_peak(bool enabled, size_t duration);

    void paint(Painter& painter) override;
    void on_focus() override;
    bool on_key(const KeyEvent key) override;
    bool on_touch(const TouchEvent event) override;
    void set_db(int16_t db);

   private:
    int8_t min_ = 0;
    int8_t avg_ = 0;
    int8_t max_ = 0;
    int8_t peak_ = 0;
    size_t peak_duration_ = 0;
    int16_t db_ = 0;
    bool instant_exec_{false};

    bool pitch_rssi_enabled = false;
    bool vertical_rssi_enabled = false;  // scale [vertically/from bottom to top]
                                         // instead of [horizontally/from left to right]
    bool peak_enabled = false;
    size_t peak_duration = 1000;  // peak duration in msec before being reset to actual max_rssi

    MessageHandlerRegistration message_handler_stats{
        Message::ID::RSSIStatistics,
        [this](const Message* const p) {
            this->on_statistics_update(static_cast<const RSSIStatisticsMessage*>(p)->statistics);
        }};

    MessageHandlerRegistration message_handler_pitch_rssi{
        Message::ID::PitchRSSIConfigure,
        [this](const Message* const p) {
            const auto message = *reinterpret_cast<const PitchRSSIConfigureMessage*>(p);
            this->set_pitch_rssi(message.enabled);
        }};

    void on_statistics_update(const RSSIStatistics& statistics);
    void set_pitch_rssi(bool enabled);
};

struct RSSIGraph_entry {
    int16_t rssi_min{0};
    int16_t rssi_avg{0};
    int16_t rssi_max{0};
    int16_t db{0};
};

using RSSIGraphList = std::vector<RSSIGraph_entry>;

class RSSIGraph : public Widget {
   public:
    RSSIGraph(
        const Rect parent_rect)
        : Widget{parent_rect} {
    }
    void paint(Painter& painter) override;
    void add_values(int16_t rssi_min, int16_t rssi_avg, int16_t rssi_max, int16_t db);
    void set_nb_columns(int16_t nb);

    void on_hide() override;
    void on_show() override;
    // get whole graph_list min/avg/max/delta
    uint8_t get_graph_min();
    uint8_t get_graph_avg();
    uint8_t get_graph_max();
    uint8_t get_graph_delta();

   private:
    int16_t graph_min_ = 0;
    int16_t graph_avg_ = 0;
    int16_t graph_max_ = 0;
    uint16_t nb_columns_before_hide = 16;
    uint16_t nb_columns = 16;
    RSSIGraphList graph_list{};
};

}  // namespace ui

#endif /*__UI_RSSI_H__*/
