
#ifndef __FPROTO_LINEARDELTA3_H__
#define __FPROTO_LINEARDELTA3_H__

#include "subghzdbase.hpp"

typedef enum : uint8_t {
    LinearD3DecoderStepReset = 0,
    LinearD3DecoderStepSaveDuration,
    LinearD3DecoderStepCheckDuration,
} LinearD3DecoderStep;

class FProtoSubGhzDLinearDelta3 : public FProtoSubGhzDBase {
   public:
    FProtoSubGhzDLinearDelta3() {
        sensorType = FPS_LINEARDELTA3;
        te_short = 500;
        te_long = 2000;
        te_delta = 150;
        min_count_bit_for_found = 8;
    }

    void feed(bool level, uint32_t duration) {
        switch (parser_step) {
            case LinearD3DecoderStepReset:
                if ((!level) && (DURATION_DIFF(duration, te_short * 70) < te_delta * 24)) {
                    // Found header Linear
                    decode_data = 0;
                    decode_count_bit = 0;
                    parser_step = LinearD3DecoderStepSaveDuration;
                }
                break;
            case LinearD3DecoderStepSaveDuration:
                if (level) {
                    te_last = duration;
                    parser_step = LinearD3DecoderStepCheckDuration;
                } else {
                    parser_step = LinearD3DecoderStepReset;
                }
                break;
            case LinearD3DecoderStepCheckDuration:
                if (!level) {
                    if (duration >= (te_short * 10)) {
                        parser_step = LinearD3DecoderStepReset;
                        if (DURATION_DIFF(te_last, te_short) < te_delta) {
                            subghz_protocol_blocks_add_bit(1);
                        } else if (
                            DURATION_DIFF(te_last, te_long) < te_delta) {
                            subghz_protocol_blocks_add_bit(0);
                        }
                        if (decode_count_bit == min_count_bit_for_found) {
                            if ((data == decode_data) && data) {
                                data = decode_data;
                                data_count_bit = decode_count_bit;
                                if (callback) callback(this);
                            }
                            parser_step = LinearD3DecoderStepSaveDuration;
                        }
                        break;
                    }

                    if ((DURATION_DIFF(te_last, te_short) < te_delta) &&
                        (DURATION_DIFF(duration, te_short * 7) < te_delta)) {
                        subghz_protocol_blocks_add_bit(1);
                        parser_step = LinearD3DecoderStepSaveDuration;
                    } else if (
                        (DURATION_DIFF(te_last, te_long) < te_delta) &&
                        (DURATION_DIFF(duration, te_long) < te_delta)) {
                        subghz_protocol_blocks_add_bit(0);
                        parser_step = LinearD3DecoderStepSaveDuration;
                    } else {
                        parser_step = LinearD3DecoderStepReset;
                    }

                } else {
                    parser_step = LinearD3DecoderStepReset;
                }
                break;
        }
    }
};

#endif
