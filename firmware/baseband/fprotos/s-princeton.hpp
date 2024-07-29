
#ifndef __FPROTO_PRINCETON_H__
#define __FPROTO_PRINCETON_H__

#include "subghzdbase.hpp"

typedef enum : uint8_t {
    PrincetonDecoderStepReset = 0,
    PrincetonDecoderStepSaveDuration,
    PrincetonDecoderStepCheckDuration,
} PrincetonDecoderStep;

class FProtoSubGhzDPrinceton : public FProtoSubGhzDBase {
   public:
    FProtoSubGhzDPrinceton() {
        sensorType = FPS_PRINCETON;
        te_short = 390;
        te_long = 1170;
        te_delta = 300;
        min_count_bit_for_found = 24;
    }

    void feed(bool level, uint32_t duration) {
        switch (parser_step) {
            case PrincetonDecoderStepReset:
                if ((!level) && (DURATION_DIFF(duration, te_short * 36) < te_delta * 36)) {
                    // Found Preambula
                    parser_step = PrincetonDecoderStepSaveDuration;
                    decode_data = 0;
                    decode_count_bit = 0;
                }
                break;
            case PrincetonDecoderStepSaveDuration:
                // save duration
                if (level) {
                    te_last = duration;
                    parser_step = PrincetonDecoderStepCheckDuration;
                }
                break;
            case PrincetonDecoderStepCheckDuration:
                if (!level) {
                    if (duration >= ((uint32_t)te_long * 2)) {
                        parser_step = PrincetonDecoderStepSaveDuration;
                        if (decode_count_bit == min_count_bit_for_found) {
                            data = decode_data;
                            data_count_bit = decode_count_bit;
                            if (callback) callback(this);
                        }
                        decode_data = 0;
                        decode_count_bit = 0;
                        break;
                    }

                    if ((DURATION_DIFF(te_last, te_short) < te_delta) &&
                        (DURATION_DIFF(duration, te_long) < te_delta * 3)) {
                        subghz_protocol_blocks_add_bit(0);
                        parser_step = PrincetonDecoderStepSaveDuration;
                    } else if (
                        (DURATION_DIFF(te_last, te_long) < te_delta * 3) &&
                        (DURATION_DIFF(duration, te_short) < te_delta)) {
                        subghz_protocol_blocks_add_bit(1);
                        parser_step = PrincetonDecoderStepSaveDuration;
                    } else {
                        parser_step = PrincetonDecoderStepReset;
                    }
                } else {
                    parser_step = PrincetonDecoderStepReset;
                }
                break;
        }
    }
};

#endif
