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

namespace ui::external_app::epirb_rx {

class Location {
   public:
    class Angle {
       public:
        long degrees = 0;
        long minutes = 0;
        long seconds = 0;
        bool orientation = false;
        double getFloatValue();
        void clear();
        void toFloatString(char* angleStr);
        Angle();
        Angle(long degrees);

       private:
        double floatValue = 255;
    };

    enum class LocationFormat { DECIMAL,
                                SEXAGESIMAL,
                                MAIDENHEAD_LOCATOR };

    Angle latitude = Angle(127);
    Angle longitude = Angle(255);
    void clear();
    bool isUnknown();
    static char gps_letterize(int x);
    static void gps_compute_locator(double lat, double lon, char* gps_locator);
    std::string toString(LocationFormat format);
    void formatFloatLocation(char* buffer, size_t size, const char* format);

   private:
    std::string decimalFormat{};
    std::string sexagesimalFormat{};
    std::string locatorFormat{};
};

}  // namespace ui::external_app::epirb_rx

#endif  // __LOCATION_RX_H__