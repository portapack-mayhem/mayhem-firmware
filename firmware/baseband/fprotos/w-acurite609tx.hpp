
#ifndef __FPROTO_Acurite_609TX_H__
#define __FPROTO_Acurite_609TX_H__

#include "weatherbase.hpp"

typedef enum {
    Acurite_609TXCDecoderStepReset = 0,
    Acurite_609TXCDecoderStepSaveDuration,
    Acurite_609TXCDecoderStepCheckDuration,
} Acurite_609TXCDecoderStep;

class FProtoWeatherAcurite609TX : public FProtoWeatherBase {
   public:
    FProtoWeatherAcurite609TX() {
        sensorType = FPW_Acurite609TX;
    }

    void feed(bool level, uint32_t duration) {
        switch (parser_step) {
            case Acurite_609TXCDecoderStepReset:
                if ((!level) && (DURATION_DIFF(duration, te_short * 17) <
                                 te_delta * 8)) {
                    // Found syncPrefix
                    parser_step = Acurite_609TXCDecoderStepSaveDuration;
                    decode_data = 0;
                    decode_count_bit = 0;
                }
                break;

            case Acurite_609TXCDecoderStepSaveDuration:
                if (level) {
                    te_last = duration;
                    parser_step = Acurite_609TXCDecoderStepCheckDuration;
                } else {
                    parser_step = Acurite_609TXCDecoderStepReset;
                }
                break;

            case Acurite_609TXCDecoderStepCheckDuration:
                if (!level) {
                    if (DURATION_DIFF(te_last, te_short) < te_delta) {
                        if ((DURATION_DIFF(duration, te_short) < te_delta) || (duration > te_long * 3)) {
                            // Found syncPostfix
                            parser_step = Acurite_609TXCDecoderStepReset;
                            if ((decode_count_bit == min_count_bit_for_found) && ws_protocol_acurite_609txc_check()) {
                                data = decode_data;
                                data_count_bit = decode_count_bit;
                                ws_protocol_acurite_609txc_remote_controller();
                                if (callback) callback(this);
                            }
                            decode_data = 0;
                            decode_count_bit = 0;
                        } else if (DURATION_DIFF(duration, te_long) < te_delta * 2) {
                            subghz_protocol_blocks_add_bit(0);
                            parser_step = Acurite_609TXCDecoderStepSaveDuration;
                        } else if (DURATION_DIFF(duration, te_long * 2) < te_delta * 4) {
                            subghz_protocol_blocks_add_bit(1);
                            parser_step = Acurite_609TXCDecoderStepSaveDuration;
                        } else {
                            parser_step = Acurite_609TXCDecoderStepReset;
                        }
                    } else {
                        parser_step = Acurite_609TXCDecoderStepReset;
                    }
                } else {
                    parser_step = Acurite_609TXCDecoderStepReset;
                }
                break;
        }
    }

   protected:
    uint32_t te_short = 500;
    uint32_t te_long = 1000;
    uint32_t te_delta = 150;
    uint32_t min_count_bit_for_found = 32;

    void ws_protocol_acurite_609txc_remote_controller() {
        id = (data >> 32) & 0xFF;
        battery_low = (data >> 31) & 1;
        channel = WS_NO_CHANNEL;

        // Temperature in Celsius is encoded as a 12 bit integer value
        // multiplied by 10 using the 4th - 6th nybbles (bytes 1 & 2)
        // negative values are recovered by sign extend from int16_t.
        int16_t temp_raw =
            (int16_t)(((data >> 12) & 0xf000) | ((data >> 16) << 4));
        temp = (temp_raw >> 4) * 0.1f;
        humidity = (data >> 8) & 0xff;
    }
    bool ws_protocol_acurite_609txc_check() {
        if (!decode_data) return false;
        uint8_t crc = (uint8_t)(decode_data >> 32) +
                      (uint8_t)(decode_data >> 24) +
                      (uint8_t)(decode_data >> 16) +
                      (uint8_t)(decode_data >> 8);
        return (crc == (decode_data & 0xFF));
    }
};

#endif
