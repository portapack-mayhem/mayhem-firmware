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

#ifndef __BEACON_DB_H__
#define __BEACON_DB_H__

#include "beacon.hpp"

namespace ui::external_app::epirb_rx {

#define BEACON_HISTORY_SIZE 13

class BeaconDB {
   public:
    Beacon& add_beacon();
    Beacon& get_beacon(size_t index);
    Beacon& get_current_beacon();
    size_t get_current_beacon_index();
    void set_current_beacon(size_t index);
    size_t size();
    bool empty();
    void clear();

   private:
    Beacon recent_beacons[BEACON_HISTORY_SIZE];
    int8_t recent_beacon_pos{0};
    size_t current_beacon_index{0};
    bool recent_beacon_full{false};
};

}  // namespace ui::external_app::epirb_rx

#endif /* __BEACON_DB_H__ */
