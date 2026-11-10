/*
 * Copyright (C) 2026 TehRabbitt, Rabbit-Labs LLC
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

#ifndef __LCD_TUNING_FILE_H__
#define __LCD_TUNING_FILE_H__

#include "lcd_ili9341.hpp"

/* Optional user LCD tuning in SETTINGS/lcd_gamma.txt.
 * Without the file, the built-in tuning for the selected panel type is used. */
namespace lcd_tuning_file {

enum class Status { Missing,
                    Ok,
                    Invalid };

/* Invalid covers oversized, unreadable or malformed files. They are ignored
 * at boot, and Reset to default in the LCD Gamma app deletes them. */
Status read(lcd::panel_tuning_t& t, bool& ips);
bool write(const lcd::panel_tuning_t& t, const bool ips);
void remove();

/* Loads the file if it matches the current panel type. The panel is only
 * rewritten when the result differs from what is on screen, or when force is set. */
void apply(const bool force);

}  // namespace lcd_tuning_file

#endif /*__LCD_TUNING_FILE_H__*/
