
#ifndef __FPROTO_Ambient_H__
#define __FPROTO_Ambient_H__

#include "weatherbase.hpp"

#define AMBIENT_WEATHER_PACKET_HEADER_1 0xFFD440000000000  // 0xffd45 .. 0xffd46
#define AMBIENT_WEATHER_PACKET_HEADER_2 0x001440000000000  // 0x00145 .. 0x00146
#define AMBIENT_WEATHER_PACKET_HEADER_MASK 0xFFFFC0000000000

class FProtoWeatherAmbient : public FProtoWeatherBase {
   public:
    FProtoWeatherAmbient() {
        sensorType = FPW_Ambient;
    }

    void feed(bool level, uint32_t duration) {
        ManchesterEvent event = ManchesterEventReset;
        if (!level) {
            if (DURATION_DIFF(duration, te_short) < te_delta) {
                event = ManchesterEventShortLow;
            } else if (
                DURATION_DIFF(duration, te_long) < te_delta * 2) {
                event = ManchesterEventLongLow;
            }
        } else {
            if (DURATION_DIFF(duration, te_short) < te_delta) {
                event = ManchesterEventShortHigh;
            } else if (
                DURATION_DIFF(duration, te_long) < te_delta * 2) {
                event = ManchesterEventLongHigh;
            }
        }
        if (event != ManchesterEventReset) {
            bool bit;
            bool data_ok = FProtoGeneral::manchester_advance(manchester_saved_state, event, &manchester_saved_state, &bit);

            if (data_ok) {
                decode_data = (decode_data << 1) | !bit;
            }

            if (((decode_data & AMBIENT_WEATHER_PACKET_HEADER_MASK) == AMBIENT_WEATHER_PACKET_HEADER_1) ||
                ((decode_data & AMBIENT_WEATHER_PACKET_HEADER_MASK) == AMBIENT_WEATHER_PACKET_HEADER_2)) {
                if (ws_protocol_ambient_weather_check_crc()) {
                    if (callback) callback(this);
                    decode_data = 0;
                    decode_count_bit = 0;
                }
            }
        } else {
            decode_data = 0;
            decode_count_bit = 0;
            FProtoGeneral::manchester_advance(manchester_saved_state, ManchesterEventReset, &manchester_saved_state, NULL);
        }
    }

   protected:
    uint32_t te_short = 500;
    uint32_t te_long = 1000;
    uint32_t te_delta = 120;
    uint32_t min_count_bit_for_found = 48;
    ManchesterState manchester_saved_state = ManchesterStateMid1;
    bool ws_protocol_ambient_weather_check_crc() {
        uint8_t msg[] = {
            static_cast<uint8_t>(decode_data >> 40),
            static_cast<uint8_t>(decode_data >> 32),
            static_cast<uint8_t>(decode_data >> 24),
            static_cast<uint8_t>(decode_data >> 16),
            static_cast<uint8_t>(decode_data >> 8)};

        uint8_t crc = FProtoGeneral::subghz_protocol_blocks_lfsr_digest8(msg, 5, 0x98, 0x3e) ^ 0x64;
        return (crc == (uint8_t)(decode_data & 0xFF));
    }
};

#endif
