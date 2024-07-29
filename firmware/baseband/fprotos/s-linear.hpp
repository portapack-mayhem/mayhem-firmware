
#ifndef __FPROTO_LINEAR_H__
#define __FPROTO_LINEAR_H__

#include "subghzdbase.hpp"

typedef enum : uint8_t {
    LinearDecoderStepReset = 0,
    LinearDecoderStepSaveDuration,
    LinearDecoderStepCheckDuration,
} LinearDecoderStep;

class FProtoSubGhzDLinear : public FProtoSubGhzDBase {
   public:
    FProtoSubGhzDLinear() {
        sensorType = FPS_LINEAR;
        te_short = 500;
        te_long = 1500;
        te_delta = 150;
        min_count_bit_for_found = 10;
    }

    void feed(bool level, uint32_t duration) {
        switch (parser_step) {
            case LinearDecoderStepReset:
                if ((!level) && (DURATION_DIFF(duration, te_short * 42) < te_delta * 20)) {
                    // Found header Linear
                    decode_data = 0;
                    decode_count_bit = 0;
                    parser_step = LinearDecoderStepSaveDuration;
                }
                break;
            case LinearDecoderStepSaveDuration:
                if (level) {
                    te_last = duration;
                    parser_step = LinearDecoderStepCheckDuration;
                } else {
                    parser_step = LinearDecoderStepReset;
                }
                break;
            case LinearDecoderStepCheckDuration:
                if (!level) {  // save interval
                    if (duration >= (te_short * 5)) {
                        parser_step = LinearDecoderStepReset;
                        // checking that the duration matches the guardtime
                        if ((DURATION_DIFF(duration, te_short * 42) > te_delta * 20)) {
                            break;
                        }
                        if (DURATION_DIFF(te_last, te_short) < te_delta) {
                            subghz_protocol_blocks_add_bit(0);
                        } else if (
                            DURATION_DIFF(te_last, te_long) <
                            te_delta) {
                            subghz_protocol_blocks_add_bit(1);
                        }
                        if (decode_count_bit == min_count_bit_for_found) {
                            data = decode_data;
                            data_count_bit = decode_count_bit;
                            if (callback) callback(this);
                        }
                        break;
                    }

                    if ((DURATION_DIFF(te_last, te_short) < te_delta) &&
                        (DURATION_DIFF(duration, te_long) < te_delta)) {
                        subghz_protocol_blocks_add_bit(0);
                        parser_step = LinearDecoderStepSaveDuration;
                    } else if (
                        (DURATION_DIFF(te_last, te_long) < te_delta) &&
                        (DURATION_DIFF(duration, te_short) < te_delta)) {
                        subghz_protocol_blocks_add_bit(1);
                        parser_step = LinearDecoderStepSaveDuration;
                    } else {
                        parser_step = LinearDecoderStepReset;
                    }

                } else {
                    parser_step = LinearDecoderStepReset;
                }
                break;
        }
    }
};

#endif
