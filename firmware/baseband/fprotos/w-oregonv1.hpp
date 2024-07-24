
#ifndef __FPROTO_OREGONv1_H__
#define __FPROTO_OREGONv1_H__

#include "weatherbase.hpp"

typedef enum {
    Oregon_V1DecoderStepReset = 0,
    Oregon_V1DecoderStepFoundPreamble,
    Oregon_V1DecoderStepParse,
} Oregon_V1DecoderStep;

#define OREGON_V1_HEADER_OK 0xFF

class FProtoWeatherOregonV1 : public FProtoWeatherBase {
   public:
    FProtoWeatherOregonV1() {
        sensorType = FPW_OREGONv1;
    }

    void feed(bool level, uint32_t duration) override {
        ManchesterEvent event = ManchesterEventReset;
        switch (parser_step) {
            case Oregon_V1DecoderStepReset:
                if ((level) && (DURATION_DIFF(duration, te_short) <
                                te_delta)) {
                    parser_step = Oregon_V1DecoderStepFoundPreamble;
                    te_last = duration;
                    header_count = 0;
                }
                break;
            case Oregon_V1DecoderStepFoundPreamble:
                if (level) {
                    // keep high levels, if they suit our durations
                    if ((DURATION_DIFF(duration, te_short) <
                         te_delta) ||
                        (DURATION_DIFF(duration, te_short * 4) <
                         te_delta)) {
                        te_last = duration;
                    } else {
                        parser_step = Oregon_V1DecoderStepReset;
                    }
                } else if (
                    // checking low levels
                    (DURATION_DIFF(duration, te_short) <
                     te_delta) &&
                    (DURATION_DIFF(te_last, te_short) <
                     te_delta)) {
                    // Found header
                    header_count++;
                } else if (
                    (DURATION_DIFF(duration, te_short * 3) <
                     te_delta) &&
                    (DURATION_DIFF(te_last, te_short) <
                     te_delta)) {
                    // check header
                    if (header_count > 7) {
                        header_count = OREGON_V1_HEADER_OK;
                    }
                } else if (
                    (header_count == OREGON_V1_HEADER_OK) &&
                    (DURATION_DIFF(te_last, te_short * 4) <
                     te_delta)) {
                    // found all the necessary patterns
                    decode_data = 0;
                    decode_count_bit = 1;
                    FProtoGeneral::manchester_advance(manchester_saved_state, ManchesterEventReset, &manchester_saved_state, NULL);
                    parser_step = Oregon_V1DecoderStepParse;
                    if (duration < te_short * 4) {
                        first_bit = 1;
                    } else {
                        first_bit = 0;
                    }
                } else {
                    parser_step = Oregon_V1DecoderStepReset;
                }
                break;
            case Oregon_V1DecoderStepParse:
                if (level) {
                    if (DURATION_DIFF(duration, te_short) <
                        te_delta) {
                        event = ManchesterEventShortHigh;
                    } else if (
                        DURATION_DIFF(duration, te_long) <
                        te_delta) {
                        event = ManchesterEventLongHigh;
                    } else {
                        parser_step = Oregon_V1DecoderStepReset;
                    }
                } else {
                    if (DURATION_DIFF(duration, te_short) <
                        te_delta) {
                        event = ManchesterEventShortLow;
                    } else if (
                        DURATION_DIFF(duration, te_long) <
                        te_delta) {
                        event = ManchesterEventLongLow;
                    } else if (duration >= ((uint32_t)te_long * 2)) {
                        if (decode_count_bit ==
                            min_count_bit_for_found) {
                            if (first_bit) {
                                decode_data = ~decode_data | (1 << 31);
                            }
                            if (ws_protocol_oregon_v1_check()) {
                                data = decode_data;
                                data_count_bit = decode_count_bit;
                                ws_protocol_oregon_v1_remote_controller();
                                if (callback) callback(this);
                            }
                        }
                        decode_data = 0;
                        decode_count_bit = 0;
                        FProtoGeneral::manchester_advance(manchester_saved_state, ManchesterEventReset, &manchester_saved_state, NULL);
                    } else {
                        parser_step = Oregon_V1DecoderStepReset;
                    }
                }
                if (event != ManchesterEventReset) {
                    bool bit;
                    bool data_ok = FProtoGeneral::manchester_advance(manchester_saved_state, event, &manchester_saved_state, &bit);

                    if (data_ok) {
                        decode_data = (decode_data << 1) | !bit;
                        decode_count_bit++;
                    }
                }

                break;
        }
    }

   protected:
    // timing values
    uint32_t te_short = 1465;
    uint32_t te_long = 2930;
    uint32_t te_delta = 350;
    uint32_t min_count_bit_for_found = 32;

    uint8_t first_bit{0};
    ManchesterState manchester_saved_state = ManchesterStateMid1;
    bool ws_protocol_oregon_v1_check() {
        if (!decode_data) return false;
        uint64_t data = FProtoGeneral::subghz_protocol_blocks_reverse_key(decode_data, 32);
        uint16_t crc = (data & 0xff) + ((data >> 8) & 0xff) + ((data >> 16) & 0xff);
        crc = (crc & 0xff) + ((crc >> 8) & 0xff);
        return (crc == ((data >> 24) & 0xFF));
    }
    void ws_protocol_oregon_v1_remote_controller() {
        uint64_t data2 = FProtoGeneral::subghz_protocol_blocks_reverse_key(data, 32);

        id = data2 & 0xFF;
        channel = ((data2 >> 6) & 0x03) + 1;

        float temp_raw =
            ((data2 >> 8) & 0x0F) * 0.1f + ((data2 >> 12) & 0x0F) + ((data2 >> 16) & 0x0F) * 10.0f;
        if (!((data2 >> 21) & 1)) {
            temp = temp_raw;
        } else {
            temp = -temp_raw;
        }
        battery_low = !((data2 >> 23) & 1ULL);
        humidity = WS_NO_HUMIDITY;
    }
};

#endif
