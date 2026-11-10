/*
 * Copyright (C) 2026 justin080
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

/*
 * M0 <-> M4 protocol of the Seewetter (DWD RTTY) app.
 *
 * The app deliberately reuses the existing RTTYDataMessage (and the existing
 * baseband::set_rtty_config() API) instead of new message IDs, so the main
 * firmware stays byte identical and the .ppma runs on the official build of
 * the same version. The 'stopbits' field carries the frame kind.
 *
 * M0 -> M4 (config):  kind = KIND_CONFIG, baud (x100), shift (Hz),
 *                     data[0] = polarity (0 auto, 1 normal, 2 inverted),
 *                     data[1] = AFC (0/1), data[2] = squelch (0..3), data_len = 3
 * M4 -> M0 (text):    kind = KIND_TEXT, data[] = 5 bit ITA2 codes
 * M4 -> M0 (status):  kind = KIND_STATUS, data[0..SPECTRUM_BINS-1] = spectrum,
 *                     mark_tone = applied AFC offset (Hz), space_tone = measured
 *                     signal centre (Hz), baud = quality (0..100), shift = flags
 */

#ifndef __SEEWETTER_PROTOCOL_HPP__
#define __SEEWETTER_PROTOCOL_HPP__

#include <cstdint>

namespace seewetter {

constexpr uint8_t KIND_CONFIG = 0xC5;
constexpr uint8_t KIND_TEXT = 0xD5;
constexpr uint8_t KIND_STATUS = 0xE5;

constexpr uint16_t FLAG_INVERTED = 1 << 0;  // inverted polarity is active
constexpr uint16_t FLAG_LOCKED = 1 << 1;    // two tone signal with the set shift found
constexpr uint16_t FLAG_SQUELCH_OPEN = 1 << 2;
constexpr uint16_t FLAG_AFC = 1 << 3;

constexpr uint8_t SPECTRUM_BINS = 80;  // +-1250 Hz around the tuned frequency
constexpr uint16_t SPECTRUM_HZ_PER_BIN_X100 = 3125;

}  // namespace seewetter

#endif /*__SEEWETTER_PROTOCOL_HPP__*/
