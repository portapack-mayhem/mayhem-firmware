
#ifndef __FPROTO_X10_H__
#define __FPROTO_X10_H__

#include "subghzdbase.hpp"

typedef enum {
    X10DecoderStepReset = 0,
    X10DecoderStepFoundPreambula,
    X10DecoderStepSaveDuration,
    X10DecoderStepCheckDuration,
} X10DecoderStep;

class FProtoSubGhzDX10 : public FProtoSubGhzDBase {
   public:
    FProtoSubGhzDX10() {
        sensorType = FPS_X10;
        te_short = 600;
        te_long = 1800;
        te_delta = 100;
        min_count_bit_for_found = 32;
    }

    void feed(bool level, uint32_t duration) {
        switch (parser_step) {
            case X10DecoderStepReset:
                if ((level) && (DURATION_DIFF(duration, te_short * 16) < te_delta * 7)) {
                    parser_step = X10DecoderStepFoundPreambula;
                }
                break;
            case X10DecoderStepFoundPreambula:
                if ((!level) && (DURATION_DIFF(duration, te_short * 8) < te_delta * 5)) {
                    parser_step = X10DecoderStepSaveDuration;
                    decode_data = 0;
                    decode_count_bit = 0;
                } else {
                    decode_data = 0;
                    decode_count_bit = 0;
                    parser_step = X10DecoderStepReset;
                }
                break;
            case X10DecoderStepSaveDuration:
                if (level) {
                    if (DURATION_DIFF(duration, te_short) < te_delta) {
                        if (decode_count_bit == min_count_bit_for_found) {
                            parser_step = X10DecoderStepReset;
                            if (decode_count_bit >= min_count_bit_for_found &&
                                ((((decode_data >> 24) ^ (decode_data >> 16)) & 0xFF) == 0xFF) &&
                                ((((decode_data >> 8) ^ (decode_data)) & 0xFF) == 0xFF)) {
                                data = decode_data;
                                data_count_bit = decode_count_bit;

                                if (callback) callback(this);
                            }
                            decode_data = 0;
                            decode_count_bit = 0;
                            parser_step = X10DecoderStepReset;
                        } else {
                            te_last = duration;
                            parser_step = X10DecoderStepCheckDuration;
                        }
                    } else {
                        decode_data = 0;
                        decode_count_bit = 0;
                        parser_step = X10DecoderStepReset;
                    }
                } else {
                    decode_data = 0;
                    decode_count_bit = 0;
                    parser_step = X10DecoderStepReset;
                }
                break;
            case X10DecoderStepCheckDuration:
                if (!level) {
                    if ((DURATION_DIFF(te_last, te_short) < te_delta) &&
                        (DURATION_DIFF(duration, te_short) < te_delta)) {
                        subghz_protocol_blocks_add_bit(0);
                        parser_step = X10DecoderStepSaveDuration;
                    } else if (
                        (DURATION_DIFF(te_last, te_short) < te_delta) &&
                        (DURATION_DIFF(duration, te_long) < te_delta * 2)) {
                        subghz_protocol_blocks_add_bit(1);
                        parser_step = X10DecoderStepSaveDuration;
                    } else {
                        decode_data = 0;
                        decode_count_bit = 0;
                        parser_step = X10DecoderStepReset;
                    }
                } else {
                    decode_data = 0;
                    decode_count_bit = 0;
                    parser_step = X10DecoderStepReset;
                }
                break;
        }
    }
};

#endif
