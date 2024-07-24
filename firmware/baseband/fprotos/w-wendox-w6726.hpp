
#ifndef __FPROTO_WENDOX_W6726_H__
#define __FPROTO_WENDOX_W6726_H__

#include "weatherbase.hpp"
typedef enum {
    WendoxW6726DecoderStepReset = 0,
    WendoxW6726DecoderStepCheckPreambule,
    WendoxW6726DecoderStepSaveDuration,
    WendoxW6726DecoderStepCheckDuration,
} WendoxW6726DecoderStep;

class FProtoWeatherWendoxW6726 : public FProtoWeatherBase {
   public:
    FProtoWeatherWendoxW6726() {
        sensorType = FPW_WENDOX_W6726;
    }

    void feed(bool level, uint32_t duration) override {
        switch (parser_step) {
            case WendoxW6726DecoderStepReset:
                if ((level) && (DURATION_DIFF(duration, te_short) <
                                te_delta)) {
                    parser_step = WendoxW6726DecoderStepCheckPreambule;
                    te_last = duration;
                    header_count = 0;
                }
                break;

            case WendoxW6726DecoderStepCheckPreambule:
                if (level) {
                    te_last = duration;
                } else {
                    if ((DURATION_DIFF(te_last, te_short) <
                         te_delta * 1) &&
                        (DURATION_DIFF(duration, te_long) <
                         te_delta * 2)) {
                        header_count++;
                    } else if ((header_count > 4) && (header_count < 12)) {
                        if ((DURATION_DIFF(
                                 te_last, te_long) <
                             te_delta * 2) &&
                            (DURATION_DIFF(duration, te_short) <
                             te_delta)) {
                            decode_data = 0;
                            decode_count_bit = 0;
                            subghz_protocol_blocks_add_bit(1);
                            parser_step = WendoxW6726DecoderStepSaveDuration;
                        } else {
                            parser_step = WendoxW6726DecoderStepReset;
                        }

                    } else {
                        parser_step = WendoxW6726DecoderStepReset;
                    }
                }
                break;

            case WendoxW6726DecoderStepSaveDuration:
                if (level) {
                    te_last = duration;
                    parser_step = WendoxW6726DecoderStepCheckDuration;
                } else {
                    parser_step = WendoxW6726DecoderStepReset;
                }
                break;

            case WendoxW6726DecoderStepCheckDuration:
                if (!level) {
                    if (duration >
                        te_short + te_long) {
                        if (DURATION_DIFF(te_last, te_short) < te_delta) {
                            subghz_protocol_blocks_add_bit(0);
                            parser_step = WendoxW6726DecoderStepSaveDuration;
                        } else if (
                            DURATION_DIFF(te_last, te_long) < te_delta * 2) {
                            subghz_protocol_blocks_add_bit(1);
                            parser_step = WendoxW6726DecoderStepSaveDuration;
                        } else {
                            parser_step = WendoxW6726DecoderStepReset;
                        }
                        if ((decode_count_bit ==
                             min_count_bit_for_found) &&
                            ws_protocol_wendox_w6726_check()) {
                            data = decode_data;
                            data_count_bit = decode_count_bit;
                            ws_protocol_wendox_w6726_remote_controller();
                            if (callback) callback(this);
                        }

                        parser_step = WendoxW6726DecoderStepReset;
                    } else if (
                        (DURATION_DIFF(te_last, te_short) <
                         te_delta) &&
                        (DURATION_DIFF(duration, te_long) <
                         te_delta * 3)) {
                        subghz_protocol_blocks_add_bit(0);
                        parser_step = WendoxW6726DecoderStepSaveDuration;
                    } else if (
                        (DURATION_DIFF(te_last, te_long) <
                         te_delta * 2) &&
                        (DURATION_DIFF(duration, te_short) <
                         te_delta)) {
                        subghz_protocol_blocks_add_bit(1);
                        parser_step = WendoxW6726DecoderStepSaveDuration;
                    } else {
                        parser_step = WendoxW6726DecoderStepReset;
                    }
                } else {
                    parser_step = WendoxW6726DecoderStepReset;
                }
                break;
        }
    }

   protected:
    // timing values
    uint32_t te_short = 1955;
    uint32_t te_long = 5865;
    uint32_t te_delta = 300;
    uint32_t min_count_bit_for_found = 29;

    bool ws_protocol_wendox_w6726_check() {
        if (!decode_data) return false;
        uint8_t msg[] = {
            static_cast<uint8_t>(decode_data >> 28),
            static_cast<uint8_t>(decode_data >> 20),
            static_cast<uint8_t>(decode_data >> 12),
            static_cast<uint8_t>(decode_data >> 4)};

        uint8_t crc = FProtoGeneral::subghz_protocol_blocks_crc4(msg, 4, 0x9, 0xD);
        return (crc == (decode_data & 0x0F));
    }
    void ws_protocol_wendox_w6726_remote_controller() {
        id = (data >> 24) & 0xFF;
        battery_low = (data >> 6) & 1;
        channel = WS_NO_CHANNEL;

        if (((data >> 23) & 1)) {
            temp = (float)(((data >> 14) & 0x1FF) + 12) / 10.0f;
        } else {
            temp = (float)((~(data >> 14) & 0x1FF) + 1 - 12) / -10.0f;
        }

        if (temp < -50.0f) {
            temp = -50.0f;
        } else if (temp > 70.0f) {
            temp = 70.0f;
        }
        humidity = WS_NO_HUMIDITY;
    }
};

#endif
