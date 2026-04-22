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
    }

    inline bool isUnknown() {
        return (latitude.degrees >= 255 || longitude.degrees >= 255);
    }

    static inline char gps_letterize(int x) {
        return (char)x + 65;
    }

    static inline size_t gps_compute_locator(char* buffer, float lat, float lon, int precision = 6) {
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

    inline size_t toString(char* buffer, LocationFormat format, int precision = 6) {
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
                return gps_compute_locator(buffer,latitude.getFloatValue(), longitude.getFloatValue(), precision);
            default:
            case LocationFormat::DECIMAL:
                return sprintf(buffer,"%s, %s", to_string_decimal(latitude.getFloatValue(), 5).c_str(), to_string_decimal(longitude.getFloatValue(), 5).c_str());
        }
    }

    inline void formatFloatLocation(char* buffer, const char* format) {
        sprintf(buffer, format, to_string_decimal(latitude.getFloatValue(), 6).c_str(), to_string_decimal(longitude.getFloatValue(), 6).c_str());
    }
};

}  // namespace ui::external_app::epirb_rx

#endif  // __LOCATION_RX_H__