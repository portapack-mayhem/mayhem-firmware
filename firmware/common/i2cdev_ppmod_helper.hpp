#ifndef I2CDEV_PPMOD_HELPER_H
#define I2CDEV_PPMOD_HELPER_H

#include <cstdint>

enum class SupportedFeatures : uint64_t {
    FEAT_NONE = 0,
    FEAT_EXT_APP = 1 << 0,
    FEAT_UART = 1 << 1,
    FEAT_GPS = 1 << 2,
    FEAT_ORIENTATION = 1 << 3,
    FEAT_ENVIRONMENT = 1 << 4,
    FEAT_LIGHT = 1 << 5,
    FEAT_DISPLAY = 1 << 6
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