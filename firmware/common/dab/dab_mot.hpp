/*
 * Copyright (C) 2026 Mayhem firmware contributors
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

/* MOT slideshow per ETSI EN 301 234 / TS 101 499. MSC data group: ext 1 | crc 1 |
 * seg 1 | UA 1 | type 4, CI 4 | RI 4, [ext 16], [last 1 | segment 15], [user
 * access, holds the transport id], data, [CRC 16]. Type 3 is the MOT header, 4 a
 * body segment; all but the last segment share one size, so each can be placed
 * on arrival. A different transport id starts a new object. */

#ifndef __DAB_MOT_H__
#define __DAB_MOT_H__

#include <cstddef>
#include <cstdint>

namespace dab {

/* Under the 64 KB MOT limit: the M0 has 64 KB of AHB RAM in total. Larger
 * objects are abandoned, not truncated. */
constexpr size_t kMaxMotObjectBytes = 32768;
constexpr size_t kMaxMotSegments = 128;

constexpr uint8_t kMotContentTypeImage = 2;

class MotAssembler {
   public:
    MotAssembler() { reset(); }

    void reset();

    /* True when a complete object became available. */
    bool push_data_group(const uint8_t* data, size_t length);

    const uint8_t* image() const { return buffer_; }
    size_t image_size() const { return image_size_; }
    uint8_t content_type() const { return content_type_; }
    uint16_t content_subtype() const { return content_subtype_; }
    uint16_t transport_id() const { return transport_id_; }

    bool take_new_image();

   private:
    void start_object(uint16_t transport_id);
    bool place_segment(uint16_t segment_number, const uint8_t* data, size_t length);
    bool try_complete();

    uint8_t buffer_[kMaxMotObjectBytes]{};
    size_t image_size_{0};
    bool new_image_{false};

    uint16_t transport_id_{0};
    bool have_transport_id_{false};

    uint32_t declared_body_size_{0};
    bool have_header_{false};
    uint8_t content_type_{0};
    uint16_t content_subtype_{0};

    uint16_t segment_length_[kMaxMotSegments]{};
    bool segment_present_[kMaxMotSegments]{};
    uint16_t nominal_segment_size_{0};
    int32_t last_segment_{-1};
    bool abandoned_{false};

    /* Refused until a new object; a corrupt segment only spoils this carousel
     * pass and is retried next time round. */
    bool oversized_{false};

    /* A last segment that arrived before the stride was known. */
    uint8_t pending_[512]{};
    uint16_t pending_length_{0};
    int32_t pending_segment_{-1};
};

} /* namespace dab */

#endif /*__DAB_MOT_H__*/
