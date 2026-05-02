/*
 * Copyright (C) 2026 Frederic BORRY - ADRASEC 31
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

#include "beacon.hpp"
#include <cstdio>
#include "string_format.hpp"

namespace ui::external_app::epirb_rx {

size_t Beacon::formatSummary(char* buffer, bool with_time) {
    size_t result = 0;
    if (with_time) {
        result += formatTime(buffer);
        buffer[result++] = '-';
    }
    result += sprintf((buffer + result), "%4s-%5s-", shortId().c_str(), getType());
    if (location.isUnknown()) {
        result += sprintf((buffer + result), "      ");
    } else {
        result += location.toString((buffer + result), Location::LocationFormat::MAIDENHEAD_LOCATOR);
    }
    result += sprintf((buffer + result), "[%s%s%s]", isFrameValid() ? (bch1Corrected || bch2Corrected) ? STR_COLOR_YELLOW : STR_COLOR_GREEN : STR_COLOR_RED, getSatus().c_str(), STR_COLOR_WHITE);
    return result;
}

bool Beacon::isFrameValid() {
    return isBch1Valid() && ((!hasBch2) || isBch2Valid()) && (!isEmpty);
}

std::string Beacon::shortId() {
    std::string result = std::string(hexId);
    return (result.size() >= 4) ? result.substr(0, 4) : result;
}

size_t Beacon::formatTime(char* buffer) {
    return sprintf(buffer, "%02d:%02d:%02d", date.hour(), date.minute(), date.second());
}

}  // namespace ui::external_app::epirb_rx
