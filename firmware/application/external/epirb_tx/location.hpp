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
#ifndef __LOCATION_H__
#define __LOCATION_H__

#include "portapack.hpp"
#include "ui_epirb_tx.hpp"
#include <cmath>
#include <cctype>
#include <cstdio>

namespace ui::external_app::epirb_tx {

/**
 * Convert decimal coordinates to sexagesimal coordinates
 * @param value the decimal value
 * @param negative output N(false)/S(true) or E(false)/W(true) value
 * @param deg output degrees value
 * @param min output minutes value
 * @param sec output seconds value
 */
static void decimal_to_dms(double value, bool& negative, uint16_t& deg, uint8_t& min, uint8_t& sec) {
    negative = value < 0;
    value = std::fabs(value);

    deg = (uint16_t)value;

    double m = (value - deg) * 60.0;
    min = (uint8_t)m;

    double s = (m - min) * 60.0;
    sec = (uint8_t)(s + 0.5);

    if (sec == 60) {
        sec = 0;
        min++;
    }

    if (min == 60) {
        min = 0;
        deg++;
    }
}

/**
 * Convert sexagesimal coordinates to decimal
 * @param negative N(false)/S(true) or E(false)/W(true) value
 * @param deg degrees value
 * @param min minutes value
 * @param sec seconds value
 * @return value the decimal value
 */
static double dms_to_decimal(bool negative, uint16_t deg, uint8_t min, uint8_t sec) {
    double v = deg + min / 60.0 + sec / 3600.0;
    return negative ? -v : v;
}

/**
 * Convert maidenhead locator to decima coordinates
 * @param loc the locator
 * @param lat output latitude
 * @param lon output longitude
 */
static void maidenhead_to_decimal(const std::string& loc, float& lat, float& lon) {
    static const double lon_step[] =
        {
            20.0,
            2.0,
            1.0 / 12.0,
            1.0 / 120.0,
            1.0 / 2880.0};

    static const double lat_step[] =
        {
            10.0,
            1.0,
            1.0 / 24.0,
            1.0 / 240.0,
            1.0 / 5760.0};

    lon = -180.0;
    lat = -90.0;

    int pairs = loc.size() / 2;
    if (pairs > 5)
        pairs = 5;

    for (int i = 0; i < pairs; i++) {
        char c1 = std::toupper(loc[i * 2]);
        char c2 = std::toupper(loc[i * 2 + 1]);

        double lon_size = lon_step[i];
        double lat_size = lat_step[i];

        int v1, v2;

        if (i % 2 == 0)  // letters
        {
            v1 = c1 - 'A';
            v2 = c2 - 'A';
        } else  // digits
        {
            v1 = c1 - '0';
            v2 = c2 - '0';
        }

        lon += v1 * lon_size;
        lat += v2 * lat_size;
    }

    // Cell center
    lon += lon_step[pairs - 1] / 2.0;
    lat += lat_step[pairs - 1] / 2.0;
}

/**
 * Convert decimal coordinates to maidenhead locator
 * @param lat the latitude
 * @param lon the longitude
 * @param precision (optional, defaults to 8) the number of charater for the maidenhead locator
 * @return the locator
 */
static std::string decimal_to_maidenhead(double lat, double lon, int precision = 8) {
    lon += 180.0;
    lat += 90.0;

    int A = lon / 20;
    int B = lat / 10;

    lon -= A * 20;
    lat -= B * 10;

    int C = lon / 2;
    int D = lat / 1;

    lon -= C * 2;
    lat -= D * 1;

    int E = lon / (5.0 / 60.0);
    int F = lat / (2.5 / 60.0);

    lon -= E * (5.0 / 60.0);
    lat -= F * (2.5 / 60.0);

    int G = lon / (5.0 / 600.0);
    int H = lat / (2.5 / 600.0);

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

/**
 * Init the provided Location values from its locator
 */
void init_from_locator(Location& loc) {
    maidenhead_to_decimal(loc.locator, loc.latitude, loc.longitude);

    decimal_to_dms(loc.latitude,
                   loc.south,
                   loc.lat_deg,
                   loc.lat_min,
                   loc.lat_sec);

    decimal_to_dms(loc.longitude,
                   loc.west,
                   loc.long_deg,
                   loc.long_min,
                   loc.long_sec);
}

/**
 * Init the provided Location values from its decimal coordinates
 */
void init_from_decimal(Location& loc) {
    decimal_to_dms(loc.latitude,
                   loc.south,
                   loc.lat_deg,
                   loc.lat_min,
                   loc.lat_sec);

    decimal_to_dms(loc.longitude,
                   loc.west,
                   loc.long_deg,
                   loc.long_min,
                   loc.long_sec);

    loc.locator = decimal_to_maidenhead(loc.latitude, loc.longitude);
}

/**
 * Init the provided Location values from its sexagesimal coordinates
 */
void init_from_dms(Location& loc) {
    loc.latitude = dms_to_decimal(
        loc.south,
        loc.lat_deg,
        loc.lat_min,
        loc.lat_sec);

    loc.longitude = dms_to_decimal(
        loc.west,
        loc.long_deg,
        loc.long_min,
        loc.long_sec);

    loc.locator = decimal_to_maidenhead(loc.latitude, loc.longitude);
}

/**
 * Convert the provided Loaction to it's latitude string (format <xxx°yy'zz"N>)
 */
std::string to_latitude_string(const Location& location) {
    char buffer[16];
    // Format :  <xxx°yy'zz"N>
    snprintf(buffer, sizeof(buffer), "%3d\260%02d'%02d\"%c", location.lat_deg, location.lat_min, location.lat_sec, location.south ? 'S' : 'N');
    return std::string(buffer);
}

/**
 * Convert the provided Loaction to it's longitude string (format <xxx°yy'zz"W>)
 */
std::string to_longitude_string(const Location& location) {
    char buffer[16];
    // Format :  <xxx°yy'zz"W>
    snprintf(buffer, sizeof(buffer), "%3d\260%02d'%02d\"%c", location.long_deg, location.long_min, location.long_sec, location.west ? 'W' : 'E');
    return std::string(buffer);
}

}  // namespace ui::external_app::epirb_tx

#endif /*__LOCATION_H__*/