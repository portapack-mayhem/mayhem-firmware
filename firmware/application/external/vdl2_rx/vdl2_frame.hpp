/*
 * Copyright (C) 2026 Xmoo26
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

#ifndef __VDL2_FRAME_H__
#define __VDL2_FRAME_H__

#include <cstddef>
#include <cstdint>
#include <string>

namespace ui::external_app::vdl2_rx {

// What an AVLC frame says: who sends it to whom and, for ACARS, the message.
struct Frame {
    enum class Kind { ACARS,
                      Data,
                      Link };

    uint32_t src{0};
    uint32_t dst{0};
    uint8_t src_type{0};  // 1 aircraft, 4 / 5 ground station, 7 all
    uint8_t dst_type{0};
    Kind kind{Kind::Link};

    // ACARS (ARINC 618)
    char mode{0};
    char block_id{0};
    std::string reg{};
    std::string label{};
    std::string msg_num{};
    std::string flight{};
    std::string text{};

    bool from_aircraft() const { return src_type == 1; }
    bool to_aircraft() const { return dst_type == 1; }
    // the aircraft this frame is about, or 0
    uint32_t aircraft() const { return from_aircraft() ? src : (to_aircraft() ? dst : 0); }
    // the ground station sending it, or 0
    uint32_t ground_station() const { return (src_type == 4 || src_type == 5) ? src : 0; }
};

// frame: the AVLC frame without its FCS
bool parse_frame(const uint8_t* data, const size_t length, Frame& frame);

}  // namespace ui::external_app::vdl2_rx

#endif /*__VDL2_FRAME_H__*/
