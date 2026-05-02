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

#include "location.hpp"
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

void Location::Angle::clear() {
    degrees = 255;
    minutes = 0;
    seconds = 0;
    orientation = false;
    floatValue = 255;
}

float Location::Angle::getFloatValue() {
    if (floatValue >= 255) {
        floatValue = (float)degrees;
        floatValue += ((float)minutes / 60.0f);
        floatValue += ((float)seconds / 3600.0f);
        if (orientation) {
            floatValue = -floatValue;
        }
    }
    return floatValue;
}

void Location::clear() {
    latitude.clear();
    longitude.clear();
}

bool Location::isUnknown() {
    return (latitude.degrees >= 255 || longitude.degrees >= 255);
}

char Location::gps_letterize(int x) {
    return (char)x + 65;
}

size_t Location::gps_compute_locator(char* buffer, float lat, float lon, int precision) {
    size_t result = 0;

    lon += 180.0f;
    lat += 90.0f;

    int A = lon / 20;
    int B = lat / 10;

    lon -= A * 20;
    lat -= B * 10;

    int C = lon / 2;
    int D = lat / 1;

    lon -= C * 2;
    lat -= D * 1;

    int E = lon / (5.0f / 60.0f);
    int F = lat / (2.5f / 60.0f);

    lon -= E * (5.0f / 60.0f);
    lat -= F * (2.5f / 60.0f);

    int G = lon / (5.0f / 600.0f);
    int H = lat / (2.5f / 600.0f);

    buffer[result++] = char('A' + A);
    buffer[result++] = char('A' + B);

    if (precision >= 4) {
        buffer[result++] = char('0' + C);
        buffer[result++] = char('0' + D);
    }

    if (precision >= 6) {
        buffer[result++] = char('a' + E);
        buffer[result++] = char('a' + F);
    }

    if (precision >= 8) {
        buffer[result++] = char('0' + G);
        buffer[result++] = char('0' + H);
    }
    buffer[result] = 0;
    return result;
}

size_t Location::toString(char* buffer, LocationFormat format, int precision) {
    if (isUnknown()) {
        return sprintf(buffer, "%s", "GPS not synchronized");
    }
    switch (format) {
        case LocationFormat::SEXAGESIMAL: {
            return sprintf(buffer, "%ld\xB0%02ld'%02ld\"%c, %ld\xB0%02ld'%02ld\"%c",  // 0xB0 is degree ° symbol in our 8x16 font
                           latitude.degrees, latitude.minutes, latitude.seconds, latitude.orientation ? 'S' : 'N',
                           longitude.degrees, longitude.minutes, longitude.seconds, longitude.orientation ? 'W' : 'E');
        }
        case LocationFormat::MAIDENHEAD_LOCATOR:
            return gps_compute_locator(buffer, latitude.getFloatValue(), longitude.getFloatValue(), precision);
        default:
        case LocationFormat::DECIMAL:
            return sprintf(buffer, "%s, %s", to_string_decimal(latitude.getFloatValue(), 5).c_str(), to_string_decimal(longitude.getFloatValue(), 5).c_str());
    }
}

void Location::formatFloatLocation(char* buffer, const char* format) {
    sprintf(buffer, format, to_string_decimal(latitude.getFloatValue(), 6).c_str(), to_string_decimal(longitude.getFloatValue(), 6).c_str());
}

}  // namespace ui::external_app::epirb_rx
