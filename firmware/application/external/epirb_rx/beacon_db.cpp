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

#include "beacon_db.hpp"

namespace ui::external_app::epirb_rx {

Beacon& BeaconDB::add_beacon() {
    Beacon& result = recent_beacons[recent_beacon_pos];
    recent_beacon_pos = (recent_beacon_pos + 1) % BEACON_HISTORY_SIZE;
    if (recent_beacon_pos == 0) recent_beacon_full = true;
    return result;
}

size_t BeaconDB::size() {
    return recent_beacon_full ? BEACON_HISTORY_SIZE : recent_beacon_pos;
}

bool BeaconDB::empty() {
    return (!recent_beacon_full && (recent_beacon_pos == 0));
}

Beacon& BeaconDB::get_beacon(size_t index){
    int16_t pos = (int16_t)recent_beacon_pos - 1 - index;
    while (pos < 0) pos += BEACON_HISTORY_SIZE;
    return recent_beacons[pos % BEACON_HISTORY_SIZE];
}

Beacon& BeaconDB::get_latest_beacon(){
    return get_beacon(0);
}

void BeaconDB::clear() {
    recent_beacon_pos = 0;
    recent_beacon_full = false;
}

}  // namespace ui::external_app::epirb_rx
