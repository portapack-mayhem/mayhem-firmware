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
Decoding of all weather sensor protocols: turns the WeatherDataMessage fields from the baseband into WeatherReadings.
- most protocols send their bits in decode_data (max 64 bits)
- Fine Offset / Ecowitt send the raw frame in payload (up to 32 bytes)
No UI dependencies here, so it can be tested on a PC.
Fine Offset field layouts are from rtl_433 ( https://github.com/merbanan/rtl_433 src/devices/fineoffset*.c, ambientweather_wh31e.c, ecowitt.c ).
*/

#ifndef __WEATHER_READINGS_H__
#define __WEATHER_READINGS_H__

#include <stdint.h>
#include "../baseband/fprotos/weathertypes.hpp"
#include "../baseband/fprotos/fprotogeneral.hpp"

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
    uint16_t strikes = WS_NO_U16;    // lightning strike count, 0..255
    uint8_t leak = WS_NO_U8;         // 1 = leak alarm
};

namespace weather_decode {

inline uint16_t bcd_decode_short(uint32_t data) {
    return (data & 0xF) * 10 + ((data >> 4) & 0xF);
}

// the protocols that send their data in decode_data
inline void decode_data_64(uint8_t type, uint64_t data, WeatherReadings& r) {
    static constexpr uint8_t LACROSSE_TX_MSG_TYPE_TEMP = 0x00;
    static constexpr uint8_t LACROSSE_TX_MSG_TYPE_HUM = 0x0E;
    int16_t i16 = 0;
    int32_t i32 = 0;
    uint16_t u16 = 0;
    uint64_t u64 = 0;
    uint8_t u8 = 0;
    uint8_t channel_3021[] = {3, 0, 2, 1};
    float flo = 0.0;
    switch (type) {
        case FPW_NexusTH:
            r.id = (data >> 28) & 0xFF;
            r.battery_low = !((data >> 27) & 1);
            r.channel = ((data >> 24) & 0x03) + 1;
            if (!((data >> 23) & 1)) {
                r.temp = (float)((data >> 12) & 0x07FF) / 10.0f;
            } else {
                r.temp = (float)((~(data >> 12) & 0x07FF) + 1) / -10.0f;
            }
            r.humidity = data & 0xFF;
            if (r.humidity > 95)
                r.humidity = 95;
            else if (r.humidity < 20)
                r.humidity = 20;
            break;
        case FPW_Acurite592TXR:

            u8 = ((data >> 54) & 0x03);
            r.channel = channel_3021[u8];
            r.id = (data >> 40) & 0x3FFF;
            r.battery_low = !((data >> 38) & 1);
            r.humidity = (data >> 24) & 0x7F;
            u16 = ((data >> 9) & 0xF80) | ((data >> 8) & 0x7F);
            r.temp = ((float)(u16)-1000) / 10.0f;
            break;
        case FPW_Acurite606TX:
            r.id = (data >> 24) & 0xFF;
            r.battery_low = (data >> 23) & 1;
            if (!((data >> 19) & 1)) {
                r.temp = (float)((data >> 8) & 0x07FF) / 10.0f;
            } else {
                r.temp = (float)((~(data >> 8) & 0x07FF) + 1) / -10.0f;
            }
            break;
        case FPW_Acurite609TX:
            r.id = (data >> 32) & 0xFF;
            r.battery_low = (data >> 31) & 1;
            r.channel = WS_NO_CHANNEL;
            // Temperature in Celsius is encoded as a 12 bit integer value
            // multiplied by 10 using the 4th - 6th nybbles (bytes 1 & 2)
            // negative values are recovered by sign extend from int16_t.
            i16 = (int16_t)(((data >> 12) & 0xf000) | ((data >> 16) << 4));
            r.temp = (i16 >> 4) * 0.1f;
            r.humidity = (data >> 8) & 0xff;
            break;
        case FPW_Ambient:
            r.id = (data >> 32) & 0xFF;
            r.battery_low = (data >> 31) & 1;
            r.channel = ((data >> 28) & 0x07) + 1;
            r.temp = FProtoGeneral::locale_fahrenheit_to_celsius(((float)((data >> 16) & 0x0FFF) - 400.0f) / 10.0f);
            r.humidity = (data >> 8) & 0xFF;
            break;
        case FPW_AuriolAhfl:
            r.id = data >> 34;
            r.battery_low = (data >> 33) & 1;
            // btn = (data >> 32) & 1;
            r.channel = ((data >> 30) & 0x3) + 1;
            if (!((data >> 29) & 1)) {
                r.temp = (float)((data >> 18) & 0x07FF) / 10.0f;
            } else {
                r.temp = (float)((~(data >> 18) & 0x07FF) + 1) / -10.0f;
            }
            r.humidity = (data >> 11) & 0x7F;
            break;
        case FPW_AuriolTH:
            r.id = (data >> 31) & 0xFF;
            r.battery_low = ((data >> 30) & 1);
            r.channel = ((data >> 25) & 0x03) + 1;
            if (!((data >> 23) & 1)) {
                r.temp = (float)((data >> 13) & 0x07FF) / 10.0f;
            } else {
                r.temp = (float)((~(data >> 13) & 0x07FF) + 1) / -10.0f;
            }
            r.humidity = (data >> 1) & 0x7F;
            break;
        case FPW_GTWT02:
            r.id = (data >> 29) & 0xFF;
            r.battery_low = (data >> 28) & 1;
            // btn = (data >> 27) & 1;
            r.channel = ((data >> 25) & 0x3) + 1;

            if (!((data >> 24) & 1)) {
                r.temp = (float)((data >> 13) & 0x07FF) / 10.0f;
            } else {
                r.temp = (float)((~(data >> 13) & 0x07FF) + 1) / -10.0f;
            }
            r.humidity = (data >> 6) & 0x7F;
            if (r.humidity <= 10)  // actually the sensors sends 10 below working range of 20%
                r.humidity = 0;
            else if (r.humidity > 90)  // actually the sensors sends 110 above working range of 90%
                r.humidity = 100;
            break;
        case FPW_GTWT03:
            r.id = data >> 33;
            r.humidity = (data >> 25) & 0xFF;
            if (r.humidity <= 10) {  // actually the sensors sends 10 below working range of 20%
                r.humidity = 0;
            } else if (r.humidity > 95) {  // actually the sensors sends 110 above working range of 90%
                r.humidity = 100;
            }
            r.battery_low = (data >> 24) & 1;
            // (data >> 23) & 1;
            r.channel = ((data >> 21) & 0x03) + 1;
            if (!((data >> 20) & 1)) {
                r.temp = (float)((data >> 9) & 0x07FF) / 10.0f;
            } else {
                r.temp = (float)((~(data >> 9) & 0x07FF) + 1) / -10.0f;
            }
            break;
        case FPW_INFACTORY:
            r.id = data >> 32;
            r.battery_low = (data >> 26) & 1;
            r.temp = FProtoGeneral::locale_fahrenheit_to_celsius(((float)((data >> 12) & 0x0FFF) - 900.0f) / 10.0f);
            r.humidity = (((data >> 8) & 0x0F) * 10) + ((data >> 4) & 0x0F);  // BCD, 'A0'=100%rH
            r.channel = data & 0x03;
            break;
        case FPW_LACROSSETX:
            u8 = (data >> 32) & 0x0F;
            r.id = (((data >> 28) & 0x0F) << 3) | (((data >> 24) & 0x0F) >> 1);
            flo = (float)((data >> 20) & 0x0F) * 10.0f + (float)((data >> 16) & 0x0F) + (float)((data >> 12) & 0x0F) * 0.1f;
            if (u8 == LACROSSE_TX_MSG_TYPE_TEMP) {  //-V1051
                r.temp = flo - 50.0f;
                r.humidity = WS_NO_HUMIDITY;
            } else if (u8 == LACROSSE_TX_MSG_TYPE_HUM) {
                // ToDo for verification, records are needed with sensors maintaining temperature and temperature for this standard
                r.humidity = (uint8_t)flo;
            }
            break;
        case FPW_LACROSSETX141thbv2:
            r.id = data >> 32;
            r.battery_low = (data >> 31) & 1;
            // btn = (data >> 30) & 1;
            r.channel = ((data >> 28) & 0x03) + 1;
            r.temp = ((float)((data >> 16) & 0x0FFF) - 500.0f) / 10.0f;
            r.humidity = (data >> 8) & 0xFF;
            break;
        case FPW_OREGON2:
            i32 = bcd_decode_short(data >> 4);
            i32 *= 10;
            i32 += (data >> 12) & 0xf;
            if (data & 0xF) i32 = -i32;
            r.temp = (float)i32 / 10.0;
            r.battery_low = ((data >> 32) & 0x4) ? 1 : 0;
            u8 = (data >> 44) & 0xF;
            r.channel = 1;
            while (u8 > 1) {
                r.channel++;
                u8 >>= 1;
            }
            break;
        case FPW_OREGON2B:
            r.humidity = bcd_decode_short(data);
            u64 = data >> 8;
            i32 = bcd_decode_short(u64 >> 4);
            i32 *= 10;
            i32 += (u64 >> 12) & 0xf;
            if (u64 & 0xF) i32 = -i32;
            r.temp = (float)i32 / 10.0;
            r.battery_low = ((data >> 32) & 0x4) ? 1 : 0;
            u8 = (data >> 44) & 0xF;
            r.channel = 1;
            while (u8 > 1) {
                r.channel++;
                u8 >>= 1;
            }
            break;
        case FPW_OREGON3:
            // todo check
            r.humidity = ((data >> 4) & 0xF) * 10 + (((data >> 4) >> 4) & 0xF);
            i16 = (((data >> 12) >> 4) & 0xF) * 10 + ((((data >> 12) >> 4) >> 4) & 0xF);  // ws_oregon3_bcd_decode_short((data >> 12) >> 4);
            i16 *= 10;
            i16 += ((data >> 12) >> 12) & 0xF;
            if ((data >> 12) & 0xF) i16 = -i16;
            r.temp = (float)i16 / 10.0;
            break;
        case FPW_OREGONv1:
            u64 = FProtoGeneral::subghz_protocol_blocks_reverse_key(data, 32);
            r.id = u64 & 0xFF;
            r.channel = ((u64 >> 6) & 0x03) + 1;
            r.temp = ((u64 >> 8) & 0x0F) * 0.1f + ((u64 >> 12) & 0x0F) + ((u64 >> 16) & 0x0F) * 10.0f;
            if (!((u64 >> 21) & 1)) {
                r.temp = r.temp;
            } else {
                r.temp = -r.temp;
            }
            r.battery_low = !((u64 >> 23) & 1ULL);
            break;
        case FPW_THERMOPROTX4:
            r.id = (data >> 25) & 0xFF;
            r.battery_low = (data >> 24) & 1;
            // btn = (data >> 23) & 1;
            r.channel = ((data >> 21) & 0x03) + 1;
            if (!((data >> 20) & 1)) {
                r.temp = (float)((data >> 9) & 0x07FF) / 10.0f;
            } else {
                r.temp = (float)((~(data >> 9) & 0x07FF) + 1) / -10.0f;
            }
            r.humidity = (data >> 1) & 0xFF;
            break;
        case FPW_TX_8300:
            r.humidity = (((data >> 28) & 0x0F) * 10) + ((data >> 24) & 0x0F);
            if (!((data >> 22) & 0x03))
                r.battery_low = 0;
            else
                r.battery_low = 1;
            r.channel = (data >> 20) & 0x03;
            r.id = (data >> 12) & 0x7F;
            r.temp = ((data >> 8) & 0x0F) * 10.0f + ((data >> 4) & 0x0F) +
                     (data & 0x0F) * 0.1f;
            if (!((data >> 19) & 1)) {
                r.temp = r.temp;
            } else {
                r.temp = -r.temp;
            }
            break;
        case FPW_WENDOX_W6726:
            r.id = (data >> 24) & 0xFF;
            r.battery_low = (data >> 6) & 1;
            if (((data >> 23) & 1)) {
                r.temp = (float)(((data >> 14) & 0x1FF) + 12) / 10.0f;
            } else {
                r.temp = (float)((~(data >> 14) & 0x1FF) + 1 - 12) / -10.0f;
            }
            if (r.temp < -50.0f) {
                r.temp = -50.0f;
            } else if (r.temp > 70.0f) {
                r.temp = 70.0f;
            }
            break;
        case FPW_Acurite986:
            r.id = FProtoGeneral::subghz_protocol_blocks_reverse_key(data >> 24, 8);
            r.id = (r.id << 8) | FProtoGeneral::subghz_protocol_blocks_reverse_key(data >> 16, 8);
            r.battery_low = (data >> 14) & 1;
            r.channel = ((data >> 15) & 1) + 1;
            i16 = FProtoGeneral::subghz_protocol_blocks_reverse_key(data >> 32, 8);
            if (i16 & 0x80) {
                i16 = -(i16 & 0x7F);
            }
            r.temp = FProtoGeneral::locale_fahrenheit_to_celsius((float)i16);
            break;
        case FPW_KEDSUM:
            r.id = data >> 32;
            if ((data >> 30) & 0x3) {
                r.battery_low = 0;
            } else {
                r.battery_low = 1;
            }
            r.channel = ((data >> 28) & 0x3) + 1;
            u16 = ((data >> 16) & 0x0f) << 8 | ((data >> 20) & 0x0f) << 4 | ((data >> 24) & 0x0f);
            r.temp = FProtoGeneral::locale_fahrenheit_to_celsius(((float)u16 - 900.0f) / 10.0f);
            r.humidity = ((data >> 8) & 0x0f) << 4 | ((data >> 12) & 0x0f);
            break;
        case FPW_Acurite5in1:
            u8 = ((data >> 62) & 0x03);
            r.channel = channel_3021[u8];
            r.id = (data >> 48) & 0x3FFF;
            r.battery_low = !((data >> 46) & 1);
            r.humidity = (data >> 8) & 0x7F;
            u16 = ((data >> (24 - 7)) & 0x780) | ((data >> 16) & 0x7F);
            r.temp = FProtoGeneral::locale_fahrenheit_to_celsius(((float)(u16)-400) / 10.0f);
            break;
        case FPW_EmosE601x:
            r.id = 0xffff;  //(upper_decode_data >> 24) & 0xff; //todo maybe with oregon, and +64bit usage/message
            r.battery_low = (data >> 10) & 1;
            i16 = (data >> 40) & 0xfff;
            /* Handle signed data */
            if (i16 & 0x800) {
                i16 |= 0xf000;
            }
            r.temp = (float)i16 / 10.0;
            r.humidity = (data >> 32) & 0xff;
            r.channel = (data >> 52) & 0x03;
            break;
        case FPW_SolightTE44:
            r.id = (data >> 28) & 0xff;
            r.battery_low = !(data >> 27) & 0x01;
            r.channel = ((data >> 24) & 0x03) + 1;
            i16 = (data >> 12) & 0x0fff;
            /* Handle signed data */
            if (i16 & 0x0800) {
                i16 |= 0xf000;
            }
            r.temp = (float)i16 / 10.0;
            break;
        case FPW_Bresser3CH:
            r.id = (data >> 28) & 0xff;
            r.channel = ((data >> 27) & 0x01) | (((data >> 26) & 0x01) << 1);
            // r.btn = ((data >> 25) & 0x1);
            r.battery_low = ((data >> 24) & 0x1);
            i16 = (data >> 12) & 0x0fff;
            /* Handle signed data */
            if (i16 & 0x0800) {
                i16 |= 0xf000;
            }
            r.temp = (float)i16 / 10.0;
            r.humidity = data & 0xff;
            break;
        case FPW_Bresser3CH_V1:
            r.id = (data >> 32) & 0xff;
            r.battery_low = ((data >> 31) & 0x1);
            // r.btn = (data >> 30) & 0x1;
            r.channel = (data >> 28) & 0x3;
            r.temp = (data >> 16) & 0xfff;
            r.temp = FProtoGeneral::locale_fahrenheit_to_celsius((float)(r.temp - 900) / 10.0);
            r.humidity = (data >> 8) & 0xff;
            break;

        case FPW_Vauno_EN8822:
            r.id = (data >> 34) & 0xff;
            r.battery_low = (data >> 33) & 0x01;
            r.channel = ((data >> 30) & 0x03);
            i16 = (data >> 18) & 0x0fff;
            /* Handle signed data */
            if (i16 & 0x0800) {
                i16 |= 0xf000;
            }
            r.temp = (float)i16 / 10.0;
            r.humidity = (data >> 11) & 0x7f;

            break;
        default:
            break;
    }
}

// Fine Offset / Ecowitt: they send the raw frame

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
inline void decode_fineoffset(uint8_t type, const uint8_t* p, uint8_t len, WeatherReadings& r) {
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
            r.id = (p[2] << 8) | p[3];
            r.channel = (p[1] >> 4) + 1;
            r.battery_low = p[4] <= 1;  // 20% steps
            r.leak = (p[7] >> 6) & 1;
            break;
        default:
            break;
    }
}

}  // namespace weather_decode

// decodes the data of any weather protocol, as sent by the baseband in WeatherDataMessage
inline void weather_decode_all(uint8_t type, uint64_t decode_data, const uint8_t* payload, uint8_t payload_len, WeatherReadings& r) {
    r.sensorType = type;
    if (type >= FPW_FO_WH2)
        weather_decode::decode_fineoffset(type, payload, payload_len, r);
    else
        weather_decode::decode_data_64(type, decode_data, r);
}

#endif /*__WEATHER_READINGS_H__*/
