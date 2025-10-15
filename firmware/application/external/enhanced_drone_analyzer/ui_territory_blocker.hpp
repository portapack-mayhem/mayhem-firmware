// ui_territory_blocker.hpp
// Territory blocking system for Enhanced Drone Analyzer
// Checks GPS coordinates, IP addresses, and hardware serial numbers

#ifndef __UI_TERRITORY_BLOCKER_HPP__
#define __UI_TERRITORY_BLOCKER_HPP__

#include <cstdint>
#include <cstring>
#include <string>

namespace ui::external_app::enhanced_drone_analyzer {

struct TerritoryCoordinates {
    double latitude;
    double longitude;
    const char* country_code;
};

struct TerritoryRange {
    double min_lat;
    double max_lat;
    double min_lon;
    double max_lon;
    const char* country_code;
};

// Ukraine geographical boundaries (approximate)
static constexpr TerritoryRange UKRAINE_BOUNDS[] = {
    {44.0, 52.5, 22.0, 40.0, "UA"}  // Main Ukraine region
};

// EU member states geographical boundaries (approximate)
static constexpr TerritoryRange EU_COUNTRIES[] = {
    // Austria
    {46.4, 49.0, 9.5, 17.0, "AT"},
    // Belgium
    {49.5, 51.5, 2.5, 6.5, "BE"},
    // Bulgaria
    {41.0, 44.0, 22.5, 29.0, "BG"},
    // Croatia
    {42.5, 46.5, 13.0, 19.5, "HR"},
    // Cyprus
    {34.5, 35.5, 32.0, 34.5, "CY"},
    // Czech Republic
    {48.5, 51.0, 12.0, 18.5, "CZ"},
    // Denmark
    {54.5, 57.5, 8.0, 15.0, "DK"},
    // Estonia
    {57.5, 59.5, 21.5, 28.5, "EE"},
    // Finland
    {59.5, 70.0, 19.0, 31.5, "FI"},
    // France
    {41.0, 51.5, -5.5, 9.5, "FR"},
    // Germany
    {47.0, 55.0, 5.0, 15.0, "DE"},
    // Greece
    {34.5, 41.5, 19.0, 29.0, "GR"},
    // Hungary
    {45.5, 48.5, 16.0, 23.0, "HU"},
    // Ireland
    {51.5, 55.5, -11.0, -5.5, "IE"},
    // Italy
    {36.5, 47.0, 6.5, 18.5, "IT"},
    // Latvia
    {55.5, 58.0, 20.5, 28.5, "LV"},
    // Lithuania
    {53.5, 56.5, 20.5, 26.5, "LT"},
    // Luxembourg
    {49.5, 50.5, 5.5, 6.5, "LU"},
    // Malta
    {35.5, 36.5, 14.0, 14.5, "MT"},
    // Netherlands
    {50.5, 53.5, 3.0, 7.5, "NL"},
    // Poland
    {49.0, 55.0, 14.0, 24.5, "PL"},
    // Portugal
    {36.5, 42.5, -9.5, -6.0, "PT"},
    // Romania
    {43.0, 48.5, 20.0, 30.0, "RO"},
    // Slovakia
    {47.5, 49.5, 16.5, 22.5, "SK"},
    // Slovenia
    {45.0, 46.5, 13.0, 16.5, "SI"},
    // Spain
    {35.5, 44.0, -9.5, 4.5, "ES"},
    // Sweden
    {55.0, 69.0, 11.0, 25.0, "SE"}
};

class TerritoryBlocker {
public:
    TerritoryBlocker();
    ~TerritoryBlocker() = default;

    // GPS location blocking
    bool is_location_blocked(double latitude, double longitude) const;

    // IP address blocking (simplified check)
    bool is_ip_blocked(uint32_t ip_address) const;

    // Hardware serial blocking
    bool is_hardware_blocked(const char* hardware_serial) const;

    // Combined territory check
    bool is_territory_restricted() const;

    // Country-specific checks
    bool is_in_ukraine(double latitude, double longitude) const;
    bool is_in_eu(double latitude, double longitude) const;

    // Error message for blocked territories
    const char* get_block_reason() const;

private:
    mutable std::string last_block_reason_;

    // IP ranges for known EU/Ukraine ISPs (simplified)
    static constexpr uint32_t EU_IP_RANGES[][2] = {
        {0x0A000000, 0x0AFFFFFF}, // 10.0.0.0/8 - Private EU ranges
        {0xC0000000, 0xC0FFFFFF}, // 192.168.0.0/16 - Various ranges
        {0x64400000, 0x647FFFFF}  // 100.64.0.0/10 - CGNAT EU
    };

    static constexpr uint32_t UA_IP_RANGES[][2] = {
        {0x2B000000, 0x2BFFFFFF}, // 43.0.0.0/8 - Ukrainian ranges
        {0x5D000000, 0x5DFFFFFF}, // 93.0.0.0/8 - Ukrainian ISP
        {0xB2000000, 0xB2FFFFFF}  // 178.0.0.0/8 - Ukrainian ranges
    };

    bool check_coordinate_bounds(double lat, double lon,
                               double min_lat, double max_lat,
                               double min_lon, double max_lon) const;

    bool check_ip_range(uint32_t ip, const uint32_t ranges[][2], size_t count) const;
};

extern TerritoryBlocker global_territory_blocker;

} // namespace ui::external_app::enhanced_drone_analyzer

#endif // __UI_TERRITORY_BLOCKER_HPP__
