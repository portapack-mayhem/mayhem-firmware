/*
 * Copyright (C) 2026 Frederic BORRY - ADRASEC 31
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

#ifndef __UI_BEACONLIST_H__
#define __UI_BEACONLIST_H__

#include "ui.hpp"
#include "ui_painter.hpp"
#include "ui_widget.hpp"
#include "beacon_db.hpp"

namespace ui::external_app::epirb_rx {

class BeaconUIList : public View {
   public:
    std::function<void(size_t)> on_select{};
    std::function<void()> on_leave{};  // Called when Right is pressed.

    BeaconUIList(Rect parent_rect);
    BeaconUIList(const BeaconUIList& other) = delete;
    BeaconUIList& operator=(const BeaconUIList& other) = delete;

    void paint(Painter& painter) override;
    void on_show() override;
    bool on_key(const KeyEvent key) override;
    bool on_encoder(EncoderEvent delta) override;
//    bool on_keyboard(const KeyboardEvent event) override;
    bool on_touch(const TouchEvent event) override;

    void set_index(size_t index);
    size_t get_index() const;
    void set_db(BeaconDB& db);

   private:
    void adjust_selected_index(int index);

    BeaconDB* db_{nullptr};
    size_t start_index_{0};
    size_t selected_index_{0};
};

}  // namespace ui::external_app::epirb_rx

#endif /*__UI_BEACONLIST_H__*/
