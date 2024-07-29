
#ifndef __FPROTO_MAGELLAN_H__
#define __FPROTO_MAGELLAN_H__

#include "subghzdbase.hpp"

typedef enum : uint8_t {
    MagellanDecoderStepReset = 0,
    MagellanDecoderStepCheckPreambula,
    MagellanDecoderStepFoundPreambula,
    MagellanDecoderStepSaveDuration,
    MagellanDecoderStepCheckDuration,
} MagellanDecoderStep;

class FProtoSubGhzDMagellan : public FProtoSubGhzDBase {
   public:
    FProtoSubGhzDMagellan() {
        sensorType = FPS_MAGELLAN;
        te_short = 200;
        te_long = 400;
        te_delta = 100;
        min_count_bit_for_found = 32;
    }

    void feed(bool level, uint32_t duration) {
        switch (parser_step) {
            case MagellanDecoderStepReset:
                if ((level) && (DURATION_DIFF(duration, te_short) < te_delta)) {
                    parser_step = MagellanDecoderStepCheckPreambula;
                    te_last = duration;
                    header_count = 0;
                }
                break;

            case MagellanDecoderStepCheckPreambula:
                if (level) {
                    te_last = duration;
                } else {
                    if ((DURATION_DIFF(te_last, te_short) < te_delta) &&
                        (DURATION_DIFF(duration, te_short) < te_delta)) {
                        // Found header
                        header_count++;
                    } else if (
                        (DURATION_DIFF(te_last, te_short) < te_delta) &&
                        (DURATION_DIFF(duration, te_long) < te_delta * 2) &&
                        (header_count > 10)) {
                        parser_step = MagellanDecoderStepFoundPreambula;
                    } else {
                        parser_step = MagellanDecoderStepReset;
                    }
                }
                break;

            case MagellanDecoderStepFoundPreambula:
                if (level) {
                    te_last = duration;
                } else {
                    if ((DURATION_DIFF(te_last, te_short * 6) < te_delta * 3) &&
                        (DURATION_DIFF(duration, te_long) < te_delta * 2)) {
                        parser_step = MagellanDecoderStepSaveDuration;
                        decode_data = 0;
                        decode_count_bit = 0;
                    } else {
                        parser_step = MagellanDecoderStepReset;
                    }
                }
                break;

            case MagellanDecoderStepSaveDuration:
                if (level) {
                    te_last = duration;
                    parser_step = MagellanDecoderStepCheckDuration;
                } else {
                    parser_step = MagellanDecoderStepReset;
                }
                break;

            case MagellanDecoderStepCheckDuration:
                if (!level) {
                    if ((DURATION_DIFF(te_last, te_short) < te_delta) &&
                        (DURATION_DIFF(duration, te_long) < te_delta)) {
                        subghz_protocol_blocks_add_bit(1);
                        parser_step = MagellanDecoderStepSaveDuration;
                    } else if (
                        (DURATION_DIFF(te_last, te_long) < te_delta) &&
                        (DURATION_DIFF(duration, te_short) < te_delta)) {
                        subghz_protocol_blocks_add_bit(0);
                        parser_step = MagellanDecoderStepSaveDuration;
                    } else if (duration >= (te_long * 3)) {
                        // Found stop bit
                        if ((decode_count_bit == min_count_bit_for_found) &&
                            subghz_protocol_magellan_check_crc()) {
                            data = decode_data;
                            data_count_bit = decode_count_bit;
                            if (callback) callback(this);
                        }
                        decode_data = 0;
                        decode_count_bit = 0;
                        parser_step = MagellanDecoderStepReset;
                    } else {
                        parser_step = MagellanDecoderStepReset;
                    }
                } else {
                    parser_step = MagellanDecoderStepReset;
                }
                break;
        }
    }

   protected:
    bool subghz_protocol_magellan_check_crc() {
        uint8_t data[3] = {
            (uint8_t)(decode_data >> 24),
            (uint8_t)(decode_data >> 16),
            (uint8_t)(decode_data >> 8)};
        return (decode_data & 0xFF) == subghz_protocol_magellan_crc8(data, sizeof(data));
    }
    uint8_t subghz_protocol_magellan_crc8(uint8_t* data, size_t len) {
        uint8_t crc = 0x00;
        uint8_t i, j;
        for (i = 0; i < len; i++) {
            crc ^= data[i];
            for (j = 0; j < 8; j++) {
                if ((crc & 0x80) != 0)
                    crc = (uint8_t)((crc << 1) ^ 0x31);
                else
                    crc <<= 1;
            }
        }
        return crc;
    }
};

#endif
