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

#include "dab_xpad.hpp"
#include "dab_crc.hpp"

#include <cstring>

namespace dab {

namespace {

constexpr size_t kFpadBytes = 2;
constexpr size_t kShortXpadBytes = 4;
constexpr size_t kMaxContentsIndicators = 4;
/* The M4 ships at most 128 bytes of PAD, two of them F-PAD. */
constexpr size_t kMaxXpadBytes = 128;

constexpr size_t kSubfieldLengths[8] = {4, 6, 8, 12, 16, 24, 32, 48};

bool is_start(uint8_t app) {
    return app == kXpadAppDlsStart || app == kXpadAppMotStart;
}

bool is_continue(uint8_t app) {
    return app == kXpadAppDlsContinue || app == kXpadAppMotContinue;
}

uint8_t base_app(uint8_t app) {
    return is_continue(app) ? static_cast<uint8_t>(app - 1) : app;
}

}  // namespace

size_t xpad_subfield_length(uint8_t length_index) {
    return kSubfieldLengths[length_index & 0x07];
}

void XpadParser::reset() {
    group_open_ = false;
    group_length_ = 0;
    group_app_ = 0;
    announced_length_ = 0;
    pending_length_ = 0;
    continuation_length_ = 0;
}

void XpadParser::begin(uint8_t app_type) {
    group_app_ = base_app(app_type);
    group_length_ = 0;
    group_open_ = true;
}

void XpadParser::finish_group() {
    group_open_ = false;
    group_length_ = 0;
    announced_length_ = 0;
}

void XpadParser::append(const uint8_t* data, size_t length) {
    if (!group_open_)
        return;

    const size_t room = kMaxDataGroupBytes - group_length_;
    if (length > room) {
        /* Abandon rather than emit a truncated group. */
        group_open_ = false;
        group_length_ = 0;
        return;
    }

    std::memcpy(&group_[group_length_], data, length);
    group_length_ += length;
}

/* DLS: two-byte prefix, (prefix & 0x0F) + 1 characters, two-byte CRC.
 * MOT: whatever a preceding length indicator announced. 0 means unknown. */
size_t XpadParser::expected_length() const {
    if (group_app_ == kXpadAppMotStart)
        return announced_length_;

    if (group_app_ != kXpadAppDlsStart || group_length_ < 1)
        return 0;

    const size_t chars = static_cast<size_t>(group_[0] & 0x0F) + 1;
    return 4 + chars;
}

void XpadParser::flush(XpadSink& sink) {
    if (!group_open_ || group_length_ == 0)
        return;

    if (group_app_ == kXpadAppDlsStart) {
        /* A CRC failure means the group was spliced from the wrong bytes. */
        const size_t expected = expected_length();
        if (expected != 0 && group_length_ >= expected) {
            const uint16_t crc = static_cast<uint16_t>(crc16_ccitt(group_, expected - 2) ^ 0xFFFF);
            if (crc == static_cast<uint16_t>((group_[expected - 2] << 8) | group_[expected - 1]))
                sink.on_dls_group(group_, expected);
        }
    } else if (group_app_ == kXpadAppMotStart)
        sink.on_mot_group(group_, group_length_);

    finish_group();
}

bool XpadParser::push_pad(const uint8_t* pad, size_t length, XpadSink& sink) {
    if (pad == nullptr || length < kFpadBytes)
        return false;

    const uint8_t* fpad = &pad[length - kFpadBytes];
    const uint8_t indicator = static_cast<uint8_t>((fpad[0] >> 4) & 0x03);

    if (indicator == 0)
        return true;  // no X-PAD
    if (indicator == 3)
        return false;  // reserved

    /* The X-PAD is byte-reversed before the F-PAD (EN 300 401 7.4); undo it
     * so the indicators come first and subfields read forward. */
    const size_t xpad_length = (length - kFpadBytes < kMaxXpadBytes) ? length - kFpadBytes : kMaxXpadBytes;
    if (xpad_length == 0)
        return false;
    uint8_t x[kMaxXpadBytes];
    for (size_t i = 0; i < xpad_length; i++)
        x[i] = pad[length - kFpadBytes - 1 - i];

    /* CI flag (F-PAD byte 2, bit 1): clear when the X-PAD continues the
     * subfield the last frame opened. */
    const bool ci_present = ((fpad[1] >> 1) & 1) != 0;

    if (!ci_present) {
        const size_t take = (indicator == 1) ? kShortXpadBytes : continuation_length_;
        if (group_open_ && take != 0 && take <= xpad_length) {
            append(x, take);
            const size_t expected = expected_length();
            if (expected != 0 && group_length_ >= expected)
                flush(sink);
        }
        return true;
    }

    uint8_t apps[kMaxContentsIndicators];
    size_t lengths[kMaxContentsIndicators];
    size_t count = 0;
    size_t ci_bytes = 0;

    if (indicator == 1) {
        /* Short X-PAD: one contents indicator, then three data bytes. */
        if (xpad_length < kShortXpadBytes)
            return false;
        apps[0] = static_cast<uint8_t>(x[0] & 0x1F);
        lengths[0] = 3;
        count = 1;
        ci_bytes = 1;
    } else {
        while (count < kMaxContentsIndicators && ci_bytes < xpad_length) {
            const uint8_t ci = x[ci_bytes++];
            const uint8_t app = static_cast<uint8_t>(ci & 0x1F);
            if (app == 0)
                break;  // end marker
            apps[count] = app;
            lengths[count] = xpad_subfield_length(static_cast<uint8_t>((ci >> 5) & 0x07));
            ++count;
        }
    }

    if (count == 0)
        return true;

    size_t offset = ci_bytes;
    size_t used = 0;
    for (size_t i = 0; i < count; i++) {
        const uint8_t app = apps[i];
        const size_t len = lengths[i];
        if (offset + len > xpad_length)
            break;  // window ends inside this subfield
        const uint8_t* data = &x[offset];
        offset += len;
        ++used;

        if (app == kXpadAppLengthIndicator) {
            /* 14-bit length then a CRC, for the group that follows. */
            if (len >= 2)
                pending_length_ = static_cast<size_t>(((data[0] & 0x3F) << 8) | data[1]);
            continue;
        }

        if (is_start(app)) {
            flush(sink);
            begin(app);
            /* After flush(), which clears it; it arrived earlier this frame. */
            announced_length_ = pending_length_;
            pending_length_ = 0;
            continuation_length_ = len;
            append(data, len);
        } else if (is_continue(app) && group_open_ && base_app(app) == group_app_) {
            continuation_length_ = len;
            append(data, len);
        } else {
            continue;
        }

        const size_t expected = expected_length();
        if (expected != 0 && group_length_ >= expected)
            flush(sink);
    }

    return used > 0;
}

} /* namespace dab */
