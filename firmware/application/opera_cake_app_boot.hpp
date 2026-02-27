/*
 * copyleft spammingdramaqueen
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

#ifndef __OPERA_CAKE_APP_BOOT_H__
#define __OPERA_CAKE_APP_BOOT_H__

#include <stdint.h>
#include "rf_path.hpp"

namespace opera_cake {

static constexpr uint8_t NUM_PORTS = 4;

struct FreqRanges {
    uint16_t mins[NUM_PORTS];  // MHz
    uint16_t maxs[NUM_PORTS];  // MHz
};

// Reads the last saved Opera Cake settings from the SD card and applies
// them to the board over I2C.  Must be called after portapack::init()
// (I2C bus up) and after the SD card is mounted.
// Safe to call even when no Opera Cake board is attached — the I2C probe
// fails quickly and the function returns without side-effects.
void restore_at_boot();

// Called by ReceiverModel whenever the tuning frequency changes.
// In frequency mode, looks up the new frequency in the stored ranges
// and switches ports if needed.  Fast no-op when mode != frequency
// or no board is detected.
void on_frequency_changed(rf::Frequency freq_hz);

// Called by the Opera Cake app to update the shared configuration.
// mode 0 = manual (applies port_a/port_b immediately),
// mode 1 = frequency (stores ranges; switching happens automatically
//          on every retune via on_frequency_changed).
// Returns true if the board was found and written successfully.
bool update_config(
    uint8_t mode,
    uint8_t port_a,
    uint8_t port_b,
    const FreqRanges& ranges);

// Returns true if an Opera Cake board has been detected on the I2C bus.
bool is_board_present();

}  // namespace opera_cake

#endif  // __OPERA_CAKE_APP_BOOT_H__
