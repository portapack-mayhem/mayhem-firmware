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

/**
 * Add a beacon to the database.
 * @return the added beacon reference.
 */
Beacon& BeaconDB::add_beacon() {
    Beacon& result = recent_beacons[recent_beacon_pos];
    recent_beacon_pos = (recent_beacon_pos + 1) % BEACON_HISTORY_SIZE;
    if (recent_beacon_pos == 0) recent_beacon_full = true;
    return result;
}

/**
 * Get the size of the beacon database
 * @return the size of the beacon database
 */
size_t BeaconDB::size() {
    return recent_beacon_full ? BEACON_HISTORY_SIZE : recent_beacon_pos;
}

/**
 * Returns true if the database is empty
 * @return true if the database is empty
 */
bool BeaconDB::empty() {
    return (!recent_beacon_full && (recent_beacon_pos == 0));
}

/**
 * Returns the beacon at the provided index
 */
Beacon& BeaconDB::get_beacon(size_t index) {
    // Start from last beacon et go backward
    int16_t pos = (int16_t)recent_beacon_pos - 1 - index;
    while (pos < 0) pos += BEACON_HISTORY_SIZE;
    return recent_beacons[pos % BEACON_HISTORY_SIZE];
}

/**
 * Set currently selected beacon
 * @param index the selected index
 */
void BeaconDB::set_current_beacon(size_t index) {
    current_beacon_index = index;
}

/**
 * Get currently selected index
 * @return the currently selected index
 */
size_t BeaconDB::get_current_beacon_index() {
    return current_beacon_index;
}

/**
 * Get the currently selected beacon
 * @return the currently selected beacon
 */
Beacon& BeaconDB::get_current_beacon() {
    return get_beacon(current_beacon_index);
}

/**
 * Clear beacon database
 */
void BeaconDB::clear() {
    recent_beacon_pos = 0;
    recent_beacon_full = false;
}

}  // namespace ui::external_app::epirb_rx
