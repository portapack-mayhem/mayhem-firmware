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
#include <cstdio>

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

    static inline void gps_compute_locator(float lat, float lon, char* gps_locator) {
        if (lat < -90 || lat > 90 || lon < -180 || lon > 180) {
            gps_locator[0] = 0;
        } else {
            float LON_F[] = {20.0f, 2.0f, 0.083333f, 0.008333f, 0.0003472083333333333f};
            float LAT_F[] = {10.0f, 1.0f, 0.0416665f, 0.004166f, 0.0001735833333333333f};
            int i;
            lon += 180.0f;
            lat += 90.0f;

            for (i = 0; i < 5; i++) {
                if (i % 2 == 1) {
                    gps_locator[i * 2] = (char)(lon / LON_F[i] + '0');
                    gps_locator[i * 2 + 1] = (char)(lat / LAT_F[i] + '0');
                } else {
                    gps_locator[i * 2] = gps_letterize((int)(lon / LON_F[i]));
                    gps_locator[i * 2 + 1] = gps_letterize((int)(lat / LAT_F[i]));
                }
                lon = fmodf(lon, LON_F[i]);
                lat = fmodf(lat, LAT_F[i]);
            }
            gps_locator[10] = 0;
        }
    }

    inline void formatFloatLocation(char* buffer, size_t size, const char* format) {
        char latStr[16];
        latitude.toFloatString(latStr);
        char longStr[16];
        longitude.toFloatString(longStr);
        std::snprintf(buffer, size, format, latStr, longStr);
    }

    inline std::string toString(LocationFormat format) {
        if (isUnknown()) {
            return "GPS not synchronized";
        }
        switch (format) {
            case LocationFormat::SEXAGESIMAL:
                if (sexagesimalFormat.empty()) {
                    char buffer[64];
                    std::snprintf(buffer, sizeof(buffer), "%ld°%02ld'%02ld\"%c, %ld°%02ld'%02ld\"%c",
                                  latitude.degrees, latitude.minutes, latitude.seconds, latitude.orientation ? 'S' : 'N',
                                  longitude.degrees, longitude.minutes, longitude.seconds, longitude.orientation ? 'W' : 'E');
                    sexagesimalFormat = buffer;
                }
                return sexagesimalFormat;
            case LocationFormat::MAIDENHEAD_LOCATOR:
                if (locatorFormat.empty()) {
                    char buffer[32];
                    gps_compute_locator(latitude.getFloatValue(), longitude.getFloatValue(), buffer);
                    locatorFormat = buffer;
                }
                return locatorFormat;
            default:
            case LocationFormat::DECIMAL:
                if (decimalFormat.empty()) {
                    char buffer[64];
                    formatFloatLocation(buffer, sizeof(buffer), "%s, %s");
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