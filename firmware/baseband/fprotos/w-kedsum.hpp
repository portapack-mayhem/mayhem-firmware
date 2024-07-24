
#ifndef __FPROTO_Kedsum_H__
#define __FPROTO_Kedsum_H__

#include "weatherbase.hpp"

typedef enum {
    KedsumTHDecoderStepReset = 0,
    KedsumTHDecoderStepCheckPreambule,
    KedsumTHDecoderStepSaveDuration,
    KedsumTHDecoderStepCheckDuration,
} KedsumTHDecoderStep;

class FProtoWeatherKedsum : public FProtoWeatherBase {
   public:
    FProtoWeatherKedsum() {
        sensorType = FPW_KEDSUM;
    }

    void feed(bool level, uint32_t duration) {
        switch (parser_step) {
            case KedsumTHDecoderStepReset:
                if ((level) && (DURATION_DIFF(duration, te_short) < te_delta)) {
                    parser_step = KedsumTHDecoderStepCheckPreambule;
                    te_last = duration;
                    header_count = 0;
                }
                break;

            case KedsumTHDecoderStepCheckPreambule:
                if (level) {
                    te_last = duration;
                } else {
                    if ((DURATION_DIFF(te_last, te_short) < te_delta) &&
                        (DURATION_DIFF(duration, te_long * 4) < te_delta * 4)) {
                        // Found preambule
                        header_count++;
                    } else if (
                        (DURATION_DIFF(te_last, te_short) < te_delta) &&
                        (duration < (te_long * 2 + te_delta * 2))) {
                        // Found syncPrefix
                        if (header_count > 0) {
                            parser_step = KedsumTHDecoderStepSaveDuration;
                            decode_data = 0;
                            decode_count_bit = 0;
                            if ((DURATION_DIFF(te_last, te_short) < te_delta) &&
                                (DURATION_DIFF(duration, te_long) < te_delta * 2)) {
                                subghz_protocol_blocks_add_bit(0);
                                parser_step = KedsumTHDecoderStepSaveDuration;
                            } else if (
                                (DURATION_DIFF(te_last, te_short) < te_delta) &&
                                (DURATION_DIFF(duration, te_long * 2) < te_delta * 4)) {
                                subghz_protocol_blocks_add_bit(1);
                                parser_step = KedsumTHDecoderStepSaveDuration;
                            } else {
                                parser_step = KedsumTHDecoderStepReset;
                            }
                        }
                    } else {
                        parser_step = KedsumTHDecoderStepReset;
                    }
                }
                break;

            case KedsumTHDecoderStepSaveDuration:
                if (level) {
                    te_last = duration;
                    parser_step = KedsumTHDecoderStepCheckDuration;
                } else {
                    parser_step = KedsumTHDecoderStepReset;
                }
                break;

            case KedsumTHDecoderStepCheckDuration:
                if (!level) {
                    if (DURATION_DIFF(duration, te_long * 4) < te_delta * 4) {
                        // Found syncPostfix
                        if ((decode_count_bit ==
                             min_count_bit_for_found) &&
                            ws_protocol_kedsum_th_check_crc()) {
                            data = decode_data;
                            data_count_bit = decode_count_bit;
                            ws_protocol_kedsum_th_remote_controller();
                            if (callback) callback(this);
                        }
                        decode_data = 0;
                        decode_count_bit = 0;
                        parser_step = KedsumTHDecoderStepReset;
                        break;
                    } else if (
                        (DURATION_DIFF(te_last, te_short) < te_delta) &&
                        (DURATION_DIFF(duration, te_long) < te_delta * 2)) {
                        subghz_protocol_blocks_add_bit(0);
                        parser_step = KedsumTHDecoderStepSaveDuration;
                    } else if (
                        (DURATION_DIFF(te_last, te_short) <
                         te_delta) &&
                        (DURATION_DIFF(duration, te_long * 2) <
                         te_delta * 4)) {
                        subghz_protocol_blocks_add_bit(1);
                        parser_step = KedsumTHDecoderStepSaveDuration;
                    } else {
                        parser_step = KedsumTHDecoderStepReset;
                    }
                } else {
                    parser_step = KedsumTHDecoderStepReset;
                }
                break;
        }
    }

   protected:
    uint32_t te_short = 500;
    uint32_t te_long = 2000;
    uint32_t te_delta = 150;
    uint32_t min_count_bit_for_found = 42;

    bool ws_protocol_kedsum_th_check_crc() {
        uint8_t msg[] = {
            static_cast<uint8_t>(decode_data >> 32),
            static_cast<uint8_t>(decode_data >> 24),
            static_cast<uint8_t>(decode_data >> 16),
            static_cast<uint8_t>(decode_data >> 8),
            static_cast<uint8_t>(decode_data)};

        uint8_t crc = FProtoGeneral::subghz_protocol_blocks_crc4(msg, 4, 0x03, 0);  // CRC-4 poly 0x3 init 0x0 xor last 4 bits
        crc ^= msg[4] >> 4;                                                         // last nibble is only XORed
        return (crc == (msg[4] & 0x0F));
    }

    void ws_protocol_kedsum_th_remote_controller() {
        id = data >> 32;
        if ((data >> 30) & 0x3) {
            battery_low = 0;
        } else {
            battery_low = 1;
        }
        channel = ((data >> 28) & 0x3) + 1;
        uint16_t temp_raw = ((data >> 16) & 0x0f) << 8 |
                            ((data >> 20) & 0x0f) << 4 | ((data >> 24) & 0x0f);
        temp = FProtoGeneral::locale_fahrenheit_to_celsius(((float)temp_raw - 900.0f) / 10.0f);
        humidity = ((data >> 8) & 0x0f) << 4 | ((data >> 12) & 0x0f);
    }
};

#endif
