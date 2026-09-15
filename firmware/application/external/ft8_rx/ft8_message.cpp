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

#include "ft8_message.hpp"

#include <cstring>

namespace ui::external_app::ft8_rx {
namespace {

/* ft8_lib's unpacker is pulled in here rather than compiled as a separate C object on
 * purpose. External apps are linked out of the shared application image by a rule in
 * external.ld that matches both the object's path and the symbol's name, so plain C
 * symbols such as ftx_message_decode would stay behind in the firmware's own flash and
 * cost every user of the device a few kilobytes. Inside this anonymous namespace the
 * same code compiles with internal linkage in a translation unit that lives under
 * external/ft8_rx/, which is what the rule expects. */
/* In C, strchr on a const char* yields char*; C++ overloads it to return const char*,
 * which message.c assigns to a char*. Shimming it here keeps the vendored file identical
 * to its upstream copy. */
inline char* ft8_strchr(const char* s, int c) {
    return const_cast<char*>(std::strchr(s, c));
}
#define strchr ft8_strchr

/* stpcpy is POSIX rather than standard C++, and the ARM newlib headers do not declare it
 * in C++ mode. */
inline char* ft8_stpcpy(char* dst, const char* src) {
    while ((*dst = *src++) != '\0')
        dst++;
    return dst;
}
#define stpcpy ft8_stpcpy

#include "../../../baseband/ft8_lib/text.c"
#include "../../../baseband/ft8_lib/message.c"

#undef strchr
#undef stpcpy

}  // namespace

std::string payload_to_text(const uint8_t* payload) {
    ftx_message_t message{};
    std::memcpy(message.payload, payload, FTX_PAYLOAD_LENGTH_BYTES);

    char text[FTX_MAX_MESSAGE_LENGTH]{};
    ftx_message_offsets_t offsets{};

    /* A null hash interface renders callsigns that were sent hashed as <...>; keeping a
     * hash table would mean holding every callsign heard this session. */
    if (ftx_message_decode(&message, nullptr, text, &offsets) != FTX_MESSAGE_RC_OK) {
        text[0] = '\0';
        ftx_message_decode_free(&message, text);
    }

    return std::string{text};
}

}  // namespace ui::external_app::ft8_rx
