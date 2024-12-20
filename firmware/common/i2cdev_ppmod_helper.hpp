#ifndef I2CDEV_PPMOD_HELPER_H
#define I2CDEV_PPMOD_HELPER_H

#include <cstdint>

enum class SupportedFeatures : uint64_t {
    FEAT_NONE = 0,              // no polling needed
    FEAT_EXT_APP = 1 << 0,      // supplies external app
    FEAT_UART = 1 << 1,         // supplies UART communication
    FEAT_GPS = 1 << 2,          // can be queried for GPS info
    FEAT_ORIENTATION = 1 << 3,  // can be queried for orientation info
    FEAT_ENVIRONMENT = 1 << 4,  // can be queried for environment info
    FEAT_LIGHT = 1 << 5,        // can be queried for light info
    FEAT_DISPLAY = 1 << 6,      // can handle special display output
    FEAT_SHELL = 1 << 7,        // can handle shell commands (polling needed for that)
};

typedef struct
{
    uint8_t hour;      /*!< Hour */
    uint8_t minute;    /*!< Minute */
    uint8_t second;    /*!< Second */
    uint16_t thousand; /*!< Thousand */
} gps_time_t;

typedef struct
{
    uint8_t day;   /*!< Day (start from 1) */
    uint8_t month; /*!< Month (start from 1) */
    uint16_t year; /*!< Year (start from 2000) */
} gps_date_t;

typedef struct
{
    float latitude;       /*!< Latitude (degrees) */
    float longitude;      /*!< Longitude (degrees) */
    float altitude;       /*!< Altitude (meters) */
    uint8_t sats_in_use;  /*!< Number of satellites in use */
    uint8_t sats_in_view; /*!< Number of satellites in view */
    float speed;          /*!< Ground speed, unit: m/s */
    gps_date_t date;      /*!< Fix date */
    gps_time_t tim;       /*!< time in UTC */
} gpssmall_t;

typedef struct
{
    float angle;
    float tilt;
} orientation_t;

typedef struct
{
    float temperature;
    float humidity;
    float pressure;
} environment_t;

// light is uint16_t

#endif