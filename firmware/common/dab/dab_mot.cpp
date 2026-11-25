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

#include "dab_mot.hpp"

#include <cstring>

namespace dab {

namespace {
constexpr uint8_t kGroupTypeMotHeader = 3;
constexpr uint8_t kGroupTypeMotBody = 4;
}  // namespace

void MotAssembler::reset() {
    image_size_ = 0;
    new_image_ = false;
    transport_id_ = 0;
    have_transport_id_ = false;
    declared_body_size_ = 0;
    have_header_ = false;
    content_type_ = 0;
    content_subtype_ = 0;
    nominal_segment_size_ = 0;
    last_segment_ = -1;
    pending_segment_ = -1;
    pending_length_ = 0;
    abandoned_ = false;
    oversized_ = false;

    for (size_t i = 0; i < kMaxMotSegments; ++i) {
        segment_present_[i] = false;
        segment_length_[i] = 0;
    }
}

bool MotAssembler::take_new_image() {
    const bool v = new_image_;
    new_image_ = false;
    return v;
}

void MotAssembler::start_object(uint16_t transport_id) {
    for (size_t i = 0; i < kMaxMotSegments; ++i) {
        segment_present_[i] = false;
        segment_length_[i] = 0;
    }
    nominal_segment_size_ = 0;
    last_segment_ = -1;
    pending_segment_ = -1;
    pending_length_ = 0;
    have_header_ = false;
    declared_body_size_ = 0;
    abandoned_ = false;
    oversized_ = false;
    transport_id_ = transport_id;
    have_transport_id_ = true;
}

bool MotAssembler::push_data_group(const uint8_t* data, size_t length) {
    if (data == nullptr || length < 2)
        return false;

    const bool extension_flag = ((data[0] >> 7) & 1) != 0;
    const bool crc_flag = ((data[0] >> 6) & 1) != 0;
    const bool segment_flag = ((data[0] >> 5) & 1) != 0;
    const bool user_access_flag = ((data[0] >> 4) & 1) != 0;
    const uint8_t group_type = static_cast<uint8_t>(data[0] & 0x0F);

    if (group_type != kGroupTypeMotHeader && group_type != kGroupTypeMotBody)
        return false;

    size_t pos = 2;

    if (extension_flag) {
        if (pos + 2 > length)
            return false;
        pos += 2;
    }

    uint16_t segment_number = 0;
    bool last = true;

    if (segment_flag) {
        if (pos + 2 > length)
            return false;
        last = ((data[pos] >> 7) & 1) != 0;
        segment_number = static_cast<uint16_t>(((data[pos] & 0x7F) << 8) | data[pos + 1]);
        pos += 2;
    }

    uint16_t group_transport_id = 0;
    bool have_id = false;

    if (user_access_flag) {
        if (pos + 1 > length)
            return false;
        const bool transport_id_flag = ((data[pos] >> 4) & 1) != 0;
        const uint8_t length_indicator = static_cast<uint8_t>(data[pos] & 0x0F);
        ++pos;

        if (pos + length_indicator > length)
            return false;

        if (transport_id_flag) {
            if (length_indicator < 2)
                return false;
            group_transport_id = static_cast<uint16_t>((data[pos] << 8) | data[pos + 1]);
            have_id = true;
        }
        pos += length_indicator;
    }

    size_t data_end = length;
    if (crc_flag) {
        if (data_end < pos + 2)
            return false;
        data_end -= 2;
    }

    if (data_end < pos)
        return false;

    const size_t field_length = data_end - pos;

    if (have_id && (!have_transport_id_ || group_transport_id != transport_id_))
        start_object(group_transport_id);

    if (!have_transport_id_)
        start_object(group_transport_id);

    /* Carousel: segment 0 starts a fresh copy, so retry an abandoned object. */
    if (abandoned_ && !oversized_ && group_type == kGroupTypeMotBody && segment_flag && segment_number == 0)
        start_object(have_transport_id_ ? transport_id_ : group_transport_id);

    if (abandoned_)
        return false;

    if (group_type == kGroupTypeMotHeader) {
        if (field_length < 7)
            return false;

        declared_body_size_ = (static_cast<uint32_t>(data[pos]) << 20) |
                              (static_cast<uint32_t>(data[pos + 1]) << 12) |
                              (static_cast<uint32_t>(data[pos + 2]) << 4) |
                              (static_cast<uint32_t>(data[pos + 3]) >> 4);

        /* Core header: BodySize(28) HeaderSize(13) ContentType(6)
         * ContentSubType(9). */
        content_type_ = static_cast<uint8_t>((data[pos + 5] >> 1) & 0x3F);
        content_subtype_ = static_cast<uint16_t>(((data[pos + 5] & 0x01) << 8) | data[pos + 6]);

        if (declared_body_size_ > kMaxMotObjectBytes) {
            abandoned_ = true;
            oversized_ = true;
            return false;
        }

        have_header_ = true;
        return try_complete();
    }

    if (segment_number >= kMaxMotSegments) {
        abandoned_ = true;
        return false;
    }

    if (!last && nominal_segment_size_ == 0)
        nominal_segment_size_ = static_cast<uint16_t>(field_length);

    if (last)
        last_segment_ = static_cast<int32_t>(segment_number);

    if (nominal_segment_size_ == 0) {
        if (segment_number == 0) {
            /* A single-segment object: its own length is the stride. */
            nominal_segment_size_ = static_cast<uint16_t>(field_length);
        } else {
            /* Last segment before any full-size one: offset unknown, hold it. */
            if (field_length > sizeof(pending_))
                return false;
            std::memcpy(pending_, &data[pos], field_length);
            pending_length_ = static_cast<uint16_t>(field_length);
            pending_segment_ = static_cast<int32_t>(segment_number);
            return false;
        }
    }

    if (!place_segment(segment_number, &data[pos], field_length))
        return false;

    if (pending_segment_ >= 0) {
        const int32_t held = pending_segment_;
        pending_segment_ = -1;
        if (!place_segment(static_cast<uint16_t>(held), pending_, pending_length_))
            return false;
    }

    return try_complete();
}

bool MotAssembler::place_segment(uint16_t segment_number, const uint8_t* data, size_t length) {
    /* A short non-final segment would leave a hole that image_size_, the sum
     * of the lengths, does not show. */
    const bool is_last = (last_segment_ >= 0) && (static_cast<int32_t>(segment_number) == last_segment_);
    if (!is_last && length != nominal_segment_size_) {
        abandoned_ = true;
        return false;
    }

    const size_t offset = static_cast<size_t>(segment_number) * nominal_segment_size_;
    if (offset + length > kMaxMotObjectBytes) {
        abandoned_ = true;
        return false;
    }

    std::memcpy(&buffer_[offset], data, length);
    segment_length_[segment_number] = static_cast<uint16_t>(length);
    segment_present_[segment_number] = true;
    return true;
}

bool MotAssembler::try_complete() {
    if (abandoned_ || last_segment_ < 0)
        return false;

    size_t total = 0;
    for (int32_t i = 0; i <= last_segment_; ++i) {
        if (!segment_present_[i])
            return false;
        total += segment_length_[i];
    }

    if (have_header_ && declared_body_size_ != 0 && total != declared_body_size_)
        return false;

    image_size_ = total;
    new_image_ = true;
    return true;
}

} /* namespace dab */
