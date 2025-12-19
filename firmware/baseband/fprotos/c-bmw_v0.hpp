#pragma once
#include "subcarbase.hpp"
#include <cstring>

typedef enum {
    BMWDecoderStepReset = 0,
    BMWDecoderStepCheckPreambula,
    BMWDecoderStepSaveDuration,
    BMWDecoderStepCheckDuration,
} BMWDecoderStep;

class FProtoSubCarBMWV0 : public FProtoSubCarBase {
   public:
    FProtoSubCarBMWV0() {
        sensorType = FPC_BMWV0;
        te_short = 350;
        te_long = 700;
        te_delta = 120;
        min_count_bit_for_found = 61;
    }
    uint8_t subghz_protocol_bmw_crc8(uint8_t* data, size_t len) {
        uint8_t crc = 0x00;
        for (size_t i = 0; i < len; i++) {
            crc ^= data[i];
            for (uint8_t j = 0; j < 8; j++) {
                if (crc & 0x80)
                    crc = (uint8_t)((crc << 1) ^ 0x31);
                else
                    crc <<= 1;
            }
        }
        return crc;
    }

    uint16_t subghz_protocol_bmw_crc16(uint8_t* data, size_t len) {
        uint16_t crc = 0xFFFF;
        for (size_t i = 0; i < len; i++) {
            crc ^= ((uint16_t)data[i] << 8);
            for (uint8_t j = 0; j < 8; j++) {
                if (crc & 0x8000)
                    crc = (crc << 1) ^ 0x1021;
                else
                    crc <<= 1;
            }
        }
        return crc;
    }
    void subghz_protocol_decoder_bmw_reset_internal() {
        decode_data = 0;
        decode_count_bit = 0;
        decode_data2 = 0;
        parser_step = BMWDecoderStepReset;
        header_count = 0;
        crc_type = 0;
    }

    void feed(bool level, uint32_t duration) {
        switch (parser_step) {
            case BMWDecoderStepReset:
                if (level && (DURATION_DIFF(duration, te_short) <
                              te_delta)) {
                    parser_step = BMWDecoderStepCheckPreambula;
                    te_last = duration;
                    header_count = 0;
                    decode_data = 0;
                    decode_count_bit = 0;
                }
                break;

            case BMWDecoderStepCheckPreambula:
                if (level) {
                    if ((DURATION_DIFF(duration, te_short) <
                         te_delta) ||
                        (DURATION_DIFF(duration, te_long) <
                         te_delta)) {
                        te_last = duration;
                    } else {
                        parser_step = BMWDecoderStepReset;
                    }
                } else if (
                    (DURATION_DIFF(duration, te_short) <
                     te_delta) &&
                    (DURATION_DIFF(te_last, te_short) <
                     te_delta)) {
                    header_count++;
                } else if (
                    (DURATION_DIFF(duration, te_long) <
                     te_delta) &&
                    (DURATION_DIFF(te_last, te_long) <
                     te_delta)) {
                    if (header_count > 15) {
                        parser_step = BMWDecoderStepSaveDuration;
                        decode_data = 0ULL;
                        decode_count_bit = 0;
                    } else {
                        parser_step = BMWDecoderStepReset;
                    }
                } else {
                    parser_step = BMWDecoderStepReset;
                }
                break;

            case BMWDecoderStepSaveDuration:
                if (level) {
                    if (duration >=
                        (te_long + te_delta * 2UL)) {
                        if (decode_count_bit >=
                            min_count_bit_for_found) {
                            // instance->generic.data = decode_data;
                            data_count_bit = decode_count_bit;

                            // Perform CRC check with both CRC8 and CRC16
                            uint8_t* raw_bytes = (uint8_t*)decode_data;
                            size_t raw_len = (decode_count_bit + 7) / 8;
                            uint8_t crc8 = subghz_protocol_bmw_crc8(raw_bytes, raw_len - 1);
                            if (crc8 == raw_bytes[raw_len - 1]) {
                                crc_type = 8;
                            } else {
                                uint16_t crc16 = subghz_protocol_bmw_crc16(raw_bytes, raw_len - 2);
                                uint16_t rx_crc16 = (raw_bytes[raw_len - 2] << 8) | raw_bytes[raw_len - 1];
                                if (crc16 == rx_crc16) {
                                    crc_type = 16;
                                } else {
                                    crc_type = 0;  // invalid
                                }
                            }

                            if (crc_type != 0 && callback) {
                                callback(this);
                            }
                        }
                        subghz_protocol_decoder_bmw_reset_internal();
                    } else {
                        te_last = duration;
                        parser_step = BMWDecoderStepCheckDuration;
                    }
                } else {
                    parser_step = BMWDecoderStepReset;
                }
                break;

            case BMWDecoderStepCheckDuration:
                if (!level) {
                    if ((DURATION_DIFF(te_last, te_short) <
                         te_delta) &&
                        (DURATION_DIFF(duration, te_short) <
                         te_delta)) {
                        subghz_protocol_blocks_add_bit(0);
                        parser_step = BMWDecoderStepSaveDuration;
                    } else if (
                        (DURATION_DIFF(te_last, te_long) <
                         te_delta) &&
                        (DURATION_DIFF(duration, te_long) <
                         te_delta)) {
                        subghz_protocol_blocks_add_bit(1);
                        parser_step = BMWDecoderStepSaveDuration;
                    } else {
                        parser_step = BMWDecoderStepReset;
                    }
                } else {
                    parser_step = BMWDecoderStepReset;
                }
                break;
        }
    }

    uint16_t header_count = 0;
    uint8_t crc_type = 0;
};
