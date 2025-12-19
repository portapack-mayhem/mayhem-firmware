#pragma once
#include "subcarbase.hpp"
#include <cstring>

typedef enum {
    SubaruDecoderStepReset = 0,
    SubaruDecoderStepCheckPreamble,
    SubaruDecoderStepFoundGap,
    SubaruDecoderStepFoundSync,
    SubaruDecoderStepSaveDuration,
    SubaruDecoderStepCheckDuration,
} SubaruDecoderStep;

class FProtoSubCarSubaru : public FProtoSubCarBase {
   public:
    FProtoSubCarSubaru() {
        sensorType = FPC_SUBARU;
        te_short = 800;
        te_long = 1600;
        te_delta = 260;
        min_count_bit_for_found = 64;
    }
    void subghz_protocol_decoder_subaru_reset() {
        parser_step = SubaruDecoderStepReset;
        te_last = 0;
        header_count = 0;
        bit_count = 0;
        memset(data, 0, sizeof(data));
    }

    void subaru_add_bit(bool bit) {
        if (bit_count < 64) {
            uint8_t byte_idx = bit_count / 8;
            uint8_t bit_idx = 7 - (bit_count % 8);
            if (bit) {
                data[byte_idx] |= (1 << bit_idx);

            } else {
                data[byte_idx] &= ~(1 << bit_idx);
            }
            bit_count++;
        }
    }

    bool subaru_process_data() {
        if (bit_count < 64) {
            return false;
        }
        uint8_t* b = data;

        uint64_t key = ((uint64_t)b[0] << 56) | ((uint64_t)b[1] << 48) |
                       ((uint64_t)b[2] << 40) | ((uint64_t)b[3] << 32) |
                       ((uint64_t)b[4] << 24) | ((uint64_t)b[5] << 16) |
                       ((uint64_t)b[6] << 8) | ((uint64_t)b[7]);
        decode_data = key;
        // uint32_t serial = ((uint32_t)b[1] << 16) | ((uint32_t)b[2] << 8) | b[3];
        // uint8_t button = b[0] & 0x0F;
        // uint16_t cnt;
        // subaru_decode_count(b, &cnt);
        data_count_bit = decode_count_bit;
        if (callback) {
            callback(this);
        }
        return true;
    }

    void feed(bool level, uint32_t duration) {
        switch (parser_step) {
            case SubaruDecoderStepReset:
                if (level && DURATION_DIFF(duration, te_long) < te_delta) {
                    parser_step = SubaruDecoderStepCheckPreamble;
                    te_last = duration;
                    header_count = 1;
                }
                break;

            case SubaruDecoderStepCheckPreamble:
                if (!level) {
                    if (DURATION_DIFF(duration, te_long) < te_delta) {
                        header_count++;
                    } else if (duration > 2000 && duration < 3500) {
                        if (header_count > 20) {
                            parser_step = SubaruDecoderStepFoundGap;
                        } else {
                            parser_step = SubaruDecoderStepReset;
                        }
                    } else {
                        parser_step = SubaruDecoderStepReset;
                    }
                } else {
                    if (DURATION_DIFF(duration, te_long) < te_delta) {
                        te_last = duration;
                        header_count++;
                    } else {
                        parser_step = SubaruDecoderStepReset;
                    }
                }
                break;

            case SubaruDecoderStepFoundGap:
                if (level && duration > 2000 && duration < 3500) {
                    parser_step = SubaruDecoderStepFoundSync;
                } else {
                    parser_step = SubaruDecoderStepReset;
                }
                break;

            case SubaruDecoderStepFoundSync:
                if (!level && DURATION_DIFF(duration, te_long) < te_delta) {
                    parser_step = SubaruDecoderStepSaveDuration;
                    bit_count = 0;
                    memset(data, 0, sizeof(data));
                } else {
                    parser_step = SubaruDecoderStepReset;
                }
                break;

            case SubaruDecoderStepSaveDuration:
                if (level) {
                    // HIGH pulse duration encodes the bit:
                    // Short HIGH (~800µs) = 1
                    // Long HIGH (~1600µs) = 0
                    if (DURATION_DIFF(duration, te_short) < te_delta) {
                        // Short HIGH = bit 1
                        subaru_add_bit(true);
                        te_last = duration;
                        parser_step = SubaruDecoderStepCheckDuration;
                    } else if (DURATION_DIFF(duration, te_long) < te_delta) {
                        // Long HIGH = bit 0
                        subaru_add_bit(false);
                        te_last = duration;
                        parser_step = SubaruDecoderStepCheckDuration;
                    } else if (duration > 3000) {
                        // End of transmission
                        if (bit_count >= 64) {
                            subaru_process_data();
                        }
                        parser_step = SubaruDecoderStepReset;
                    } else {
                        parser_step = SubaruDecoderStepReset;
                    }
                } else {
                    parser_step = SubaruDecoderStepReset;
                }
                break;

            case SubaruDecoderStepCheckDuration:
                if (!level) {
                    // LOW pulse - just validates timing, doesn't encode bit
                    if (DURATION_DIFF(duration, te_short) < te_delta ||
                        DURATION_DIFF(duration, te_long) < te_delta) {
                        parser_step = SubaruDecoderStepSaveDuration;
                    } else if (duration > 3000) {
                        // Gap - end of packet
                        if (bit_count >= 64) {
                            subaru_process_data();
                        }
                        parser_step = SubaruDecoderStepReset;
                    } else {
                        parser_step = SubaruDecoderStepReset;
                    }
                } else {
                    parser_step = SubaruDecoderStepReset;
                }
                break;
        }
    }

    uint16_t header_count = 0;
    uint8_t data[8];
    uint8_t bit_count = 0;
};
