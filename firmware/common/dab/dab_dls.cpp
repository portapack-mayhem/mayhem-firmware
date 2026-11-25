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

#include "dab_dls.hpp"
#include "dab_charset.hpp"

#include <cstring>

namespace dab {

void DlsAssembler::reset() {
    text_[0] = '\0';
    new_text_ = false;
    current_toggle_ = -1;
    clear_segments();
}

void DlsAssembler::clear_segments() {
    for (size_t i = 0; i < kMaxDlsSegments; ++i) {
        segment_present_[i] = false;
        segment_length_[i] = 0;
    }
    last_segment_ = -1;
}

bool DlsAssembler::take_new_text() {
    const bool v = new_text_;
    new_text_ = false;
    return v;
}

bool DlsAssembler::push_segment(const uint8_t* data, size_t length) {
    if (data == nullptr || length < 2)
        return false;

    const uint8_t b0 = data[0];
    const uint8_t toggle = static_cast<uint8_t>((b0 >> 7) & 1);
    const bool first = ((b0 >> 6) & 1) != 0;
    const bool last = ((b0 >> 5) & 1) != 0;
    const bool command = ((b0 >> 4) & 1) != 0;

    if (command) {
        /* Command 1 is "clear display"; others are ignored. */
        const uint8_t code = static_cast<uint8_t>(b0 & 0x0F);
        if (code == 1) {
            reset();
            new_text_ = true;
            return true;
        }
        return false;
    }

    const size_t char_count = static_cast<size_t>(b0 & 0x0F) + 1;
    if (length < 2 + char_count)
        return false;

    const uint8_t segment_number = first ? 0 : static_cast<uint8_t>((data[1] >> 4) & 0x07);
    if (segment_number >= kMaxDlsSegments)
        return false;

    if (current_toggle_ != static_cast<int8_t>(toggle)) {
        clear_segments();
        current_toggle_ = static_cast<int8_t>(toggle);
    }

    if (first)
        charset_ = static_cast<uint8_t>(data[1] >> 4);
    std::memcpy(segments_[segment_number], &data[2], char_count);
    segments_[segment_number][char_count] = '\0';
    segment_length_[segment_number] = static_cast<uint8_t>(char_count);
    segment_present_[segment_number] = true;

    if (last)
        last_segment_ = static_cast<int8_t>(segment_number);

    if (last_segment_ < 0)
        return false;

    for (int8_t i = 0; i <= last_segment_; ++i) {
        if (!segment_present_[i])
            return false;
    }

    publish();
    return true;
}

void DlsAssembler::publish() {
    uint8_t raw[kMaxDlsChars];
    size_t raw_length = 0;
    for (int8_t i = 0; i <= last_segment_ && raw_length < kMaxDlsChars; ++i) {
        const size_t n = segment_length_[i];
        const size_t room = kMaxDlsChars - raw_length;
        const size_t take = (n < room) ? n : room;
        std::memcpy(&raw[raw_length], segments_[i], take);
        raw_length += take;
    }
    char assembled[kMaxDlsChars + 1];
    const size_t pos = to_display(raw, raw_length, charset_, assembled, kMaxDlsChars);
    assembled[pos] = '\0';

    /* Labels repeat on air; an unchanged one must not redraw the display. */
    if (std::strcmp(assembled, text_) == 0)
        return;

    std::memcpy(text_, assembled, pos + 1);
    new_text_ = true;
}

} /* namespace dab */
