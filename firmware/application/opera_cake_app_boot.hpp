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

namespace opera_cake {

// Reads the last saved Opera Cake settings from the SD card and applies
// them to the board over I2C.  Must be called after portapack::init()
// (I2C bus up) and after the SD card is mounted.
// Safe to call even when no Opera Cake board is attached — the I2C probe
// fails quickly and the function returns without side-effects.
void restore_at_boot();

}  // namespace opera_cake

#endif  // __OPERA_CAKE_APP_BOOT_H__
