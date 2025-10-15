// ui_territory_blocker.cpp
// Implementation of territory blocking for Enhanced Drone Analyzer

#include "ui_territory_blocker.hpp"
#include <algorithm>

namespace ui::external_app::enhanced_drone_analyzer {

TerritoryBlocker global_territory_blocker;

TerritoryBlocker::TerritoryBlocker()
    : last_block_reason_("Unknown") {
}

bool TerritoryBlocker::is_location_blocked(double latitude, double longitude) const {
    // Check Ukraine first
    if (is_in_ukraine(latitude, longitude)) {
        last_block_reason_ = "ACCESS DENIED: Ukraine territory detected. Drone analysis prohibited.";
        return true;
    }

    // Check EU countries
    if (is_in_eu(latitude, longitude)) {
        last_block_reason_ = "ACCESS DENIED: EU member state territory detected. Drone analysis prohibited.";
        return true;
    }

    last_block_reason_ = "Access granted.";
    return false;
}

bool TerritoryBlocker::is_ip_blocked(uint32_t ip_address) const {
    // Check Ukrainian IP ranges
    if (check_ip_range(ip_address, UA_IP_RANGES,
                      sizeof(UA_IP_RANGES) / sizeof(UA_IP_RANGES[0]))) {
        last_block_reason_ = "ACCESS DENIED: Ukrainian IP address detected.";
        return true;
    }

    // Check EU IP ranges
    if (check_ip_range(ip_address, EU_IP_RANGES,
                      sizeof(EU_IP_RANGES) / sizeof(EU_IP_RANGES[0]))) {
        last_block_reason_ = "ACCESS DENIED: EU IP address detected.";
        return true;
    }

    return false;
}

bool TerritoryBlocker::is_hardware_blocked(const char* hardware_serial) const {
    if (!hardware_serial) return false;

    // Check for Ukrainian/EU registered hardware patterns
    // UA-XXXX, EU-XXXX patterns in serial numbers
    const char* ua_patterns[] = {"UA-", "UKR-", "UKRAINE-"};
    const char* eu_patterns[] = {"EU-", "EUROPE-", "EC-"};

    size_t serial_len = strlen(hardware_serial);

    // Check Ukrainian patterns
    for (const char* pattern : ua_patterns) {
        if (strncasecmp(hardware_serial, pattern, strlen(pattern)) == 0) {
            last_block_reason_ = "ACCESS DENIED: Hardware registered in Ukraine.";
            return true;
        }
    }

    // Check EU patterns
    for (const char* pattern : eu_patterns) {
        if (strncasecmp(hardware_serial, pattern, strlen(pattern)) == 0) {
            last_block_reason_ = "ACCESS DENIED: Hardware registered in EU.";
            return true;
        }
    }

    return false;
}

bool TerritoryBlocker::is_territory_restricted() const {
    // This is called during app initialization - simplified check
    // In real implementation, would get current GPS/IP from system
    return false; // Default to allowed for compilation
}

bool TerritoryBlocker::is_in_ukraine(double latitude, double longitude) const {
    return std::any_of(std::begin(UKRAINE_BOUNDS), std::end(UKRAINE_BOUNDS),
        [this, latitude, longitude](const TerritoryRange& range) {
            return check_coordinate_bounds(latitude, longitude,
                                         range.min_lat, range.max_lat,
                                         range.min_lon, range.max_lon);
        });
}

bool TerritoryBlocker::is_in_eu(double latitude, double longitude) const {
    return std::any_of(std::begin(EU_COUNTRIES), std::end(EU_COUNTRIES),
        [this, latitude, longitude](const TerritoryRange& range) {
            return check_coordinate_bounds(latitude, longitude,
                                         range.min_lat, range.max_lat,
                                         range.min_lon, range.max_lon);
        });
}

const char* TerritoryBlocker::get_block_reason() const {
    return last_block_reason_.c_str();
}

bool TerritoryBlocker::check_coordinate_bounds(double lat, double lon,
                                              double min_lat, double max_lat,
                                              double min_lon, double max_lon) const {
    return (lat >= min_lat && lat <= max_lat &&
            lon >= min_lon && lon <= max_lon);
}

bool TerritoryBlocker::check_ip_range(uint32_t ip,
                                     const uint32_t ranges[][2],
                                     size_t count) const {
    for (size_t i = 0; i < count; ++i) {
        if (ip >= ranges[i][0] && ip <= ranges[i][1]) {
            return true;
        }
    }
    return false;
}

} // namespace ui::external_app::enhanced_drone_analyzer
