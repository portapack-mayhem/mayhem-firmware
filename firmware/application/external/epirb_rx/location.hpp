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
#ifndef __LOCATION_RX_H__
#define __LOCATION_RX_H__

#include <cstdint>
#include <string>
#include <cmath>

namespace ui::external_app::epirb_rx {

class Location {
   public:
    class Angle {
       public:
        long degrees = 0;
        long minutes = 0;
        long seconds = 0;
        bool orientation = false;  // false = N/E, true = S/W

        Angle() {}
        Angle(long degrees) : degrees(degrees) {}

        inline void clear() {
            degrees = 255;
            minutes = 0;
            seconds = 0;
            orientation = false;
            floatValue = 255;
        }

        inline float getFloatValue() {
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

        inline void toFloatString(char* angleStr) {
            std::sprintf(angleStr, "%015.6f\n", getFloatValue());
        }

       private:
        float floatValue = 255.0f;
    };

    enum class LocationFormat { DECIMAL,
                                SEXAGESIMAL,
                                MAIDENHEAD_LOCATOR };

    Angle latitude = Angle(127);
    Angle longitude = Angle(255);

    inline void clear() {
        latitude.clear();
        longitude.clear();
        decimalFormat = "";
        sexagesimalFormat = "";
        locatorFormat = "";
    }

    inline bool isUnknown() {
        return (latitude.degrees >= 255 || longitude.degrees >= 255);
    }

    static inline char gps_letterize(int x) {
        return (char)x + 65;
    }

    static inline std::string gps_compute_locator(float lat, float lon, int precision = 6) {
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

        std::string locator;

        locator += char('A' + A);
        locator += char('A' + B);

        if (precision >= 4) {
            locator += char('0' + C);
            locator += char('0' + D);
        }

        if (precision >= 6) {
            locator += char('a' + E);
            locator += char('a' + F);
        }

        if (precision >= 8) {
            locator += char('0' + G);
            locator += char('0' + H);
        }
        return locator;
    }

    inline void formatFloatLocation(char* buffer, const char* format) {
        char latStr[16];
        latitude.toFloatString(latStr);
        char longStr[16];
        longitude.toFloatString(longStr);
        std::sprintf(buffer, format, latStr, longStr);
    }

    inline std::string toString(LocationFormat format) {
        if (isUnknown()) {
            return "GPS not synchronized";
        }
        switch (format) {
            case LocationFormat::SEXAGESIMAL:
                if (sexagesimalFormat.empty()) {
                    char buffer[64];
                    std::sprintf(buffer, "%ld°%02ld'%02ld\"%c, %ld°%02ld'%02ld\"%c",
                                  latitude.degrees, latitude.minutes, latitude.seconds, latitude.orientation ? 'S' : 'N',
                                  longitude.degrees, longitude.minutes, longitude.seconds, longitude.orientation ? 'W' : 'E');
                    sexagesimalFormat = buffer;
                }
                return sexagesimalFormat;
            case LocationFormat::MAIDENHEAD_LOCATOR:
                if (locatorFormat.empty()) {
                    locatorFormat = gps_compute_locator(latitude.getFloatValue(), longitude.getFloatValue());
                }
                return locatorFormat;
            default:
            case LocationFormat::DECIMAL:
                if (decimalFormat.empty()) {
                    char buffer[64];
                    formatFloatLocation(buffer, "%s, %s");
                    decimalFormat = buffer;
                }
                return decimalFormat;
        }
    }

   private:
    std::string decimalFormat{};
    std::string sexagesimalFormat{};
    std::string locatorFormat{};
};

}  // namespace ui::external_app::epirb_rx

#endif  // __LOCATION_RX_H__