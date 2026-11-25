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

/* DLS assembly per ETSI EN 300 401 Annex E: up to 8 segments, each with a
 * 2-byte prefix. Byte 0: toggle 1 | first 1 | last 1 | C 1 | len-1 or cmd 4;
 * byte 1: charset 4 | rfa 4 (first) or rfa 1 | segment 3 | rfa 4 (others).
 * A toggle change starts a new label so two never splice; a partial label is
 * never published. */

#ifndef __DAB_DLS_H__
#define __DAB_DLS_H__

#include <cstddef>
#include <cstdint>

namespace dab {

constexpr size_t kMaxDlsSegments = 8;
constexpr size_t kMaxDlsChars = 128;

class DlsAssembler {
   public:
    DlsAssembler() { reset(); }

    void reset();

    /* True when this segment completed a label, even an unchanged one;
     * take_new_text() reports a change. */
    bool push_segment(const uint8_t* data, size_t length);

    const char* text() const { return text_; }

    bool take_new_text();

   private:
    void publish();
    void clear_segments();

    char text_[kMaxDlsChars + 1]{};
    bool new_text_{false};
    uint8_t charset_{0};

    char segments_[kMaxDlsSegments][17]{};
    uint8_t segment_length_[kMaxDlsSegments]{};
    bool segment_present_[kMaxDlsSegments]{};
    int8_t last_segment_{-1};
    int8_t current_toggle_{-1};
};

} /* namespace dab */

#endif /*__DAB_DLS_H__*/
