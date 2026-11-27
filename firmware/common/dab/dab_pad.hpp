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

/* Programme-associated data (M0): X-PAD data groups and the dynamic
 * label (DLS). */

#ifndef __DAB_PAD_H__
#define __DAB_PAD_H__

#include <cstddef>
#include <cstdint>

namespace dab {

/* ---- Dynamic label ---- */

/* DLS assembly per ETSI EN 300 401 Annex E: up to 8 segments, each with a
 * 2-byte prefix. Byte 0: toggle 1 | first 1 | last 1 | C 1 | len-1 or cmd 4;
 * byte 1: charset 4 | rfa 4 (first) or rfa 1 | segment 3 | rfa 4 (others).
 * A toggle change starts a new label so two never splice; a partial label is
 * never published. */

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

/* ---- X-PAD ---- */

/* PAD extraction per ETSI EN 300 401 7.4. Each audio frame ends with 2 F-PAD
 * bytes; their X-PAD indicator is 0 none, 1 short (4 bytes), 2 variable, 3 rfu.
 * The X-PAD runs backwards from the F-PAD: contents indicators (length index 3 |
 * app type 5; app 0 or four end the list), then the subfields. App types 2/3
 * are DLS, 12/13 MOT; the even one starts a data group, the odd continues it. */

/* Announces the next data group's length; MOT needs it, DLS has its own. */
constexpr uint8_t kXpadAppLengthIndicator = 1;
constexpr uint8_t kXpadAppDlsStart = 2;
constexpr uint8_t kXpadAppDlsContinue = 3;
constexpr uint8_t kXpadAppMotStart = 12;
constexpr uint8_t kXpadAppMotContinue = 13;

constexpr size_t kMaxDataGroupBytes = 256;

size_t xpad_subfield_length(uint8_t length_index);

struct XpadSink {
    virtual ~XpadSink() = default;

    /* CRC-checked DLS segment, prefix and CRC included. */
    virtual void on_dls_group(const uint8_t* /*data*/, size_t /*length*/) {}

    virtual void on_mot_group(const uint8_t* /*data*/, size_t /*length*/) {}
};

class XpadParser {
   public:
    void reset();

    /* pad: a frame's trailing window ending in the two F-PAD bytes. False if
     * too short or the indicator is reserved. */
    bool push_pad(const uint8_t* pad, size_t length, XpadSink& sink);

   private:
    void flush(XpadSink& sink);
    size_t expected_length() const;
    void begin(uint8_t app_type);
    void finish_group();
    void append(const uint8_t* data, size_t length);

    uint8_t group_app_{0};
    size_t announced_length_{0};
    size_t pending_length_{0};

    /* A continuation frame's share, sized by the last indicator; the rest of
     * the caller's window is audio and must not be appended. */
    size_t continuation_length_{0};
    uint8_t group_[kMaxDataGroupBytes]{};
    size_t group_length_{0};
    bool group_open_{false};
};

} /* namespace dab */

#endif /*__DAB_PAD_H__*/
