/*
 * Copyright (C) 2026 Dmytro Onyshko
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

#ifndef __FT8_MESSAGE_H__
#define __FT8_MESSAGE_H__

#include <cstddef>
#include <cstdint>
#include <string>

namespace ui::external_app::ft8_rx {

/* Longest text an FT8 payload can unpack to: two callsigns, a report and terminator. */
constexpr size_t ft8_max_text_length = 35;

/* Unpacks the raw 77-bit payload a decode produced into displayable text.
 * Returns an empty string when the payload does not correspond to any known message type.
 *
 * The unpacker lives on the application core because ft8_lib's message and text modules
 * do not fit in the baseband image beside the decoder itself. Building it here, inside
 * this namespace, is also what keeps its code inside the external app's own flash slot
 * rather than in the shared firmware image. */
std::string payload_to_text(const uint8_t* payload);

}  // namespace ui::external_app::ft8_rx

#endif  // __FT8_MESSAGE_H__
