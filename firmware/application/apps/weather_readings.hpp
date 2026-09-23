/*
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

/*
Decoded values of a weather sensor, and the parser of the Fine Offset / Ecowitt frames.
No UI dependencies here, so it can be tested on a PC.
Field layouts are from rtl_433 ( https://github.com/merbanan/rtl_433 src/devices/fineoffset*.c, ambientweather_wh31e.c, ecowitt.c ).
*/

#ifndef __WEATHER_READINGS_H__
#define __WEATHER_READINGS_H__

#include <stdint.h>
#include "../baseband/fprotos/weathertypes.hpp"

struct WeatherReadings {
    uint8_t sensorType = FPW_Invalid;
    uint32_t id = WS_NO_ID;
    float temp = WS_NO_TEMPERATURE;
    uint8_t humidity = WS_NO_HUMIDITY;
    uint8_t battery_low = WS_NO_BATT;
    uint8_t channel = WS_NO_CHANNEL;
    // only some sensors have these. WS_NO_Uxx if not present.
    uint16_t wind_avg = WS_NO_U16;   // 0.1 m/s
    uint16_t wind_gust = WS_NO_U16;  // 0.1 m/s
    uint16_t wind_dir = WS_NO_U16;   // degrees
    uint16_t pressure = WS_NO_U16;   // 0.1 hPa
    uint16_t pm25 = WS_NO_U16;       // 0.1 ug/m3
    uint32_t rain = WS_NO_U32;       // 0.1 mm, total since the sensor started
    uint32_t light = WS_NO_U32;      // lux
    uint8_t uv = WS_NO_U8;           // 0.1 UV index
    uint8_t moisture = WS_NO_U8;     // %
    uint8_t storm_dist = WS_NO_U8;   // km
    uint8_t strikes = WS_NO_U8;      // lightning strike count
};

