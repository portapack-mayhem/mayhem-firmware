
#ifndef __FPROTO_Acurite_986_H__
#define __FPROTO_Acurite_986_H__

#include "weatherbase.hpp"

typedef enum {
    Acurite_986DecoderStepReset = 0,
    Acurite_986DecoderStepSync1,
    Acurite_986DecoderStepSync2,
    Acurite_986DecoderStepSync3,
    Acurite_986DecoderStepSaveDuration,
    Acurite_986DecoderStepCheckDuration,
} Acurite_986DecoderStep;

class FProtoWeatherAcurite986 : public FProtoWeatherBase {
   public:
    FProtoWeatherAcurite986() {
        sensorType = FPW_Acurite986;
    }

    void feed(bool level, uint32_t duration) {
        switch (parser_step) {
            case Acurite_986DecoderStepReset:
                if ((!level) && (DURATION_DIFF(duration, te_long) < te_delta * 15)) {
                    // Found 1st sync bit
                    parser_step = Acurite_986DecoderStepSync1;
                    decode_data = 0;
                    decode_count_bit = 0;
                }
                break;

            case Acurite_986DecoderStepSync1:
                if (DURATION_DIFF(duration, te_long) < te_delta * 15) {
                    if (!level) {
                        parser_step = Acurite_986DecoderStepSync2;
                    }
                } else {
                    parser_step = Acurite_986DecoderStepReset;
                }
                break;

            case Acurite_986DecoderStepSync2:
                if (DURATION_DIFF(duration, te_long) < te_delta * 15) {
                    if (!level) {
                        parser_step = Acurite_986DecoderStepSync3;
                    }
                } else {
                    parser_step = Acurite_986DecoderStepReset;
                }
                break;

            case Acurite_986DecoderStepSync3:
                if (DURATION_DIFF(duration, te_long) < te_delta * 15) {
                    if (!level) {
                        parser_step = Acurite_986DecoderStepSaveDuration;
                    }
                } else {
                    parser_step = Acurite_986DecoderStepReset;
                }
                break;

            case Acurite_986DecoderStepSaveDuration:
                if (level) {
                    te_last = duration;
                    parser_step = Acurite_986DecoderStepCheckDuration;
                } else {
                    parser_step = Acurite_986DecoderStepReset;
                }
                break;

            case Acurite_986DecoderStepCheckDuration:
                if (!level) {
                    if (DURATION_DIFF(duration, te_short) <
                        te_delta * 10) {
                        if (duration < te_short) {
                            subghz_protocol_blocks_add_bit(0);
                            parser_step = Acurite_986DecoderStepSaveDuration;
                        } else {
                            subghz_protocol_blocks_add_bit(1);
                            parser_step = Acurite_986DecoderStepSaveDuration;
                        }
                    } else {
                        // Found syncPostfix
                        parser_step = Acurite_986DecoderStepReset;
                        if ((decode_count_bit == min_count_bit_for_found) && ws_protocol_acurite_986_check()) {
                            data = decode_data;
                            data_count_bit = decode_count_bit;
                            ws_protocol_acurite_986_remote_controller();
                            if (callback) callback(this);
                        }
                        decode_data = 0;
                        decode_count_bit = 0;
                    }
                } else {
                    parser_step = Acurite_986DecoderStepReset;
                }
                break;
        }
    }

   protected:
    uint32_t te_short = 800;
    uint32_t te_long = 1750;
    uint32_t te_delta = 50;
    uint32_t min_count_bit_for_found = 40;

    void ws_protocol_acurite_986_remote_controller() {
        int temp;

        id = FProtoGeneral::subghz_protocol_blocks_reverse_key(data >> 24, 8);
        id = (id << 8) | FProtoGeneral::subghz_protocol_blocks_reverse_key(data >> 16, 8);
        battery_low = (data >> 14) & 1;
        channel = ((data >> 15) & 1) + 1;

        temp = FProtoGeneral::subghz_protocol_blocks_reverse_key(data >> 32, 8);
        if (temp & 0x80) {
            temp = -(temp & 0x7F);
        }
        temp = FProtoGeneral::locale_fahrenheit_to_celsius((float)temp);
        humidity = WS_NO_HUMIDITY;
    }

    bool ws_protocol_acurite_986_check() {
        if (!decode_data) return false;
        uint8_t msg[] = {
            (uint8_t)(decode_data >> 32),
            (uint8_t)(decode_data >> 24),
            (uint8_t)(decode_data >> 16),
            (uint8_t)(decode_data >> 8)};

        uint8_t crc = FProtoGeneral::subghz_protocol_blocks_crc8(msg, 4, 0x07, 0x00);
        return (crc == (decode_data & 0xFF));
    }
};

#endif
