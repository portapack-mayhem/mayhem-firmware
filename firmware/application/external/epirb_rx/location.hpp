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

#include <cstddef>

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
        Angle(long degrees)
            : degrees(degrees) {}

        void clear();

        float getFloatValue();

       private:
        float floatValue = 255.0f;
    };

    enum class LocationFormat { DECIMAL,
                                SEXAGESIMAL,
                                MAIDENHEAD_LOCATOR };

    Angle latitude = Angle(127);
    Angle longitude = Angle(255);

    void clear();

    bool isUnknown();

    static char gps_letterize(int x);

    // For code size optimization reasons, string manipulation is performed with char buffers
    // with no char buffer size check. Caller methods take care of providing large enough buffers
    static size_t gps_compute_locator(char* buffer, float lat, float lon, int precision = 6);
    size_t toString(char* buffer, LocationFormat format, int precision = 6);

    void formatFloatLocation(char* buffer, const char* format);
};

}  // namespace ui::external_app::epirb_rx

#endif  // __LOCATION_RX_H__