namespace weather_fineoffset {

// 10 bit, offset 40C, scale 10. 0x3ff is invalid.
inline float temp_offset40(uint16_t raw) {
    return (raw == 0x3FF || raw == 0x7FF) ? WS_NO_TEMPERATURE : ((int16_t)raw - 400) / 10.0f;
}

inline uint16_t invalid_to_none(uint16_t v, uint16_t invalid) {
    return v == invalid ? WS_NO_U16 : v;
}

// WS80, WS90: shared first 14 bytes
inline void parse_ws80(const uint8_t* p, WeatherReadings& r) {
    r.id = (p[1] << 16) | (p[2] << 8) | p[3];
    uint16_t light_raw = (p[4] << 8) | p[5];
    if (light_raw != 0xFFFF) r.light = light_raw * 10;
    r.battery_low = p[6] * 20 < 1800;  // mV, 3.0V .. 1.4V
    r.temp = temp_offset40(((p[7] & 0x03) << 8) | p[8]);
    r.humidity = p[9];
    r.wind_avg = invalid_to_none(((p[7] & 0x10) << 4) | p[10], 0x1FF);
    r.wind_dir = invalid_to_none(((p[7] & 0x20) << 3) | p[11], 0x1FF);
    r.wind_gust = invalid_to_none(((p[7] & 0x40) << 2) | p[12], 0x1FF);
    r.uv = p[13];
}

// payload: the frame from the family code, or after the preamble for the OOK ones
inline void parse(uint8_t type, const uint8_t* p, uint8_t len, WeatherReadings& r) {
    static const uint8_t payload_len[] = {
        5, 5, 8, 10, 7, 6,          // WH2 .. WH53
        17, 17, 25, 8, 7, 8, 7, 9,  // WH24 .. WH40
        10, 16, 14, 18, 28, 32,     // WN20 .. WS90
        9, 9, 9};                   // WN34 .. WH55
    static const uint16_t wh1080_dir[] = {0, 23, 45, 68, 90, 113, 135, 158, 180, 203, 225, 248, 270, 293, 315, 338};
    static const uint16_t uvi_upper[] = {432, 851, 1210, 1570, 2017, 2450, 2761, 3100, 3512, 3918, 4277, 4650, 5029};

    r.sensorType = type;
    if (type < FPW_FO_WH2 || type >= FPW_COUNT || len < payload_len[type - FPW_FO_WH2]) return;

    uint16_t u16;
    uint32_t u32;
    switch (type) {
        case FPW_FO_WH2:  // also WH2A, Telldus
            r.id = ((p[0] & 0x0F) << 4) | (p[1] >> 4);
            u16 = ((p[1] & 0x0F) << 8) | p[2];
            r.temp = (u16 & 0x800) ? (u16 & 0x7FF) / -10.0f : u16 / 10.0f;  // signed magnitude
            r.humidity = p[3];
            break;
        case FPW_FO_WH5:  // also TFA 30.3225
            r.id = ((p[0] & 0x0F) << 4) | (p[1] >> 4);
            u16 = ((p[1] & 0x0F) << 8) | p[2];
            r.humidity = p[3];
            if (p[3] == 0xFF) {  // TFA: no humidity, top bit is battery low
                r.battery_low = (u16 & 0x800) ? 1 : 0;
                u16 &= 0x7FF;
            }
            r.temp = ((int16_t)u16 - 400) / 10.0f;
            break;
        case FPW_FO_WH0530:
            r.id = ((p[0] & 0x0F) << 4) | (p[1] >> 4);
            r.battery_low = (p[1] >> 3) & 1;
            r.temp = temp_offset40(((p[1] & 0x07) << 8) | p[2]);
            r.rain = ((p[4] << 8) | p[3]) * 3;  // 0.3 mm / tip
            break;
        case FPW_FO_WH1080:
            r.id = ((p[0] << 4) & 0xF0) | (p[1] >> 4);
            r.temp = temp_offset40(((p[1] & 0x03) << 8) | p[2]);
            r.humidity = p[3];
            r.wind_avg = p[4] * 34 / 10;  // 0.34 m/s / step
            r.wind_gust = p[5] * 34 / 10;
            r.rain = (((p[6] & 0x0F) << 8) | p[7]) * 3;  // 0.3 mm / tip
            r.wind_dir = wh1080_dir[p[8] & 0x0F];
            r.battery_low = (p[8] >> 4) == 1;
            break;
        case FPW_FO_WH3080UV:
            r.id = ((p[0] << 4) & 0xF0) | (p[1] >> 4);
            r.uv = (p[1] & 0x0F) * 10;
            r.light = ((p[3] << 16) | (p[4] << 8) | p[5]) / 10;
            break;
        case FPW_FO_WH53:
            r.id = p[1];
            r.channel = (p[2] >> 4) + 1;
            r.temp = temp_offset40(((p[2] & 0x03) << 8) | p[3]);
            break;
        case FPW_FO_WH24:
        case FPW_FO_WH65:
        case FPW_FO_WS69: {
            bool wh24 = type == FPW_FO_WH24;  // wind 1.12 m/s and rain 0.3 mm / step, others are 0.51 m/s and 0.254 mm
            r.id = p[1];
            r.wind_dir = invalid_to_none(p[2] | ((p[3] & 0x80) << 1), 0x1FF);
            r.battery_low = (p[3] & 0x08) >> 3;
            r.temp = temp_offset40(((p[3] & 0x07) << 8) | p[4]);
            r.humidity = p[5];
            u16 = p[6] | ((p[3] & 0x10) << 4);  // scaled by 8
            if (u16 != 0x1FF) r.wind_avg = u16 * (wh24 ? 112 : 51) / 80;
            if (p[7] != 0xFF) r.wind_gust = p[7] * (wh24 ? 112 : 51) / 10;
            u32 = (p[8] << 8) | p[9];
            r.rain = wh24 ? u32 * 3 : u32 * 254 / 100;
            u16 = (p[10] << 8) | p[11];
            if (u16 != 0xFFFF) {
                uint8_t uvi = 0;
                while (uvi < 13 && uvi_upper[uvi] < u16) uvi++;
                r.uv = uvi * 10;
            }
            u32 = (p[12] << 16) | (p[13] << 8) | p[14];
            if (u32 != 0xFFFFFF) r.light = u32 / 10;
            if (type == FPW_FO_WS69) {  // the baseband checked the crc of the extra bytes
                u32 = ((p[17] & 0x01) << 16) | (p[18] << 8) | p[19];
                if (u32 < 0x1FFFF) r.pressure = u32 / 10;  // 0.01 hPa
            }
            break;
        }
        case FPW_FO_WH25:
        case FPW_FO_WH32:
            r.id = ((p[0] & 0x0F) << 4) | (p[1] >> 4);
            r.battery_low = (p[1] & 0x08) >> 3;
            r.temp = temp_offset40(((p[1] & 0x03) << 8) | p[2]);
            r.humidity = p[3];
            if ((p[0] & 0xF0) == 0xE0) r.pressure = invalid_to_none((p[4] << 8) | p[5], 0xFFFF);
            break;
        case FPW_FO_WH0290:
            r.id = p[1];
            r.pm25 = ((p[2] & 0x3F) << 8) | p[3];
            r.battery_low = (((p[2] & 0x40) >> 4) | ((p[4] & 0xC0) >> 6)) <= 1;  // bars out of 5
            break;
        case FPW_FO_WH31E:
            r.id = p[1];
            r.battery_low = (p[2] & 0x04) >> 2;
            r.channel = ((p[2] & 0x70) >> 4) + 1;
            r.temp = temp_offset40(((p[2] & 0x03) << 8) | p[3]);
            r.humidity = p[4];
            break;
        case FPW_FO_WH40:
            r.id = ((p[1] & 0x0F) << 16) | (p[2] << 8) | p[3];
            if (p[4] & 0x1F) r.battery_low = (p[4] & 0x1F) <= 10;  // 0.1V, 1.5 .. 0.9V
            r.rain = (p[5] << 8) | p[6];
            break;
        case FPW_FO_WN20:
            r.id = (p[2] << 8) | p[3];
            r.battery_low = p[4] <= 90;  // 20 mV, 3.0 .. 1.8V
            r.rain = (p[5] << 8) | p[6];
            break;
        case FPW_FO_WS68:
            r.id = (p[2] << 8) | p[3];
            r.light = ((p[4] << 8) | p[5]) * 10;
            r.battery_low = p[6] <= 0x20;
            r.wind_avg = ((p[7] & 0x10) << 4) | p[10];
            r.wind_dir = ((p[7] & 0x20) << 3) | p[11];
            r.wind_gust = ((p[7] & 0x40) << 2) | p[12];
            r.uv = p[13] / 10 * 10;
            break;
        case FPW_FO_WH51:
            r.id = (p[1] << 16) | (p[2] << 8) | p[3];
            r.battery_low = (p[4] & 0x1F) < 13;  // 0.1V
            r.moisture = p[6];
            break;
        case FPW_FO_WS80:
            parse_ws80(p, r);
            break;
        case FPW_FO_WS90:
            parse_ws80(p, r);
            u16 = (p[14] << 8) | p[15];
            if (u16 != 0x3FFF) r.pressure = u16 * 10;
            r.rain = (p[19] << 8) | p[20];
            break;
        case FPW_FO_WS85:
            r.id = (p[1] << 16) | (p[2] << 8) | p[3];
            r.battery_low = p[4] * 20 <= 2400;  // mV
            r.wind_avg = invalid_to_none(((p[5] & 0x10) << 4) | p[7], 0x1FF);
            r.wind_dir = invalid_to_none(((p[5] & 0x20) << 3) | p[8], 0x1FF);
            r.wind_gust = invalid_to_none(((p[5] & 0x40) << 2) | p[9], 0x1FF);
            r.rain = (p[15] << 8) | p[16];
            break;
        case FPW_FO_WN34: {  // also WN38
            r.id = (p[1] << 16) | (p[2] << 8) | p[3];
            int16_t t = (int16_t)(((p[4] & 0x0F) << 12) | (p[5] << 4)) >> 4;
            r.temp = ((p[4] >> 4) == 4) ? t / 10.0f : t / 10.0f - 40.0f;  // WN34D has no offset
            r.battery_low = (p[6] & 0x7F) * 20 <= 1200;                   // mV
            break;
        }
        case FPW_FO_WH57:
            r.id = ((p[1] & 0x0F) << 16) | (p[2] << 8) | p[3];
            r.battery_low = ((p[4] & 0x06) >> 1) == 0;
            if ((p[5] & 0x3F) != 63) r.storm_dist = p[5] & 0x3F;
            r.strikes = p[6];
            break;
        case FPW_FO_WH55:
            r.id = ((p[1] & 0x0F) << 16) | (p[2] << 8) | p[3];
            r.channel = (p[1] >> 4) + 1;
            r.battery_low = p[4] <= 1;  // 20% steps
            break;
        default:
            break;
    }
}

}  // namespace weather_fineoffset

#endif /*__WEATHER_READINGS_H__*/
