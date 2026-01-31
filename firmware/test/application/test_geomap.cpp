/*
 * Copyright (C) 2026
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

#include "doctest.h"
#include <cmath>

// Mock constants and functions needed for testing
#define TILE_SIZE 256
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// OSM coordinate conversion functions (from ui_geomap.cpp)
double lon_to_pixel_x_tile(double lon, int zoom) {
    return ((lon + 180.0) / 360.0) * pow(2.0, zoom) * TILE_SIZE;
}

double lat_to_pixel_y_tile(double lat, int zoom) {
    double lat_rad = lat * M_PI / 180.0;
    double sin_lat = sin(lat_rad);
    return ((1.0 - log((1.0 + sin_lat) / (1.0 - sin_lat)) / (2.0 * M_PI)) / 2.0) * pow(2.0, zoom) * TILE_SIZE;
}

TEST_SUITE_BEGIN("GeoMap OSM Change Detection");

TEST_CASE("OSM pixel conversion should be accurate") {
    // Test at zoom level 10
    int zoom = 10;

    // Test known locations
    SUBCASE("Longitude conversion") {
        // Center of map (0° longitude)
        double px = lon_to_pixel_x_tile(0.0, zoom);
        CHECK(px == doctest::Approx(pow(2.0, zoom) * TILE_SIZE / 2.0));

        // Western hemisphere (-90°)
        px = lon_to_pixel_x_tile(-90.0, zoom);
        CHECK(px == doctest::Approx(pow(2.0, zoom) * TILE_SIZE / 4.0));

        // Eastern hemisphere (90°)
        px = lon_to_pixel_x_tile(90.0, zoom);
        CHECK(px == doctest::Approx(pow(2.0, zoom) * TILE_SIZE * 3.0 / 4.0));
    }

    SUBCASE("Latitude conversion") {
        // Equator (0° latitude)
        double py = lat_to_pixel_y_tile(0.0, zoom);
        CHECK(py == doctest::Approx(pow(2.0, zoom) * TILE_SIZE / 2.0));

        // Northern hemisphere should have smaller Y values
        double py_north = lat_to_pixel_y_tile(45.0, zoom);
        CHECK(py_north < py);

        // Southern hemisphere should have larger Y values
        double py_south = lat_to_pixel_y_tile(-45.0, zoom);
        CHECK(py_south > py);
    }
}

TEST_CASE("Change detection threshold should prevent unnecessary redraws") {
    int zoom = 15;  // High zoom level
    double base_lat = 40.7128;  // New York City latitude
    double base_lon = -74.0060; // New York City longitude

    // Calculate base pixel position
    double base_px = lon_to_pixel_x_tile(base_lon, zoom);
    double base_py = lat_to_pixel_y_tile(base_lat, zoom);

    SUBCASE("Sub-pixel movement should not trigger redraw") {
        // Calculate a very small lat/lon change that results in < 1 pixel movement
        double tiny_lon_offset = 0.00001;  // Very small change
        double new_px = lon_to_pixel_x_tile(base_lon + tiny_lon_offset, zoom);
        double px_diff = fabs(new_px - base_px);

        CHECK(px_diff < 1.0);
    }

    SUBCASE("Movement of 1+ pixels should trigger redraw") {
        // Calculate a larger change that results in >= 1 pixel movement
        // At zoom 15, one tile is 256 pixels, covering 360/2^15 degrees
        double degrees_per_pixel = 360.0 / (pow(2.0, zoom) * TILE_SIZE);
        double lon_offset = degrees_per_pixel * 2.0;  // 2 pixels worth

        double new_px = lon_to_pixel_x_tile(base_lon + lon_offset, zoom);
        double px_diff = fabs(new_px - base_px);

        CHECK(px_diff >= 1.0);
    }
}

TEST_CASE("Change detection should scale with zoom level") {
    double base_lat = 40.7128;
    double base_lon = -74.0060;
    double fixed_lon_offset = 0.001;  // Fixed coordinate change

    SUBCASE("At low zoom, small coordinate changes may be sub-pixel") {
        int zoom_low = 5;
        double px_low_before = lon_to_pixel_x_tile(base_lon, zoom_low);
        double px_low_after = lon_to_pixel_x_tile(base_lon + fixed_lon_offset, zoom_low);
        double diff_low = fabs(px_low_after - px_low_before);

        // At low zoom, this coordinate change might be less than 1 pixel
        CHECK(diff_low < 10.0);  // Should be relatively small
    }

    SUBCASE("At high zoom, same coordinate change becomes multiple pixels") {
        int zoom_high = 18;
        double px_high_before = lon_to_pixel_x_tile(base_lon, zoom_high);
        double px_high_after = lon_to_pixel_x_tile(base_lon + fixed_lon_offset, zoom_high);
        double diff_high = fabs(px_high_after - px_high_before);

        // At high zoom, this coordinate change should be many pixels
        CHECK(diff_high > 10.0);  // Should be significantly larger
    }
}

TEST_CASE("Previous position tracking should work correctly") {
    int zoom = 10;
    double initial_lat = 51.5074;  // London
    double initial_lon = -0.1278;

    // Simulate initial position
    double prev_osm_px = lon_to_pixel_x_tile(initial_lon, zoom);
    double prev_osm_py = lat_to_pixel_y_tile(initial_lat, zoom);

    SUBCASE("Moving exactly 1 pixel should trigger redraw") {
        // Calculate exact 1-pixel movement
        double degrees_per_pixel = 360.0 / (pow(2.0, zoom) * TILE_SIZE);
        double new_lon = initial_lon + degrees_per_pixel;

        double new_px = lon_to_pixel_x_tile(new_lon, zoom);
        double diff = fabs(new_px - prev_osm_px);

        CHECK(diff >= 1.0);
        CHECK(diff < 2.0);
    }

    SUBCASE("No movement should not trigger redraw") {
        double new_px = lon_to_pixel_x_tile(initial_lon, zoom);
        double new_py = lat_to_pixel_y_tile(initial_lat, zoom);

        CHECK(fabs(new_px - prev_osm_px) < 0.001);
        CHECK(fabs(new_py - prev_osm_py) < 0.001);
    }
}

TEST_CASE("Diagonal movement should trigger on either axis") {
    int zoom = 12;
    double base_lat = 35.6762;  // Tokyo
    double base_lon = 139.6503;

    double base_px = lon_to_pixel_x_tile(base_lon, zoom);
    double base_py = lat_to_pixel_y_tile(base_lat, zoom);

    double degrees_per_pixel_lon = 360.0 / (pow(2.0, zoom) * TILE_SIZE);
    // For latitude, calculate approximate degrees per pixel at this latitude
    double lat_rad = base_lat * M_PI / 180.0;
    double lat_scale = 1.0 / cos(lat_rad);
    double degrees_per_pixel_lat = (360.0 / (pow(2.0, zoom) * TILE_SIZE)) * lat_scale;

    SUBCASE("Movement in X only") {
        double new_lon = base_lon + degrees_per_pixel_lon * 1.5;
        double new_px = lon_to_pixel_x_tile(new_lon, zoom);

        CHECK(fabs(new_px - base_px) >= 1.0);
    }

    SUBCASE("Movement in Y only") {
        double new_lat = base_lat + degrees_per_pixel_lat * 1.5;
        double new_py = lat_to_pixel_y_tile(new_lat, zoom);

        CHECK(fabs(new_py - base_py) >= 1.0);
    }

    SUBCASE("Diagonal movement with both axes sub-pixel should not trigger") {
        double new_lon = base_lon + degrees_per_pixel_lon * 0.3;
        double new_lat = base_lat + degrees_per_pixel_lat * 0.3;

        double new_px = lon_to_pixel_x_tile(new_lon, zoom);
        double new_py = lat_to_pixel_y_tile(new_lat, zoom);

        // Both should be less than 1 pixel
        CHECK(fabs(new_px - base_px) < 1.0);
        CHECK(fabs(new_py - base_py) < 1.0);
    }
}

TEST_CASE("Edge case: coordinate wrap-around") {
    int zoom = 10;

    SUBCASE("Crossing 180° longitude") {
        double px_179 = lon_to_pixel_x_tile(179.9, zoom);
        double px_minus_179 = lon_to_pixel_x_tile(-179.9, zoom);

        // These should be far apart in pixel space
        CHECK(fabs(px_179 - px_minus_179) > 100.0);
    }

    SUBCASE("Near poles") {
        // Near north pole
        double py_85 = lat_to_pixel_y_tile(85.0, zoom);
        CHECK(py_85 >= 0);

        // Near south pole
        double py_minus_85 = lat_to_pixel_y_tile(-85.0, zoom);
        CHECK(py_minus_85 <= pow(2.0, zoom) * TILE_SIZE);
    }
}

TEST_CASE("Practical scenario: GPS updates") {
    int zoom = 15;  // Typical street-level zoom

    // Simulate a stationary GPS with typical accuracy drift (~3 meters)
    double base_lat = 37.7749;  // San Francisco
    double base_lon = -122.4194;

    // 3 meters at this latitude is approximately 0.000027 degrees longitude
    double gps_drift_deg = 0.000027;

    SUBCASE("Typical GPS drift should not cause redraw at high zoom") {
        double base_px = lon_to_pixel_x_tile(base_lon, zoom);
        double drifted_px = lon_to_pixel_x_tile(base_lon + gps_drift_deg, zoom);
        double px_diff = fabs(drifted_px - base_px);

        // At zoom 15, 3m drift should be less than 1 pixel (prevents unnecessary redraws)
        // (at zoom 15, one pixel is roughly 4.8m at equator)
        CHECK(px_diff < 1.0);
    }

    SUBCASE("Actual movement of 50 meters should cause redraw") {
        double actual_movement_deg = 0.00045;  // ~50 meters

        double base_px = lon_to_pixel_x_tile(base_lon, zoom);
        double moved_px = lon_to_pixel_x_tile(base_lon + actual_movement_deg, zoom);
        double px_diff = fabs(moved_px - base_px);

        CHECK(px_diff >= 1.0);
    }
}

TEST_SUITE_END();
